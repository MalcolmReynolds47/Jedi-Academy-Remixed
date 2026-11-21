// Native single-player vehicle and rider validation. GPL-2.0-only.
#pragma once

#include "g_logic_settings.h"
#include "g_vehicles.h"
#include "g_workshop.h"

inline bool G_VehicleEntityPointer(const gentity_t* entity){
 const auto begin=reinterpret_cast<uintptr_t>(g_entities),address=reinterpret_cast<uintptr_t>(entity);
 if(address<begin||address-begin>=sizeof(g_entities)||(address-begin)%sizeof(gentity_t))return false;
 const auto slot=(address-begin)/sizeof(gentity_t);
 return slot<unsigned(globals.num_entities)&&slot<ENTITYNUM_WORLD&&entity->s.number==int(slot)&&entity->inuse;
}

inline gentity_t* G_TransportVehicleHull(gentity_t* activator){
 if(!G_VehicleEntityPointer(activator)||!G_LogicLiveActor(activator))return nullptr;
 if(activator->m_pVehicle&&activator->client->NPC_class==CLASS_VEHICLE)return activator;
 const int slot=activator->s.m_iVehicleNum;
 if(slot<=0||slot>=globals.num_entities||slot>=ENTITYNUM_WORLD)return nullptr;
 auto& hull=g_entities[slot];
 if(!G_VehicleEntityPointer(&hull)||!hull.client||hull.client->NPC_class!=CLASS_VEHICLE||
    !hull.m_pVehicle||hull.m_pVehicle->m_pPilot!=activator||activator->owner!=&hull)return nullptr;
 return &hull;
}

using VehicleTransportAllowed=bool(*)(const gentity_t&);

inline bool G_TransportVehicleMember(const gentity_t& member,VehicleTransportAllowed allowed){
 return G_VehicleEntityPointer(&member)&&G_LogicLiveActor(&member)&&member.client->ps.pm_type==PM_NORMAL&&
  member.client->ps.vehTurnaroundTime<=level.time&&allowed(member);
}

inline bool G_TransportVehicleReady(const gentity_t& hull,VehicleTransportAllowed allowed=G_JAR_WorkshopTransportAllowed){
 if(!G_TransportVehicleMember(hull,allowed)||!hull.m_pVehicle||hull.client->NPC_class!=CLASS_VEHICLE)return false;
 const auto& vehicle=*hull.m_pVehicle;
 if(vehicle.m_pParentEntity!=&hull||!vehicle.m_pVehicleInfo||vehicle.m_iBoarding||vehicle.m_pOldPilot||
    vehicle.m_iDieTime||(vehicle.m_pPilot&&vehicle.m_pPilot==vehicle.m_pDroidUnit))return false;
 for(auto* rider:{vehicle.m_pPilot,vehicle.m_pDroidUnit}){
  if(rider&&(rider==&hull||!G_VehicleEntityPointer(rider)||!G_TransportVehicleMember(*rider,allowed)||rider->owner!=&hull||
     rider->s.m_iVehicleNum!=hull.s.number))return false;
 }
 return true;
}
