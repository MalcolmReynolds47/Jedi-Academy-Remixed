#pragma once
#include "qcommon/jar_button.h"

constexpr char JAR_BUTTON_PORTABLE_MODEL[]="@jar_button";
inline bool G_ButtonClass(const char* name){return name&&!strcmp(name,"func_button");}
inline bool G_ButtonPortable(const gentity_t& e){
 return e.inuse&&G_ButtonClass(e.classname)&&e.model&&!strcmp(e.model,JAR_BUTTON_PORTABLE_MODEL);
}
inline void G_ButtonHull(const vec3_t angles,vec3_t mins,vec3_t maxs){
 // Union of all five frames, so the collision hull stays stable while pressing.
 const vec3_t low={-17.5f,-14,0},high={14.71875f,14,26.390625f};
 vec3_t axes[3];AnglesToAxis(angles,axes);
 for(int a=0;a<3;++a){
  mins[a]=maxs[a]=0;
  for(int i=0;i<3;++i){
   const float b=axes[i][a];
   mins[a]+=b*(b>=0?low[i]:high[i]);maxs[a]+=b*(b>=0?high[i]:low[i]);
  }
 }
}
bool G_ButtonClear(const gentity_t&,const vec3_t origin,const vec3_t angles);
bool G_ButtonMoved(gentity_t&);
bool G_ButtonSpawn(gentity_t&);
bool G_ButtonRestore(gentity_t&,const jarButtonState_t&);
bool G_ButtonRead(const gentity_t&,jarButtonState_t&);
bool G_ButtonSettings(gentity_t&,int waitMs,bool enabled);
