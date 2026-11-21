// Native saved state for owned model-backed func_usable. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "qcommon/jar_sandbox_usable.h"
#include <algorithm>
#include <cmath>

constexpr int jarUsablePortable=1<<26,jarUsablePulse=8,jarUsableNPC=32,jarUsablePlayer=64;
inline bool G_SandboxUsableModel(const gentity_t& e){
 return e.inuse&&!e.bmodel&&jar::sandboxUsable::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_box")&&
  e.targetname&&!strncmp(e.targetname,"jar_sb_",7);
}
inline bool G_SandboxUsable(const gentity_t& e){
 return (e.spawnflags&jarUsablePortable)&&G_SandboxUsableModel(e);
}
inline bool G_SandboxUsableDormant(const gentity_t& e){return G_SandboxUsable(e)&&!e.count;}
inline bool G_SandboxUsableRead(const gentity_t& e,jar::sandboxUsable::Settings& s){
 using namespace jar::sandboxUsable;
 if(!G_SandboxUsable(e)||e.damage<NoDamage||e.damage>=DamageCount)return false;
 s.capacity=e.max_health;s.health=std::max(0,e.health);s.reuse=e.delay;
 s.remaining=e.setTime>level.time?e.setTime-level.time:0;
 s.mode=(e.spawnflags&jarUsablePulse)?Pulse:Toggle;
 s.damage=static_cast<Damage>(e.damage);
 s.direct=static_cast<DirectUse>(((e.spawnflags&jarUsablePlayer)?PlayerOnly:LinkedOnly)|
                                  ((e.spawnflags&jarUsableNPC)?NpcOnly:LinkedOnly));
 s.solid=e.count!=0;s.pending=e.alt_fire;s.spent=e.bounceCount!=0;s.enabled=!(e.svFlags&SVF_INACTIVE);
 return Valid(s);
}
inline void G_SandboxUsableRefresh(gentity_t& e){
 using namespace jar::sandboxUsable;
 const bool solid=e.count!=0,enabled=!(e.svFlags&SVF_INACTIVE);
 const bool cooling=e.setTime>level.time,ready=enabled&&!e.alt_fire&&!e.bounceCount&&!cooling;
 e.contents=solid?CONTENTS_SOLID:0;
 e.clipmask=solid?MASK_SOLID:0;
 if(solid)e.s.eFlags&=~EF_NODRAW;
 else {e.s.eFlags|=EF_NODRAW;e.s.solid=0;}
 e.svFlags&=~(SVF_NOCLIENT|SVF_PLAYER_USABLE);
 if(solid&&ready&&(e.spawnflags&jarUsablePlayer))e.svFlags|=SVF_PLAYER_USABLE;
 e.takedamage=solid&&ready&&e.damage!=NoDamage&&e.health>0?qtrue:qfalse;
 e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;
 if(e.alt_fire&&enabled){e.e_ThinkFunc=thinkF_func_wait_return_solid;e.nextthink=level.time+FRAMETIME;}
 else if(cooling){e.e_ThinkFunc=thinkF_func_usable_think;e.nextthink=e.setTime;}
}
inline bool G_SandboxUsableClear(const gentity_t& e){
 trace_t hit{};
 gi.trace(&hit,e.currentOrigin,e.mins,e.maxs,e.currentOrigin,e.s.number,
  MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
}
inline bool G_SandboxUsableSet(gentity_t& e,const jar::sandboxUsable::Settings& s){
 using namespace jar::sandboxUsable;
 if(!G_SandboxUsable(e)||!Valid(s)||(!e.count&&s.solid&&!G_SandboxUsableClear(e)))return false;
 e.max_health=s.capacity;e.health=s.health;e.delay=s.reuse;e.damage=s.damage;e.count=s.solid?1:0;
 e.alt_fire=s.pending?qtrue:qfalse;e.bounceCount=s.spent?1:0;
 e.setTime=s.remaining?level.time+s.remaining:0;
 e.spawnflags=jarUsablePortable|(s.mode==Pulse?jarUsablePulse:0)|
  ((s.direct&PlayerOnly)?jarUsablePlayer:0)|((s.direct&NpcOnly)?jarUsableNPC:0);
 if(s.enabled)e.svFlags&=~SVF_INACTIVE;else e.svFlags|=SVF_INACTIVE;
 if(!s.pending)e.activator=nullptr;
 e.s.eType=ET_GENERAL;e.e_UseFunc=useF_func_usable_use;
 e.e_DieFunc=dieF_func_usable_die;e.e_PainFunc=painF_func_usable_pain;
 G_SandboxUsableRefresh(e);
 gi.linkentity(&e);
 return true;
}
inline bool G_SandboxUsableInitialize(gentity_t& e){
 using namespace jar::sandboxUsable;
 if(!G_SandboxUsableModel(e)||e.s.modelindex<=0)return false;
 Settings s;
 s.capacity=e.e_DieFunc==dieF_func_usable_die?std::max(40,std::min(maxHealth,std::max(e.health,e.max_health))):0;
 s.health=std::max(0,std::min(s.capacity,e.health));
 s.mode=(e.spawnflags&8)?Pulse:Toggle;
 s.direct=(e.spawnflags&32)?NpcOnly:Both;
 s.damage=s.capacity?OnHit:NoDamage;
 if(std::isfinite(e.wait)&&e.wait>0)s.reuse=int(std::lround(1000*std::min(maxReuse/1000.f,e.wait)));
 s.enabled=!(e.svFlags&SVF_INACTIVE);
 s.solid=!(e.s.eFlags&EF_NODRAW);
 s.pending=s.mode==Toggle&&!s.solid&&e.e_ThinkFunc==thinkF_func_wait_return_solid;
 if(s.mode==Pulse){
  s.remaining=e.e_ThinkFunc==thinkF_func_usable_think?std::max(0,std::min(maxReuse,e.nextthink-level.time)):0;
  s.spent=!s.remaining&&e.e_UseFunc==useF_NULL&&e.e_ThinkFunc!=thinkF_func_usable_think;
 }
 e.count=s.solid?1:0;e.spawnflags|=jarUsablePortable;
 e.s.eFlags&=~(EF_SHADER_ANIM|EF_ANIM_ONCE|EF_ANIM_ALLFAST);
 return G_SandboxUsableSet(e,s);
}
inline bool G_SandboxUsableRestore(gentity_t& e){
 if(!G_SandboxUsableModel(e))return true;
 if(!G_SandboxUsable(e))return G_SandboxUsableInitialize(e);
 jar::sandboxUsable::Settings settings;
 return G_SandboxUsableRead(e,settings)&&G_SandboxUsableSet(e,settings);
}
inline void G_SandboxUsableControlledFrame(gentity_t& e){
 if(!G_SandboxUsable(e)||e.nextthink<=0||e.nextthink>level.time)return;
 if(e.e_ThinkFunc==thinkF_func_wait_return_solid)func_wait_return_solid(&e);
 else if(e.e_ThinkFunc==thinkF_func_usable_think)func_usable_think(&e);
}
bool G_SandboxUsableActivate(gentity_t& e,gentity_t* actor,bool direct);
