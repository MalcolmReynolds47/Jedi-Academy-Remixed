// Single-player Sandbox NPC orders, shared with the Context menu. GPL-2.0-only.
#pragma once

#include <cmath>
#include <cstdint>

namespace jar { namespace npcOrder {
enum Mode { Autonomous, Wait, Follow, Go, Patrol, ModeCount };
constexpr int minDistance=72,maxDistance=1024,defaultDistance=110;
constexpr const char* names[]={"Autonomous","Wait here","Follow player","Go to goal","Patrol route"};
inline bool UsesDestination(Mode mode){return mode==Go||mode==Patrol;}

struct Destination {
 uint32_t owner=0,slot=0;
 template<class Save> void sg_export(Save& save) const{
  save.template write<uint32_t>(owner);
  save.template write<uint32_t>(slot);
 }
 template<class Save> void sg_import(Save& save){
  save.template read<uint32_t>(owner);
  save.template read<uint32_t>(slot);
 }
};
inline bool Same(const Destination& a,const Destination& b){return a.owner==b.owner&&a.slot==b.slot;}
inline bool ValidDestination(const Destination& goal,uint32_t entityLimit){
 return (!goal.owner&&!goal.slot)||(goal.owner<0x80000000u&&goal.owner&&goal.slot&&goal.slot<entityLimit);
}

struct Settings {
 Mode mode=Autonomous;
 float distance=0;
};

inline bool Valid(const Settings& settings){
 return settings.mode>=Autonomous&&settings.mode<ModeCount&&std::isfinite(settings.distance)&&
  (settings.mode!=Follow||settings.distance==0||
   (settings.distance>=minDistance&&settings.distance<=maxDistance));
}

inline bool Merge(const Settings& current,const Settings& expected,const Settings& requested,Settings& output){
 output=current;
 if(expected.mode!=requested.mode){
  if(current.mode!=expected.mode)return false;
  output.mode=requested.mode;
 }
 if(expected.distance!=requested.distance){
  if(current.distance!=expected.distance)return false;
  output.distance=requested.distance;
 }
 return Valid(output);
}
}}
