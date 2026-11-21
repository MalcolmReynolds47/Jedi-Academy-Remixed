// Owned retail stone brushes with native weapon damage. GPL-2.0-only.
#include "jar_mp_section_city_event.h"

static gentity_t *BreakEvent(const vec3_t at, int event);

static int SectionCityRoot(int n) {
    if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !g_entities[n].inuse ||
        sectionRole[n] != JAR_MP_SECTION_BREAKABLE) return ENTITYNUM_NONE;
    const int root = sectionParent[n] - 1;
    return SectionRoot(root) ? root : ENTITYNUM_NONE;
}

static qboolean SectionCityLive(const gentity_t *child) {
    return child->s.generic1 == JAR_MP_CITY_BRUSH_MARKER && child->r.bmodel &&
        (child->r.contents & CONTENTS_SOLID) && SectionCityRoot(child->s.number) != ENTITYNUM_NONE;
}

qboolean JAR_MP_SectionCityDamageBlocked(const gentity_t *target) {
    if (!target || target->s.generic1 != JAR_MP_CITY_BRUSH_MARKER) return qfalse;
    const int root = SectionCityRoot(target->s.number);
    return root != ENTITYNUM_NONE && (!SectionCityLive(target) || !SectionActive(root));
}

static void SectionCityCenter(const gentity_t *child, vec3_t center) {
    vec3_t middle;
    VectorAdd(child->r.mins, child->r.maxs, middle);
    VectorScale(middle, .5f, middle);
    SectionOffset(child->r.currentAngles, middle, center);
    VectorAdd(child->r.currentOrigin, center, center);
}

qboolean JAR_MP_SectionCityDamageCenter(const gentity_t *target, vec3_t center) {
    if (!SectionCityLive(target)) return qfalse;
    SectionCityCenter(target, center);
    return qtrue;
}

qboolean JAR_MP_SectionCityDamageDistance(const gentity_t *target, const vec3_t origin, float *distance) {
    if (!SectionCityLive(target)) return qfalse;
    vec3_t axis[3], offset, gap;
    AnglesToAxis(target->r.currentAngles, axis);
    VectorSubtract(origin, target->r.currentOrigin, offset);
    for (int i = 0; i < 3; ++i) {
        const float coordinate = DotProduct(offset, axis[i]);
        gap[i] = coordinate - Com_Clamp(target->r.mins[i], target->r.maxs[i], coordinate);
    }
    *distance = VectorLength(gap);
    return qtrue;
}

static void SectionCityDebris(gentity_t *child, gentity_t *attacker, qboolean destroyed) {
    vec3_t center, size, direction = {0, 0, 1};
    SectionCityCenter(child, center);
    gentity_t *event = BreakEvent(center, EV_DEBRIS);
    if (!event) return;

    VectorSubtract(child->r.maxs, child->r.mins, size);
    const int chunks = destroyed ? Q_irand(18, 24) : Q_irand(1, 3);
    const float scale = destroyed ? sqrtf(sqrtf(fmaxf(0, size[0] * size[1] * size[2]))) * 1.75f / chunks :
        VectorLength(size) / 100.0f;
    if (attacker && attacker->client) {
        VectorSubtract(center, attacker->r.currentOrigin, direction);
        if (!destroyed) VectorInverse(direction);
        VectorNormalize(direction);
    }
    event->s.generic1 = JAR_MP_CITY_BRUSH_MARKER;
    event->s.owner = event->s.number;
    VectorCopy(center, event->s.origin);
    VectorCopy(direction, event->s.angles);
    VectorCopy(child->r.maxs, event->s.origin2);
    VectorCopy(child->r.mins, event->s.angles2);
    VectorCopy(child->r.currentAngles, event->s.apos.trDelta);
    event->s.speed = 300;
    event->s.eventParm = chunks;
    event->s.trickedentindex = child->material;
    event->s.apos.trBase[0] = scale;
}

static void SectionCityHealth(gentity_t *child, int health) {
    const qboolean wasLive = child->r.bmodel;
    child->health = health;
    child->takedamage = health > 0;
    if (health > 0) {
        child->r.bmodel = qtrue;
        child->r.contents = child->genericValue3;
        child->clipmask = MASK_PLAYERSOLID;
        child->s.modelindex = child->genericValue5;
        child->s.eFlags &= ~EF_NODRAW;
        child->r.svFlags &= ~SVF_NOCLIENT;
        const int root = SectionCityRoot(child->s.number);
        if (!wasLive && root != ENTITYNUM_NONE && g_entities[root].r.linked)
            trap->LinkEntity((sharedEntity_t *)child);
        return;
    }

    if (child->r.linked) trap->UnlinkEntity((sharedEntity_t *)child);
    child->r.bmodel = qfalse;
    child->r.contents = child->clipmask = child->s.solid = child->s.modelindex = 0;
    child->s.eFlags |= EF_NODRAW;
    child->r.svFlags |= SVF_NOCLIENT;
    if (wasLive) PhysicsForget(child->s.number);
}

