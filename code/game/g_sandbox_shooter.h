// The native wait, random, spawn flags and think callback persist in saves.
#pragma once
#include "g_functions.h"
#include "qcommon/jar_shooter.h"
#include <cmath>

inline bool G_SandboxShooter(const gentity_t& e){
 return e.inuse&&e.client&&e.e_UseFunc==useF_misc_weapon_shooter_use&&
  e.targetname&&!strncmp(e.targetname,"jar_sb_",7);
}

inline int G_SandboxShooterPending(const gentity_t& e,int clock){
 if(e.e_ThinkFunc!=thinkF_misc_weapon_shooter_fire||e.nextthink<=0)return 0;
 return std::max(1,std::min(e.nextthink-clock,jar::shooter::maxInterval+jar::shooter::maxVariation));
}

inline void G_SandboxShooterSchedule(gentity_t& e,int clock,int pending){
 e.e_ThinkFunc=pending?thinkF_misc_weapon_shooter_fire:thinkF_NULL;
 e.nextthink=pending?clock+pending:0;
}

inline bool G_SandboxShooterRead(const gentity_t& e,jar::shooter::Settings& value){
 using namespace jar::shooter;
 if(!G_SandboxShooter(e)||!std::isfinite(e.wait)||e.wait<minInterval||e.wait>maxInterval||
    !std::isfinite(e.random)||e.random<0||e.random>maxVariation)return false;
 value={int(e.wait),int(e.random),(e.spawnflags&altFireFlag)!=0,(e.spawnflags&repeatFlag)!=0,(e.svFlags&SVF_INACTIVE)==0};
 return true;
}

inline bool G_SandboxShooterSet(gentity_t& e,const jar::shooter::Settings& value){
 using namespace jar::shooter;
 if(!G_SandboxShooter(e)||!Valid(value))return false;
 e.wait=value.interval;e.random=value.variation;
 e.spawnflags=(e.spawnflags&~(altFireFlag|repeatFlag))|(value.alternate?altFireFlag:0)|(value.repeat?repeatFlag:0);
 if(value.enabled)e.svFlags&=~SVF_INACTIVE;else e.svFlags|=SVF_INACTIVE;
 if(!value.enabled||!value.repeat)G_SandboxShooterSchedule(e,0,0);
 return true;
}
