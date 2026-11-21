// Single-player vehicle hull and seat pose transactions. GPL-2.0-only.
#pragma once

#include "g_sandbox_teleport.h"
#include "g_sandbox_vehicle_transport.h"

struct VehicleTeleportMember {
 gentity_t* entity;
 vec3_t position,angles,velocity;
 bool linked;
};
struct VehicleTeleportPlan {
 VehicleTeleportMember members[3]; // Hull, current pilot, optional droid.
 int count=0;
};

struct VehicleTeleportCollisionScope {
 const VehicleTeleportPlan& plan;
 explicit VehicleTeleportCollisionScope(const VehicleTeleportPlan& vehicle):plan(vehicle){
  for(int i=0;i<plan.count;++i)if(plan.members[i].linked)gi.unlinkentity(plan.members[i].entity);
 }
 ~VehicleTeleportCollisionScope(){
  for(int i=0;i<plan.count;++i)if(plan.members[i].linked)gi.linkentity(plan.members[i].entity);
 }
};

inline bool VehiclePoseFinite(const vec3_t value){
 return std::isfinite(value[0])&&std::isfinite(value[1])&&std::isfinite(value[2]);
}

inline bool AddVehicleMember(VehicleTeleportPlan& plan,gentity_t& entity,const vec3_t position,const vec3_t angles,
 const vec3_t velocity){
 if((entity.svFlags&SVF_NO_TELEPORT)||!VehiclePoseFinite(position)||!VehiclePoseFinite(angles)||
    !VehiclePoseFinite(velocity)||!VehiclePoseFinite(entity.mins)||!VehiclePoseFinite(entity.maxs))return false;
 for(int axis=0;axis<3;++axis)if(entity.mins[axis]>entity.maxs[axis])return false;
 auto& member=plan.members[plan.count++];member.entity=&entity;member.linked=entity.linked!=qfalse;
 VectorCopy(position,member.position);VectorCopy(angles,member.angles);VectorCopy(velocity,member.velocity);
 return true;
}

inline void RotateVehicleVelocity(const vec3_t velocity,const vec3_t before,const vec3_t after,vec3_t result){
 if(VectorCompare(before,after)){VectorCopy(velocity,result);return;}
 vec3_t oldAxes[3],newAxes[3];AnglesToAxis(before,oldAxes);AnglesToAxis(after,newAxes);
 VectorClear(result);
 for(int axis=0;axis<3;++axis)VectorMA(result,DotProduct(velocity,oldAxes[axis]),newAxes[axis],result);
}

inline bool G_PlanVehicleTeleportAt(gentity_t& hull,const vec3_t origin,const vec3_t angles,bool keepView,
 bool keepVelocity,int boltTime,VehicleTeleportPlan& plan){
 auto& vehicle=*hull.m_pVehicle;
 plan.count=0;
 vec3_t velocity;
 if(keepVelocity)RotateVehicleVelocity(hull.client->ps.velocity,vehicle.m_vOrientation,angles,velocity);
 else VectorClear(velocity);
 if(!AddVehicleMember(plan,hull,origin,angles,velocity))return false;

 for(auto* rider:{vehicle.m_pPilot,vehicle.m_pDroidUnit}){
  if(!rider)continue;
  const bool pilot=rider==vehicle.m_pPilot;
  const int bolt=pilot?hull.crotchBolt:vehicle.m_iDroidUnitTag;
  mdxaBone_t seat;
  if(bolt<0||hull.playerModel<0||!gi.G2API_GetBoltMatrix(hull.ghoul2,hull.playerModel,bolt,&seat,
       angles,origin,boltTime,nullptr,hull.s.modelScale))return false;
  vec3_t seatOrigin,seatAngles,seatVelocity;
  gi.G2API_GiveMeVectorFromMatrix(seat,ORIGIN,seatOrigin);
  if(keepView)VectorCopy(rider->client->ps.viewangles,seatAngles);
  else if(pilot)VectorCopy(angles,seatAngles);
  else{vec3_t direction;gi.G2API_GiveMeVectorFromMatrix(seat,NEGATIVE_Y,direction);vectoangles(direction,seatAngles);}
  if(keepVelocity)RotateVehicleVelocity(rider->client->ps.velocity,vehicle.m_vOrientation,angles,seatVelocity);
  else VectorClear(seatVelocity);
  if(!AddVehicleMember(plan,*rider,seatOrigin,seatAngles,seatVelocity))return false;
 }
 return true;
}

inline void G_CommitVehicleTeleport(const VehicleTeleportPlan& plan,bool keepVelocity){
 auto& hull=*plan.members[0].entity;
 auto& vehicle=*hull.m_pVehicle;
 const auto& angles=plan.members[0].angles;
 if(keepVelocity){
  vec3_t drift;RotateVehicleVelocity(vehicle.jarPushDrift,vehicle.m_vOrientation,angles,drift);
  VectorSet(vehicle.jarPushDrift,drift[0],drift[1],0);
 }else VectorClear(vehicle.jarPushDrift);
 VectorCopy(angles,vehicle.m_vOrientation);VectorCopy(angles,vehicle.m_vPrevOrientation);
 vec3_t travelAngles={0,angles[YAW],0};
 if(vehicle.m_pVehicleInfo->type==VH_FIGHTER)VectorCopy(angles,travelAngles);
 AngleVectors(travelAngles,hull.client->ps.moveDir,nullptr,nullptr);
 vehicle.m_LandTrace={};vehicle.m_LandTrace.fraction=1;
 vehicle.m_fStrafeTime=0;vehicle.m_vAngularVelocity=0;VectorClear(vehicle.m_vFullAngleVelocity);
 if(!keepVelocity){hull.client->ps.speed=0;VectorClear(vehicle.m_vBoardingVelocity);}
 for(int i=0;i<plan.count;++i){
  const auto& member=plan.members[i];auto& actor=*member.entity;
  G_TeleportMarkArrival(actor);
  VectorCopy(member.velocity,actor.client->ps.velocity);
  vec3_t origin,view;VectorCopy(member.position,origin);origin[2]-=1;
  VectorCopy(member.angles,view);
  G_JAR_TeleportActor(&actor,origin,view,keepVelocity);
  if(!member.linked)gi.unlinkentity(&actor);
 }
}

