// Model-backed Sandbox doors and lifts. Authored BSP movers keep their callbacks.
#include "g_sliding_door.h"
#include "g_workshop.h"
#include <algorithm>
#include <cmath>

bool G_SandboxSlideClear(const gentity_t& door,const vec3_t origin,const vec3_t angles)
{
 vec3_t mins,maxs;G_SandboxSlideHull(angles,mins,maxs,G_LiftPortable(door));
 trace_t trace{};
 gi.trace(&trace,origin,mins,maxs,origin,door.s.number,MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
 return !trace.startsolid && !trace.allsolid;
}

bool G_SandboxSlideMoved(gentity_t& door)
{
 if(!G_SandboxSlidePortable(door))return false;
 G_SandboxSlideHull(door.currentAngles,door.mins,door.maxs,G_LiftPortable(door));
 VectorCopy(door.currentAngles,door.s.angles);
 VectorCopy(door.currentOrigin,door.s.origin);
 door.painDebounceTime=level.time;
 door.s.loopSound=0;
 return true;
}

bool G_SandboxSlideRead(const gentity_t& door,jar::slidingDoor::State& state)
{
 if(!G_SandboxSlidePortable(door)||!std::isfinite(door.speed)||!std::isfinite(door.wait)||
    door.speed<JAR_SLIDE_MIN_SPEED||door.speed>JAR_SLIDE_MAX_SPEED||
    door.wait<0||door.wait>JAR_SLIDE_MAX_WAIT||std::floor(door.speed)!=door.speed||
    std::floor(door.wait)!=door.wait)return false;
 state.settings={door.damage,(int)door.speed,door.bounceCount,(int)door.wait};
 state.extension=door.count;
 state.remainingWait=door.delay;
 state.open=door.alt_fire!=qfalse;
 state.inactive=(door.svFlags&SVF_INACTIVE)!=0;
 return jar::slidingDoor::Valid(state)&&(!G_LiftPortable(door)||state.settings.direction==JAR_SLIDE_UP);
}

bool G_SandboxSlideRestore(gentity_t& door,const jar::slidingDoor::State& state)
{
 if(!G_SandboxSlidePortable(door)||!jar::slidingDoor::Valid(state)||
    (G_LiftPortable(door)&&state.settings.direction!=JAR_SLIDE_UP))return false;
 const auto& s=state.settings;
 door.damage=s.travel;door.speed=s.speed;door.bounceCount=s.direction;door.wait=s.waitMs;
 door.count=state.extension;door.delay=state.remainingWait;door.alt_fire=state.open?qtrue:qfalse;
 if(state.inactive)door.svFlags|=SVF_INACTIVE;else door.svFlags&=~SVF_INACTIVE;
 door.nextthink=level.time+1;
 door.fly_sound_debounce_time=JAR_SLIDE_REST;
 return G_SandboxSlideMoved(door);
}

bool G_SandboxSlideSettings(gentity_t& door,const jar::slidingDoor::Settings& settings)
{
 jar::slidingDoor::State state;
 if(!G_SandboxSlideRead(door,state)||
    (state.extension&&state.settings.direction!=settings.direction))return false;
 if(!jar::slidingDoor::Valid({settings,state.extension,0,state.open,state.inactive}))return false;
 if(!state.open||state.extension!=settings.travel*JAR_SLIDE_MILLI)state.remainingWait=0;
 else if(state.settings.waitMs!=settings.waitMs)state.remainingWait=settings.waitMs;
 state.settings=settings;
 return G_SandboxSlideRestore(door,state);
}

bool G_SandboxSlideSpawn(gentity_t& door)
{
 if(!G_SandboxSlidePortable(door)||!G_SandboxSlideClear(door,door.currentOrigin,door.currentAngles))return false;
 door.s.eType=ET_GENERAL;
 door.bmodel=qfalse;
 door.contents=CONTENTS_SOLID;
 door.takedamage=qfalse;
 door.health=0;
 const bool lift=G_LiftPortable(door);
 door.s.modelindex=G_ModelIndex(lift?JAR_LIFT_MODEL:JAR_SLIDING_DOOR_MODEL);
 door.sound1to2=G_SoundIndex(lift?JAR_LIFT_START_SOUND:JAR_DOOR_START_SOUND);
 door.soundLoop=G_SoundIndex(lift?JAR_LIFT_MOVE_SOUND:JAR_DOOR_MOVE_SOUND);
 door.soundPos2=G_SoundIndex(lift?JAR_LIFT_STOP_SOUND:JAR_DOOR_STOP_SOUND);
 door.e_UseFunc=useF_portable_sliding_door_use;
 door.e_ThinkFunc=thinkF_portable_sliding_door_think;
 jar::slidingDoor::State state;
 if(lift){state.settings.travel=JAR_LIFT_HEIGHT;state.settings.waitMs=JAR_LIFT_WAIT;}
 return G_SandboxSlideRestore(door,state);
}

void portable_sliding_door_use(gentity_t* door,gentity_t*,gentity_t* activator)
{
 if(!G_SandboxSlidePortable(*door)||(door->svFlags&SVF_INACTIVE)||G_JAR_WorkshopControls(door))return;
 door->alt_fire=door->alt_fire?qfalse:qtrue;
 door->activator=activator;
 door->delay=0;
 door->painDebounceTime=level.time;
 door->nextthink=level.time+1;
}

void portable_sliding_door_think(gentity_t* door)
{
 jar::slidingDoor::State state;
 if(!G_SandboxSlideRead(*door,state)){door->s.loopSound=0;return;}
 const int elapsed=std::min(100,std::max(0,level.time-door->painDebounceTime));
 door->painDebounceTime=level.time;
 door->nextthink=level.time+1;
 if(state.inactive){
  door->s.loopSound=0;door->fly_sound_debounce_time=JAR_SLIDE_PAUSED;return;
 }
 if(door->delay>0){
  door->delay=std::max(0,door->delay-elapsed);
  if(door->delay)return;
  door->alt_fire=qfalse;
 }
 const int target=door->alt_fire?state.settings.travel*JAR_SLIDE_MILLI:0;
 const int remaining=target-door->count;
 if(!remaining){door->s.loopSound=0;door->fly_sound_debounce_time=JAR_SLIDE_REST;door->nextthink=0;return;}
 if(!elapsed)return;
 const int limit=state.settings.speed*elapsed;
 const int step=std::min(limit,std::max(-limit,remaining));
 bool blocked=false;int advance=step;
 if(G_LiftPortable(*door)){
  blocked=!G_LiftTranslate(*door,float(step)/JAR_SLIDE_MILLI);
  if(blocked)advance=0;
 }else{
  vec3_t right,up,axis,next;
  AngleVectors(door->currentAngles,nullptr,right,up);
  if(state.settings.direction==JAR_SLIDE_LEFT||state.settings.direction==JAR_SLIDE_RIGHT)
   VectorScale(right,state.settings.direction==JAR_SLIDE_LEFT?-1:1,axis);
  else VectorScale(up,state.settings.direction==JAR_SLIDE_DOWN?-1:1,axis);
  VectorMA(door->currentOrigin,float(step)/JAR_SLIDE_MILLI,axis,next);
  trace_t trace{};
  gi.trace(&trace,door->currentOrigin,door->mins,door->maxs,next,door->s.number,MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
  blocked=trace.startsolid||trace.allsolid||trace.fraction<1;
  advance=trace.startsolid||trace.allsolid?0:blocked?int(step*std::max(0.f,trace.fraction)):step;
  if(advance){
   VectorMA(door->currentOrigin,float(advance)/JAR_SLIDE_MILLI,axis,next);
   G_SetOrigin(door,next);VectorCopy(next,door->s.origin);
   door->s.pos.trType=TR_INTERPOLATE;
   gi.linkentity(door);
  }
 }
 if(advance){
  door->count+=advance;
  if(!door->s.loopSound&&door->sound1to2)G_Sound(door,door->sound1to2);
 }
 if(door->count==target){
  door->s.loopSound=0;
  if(door->soundPos2)G_Sound(door,door->soundPos2);
  door->delay=door->alt_fire?state.settings.waitMs:0;
  door->fly_sound_debounce_time=JAR_SLIDE_REST;
  auto* activator=door->activator;
  if(!activator||!activator->inuse)activator=door;
  G_UseTargets2(door,activator,door->alt_fire?door->target:door->target2);
  return;
 }
 door->s.loopSound=blocked?0:door->soundLoop;
 door->fly_sound_debounce_time=blocked?JAR_SLIDE_BLOCKED:JAR_SLIDE_MOVING;
}
