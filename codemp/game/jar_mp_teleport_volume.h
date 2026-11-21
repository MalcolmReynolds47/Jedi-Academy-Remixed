// Snapshot contract for conservative player prediction of owned teleports.
// GPL-2.0-only.
#pragma once
#include "bg_jar_teleport.h"
#include "jar_mp_push_volume.h"
#define JAR_MP_TELEPORT_VOLUME 246
#define JAR_MP_TELEPORT_WINDOW 100
#define JAR_MP_VEHICLE_TELEPORT_FEATURE 63
static inline qboolean JAR_TeleportVolumeValid(const entityState_t*s){
 if(s->eType!=ET_TELEPORT_TRIGGER||s->generic1!=JAR_MP_TELEPORT_VOLUME||
    s->number<MAX_CLIENTS||s->number>=ENTITYNUM_WORLD||s->otherEntityNum<MAX_CLIENTS||
    s->otherEntityNum>=ENTITYNUM_WORLD||s->otherEntityNum==s->number||
    s->modelindex<32||s->modelindex>512||!isfinite(s->speed)||fabsf(s->speed)>8192)return qfalse;
 for(int a=0;a<3;++a)if(!isfinite(s->pos.trBase[a])||fabsf(s->pos.trBase[a])>65535||
    !isfinite(s->apos.trBase[a])||fabsf(s->apos.trBase[a])>100000||
    !isfinite(s->origin2[a])||fabsf(s->origin2[a])>65535||
    !isfinite(s->angles2[a])||fabsf(s->angles2[a])>100000)return qfalse;
 return qtrue;
}
static inline qboolean JAR_TeleportVolumePlayer(const playerState_t*ps,int time){
 return ps->pm_type==PM_NORMAL&&ps->stats[STAT_HEALTH]>0&&!ps->m_iVehicleNum&&
        !(ps->pm_flags&PMF_FOLLOW)&&!ps->duelInProgress&&!(ps->eFlags2&EF2_HELD_BY_MONSTER)&&
        ps->fd.forceGripBeingGripped<=time?qtrue:qfalse;
}
static inline qboolean JAR_TeleportVolumeContact(const entityState_t*s,const vec3_t at,const vec3_t mins,const vec3_t maxs){
 return JAR_VolumeContact(s->pos.trBase,s->apos.trBase,(float)s->modelindex,at,mins,maxs);
}
static inline qboolean JAR_TeleportVolumeArrival(const entityState_t*s,const vec3_t mins,const vec3_t maxs,vec3_t origin,vec3_t final){
 return JAR_OwnedTeleportArrival(s->origin2,s->speed,mins[2],origin,final)&&!JAR_TeleportVolumeContact(s,final,mins,maxs)?qtrue:qfalse;
}
static inline qboolean JAR_TeleportVolumeCandidate(const entityState_t*s,const playerState_t*ps,const usercmd_t*cmd,const vec3_t mins,const vec3_t maxs,vec3_t origin,vec3_t final){
 const double age=(double)ps->commandTime-s->time;
 if(age<0||age>JAR_MP_TELEPORT_WINDOW||!JAR_TeleportVolumeValid(s)||!JAR_TeleportVolumePlayer(ps,ps->commandTime)||
    (cmd->buttons&(BUTTON_USE|BUTTON_USE_HOLDABLE))||!JAR_PushVolumeAllowed(s,ps->clientNum))return qfalse;
 return JAR_TeleportVolumeContact(s,ps->origin,mins,maxs)&&JAR_TeleportVolumeArrival(s,mins,maxs,origin,final)?qtrue:qfalse;
}
static inline void JAR_TeleportVolumeView(const entityState_t*s,playerState_t*ps,const usercmd_t*cmd){
 ps->groundEntityNum=ENTITYNUM_NONE;
 VectorCopy(s->angles2,ps->viewangles);
 for(int a=0;a<3;++a)ps->delta_angles[a]=ANGLE2SHORT(s->angles2[a])-cmd->angles[a];
}
static inline void JAR_TeleportVolumeApply(const entityState_t*s,playerState_t*ps,const usercmd_t*cmd,const vec3_t origin){
 JAR_TeleportMotion(ps,origin,s->angles2,qfalse);
 JAR_TeleportVolumeView(s,ps,cmd);
}

typedef struct {
 vec3_t vehicleOrigin,vehicleMins,vehicleMaxs;
 vec3_t pilotOrigin,pilotMins,pilotMaxs;
} jarVehicleTeleportArrival_t;

