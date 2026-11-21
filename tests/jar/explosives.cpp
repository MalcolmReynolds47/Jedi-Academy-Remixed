#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_vehicles.h"
#include "test_support.h"
#include <memory>
#include <algorithm>
#include <vector>

game_import_t gi = {};
level_locals_t level = {};
static std::vector<gentity_t*> targets;
static int damageDepth, maxDamageDepth, effects, triggers;
static gentity_t *expectedAttacker;
IGhoul2InfoArray &_TheGhoul2InfoArray() { throw std::runtime_error("unexpected model allocation"); }

static int EntitiesInBox(const vec3_t, const vec3_t, gentity_t **list, int limit)
{
    CHECK(targets.size() <= size_t(limit));
    std::copy(targets.begin(), targets.end(), list);
    return int(targets.size());
}
qboolean CanDamage(gentity_t *, const vec3_t) { return qtrue; }
Vehicle_t *G_IsRidingVehicle(gentity_t *) { return nullptr; }
void G_GetMassAndVelocityForEnt(gentity_t *, float *, vec3_t) {}
void Boba_DustFallNear(const vec3_t, int) {}
void G_PlayEffect(int, const vec3_t, const vec3_t) { ++effects; }
void G_SetOrigin(gentity_t *, const vec3_t) {}
void G_UseTargets(gentity_t *, gentity_t *activator) { CHECK(activator == expectedAttacker); ++triggers; }

// Substitute damage application while running the real blast enumeration and
// explosive death/think functions. Engine collision, scripts and FX are stubbed.
void G_Damage(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker,
    const vec3_t, const vec3_t, int damage, int flags, int mod, int hitLoc)
{
    ++damageDepth;
    maxDamageDepth = std::max(maxDamageDepth, damageDepth);
    CHECK(damageDepth == 1); // fail the old recursive chain without crashing the test host
    CHECK(attacker == expectedAttacker);
    ent->health -= damage;
    if (ent->health <= 0) WP_ExplosiveDie(ent, inflictor, attacker, damage, mod, flags, hitLoc);
    --damageDepth;
}

int main()
{
    return RunTests("1,024 explosive chain reactions and next-tick damage/FX/target completion", [] {
        gi.EntitiesInBox = EntitiesInBox;
        const int count = 1024;
        auto packs = std::make_unique<gentity_t[]>(count);
        gentity_t owner = {};
        owner.s.number = 1;
        expectedAttacker = &owner;
        targets.clear();
        level.time = 100;
        for (int i = 0; i < count; ++i) {
            auto &pack = packs[i];
            pack.s.number = i + 2;
            pack.inuse = qtrue;
            pack.takedamage = qtrue;
            pack.contents = CONTENTS_SHOTCLIP;
            pack.health = 25;
            pack.fxID = 1;
            pack.splashDamage = 100;
            pack.splashRadius = 100;
            pack.activator = &owner;
            pack.target = const_cast<char*>("exploded");
            pack.s.eFlags = EF_FIRING;
            targets.push_back(&pack);
        }
        // One explosion hits all remaining packs; none may recurse into another blast.
        WP_Explode(&packs[0]);
        CHECK(maxDamageDepth == 1 && effects == 1 && triggers == 1);
        for (int i = 1; i < count; ++i) {
            CHECK(!packs[i].takedamage && !(packs[i].s.eFlags & EF_FIRING));
            CHECK(packs[i].e_ThinkFunc == thinkF_WP_Explode && packs[i].nextthink == 101);
        }
        level.time = 101;
        for (int i = 1; i < count; ++i) WP_Explode(&packs[i]);
        CHECK(effects == count && triggers == count && maxDamageDepth == 1);
        for (int i = 0; i < count; ++i) CHECK(packs[i].e_ThinkFunc == thinkF_G_FreeEntity);

        // Player-shot explosives still apply the original one-third blast rule.
        owner.s.number = 0;
        auto &shot = packs[1];
        shot.takedamage = qtrue;
        shot.splashDamage = 90;
        shot.splashRadius = 150;
        WP_ExplosiveDie(&shot, &owner, &owner, 25, MOD_BLASTER, 0, HL_NONE);
        CHECK(shot.splashDamage == 30 && shot.splashRadius == 50);
        CHECK(shot.enemy == &owner && !shot.takedamage);
        CHECK(shot.nextthink == level.time + 1 && shot.e_ThinkFunc == thinkF_WP_Explode);
        CHECK(effects == count); // damage death schedules rather than performing another blast
    });
}
