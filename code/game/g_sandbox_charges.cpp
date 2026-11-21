// Native det packs and trip mines mounted on Sandbox props. GPL-2.0-only.
#include "g_local.h"
#include "g_functions.h"
#include "g_sandbox_charges.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"
#include "jar_physics.h"
#include "qcommon/jar_angular_trajectory.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <algorithm>
#include <climits>
#include <limits>
#include <map>
#include <set>

namespace {
struct ChargeMount {
 uint16_t prop;
 uint64_t chargeLife,propLife;
 vec3_t origin,angles,beamStart;
};
std::map<uint16_t,ChargeMount> mounts;
std::set<std::pair<uint16_t,uint16_t>> mountsByProp;

decltype(mounts)::iterator EraseMount(decltype(mounts)::iterator mount){
 mountsByProp.erase({mount->second.prop,mount->first});
 return mounts.erase(mount);
}

bool LiveSlot(const gentity_t& entity){
 const int slot=entity.s.number;
 return slot>=MAX_CLIENTS&&slot<ENTITYNUM_WORLD&&slot<globals.num_entities&&
  &entity==&g_entities[slot]&&entity.inuse;
}

bool Support(const gentity_t& entity){
 return LiveSlot(entity)&&!entity.client&&!entity.bmodel&&!entity.freeAfterEvent&&entity.s.eType==ET_GENERAL&&entity.classname&&
  !strcmp(entity.classname,"jar_sandbox_prop");
}

bool PlantedCharge(const gentity_t& entity){
 if(!LiveSlot(entity)||entity.s.eType!=ET_GENERAL||entity.s.pos.trType!=TR_STATIONARY||
    !(entity.s.eFlags&EF_MISSILE_STICK)||entity.e_TouchFunc!=touchF_NULL||
    entity.e_DieFunc!=dieF_WP_ExplosiveDie||entity.e_ThinkFunc==thinkF_G_FreeEntity||
    !entity.classname||entity.owner||entity.freeAfterEvent)return false;
 return (entity.s.weapon==WP_DET_PACK&&!strcmp(entity.classname,"detpack"))||
  (entity.s.weapon==WP_TRIP_MINE&&!strcmp(entity.classname,"tripmine"));
}

bool Finite(const vec3_t v,float limit){
 for(int axis=0;axis<3;++axis)if(!std::isfinite(v[axis])||std::fabs(v[axis])>=limit)return false;
 return true;
}

bool ValidChargeBounds(const gentity_t& charge){
 if(!Finite(charge.mins,65536)||!Finite(charge.maxs,65536))return false;
 for(int axis=0;axis<3;++axis)if(charge.mins[axis]>charge.maxs[axis])return false;
 return true;
}

bool ValidLocalFrame(const ChargeMount& mount){
 return Finite(mount.origin,65536)&&Finite(mount.angles,361)&&Finite(mount.beamStart,65536);
}

bool LiveMount(uint16_t slot,const ChargeMount& mount){
 const auto& charge=g_entities[slot];const auto& prop=g_entities[mount.prop];
 return PlantedCharge(charge)&&Support(prop)&&charge.s.groundEntityNum==mount.prop&&
  G_EntityLifetime(&charge)==mount.chargeLife&&G_EntityLifetime(&prop)==mount.propLife;
}

bool ChargeMode(const gentity_t& charge,jar::chargeCopy::Mode& mode){
 using jar::chargeCopy::Mode;
 const bool pack=charge.s.weapon==WP_DET_PACK;
 if(charge.e_ThinkFunc==thinkF_WP_Explode)mode=pack?Mode::DetonatingPack:Mode::DetonatingMine;
 else if(pack&&charge.e_ThinkFunc==thinkF_NULL)mode=Mode::DetPack;
 else if(!pack&&charge.e_ThinkFunc==thinkF_laserTrapThink)mode=Mode::Tripwire;
 else if(!pack&&charge.e_ThinkFunc==thinkF_WP_prox_mine_think)mode=Mode::Proximity;
 else return false;
 return true;
}

bool StandaloneCharge(const gentity_t& charge){
 // Older native world charges used the zero-initialized ground field.
 // Detached charges use NONE. A native mover or prop mount has a live slot.
 const int ground=charge.s.groundEntityNum;
 return PlantedCharge(charge)&&charge.activator==&g_entities[0]&&!charge.target&&
  (ground==0||ground==-1||ground==ENTITYNUM_NONE||ground==ENTITYNUM_WORLD);
}

bool CaptureState(const gentity_t& charge,jar::chargeCopy::State& state){
 using jar::chargeCopy::Mode;
 if(G_JAR_WorkshopControls(&charge)||!ChargeMode(charge,state.mode))return false;
 state.health=std::max(0,charge.health);state.shootable=charge.takedamage!=qfalse;
 state.damage=charge.damage;state.splash=charge.splashDamage;state.radius=charge.splashRadius;
 const auto remaining=[](int deadline,int minimum){return int(std::min<int64_t>(INT_MAX,std::max<int64_t>(minimum,int64_t(deadline)-level.time)));};
 state.pending=state.mode==Mode::DetPack?0:remaining(charge.nextthink,1);
 state.timeout=state.mode==Mode::Proximity?remaining(charge.delay,0):0;
 state.alternate=charge.methodOfDeath==MOD_LASERTRIP_ALT;
 state.armed=state.mode==Mode::Tripwire?(charge.s.eFlags&EF_FIRING)!=0:
  state.alternate&&(charge.s.eFlags&EF_PROX_TRIP)!=0;
 return true;
}

void Detach(uint16_t slot,const ChargeMount& mount){
 auto& charge=g_entities[slot];
 if(charge.inuse&&charge.s.groundEntityNum==mount.prop&&G_EntityLifetime(&charge)==mount.chargeLife)
  charge.s.groundEntityNum=ENTITYNUM_NONE;
}

void TransformPoint(const vec3_t local,const vec3_t origin,const vec3_t angles,vec3_t out){
 const vec3_t neutral={0,0,0};
 jar::angular::RebaseVector(local,neutral,angles,out);VectorAdd(out,origin,out);
}

bool Destination(gentity_t& charge,const ChargeMount& mount,const gentity_t& prop,const vec3_t origin,const vec3_t angles,
 SandboxChargeMove& move){
 move.charge=&charge;
 move.support=&prop;
 TransformPoint(mount.origin,origin,angles,move.origin);
 TransformPoint(mount.beamStart,origin,angles,move.beamStart);
 jar::angular::Compose(mount.angles,angles,move.angles);
 return Finite(move.origin,65536)&&Finite(move.angles,361)&&Finite(move.beamStart,65536);
}

void Place(const SandboxChargeMove& move,bool teleported){
 auto& charge=*move.charge;
 G_SetOrigin(&charge,move.origin);G_SetAngles(&charge,move.angles);
 VectorCopy(move.origin,charge.s.origin);
 if(charge.e_ThinkFunc==thinkF_laserTrapThink){
  AngleVectors(move.angles,charge.movedir,nullptr,nullptr);
  VectorCopy(move.beamStart,charge.s.origin2);
 }
 if(teleported)charge.s.eFlags^=EF_TELEPORT_BIT;
 if(charge.linked)gi.linkentity(&charge);
}

void UpdateBeam(gentity_t& charge){
 if(charge.e_ThinkFunc!=thinkF_laserTrapThink||!(charge.s.eFlags&EF_FIRING))return;
 trace_t hit{};G_SandboxChargeTraceBeam(charge,hit);VectorCopy(hit.endpos,charge.pos4);
}

bool PlanCharges(const gentity_t& prop,const vec3_t origin,const vec3_t angles,
 std::vector<SandboxChargeMove>& moves,size_t limit,bool teleporting){
 if(mountsByProp.empty()||!Support(prop))return true;
 const size_t previous=moves.size();
 const auto slot=uint16_t(prop.s.number);
 for(auto it=mountsByProp.lower_bound({slot,0});it!=mountsByProp.end()&&it->first==slot;++it){
  const auto chargeSlot=it->second;
  const auto& mount=mounts.at(chargeSlot);
  if(!LiveMount(chargeSlot,mount))continue;
  auto& charge=g_entities[chargeSlot];SandboxChargeMove move{};
  if(moves.size()>=limit||(teleporting&&(charge.svFlags&SVF_NO_TELEPORT))||G_JAR_WorkshopControls(&charge)||
     !Destination(charge,mount,prop,origin,angles,move)){
   moves.resize(previous);
   return false;
  }
  moves.push_back(move);
 }
 return true;
}
}

