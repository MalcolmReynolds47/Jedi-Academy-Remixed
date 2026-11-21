// Portable stations retain native model, Use and poll callbacks. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "qcommon/jar_sandbox_station.h"
#include <algorithm>

constexpr int jarStationPortable=1<<28,jarStationUnlimited=1<<29;
inline bool G_SandboxStationNative(const gentity_t& source){
 const auto kind=jar::sandboxStation::Class(source.classname);
 return source.inuse&&source.targetname&&!strncmp(source.targetname,"jar_sb_",7)&&
  ((kind==jar::sandboxStation::Ammo&&source.e_UseFunc==useF_ammo_power_converter_use)||
   (kind==jar::sandboxStation::Shield&&source.e_UseFunc==useF_shield_power_converter_use));
}
inline bool G_SandboxStation(const gentity_t& source){
 return (source.spawnflags&jarStationPortable)&&G_SandboxStationNative(source);
}
inline bool G_SandboxStationRead(const gentity_t& source,jar::sandboxStation::Settings& s){
 if(!G_SandboxStation(source))return false;
 s={source.max_health,source.count,source.delay,(source.spawnflags&jarStationUnlimited)!=0,
    (source.svFlags&SVF_INACTIVE)==0};
 return jar::sandboxStation::Valid(s);
}
inline void G_SandboxStationStopSound(gentity_t& source){
 source.s.loopSound=0;
 source.attackDebounceTime=0;
}
inline void G_SandboxStationSchedule(gentity_t& source){
 const bool infinite=(source.spawnflags&jarStationUnlimited)!=0;
 const bool enabled=(source.svFlags&SVF_INACTIVE)==0;
 if(!enabled)G_SandboxStationStopSound(source);
 if(enabled&&!infinite&&source.delay&&source.count<source.max_health){
  if(source.painDebounceTime<=0)
   source.painDebounceTime=std::max(level.time,source.attackDebounceTime)+source.delay;
 }else source.painDebounceTime=0;
 source.e_ThinkFunc=thinkF_poll_converter;
 source.nextthink=source.painDebounceTime;
 if(source.attackDebounceTime&&(!source.nextthink||source.attackDebounceTime<source.nextthink))
  source.nextthink=source.attackDebounceTime;
 if(source.s.eFlags&EF_SHADER_ANIM)source.s.frame=source.count||infinite?0:1;
}
inline bool G_SandboxStationSet(gentity_t& source,const jar::sandboxStation::Settings& s){
 if(!G_SandboxStation(source)||!jar::sandboxStation::Valid(s))return false;
 if(source.delay!=s.recharge)source.painDebounceTime=0;
 source.max_health=s.capacity;source.count=s.charge;source.delay=s.recharge;
 if(s.unlimited)source.spawnflags|=jarStationUnlimited;
 else source.spawnflags&=~jarStationUnlimited;
 if(s.enabled)source.svFlags&=~SVF_INACTIVE;
 else source.svFlags|=SVF_INACTIVE;
 G_SandboxStationSchedule(source);
 return true;
}
inline bool G_SandboxStationInitialize(gentity_t& source,int fallbackCapacity){
 using namespace jar::sandboxStation;
 if(!G_SandboxStationNative(source))return false;
 const int charge=std::max(0,std::min(maxCapacity,source.count));
 source.spawnflags=(source.spawnflags|jarStationPortable)&~jarStationUnlimited;
 source.painDebounceTime=0;
 G_SandboxStationStopSound(source);
 return G_SandboxStationSet(source,{std::max(charge,fallbackCapacity),charge,0,false,
  (source.svFlags&SVF_INACTIVE)==0});
}
inline bool G_SandboxStationRestore(gentity_t& source,int difficulty){
 using namespace jar::sandboxStation;
 if(!Class(source.classname))return true;
 if(!G_SandboxStation(source)){
  const int difficultyCapacity=difficulty<=0?100:difficulty==1?75:50;
  return G_SandboxStationInitialize(source,difficultyCapacity);
 }
 Settings s;
 if(!G_SandboxStationRead(source,s))return false;
 // Supply, cooldown and next recharge survive a load. The use sound is transient.
 G_SandboxStationStopSound(source);
 G_SandboxStationSchedule(source);
 return true;
}
inline void G_SandboxStationPoll(gentity_t& source){
 jar::sandboxStation::Settings s;
 if(!G_SandboxStationRead(source,s)){
  G_SandboxStationStopSound(source);
  source.nextthink=0;
  return;
 }
 if(source.attackDebounceTime<=level.time)G_SandboxStationStopSound(source);
 if(s.enabled&&!s.unlimited&&s.recharge&&source.painDebounceTime>0&&source.painDebounceTime<=level.time){
  const int elapsed=1+(level.time-source.painDebounceTime)/s.recharge;
  const int restored=std::min(s.capacity-source.count,elapsed);
  source.count+=restored;
  source.painDebounceTime+=restored*s.recharge;
 }
 G_SandboxStationSchedule(source);
}
inline void G_SandboxStationControlledFrame(gentity_t& source){
 if(G_SandboxStation(source)&&source.nextthink>0&&source.nextthink<=level.time)G_SandboxStationPoll(source);
}
