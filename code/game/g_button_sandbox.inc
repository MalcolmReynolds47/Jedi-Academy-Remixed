#include "g_button_sandbox.h"
#include "g_workshop.h"
#include <algorithm>
#include <cmath>

bool G_ButtonClear(const gentity_t& button,const vec3_t origin,const vec3_t angles){
 vec3_t lo,hi;G_ButtonHull(angles,lo,hi);
 trace_t tr{};
 gi.trace(&tr,origin,lo,hi,origin,button.s.number,MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
 return !tr.startsolid&&!tr.allsolid;
}
bool G_ButtonMoved(gentity_t& button){
 if(!G_ButtonPortable(button))return false;
 G_ButtonHull(button.currentAngles,button.mins,button.maxs);
 VectorCopy(button.currentOrigin,button.s.origin);VectorCopy(button.currentAngles,button.s.angles);
 button.painDebounceTime=level.time;
 return true;
}
bool G_ButtonRead(const gentity_t& button,jarButtonState_t& state){
 if(!G_ButtonPortable(button)||!std::isfinite(button.wait)||button.wait<0||
    button.wait>JAR_BUTTON_MAX_WAIT||button.wait!=std::floor(button.wait))return false;
 state={(int)button.wait,button.count,button.s.frame,button.delay};
 return JAR_ButtonValid(state);
}
static void ButtonState(gentity_t& button,const jarButtonState_t& state){
 button.wait=state.waitMs;button.count=button.s.time=state.phase;button.s.frame=state.frame;button.delay=state.remaining;
 button.painDebounceTime=level.time;
 button.nextthink=state.phase==JAR_BUTTON_IDLE?0:level.time+1;
}
bool G_ButtonRestore(gentity_t& button,const jarButtonState_t& state){
 if(!G_ButtonPortable(button)||!JAR_ButtonValid(state))return false;
 ButtonState(button,state);return G_ButtonMoved(button);
}
bool G_ButtonSettings(gentity_t& button,int waitMs,bool enabled){
 jarButtonState_t state;
 if(!G_ButtonRead(button,state)||waitMs<0||waitMs>JAR_BUTTON_MAX_WAIT)return false;
 if(state.phase==JAR_BUTTON_HELD){
  if(!waitMs){state.phase=JAR_BUTTON_RETURNING;state.remaining=JAR_BUTTON_TICK;}
  else state.remaining=std::min(state.remaining,waitMs);
 }
 state.waitMs=waitMs;
 ButtonState(button,state);
 if(enabled)button.svFlags&=~SVF_INACTIVE;else button.svFlags|=SVF_INACTIVE;
 return true;
}
bool G_ButtonSpawn(gentity_t& button){
 if(!G_ButtonPortable(button)||!G_ButtonClear(button,button.currentOrigin,button.currentAngles))return false;
 button.s.eType=ET_GENERAL;button.bmodel=qfalse;button.contents=CONTENTS_SOLID;
 button.takedamage=qfalse;button.health=0;button.s.modelindex=G_ModelIndex(JAR_BUTTON_MODEL);
 button.sound1to2=G_SoundIndex(JAR_BUTTON_PRESS_SOUND);
 button.sound2to1=G_SoundIndex(JAR_BUTTON_RETURN_SOUND);
 button.e_UseFunc=useF_portable_button_use;button.e_ThinkFunc=thinkF_portable_button_think;
 return G_ButtonRestore(button,{JAR_BUTTON_WAIT,JAR_BUTTON_IDLE,0,0});
}
void portable_button_use(gentity_t* button,gentity_t*,gentity_t* activator){
 jarButtonState_t state;
 if(!G_ButtonRead(*button,state)||state.phase!=JAR_BUTTON_IDLE||(button->svFlags&SVF_INACTIVE)||
    G_JAR_WorkshopControls(button))return;
 button->activator=activator;
 state.phase=JAR_BUTTON_PRESSING;state.remaining=JAR_BUTTON_TICK;ButtonState(*button,state);
 if(button->sound1to2)G_Sound(button,button->sound1to2);
}
void portable_button_think(gentity_t* button){
 jarButtonState_t state;
 if(!G_ButtonRead(*button,state)||state.phase==JAR_BUTTON_IDLE)return;
 const int elapsed=std::min(100,std::max(0,level.time-button->painDebounceTime));
 const int events=(button->svFlags&SVF_INACTIVE)?0:JAR_ButtonAdvance(&state,elapsed);
 ButtonState(*button,state);
 if((events&JAR_BUTTON_RETURN)&&button->sound2to1)G_Sound(button,button->sound2to1);
 if(events&(JAR_BUTTON_PRESSED|JAR_BUTTON_RELEASED)){
  auto* actor=button->activator&&button->activator->inuse?button->activator:button;
  if(events&JAR_BUTTON_RELEASED)button->activator=nullptr;
  G_UseTargets2(button,actor,events&JAR_BUTTON_PRESSED?button->target:button->target2);
 }
}