void G_SandboxChargeAttach(gentity_t& charge,const gentity_t& prop){
 if(!PlantedCharge(charge)||!Support(prop))return;
 ChargeMount mount{};mount.prop=uint16_t(prop.s.number);
 mount.chargeLife=G_EntityLifetime(&charge);mount.propLife=G_EntityLifetime(&prop);
 if(!mount.chargeLife||!mount.propLife)return;
 const vec3_t neutral={0,0,0};
 vec3_t delta;VectorSubtract(charge.currentOrigin,prop.currentOrigin,delta);
 jar::angular::RebaseVector(delta,prop.currentAngles,neutral,mount.origin);
 jar::angular::Relative(charge.currentAngles,prop.currentAngles,mount.angles);
 if(charge.e_ThinkFunc==thinkF_laserTrapThink){
  VectorSubtract(charge.s.origin2,prop.currentOrigin,delta);
  jar::angular::RebaseVector(delta,prop.currentAngles,neutral,mount.beamStart);
 }
 if(!ValidLocalFrame(mount))return;
 const auto chargeSlot=uint16_t(charge.s.number);
 const auto previous=mounts.find(chargeSlot);
 if(previous!=mounts.end())EraseMount(previous);
 mounts.emplace(chargeSlot,mount);
 mountsByProp.emplace(mount.prop,chargeSlot);
 charge.s.groundEntityNum=prop.s.number;
}