static void SectionCityDie(gentity_t *child, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
    if (!SectionCityLive(child)) return;
    SectionCityHealth(child, 0);
    SectionCityDebris(child, attacker, qtrue);
    if (child->noise_index) {
        vec3_t center;
        SectionCityCenter(child, center);
        gentity_t *event = BreakEvent(center, EV_GENERAL_SOUND);
        if (event) {
            event->s.eventParm = child->noise_index;
            event->s.saberEntityNum = CHAN_AUTO;
        }
    }
}

static void SectionCityPain(gentity_t *child, gentity_t *attacker, int damage) {
    if (damage > 0 && SectionCityLive(child)) SectionCityDebris(child, attacker, qfalse);
}

static qboolean SectionCityInit(gentity_t *child, int root, int first, const jarMPSectionChild_t *record) {
    const int local = record->inlineModel;
    const qboolean saberOnly = (record->spawnflags & 16) != 0;
    if (local < 1 || local > JAR_MP_CITY_BRUSHES || record->health < 1 || record->health > (saberOnly ? 10 : 3))
        return qfalse;
    const int model = first + local;
    char reference[16];
    Com_sprintf(reference, sizeof(reference), "*%d", model);
    trap->SetBrushModel((sharedEntity_t *)child, reference);
    if (!child->r.bmodel || child->s.modelindex != model || !(child->r.contents & CONTENTS_SOLID))
        return qfalse;

    child->classname = "jar_section_breakable";
    child->s.eType = ET_MOVER;
    child->s.generic1 = JAR_MP_CITY_BRUSH_MARKER;
    child->s.otherEntityNum = root;
    child->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
    child->flags |= FL_BBRUSH;
    child->spawnflags = record->spawnflags;
    child->material = saberOnly ? MAT_METAL : MAT_LT_STONE;
    if (saberOnly) {
        child->flags |= FL_DMG_BY_SABER_ONLY;
        child->noise_index = G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
    }
    child->genericValue1 = local;
    child->genericValue2 = record->health;
    child->genericValue3 = child->r.contents;
    child->genericValue4 = 1; // Native G_Damage distinguishes breakable movers by this field.
    child->genericValue5 = model;
    child->die = SectionCityDie;
    child->pain = SectionCityPain;
    SectionCityHealth(child, record->health);
    return qtrue;
}

static void SectionCityCapture(int root, byte damage[JAR_MP_CITY_DAMAGE_BYTES]) {
    memset(damage, 0, JAR_MP_CITY_DAMAGE_BYTES);
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] != JAR_MP_SECTION_BREAKABLE) continue;
        const gentity_t *child = &g_entities[n];
        const int maximum = child->genericValue2;
        const int health = child->health < 0 ? 0 : child->health > maximum ? maximum : child->health;
        SectionCitySetDamage(damage, child->genericValue1, maximum - health);
    }
}

static void SectionCityRestore(int root, const byte damage[JAR_MP_CITY_DAMAGE_BYTES]) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] != JAR_MP_SECTION_BREAKABLE) continue;
        gentity_t *child = &g_entities[n];
        const int health = child->genericValue2 - SectionCityDamageAt(damage, child->genericValue1);
        SectionCityHealth(child, health);
    }
}

static void SectionYavinCapture(int root, byte *state) {
    memset(state, 0, YAVIN_STATE_BYTES);
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] != JAR_MP_SECTION_BREAKABLE) continue;
        const gentity_t *child = &g_entities[n];
        const int index = child->genericValue1 - 1;
        const int damage = child->genericValue2 - Com_Clampi(0, child->genericValue2, child->health);
        state[index / 2] |= damage << (4 * (index % 2));
    }
    const gentity_t *timer = SectionDustTimer(root);
    if (timer) SectionSentinelWriteTime(state + JAR_MP_YAVIN_DAMAGE_BYTES, SectionSentinelRemaining(timer->nextthink));
}

static void SectionYavinRestore(int root, const byte *state) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] != JAR_MP_SECTION_BREAKABLE) continue;
        gentity_t *child = &g_entities[n];
        SectionCityHealth(child, child->genericValue2 - SectionYavinDamageAt(state, child->genericValue1 - 1));
    }
    gentity_t *timer = SectionDustTimer(root);
    const int remaining = SectionSentinelTime(state + JAR_MP_YAVIN_DAMAGE_BYTES);
    if (timer && remaining) timer->nextthink = level.time + remaining;
}
