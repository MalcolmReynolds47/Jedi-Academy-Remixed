// Editable model-backed func_usable settings. GPL-2.0-only.
#pragma once
#include <cstring>

namespace jar { namespace sandboxUsable {
enum Mode { Toggle, Pulse };
enum Damage { NoDamage, OnHit, AtZero, DamageCount };
enum DirectUse { LinkedOnly, PlayerOnly, NpcOnly, Both, DirectUseCount };
constexpr int maxHealth=10000,maxReuse=60000;
inline bool Class(const char* name){return name&&!std::strcmp(name,"func_usable");}
struct Settings {
 int capacity=0,health=0,reuse=0,remaining=0;
 Mode mode=Toggle;
 Damage damage=OnHit;
 DirectUse direct=Both;
 bool solid=true,pending=false,spent=false,enabled=true;
};
inline bool Valid(const Settings& s){
 return s.capacity>=0&&s.capacity<=maxHealth&&s.health>=0&&s.health<=s.capacity&&
  s.reuse>=0&&s.reuse<=maxReuse&&s.remaining>=0&&s.remaining<=maxReuse&&
  s.mode>=Toggle&&s.mode<=Pulse&&s.damage>=NoDamage&&s.damage<DamageCount&&
  s.direct>=LinkedOnly&&s.direct<DirectUseCount&&
  (!s.pending||(s.mode==Toggle&&!s.solid))&&(!(s.spent||s.remaining)||s.mode==Pulse)&&
  !(s.spent&&s.remaining);
}
inline bool Same(const Settings& a,const Settings& b){
 return a.capacity==b.capacity&&a.health==b.health&&a.reuse==b.reuse&&a.mode==b.mode&&a.damage==b.damage&&
  a.direct==b.direct&&a.solid==b.solid&&a.pending==b.pending&&a.spent==b.spent&&a.enabled==b.enabled&&
  (a.remaining>0)==(b.remaining>0);
}
inline const char* DamageLabel(int value){
 const char* labels[]={"Damage: Off","Damage: On hit","Damage: At zero"};
 return value>=NoDamage&&value<DamageCount?labels[value]:labels[NoDamage];
}
inline const char* DirectLabel(int value){
 const char* labels[]={"Direct Use: Off","Direct Use: Player","Direct Use: NPC","Direct Use: Both"};
 return value>=LinkedOnly&&value<DirectUseCount?labels[value]:labels[LinkedOnly];
}
} }