const gentity_t* G_SandboxChargeSupport(const gentity_t& charge){
 if(!PlantedCharge(charge))return nullptr;
 const auto found=mounts.find(uint16_t(charge.s.number));
 return found!=mounts.end()&&LiveMount(found->first,found->second)?&g_entities[found->second.prop]:nullptr;
}

bool G_SandboxChargeProperties(const gentity_t& charge,jar::chargeCopy::Properties& properties){
 using jar::chargeCopy::Mode;
 if((!G_SandboxChargeSupport(charge)&&!StandaloneCharge(charge))||!ChargeMode(charge,properties.mode))return false;
 properties.health=charge.health;
 properties.splash=charge.splashDamage;
 properties.radius=charge.splashRadius;
 properties.expiry=properties.mode==Mode::Proximity?charge.delay:0;
 return true;
}

bool G_SandboxChargeSetProperties(gentity_t& charge,const jar::chargeCopy::Properties& properties){
 using namespace jar::chargeCopy;
 Properties current;
 if(!G_SandboxChargeProperties(charge,current)||current.mode!=properties.mode||Detonating(current.mode)||
    G_JAR_WorkshopControls(&charge)||!charge.takedamage||current.health<=0||
    properties.health<1||properties.health>maxEditHealth||properties.splash<0||properties.splash>maxEditDamage||
    properties.radius<0||properties.radius>maxEditRadius||
    (current.mode==Mode::Proximity?(properties.expiry<=level.time||int64_t(properties.expiry)-level.time>maxEditTimeout):properties.expiry!=0))return false;
 charge.health=properties.health;
 charge.splashDamage=properties.splash;
 charge.splashRadius=properties.radius;
 if(current.mode==Mode::Proximity)charge.delay=properties.expiry;
 return true;
}

bool G_SandboxChargesCapture(const gentity_t& prop,std::vector<jar::chargeCopy::State>& charges){
 if(!Support(prop))return true;
 const auto slot=uint16_t(prop.s.number);
 for(auto it=mountsByProp.lower_bound({slot,0});it!=mountsByProp.end()&&it->first==slot;++it){
  const auto& mount=mounts.at(it->second);
  if(!LiveMount(it->second,mount))continue;
  const auto& charge=g_entities[it->second];
  jar::chargeCopy::State state;
  if(!CaptureState(charge,state))return false;
  VectorCopy(mount.origin,state.origin);VectorCopy(mount.angles,state.angles);VectorCopy(mount.beamStart,state.beamStart);
  if(!jar::chargeCopy::Valid(state))return false;
  charges.push_back(state);
 }
 return true;
}

