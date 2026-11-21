// Relocate a released prop's native and Box3D representations together.
// GPL-2.0-only.
#ifndef G_TELEPORT_PROP_MOTION_H
#define G_TELEPORT_PROP_MOTION_H
#include "jar_physics.h"
#include "g_sandbox_charges.h"
inline void G_CommitTeleportPropMotion(gentity_t& prop,const vec3_t origin,const vec3_t linear,const vec3_t angular){
 G_SetOrigin(&prop,origin);
 prop.s.pos.trType=TR_INTERPOLATE;
 VectorCopy(linear,prop.s.pos.trDelta);VectorCopy(angular,prop.s.apos.trDelta);
 prop.s.eFlags^=EF_TELEPORT_BIT;
}
inline bool G_TeleportPropMotion(jar::physics::World& world,gentity_t& prop,const vec3_t origin,bool keepVelocity,bool rigid){
 vec3_t landing,linear,angular;VectorCopy(origin,landing);
 std::vector<SandboxChargeMove> charges;
 if(!G_SandboxChargesPlan(prop,landing,charges))return false;
 if(!charges.empty()){
  const float height=G_SandboxChargesFloorOffset(charges,landing[2]+prop.mins[2]);
  landing[2]+=height;G_SandboxChargesRaise(charges,height);
  const bool linked=prop.linked!=qfalse;
  if(linked)gi.unlinkentity(&prop);
  int traceBudget=ENTITYNUM_WORLD;
  bool clear=G_SandboxChargesClear(charges,traceBudget);
  if(clear&&height>0){
   trace_t hit{};
   gi.trace(&hit,landing,prop.mins,prop.maxs,landing,prop.s.number,MASK_SOLID|CONTENTS_BODY,G2_NOCOLLIDE,0);
   clear=!hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
  }
  if(linked)gi.linkentity(&prop);
  if(!clear)return false;
 }
 VectorCopy(keepVelocity?prop.s.pos.trDelta:vec3_origin,linear);
 VectorCopy(keepVelocity?prop.s.apos.trDelta:vec3_origin,angular);
 if(rigid){
  jar::physics::Pose pose;
  if(!world.Read(prop.s.number,pose))return false;
  pose.position={landing[0],landing[1],landing[2]};
  if(!keepVelocity){pose.linear={};pose.angular={};}
  if(!world.CorrectPose(prop.s.number,pose))return false;
 }
 G_CommitTeleportPropMotion(prop,landing,linear,angular);
 gi.linkentity(&prop);
 G_SandboxChargesCommit(charges);
 return true;
}
#endif
