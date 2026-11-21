// Owned emitter settings use native saved fields and callbacks. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "qcommon/jar_sandbox_laser.h"
#include <algorithm>

inline bool G_SandboxLaser(const gentity_t& source){
 return source.inuse&&jar::sandboxLaser::Class(source.classname)&&source.model&&
  !strcmp(source.model,jar::sandboxLaser::model);
}

inline bool G_SandboxLaserRead(const gentity_t& source,jar::sandboxLaser::Settings& settings){
 if(!G_SandboxLaser(source)||source.e_UseFunc!=useF_target_laser_use||
    source.e_ThinkFunc!=thinkF_target_laser_think)return false;
 settings={source.damage,(source.spawnflags&1)!=0,(source.svFlags&SVF_INACTIVE)==0};
 return jar::sandboxLaser::Valid(settings);
}

inline bool G_SandboxLaserSet(gentity_t& source,const jar::sandboxLaser::Settings& settings){
 if(!G_SandboxLaser(source)||!jar::sandboxLaser::Valid(settings))return false;
 source.damage=settings.damage;
 if(settings.firing)source.spawnflags|=1;
 else source.spawnflags&=~1;
 if(settings.enabled)source.svFlags&=~SVF_INACTIVE;
 else source.svFlags|=SVF_INACTIVE;
 source.alt_fire=qfalse;
 source.nextthink=settings.firing?level.time+jar::sandboxLaser::interval:0;
 return true;
}

inline void G_SandboxLaserUse(gentity_t& source){
 jar::sandboxLaser::Settings settings;
 if(!G_SandboxLaserRead(source,settings)||!settings.enabled)return;
 settings.firing=!settings.firing;
 G_SandboxLaserSet(source,settings);
}

inline void G_SandboxLaserAimChanged(gentity_t& source){
 source.alt_fire=qfalse;
}

inline gentity_t* G_SandboxLaserAim(const gentity_t& source){
 auto* target=source.target&&*source.target?G_Find(nullptr,FOFS(targetname),source.target):nullptr;
 return target&&target->inuse&&!target->freeAfterEvent&&target!=&source?target:nullptr;
}

inline bool G_SandboxLaserInitialize(gentity_t& source,const jar::sandboxLaser::Settings& settings){
 if(!G_SandboxLaserSet(source,settings))return false;
 source.s.eType=ET_GENERAL;
 source.e_UseFunc=useF_target_laser_use;
 source.e_ThinkFunc=thinkF_target_laser_think;
 source.enemy=source.activator=nullptr;
 source.contents=CONTENTS_ITEM;
 source.svFlags=(source.svFlags&~SVF_NOCLIENT)|SVF_PLAYER_USABLE;
 return true;
}

inline bool G_SandboxLaserRestore(gentity_t& source){
 if(!jar::sandboxLaser::Class(source.classname))return true;
 jar::sandboxLaser::Settings settings;
 if(!G_SandboxLaser(source)){
  // Legacy owned emitters used the authored start/think callbacks.
  if(source.e_ThinkFunc!=thinkF_target_laser_start&&source.e_ThinkFunc!=thinkF_target_laser_think)return false;
  settings.damage=std::min(jar::sandboxLaser::maxDamage,std::max(0,source.damage));
  if(source.e_ThinkFunc==thinkF_target_laser_start&&settings.damage==0)settings.damage=1;
  settings.firing=source.e_ThinkFunc==thinkF_target_laser_start?(source.spawnflags&1)!=0:source.nextthink>0;
  settings.enabled=!(source.svFlags&SVF_INACTIVE);
  if(source.model&&gi.bIsFromZone(source.model,TAG_G_ALLOC))gi.Free(source.model);
  source.model=G_NewString(jar::sandboxLaser::model);
 }else if(!G_SandboxLaserRead(source,settings))return false;
 if(!G_SandboxLaserInitialize(source,settings))return false;
 gi.linkentity(&source);
 return true;
}

inline bool G_SandboxLaserBeam(const gentity_t& source,vec3_t start,vec3_t end){
 if(!G_SandboxLaser(source)||!source.alt_fire||(source.svFlags&SVF_INACTIVE))return false;
 VectorCopy(source.pos2,start);
 VectorCopy(source.s.origin2,end);
 return true;
}

inline void G_SandboxLaserControlledFrame(gentity_t* source){
 if(G_SandboxLaser(*source)&&source->nextthink>0&&source->nextthink<=level.time)target_laser_think(source);
}