bool G_SandboxChargeCaptureStandalone(const gentity_t& charge,jar::chargeCopy::State& state){
 if(!StandaloneCharge(charge))return false;
 state={};
 return CaptureState(charge,state)&&jar::chargeCopy::Valid(state);
}

bool G_SandboxChargeBindCopy(gentity_t& charge,const gentity_t& prop,const jar::chargeCopy::State& state){
 if(!PlantedCharge(charge)||!Support(prop))return false;
 ChargeMount mount{};mount.prop=uint16_t(prop.s.number);
 mount.chargeLife=G_EntityLifetime(&charge);mount.propLife=G_EntityLifetime(&prop);
 VectorCopy(state.origin,mount.origin);VectorCopy(state.angles,mount.angles);VectorCopy(state.beamStart,mount.beamStart);
 SandboxChargeMove move{};
 if(!mount.chargeLife||!mount.propLife||!ValidLocalFrame(mount)||
    !Destination(charge,mount,prop,prop.currentOrigin,prop.currentAngles,move)||!G_SandboxChargeFits(move))return false;
 const auto slot=uint16_t(charge.s.number);
 if(!mounts.emplace(slot,mount).second)return false;
 mountsByProp.emplace(mount.prop,slot);
 charge.s.groundEntityNum=prop.s.number;
 Place(move,false);
 return true;
}

void G_SandboxChargeRefreshBeam(gentity_t& charge){UpdateBeam(charge);}

void G_SandboxChargeTraceBeam(gentity_t& charge,trace_t& hit){
 const auto found=mounts.find(uint16_t(charge.s.number));
 gentity_t* support=nullptr;
 if(found!=mounts.end()&&LiveSlot(charge)&&LiveMount(found->first,found->second))support=&g_entities[found->second.prop];
 const bool linked=support&&support->linked;
 if(linked)gi.unlinkentity(support);
 vec3_t end;VectorMA(charge.s.pos.trBase,2048,charge.movedir,end);
 const vec3_t mins={-4,-4,-4},maxs={4,4,4};
 gi.trace(&hit,charge.s.origin2,mins,maxs,end,charge.s.number,MASK_SHOT,G2_RETURNONHIT,0);
 if(linked)gi.linkentity(support);
}

void G_SandboxChargesFrame(){
 for(auto it=mounts.begin();it!=mounts.end();){
  const auto slot=it->first;const auto& mount=it->second;
  auto& charge=g_entities[slot];const auto& prop=g_entities[mount.prop];
  SandboxChargeMove move{};
  if(!LiveMount(slot,mount)||G_JAR_WorkshopControls(&charge)||!Destination(charge,mount,prop,prop.currentOrigin,prop.currentAngles,move)){
   Detach(slot,mount);it=EraseMount(it);continue;
  }
  if(!VectorCompare(charge.currentOrigin,move.origin)||!VectorCompare(charge.currentAngles,move.angles)){
   Place(move,false);UpdateBeam(charge);
  }
  ++it;
 }
}

void G_SandboxChargesForget(gentity_t& entity){
 if(mounts.empty()||!LiveSlot(entity))return;
 const auto life=G_EntityLifetime(&entity);const auto slot=uint16_t(entity.s.number);
 const auto self=mounts.find(slot);
 if(self!=mounts.end())EraseMount(self);
 if(!Support(entity))return;
 for(auto it=mountsByProp.lower_bound({slot,0});it!=mountsByProp.end()&&it->first==slot;){
  const auto mount=mounts.find((it++)->second);
  if(mount->second.propLife==life){Detach(mount->first,mount->second);EraseMount(mount);}
 }
}

void G_SandboxChargesReset(){mounts.clear();mountsByProp.clear();}

