// Shared geometry and movement contract for owned push-volume snapshots.
// Existing entity fields carry this only for ET_PUSH_TRIGGER / generic1 247.
// GPL-2.0-only.
#pragma once
#include <math.h>
#include "bg_jar_vehicle_drift.h"
#include "../../shared/qcommon/jar_box_contact.h"
#define JAR_MP_PUSH_VOLUME 247
static qboolean JAR_VolumeContact(const vec3_t center,const vec3_t angles,float extent,const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 const vec3_t half={extent,extent,extent};
 return JAR_BoxVolumeContact(center,angles,half,origin,mins,maxs);
}
// Return codes retain the server's actionable trajectory errors.
static int JAR_PushVolumeVelocity(int flags,const vec3_t center,const vec3_t angles,const float*destination,const vec3_t actor,float speed,float gravity,vec3_t velocity){
 if((flags&16)&&!destination)return 2;
 if(!destination){AngleVectors(angles,velocity,NULL,NULL);VectorScale(velocity,speed,velocity);}
 else {
  VectorSubtract(destination,(flags&16)?actor:center,velocity);
  if(flags&4){if(VectorNormalize(velocity)<.01f)return 3;VectorScale(velocity,speed,velocity);}
  else {float height=velocity[2];if(height<=.01f||!isfinite(gravity)||gravity<=0)return 4;float time=sqrtf(height/(.5f*gravity));velocity[0]/=time;velocity[1]/=time;velocity[2]=time*gravity;}
 }
 for(int a=0;a<3;++a)if(!isfinite(velocity[a])||fabsf(velocity[a])>4000)return 5;
 return 1;
}
static qboolean JAR_PushVolumeValid(const entityState_t*s){
 int mode=s->frame&~2048;
 if(s->eType!=ET_PUSH_TRIGGER||s->generic1!=JAR_MP_PUSH_VOLUME||s->number<MAX_CLIENTS||s->number>=ENTITYNUM_WORLD||s->modelindex<32||s->modelindex>512||s->frame<0||(mode!=0&&mode!=4&&mode!=20)||(!mode&&(s->frame&2048))||s->bolt2<0||s->bolt2>1||s->time2< -1||s->time2>60000||(!(s->frame&4)&&s->time2)||!isfinite(s->speed)||s->speed<50||s->speed>4000||!isfinite(s->angles2[0])||s->angles2[0]<0||s->angles2[0]>10000)return qfalse;
 if((s->frame&16)&&!s->bolt2)return qfalse;
 for(int a=0;a<3;++a)if(!isfinite(s->pos.trBase[a])||fabsf(s->pos.trBase[a])>65535||!isfinite(s->apos.trBase[a])||fabsf(s->apos.trBase[a])>100000||!isfinite(s->origin2[a])||fabsf(s->origin2[a])>65535||!isfinite(s->pos.trDelta[a])||fabsf(s->pos.trDelta[a])>5000||!isfinite(s->apos.trDelta[a])||fabsf(s->apos.trDelta[a])>20000)return qfalse;
 return qtrue;
}
static qboolean JAR_PushVolumeAllowed(const entityState_t*s,int client){
 if(client<0||client>=MAX_CLIENTS)return qfalse;
 unsigned mask=client<16?(unsigned)s->trickedentindex:(unsigned)s->trickedentindex2;
 return mask&(1u<<(client&15))?qtrue:qfalse;
}
// Separate pilot permissions keep passenger consent and player-only permission
// from being confused. The server resolves the current vehicle and all seats.
static qboolean JAR_PushVolumePilotAllowed(const entityState_t*s,int client){
 if(client<0||client>=MAX_CLIENTS)return qfalse;
 unsigned mask=client<16?(unsigned)s->trickedentindex3:(unsigned)s->trickedentindex4;
 return mask&(1u<<(client&15))?qtrue:qfalse;
}
static void JAR_PushVolumePose(const entityState_t*s,int time,vec3_t origin,vec3_t angles){
 // Match prop rider prediction's bounded velocity horizon. Stale snapshots
 // never extrapolate indefinitely, and future snapshots are not wound back.
 float dt=fmaxf(0,fminf(.1f,(float)((double)time-s->pos.trTime)*.001f));
 VectorMA(s->pos.trBase,dt,s->pos.trDelta,origin);VectorMA(s->apos.trBase,dt,s->apos.trDelta,angles);
}
typedef struct {int last; qboolean spent;} jarPushPrediction_t;
static qboolean JAR_PushVolumePredictActor(const entityState_t*s,jarPushPrediction_t*pending,playerState_t*ps,const vec3_t mins,const vec3_t maxs){
 if(!JAR_PushVolumeValid(s)||pending->spent||ps->stats[STAT_HEALTH]<=0||ps->fd.forceGripBeingGripped>ps->commandTime||(ps->pm_flags&PMF_FOLLOW))return qfalse;
 if(s->frame&4){if(ps->pm_type!=PM_NORMAL)return qfalse;}else if(ps->pm_type!=PM_NORMAL&&ps->pm_type!=PM_JETPACK&&ps->pm_type!=PM_FLOAT)return qfalse;
 int now=ps->commandTime;
 if((s->frame&4)&&(double)now<(double)pending->last+s->time2&&(!(s->frame&2048)||(pending->last&&now>pending->last)))return qfalse;
 vec3_t origin,angles,velocity;JAR_PushVolumePose(s,now,origin,angles);
 if(!JAR_VolumeContact(origin,angles,(float)s->modelindex,ps->origin,mins,maxs)||JAR_PushVolumeVelocity(s->frame,origin,angles,s->bolt2?s->origin2:NULL,ps->origin,s->speed,s->angles2[0],velocity)!=1)return qfalse;
 if(s->frame&4){VectorCopy(velocity,ps->velocity);if(s->time2==-1)pending->spent=qtrue;else if(s->time2>0)pending->last=now;}
 else {entityState_t pad=*s;VectorCopy(velocity,pad.origin2);BG_TouchJumpPad(ps,&pad);}
 return qtrue;
}
static qboolean JAR_PushVolumePredict(const entityState_t*s,jarPushPrediction_t*pending,playerState_t*ps,const vec3_t mins,const vec3_t maxs){
 if(ps->m_iVehicleNum||!JAR_PushVolumeAllowed(s,ps->clientNum))return qfalse;
 return JAR_PushVolumePredictActor(s,pending,ps,mins,maxs);
}
static qboolean JAR_PushVolumePredictVehicle(const entityState_t*s,jarPushPrediction_t*pending,playerState_t*ps,const playerState_t*pilot,const vec3_t mins,const vec3_t maxs){
 if(!JAR_PushVolumePilotAllowed(s,pilot->clientNum)||ps->clientNum<MAX_CLIENTS||ps->clientNum>=ENTITYNUM_WORLD||pilot->m_iVehicleNum!=ps->clientNum||ps->m_iVehicleNum!=pilot->clientNum+1||ps->vehBoarding||pilot->stats[STAT_HEALTH]<=0||pilot->pm_type!=PM_NORMAL||pilot->duelInProgress||pilot->fd.forceGripBeingGripped>pilot->commandTime||(pilot->eFlags2&EF2_HELD_BY_MONSTER)||(pilot->pm_flags&PMF_FOLLOW))return qfalse;
 if(!JAR_PushVolumePredictActor(s,pending,ps,mins,maxs))return qfalse;
 // Exactly the same replacement as the authoritative touch: keep horizontal
 // launch momentum outside the velocity rewritten by native vehicle steering.
 return JAR_VehiclePushVelocity(ps,ps->velocity);
}
