// Single-player Sandbox beam and explosion-trail aiming. GPL-2.0-only.
#pragma once
#include "g_subbsp_ownership.h"
#include "qcommon/jar_angular_trajectory.h"
#include <cmath>

inline bool G_WorkshopBeamClass(const gentity_t& entity){
 return entity.classname&&!strcmp(entity.classname,"fx_target_beam");
}

inline gentity_t* G_WorkshopFxEndpoint(const gentity_t& emitter){
 if(!emitter.target||!emitter.target[0])return nullptr;
 const auto matches=[&](const gentity_t* endpoint){
  return endpoint&&endpoint!=&emitter&&endpoint->inuse&&endpoint->targetname&&
         !Q_stricmp(endpoint->targetname,emitter.target)&&G_SubBSPNameVisible(endpoint);
 };
 if(matches(emitter.enemy))return emitter.enemy;
 auto* endpoint=G_Find(nullptr,FOFS(targetname),emitter.target);
 if(endpoint==&emitter)endpoint=G_Find(endpoint,FOFS(targetname),emitter.target);
 return matches(endpoint)?endpoint:nullptr;
}

inline void G_WorkshopBeamEnableUse(gentity_t& beam){
 beam.e_UseFunc=useF_fx_target_beam_use;
 beam.svFlags|=SVF_PLAYER_USABLE;
}

inline void G_WorkshopBeamIdle(gentity_t& beam){
 beam.enemy=nullptr;
 beam.e_ThinkFunc=thinkF_NULL;
 beam.nextthink=0;
 VectorCopy(beam.currentOrigin,beam.s.origin2);
 G_WorkshopBeamEnableUse(beam);
}

inline bool G_WorkshopBeamAim(gentity_t& beam,vec3_t endpoint){
 beam.enemy=G_WorkshopFxEndpoint(beam);
 if(!beam.enemy){G_WorkshopBeamIdle(beam);return false;}
 VectorCopy(beam.enemy->currentOrigin,endpoint);
 return true;
}

inline bool G_WorkshopTrailClass(const gentity_t& trail){
 return trail.classname&&!strcmp(trail.classname,"fx_explosion_trail");
}

// Returns true when the Workshop flight bypasses the native brush pusher.
bool G_WorkshopTrailFrame(gentity_t& flight);

inline void G_WorkshopTrailEnableUse(gentity_t& trail){
 trail.e_UseFunc=useF_fx_explosion_trail_use;
 trail.e_ThinkFunc=thinkF_NULL;
 trail.nextthink=0;
 trail.svFlags|=SVF_PLAYER_USABLE;
}

inline void G_WorkshopTrailDirection(const gentity_t& trail,vec3_t direction){
 if(const auto* endpoint=G_WorkshopFxEndpoint(trail)){
  VectorSubtract(endpoint->currentOrigin,trail.currentOrigin,direction);
  if(VectorNormalize(direction)>.001f)return;
 }
 AngleVectors(trail.currentAngles,direction,nullptr,nullptr);
}

inline void G_WorkshopTrailOrientFlight(gentity_t& projectile,const vec3_t direction){
 vec3_t angles;
 vectoangles(direction,angles);
 G_SetAngles(&projectile,angles);
 VectorCopy(direction,projectile.movedir);
}

inline bool G_WorkshopTrailLegacyFacing(const vec3_t saved,vec3_t facing){
 // The old native link stored a normalized direction as the entity angles.
 if(!(std::fabs(VectorLengthSquared(saved)-1)<.001f))return false;
 vectoangles(saved,facing);
 return true;
}

inline void G_WorkshopTrailRestore(gentity_t& trail,bool legacy,trajectory_t* heldRotation=nullptr,
 float* heldFacing=nullptr){
 if(legacy&&trail.e_UseFunc==useF_fx_explosion_trail_use){
  vec3_t facing;
  if(G_WorkshopTrailLegacyFacing(trail.currentAngles,facing))G_SetAngles(&trail,facing);
  if(heldRotation&&heldFacing&&G_WorkshopTrailLegacyFacing(heldFacing,facing)){
   if(!jar::angular::Rebase(*heldRotation,heldFacing,facing))VectorCopy(facing,heldRotation->trBase);
   VectorCopy(facing,heldFacing);
  }
 }
 G_WorkshopTrailEnableUse(trail);
}