void G_SandboxChargesHulls(const gentity_t& prop,std::vector<jar::physics::MountedHull>& hulls){
 hulls.clear();
 if(mountsByProp.empty()||!Support(prop))return;
 const auto slot=uint16_t(prop.s.number);
 for(auto it=mountsByProp.lower_bound({slot,0});it!=mountsByProp.end()&&it->first==slot;++it){
  const auto number=it->second;
  const auto& mount=mounts.at(number);
  const auto& charge=g_entities[number];
  if(!LiveMount(number,mount)||G_JAR_WorkshopControls(&charge))continue;
  vec3_t center,half,localCenter;
  for(int axis=0;axis<3;++axis){
   center[axis]=(charge.mins[axis]+charge.maxs[axis])*.5f;
   half[axis]=(charge.maxs[axis]-charge.mins[axis])*.5f;
  }
  if(!Finite(center,1024)||!Finite(half,1024)||half[0]<.25f||half[1]<.25f||half[2]<.25f)continue;
  TransformPoint(center,mount.origin,mount.angles,localCenter);
  if(!Finite(localCenter,65536))continue;
  hulls.push_back({number,mount.chargeLife,{half[0],half[1],half[2]},
   {localCenter[0],localCenter[1],localCenter[2]}});
 }
}

bool G_SandboxChargesPlan(const gentity_t& prop,const vec3_t origin,std::vector<SandboxChargeMove>& moves){
 return PlanCharges(prop,origin,prop.currentAngles,moves,ENTITYNUM_WORLD,true);
}

bool G_SandboxChargesPlanPose(const gentity_t& prop,const vec3_t origin,const vec3_t angles,
 std::vector<SandboxChargeMove>& moves,size_t limit){
 return PlanCharges(prop,origin,angles,moves,limit,false);
}

float G_SandboxChargesFloorOffset(const std::vector<SandboxChargeMove>& moves,float floor){
 float height=0;
 for(const auto& move:moves)height=std::max(height,floor-(move.origin[2]+move.charge->mins[2]));
 return height;
}

void G_SandboxChargesRaise(std::vector<SandboxChargeMove>& moves,float height){
 for(auto& move:moves){move.origin[2]+=height;move.beamStart[2]+=height;}
}

bool G_SandboxChargeFits(const SandboxChargeMove& move,const float* start,const float* padding){
 const auto& charge=*move.charge;
 if(!Finite(move.origin,65536)||!Finite(move.beamStart,65536)||!ValidChargeBounds(charge)||
    (start&&!Finite(start,65536))||(padding&&!Finite(padding,65536)))return false;
 vec3_t mins,maxs;
 for(int axis=0;axis<3;++axis){
  const float pad=padding?padding[axis]:0;
  mins[axis]=charge.mins[axis]-pad;maxs[axis]=charge.maxs[axis]+pad;
 }
 trace_t hit{};
 gi.trace(&hit,start?start:move.origin,mins,maxs,move.origin,charge.s.number,MASK_SHOT,G2_NOCOLLIDE,0);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
}

bool G_SandboxChargesClear(const std::vector<SandboxChargeMove>& moves,int& traceBudget){
 if(traceBudget<int(moves.size()))return false;
 SandboxChargesUnlinked unlinked(moves);
 for(const auto& move:moves){
  --traceBudget;
  if(!G_SandboxChargeFits(move))return false;
 }
 return true;
}

bool G_SandboxChargesPoseClear(gentity_t& prop,const std::vector<SandboxChargeMove>& moves,const float* arc,const float* pivot){
 if(moves.empty())return true;
 SandboxChargesUnlinked charges(moves);
 const bool linked=prop.linked;
 if(linked)gi.unlinkentity(&prop);
 const bool clear=std::all_of(moves.begin(),moves.end(),[&](const SandboxChargeMove& move){
  if(!arc)return G_SandboxChargeFits(move);
  const auto& charge=*move.charge;
  vec3_t padding;VectorScale(arc,Distance(pivot?pivot:prop.currentOrigin,charge.currentOrigin),padding);
  return G_SandboxChargeFits(move,charge.currentOrigin,padding);
 });
 if(linked)gi.linkentity(&prop);
 return clear;
}

