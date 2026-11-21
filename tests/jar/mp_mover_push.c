// Production MP mover and trajectory against deterministic collision imports.
// GPL-2.0-only.
#include "game/g_local.h"
#include "game/g_jar_sandbox.h"
#include <assert.h>

enum { MOVER_SLOT = MAX_CLIENTS, PROP_SLOT = MAX_CLIENTS + 1, PERIOD = 3000 };
static gameImport_t imports;
gameImport_t *trap = &imports;
gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;

static gclient_t clients[3];
static int candidates[4], candidateCount;
static qboolean portable, protectedSlots[MAX_GENTITIES], blockedSlots[MAX_GENTITIES];
static int damageCalls, freeCalls, thinkCalls;

static void Error(int code, const char *format, ...) {
    abort();
}
void (*Com_Error)(int code, const char *format, ...) = Error;

// Unused shared-game services required when linking its native trajectory code.
static void Print(const char *format, ...) { abort(); }
void (*Com_Printf)(const char *format, ...) = Print;
char * QDECL va(const char *format, ...) { abort(); return NULL; }
weaponData_t weaponData[WP_NUM_WEAPONS];
ammoData_t ammoData[AMMO_MAX];
vmCvar_t g_fixWeaponAttackAnim;

static void Link(sharedEntity_t *ent) {
    ent->r.linked = qtrue;
    for (int i = 0; i < 3; ++i) {
        ent->r.absmin[i] = ent->r.currentOrigin[i] + ent->r.mins[i] - 1;
        ent->r.absmax[i] = ent->r.currentOrigin[i] + ent->r.maxs[i] + 1;
    }
}

static void Unlink(sharedEntity_t *ent) {
    ent->r.linked = qfalse;
}

static int EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int capacity) {
    assert(!g_entities[MOVER_SLOT].r.linked && capacity >= candidateCount);
    int count = 0;
    for (int i = 0; i < candidateCount; ++i) {
        const gentity_t *ent = &g_entities[candidates[i]];
        assert(ent->inuse && ent->r.linked);
        qboolean overlap = qtrue;
        for (int axis = 0; axis < 3; ++axis) {
            if (ent->r.absmin[axis] > maxs[axis] || ent->r.absmax[axis] < mins[axis]) overlap = qfalse;
        }
        if (overlap) list[count++] = ent->s.number;
    }
    return count;
}

static void Trace(trace_t *trace, const vec3_t start, const vec3_t mins,
                  const vec3_t maxs, const vec3_t end, int pass, int mask,
                  int capsule, int flags, int lod) {
    assert(pass >= 0 && pass < ENTITYNUM_WORLD && VectorCompare(start, end));
    memset(trace, 0, sizeof(*trace));
    trace->startsolid = blockedSlots[pass];
    trace->entityNum = ENTITYNUM_WORLD;
}

qboolean JAR_MP_SectionMoverProtected(const gentity_t *pusher, const gentity_t *target) {
    return portable && pusher == &g_entities[MOVER_SLOT] && (!target || protectedSlots[target->s.number]);
}

void G_Damage(gentity_t *target, gentity_t *inflictor, gentity_t *attacker,
              vec3_t direction, vec3_t point, int damage, int flags, int mod) {
    assert(inflictor == &g_entities[MOVER_SLOT] && attacker == inflictor);
    assert(!portable || !protectedSlots[target->s.number]);
    assert(mod == MOD_CRUSH && damage == 99999);
    ++damageCalls;
}

void G_FreeEntity(gentity_t *entity) {
    assert(!portable || !protectedSlots[entity->s.number]);
    ++freeCalls;
    entity->inuse = qfalse;
    Unlink((sharedEntity_t *)entity);
}

void NPC_RemoveBody(gentity_t *entity) {
    G_FreeEntity(entity);
}

void G_RunThink(gentity_t *entity) {
    assert(entity == &g_entities[MOVER_SLOT]);
    ++thinkCalls;
}

#include "game/g_jar_native_mover_push.inc"

static gentity_t *SetupMover(qboolean sectionMover) {
    memset(g_entities, 0, sizeof(g_entities));
    memset(clients, 0, sizeof(clients));
    memset(protectedSlots, 0, sizeof(protectedSlots));
    memset(blockedSlots, 0, sizeof(blockedSlots));
    candidateCount = damageCalls = freeCalls = thinkCalls = 0;
    portable = sectionMover;
    level.previousTime = 1000;
    level.time = 1750;

    gentity_t *mover = &g_entities[MOVER_SLOT];
    mover->inuse = qtrue;
    mover->s.number = MOVER_SLOT;
    mover->s.eType = ET_MOVER;
    mover->classname = "func_bobbing";
    mover->r.bmodel = qtrue;
    mover->r.contents = CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP;
    VectorSet(mover->r.mins, -16, -16, -8);
    VectorSet(mover->r.maxs, 16, 16, 8);
    VectorSet(mover->r.currentOrigin, 0, 0, 100);
    VectorCopy(mover->r.currentOrigin, mover->s.pos.trBase);
    mover->s.pos.trType = TR_SINE;
    mover->s.pos.trDuration = PERIOD;
    mover->s.pos.trTime = level.previousTime;
    mover->s.pos.trDelta[2] = 20;
    Link((sharedEntity_t *)mover);
    return mover;
}

