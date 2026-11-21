// Owned combat points share native AI selection without changing authored nodes.
// count is the saved stable combat-point index; spawnflags/SVF_INACTIVE are properties.
// Included with a combatNearestNode(const vec3_t&) adapter to the native navigator.
#include "g_combat_points.h"
#include <array>
#include <cmath>
namespace {
constexpr int combatPortableFlag=0x40000000;
std::array<int,MAX_COMBAT_POINTS> combatOwners=[](){std::array<int,MAX_COMBAT_POINTS>a{};a.fill(-1);return a;}();
std::array<bool,MAX_COMBAT_POINTS> combatClear{};
std::array<int,MAX_COMBAT_POINTS> combatProbeTime{};
void combatRelease(int slot){
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!e.inuse||!e.NPC)continue;auto&n=*e.NPC;
  if(n.combatPoint==slot)n.combatPoint=-1;
  if(n.lastFailedCombatPoint==slot)n.lastFailedCombatPoint=-1;
  if(n.tempGoal&&n.tempGoal->combatPoint==slot){if(n.goalEntity==n.tempGoal)n.goalEntity=nullptr;n.tempGoal->combatPoint=-1;n.tempGoal->waypoint=WAYPOINT_NONE;}
 }
 level.combatPoints[slot].occupied=qfalse;
}
bool combatSlot(const gentity_t&e){return e.count>=0&&e.count<level.numCombatPoints&&e.count<MAX_COMBAT_POINTS&&combatOwners[e.count]==e.s.number;}
void combatRefresh(gentity_t&e,bool release){
 if(!combatSlot(e))return;const int slot=e.count;auto&point=level.combatPoints[slot];
 if(release)combatRelease(slot);combatProbeTime[slot]=level.time;
 VectorCopy(e.currentOrigin,point.origin);point.flags=combatPortableFlag|e.spawnflags;
 point.waypoint=combatNearestNode(e.currentOrigin);
 trace_t tr{};vec3_t low={-20,-20,-24},high={20,20,40};
 gi.trace(&tr,e.currentOrigin,low,high,e.currentOrigin,e.s.number,MASK_SOLID,(EG2_Collision)0,0);
 combatClear[slot]=!tr.startsolid&&!tr.allsolid&&point.waypoint!=WAYPOINT_NONE;
}
void combatEmpty(int slot){level.combatPoints[slot]={};level.combatPoints[slot].flags=combatPortableFlag;level.combatPoints[slot].waypoint=WAYPOINT_NONE;combatOwners[slot]=-1;combatClear[slot]=false;combatProbeTime[slot]=0;}
}
bool G_CombatPointPortable(const gentity_t&e){return e.inuse&&e.s.number>0&&e.s.number<ENTITYNUM_WORLD&&e.classname&&!strcmp(e.classname,"point_combat")&&e.model&&!strcmp(e.model,"@jar_combat_point");}
bool G_CombatPointRead(const gentity_t&e,jar::combatPoint::Settings&out){
 if(!G_CombatPointPortable(e))return false;jar::combatPoint::Settings s{e.spawnflags,!(e.svFlags&SVF_INACTIVE)};if(!jar::combatPoint::Valid(s))return false;out=s;return true;
}
bool G_CombatPointSet(gentity_t&e,const jar::combatPoint::Settings&s){
 if(!G_CombatPointPortable(e)||!combatSlot(e)||!jar::combatPoint::Valid(s))return false;
 const bool changed=e.spawnflags!=s.flags||bool(e.svFlags&SVF_INACTIVE)==s.enabled||!VectorCompare(e.currentOrigin,level.combatPoints[e.count].origin);
 e.spawnflags=s.flags;e.svFlags=s.enabled?e.svFlags&~SVF_INACTIVE:e.svFlags|SVF_INACTIVE;
 combatRefresh(e,changed);return true;
}
bool G_CombatPointSpawn(gentity_t&e){
 if(!e.inuse||e.s.number<=0||e.s.number>=ENTITYNUM_WORLD||!e.classname||strcmp(e.classname,"point_combat"))return false;
 int slot=-1;for(int i=0;i<level.numCombatPoints;++i)if((level.combatPoints[i].flags&combatPortableFlag)&&combatOwners[i]<0){slot=i;break;}
 if(slot<0){if(level.numCombatPoints>=MAX_COMBAT_POINTS)return false;slot=level.numCombatPoints++;}
 combatEmpty(slot);combatOwners[slot]=e.s.number;e.count=slot;e.spawnflags=0;e.svFlags&=~SVF_INACTIVE;
 if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);e.model=G_NewString("@jar_combat_point");
 e.s.eType=ET_GENERAL;e.contents=CONTENTS_ITEM;e.takedamage=qfalse;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;e.e_UseFunc=useF_marker_use;
 VectorSet(e.mins,-20,-20,-24);VectorSet(e.maxs,20,20,40);combatRefresh(e,false);return true;
}
bool G_CombatPointAvailable(int slot){
 if(slot<0||slot>=level.numCombatPoints||slot>=MAX_COMBAT_POINTS)return false;
 if(!(level.combatPoints[slot].flags&combatPortableFlag))return true;
 const int n=combatOwners[slot];if(n<=0||n>=globals.num_entities)return false;const auto&e=g_entities[n];
 return G_CombatPointPortable(e)&&e.count==slot&&!(e.svFlags&SVF_INACTIVE)&&combatClear[slot];
}
const char*G_CombatPointStatus(const gentity_t&e){
 if(!combatSlot(e))return "Unregistered";
 if(e.svFlags&SVF_INACTIVE)return "Disabled";
 if(level.combatPoints[e.count].waypoint==WAYPOINT_NONE)return "No nearby navigation node";
 if(!combatClear[e.count])return "Location blocked";
 return level.combatPoints[e.count].occupied?"Reserved by an NPC":"Available to native AI";
}
void G_CombatPointForget(gentity_t*e){
 if(!e)return;for(int slot=0;slot<level.numCombatPoints;++slot)if(combatOwners[slot]==e->s.number){combatRelease(slot);combatEmpty(slot);}
}
void G_CombatPointReset(){combatOwners.fill(-1);combatClear.fill(false);combatProbeTime.fill(0);}
void G_CombatPointFrame(){
 for(int slot=0;slot<level.numCombatPoints;++slot){int n=combatOwners[slot];if(n<0)continue;auto&e=g_entities[n];
  if(!G_CombatPointPortable(e)||e.count!=slot){combatRelease(slot);combatEmpty(slot);continue;}
  const auto&point=level.combatPoints[slot];
  if(!VectorCompare(point.origin,e.currentOrigin)||point.flags!=(combatPortableFlag|e.spawnflags))combatRefresh(e,true);
  else if(level.time<combatProbeTime[slot]||level.time-combatProbeTime[slot]>=1000)combatRefresh(e,false);
  if(!G_CombatPointAvailable(slot)&&point.occupied)combatRelease(slot);
 }
}
bool G_CombatPointRestore(){
 // Validate all saved indices before changing the native table. A quick-load
 // can retain previous portable holes; full loads begin with authored points.
 std::array<int,MAX_COMBAT_POINTS> restored{};restored.fill(-1);
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_CombatPointPortable(e))continue;jar::combatPoint::Settings s;
  if(!G_CombatPointRead(e,s)||e.count<0||e.count>=MAX_COMBAT_POINTS||restored[e.count]>=0||
     (e.count<level.numCombatPoints&&!(level.combatPoints[e.count].flags&combatPortableFlag)))return false;
  for(float x:e.currentOrigin)if(!std::isfinite(x)||std::fabs(x)>65535)return false;
  restored[e.count]=i;
 }
 G_CombatPointReset();
 for(int slot=0;slot<MAX_COMBAT_POINTS;++slot){
  if(slot<level.numCombatPoints&&(level.combatPoints[slot].flags&combatPortableFlag))combatEmpty(slot);
  if(restored[slot]<0)continue;
  while(level.numCombatPoints<=slot)combatEmpty(level.numCombatPoints++);
  combatOwners[slot]=restored[slot];combatRefresh(g_entities[restored[slot]],false);
 }
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!e.inuse||!e.NPC)continue;int slot=e.NPC->combatPoint;
  if(slot<0||slot>=MAX_COMBAT_POINTS)continue;
  if(slot<level.numCombatPoints&&!(level.combatPoints[slot].flags&combatPortableFlag))continue;
  if(G_CombatPointAvailable(slot)&&!level.combatPoints[slot].occupied)level.combatPoints[slot].occupied=qtrue;
  else{e.NPC->combatPoint=-1;if(e.NPC->tempGoal&&e.NPC->tempGoal->combatPoint==slot){if(e.NPC->goalEntity==e.NPC->tempGoal)e.NPC->goalEntity=nullptr;e.NPC->tempGoal->combatPoint=-1;}}
 }
 return true;
}
