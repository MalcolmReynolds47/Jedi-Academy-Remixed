// Portable target settings use the native entity save fields. GPL-2.0-only.
#ifndef G_LOGIC_SETTINGS_H
#define G_LOGIC_SETTINGS_H
#include "qcommon/jar_logic_settings.h"
#include "g_functions.h"
#include "g_sandbox_station.h"
#include "g_sandbox_breakable.h"
#include "g_sandbox_usable.h"
#include "g_sandbox_speaker.h"
#include "g_sandbox_shooter.h"
#include "g_sandbox_replicator.h"
#include <cmath>
#include <initializer_list>

inline jar::logic::Kind G_LogicKind(const gentity_t& source) {
 // Older Sandbox saves already have this generated target name.
 if(!source.targetname || std::strncmp(source.targetname,"jar_sb_",7))
  return jar::logic::None;
 return jar::logic::Class(source.classname);
}

inline char* gentity_t::*G_LogicOutputField(int slot) {
 static const auto fields={&gentity_t::target,&gentity_t::target2,&gentity_t::target3,&gentity_t::target4};
 return slot>=0&&slot<jar::logic::outputCount?fields.begin()[slot]:nullptr;
}

inline const char* G_LogicOutput(const gentity_t& source,int slot) {
 const auto field=G_LogicOutputField(slot);
 return field?source.*field:nullptr;
}

inline void G_LogicSetOutput(gentity_t& source,int slot,const char* name) {
 const auto field=G_LogicOutputField(slot);
 if(!field)return;
 auto*& output=source.*field;
 auto* replacement=name?G_NewString(name):nullptr;
 if(output&&gi.bIsFromZone(output,TAG_G_ALLOC))gi.Free(output);
 output=replacement;
}

inline void G_LogicRestore(gentity_t& source) {
 if(G_LogicKind(source)==jar::logic::Random&&source.e_UseFunc==useF_NULL&&(source.spawnflags&1)){
  source.count=1;
  source.e_UseFunc=useF_target_random_use;
 }
}

inline void G_LogicTimerStop(gentity_t& source) {
 source.nextthink=0;
 source.activator=nullptr;
}

inline auto G_LogicScheduledThink(jar::logic::Kind kind) -> decltype(gentity_t::e_ThinkFunc) {
 switch(kind){
 case jar::logic::Delay:return thinkF_Think_Target_Delay;
 case jar::logic::Timer:return thinkF_func_timer_think;
 case jar::logic::Relay:return thinkF_target_relay_use_go;
 default:return thinkF_NULL;
 }
}

inline gentity_t* G_LogicActivator(gentity_t& source,gentity_t* actor) {
 // Native map timers use themselves when there is no activating character.
 // Retain that contract for stock receivers that require a non-null entity.
 return actor&&actor->inuse?actor:&source;
}

inline bool G_LogicPickup(const gentity_t& pickup) {
 return pickup.inuse&&pickup.item&&pickup.s.eType==ET_ITEM&&
  pickup.e_TouchFunc==touchF_Touch_Item&&!pickup.freeAfterEvent&&
  !(pickup.s.eFlags&EF_NODRAW)&&!(pickup.svFlags&SVF_NOCLIENT)&&
  (pickup.contents&(CONTENTS_ITEM|CONTENTS_TRIGGER));
}

inline bool G_LogicCanSetUse(const gentity_t& target) {
 if(!target.inuse||target.freeAfterEvent||target.item)return false;
 if(G_SandboxReplicator(target))return true;
 if(target.e_UseFunc!=useF_NULL)return true;
 switch(target.e_TouchFunc){
 case touchF_Touch_Multi:
 case touchF_trigger_push_touch:
 case touchF_trigger_teleporter_touch:
 case touchF_hurt_touch:return true;
 default:return false;
 }
}

inline bool G_LogicUseEnabled(const gentity_t& target) {
 return !(target.svFlags&SVF_INACTIVE);
}

