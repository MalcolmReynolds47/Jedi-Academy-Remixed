#pragma once
#include "qcommon/jar_sliding_door.h"
#include "qcommon/jar_lift.h"

constexpr char JAR_SLIDING_DOOR_PORTABLE[]="@jar_sliding_door";
constexpr char JAR_LIFT_PORTABLE[]="@jar_lift";
inline bool G_LiftClass(const char* classname){return classname&&!strcmp(classname,"func_plat");}
inline bool G_LiftPortable(const gentity_t& deck){
 return deck.inuse&&G_LiftClass(deck.classname)&&deck.model&&!strcmp(deck.model,JAR_LIFT_PORTABLE);
}
inline bool G_SandboxSlideClass(const char* classname)
{
 return classname && !strcmp(classname,"func_door");
}
inline bool G_SandboxSlidePortable(const gentity_t& door)
{
 return G_LiftPortable(door)||(door.inuse&&G_SandboxSlideClass(door.classname)&&door.model&&
        !strcmp(door.model,JAR_SLIDING_DOOR_PORTABLE));
}
inline void G_SandboxSlideHull(const vec3_t angles,vec3_t mins,vec3_t maxs,bool lift=false)
{
 const vec3_t panelLow={-36,-96,0},panelHigh={6.828125f,96,170};
 const vec3_t grateLow={-32.03125f,-32.03125f,.03125f},grateHigh={32.03125f,32.046875f,4.921875f};
 const float* low=lift?grateLow:panelLow;
 const float* high=lift?grateHigh:panelHigh;
 vec3_t basis[3];AnglesToAxis(angles,basis);
 for(int a=0;a<3;++a){
  mins[a]=maxs[a]=0;
  for(int i=0;i<3;++i){
   const float b=basis[i][a];
   mins[a]+=b*(b>=0?low[i]:high[i]);
   maxs[a]+=b*(b>=0?high[i]:low[i]);
  }
 }
}
bool G_SandboxSlideClear(const gentity_t&,const vec3_t origin,const vec3_t angles);
bool G_SandboxSlideMoved(gentity_t&);
bool G_SandboxSlideSpawn(gentity_t&);
bool G_SandboxSlideRead(const gentity_t&,jar::slidingDoor::State&);
bool G_SandboxSlideSettings(gentity_t&,const jar::slidingDoor::Settings&);
bool G_SandboxSlideRestore(gentity_t&,const jar::slidingDoor::State&);
bool G_LiftTranslate(gentity_t&,float);
