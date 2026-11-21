// Portable Item Replicator settings and clipboard state. GPL-2.0-only.
#pragma once
namespace jar { namespace replicator {
constexpr int defaultHeal=5,maxHeal=100,defaultDelay=4000,minDelay=100,maxDelay=10000;
enum Phase { Idle, Forming, Ready };
struct State {
 int phase=Idle,remaining=0,variant=0;
};
inline bool Valid(const State& value){
 return value.phase>=Idle&&value.phase<=Ready&&value.variant>=0&&value.variant<2&&
  (value.phase==Forming?value.remaining>=1&&value.remaining<=maxDelay:value.remaining==0);
}
} }
