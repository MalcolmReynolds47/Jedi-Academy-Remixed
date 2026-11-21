// Native Workshop thrust follows the hull instead of driving its pilot. GPL-2.0-only.
#pragma once

#include "g_sandbox_vehicle_push.h"
#include "g_sandbox_vehicle_transport.h"

inline bool G_WorkshopVehicleThrust(gentity_t& hull,const vec3_t acceleration,float seconds){
 if(!(seconds>0)||!std::isfinite(seconds)||(hull.svFlags&SVF_INACTIVE)||!G_TransportVehicleReady(hull))return false;
 auto& vehicle=*hull.m_pVehicle;
 for(const auto* member:{&hull,vehicle.m_pPilot,vehicle.m_pDroidUnit})
  if(member&&(member->client->ps.eFlags&JAR_VEHICLE_IMMOBILE_FLAGS))return false;
 auto& ps=hull.client->ps;
 if(!G_VehiclePushDriftValid(vehicle.jarPushDrift))return false;
 for(float value:ps.velocity)if(!std::isfinite(value))return false;

 vec3_t step,nextDrift;
 VectorScale(acceleration,seconds,step);
 for(float value:step)if(!std::isfinite(value))return false;
 const float priorSpeed=std::hypot(vehicle.jarPushDrift[0],vehicle.jarPushDrift[1]);
 const float driftLimit=std::max(float(jar::push::maxSpeed),priorSpeed);
 VectorSet(nextDrift,vehicle.jarPushDrift[0]+step[0],vehicle.jarPushDrift[1]+step[1],0);
 const float speed=std::hypot(nextDrift[0],nextDrift[1]);
 if(!std::isfinite(speed))return false;
 if(speed>driftLimit)VectorScale(nextDrift,driftLimit/speed,nextDrift);
 if(!G_VehiclePushDriftValid(nextDrift))return false;

 // Only replace the drift component. The stock vehicle still owns throttle,
 // steering, gravity and seat placement.
 vec3_t result;VectorCopy(ps.velocity,result);
 for(int axis=0;axis<2;++axis)result[axis]+=nextDrift[axis]-vehicle.jarPushDrift[axis];
 const float verticalLimit=std::max(float(jar::push::maxSpeed),std::fabs(ps.velocity[2]));
 result[2]=std::max(-verticalLimit,std::min(verticalLimit,ps.velocity[2]+step[2]));
 for(float value:result)if(!std::isfinite(value))return false;
 if(result[2]!=ps.velocity[2]){
  if(result[2]>0){ps.groundEntityNum=ENTITYNUM_NONE;hull.s.groundEntityNum=ENTITYNUM_NONE;}
  ps.forceJumpZStart=0;
  ps.jumpZStart=ps.origin[2];
  ps.pm_flags|=PMF_TRIGGER_PUSHED;
 }
 VectorCopy(nextDrift,vehicle.jarPushDrift);
 VectorCopy(result,ps.velocity);
 VectorCopy(result,hull.s.pos.trDelta);
 return true;
}
