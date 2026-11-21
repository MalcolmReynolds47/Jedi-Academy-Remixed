// Portable crate movers use native pusher collision and saved entity fields. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "g_bobbing_sandbox.h"
#include "qcommon/jar_sandbox_angular.h"
#include "qcommon/jar_angular_trajectory.h"
#include <algorithm>
#include <cmath>

constexpr int jarAngularRunning=4;

inline bool G_SandboxAngular(const gentity_t& e){
 return e.inuse&&e.model&&!strcmp(e.model,jar::sandboxAngular::model)&&jar::sandboxAngular::Class(e.classname);
}

inline bool G_SandboxAngularRead(const gentity_t& e,jar::sandboxAngular::Settings& s){
 using namespace jar::sandboxAngular;
 if(!G_SandboxAngular(e)||!(e.speed>=-maxSpeed&&e.speed<=maxSpeed&&e.speed==std::floor(e.speed))||
    !(e.wait>=minPeriod&&e.wait<=maxPeriod&&e.wait==std::floor(e.wait)))return false;
 s={Class(e.classname),e.spawnflags&3,int(e.speed),int(e.wait),e.count,(e.spawnflags&jarAngularRunning)!=0,!(e.svFlags&SVF_INACTIVE)};
 return Valid(s);
}

inline bool G_SandboxAngularSet(gentity_t& e,const jar::sandboxAngular::Settings& s){
 if(!G_SandboxAngular(e)||jar::sandboxAngular::Class(e.classname)!=s.kind||!jar::sandboxAngular::Valid(s))return false;
 e.speed=s.rate;e.wait=s.period;e.count=s.phase;
 e.spawnflags=s.axis|(s.running?jarAngularRunning:0);
 if(s.enabled)e.svFlags&=~SVF_INACTIVE;
 else e.svFlags|=SVF_INACTIVE;
 e.s.eFlags&=~EF_BLOCKED_MOVER;
 return true;
}

inline void G_SandboxAngularHull(const vec3_t angles,vec3_t mins,vec3_t maxs){
 G_BobbingHull(angles,mins,maxs); // The same stock Imperial crate and local bounds.
}

inline bool G_SandboxAngularClear(const gentity_t& e,const vec3_t origin,const vec3_t angles){
 vec3_t mins,maxs;G_SandboxAngularHull(angles,mins,maxs);
 trace_t hit{};
 gi.trace(&hit,origin,mins,maxs,origin,e.s.number,MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
 return !hit.startsolid&&!hit.allsolid;
}

inline bool G_SandboxAngularMoved(gentity_t& e){
 if(!G_SandboxAngular(e))return false;
 G_SandboxAngularHull(e.currentAngles,e.mins,e.maxs);
 VectorCopy(e.currentOrigin,e.s.origin);
 VectorCopy(e.currentAngles,e.s.angles);
 VectorCopy(e.currentAngles,e.s.apos.trBase);
 VectorClear(e.s.apos.trDelta);
 e.s.apos.trType=TR_AXIS_INTERPOLATE;
 return true;
}

inline bool G_SandboxAngularInit(gentity_t& e,const jar::sandboxAngular::Settings& s){
 if(!G_SandboxAngularSet(e,s))return false;
 e.bmodel=qfalse;e.s.eType=ET_MOVER;e.contents=CONTENTS_SOLID;e.clipmask=MASK_PLAYERSOLID;
 e.health=e.damage=0;e.takedamage=qfalse;
 e.s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");
 e.e_UseFunc=useF_func_rotating_use;e.e_TouchFunc=touchF_NULL;e.e_ThinkFunc=thinkF_NULL;
 e.svFlags|=SVF_PLAYER_USABLE;
 return G_SandboxAngularMoved(e);
}

inline bool G_SandboxAngularRestore(gentity_t& e,const trajectory_t& old,int time){
 using namespace jar::sandboxAngular;
 const Kind kind=Class(e.classname);
 if(!kind)return true;
 if(G_SandboxAngular(e)){
  Settings s;
  if(!G_SandboxAngularRead(e,s)||!G_SandboxAngularMoved(e))return false;
 }else{
  if(e.bmodel||!e.model||strcmp(e.model,"@jar_box"))return false;
  Settings s=Defaults(kind);
  s.enabled=!(e.svFlags&SVF_INACTIVE);
  s.running=old.trType==TR_LINEAR||old.trType==TR_SINE;
  if(kind==Spinner)s.axis=e.spawnflags&4?Forward:e.spawnflags&8?Side:Up;
  const float rate=old.trDelta[s.axis==Forward?ROLL:s.axis==Side?PITCH:YAW];
  if(std::isfinite(rate)&&std::fabs(rate)>=1&&(kind==Spinner?std::fabs(rate)<=maxSpeed:rate>=1&&rate<=maxSwing))
   s.rate=int(std::round(rate));
  if(kind==Pendulum&&old.trDuration>=minPeriod&&old.trDuration<=maxPeriod){
   s.period=old.trDuration;
   const int64_t phase=(int64_t(time)-old.trTime)%s.period;
   s.phase=int((phase+s.period)%s.period);
  }
  if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);
  e.model=G_NewString(model);
  if(!G_SandboxAngularInit(e,s))return false;
 }
 gi.linkentity(&e);
 return true;
}

inline void G_SandboxAngularUse(gentity_t& e){
 jar::sandboxAngular::Settings s;
 if(!G_SandboxAngularRead(e,s)||!s.enabled)return;
 s.running=!s.running;
 G_SandboxAngularSet(e,s);
}

inline void G_SandboxAngularAxis(const gentity_t& e,int axis,vec3_t direction){
 vec3_t axes[3];AnglesToAxis(e.currentAngles,axes);
 const int basis=axis==jar::sandboxAngular::Up?2:axis==jar::sandboxAngular::Forward?0:1;
 VectorCopy(axes[basis],direction);
}

inline void G_SandboxAngularVelocity(const gentity_t& e,vec3_t velocity){
 using namespace jar::sandboxAngular;
 Settings s;
 if(!G_SandboxAngularRead(e,s)||!s.running||!s.enabled||(e.s.eFlags&(EF_BLOCKED_MOVER|EF_FORCE_GRIPPED)))return;
 float rate=s.rate*float(M_PI/180);
 if(s.kind==Pendulum){
  const double radians=2*M_PI/s.period;
  rate*=float(std::cos(s.phase*radians)*radians*1000);
 }
 G_SandboxAngularAxis(e,s.axis,velocity);
 VectorScale(velocity,rate,velocity);
}
