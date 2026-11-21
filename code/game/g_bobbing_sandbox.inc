// The portable deck uses the lift's swept transaction for its support group.
#include "g_bobbing_sandbox.h"
#include "g_sliding_door.h"
#include "g_workshop.h"
#include <algorithm>
#include <cmath>

bool G_BobbingClear(const gentity_t& deck,const vec3_t origin,const vec3_t angles){
 vec3_t mins,maxs;G_BobbingHull(angles,mins,maxs);
 trace_t trace{};
 gi.trace(&trace,origin,mins,maxs,origin,deck.s.number,MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
 return !trace.startsolid&&!trace.allsolid;
}

namespace {
void BobbingRebase(gentity_t& deck){
 deck.pos1[2]=deck.pos2[2]=deck.currentOrigin[2];
 deck.bounceCount=deck.count;
 deck.delay=0;
}
}

bool G_BobbingMoved(gentity_t& deck){
 if(!G_BobbingPortable(deck))return false;
 G_BobbingHull(deck.currentAngles,deck.mins,deck.maxs);
 VectorCopy(deck.currentAngles,deck.s.angles);
 VectorCopy(deck.currentOrigin,deck.s.origin);
 VectorCopy(deck.currentOrigin,deck.s.pos.trBase);
 VectorClear(deck.s.pos.trDelta);
 deck.s.pos.trType=TR_INTERPOLATE;
 deck.painDebounceTime=level.time;
 BobbingRebase(deck);
 return true;
}

bool G_BobbingRead(const gentity_t& deck,jarBobState_t& state){
 if(!G_BobbingPortable(deck)||!(deck.speed>=JAR_BOB_MIN_AMPLITUDE&&deck.speed<=JAR_BOB_MAX_AMPLITUDE&&
    deck.wait>=JAR_BOB_MIN_PERIOD&&deck.wait<=JAR_BOB_MAX_PERIOD&&
    deck.speed==std::floor(deck.speed)&&deck.wait==std::floor(deck.wait)))return false;
 state={int(deck.speed),int(deck.wait),deck.count,deck.alt_fire!=qfalse,(deck.svFlags&SVF_INACTIVE)!=0};
 return JAR_BobValid(state)!=0;
}

bool G_BobbingRestore(gentity_t& deck,const jarBobState_t& state){
 if(!G_BobbingPortable(deck)||!JAR_BobValid(state))return false;
 deck.speed=state.amplitude;deck.wait=state.periodMs;deck.count=state.phase;
 deck.alt_fire=state.running?qtrue:qfalse;
 if(state.inactive)deck.svFlags|=SVF_INACTIVE;else deck.svFlags&=~SVF_INACTIVE;
 deck.nextthink=level.time+1;
 deck.fly_sound_debounce_time=JAR_BOB_STOPPED;
 return G_BobbingMoved(deck);
}

bool G_BobbingSettings(gentity_t& deck,const jarBobState_t& settings){
 jarBobState_t state{};
 if(!G_BobbingRead(deck,state))return false;
 const jarBobState_t next={settings.amplitude,settings.periodMs,state.phase,settings.running,settings.inactive};
 if(!JAR_BobValid(next))return false;
 if(next.amplitude!=state.amplitude)BobbingRebase(deck);
 deck.speed=next.amplitude;deck.wait=next.periodMs;
 deck.alt_fire=next.running?qtrue:qfalse;
 if(next.inactive)deck.svFlags|=SVF_INACTIVE;else deck.svFlags&=~SVF_INACTIVE;
 deck.painDebounceTime=level.time;
 return true;
}

namespace {
void BobbingInit(gentity_t& deck){
 deck.s.eType=ET_GENERAL;
 deck.bmodel=qfalse;deck.contents=CONTENTS_SOLID;deck.clipmask=MASK_PLAYERSOLID;
 deck.takedamage=qfalse;deck.health=0;
 deck.s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");
 deck.svFlags|=SVF_PLAYER_USABLE;
 deck.e_UseFunc=useF_portable_bobbing_use;
 deck.e_ThinkFunc=thinkF_portable_bobbing_think;
}
}

bool G_BobbingSpawn(gentity_t& deck){
 if(!G_BobbingPortable(deck)||!G_BobbingClear(deck,deck.currentOrigin,deck.currentAngles))return false;
 BobbingInit(deck);
 return G_BobbingRestore(deck,{JAR_BOB_AMPLITUDE,JAR_BOB_PERIOD,JAR_BOB_BOTTOM,1,0});
}

bool G_BobbingMigrate(gentity_t& deck,const trajectory_t& old,int sampleTime){
 if(!G_BobbingLegacy(deck))return false;
 jarBobState_t state={JAR_BOB_AMPLITUDE,JAR_BOB_PERIOD,JAR_BOB_BOTTOM,old.trType==TR_SINE,(deck.svFlags&SVF_INACTIVE)!=0};
 const float amplitude=VectorLength(old.trDelta);
 if(amplitude>=JAR_BOB_MIN_AMPLITUDE&&amplitude<=JAR_BOB_MAX_AMPLITUDE)state.amplitude=int(amplitude+.5f);
 if(old.trDuration>=JAR_BOB_MIN_PERIOD&&old.trDuration<=JAR_BOB_MAX_PERIOD){
  state.periodMs=old.trDuration;
  if(state.running){
   const int offset=int((int64_t(sampleTime)-old.trTime)%old.trDuration);
   state.phase=int(int64_t((offset+old.trDuration)%old.trDuration)*JAR_BOB_CYCLE/old.trDuration);
  }else if(std::isfinite(deck.radius)){
   const double phase=deck.radius-std::floor(double(deck.radius));
   state.phase=std::min(JAR_BOB_CYCLE-1,int(phase*JAR_BOB_CYCLE));
  }
 }
 if(deck.model&&gi.bIsFromZone(deck.model,TAG_G_ALLOC))gi.Free(deck.model);
 deck.model=G_NewString(JAR_BOB_PORTABLE);
 BobbingInit(deck);
 return G_BobbingRestore(deck,state);
}

void portable_bobbing_use(gentity_t* deck,gentity_t*,gentity_t*){
 if(!G_BobbingPortable(*deck)||(deck->svFlags&SVF_INACTIVE)||G_JAR_WorkshopControls(deck))return;
 deck->alt_fire=deck->alt_fire?qfalse:qtrue;
 deck->painDebounceTime=level.time;
}

namespace {
bool BobbingStep(gentity_t& deck,int phase){
 const float height=JAR_BobHeight(int(deck.speed),phase,deck.bounceCount,deck.pos1[2]);
 const float travel=height-deck.currentOrigin[2];
 if(travel&&!G_LiftTranslate(deck,travel)){
  // Clip a cycle at world geometry. A second blocked leg waits for clearance.
  // A rider or loose object blocking the transaction keeps the current phase.
  if(!deck.delay){
   vec3_t destination;VectorCopy(deck.currentOrigin,destination);destination[2]+=travel;
   trace_t hit{};
   gi.trace(&hit,deck.currentOrigin,deck.mins,deck.maxs,destination,deck.s.number,
            MASK_PLAYERSOLID|CONTENTS_CORPSE,(EG2_Collision)0,0);
   if(!hit.startsolid&&!hit.allsolid&&hit.fraction<1&&hit.entityNum==ENTITYNUM_WORLD&&
      (travel<0?hit.plane.normal[2]>=.7f:hit.plane.normal[2]<=-.7f)){
    deck.count=travel<0?JAR_BOB_BOTTOM:JAR_BOB_TOP;
    BobbingRebase(deck);
    deck.delay=1;
   }
  }
  return false;
 }
 if(travel)deck.delay=0;
 deck.pos2[2]=deck.currentOrigin[2];
 deck.count=phase;
 return true;
}
}

void portable_bobbing_think(gentity_t* deck){
 jarBobState_t state{};
 if(!G_BobbingRead(*deck,state))return;
 const int elapsed=std::min(100,std::max(0,level.time-deck->painDebounceTime));
 deck->painDebounceTime=level.time;
 deck->nextthink=level.time+1;
 if(state.inactive||!state.running||G_JAR_WorkshopControls(deck)){
  deck->fly_sound_debounce_time=JAR_BOB_STOPPED;
  return;
 }
 if(!elapsed)return;
 if(deck->pos2[2]!=deck->currentOrigin[2])BobbingRebase(*deck);
 const int target=state.phase+elapsed*JAR_BOB_CYCLE/state.periodMs;
 const int turn=JAR_BobNextTurn(state.phase);
 const bool blocked=(target>turn&&!BobbingStep(*deck,turn%JAR_BOB_CYCLE))||
                    !BobbingStep(*deck,target%JAR_BOB_CYCLE);
 deck->fly_sound_debounce_time=blocked?JAR_BOB_BLOCKED:JAR_BOB_MOVING;
}
