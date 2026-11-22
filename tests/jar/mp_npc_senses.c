// Exercise the multiplayer alert queue and selectors with a controlled map trace.
#include "game/b_local.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

gentity_t g_entities[MAX_GENTITIES];
level_locals_t level;
int eventClearTime;
static gameImport_t imports;
gameImport_t *trap = &imports;
// Unrelated search, map-script and fleeing entry points in NPC_senses.c.
npcStatic_t NPCS;
void (*Com_Printf)(const char *, ...);
char *G_NewString(const char *s) { abort(); }
void G_UseTargets2(gentity_t *e, gentity_t *a, const char *s) { abort(); }
void G_FreeEntity(gentity_t *e) { abort(); }
void NPC_StartFlee(gentity_t *e, vec3_t p, int d, int a, int b) { abort(); }
qboolean CanShoot(gentity_t *e, gentity_t *s) { abort(); }
qboolean EntIsGlass(gentity_t *e) { abort(); }
qboolean ShotThroughGlass(trace_t *t, gentity_t *e, vec3_t p, int mask) { abort(); }
static qboolean wall;
static int traces;

void CalcEntitySpot(const gentity_t *ent, const spot_t spot, vec3_t point)
{
    (void)spot;
    VectorCopy(ent->r.currentOrigin, point);
}

static void mapTrace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                     const vec3_t end, int pass, int mask, int capsule, int flags, int lod)
{
    (void)start; (void)mins; (void)maxs; (void)end; (void)pass;
    (void)mask; (void)capsule; (void)flags; (void)lod;
    ++traces;
    *result = (trace_t){0};
    result->fraction = wall ? 0.5f : 1.0f;
    result->entityNum = ENTITYNUM_WORLD;
}

static int sense(gentity_t *listener, qboolean sight, qboolean sound, int ignore)
{
    return G_CheckAlertEvents(listener, sight, sound, 256, 128, ignore, qtrue, AEL_MINOR);
}

int main(void)
{
    static gclient_t client;
    gentity_t *listener = &g_entities[64], *player = &g_entities[1];
    listener->client = &client;
    imports.Trace = mapTrace;
    vec3_t front = {64, 0, 0}, rear = {-64, 0, 0};

    // An empty or dead first player must not change another listener's senses.
    assert(!g_entities[0].inuse && !g_entities[0].health);
    assert(sense(listener, qtrue, qtrue, -1) == -1);
    AddSoundEvent(player, rear, 100, AEL_DISCOVERED, qfalse);
    int id = level.alertEvents[0].ID;
    assert(id > 0 && sense(listener, qfalse, qtrue, -1) == 0);
    assert(sense(listener, qtrue, qfalse, -1) == -1 && !traces);
    AddSightEvent(player, front, 180, AEL_DISCOVERED, 20);
    assert(sense(listener, qtrue, qfalse, -1) == 1);
    assert(sense(listener, qtrue, qtrue, -1) == 0);
    assert(sense(listener, qtrue, qtrue, id) == 1);
    assert(sense(listener, qfalse, qfalse, -1) == -1);
    wall = qtrue;
    assert(sense(listener, qtrue, qfalse, -1) == -1);
    AddSoundEvent(player, front, 100, AEL_DANGER, qtrue);
    assert(sense(listener, qfalse, qtrue, -1) == 0);
    wall = qfalse;
    assert(sense(listener, qfalse, qtrue, -1) == 2);
    assert(G_CheckAlertEvents(listener, qtrue, qtrue, 20, 20, -1, qtrue, AEL_MINOR) == -1);
    level.time = 150;
    AddSightEvent(player, rear, 256, AEL_DISCOVERED, 20);
    level.time = ALERT_CLEAR_TIME + 1;
    ClearPlayerAlertEvents();
    assert(level.numAlertEvents == 1 && sense(listener, qtrue, qtrue, -1) == -1);

    // Equal-timestamp bursts, disposal at capacity, and ignoring by stable
    // ID even after earlier events disappear.
    G_ForgetAlertOwner(player);
    assert(!level.numAlertEvents);
    for (int i = 0; i < MAX_ALERT_EVENTS + 3; ++i)
        AddSoundEvent(i % 2 ? player : listener, front, 180, AEL_DISCOVERED, qfalse);
    assert(level.numAlertEvents == MAX_ALERT_EVENTS);
    int newest = level.curAlertID;
    assert(level.alertEvents[0].ID == newest - MAX_ALERT_EVENTS + 1);
    AddSoundEvent(NULL, front, 100, AEL_MINOR, qfalse);
    assert(level.curAlertID == newest && level.numAlertEvents == MAX_ALERT_EVENTS);
    G_ForgetAlertOwner(player);
    assert(level.numAlertEvents == MAX_ALERT_EVENTS / 2);
    for (int i = 0; i < level.numAlertEvents; ++i)
        assert(level.alertEvents[i].owner == listener && (!i || level.alertEvents[i].ID > level.alertEvents[i-1].ID));
    int last = level.numAlertEvents - 1;
    assert(sense(listener, qfalse, qtrue, level.alertEvents[last].ID) == last - 1);
    level.time += ALERT_CLEAR_TIME + 1;
    ClearPlayerAlertEvents();
    assert(!level.numAlertEvents);
    puts("PASS multiplayer independent sight, hearing, priority, occlusion, expiry, capacity and owner lifetime");
    return 0;
}
