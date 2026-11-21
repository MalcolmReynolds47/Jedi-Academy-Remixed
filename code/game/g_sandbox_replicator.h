// Portable replicators use native entity fields and callbacks in saves.
#pragma once
#include "g_local.h"
#include "g_functions.h"
#include "g_workshop.h"
#include "qcommon/jar_replicator.h"
#include "../../shared/qcommon/jar_logic_marker.h"
#include <algorithm>

inline bool G_SandboxReplicator(const gentity_t& e){
 return e.classname&&!strcmp(e.classname,"misc_replicator_item")&&G_JAR_WorkshopOwns(&e);
}
inline int G_ReplicatorHeal(const gentity_t& e){return e.wait>0?e.damage:jar::replicator::defaultHeal;}
inline int G_ReplicatorDelay(const gentity_t& e){return e.wait>0?int(e.wait):jar::replicator::defaultDelay;}
inline void G_ReplicatorModels(gentity_t& e){
 e.bounceCount=G_ModelIndex("models/map_objects/nar_shaddar/cup.md3");
 e.fly_sound_debounce_time=G_ModelIndex("models/items/medpac.md3");
}
inline void G_ReplicatorApplyPhase(gentity_t& e,const jar::replicator::State& state,int clock){
 using namespace jar::replicator;
 e.s.eFlags&=~EF_NODRAW;
 if(state.phase==Idle){
  e.s.modelindex=G_ModelIndex(JAR_LOGIC_PLACEHOLDER_MODEL);
  e.e_UseFunc=useF_misc_replicator_item_spawn;
 }else{
  e.s.modelindex=state.variant?e.fly_sound_debounce_time:e.bounceCount;
  e.e_UseFunc=state.phase==Ready?useF_misc_replicator_item_remove:useF_NULL;
 }
 e.e_ThinkFunc=state.phase==Forming?thinkF_misc_replicator_item_finish_spawn:thinkF_NULL;
 e.nextthink=state.phase==Forming?clock+state.remaining:0;
}
inline jar::replicator::State G_ReplicatorPhase(const gentity_t& e,int clock){
 using namespace jar::replicator;
 const int variant=e.s.modelindex&&e.s.modelindex==e.fly_sound_debounce_time?1:0;
 if(e.e_ThinkFunc==thinkF_misc_replicator_item_finish_spawn&&e.nextthink>0)
  return {Forming,std::max(1,std::min(maxDelay,e.nextthink-clock)),variant};
 if(e.e_UseFunc==useF_misc_replicator_item_remove)return {Ready,0,variant};
 return {};
}