static inline qboolean JAR_TeleportVolumeVehicleState(const playerState_t*hull,const playerState_t*pilot,int time){
 if(pilot->clientNum<0||pilot->clientNum>=MAX_CLIENTS||hull->clientNum<MAX_CLIENTS||hull->clientNum>=ENTITYNUM_WORLD||
    pilot->m_iVehicleNum!=hull->clientNum||hull->m_iVehicleNum!=pilot->clientNum+1||
    pilot->pm_type!=PM_NORMAL||hull->pm_type!=PM_NORMAL||pilot->stats[STAT_HEALTH]<=0||hull->stats[STAT_HEALTH]<=0||
    ((pilot->pm_flags|hull->pm_flags)&PMF_FOLLOW)||pilot->duelInProgress||hull->duelInProgress||
    ((pilot->eFlags2|hull->eFlags2)&(EF2_HELD_BY_MONSTER|EF2_HYPERSPACE|EF2_SHIP_DEATH))||
    pilot->fd.forceGripBeingGripped>time||hull->fd.forceGripBeingGripped>time||
    hull->vehBoarding||hull->vehSurfaces||hull->vehTurnaroundTime>time||hull->hyperSpaceTime>time)return qfalse;
 return qtrue;
}

static inline qboolean JAR_TeleportVolumeHullValid(const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 for(int a=0;a<3;++a){
  if(!isfinite(origin[a])||fabsf(origin[a])>65535||!isfinite(mins[a])||fabsf(mins[a])>8192||
     !isfinite(maxs[a])||fabsf(maxs[a])>8192||mins[a]>maxs[a])return qfalse;
 }
 return qtrue;
}

// G_TouchTriggers first queries a fixed box around the actor's center. A
// long hull can overlap the rotated volume while its center is out of reach.
static inline qboolean JAR_TeleportVolumeNativeContact(const entityState_t*s,const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 const vec3_t range={40,40,52};
 vec3_t axes[3];
 AnglesToAxis(s->apos.trBase,axes);
 for(int a=0;a<3;++a){
  const float span=s->modelindex*(fabsf(axes[0][a])+fabsf(axes[1][a])+fabsf(axes[2][a]));
  if(fabsf(origin[a]-s->pos.trBase[a])>range[a]+span)return qfalse;
 }
 return JAR_TeleportVolumeContact(s,origin,mins,maxs);
}

static inline qboolean JAR_TeleportVolumeVehicleCandidate(const entityState_t*s,const playerState_t*hull,const playerState_t*pilot,
                                                          const usercmd_t*cmd,const vec3_t mins,const vec3_t maxs,
                                                          jarVehicleTeleportArrival_t*arrival){
 const double age=(double)pilot->commandTime-s->time, hullAge=(double)hull->commandTime-s->time;
 if(age<0||age>JAR_MP_TELEPORT_WINDOW||hullAge<0||hullAge>JAR_MP_TELEPORT_WINDOW||
    !JAR_TeleportVolumeValid(s)||s->otherEntityNum2!=hull->clientNum||
    s->otherEntityNum2==s->number||s->otherEntityNum2==s->otherEntityNum||
    !JAR_TeleportVolumeVehicleState(hull,pilot,pilot->commandTime)||
    (cmd->buttons&(BUTTON_USE|BUTTON_USE_HOLDABLE))||!JAR_PushVolumePilotAllowed(s,pilot->clientNum)||
    !JAR_TeleportVolumeHullValid(hull->origin,mins,maxs)||
    !JAR_TeleportVolumeHullValid(s->origin,s->pos.trDelta,s->apos.trDelta)||
    !JAR_TeleportVolumeHullValid(s->angles,s->boneAngles1,s->boneAngles2)||
    !JAR_TeleportVolumeNativeContact(s,hull->origin,mins,maxs)||
    JAR_TeleportVolumeContact(s,s->origin,s->pos.trDelta,s->apos.trDelta)||
    JAR_TeleportVolumeContact(s,s->angles,s->boneAngles1,s->boneAngles2))return qfalse;
 // The grant uses the native vehicle plan, including its exact Ghoul2 driver bolt.
 VectorCopy(s->origin,arrival->vehicleOrigin);
 VectorCopy(s->pos.trDelta,arrival->vehicleMins);
 VectorCopy(s->apos.trDelta,arrival->vehicleMaxs);
 VectorCopy(s->angles,arrival->pilotOrigin);
 VectorCopy(s->boneAngles1,arrival->pilotMins);
 VectorCopy(s->boneAngles2,arrival->pilotMaxs);
 return qtrue;
}

static inline void JAR_TeleportVolumeApplyVehicle(const entityState_t*s,playerState_t*hull,playerState_t*pilot,
                                                  const usercmd_t*cmd,const jarVehicleTeleportArrival_t*arrival){
 vec3_t origin;
 VectorCopy(arrival->vehicleOrigin,origin);
 origin[2]-=1;
 JAR_TeleportVolumeApply(s,hull,cmd,origin);
 hull->speed=400;
 VectorCopy(s->angles2,hull->vehOrientation);
 VectorClear(hull->jarVehicleDrift);
 VectorCopy(arrival->pilotOrigin,pilot->origin);
 VectorClear(pilot->velocity);
 JAR_TeleportVolumeView(s,pilot,cmd);
 pilot->eFlags^=EF_TELEPORT_BIT;
}
