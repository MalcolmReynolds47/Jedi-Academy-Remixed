// Value-only clipboard data for a deployed portable shield. GPL-2.0-only.
#pragma once
#include "jar_volume.h"

namespace jar { namespace shieldCopy {
constexpr int lifetime=25000;
constexpr int maxHealth=100000;
struct Properties {
 int health=0,deadline=0;
 float left=0,right=0,height=0;
};
inline bool Same(const Properties& a,const Properties& b){
 return a.health==b.health&&a.deadline==b.deadline&&a.left==b.left&&a.right==b.right&&a.height==b.height;
}
struct State {
 float mins[3]{},maxs[3]{};
 int health=0,capacity=0;
 int remaining=0,nextCheck=0,rearm=0,damageFlash=0;
 bool raised=false;
};
inline bool ValidBounds(const float* mins,const float* maxs){
 return volume::Valid(mins,maxs)&&mins[0]==-4&&maxs[0]==4&&mins[2]==0&&
  mins[1]>=-254&&mins[1]<=-3&&maxs[1]>=3&&maxs[1]<=254&&maxs[2]>=32&&maxs[2]<=253;
}
inline bool Valid(const State& state){
 return ValidBounds(state.mins,state.maxs)&&state.health>0&&state.health<=maxHealth&&
  state.capacity>=0&&state.capacity<=maxHealth&&state.remaining>0&&state.remaining<=lifetime&&
  state.nextCheck>=0&&state.nextCheck<=500&&state.rearm>=0&&state.rearm<=500&&
  state.damageFlash>=0&&state.damageFlash<=200;
}
} }
