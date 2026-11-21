// Stock Force Boon pickup and timing rules for single-player. GPL-2.0-only.
#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace jar { namespace forceBoon {
constexpr char classname[]="item_force_boon";
constexpr char title[]="Force Boon";
constexpr char model[]="models/map_objects/mp/force_boon.md3";
constexpr char icon[]="gfx/hud/mpi_fboon";
constexpr char sound[]="sound/player/boon.mp3";
constexpr int seconds=25;

inline int Deadline(int time,int previous,int duration){
 const int start=std::max(previous,time-time%1000);
 return int(std::min<int64_t>(std::numeric_limits<int>::max(),int64_t(start)+int64_t(duration)*1000));
}

inline int Drain(int expires,int time,int normal){
 return expires>time&&normal>0?std::max(1,normal/2):normal;
}

inline int Recharge(int expires,int time,int normal){
 return expires>time?6:normal;
}
} }
