// Native single-player shield recharge. GPL-2.0-only.
#pragma once
#include "g_vehicles.h"
#include <cstdint>

inline void G_VehicleRechargeShields(Vehicle_t& vehicle,playerState_t& state,int commandTime){
 const auto& info=*vehicle.m_pVehicleInfo;
 int& shields=state.stats[STAT_ARMOR];
 if(info.shieldRechargeMS<=0||shields<=0||shields>=info.shields||
    int64_t(commandTime)-vehicle.lastShieldInc<info.shieldRechargeMS)return;
 vehicle.lastShieldInc=commandTime;
 vehicle.m_iShields=++shields;
}
