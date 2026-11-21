// Exercise the game's actual navigation functions with a known waypoint graph.
#include <stdlib.h>
#define MAX_GROUP_MEMBERS 32
#define ENTITYNUM_NONE 99
#define WAYPOINT_NONE (-1)
#define Q3_INFINITE 9999999
typedef struct { int wp; struct { int currentOrigin; } r; } gentity_t;
typedef struct { int number, waypoint, pathCostToEnemy, closestBuddy; } AIGroupMember_t;
typedef struct { gentity_t *enemy; int enemyWP, numGroup; AIGroupMember_t member[MAX_GROUP_MEMBERS]; } AIGroupInfo_t;
static gentity_t g_entities[MAX_GROUP_MEMBERS+1];
static int DistanceSquared(int a,int b) { return (a-b)*(a-b); }
static int NAV_FindClosestWaypointForEnt(gentity_t *e,int unused) { (void)unused; return e->wp; }
static int Cost(int a,int b) { return abs(a-b); }
static struct { int (*Nav_GetPathCost)(int,int); } api={Cost}, *trap=&api;
#include "../../codemp/game/g_jar_squad_navigation.inc"

static int check(const int wp[5],const int order[5],int enemy) {
    AIGroupInfo_t group={0}; gentity_t target={50};
    group.numGroup=5; group.enemy=enemy?&target:NULL;
    for(int i=0;i<5;i++) {
        g_entities[i].wp=wp[i]; g_entities[i].r.currentOrigin=i;
        group.member[i].number=i;
    }
    AI_SetClosestBuddy(&group);
    AI_SortGroupByPathCostToEnemy(&group);
    for(int i=0;i<5;i++) {
        AIGroupMember_t row=group.member[i]; int id=order[i];
        if(row.number!=id || row.waypoint!=(enemy?wp[id]:WAYPOINT_NONE) ||
           row.pathCostToEnemy!=((enemy&&wp[id]!=WAYPOINT_NONE)?abs(50-wp[id]):Q3_INFINITE) ||
           row.closestBuddy!=(id==0?1:id-1)) return 1;
    }
    return 0;
}
int main(void) {
    const int initial[]={0,1,2,3,4}, reversed[]={4,3,2,1,0}, mixed[]={3,1,2,0,4};
    const int asc[]={50,40,30,20,10}, desc[]={10,20,30,40,50}, tied[]={-1,45,55,50,-1};
    return check(asc,initial,1)||check(desc,reversed,1)||check(tied,mixed,1)||check(desc,initial,0);
}
