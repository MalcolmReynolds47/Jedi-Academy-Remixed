// Production native combat-point selection/controller with deterministic engine services.
#include "game/g_local.h"
#include "game/b_local.h"
#include "game/g_nav.h"
#include "game/g_navigator.h"
#include "game/g_functions.h"
#include "game/g_combat_points.h"
#include "memory_save.h"
#include <deque>
#include <map>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
gentity_t*NPC=nullptr;gNPC_t*NPCInfo=nullptr;int delayedShutDown=0;
static std::deque<std::string> strings;
static bool obstructed=false,sameRegion=true;static int nearestNode=7;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_FreeEntity(gentity_t*e){G_CombatPointForget(e);e->inuse=qfalse;}
char*vtos(const vec3_t){return const_cast<char*>("fixture");}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
void CalcEntitySpot(gentity_t*e,spot_t,vec3_t p){VectorCopy(e->currentOrigin,p);}
qboolean G_CheckInSolid(gentity_t*,qboolean){return qfalse;}
qboolean G_ClearLOS(gentity_t*,const vec3_t,gentity_t*){return qtrue;}
namespace NAV {
void SpawnedPoint(gentity_t*,EPointType){}
TNodeHandle GetNearestNode(const vec3_t&,TNodeHandle,TNodeHandle,int,bool){return nearestNode;}
bool InSameRegion(gentity_t*,const vec3_t&){return sameRegion;}
bool SafePathExists(const CVec3&,const CVec3&,const CVec3&,float){return sameRegion;}
}
static int combatNearestNode(const vec3_t&p){return NAV::GetNearestNode(p);}
#include "game/g_combat_points.inc"
#include "game/g_combat_point_ai.inc"
static gentity_t&Make(int n,float x){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString("point_combat");e.model=nullptr;VectorSet(e.currentOrigin,x,0,24);CHECK(G_CombatPointSpawn(e));return e;}
int main(){return RunTests("native combat-point AI, editable lifetime, reservations, save restoration and exhaustion",[]{
 globals.num_entities=100;gi.Printf=[](const char*,...){};gi.linkentity=[](gentity_t*){};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.inPVS=[](const vec3_t,const vec3_t){return qtrue;};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=1;t->startsolid=obstructed?qtrue:qfalse;VectorCopy(end,t->endpos);};
 // The original map spawner still owns its native point and deletes its entity.
 auto&map=g_entities[10];map.inuse=qtrue;map.s.number=10;VectorSet(map.s.origin,400,0,24);SP_point_combat(&map);CHECK(!map.inuse&&level.numCombatPoints==1);CP_FindCombatPointWaypoints();const auto authored=level.combatPoints[0];
 gNPC_t brain{};gclient_t client{};auto&actor=g_entities[1];actor.inuse=qtrue;actor.s.number=1;actor.NPC=&brain;actor.client=&client;brain.stats.visrange=1024;brain.combatPoint=brain.lastFailedCombatPoint=-1;NPC=&actor;NPCInfo=&brain;VectorSet(actor.currentOrigin,0,0,24);
 auto&goal=g_entities[2];goal.inuse=qtrue;brain.tempGoal=&goal;goal.combatPoint=-1;
 auto&e=Make(20,80);int slot=e.count;CHECK(slot==1&&G_CombatPointAvailable(slot)&&level.combatPoints[slot].waypoint==7);
 vec3_t enemy={100,0,24};CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_ANY,0)==slot);
 CHECK(NPC_SetCombatPoint(slot)&&brain.combatPoint==slot&&level.combatPoints[slot].occupied);CHECK(!NPC_ReserveCombatPoint(slot));
 CHECK(NPC_FreeCombatPoint(slot,qfalse));brain.combatPoint=-1;
 CHECK(G_CombatPointSet(e,{CPF_FLEE|CPF_SNIPE,true}));CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_FLEE,0)==slot);
 CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_DUCK,0)==-1);
 sameRegion=false;CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_FLEE|CP_HAS_ROUTE,0)==-1);sameRegion=true;
 CHECK(G_CombatPointSet(e,{CPF_INVESTIGATE,true}));CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_INVESTIGATE,0,0)==-1);CHECK(G_CombatPointSet(e,{0,true}));CHECK(NPC_FindCombatPoint(actor.currentOrigin,nullptr,enemy,CP_INVESTIGATE,0,0)==slot);
 // Movement revokes reservations and the old temporary goal before reuse.
 CHECK(NPC_SetCombatPoint(slot));brain.goalEntity=&goal;goal.combatPoint=slot;brain.lastFailedCombatPoint=slot;e.currentOrigin[0]=120;G_CombatPointFrame();
 CHECK(brain.combatPoint==-1&&brain.lastFailedCombatPoint==-1&&!brain.goalEntity&&goal.combatPoint==-1&&level.combatPoints[slot].origin[0]==120);
 CHECK(G_CombatPointSet(e,{CPF_SQUAD,true}));CHECK(NPC_FindSquadPoint(actor.currentOrigin)==slot);
 CHECK(G_CombatPointSet(e,{CPF_SQUAD,false}));CHECK(!G_CombatPointAvailable(slot)&&NPC_FindSquadPoint(actor.currentOrigin)==-1);
 CHECK(!G_CombatPointSet(e,{64,true}));CHECK(!NPC_ReserveCombatPoint(-1)&&!NPC_ReserveCombatPoint(level.numCombatPoints)&&!NPC_FreeCombatPoint(-1,qfalse)&&!NPC_SetCombatPoint(-1));
 nearestNode=0;e.currentOrigin[0]+=1;G_CombatPointFrame();CHECK(!G_CombatPointAvailable(slot));nearestNode=7;obstructed=true;CHECK(G_CombatPointSet(e,{CPF_FLEE,true}));CHECK(!G_CombatPointAvailable(slot));obstructed=false;level.time+=1001;G_CombatPointFrame();CHECK(G_CombatPointAvailable(slot));
 // Native entity serialization retains the stable index and settings. The
 // native table itself is rebuilt (it lies after LEVEL_LOCALS_T_SAVESTOP).
 MemorySave save;ojk::SavedGameHelper writer(&save);gentity_t stored{},loaded{};stored.count=e.count;stored.spawnflags=e.spawnflags;stored.svFlags=e.svFlags;VectorCopy(e.currentOrigin,stored.currentOrigin);stored.sg_export(writer);save.reset_buffer_offset();loaded.sg_import(writer);writer.ensure_all_data_read();CHECK(loaded.count==slot&&loaded.spawnflags==CPF_FLEE&&loaded.currentOrigin[0]==e.currentOrigin[0]);
 CHECK(NPC_SetCombatPoint(slot));brain.goalEntity=&goal;goal.combatPoint=slot;G_CombatPointReset();level.numCombatPoints=1;CHECK(G_CombatPointRestore());CHECK(G_CombatPointAvailable(slot)&&brain.combatPoint==slot&&level.combatPoints[slot].occupied);
 CHECK(!memcmp(&authored,&level.combatPoints[0],sizeof(authored)));
 auto&second=Make(21,200);const int secondSlot=second.count;second.count=slot;CHECK(!G_CombatPointRestore());CHECK(level.combatPoints[slot].occupied);second.count=secondSlot;
 second.count=0;CHECK(!G_CombatPointRestore()&&!memcmp(&authored,&level.combatPoints[0],sizeof(authored)));second.count=secondSlot;
 G_FreeEntity(&e);CHECK(brain.combatPoint==-1&&!brain.goalEntity&&!G_CombatPointAvailable(slot));auto&reuse=Make(20,320);CHECK(reuse.count==slot&&!level.combatPoints[slot].occupied);
 G_FreeEntity(&reuse);G_FreeEntity(&second);CHECK(G_CombatPointRestore()&&!G_CombatPointAvailable(slot));
 // No allocation or modification of an authored record at fixed exhaustion.
 level.numCombatPoints=MAX_COMBAT_POINTS;for(auto&p:level.combatPoints)p.flags=0;auto&full=g_entities[30];full.inuse=qtrue;full.s.number=30;full.classname=G_NewString("point_combat");CHECK(!G_CombatPointSpawn(full)&&level.numCombatPoints==MAX_COMBAT_POINTS);
 CHECK(delayedShutDown==0);
 });}
