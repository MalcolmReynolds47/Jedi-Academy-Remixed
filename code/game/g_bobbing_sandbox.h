#pragma once
#include "qcommon/jar_bobbing_motion.h"

constexpr char JAR_BOB_PORTABLE[]="@jar_bobbing";
inline bool G_BobbingClass(const char* name){return name&&!strcmp(name,"func_bobbing");}
inline bool G_BobbingPortable(const gentity_t& deck){
 return deck.inuse&&G_BobbingClass(deck.classname)&&deck.model&&!strcmp(deck.model,JAR_BOB_PORTABLE);
}
inline bool G_BobbingLegacy(const gentity_t& deck){
 return deck.inuse&&!deck.bmodel&&G_BobbingClass(deck.classname)&&deck.model&&!strcmp(deck.model,"@jar_box");
}

inline void G_BobbingHull(const vec3_t angles,vec3_t mins,vec3_t maxs){
 const vec3_t low={-16.75f,-10.921875f,.0625f},high={16.828125f,10.921875f,21.90625f};
 vec3_t basis[3];AnglesToAxis(angles,basis);
 for(int axis=0;axis<3;++axis){
  mins[axis]=maxs[axis]=0;
  for(int i=0;i<3;++i){
   const float b=basis[i][axis];
   mins[axis]+=b*(b>=0?low[i]:high[i]);
   maxs[axis]+=b*(b>=0?high[i]:low[i]);
  }
 }
}
bool G_BobbingClear(const gentity_t&,const vec3_t origin,const vec3_t angles);
bool G_BobbingMoved(gentity_t&);
bool G_BobbingSpawn(gentity_t&);
bool G_BobbingRead(const gentity_t&,jarBobState_t&);
bool G_BobbingRestore(gentity_t&,const jarBobState_t&);
bool G_BobbingSettings(gentity_t&,const jarBobState_t&);
bool G_BobbingMigrate(gentity_t&,const trajectory_t&,int sampleTime);
