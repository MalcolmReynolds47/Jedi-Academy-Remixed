// Shared native teleport motion; events, killbox and linking stay server-owned.
// GPL-2.0-only.
#pragma once
#include <math.h>

static inline void JAR_TeleportFinal(const vec3_t origin,vec3_t final){
 VectorCopy(origin,final);final[2]+=1;
}
static inline void JAR_TeleportMotion(playerState_t*ps,const vec3_t origin,const vec3_t angles,qboolean noAngles){
 JAR_TeleportFinal(origin,ps->origin);
 if(!noAngles){
  AngleVectors(angles,ps->velocity,NULL,NULL);VectorScale(ps->velocity,400,ps->velocity);
  ps->pm_time=160;ps->pm_flags|=PMF_TIME_KNOCKBACK;
 }
 ps->eFlags^=EF_TELEPORT_BIT;
}
static inline qboolean JAR_OwnedTeleportArrival(const vec3_t marker,float top,float bottom,vec3_t origin,vec3_t final){
 VectorCopy(marker,origin);origin[2]+=top-bottom+4;
 JAR_TeleportFinal(origin,final);
 for(int a=0;a<3;++a)if(!isfinite(final[a])||fabsf(final[a])>65535)return qfalse;
 return qtrue;
}