SandboxMountedChargeCollision::SandboxMountedChargeCollision(){
 for(const auto& entry:mounts){
  const auto slot=entry.first;
  auto& charge=g_entities[slot];
  if(!LiveMount(slot,entry.second)||G_JAR_WorkshopControls(&charge)||!charge.linked||
     (charge.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS))continue;
  marked.emplace_back(&charge,entry.second.chargeLife);
  charge.contents|=SANDBOX_MOUNTED_CHARGE_CONTENTS;
 }
}

SandboxMountedChargeCollision::~SandboxMountedChargeCollision(){
 for(const auto& entry:marked){
  auto& charge=*entry.first;
  if(charge.inuse&&G_EntityLifetime(&charge)==entry.second)charge.contents&=~SANDBOX_MOUNTED_CHARGE_CONTENTS;
 }
}

// Let a native prop back out of an overlap that placement allowed. Only
// ignore Sandbox props whose initial overlap decreases along this move.
static float PropPenetration(const gentity_t& prop,const vec3_t origin,const gentity_t& other,const JarSweepHull* hull){
 float depth;
 if(G_JAR_WorkshopPropDepth(other,origin,prop.mins,prop.maxs,hull,depth))return depth;
 depth=std::numeric_limits<float>::infinity();
 for(int axis=0;axis<3;++axis){
  const float low=origin[axis]+prop.mins[axis],high=origin[axis]+prop.maxs[axis];
  depth=std::min({depth,high-other.currentOrigin[axis]-other.mins[axis],
   other.currentOrigin[axis]+other.maxs[axis]-low});
 }
 return depth;
}

static void TracePropBody(gentity_t& prop,const vec3_t target,trace_t& hit,const JarSweepHull* hull){
 constexpr int mask=MASK_SOLID|CONTENTS_BODY|SANDBOX_MOUNTED_CHARGE_CONTENTS;
 const auto trace=[&](){
  if(hull&&gi.JAR_TraceMovingHull)
   gi.JAR_TraceMovingHull(&hit,prop.currentOrigin,target,prop.mins,prop.maxs,prop.s.number,mask,*hull);
  else gi.trace(&hit,prop.currentOrigin,prop.mins,prop.maxs,target,prop.s.number,mask,G2_NOCOLLIDE,0);
 };
 trace();
 if((!hit.startsolid&&!hit.allsolid)||!gi.EntitiesInBox)return;
 gentity_t* nearby[256];
 const int count=gi.EntitiesInBox(prop.absmin,prop.absmax,nearby,256);
 std::vector<gentity_t*> escaping;
 for(int i=0;i<count;++i){
  auto* other=nearby[i];
  if(other==&prop||!other->inuse||!other->linked||!(other->contents&CONTENTS_BODY)||
     !other->classname||strcmp(other->classname,"jar_sandbox_prop"))continue;
  const float depth=PropPenetration(prop,prop.currentOrigin,*other,hull);
  if(depth<0||PropPenetration(prop,target,*other,hull)>=depth-.0001f)continue;
  escaping.push_back(other);
 }
 if(escaping.empty())return;
 for(auto* other:escaping)gi.unlinkentity(other);
 trace();
 for(auto* other:escaping)gi.linkentity(other);
}

