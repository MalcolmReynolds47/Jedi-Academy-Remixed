// Native fields own settings and phase for Sandbox contact volumes.
#ifndef G_CONTACT_TRIGGER_H
#define G_CONTACT_TRIGGER_H
#include "g_volume_geometry.h"
#include "g_functions.h"

void G_ContactTouchAt(gentity_t& trigger,gentity_t* actor,const vec3_t origin);

inline jar::contactTrigger::Kind G_ContactKind(const gentity_t& trigger){
 return jar::volumeEntity::Portable(trigger)?jar::contactTrigger::Class(trigger.classname):jar::contactTrigger::None;
}
inline bool G_ContactRead(const gentity_t& trigger,jar::contactTrigger::Settings& settings){
 using namespace jar::contactTrigger;
 const auto kind=G_ContactKind(trigger);
 if(kind==Hurt){
  if(trigger.e_UseFunc!=useF_hurt_use||trigger.e_TouchFunc!=touchF_hurt_touch||
     !std::isfinite(trigger.wait)||trigger.wait<minInterval||trigger.wait>maxTime)return false;
  settings={Hurt,0,int(std::lround(trigger.wait)),0,trigger.spawnflags&2?Player:trigger.spawnflags&4096?NPCs:Actors,
   bool(trigger.spawnflags&8192),!(trigger.svFlags&SVF_INACTIVE),trigger.damage,
   trigger.spawnflags&32?Falling:trigger.spawnflags&64?Electrical:Normal,bool(trigger.contents&CONTENTS_TRIGGER)};
  return Valid(settings);
 }
 if(!kind||trigger.e_UseFunc!=useF_Use_Multi||trigger.e_TouchFunc!=touchF_Touch_Multi)return false;
 if(kind==Repeat&&(!std::isfinite(trigger.wait)||trigger.wait<0||trigger.wait>60||
    !std::isfinite(trigger.random)||trigger.random<0||trigger.random>60))return false;
 settings={kind,trigger.delay,kind==Repeat?int(std::lround(trigger.wait*1000)):0,
  kind==Repeat?int(std::lround(trigger.random*1000)):0,
  trigger.spawnflags&1?Player:trigger.spawnflags&16?NPCs:Actors,
  bool(trigger.spawnflags&4),!(trigger.svFlags&SVF_INACTIVE)};
 return Valid(settings);
}
inline bool G_ContactSettings(gentity_t& trigger,const jar::contactTrigger::Settings& settings){
 using namespace jar::contactTrigger;
 if(!Valid(settings)||settings.kind!=G_ContactKind(trigger))return false;
 bool relink=false;
 if(settings.kind==Hurt){
  trigger.wait=settings.interval;trigger.damage=settings.damage;
  // Retain the native damage mode flags and silence. Portable contact-only
  // flags occupy bits unused by the authored trigger.
  trigger.spawnflags=4|(settings.subject==Player?2:settings.subject==NPCs?4096:0)|
   (settings.useRequired?8192:0)|(settings.mode==Falling?32:settings.mode==Electrical?64:0);
  relink=settings.armed!=bool(trigger.contents&CONTENTS_TRIGGER);
  trigger.contents=settings.armed?CONTENTS_TRIGGER:0;
 }else{
  trigger.delay=settings.delay;
  trigger.wait=settings.kind==Once?-1.f:settings.interval/1000.f;
  trigger.random=settings.variation/1000.f;
  trigger.spawnflags=(trigger.spawnflags&~21)|(settings.subject==Player?1:settings.subject==NPCs?16:0)|
   (settings.useRequired?4:0);
 }
 if(settings.enabled)trigger.svFlags&=~SVF_INACTIVE;
 else trigger.svFlags|=SVF_INACTIVE;
 if(relink)gi.linkentity(&trigger);
 return true;
}
inline bool G_ContactSpent(const gentity_t& trigger){
 return G_ContactKind(trigger)==jar::contactTrigger::Once&&trigger.count!=0;
}
inline void G_ContactSetSpent(gentity_t& trigger,bool spent){
 if(G_ContactKind(trigger)!=jar::contactTrigger::Once)return;
 trigger.count=spent?1:0;
 if(spent)trigger.contents&=~CONTENTS_TRIGGER;
 else trigger.contents|=CONTENTS_TRIGGER;
 gi.linkentity(&trigger);
}
inline bool G_ContactPending(const gentity_t& trigger){
 return trigger.e_ThinkFunc==thinkF_multi_trigger_run&&trigger.nextthink>0;
}
inline bool G_ContactInitialize(gentity_t& trigger){
 using namespace jar::contactTrigger;
 const auto kind=Class(trigger.classname);
 if(!kind||trigger.bmodel||!trigger.model||strcmp(trigger.model,"@jar_box")||
    !jar::volume::Valid(trigger.mins,trigger.maxs))return false;
 const bool spent=kind==Once&&trigger.e_TouchFunc==touchF_NULL&&trigger.e_UseFunc==useF_NULL;
 const bool pending=kind!=Hurt&&G_ContactPending(trigger)&&!spent;
 const int ready=kind==Repeat&&!pending&&trigger.nextthink>0?trigger.nextthink:0;
 Settings settings{kind};
 settings.delay=trigger.delay;
 settings.interval=kind==Repeat?1000:0;
 if(kind==Repeat&&std::isfinite(trigger.wait)&&trigger.wait>=.1f&&trigger.wait<=60){
  settings.interval=int(std::lround(trigger.wait*1000));
  if(std::isfinite(trigger.random)&&trigger.random>=0&&trigger.random<=60)
   settings.variation=int(std::lround(trigger.random*1000));
 }
 settings.subject=trigger.spawnflags&1?Player:trigger.spawnflags&16?NPCs:Actors;
 settings.useRequired=(trigger.spawnflags&4)!=0;
 settings.enabled=!(trigger.svFlags&SVF_INACTIVE);
 if(kind==Hurt){
  const int interval=std::isfinite(trigger.wait)&&trigger.wait>=minInterval&&trigger.wait<=maxTime?
   int(std::lround(trigger.wait)):1000;
  settings={Hurt,0,interval,0,trigger.spawnflags&2?Player:Actors,false,
   settings.enabled,trigger.damage>=1&&trigger.damage<=maxDamage?trigger.damage:defaultDamage,
   trigger.spawnflags&32?Falling:trigger.spawnflags&64?Electrical:Normal,bool(trigger.linked)};
  trigger.delay=0;trigger.attackDebounceTime=0;
 }
 if(!Valid(settings))return false;
 if(gi.bIsFromZone(trigger.model,TAG_G_ALLOC))gi.Free(trigger.model);
 trigger.model=G_NewString(model);
 VectorCopy(trigger.mins,trigger.pos1);VectorCopy(trigger.maxs,trigger.pos2);
 trigger.e_UseFunc=kind==Hurt?useF_hurt_use:useF_Use_Multi;
 trigger.e_TouchFunc=kind==Hurt?touchF_hurt_touch:touchF_Touch_Multi;
 trigger.svFlags&=~SVF_PLAYER_USABLE;
 trigger.count=spent?1:0;
 trigger.painDebounceTime=ready;
 if(!pending){trigger.nextthink=0;trigger.e_ThinkFunc=thinkF_NULL;trigger.activator=nullptr;}
 if(spent)trigger.contents&=~CONTENTS_TRIGGER;
 if(!G_ContactSettings(trigger,settings)||!jar::volumeEntity::Refresh(trigger))return false;
 gi.linkentity(&trigger);
 return true;
}
inline bool G_ContactRestore(gentity_t& trigger){
 if(!jar::contactTrigger::Class(trigger.classname))return true;
 if(!G_ContactKind(trigger))return G_ContactInitialize(trigger);
 jar::contactTrigger::Settings settings;
 if(!G_ContactRead(trigger,settings)||!jar::volumeEntity::Refresh(trigger)||
    (trigger.e_ThinkFunc!=thinkF_NULL&&trigger.e_ThinkFunc!=thinkF_multi_trigger_run)||
    (settings.kind==jar::contactTrigger::Hurt&&trigger.e_ThinkFunc!=thinkF_NULL)||
    (trigger.e_ThinkFunc==thinkF_multi_trigger_run&&!G_ContactPending(trigger))||
    (G_ContactSpent(trigger)&&G_ContactPending(trigger)))return false;
 if(G_ContactSpent(trigger))trigger.contents&=~CONTENTS_TRIGGER;
 gi.linkentity(&trigger);
 return true;
}
#endif
