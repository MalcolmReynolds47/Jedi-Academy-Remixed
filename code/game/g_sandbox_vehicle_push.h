// Keep launch drift independent of native throttle and steering. GPL-2.0-only.
#pragma once

#include "qcommon/jar_push.h"

constexpr float JAR_VEHICLE_DRIFT_LIMIT=4.f*jar::push::maxSpeed;
constexpr int JAR_VEHICLE_IMMOBILE_FLAGS=EF_FORCE_GRIPPED|EF_FORCE_DRAINED|EF_HELD_BY_RANCOR|
 EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE|EF_LOCKED_TO_WEAPON;

inline bool G_VehiclePushDriftValid(const vec3_t drift){
 return drift[2]==0&&std::isfinite(drift[0])&&std::isfinite(drift[1])&&
  VectorLengthSquared(drift)<=JAR_VEHICLE_DRIFT_LIMIT*JAR_VEHICLE_DRIFT_LIMIT;
}

inline bool G_VehiclePushDrift(const playerState_t& ps,const vec3_t previous,const vec3_t velocity,
 bool additive,vec3_t drift){
 VectorSet(drift,velocity[0],velocity[1],0);
 if(additive)for(int axis=0;axis<2;++axis)drift[axis]-=ps.velocity[axis]-previous[axis];
 return G_VehiclePushDriftValid(drift);
}