bool G_SandboxPropTrace(gentity_t& prop,const vec3_t target,trace_t& hit,
 std::vector<SandboxChargeMove>& moves,bool& chargeContact,const JarSweepHull* hull){
 hit={};chargeContact=false;
 if(!G_SandboxChargesPlanPose(prop,target,prop.currentAngles,moves,ENTITYNUM_WORLD))return false;
 SandboxChargesUnlinked charges(moves);
 TracePropBody(prop,target,hit,hull);
 if(hit.startsolid||hit.allsolid)return false;
 if(hit.fraction<1&&hit.entityNum>=0&&hit.entityNum<ENTITYNUM_WORLD)
  chargeContact=(g_entities[hit.entityNum].contents&SANDBOX_MOUNTED_CHARGE_CONTENTS)!=0;
 if(moves.empty())return true;

 vec3_t bodyEnd,trim;
 VectorCopy(hit.endpos,bodyEnd);
 VectorSubtract(target,bodyEnd,trim);
 const float bodyFraction=hit.fraction;
 const bool linked=prop.linked;
 if(linked)gi.unlinkentity(&prop);
 trace_t first{};first.fraction=1;
 for(const auto& move:moves){
  const auto& charge=*move.charge;
  vec3_t end;
  VectorSubtract(move.origin,trim,end);
  trace_t contact{};
  if(!Finite(charge.currentOrigin,65536)||!Finite(end,65536)||!ValidChargeBounds(charge)){
   first.startsolid=qtrue;first.fraction=0;break;
  }
  gi.trace(&contact,charge.currentOrigin,charge.mins,charge.maxs,end,charge.s.number,MASK_SHOT,G2_NOCOLLIDE,0);
  if(contact.startsolid&&!contact.allsolid&&contact.fraction>=1)
   gi.trace(&contact,end,charge.mins,charge.maxs,end,charge.s.number,MASK_SHOT,G2_NOCOLLIDE,0);
  if(contact.startsolid||contact.allsolid){first=contact;first.fraction=0;break;}
  if(contact.fraction<first.fraction)first=contact;
 }
 if(linked)gi.linkentity(&prop);
 if(first.fraction<1){
  chargeContact=true;hit=first;hit.fraction*=bodyFraction;
  vec3_t travel;VectorSubtract(bodyEnd,prop.currentOrigin,travel);
  VectorMA(prop.currentOrigin,first.fraction,travel,hit.endpos);
 }
 if(hit.startsolid||hit.allsolid)return false;
 VectorSubtract(target,hit.endpos,trim);
 for(auto& move:moves){
  VectorSubtract(move.origin,trim,move.origin);
  VectorSubtract(move.beamStart,trim,move.beamStart);
 }
 return true;
}

void G_SandboxChargesPlace(const std::vector<SandboxChargeMove>& moves){
 for(const auto& move:moves)Place(move,false);
}

void G_SandboxChargesRefresh(const std::vector<SandboxChargeMove>& moves){
 for(const auto& move:moves)UpdateBeam(*move.charge);
}

void G_SandboxChargesCommit(const std::vector<SandboxChargeMove>& moves){
 for(const auto& move:moves)Place(move,true);
 G_SandboxChargesRefresh(moves);
}

void G_SandboxChargesExport(bool autosave,ojk::SavedGameHelper& save){
 std::vector<uint16_t> live;
 if(!autosave)for(const auto& entry:mounts)if(LiveMount(entry.first,entry.second))live.push_back(entry.first);
 save.write<uint32_t>(uint32_t(live.size()));
 for(const auto slot:live){
  const auto& mount=mounts.at(slot);
  save.write<uint16_t>(slot);save.write<uint16_t>(mount.prop);
  save.write<float>(mount.origin);save.write<float>(mount.angles);save.write<float>(mount.beamStart);
 }
}

bool G_SandboxChargesImport(bool autosave,ojk::SavedGameHelper& save){
 uint32_t count;save.read<uint32_t>(count);
 if(count>=ENTITYNUM_WORLD||(autosave&&count))return false;
 decltype(mounts) candidate;
 for(uint32_t i=0;i<count;++i){
  uint16_t slot;ChargeMount mount{};
  save.read<uint16_t>(slot);save.read<uint16_t>(mount.prop);
  save.read<float>(mount.origin);save.read<float>(mount.angles);save.read<float>(mount.beamStart);
  if(slot>=ENTITYNUM_WORLD||mount.prop>=ENTITYNUM_WORLD||slot==mount.prop||!ValidLocalFrame(mount))return false;
  mount.chargeLife=G_EntityLifetime(&g_entities[slot]);mount.propLife=G_EntityLifetime(&g_entities[mount.prop]);
  if(!mount.chargeLife||!mount.propLife||!LiveMount(slot,mount)||!candidate.emplace(slot,mount).second)return false;
 }
 save.ensure_all_data_read();
 mounts.swap(candidate);
 mountsByProp.clear();
 for(const auto& entry:mounts)mountsByProp.emplace(entry.second.prop,entry.first);
 return true;
}
