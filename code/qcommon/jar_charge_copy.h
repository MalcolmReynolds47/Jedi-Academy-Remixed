// Clipboard state for native charges planted on Sandbox props. GPL-2.0-only.
#pragma once
#include <cmath>

namespace jar { namespace chargeCopy {
enum class Mode { DetPack, DetonatingPack, Tripwire, Proximity, DetonatingMine };

constexpr int maxEditHealth=100000, maxEditDamage=10000, maxEditRadius=4096, maxEditTimeout=120000;
struct Properties {
 Mode mode=Mode::DetPack;
 int health=0,splash=0,radius=0,expiry=0;
};
inline bool Detonating(Mode mode){return mode==Mode::DetonatingPack||mode==Mode::DetonatingMine;}
inline bool Same(const Properties& a,const Properties& b){
 return a.mode==b.mode&&a.health==b.health&&a.splash==b.splash&&a.radius==b.radius&&a.expiry==b.expiry;
}

struct State {
 Mode mode=Mode::DetPack;
 int health=0,damage=0,splash=0,radius=0;
 int pending=0,timeout=0;
 bool shootable=false,armed=false,alternate=false;
 float origin[3]{},angles[3]{},beamStart[3]{};
};

inline bool Pack(Mode mode){return mode==Mode::DetPack||mode==Mode::DetonatingPack;}
inline float HalfSize(Mode mode){return Pack(mode)?10.f:4.f;}
inline bool Valid(const State& state){
 const int mode=int(state.mode);
 if(mode<int(Mode::DetPack)||mode>int(Mode::DetonatingMine)||state.health<0||
    state.damage<0||state.splash<0||state.radius<0||
    (state.mode==Mode::DetPack?state.pending!=0:state.pending<=0)||
    (state.mode==Mode::Proximity?state.timeout<0:state.timeout!=0))return false;
 for(int i=0;i<3;++i)if(!std::isfinite(state.origin[i])||std::fabs(state.origin[i])>=65536||
    !std::isfinite(state.angles[i])||std::fabs(state.angles[i])>=361||
    !std::isfinite(state.beamStart[i])||std::fabs(state.beamStart[i])>=65536)return false;
 return true;
}
} }
