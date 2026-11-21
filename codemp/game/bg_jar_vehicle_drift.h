// Horizontal vehicle momentum is separate from native throttle/steering.
// Shared by the server and prediction. GPL-2.0-only.
#ifndef BG_JAR_VEHICLE_DRIFT_H
#define BG_JAR_VEHICLE_DRIFT_H
static void JAR_VehicleDriftVelocity(playerState_t*ps,const vec3_t total){
 vec3_t base,drift;VectorSubtract(ps->velocity,ps->jarVehicleDrift,base);VectorSubtract(total,base,drift);
 for(int a=0;a<3;++a)if(!isfinite(drift[a]))return;
 // Vertical impulses share native velocity/gravity. Splitting them into two
 // accumulated components can cap thrust while gravity keeps accumulating.
 drift[2]=0;float speed=VectorLength(drift),limit=fmaxf(1500,VectorLength(ps->jarVehicleDrift));if(speed>limit)VectorScale(drift,limit/speed,drift);
 const float verticalLimit=fmaxf(1500,fabsf(ps->velocity[2]));
 VectorCopy(drift,ps->jarVehicleDrift);VectorAdd(base,drift,ps->velocity);
 ps->velocity[2]=fmaxf(-verticalLimit,fminf(verticalLimit,total[2]));
}
// target_push replaces current travel velocity; native controls resume from rest.
// Store the horizontal impulse separately so the next steering update retains it.
static qboolean JAR_VehiclePushVelocity(playerState_t*ps,const vec3_t total){
 for(int a=0;a<3;++a)if(!isfinite(total[a])||fabsf(total[a])>4000)return qfalse;
 VectorCopy(total,ps->velocity);VectorCopy(total,ps->jarVehicleDrift);ps->jarVehicleDrift[2]=0;ps->speed=0;
 if(total[2]>0)ps->groundEntityNum=ENTITYNUM_NONE;
 return qtrue;
}
#endif
