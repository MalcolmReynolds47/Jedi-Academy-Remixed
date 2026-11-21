// Editable model-backed Sandbox breakables. GPL-2.0-only.
#pragma once
#include <cstring>

namespace jar { namespace sandboxBreakable {
enum Kind { None, Brush, Glass, Model };
enum Phase { Intact=1, Pending, Broken };
enum Damage { Normal, Saber, Heavy, UseOnly, DamageCount };
constexpr int maxHealth=10000,maxFuse=60000,maxChunks=300,maxSplash=1000,maxRadius=1024,materialCount=16;
inline Kind Class(const char* name){
 if(!name)return None;
 if(!std::strcmp(name,"func_breakable"))return Brush;
 if(!std::strcmp(name,"func_glass"))return Glass;
 if(!std::strcmp(name,"misc_model_breakable"))return Model;
 return None;
}
struct Settings {
 int capacity=0,health=0,fuse=0,remaining=0,material=0,chunks=100,splash=0,radius=0;
 Phase phase=Intact;
 Damage mode=Normal;
 bool enabled=true,playerUse=false,explosion=true;
};
inline bool Valid(const Settings& s){
 return s.capacity>=1&&s.capacity<=maxHealth&&s.health>=0&&s.health<=s.capacity&&
  s.fuse>=0&&s.fuse<=maxFuse&&s.phase>=Intact&&s.phase<=Broken&&
  (s.phase==Intact?s.health>0:s.health==0)&&
  (s.phase==Pending?s.remaining>=1&&s.remaining<=maxFuse:s.remaining==0)&&
  s.mode>=Normal&&s.mode<DamageCount&&s.material>=0&&s.material<materialCount&&
  s.chunks>=0&&s.chunks<=maxChunks&&s.splash>=0&&s.splash<=maxSplash&&s.radius>=0&&s.radius<=maxRadius;
}
inline bool Same(const Settings& a,const Settings& b){
 return a.capacity==b.capacity&&a.health==b.health&&a.fuse==b.fuse&&a.material==b.material&&
  a.chunks==b.chunks&&a.splash==b.splash&&a.radius==b.radius&&a.phase==b.phase&&a.mode==b.mode&&
  a.enabled==b.enabled&&a.playerUse==b.playerUse&&a.explosion==b.explosion;
}
inline const char* Mode(int n){
 const char* labels[]={"Any weapon","Saber only","Heavy weapons","Use only"};
 return n>=Normal&&n<DamageCount?labels[n]:labels[Normal];
}
inline const char* Material(int n){
 const char* labels[]={"Metal","Glass","Electrical","Electrical metal","Dark stone","Light stone","Glass and metal","Metal 2",
  "None","Grey stone","Metal 3","Crate","Grate","Rope","Crate 2","White metal"};
 return n>=0&&n<materialCount?labels[n]:labels[0];
}
} }
