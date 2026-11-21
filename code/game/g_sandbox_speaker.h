// Portable speaker playback uses native sound events and saved entity fields. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "qcommon/jar_sandbox_speaker.h"
#include <algorithm>

constexpr int jarSpeakerActive=8;

inline bool G_SandboxSpeaker(const gentity_t& e){
 return e.inuse&&e.e_UseFunc==useF_Use_Target_Speaker&&jar::speaker::Class(e.classname)&&
  e.model&&!strcmp(e.model,jar::speaker::model);
}

inline bool G_SandboxSpeakerRead(const gentity_t& e,jar::speaker::Settings& s){
 using namespace jar::speaker;
 if(!G_SandboxSpeaker(e)||!(e.wait>=minDelay&&e.wait<=maxPeriod)||
    !(e.random>=0&&e.random<=e.wait-minDelay))return false;
 s={e.count,e.spawnflags&7,int(e.wait),int(e.random),!(e.svFlags&SVF_INACTIVE),
    (e.spawnflags&jarSpeakerActive)!=0,0};
 if(Timed(s))s.remaining=s.enabled?std::max(0,e.nextthink-level.time):e.fly_sound_debounce_time;
 return Valid(s);
}

inline int G_SandboxSpeakerIndex(int cue){
 const char* name=jar::speaker::cues[cue].path;
 for(int i=1;i<MAX_SOUNDS;++i){
  char existing[MAX_QPATH];
  gi.GetConfigstring(CS_SOUNDS+i,existing,sizeof(existing));
  if(!Q_stricmp(existing,name))return i;
  if(!existing[0]){
   gi.SetConfigstring(CS_SOUNDS+i,name);
   return i;
  }
 }
 return 0;
}

inline bool G_SandboxSpeakerSet(gentity_t& e,const jar::speaker::Settings& s){
 using namespace jar::speaker;
 if(!G_SandboxSpeaker(e)||!Valid(s))return false;
 const int index=e.count==s.cue&&e.noise_index>0&&e.noise_index<MAX_SOUNDS?
  e.noise_index:G_SandboxSpeakerIndex(s.cue);
 if(!index)return false;
 e.count=s.cue;
 e.noise_index=index;
 e.wait=s.period;
 e.random=s.variation;
 e.spawnflags=s.mode|(s.active?jarSpeakerActive:0);
 if(s.enabled)e.svFlags&=~SVF_INACTIVE;
 else e.svFlags|=SVF_INACTIVE;
 if(s.mode==Global)e.svFlags|=SVF_BROADCAST;
 else e.svFlags&=~SVF_BROADCAST;
 e.s.loopSound=s.enabled&&s.active&&s.mode==Loop?index:0;
 e.nextthink=s.enabled&&Timed(s)?level.time+std::max(1,s.remaining):0;
 e.fly_sound_debounce_time=!s.enabled&&Timed(s)?s.remaining:0;
 return true;
}

inline bool G_SandboxSpeakerInit(gentity_t& e,const jar::speaker::Settings& s){
 if(!jar::speaker::Class(e.classname)||!e.model||strcmp(e.model,jar::speaker::model))return false;
 e.e_UseFunc=useF_Use_Target_Speaker;
 e.e_ThinkFunc=thinkF_NULL;
 e.s.eType=ET_GENERAL;
 e.s.frame=e.s.clientNum=0;
 e.contents=CONTENTS_ITEM;
 e.svFlags=(e.svFlags&~SVF_NOCLIENT)|SVF_PLAYER_USABLE;
 e.noise_index=0;
 return G_SandboxSpeakerSet(e,s);
}

inline bool G_SandboxSpeakerRestore(gentity_t& e){
 using namespace jar::speaker;
 if(!Class(e.classname))return true;
 Settings s;
 if(G_SandboxSpeaker(e)){
  if(!G_SandboxSpeakerRead(e,s))return false;
 }else{
  if(e.e_UseFunc!=useF_Use_Target_Speaker)return false;
  // Earlier owned speakers automatically repeated the stock Bryar shot.
  s.enabled=!(e.svFlags&SVF_INACTIVE);
  if(e.s.frame>0){
   s.mode=Repeat;s.active=true;
   s.period=std::min(600,std::max(5,e.s.frame))*100;
   s.variation=std::min((s.period-minDelay)/100,std::max(0,e.s.clientNum))*100;
   s.remaining=s.period;
  }
  if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);
  e.model=G_NewString(model);
 }
 if(!G_SandboxSpeakerInit(e,s))return false;
 gi.linkentity(&e);
 return true;
}

inline void G_SandboxSpeakerEnable(gentity_t& e,bool enabled){
 jar::speaker::Settings settings;
 if(!G_SandboxSpeakerRead(e,settings)||settings.enabled==enabled)return;
 settings.enabled=enabled;
 G_SandboxSpeakerSet(e,settings);
}

void G_SandboxSpeakerTick(gentity_t& e);