static gentity_t *AddRider(int n, float x) {
    gentity_t *actor = &g_entities[n];
    actor->inuse = qtrue;
    actor->s.number = n;
    actor->s.eType = ET_PLAYER;
    actor->s.groundEntityNum = MOVER_SLOT;
    actor->client = &clients[n];
    actor->client->ps.groundEntityNum = MOVER_SLOT;
    actor->client->ps.delta_angles[YAW] = 123;
    actor->health = 100;
    actor->clipmask = MASK_PLAYERSOLID;
    VectorSet(actor->r.mins, -2, -2, -4);
    VectorSet(actor->r.maxs, 2, 2, 4);
    VectorSet(actor->r.currentOrigin, x, 0, 112);
    VectorSet(actor->s.pos.trBase, x, 0, 112.125f);
    VectorSet(actor->client->ps.origin, x, 0, 112.25f);
    VectorSet(actor->r.currentAngles, 5, 7, 9);
    VectorSet(actor->s.apos.trBase, 2, 4, 6);
    Link((sharedEntity_t *)actor);
    candidates[candidateCount++] = n;
    return actor;
}

static gentity_t *AddDroppedWeapon(void) {
    gentity_t *item = &g_entities[PROP_SLOT];
    item->inuse = item->physicsObject = item->takedamage = qtrue;
    item->health = 10;
    item->s.weapon = WP_THERMAL;
    item->s.eType = ET_ITEM;
    item->s.number = PROP_SLOT;
    item->s.groundEntityNum = MOVER_SLOT;
    item->s.eFlags = EF_DROPPEDWEAPON;
    item->r.ownerNum = 0;
    VectorSet(item->r.currentOrigin, 0, 0, 112);
    VectorCopy(item->r.currentOrigin, item->s.pos.trBase);
    VectorSet(item->r.mins, -2, -2, -4);
    VectorSet(item->r.maxs, 2, 2, 4);
    Link((sharedEntity_t *)item);
    candidates[candidateCount++] = PROP_SLOT;
    return item;
}

int main(void) {
    imports.Error = Error;
    imports.LinkEntity = Link;
    imports.UnlinkEntity = Unlink;
    imports.EntitiesInBox = EntitiesInBox;
    imports.Trace = Trace;

    gentity_t *mover = SetupMover(qtrue);
    gentity_t *rider = AddRider(0, 0);
    protectedSlots[0] = qtrue;
    // A room tilted by 90 degrees makes the stock local-Z path horizontal.
    mover->r.currentAngles[PITCH] = mover->s.apos.trBase[PITCH] = 90;
    VectorSet(mover->s.pos.trDelta, 20, 0, 0);
    G_RunMover(mover);
    assert(mover->r.currentOrigin[0] == 20 && mover->s.pos.trTime == 1000);
    assert(rider->client->ps.origin[0] == 20 && rider->r.currentOrigin[0] == 20);
    assert(rider->s.groundEntityNum == MOVER_SLOT && !damageCalls && thinkCalls == 1);

    mover = SetupMover(qtrue);
    // Put an allowed victim and weapon before protected contacts in broad-phase order.
    AddRider(2, -6);
    gentity_t *weapon = AddDroppedWeapon();
    rider = AddRider(0, 6);
    gentity_t *blocker = AddRider(1, 0);
    blockedSlots[2] = blockedSlots[1] = blockedSlots[PROP_SLOT] = qtrue;
    protectedSlots[0] = protectedSlots[1] = qtrue;
    const gentity_t saved = *rider;
    const playerState_t savedPlayer = rider->client->ps;
    G_RunMover(mover);
    assert(mover->r.linked && mover->r.currentOrigin[2] == 100 && mover->s.pos.trTime == 1750);
    assert(rider->r.linked && VectorCompare(rider->r.currentOrigin, saved.r.currentOrigin));
    assert(VectorCompare(rider->r.currentAngles, saved.r.currentAngles));
    assert(VectorCompare(rider->r.absmin, saved.r.absmin) && VectorCompare(rider->r.absmax, saved.r.absmax));
    assert(VectorCompare(rider->s.pos.trBase, saved.s.pos.trBase));
    assert(VectorCompare(rider->s.apos.trBase, saved.s.apos.trBase));
    assert(VectorCompare(rider->client->ps.origin, savedPlayer.origin));
    assert(rider->client->ps.delta_angles[YAW] == savedPlayer.delta_angles[YAW]);
    assert(rider->s.groundEntityNum == MOVER_SLOT && blocker->r.currentOrigin[2] == 112);
    assert(!damageCalls && !freeCalls && weapon->inuse && thinkCalls == 1);

    mover = SetupMover(qtrue);
    weapon = AddDroppedWeapon();
    blockedSlots[PROP_SLOT] = protectedSlots[PROP_SLOT] = qtrue;
    G_RunMover(mover);
    assert(!damageCalls && !freeCalls && weapon->inuse && weapon->r.linked);
    assert(mover->r.currentOrigin[2] == 100 && weapon->r.currentOrigin[2] == 112);

    // Existing authored movers and allowed Sandbox damage retain the native crush.
    for (int section = 0; section < 2; ++section) {
        mover = SetupMover(section);
        AddRider(0, 0);
        blockedSlots[0] = qtrue;
        protectedSlots[0] = !section;
        G_RunMover(mover);
        assert(damageCalls == 1 && !freeCalls && mover->r.currentOrigin[2] == 120);
        assert(mover->s.pos.trTime == 1000 && thinkCalls == 1);
    }

    puts("MP native mover: rotated rider motion, protected rollback, prop protection and stock crush passed.");
    return 0;
}
