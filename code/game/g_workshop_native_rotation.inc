// Collision admission for Sandbox native entities turned by the Workshop tools.
// Included in the workshop namespace. GPL-2.0-only.
#include "../../shared/game/jar_rotation_sweep.h"

bool nativeObjectTurnClear(const gentity_t& e,const vec3_t origin,const vec3_t angles,
 const float* pivot,const jar::physics::PhysgunGrip& grip,bool traceGeneric){
 const bool wall=G_WallPortable(e)&&e.count;
 const bool door=G_SandboxSlidePortable(e)&&!G_LiftPortable(e);
 const bool button=G_ButtonPortable(e);
 const int mask=(door||button)?MASK_PLAYERSOLID|CONTENTS_CORPSE:MASK_SOLID|CONTENTS_BODY;
 const auto clear=[&](const vec3_t start,const vec3_t low,const vec3_t high,const vec3_t end){
  trace_t hit{};
  gi.trace(&hit,start,low,high,end,e.s.number,mask,G2_NOCOLLIDE,0);
  return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
 };
 if(!wall&&!door&&!button)
  return !traceGeneric||clear(e.currentOrigin,e.mins,e.maxs,origin);

 const auto bounds=[&](const vec3_t pose,vec3_t low,vec3_t high){
  if(wall)G_WallVisibleBounds(e,pose,low,high);
  else if(door)G_SandboxSlideHull(pose,low,high);
  else G_ButtonHull(pose,low,high);
 };
 const auto before=pv(e.currentAngles),after=pv(angles);
 const auto spin=jar::physics::AngularVelocity(before,after,1);
 vec3_t axis,low,high;
 nativeVector(spin,axis);
 const float radians=VectorNormalize(axis);
 if(radians<.0001f){
  bounds(angles,low,high);
  return clear(e.currentOrigin,low,high,origin);
 }

 const int slices=JAR_RotationSlices(radians);
 bounds(vec3_origin,low,high);
 const bool fixedHull=wall&&std::strcmp(e.model,JAR_WALL_PORTABLE_MODEL);
 const float radius=(fixedHull?0:RadiusFromBounds(low,high))+(pivot?Distance(pivot,e.currentOrigin):0);
 vec3_t padding,start;
 JAR_RotationPadding(axis,radians/slices,radius,padding);
 VectorCopy(e.currentOrigin,start);
 bounds(e.currentAngles,low,high);
 for(int i=1;i<=slices;++i){
  const auto pose=i==slices?after:jar::physics::AdvanceAngles(before,spin,float(i)/slices);
  vec3_t end,rotation,move,nextLow,nextHigh,sweepLow,sweepHigh;
  nativeVector(pose,rotation);
  if(i==slices||!pivot)VectorCopy(origin,end);
  else nativeVector(grip.Origin(pv(pivot),pose),end);
  bounds(rotation,nextLow,nextHigh);
  VectorSubtract(end,start,move);
  JAR_RotationEnvelope(move,low,high,nextLow,nextHigh,padding,sweepLow,sweepHigh);
  if(!clear(start,sweepLow,sweepHigh,start))return false;
  VectorCopy(end,start);
  VectorCopy(nextLow,low);
  VectorCopy(nextHigh,high);
 }
 return true;
}
