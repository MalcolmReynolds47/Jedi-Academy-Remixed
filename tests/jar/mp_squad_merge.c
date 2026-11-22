// Run the MP merge loop with controlled patrol positions and entity slots.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_GROUP_MEMBERS 32
#define MAX_FRAME_GROUPS 4
typedef int qboolean;
enum { qfalse, qtrue };
typedef enum { neutral, ally, hostile } npcteam_t;
typedef struct {
    npcteam_t playerTeam;
} client_t;
typedef struct group_s AIGroupInfo_t;
typedef struct {
    int x, id;
    client_t *client;
    AIGroupInfo_t *owner;
} gentity_t;
typedef struct {
    int number;
} row_t;
struct group_s {
    int team, numGroup;
    gentity_t *enemy;
    row_t member[MAX_GROUP_MEMBERS];
};
static struct {
    AIGroupInfo_t groups[MAX_FRAME_GROUPS];
} level;
static gentity_t g_entities[128], target;
static client_t clients[128];
static int hidden;

static gentity_t *AI_LivingGroupMember(int id) {
    return id >= 0 && id < 128 && g_entities[id].client ? &g_entities[id] : NULL;
}
static int AI_ValidateNoEnemyGroupMember(AIGroupInfo_t *dst, gentity_t *m) {
    return m->id != hidden && abs(m->x - g_entities[dst->member[0].number].x) <= 384;
}
static void AI_DeleteGroupMember(AIGroupInfo_t *g, int j) {
    g_entities[g->member[j].number].owner = NULL;
    memmove(g->member + j, g->member + j + 1, (--g->numGroup - j) * sizeof(row_t));
}
static void AI_InsertGroupMember(AIGroupInfo_t *g, gentity_t *m) {
    if (g->numGroup >= MAX_GROUP_MEMBERS || m->owner)
        abort();
    g->member[g->numGroup++].number = m->id;
    m->owner = g;
}
#include "../../codemp/game/g_jar_squad_merge.inc"

static AIGroupInfo_t *group(int slot, npcteam_t team, gentity_t *enemy) {
    AIGroupInfo_t *g = &level.groups[slot];
    g->team = team;
    g->enemy = enemy;
    return g;
}
static void add(AIGroupInfo_t *g, int id, int x) {
    clients[id].playerTeam = (npcteam_t)g->team;
    g_entities[id] = (gentity_t){x, id, &clients[id], NULL};
    AI_InsertGroupMember(g, &g_entities[id]);
}
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                        \
            ++errors;                                                                              \
        }                                                                                          \
    } while (0)
int main(void) {
    int errors = 0;
    for (int combat = 0; combat <= 1; combat++) {
        memset(&level, 0, sizeof(level));
        AIGroupInfo_t *first = group(0, ally, combat ? &target : NULL);
        AIGroupInfo_t *second = group(1, hostile, first->enemy);
        add(first, 20, 100);
        add(second, 23, 100);
        CHECK(!AI_MergeIntoEarlierGroup(second));
        CHECK(first->numGroup == 1 && second->numGroup == 1 && g_entities[23].owner == second);
    }
    memset(&level, 0, sizeof(level));
    AIGroupInfo_t *a = group(0, ally, &target), *b = group(1, ally, &g_entities[40]);
    add(a, 20, 0);
    add(b, 21, 0);
    CHECK(!AI_MergeIntoEarlierGroup(b) && g_entities[21].owner == b);
    // An empty slot left by despawn must not acquire a patrol via entity zero.
    memset(&level, 0, sizeof(level));
    AIGroupInfo_t *src = group(2, ally, NULL);
    add(src, 24, 10);
    CHECK(!AI_MergeIntoEarlierGroup(src) && g_entities[24].owner == src);
    // An incompatible earlier patrol must not prevent a later valid match.
    // A distant ally stays behind and may join on the next refresh.
    memset(&level, 0, sizeof(level));
    AIGroupInfo_t *other = group(0, hostile, NULL), *dst = group(1, ally, NULL);
    src = group(2, ally, NULL);
    add(other, 22, 16);
    add(dst, 20, 16);
    add(src, 21, 1200);
    add(src, 23, 24);
    CHECK(!AI_MergeIntoEarlierGroup(src));
    CHECK(dst->numGroup == 2 && src->numGroup == 1 && g_entities[23].owner == dst &&
          g_entities[21].owner == src);
    g_entities[21].x = 24;
    hidden = 21;
    CHECK(!AI_MergeIntoEarlierGroup(src) && g_entities[21].owner == src);
    hidden = 0;
    CHECK(AI_MergeIntoEarlierGroup(src));
    CHECK(dst->numGroup == 3 && src->numGroup == 0 && other->numGroup == 1);
    // A member may change team before the periodic membership validation.
    memset(&level, 0, sizeof(level));
    dst = group(0, ally, &target);
    src = group(1, ally, &target);
    add(dst, 20, 0);
    add(src, 21, 0);
    add(src, 22, 0);
    clients[21].playerTeam = hostile;
    AI_PruneSquadRoster(src);
    CHECK(AI_MergeIntoEarlierGroup(src));
    CHECK(!g_entities[21].owner && g_entities[22].owner == dst);
    // Keep a combined roster within the same limit as ordinary admissions.
    memset(&level, 0, sizeof(level));
    dst = group(0, ally, &target);
    src = group(1, ally, &target);
    for (int i = 1; i < MAX_GROUP_MEMBERS - 1; i++)
        add(dst, i, 0);
    add(src, 50, 0);
    add(src, 51, 0);
    CHECK(!AI_MergeIntoEarlierGroup(src) && src->numGroup == 2);
    AI_DeleteGroupMember(src, 1);
    CHECK(AI_MergeIntoEarlierGroup(src) && dst->numGroup == MAX_GROUP_MEMBERS - 1);
    return errors != 0;
}
