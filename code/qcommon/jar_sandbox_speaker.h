// Stock sound choices and saved Workshop speaker settings. GPL-2.0-only.
#pragma once
#include <cstring>

namespace jar { namespace speaker {
constexpr char model[]="@jar_speaker";
enum Mode {Spatial, Global, Activator, Loop, Repeat, ModeCount};
struct Cue {const char* label;const char* path;bool loop;};
// Cue indices are stored in native saves. Append new entries at the end.
constexpr Cue cues[]={
 {"Bryar shot","sound/weapons/bryar/fire",false},
 {"Console button","sound/interface/button1",false},
 {"Alarm","sound/effects/mpalarm",false},
 {"Sparks","sound/ambience/spark1",false},
 {"Fire burst","sound/effects/fireburst",false},
 {"Water drain","sound/effects/water_drain",false},
 {"Outdoor wind","sound/ambience/outdoor/windy1_lp",true},
 {"Steam pipe","sound/ambience/pipe_steam_lp",true},
 {"Flowing water","sound/ambience/water_fast_lp",true},
 {"Campfire","sound/effects/fire_lp",true},
 {"Waterfall","sound/ambience/yavin/waterfall_medium_lp",true}
};
constexpr int cueCount=sizeof(cues)/sizeof(cues[0]);
constexpr int defaultLoopCue=6,minDelay=500,maxPeriod=60000,oneShotCooldown=200;

struct Settings {
 int cue=0,mode=Spatial,period=1000,variation=0;
 bool enabled=true,active=false;
 int remaining=0;
};

inline bool Class(const char* name){return name&&!std::strcmp(name,"target_speaker");}
inline bool Timed(const Settings& s){return s.mode==Repeat&&s.active;}
inline bool Valid(const Settings& s){
 return s.cue>=0&&s.cue<cueCount&&s.mode>=Spatial&&s.mode<ModeCount&&
  (s.mode!=Loop||cues[s.cue].loop)&&(!s.active||s.mode==Loop||s.mode==Repeat)&&
  s.period>=minDelay&&s.period<=maxPeriod&&s.variation>=0&&s.variation<=s.period-minDelay&&
  s.remaining>=0&&(Timed(s)?s.remaining<=s.period+s.variation:!s.remaining);
}
inline bool SameControl(const Settings& a,const Settings& b){
 return a.cue==b.cue&&a.mode==b.mode&&a.period==b.period&&a.variation==b.variation&&
  a.enabled==b.enabled&&a.active==b.active;
}
inline int CycleCue(int cue,int direction,int mode){
 do{cue=(cue+direction+cueCount)%cueCount;}while(mode==Loop&&!cues[cue].loop);
 return cue;
}
inline const char* ModeLabel(int mode){
 constexpr const char* labels[]={"Spatial shot","Global shot","On activator","Spatial loop","Spatial repeat"};
 return mode>=Spatial&&mode<ModeCount?labels[mode]:labels[Spatial];
}
}}