inline void G_LogicSetUseEnabled(gentity_t& target,bool enabled) {
 if(G_SandboxSpeaker(target)){G_SandboxSpeakerEnable(target,enabled);return;}
 if(enabled)target.svFlags&=~SVF_INACTIVE;
 else target.svFlags|=SVF_INACTIVE;
 if(!enabled&&G_SandboxShooter(target))G_SandboxShooterSchedule(target,0,0);
 if(G_SandboxStation(target))G_SandboxStationSchedule(target);
 if(G_SandboxBreakable(target))G_SandboxBreakableRefresh(target);
 if(G_SandboxUsable(target))G_SandboxUsableRefresh(target);
}

inline bool G_LogicLiveActor(const gentity_t* actor) {
 return actor&&actor->inuse&&actor->client&&actor->health>0&&!actor->freeAfterEvent;
}

inline bool G_LogicRead(const gentity_t& source,jar::logic::State& value) {
 using namespace jar::logic;
 value={};
 const Kind kind=G_LogicKind(source);
 if((kind==Delay&&source.e_UseFunc==useF_Use_Target_Delay)||
    (kind==Timer&&source.e_UseFunc==useF_func_timer_use)){
  if(!std::isfinite(source.wait) || source.wait<0 || source.wait>maxDelay*.001f ||
     !std::isfinite(source.random) || source.random<0 || source.random>maxDelay*.001f)return false;
  value.delayMs=int(std::lround(source.wait*1000));
  value.jitterMs=int(std::lround(source.random*1000));
 }else if(kind==Relay&&source.e_UseFunc==useF_target_relay_use){
  if(!std::isfinite(source.wait)||source.wait<0||source.wait>maxDelay||source.count<0||source.count>1)return false;
  value.delayMs=source.delay;
  value.cooldownMs=int(std::lround(source.wait));
  value.remaining=source.count?0:1;
  value.repeat=(source.spawnflags&8)==0;
 }else if(kind==Score&&source.e_UseFunc==useF_Use_Target_Score){
  value.points=source.count;
 }else if(kind==Kill&&source.e_UseFunc==useF_target_kill_use){
  value.killMode=source.spawnflags&1?Falling:source.spawnflags&2?Electrical:Normal;
 }else if(kind==Random&&source.e_UseFunc==useF_target_random_use){
  if(source.count<0||source.count>1)return false;
  value.remaining=source.count?0:1;
  value.repeat=(source.spawnflags&1)==0;
 }else if(kind==Counter && source.e_UseFunc==useF_target_counter_use){
  value.hits=source.max_health;
  value.remaining=source.count;
  value.repeat=source.bounceCount==-1;
 }else if((kind!=Give||source.e_UseFunc!=useF_Use_Target_Give)&&
          (kind!=Enable||source.e_UseFunc!=useF_target_activate_use)&&
          (kind!=Disable||source.e_UseFunc!=useF_target_deactivate_use))return false;
 value.kind=kind;
 value.enabled=G_LogicUseEnabled(source);
 return Valid(value);
}

inline bool G_LogicSet(gentity_t& source,const jar::logic::State& value) {
 jar::logic::State previous;
 if(!G_LogicRead(source,previous) || previous.kind!=value.kind || !jar::logic::Valid(value))return false;
 if(value.kind==jar::logic::Delay||value.kind==jar::logic::Timer){
  source.wait=value.delayMs/1000.f;
  source.random=value.jitterMs/1000.f;
 }else if(value.kind==jar::logic::Relay){
  source.delay=value.delayMs;
  source.wait=float(value.cooldownMs);
  if(value.repeat)source.spawnflags&=~8;
  else source.spawnflags|=8;
  source.count=value.remaining?0:1;
 }else if(value.kind==jar::logic::Score){
  source.count=value.points;
 }else if(value.kind==jar::logic::Kill){
  source.spawnflags=(source.spawnflags&~3)|(value.killMode==jar::logic::Falling?1:value.killMode==jar::logic::Electrical?2:0);
 }else if(value.kind==jar::logic::Random){
  if(value.repeat)source.spawnflags&=~1;
  else source.spawnflags|=1;
  source.count=value.remaining?0:1;
 }else if(value.kind==jar::logic::Counter){
  source.max_health=value.hits;
  source.count=value.remaining;
  source.bounceCount=value.repeat?-1:0;
 }
 G_LogicSetUseEnabled(source,value.enabled);
 return true;
}
#endif
