// Native saved fields for model-backed Sandbox breakables. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "qcommon/jar_sandbox_breakable.h"
#include <algorithm>
#include <cmath>

constexpr int jarBreakablePortable=1<<27,jarBreakablePlayerUse=128,jarBreakableNoExplosion=2048;
inline bool G_SandboxBreakableModel(const gentity_t& e){
 return e.inuse&&!e.bmodel&&jar::sandboxBreakable::Class(e.classname)&&
  e.targetname&&!strncmp(e.targetname,"jar_sb_",7);
}
inline bool G_SandboxBreakable(const gentity_t& e){
 return (e.spawnflags&jarBreakablePortable)&&G_SandboxBreakableModel(e);
}
inline bool G_SandboxBreakableBroken(const gentity_t& e){
 return G_SandboxBreakable(e)&&e.count==jar::sandboxBreakable::Broken;
}
inline bool G_SandboxBreakableRead(const gentity_t& e,jar::sandboxBreakable::Settings& s){
 using namespace jar::sandboxBreakable;
 if(!G_SandboxBreakable(e)||!std::isfinite(e.radius)||e.radius<0||e.radius>maxChunks/100.f)return false;
 s.capacity=e.max_health;s.health=e.health;s.fuse=e.delay;
 s.phase=static_cast<Phase>(e.count);s.remaining=s.phase==Pending?std::max(1,e.nextthink-level.time):0;
 s.mode=static_cast<Damage>(e.damage);s.material=e.material;s.chunks=int(std::lround(100*e.radius));
 s.splash=e.splashDamage;s.radius=e.splashRadius;
 s.enabled=!(e.svFlags&SVF_INACTIVE);s.playerUse=(e.spawnflags&jarBreakablePlayerUse)!=0;
 s.explosion=!(e.spawnflags&jarBreakableNoExplosion);
 return Valid(s);
}
inline void G_SandboxBreakableRefresh(gentity_t& e){
 using namespace jar::sandboxBreakable;
 const bool intact=e.count==Intact,broken=e.count==Broken;
 const bool enabled=!(e.svFlags&SVF_INACTIVE);
 e.flags&=~(FL_DMG_BY_SABER_ONLY|FL_DMG_BY_HEAVY_WEAP_ONLY);
 if(e.damage==Saber)e.flags|=FL_DMG_BY_SABER_ONLY;
 if(e.damage==Heavy)e.flags|=FL_DMG_BY_HEAVY_WEAP_ONLY;
 e.takedamage=intact&&enabled&&e.damage!=UseOnly?qtrue:qfalse;
 e.contents=broken?0:CONTENTS_SOLID;
 e.clipmask=broken?0:MASK_SOLID;
 if(broken){e.s.eFlags|=EF_NODRAW;e.svFlags|=SVF_BROKEN;e.s.solid=0;}
 else {e.s.eFlags&=~EF_NODRAW;e.svFlags&=~SVF_BROKEN;}
 e.svFlags&=~(SVF_NOCLIENT|SVF_BBRUSH|SVF_PLAYER_USABLE);
 if(intact&&enabled&&(e.spawnflags&jarBreakablePlayerUse))e.svFlags|=SVF_PLAYER_USABLE;
}
inline bool G_SandboxBreakableSet(gentity_t& e,const jar::sandboxBreakable::Settings& s){
 using namespace jar::sandboxBreakable;
 if(!G_SandboxBreakable(e)||!Valid(s))return false;
 if(e.count==Broken&&s.phase!=Broken){
  trace_t hit{};
  gi.trace(&hit,e.currentOrigin,e.mins,e.maxs,e.currentOrigin,e.s.number,
   MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
  if(hit.startsolid||hit.allsolid||hit.fraction<1)return false;
 }
 const bool keepFuse=e.count==Pending&&s.phase==Pending&&e.nextthink>0;
 const bool keepMotion=e.count==Intact&&s.phase==Intact&&e.e_ThinkFunc==thinkF_G_RunObject;
 e.count=s.phase;e.health=s.health;e.max_health=s.capacity;e.delay=s.fuse;e.damage=s.mode;
 e.material=static_cast<material_t>(s.material);e.radius=s.chunks/100.f;e.splashDamage=s.splash;e.splashRadius=s.radius;
 e.spawnflags=jarBreakablePortable|(s.playerUse?jarBreakablePlayerUse:0)|(s.explosion?0:jarBreakableNoExplosion);
 if(s.enabled)e.svFlags&=~SVF_INACTIVE;else e.svFlags|=SVF_INACTIVE;
 e.s.eType=ET_GENERAL;e.e_UseFunc=useF_funcBBrushUse;e.e_DieFunc=dieF_funcBBrushDie;
 e.e_PainFunc=painF_NULL;e.e_TouchFunc=touchF_NULL;
 if(!keepMotion)e.e_ThinkFunc=s.phase==Pending?thinkF_funcBBrushDieGo:thinkF_NULL;
 if(!keepFuse&&!keepMotion){e.nextthink=s.phase==Pending?level.time+s.remaining:0;e.enemy=nullptr;}
 G_SandboxBreakableRefresh(e);
 gi.linkentity(&e);
 return true;
}
inline bool G_SandboxBreakableInitialize(gentity_t& e){
 using namespace jar::sandboxBreakable;
 if(!G_SandboxBreakableModel(e)||e.s.modelindex<=0)return false;
 Settings s;
 s.capacity=std::max(1,std::min(maxHealth,std::max(e.health,e.max_health)));
 s.fuse=1000*std::max(0,std::min(maxFuse/1000,e.delay));
 s.phase=e.e_ThinkFunc==thinkF_funcBBrushDieGo?Pending:
  ((e.svFlags&SVF_BROKEN)||e.e_ThinkFunc==thinkF_G_FreeEntity?Broken:Intact);
 if(s.phase==Pending)s.remaining=std::max(1,std::min(maxFuse,e.nextthink-level.time));
 if(s.phase==Intact)s.health=std::max(1,std::min(s.capacity,e.health));
 s.mode=(e.spawnflags&1)&&Class(e.classname)!=Model?UseOnly:
  (e.flags&FL_DMG_BY_SABER_ONLY)?Saber:(e.flags&FL_DMG_BY_HEAVY_WEAP_ONLY)?Heavy:Normal;
 s.enabled=!(e.svFlags&SVF_INACTIVE);
 s.playerUse=(e.svFlags&SVF_PLAYER_USABLE)!=0;s.explosion=!(e.spawnflags&jarBreakableNoExplosion);
 s.material=std::max(0,std::min(materialCount-1,int(e.material)));
 if(std::isfinite(e.radius)&&e.radius>0)s.chunks=int(std::lround(100*std::min(maxChunks/100.f,e.radius)));
 s.splash=std::max(0,std::min(maxSplash,e.splashDamage));s.radius=std::max(0,std::min(maxRadius,e.splashRadius));
 if(s.phase==Broken&&Class(e.classname)==Model&&e.sound2to1>0)e.s.modelindex=e.sound2to1;
 auto* attacker=s.phase==Pending?e.enemy:nullptr;
 e.count=Intact;e.spawnflags|=jarBreakablePortable;
 if(!G_SandboxBreakableSet(e,s))return false;
 e.enemy=attacker;
 return true;
}
inline bool G_SandboxBreakableRestore(gentity_t& e){
 if(!G_SandboxBreakableModel(e))return true;
 if(!G_SandboxBreakable(e))return G_SandboxBreakableInitialize(e);
 jar::sandboxBreakable::Settings s;
 return G_SandboxBreakableRead(e,s)&&G_SandboxBreakableSet(e,s);
}
inline void G_SandboxBreakableControlledFrame(gentity_t& e){
 if(G_SandboxBreakable(e)&&e.e_ThinkFunc==thinkF_funcBBrushDieGo&&e.nextthink>0&&e.nextthink<=level.time)
  funcBBrushDieGo(&e);
}
