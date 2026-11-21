// Translate a mover's running trajectory without firing it.
// GPL-2.0-only.
#pragma once

#include "g_mover_motion.h"
#include "g_sandbox_teleport.h"
#include "g_sandbox_vehicle_transport.h"
#include "g_subbsp_ownership.h"
#include "qcommon/jar_lift_motion.h"
#include "g_mover_portal.h"
#include "qcommon/jar_constraint.h"
#include <algorithm>
#include <vector>

qboolean Q3_TaskIDPending(gentity_t*,taskID_t);

struct TeleportMoverMotion {
 trajectory_t position;
 vec3_t base,first,second;
 JarMoverMotion external;
 bool hasExternal=false,binary=false;
};

inline bool G_TeleportBinaryTeam(gentity_t& mover,std::vector<gentity_t*>& parts){
 auto* leader=mover.teammaster?mover.teammaster:&mover;
 if(!G_VehicleEntityPointer(leader)||(leader->flags&FL_TEAMSLAVE)||
    (leader->teammaster&&leader->teammaster!=leader))return false;
 bool containsMover=false;
 for(auto* part=leader;part;part=part->teamchain){
  if(parts.size()>jar::constraint::limit||!G_VehicleEntityPointer(part)||
     G_TeleportMoverKind(*part)!=TeleportMoverType::Binary||
     (part!=leader&&(part->teammaster!=leader||!(part->flags&FL_TEAMSLAVE)))||
     std::find(parts.begin(),parts.end(),part)!=parts.end())return false;
  parts.push_back(part);
  containsMover|=part==&mover;
 }
 return containsMover;
}

inline bool G_TeleportMoverReady(gentity_t& mover,TeleportMoverType type){
 if(!G_TeleportBrushMover(type))return true;
 if(mover.next_roff_time>0||G_SubBSPOwner(&mover)||Q3_TaskIDPending(&mover,TID_MOVE_NAV)||
    Q3_TaskIDPending(&mover,TID_ANGLE_FACE))return false;
 if(type==TeleportMoverType::Binary)
  return mover.nextthink<=0||(mover.e_ThinkFunc!=thinkF_Think_SpawnNewDoorTrigger&&mover.e_ThinkFunc!=thinkF_Think_MatchTeam);
 if(type!=TeleportMoverType::Train)return true;
 const auto* corner=mover.nextTrain;
 return !(mover.nextthink>0&&mover.e_ThinkFunc==thinkF_Think_SetupTrainTargets)&&G_VehicleEntityPointer(corner)&&
  corner->classname&&!strcmp(corner->classname,"path_corner");
}

inline bool G_PlanTeleportMover(gentity_t& mover,TeleportMoverType type,const vec3_t arrival,
 bool keepVelocity,TeleportMoverMotion& plan){
 plan={};
 plan.binary=type==TeleportMoverType::Binary;
 vec3_t offset;VectorSubtract(arrival,mover.currentOrigin,offset);
 plan.position=mover.s.pos;
 VectorAdd(plan.position.trBase,offset,plan.position.trBase);
 VectorCopy(arrival,plan.base);
 VectorCopy(mover.pos1,plan.first);VectorCopy(mover.pos2,plan.second);
 if(type==TeleportMoverType::Bobbing){
  jarBobState_t state;
  if(!G_BobbingRead(mover,state))return false;
  plan.first[2]+=offset[2];
  plan.second[2]=arrival[2];
 }else if(type==TeleportMoverType::Section){
  // pos2 is the published orientation, not a second endpoint.
  VectorCopy(arrival,plan.first);
 }else if(type==TeleportMoverType::Rail){
  jar::rail::Settings settings;
  if(!G_RailRead(mover,settings))return false;
  VectorAdd(plan.first,offset,plan.first);VectorAdd(plan.second,offset,plan.second);
 }else if(G_TeleportBrushMover(type)){
  VectorAdd(mover.s.origin,offset,plan.base);
  VectorAdd(plan.first,offset,plan.first);VectorAdd(plan.second,offset,plan.second);
 }else if(type==TeleportMoverType::Slide){
  jar::slidingDoor::State state;
  if(!G_SandboxSlideRead(mover,state))return false;
 }else if(type==TeleportMoverType::Angular){
  jar::sandboxAngular::Settings settings;
  if(!G_SandboxAngularRead(mover,settings))return false;
 }else return false;
 plan.hasExternal=G_JAR_MoverMotionRead(mover,plan.external)||type==TeleportMoverType::Rail||type==TeleportMoverType::Train;
 if(plan.hasExternal){
  VectorAdd(plan.external.offset,offset,plan.external.offset);
  if(!keepVelocity){VectorClear(plan.external.velocity);VectorClear(plan.external.angular);}
  if(!G_JAR_MoverMotionValid(plan.external))return false;
 }
 for(const auto* point:{plan.position.trBase,plan.base,plan.first,plan.second}){
  for(int axis=0;axis<3;++axis)if(!std::isfinite(point[axis])||std::fabs(point[axis])>131072)return false;
 }
 return true;
}

inline void G_CommitTeleportMover(gentity_t& mover,const vec3_t arrival,const TeleportMoverMotion& plan){
 // Keep the authored opening visible after removing its brush. The engine
 // saves the portal counts, and the relocated mover no longer changes them.
 if(plan.binary&&!(mover.flags&jarDetachedMoverAreaPortal)){
  if(mover.linked&&mover.moverState==MOVER_POS1&&(!mover.teammaster||mover.teammaster==&mover))
   G_AdjustMoverAreaPortalState(&mover,qtrue);
  mover.flags|=jarDetachedMoverAreaPortal;
 }
 mover.s.pos=plan.position;
 VectorCopy(arrival,mover.currentOrigin);VectorCopy(plan.base,mover.s.origin);
 VectorCopy(plan.first,mover.pos1);VectorCopy(plan.second,mover.pos2);
 if(plan.hasExternal)G_JAR_MoverMotionRestore(mover,plan.external);
 mover.s.eFlags^=EF_TELEPORT_BIT;
}
