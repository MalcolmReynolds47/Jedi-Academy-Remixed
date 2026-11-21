// Native controllers are suspended while a Sandbox tool owns their motion.
// Original trajectories and gravity are restored without invoking Force damage.
#include "g_bsp_section.h"
#include "g_sliding_door.h"
#include "g_sandbox_laser.h"
#include "g_sandbox_station.h"
#include "g_sandbox_breakable.h"
#include "g_sandbox_usable.h"
#include "g_sandbox_speaker.h"
#include "g_sandbox_angular.h"
#include "g_mover_motion.h"
#include "g_sandbox_vehicle_teleport.h"
#include "g_sandbox_vehicle_push.h"
#include "g_workshop_rack_pose.h"
#include "g_workshop_grip.inc"
#include "../../shared/game/jar_rotation_sweep.h"
struct NativeControl {
 unsigned entity=0;int started=0,gripFlag=0,gravity=0;bool frozen=false;
 trajectory_t position{},rotation{};vec3_t original{},originalAngles{},velocity{};
};
std::map<unsigned,NativeControl> nativeControls;
bool nativeOwnedControl(unsigned entity){return nativeControls.count(entity)!=0;}
bool nativeControlledRider(const gentity_t& rider){
 if(!rider.client||rider.m_pVehicle||rider.s.number<=0)return false;
 const int slot=rider.s.m_iVehicleNum;
 if(slot<=0||slot>=globals.num_entities||slot>=ENTITYNUM_WORLD||!nativeOwnedControl(slot))return false;
 const auto& hull=g_entities[slot];
 if(!G_VehicleEntityPointer(&hull)||!hull.m_pVehicle||hull.m_pVehicle->m_pParentEntity!=&hull||rider.owner!=&hull)return false;
 const auto& vehicle=*hull.m_pVehicle;
 return vehicle.m_pPilot==&rider||vehicle.m_pDroidUnit==&rider;
}
bool nativeVehicleHoldReady(const gentity_t& hull){
 if(!hull.m_pVehicle||hull.m_pVehicle->m_pPilot==&g_entities[0])return false;
 return G_TransportVehicleReady(hull,[](const gentity_t& member){
  if(member.svFlags&SVF_NO_TELEPORT)return false;
  if(!member.m_pVehicle&&(member.s.number==0||nativeOwnedControl(member.s.number)))return false;
  const auto control=nativeControls.find(member.s.number);
  const int allowed=control!=nativeControls.end()&&!control->second.gripFlag?EF_FORCE_GRIPPED:0;
  return !(member.client->ps.eFlags&(JAR_VEHICLE_IMMOBILE_FLAGS&~allowed))&&
   member.client->ps.saberLockTime<=level.time&&member.next_roff_time<=level.time;
 });
}
void nativeBegin(gentity_t&e,bool frozen){
 G_JAR_MoverMotionStop(e);
 if(G_SandboxSlidePortable(e)){e.s.loopSound=0;e.fly_sound_debounce_time=JAR_SLIDE_PAUSED;}
 auto it=nativeControls.find(e.s.number);
 if(it==nativeControls.end()){
  NativeControl record;record.entity=e.s.number;record.started=level.time;record.position=e.s.pos;record.rotation=e.s.apos;
  VectorCopy(e.currentOrigin,record.original);VectorCopy(e.currentAngles,record.originalAngles);record.gripFlag=e.client?(e.client->ps.eFlags&EF_FORCE_GRIPPED):(e.s.eFlags&EF_FORCE_GRIPPED);
  record.gravity=e.client?e.client->ps.gravity:0;it=nativeControls.emplace(e.s.number,record).first;
  if(e.client && !e.m_pVehicle && e.client->clientInfo.animFileIndex==0 && !frozen)NPC_SetAnim(&e,SETANIM_TORSO,BOTH_CHOKE1,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
 }
 it->second.frozen=frozen;if(frozen)VectorClear(it->second.velocity);
 if(e.client){e.client->ps.eFlags|=EF_FORCE_GRIPPED;e.client->ps.gravity=0;VectorClear(e.client->ps.velocity);}
 if(e.m_pVehicle)VectorClear(e.m_pVehicle->jarPushDrift);
 else e.s.eFlags|=EF_FORCE_GRIPPED;
 e.s.pos.trType=TR_INTERPOLATE;e.s.apos.trType=jar::angular::Path(it->second.rotation.trType)?TR_AXIS_INTERPOLATE:TR_INTERPOLATE;VectorCopy(e.currentOrigin,e.s.pos.trBase);VectorCopy(e.currentAngles,e.s.apos.trBase);
 VectorClear(e.s.pos.trDelta);VectorClear(e.s.apos.trDelta);
}
void nativeRelease(unsigned entity,bool fling){
 auto it=nativeControls.find(entity);if(it==nativeControls.end())return;const auto r=it->second;nativeControls.erase(it);
 if(entity>=ENTITYNUM_WORLD||!g_entities[entity].inuse)return;auto&e=g_entities[entity];
 const int paused=std::max(0,level.time-r.started);vec3_t offset;VectorSubtract(e.currentOrigin,r.original,offset);
 e.s.pos=r.position;e.s.apos=r.rotation;VectorAdd(e.s.pos.trBase,offset,e.s.pos.trBase);e.s.pos.trTime+=paused;e.s.apos.trTime+=paused;
 if(!jar::angular::Rebase(e.s.apos,r.originalAngles,e.currentAngles))
  for(int axis=0;axis<3;++axis)e.s.apos.trBase[axis]+=AngleSubtract(e.currentAngles[axis],r.originalAngles[axis]);
 if(e.next_roff_time>0){VectorAdd(e.pos1,offset,e.pos1);for(int axis=0;axis<3;++axis)e.pos2[axis]+=AngleSubtract(e.currentAngles[axis],r.originalAngles[axis]);}
 else if(e.s.eType==ET_MOVER&&!jar::bspSection::Portable(e)){VectorAdd(e.pos1,offset,e.pos1);VectorAdd(e.pos2,offset,e.pos2);}
 G_JAR_MoverMotionRebase(e,offset);
 if(e.nextthink>0&&!G_LaserArmPortable(e)&&!G_SandboxLaser(e)&&!G_SandboxStation(e)&&!G_SandboxBreakable(e)&&
    !G_SandboxUsable(e)&&!G_SandboxSpeaker(e)&&e.e_ThinkFunc!=thinkF_portable_shield_think)e.nextthink+=paused;
 if(e.next_roff_time>0)e.next_roff_time+=paused;
 const bool setup=setupMoved(e);
 if(e.client){e.client->ps.eFlags=(e.client->ps.eFlags&~EF_FORCE_GRIPPED)|r.gripFlag;e.client->ps.gravity=r.gravity;
  VectorCopy(e.currentOrigin,e.client->ps.origin);
  if(fling){
   VectorCopy(r.velocity,e.client->ps.velocity);
   if(e.m_pVehicle){
    VectorSet(e.m_pVehicle->jarPushDrift,r.velocity[0],r.velocity[1],0);
    e.client->ps.speed=0;
   }
  }
  e.client->ps.torsoAnimTimer=0;
 }else {e.s.eFlags=(e.s.eFlags&~EF_FORCE_GRIPPED)|r.gripFlag;
  const bool rackMomentum=workshopRack(e)&&VectorLengthSquared(r.velocity)>0;
  if(fling && (!setup||rackMomentum) && !e.bmodel && e.s.eType!=ET_MOVER && (e.s.eType==ET_ITEM||e.s.eType==ET_MISSILE||e.e_ThinkFunc==thinkF_NULL||e.e_ThinkFunc==thinkF_G_RunObject)){VectorCopy(e.currentOrigin,e.s.pos.trBase);VectorCopy(r.velocity,e.s.pos.trDelta);e.s.pos.trTime=level.time;e.s.pos.trType=TR_GRAVITY;
   if(e.s.eType!=ET_ITEM&&e.s.eType!=ET_MISSILE){e.e_ThinkFunc=thinkF_G_RunObject;e.nextthink=level.time+FRAMETIME;e.clipmask|=MASK_SOLID|CONTENTS_BODY;}}}
 gi.linkentity(&e);
}
void nativeReleaseAll(){while(!nativeControls.empty())nativeRelease(nativeControls.begin()->first,false);}
bool nativeVehiclePose(gentity_t& hull,const vec3_t origin,const vec3_t angles,int boltTime,
 VehicleTeleportPlan& plan,bool sweep,const float* pivot=nullptr){
 if(!nativeVehicleHoldReady(hull))return false;
 if(!G_PlanVehicleTeleportAt(hull,origin,angles,true,false,boltTime,plan))return false;
 VehicleTeleportCollisionScope unlinked(plan);
 vec3_t starts[3];
 for(int i=0;i<plan.count;++i)
  VectorCopy(sweep?plan.members[i].entity->currentOrigin:plan.members[i].position,starts[i]);
 const auto clear=[&](const VehicleTeleportPlan& next,const vec3_t axis,float stepRadians){
  for(int i=0;i<next.count;++i){
   const auto& member=next.members[i];
   const auto& entity=*member.entity;
   for(float value:member.position)if(std::fabs(value)>65535)return false;
   vec3_t pad{},lo,hi;
   if(stepRadians>0){
    const float radius=std::max(Distance(pivot,starts[i]),Distance(pivot,member.position));
    JAR_RotationPadding(axis,stepRadians,radius,pad);
   }
   VectorSubtract(entity.mins,pad,lo);
   VectorAdd(entity.maxs,pad,hi);
   trace_t hit{};
   gi.trace(&hit,starts[i],lo,hi,member.position,entity.s.number,MASK_PLAYERSOLID|entity.clipmask,G2_NOCOLLIDE,0);
   if(hit.startsolid||hit.allsolid||hit.fraction<1)return false;
   VectorCopy(member.position,starts[i]);
  }
  return true;
 };
 if(!sweep||!pivot)return clear(plan,vec3_origin,0);

 const auto& old=hull.currentAngles;
 const jar::physics::Vec before={old[0],old[1],old[2]},after={angles[0],angles[1],angles[2]};
 const auto spin=jar::physics::AngularVelocity(before,after,1);
 vec3_t axis={spin.x,spin.y,spin.z};
 const float radians=VectorNormalize(axis);
 if(radians<.0001f)return clear(plan,vec3_origin,0);
 const jar::physics::Vec center={pivot[0],pivot[1],pivot[2]};
 jar::physics::PhysgunGrip grip;
 const auto& start=hull.currentOrigin;
 if(!grip.Begin({start[0],start[1],start[2]},before,center,0))return false;

 // Account for any difference between each current rider position and its
 // native seat bolt before tracing the arc at the same animation time.
 VehicleTeleportPlan step;
 if(!G_PlanVehicleTeleportAt(hull,start,old,true,false,boltTime,step)||!clear(step,vec3_origin,0))return false;
 const int slices=JAR_RotationSlices(radians);
 const float stepRadians=radians/slices;
 for(int i=1;i<slices;++i){
  const auto facing=jar::physics::AdvanceAngles(before,spin,float(i)/slices);
  const auto position=grip.Origin(center,facing);
  const vec3_t facingVector={facing.x,facing.y,facing.z},positionVector={position.x,position.y,position.z};
  if(!G_PlanVehicleTeleportAt(hull,positionVector,facingVector,true,false,boltTime,step)||
     !clear(step,axis,stepRadians))return false;
 }
 return clear(plan,axis,stepRadians);
}
void nativeCommitVehiclePose(const VehicleTeleportPlan& plan){
 auto& hull=*plan.members[0].entity;
 const auto& angles=plan.members[0].angles;
 for(int i=0;i<plan.count;++i){
  const auto& member=plan.members[i];
  G_SetOrigin(member.entity,member.position);
  VectorCopy(member.position,member.entity->client->ps.origin);
 }
 G_SetAngles(&hull,angles);
 SetClientViewAngle(&hull,hull.currentAngles);
 VectorCopy(angles,hull.m_pVehicle->m_vOrientation);
 VectorCopy(angles,hull.m_pVehicle->m_vPrevOrientation);
 for(int i=0;i<plan.count;++i)if(plan.members[i].linked)gi.linkentity(plan.members[i].entity);
}
bool nativeMoveVehicle(gentity_t& hull,const vec3_t origin,const vec3_t angles,int boltTime,
 const float* pivot=nullptr){
 VehicleTeleportPlan plan;
 if(!nativeVehiclePose(hull,origin,angles,boltTime,plan,true,pivot))return false;
 nativeCommitVehiclePose(plan);
 return true;
}
void nativeFrame(float dt,bool (*planStock)(gentity_t&,const vec3_t,const vec3_t,bool,RackStockPlan&)=nullptr,
                 void (*detachStock)(gentity_t&)=nullptr){
 for(auto it=nativeControls.begin();it!=nativeControls.end();){
  const auto entity=it->first;if(entity>=ENTITYNUM_WORLD||!g_entities[entity].inuse){it=nativeControls.erase(it);continue;}
  auto&e=g_entities[entity];auto&r=it->second;++it;
  if(e.m_pVehicle&&!nativeVehicleHoldReady(e)){
   // Held hulls are disarmed by the caller before running this frame.
   if(held!=entity)nativeRelease(entity,false);
   continue;
  }
  if(r.frozen || held!=entity || dt<=0)continue;
  vec3_t target,old;VectorCopy(e.currentOrigin,old);
  if(physgunGrip.active)physgunOriginGoal(e.currentAngles,holdDistance,target);
  else{
   vec3_t start,dir;eye(start,dir);VectorMA(start,holdDistance,dir,target);
   if(jar::bspSection::Portable(e)){vec3_t center,offset;jar::bspSection::Center(e,center);VectorSubtract(center,e.currentOrigin,offset);VectorSubtract(target,offset,target);}
  }
  vec3_t delta;VectorSubtract(target,old,delta);float distance=VectorNormalize(delta);VectorMA(old,std::min(distance,1024.f*dt),delta,target);
  if(e.m_pVehicle){
   if(nativeMoveVehicle(e,target,e.currentAngles,level.time)){
    VectorSubtract(e.currentOrigin,old,delta);VectorScale(delta,1.f/dt,r.velocity);
   }else VectorClear(r.velocity);
   continue;
  }
  if(G_JAR_PushableMover(e)){
   vec3_t move,actual,pathStart,pathEnd;
   VectorSubtract(target,old,move);
   VectorCopy(e.pos1,pathStart);VectorCopy(e.pos2,pathEnd);
   G_JAR_MoverTranslate(e,move,actual);
   // Release shifts the paused path by the total movement once.
   VectorCopy(pathStart,e.pos1);VectorCopy(pathEnd,e.pos2);
   if(VectorLengthSquared(actual)>0)setupMoved(e);
   VectorScale(actual,1.f/dt,r.velocity);
   continue;
  }
  trace_t tr{};
  if(jar::volumeEntity::Portable(e)){tr.fraction=1;VectorCopy(target,tr.endpos);}
  else gi.trace(&tr,old,e.mins,e.maxs,target,entity,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
  if(tr.startsolid||tr.allsolid){VectorClear(r.velocity);continue;}
  RackStockPlan stock;
  if(planStock&&!planStock(e,tr.endpos,e.currentAngles,true,stock)){VectorClear(r.velocity);continue;}
  if(detachStock&&!VectorCompare(old,tr.endpos))detachStock(e);
  G_SetOrigin(&e,tr.endpos);setupMoved(e);VectorSubtract(tr.endpos,old,delta);VectorScale(delta,1.f/dt,r.velocity);
  if(e.client){VectorCopy(tr.endpos,e.client->ps.origin);VectorClear(e.client->ps.velocity);}
  gi.linkentity(&e);commitRackStock(stock);
 }
}
void nativeSave(bool autosave,ojk::SavedGameHelper&h){
 const uint32_t count=autosave?0:uint32_t(nativeControls.size());h.write_chunk<uint32_t>(INT_ID('J','N','C','N'),count);
 h.reset_buffer();if(!autosave)for(const auto&entry:nativeControls){const auto&r=entry.second;
  h.write<uint32_t>(r.entity);h.write<int32_t>(r.started);h.write<int32_t>(r.gripFlag);h.write<int32_t>(r.gravity);
  r.position.sg_export(h);r.rotation.sg_export(h);h.write<float>(r.original,3);h.write<float>(r.originalAngles,3);
 }h.write_chunk(INT_ID('J','N','A','T'));
}
void nativeRead(bool autosave,ojk::SavedGameHelper&h){
 uint32_t count=0;h.read_chunk<uint32_t>(INT_ID('J','N','C','N'),count);
 if(count>=ENTITYNUM_WORLD || (autosave && count))gi.Error(ERR_DROP,"Invalid Sandbox native controller count");
 h.read_chunk(INT_ID('J','N','A','T'));std::map<unsigned,NativeControl> candidate;
 for(unsigned i=0;i<count;++i){NativeControl r;h.read<uint32_t>(r.entity);h.read<int32_t>(r.started);h.read<int32_t>(r.gripFlag);h.read<int32_t>(r.gravity);
  r.position.sg_import(h);r.rotation.sg_import(h);h.read<float>(r.original,3);h.read<float>(r.originalAngles,3);r.frozen=true;
  bool valid=r.entity<ENTITYNUM_WORLD && g_entities[r.entity].inuse && !candidate.count(r.entity) &&
   (r.gripFlag==0||r.gripFlag==EF_FORCE_GRIPPED) && r.gravity>=0 && r.gravity<=10000;
  for(int axis=0;axis<3;++axis)valid&=std::isfinite(r.originalAngles[axis])&&std::fabs(r.originalAngles[axis])<100000&&std::isfinite(r.original[axis])&&std::fabs(r.original[axis])<65536 &&
   std::isfinite(r.position.trBase[axis])&&std::isfinite(r.position.trDelta[axis])&&std::isfinite(r.rotation.trBase[axis])&&std::isfinite(r.rotation.trDelta[axis]);
  valid&=r.position.trType>=TR_STATIONARY&&r.position.trType<=TR_GRAVITY&&r.rotation.trType>=TR_STATIONARY&&r.rotation.trType<=TR_AXIS_INTERPOLATE;
  if(r.position.trType==TR_SINE)valid&=r.position.trDuration>0;
  if(r.rotation.trType==TR_SINE||r.rotation.trType==TR_AXIS_SINE)valid&=r.rotation.trDuration>0;
  if(!valid)gi.Error(ERR_DROP,"Invalid Sandbox native controller record");candidate.emplace(r.entity,r);
 }h.ensure_all_data_read();nativeControls=std::move(candidate);
}
