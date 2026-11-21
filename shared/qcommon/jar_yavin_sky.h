// Stock Yavin Training Grounds' detached sky geometry. GPL-2.0-only.
#pragma once
#include "jar_hoth_sky.h"
namespace jar { namespace yavinSky {
constexpr char resource[] = "#mp/duel6";
constexpr unsigned checksum = 0x87e84ef8u;
constexpr int arenaArea = 0;
constexpr int surfaceCount = 3449;
constexpr bool BackdropSurface(int index) {
    return (index >= 4 && index <= 6) || (index >= 3438 && index <= 3443);
}
} }

namespace jar { namespace sectionSky {
inline bool SubmitSurface(int count,int index,bool backdrop){
    if(count==hothSky::surfaceCount)return hothSky::BackdropSurface(index)==backdrop;
    if(count==yavinSky::surfaceCount)return yavinSky::BackdropSurface(index)==backdrop;
    return !backdrop;
}
} }
