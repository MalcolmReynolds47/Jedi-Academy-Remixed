// Value-only vehicle clipboard state. GPL-2.0-only.
#pragma once
#include "g_local.h"
#include "g_vehicles.h"
#include "qcommon/jar_vehicle_status.h"

static_assert(MAX_VEHICLE_WEAPONS+MAX_VEHICLE_TURRETS==jar::vehicleStatus::ammoSlots,
              "Update the vehicle clipboard ammo layout");

inline bool G_WorkshopCaptureVehicleStatus(const gentity_t& hull,jar::vehicleStatus::Values& status){
 const auto* vehicle=hull.m_pVehicle;
 if(!vehicle||!hull.client||!hull.takedamage||hull.health<=0||vehicle->m_pParentEntity!=&hull||
    !vehicle->m_pVehicleInfo||vehicle->m_iDieTime||vehicle->m_iRemovedSurfaces)return false;
 status.value[0]=vehicle->m_iArmor;
 status.value[1]=hull.client->ps.stats[STAT_ARMOR];
 for(int i=0;i<MAX_VEHICLE_WEAPONS;++i)status.value[2+i]=vehicle->weaponStatus[i].ammo;
 for(int i=0;i<MAX_VEHICLE_TURRETS;++i)status.value[2+MAX_VEHICLE_WEAPONS+i]=vehicle->turretStatus[i].ammo;
 return jar::vehicleStatus::Valid(status);
}

inline jar::vehicleStatus::Values G_WorkshopVehicleLimits(const vehicleInfo_t& info){
 jar::vehicleStatus::Values limits;
 limits.value[0]=info.armor;
 limits.value[1]=info.shields;
 for(int i=0;i<MAX_VEHICLE_WEAPONS;++i)limits.value[2+i]=info.weapon[i].ammoMax;
 for(int i=0;i<MAX_VEHICLE_TURRETS;++i)limits.value[2+MAX_VEHICLE_WEAPONS+i]=info.turret[i].iAmmoMax;
 for(int& value:limits.value)value=std::max(0,std::min(jar::vehicleStatus::maximum,value));
 return limits;
}

inline bool G_WorkshopSetVehicleResources(gentity_t& hull,const jar::vehicleStatus::Values& status){
 auto* vehicle=hull.m_pVehicle;
 if(!vehicle||!hull.client||vehicle->m_pParentEntity!=&hull||!vehicle->m_pVehicleInfo||!jar::vehicleStatus::Valid(status))return false;
 auto& ps=hull.client->ps;
 vehicle->m_iArmor=status.value[0];
 vehicle->m_iShields=ps.stats[STAT_ARMOR]=status.value[1];
 for(int i=0;i<MAX_VEHICLE_WEAPONS;++i)vehicle->weaponStatus[i].ammo=ps.ammo[i]=status.value[2+i];
 for(int i=0;i<MAX_VEHICLE_TURRETS;++i){
  const int index=MAX_VEHICLE_WEAPONS+i;
  vehicle->turretStatus[i].ammo=ps.ammo[index]=status.value[2+index];
 }
 return true;
}

inline bool G_WorkshopApplyVehicleStatus(gentity_t& hull,const jar::vehicleStatus::Values& status){
 if(!G_WorkshopSetVehicleResources(hull,status))return false;
 auto& vehicle=*hull.m_pVehicle;
 for(auto& slot:vehicle.weaponStatus)slot.lastAmmoInc=level.time;
 for(auto& slot:vehicle.turretStatus)slot.lastAmmoInc=level.time;
 vehicle.lastShieldInc=level.time;
 VectorCopy(hull.currentAngles,vehicle.m_vOrientation);
 VectorCopy(hull.currentAngles,vehicle.m_vPrevOrientation);
 return true;
}
