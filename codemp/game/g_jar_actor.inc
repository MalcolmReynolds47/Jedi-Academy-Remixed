// Multiplayer Physgun actor control. Uses native pose/snapshots, never Force damage.
// Included by g_jar_sandbox.c. GPL-2.0-only.
typedef struct {
 int controller,pmType,hand,handTime,legs,torso;
 qboolean active,posed,thrown;
} jarActor_t;
static jarActor_t actors[MAX_GENTITIES];
extern void BG_VehicleAdjustBBoxForOrientation(Vehicle_t*,vec3_t,vec3_t,vec3_t,int,int,void(*)(trace_t*,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int));

static qboolean ActorVehicle(const gentity_t*e){return e&&e->client&&(e->m_pVehicle||e->client->NPC_class==CLASS_VEHICLE);}
static qboolean VehicleOccupied(const Vehicle_t*v){
 if(!v)return qfalse;if(v->m_pPilot||v->m_pOldPilot||v->m_iNumPassengers||v->m_iBoarding)return qtrue;
 for(int i=0;i<VEH_MAX_PASSENGERS;++i)if(v->m_ppPassengers[i])return qtrue;return qfalse;
}
static void VehicleDriftClear(gentity_t*e);
#include "g_jar_vehicle_seats.inc"
static qboolean VehicleAvailableMode(gentity_t*e,qboolean transition){
 Vehicle_t*v=e->m_pVehicle;
 if(e->s.number<MAX_CLIENTS||e->client->NPC_class!=CLASS_VEHICLE||!v||!v->m_pVehicleInfo||v->m_pParentEntity!=(bgEntity_t*)e||!v->m_vOrientation||
    v->m_iDieTime||objects[e->s.number].copyPending||(e->s.eFlags&EF_NODRAW))return qfalse;
 return VehicleSeatsValidMode(e,transition);
}
static qboolean VehicleAvailable(gentity_t*e){return VehicleAvailableMode(e,qfalse);}
static void VehicleOrientation(gentity_t*e){
 if(!e->m_pVehicle||!e->client)return;
 VectorCopy(e->r.currentAngles,e->client->ps.vehOrientation);
 if(e->m_pVehicle->m_vOrientation)VectorCopy(e->r.currentAngles,e->m_pVehicle->m_vOrientation);
 VectorCopy(e->r.currentAngles,e->m_pVehicle->m_vPrevOrientation);
}
static void VehicleStop(gentity_t*e){
 Vehicle_t*v=e->m_pVehicle;if(!v)return;
 e->client->ps.speed=0;VectorClear(e->client->ps.jarVehicleDrift);v->m_vAngularVelocity=0;VectorClear(v->m_vFullAngleVelocity);VectorClear(v->m_vBoardingVelocity);
 if(v->m_iTurboTime>level.time)v->m_iTurboTime=level.time;
 memset(&v->m_ucmd,0,sizeof(v->m_ucmd));v->m_ucmd.serverTime=level.time;
}
static void VehicleDriftClear(gentity_t*e){
 playerState_t*ps=&e->client->ps;VectorSubtract(ps->velocity,ps->jarVehicleDrift,ps->velocity);VectorClear(ps->jarVehicleDrift);VectorCopy(ps->velocity,e->s.pos.trDelta);
}
static qboolean VehicleBounds(gentity_t*e,const vec3_t angle,vec3_t lo,vec3_t hi){
 VectorCopy(e->r.mins,lo);VectorCopy(e->r.maxs,hi);if(!e->m_pVehicle)return qtrue;
 if(!e->m_pVehicle->m_pVehicleInfo)return qfalse;
 Vehicle_t candidate=*e->m_pVehicle;vec3_t orientation,origin;VectorCopy(angle,orientation);VectorCopy(e->r.currentOrigin,origin);candidate.m_vOrientation=orientation;
 BG_VehicleAdjustBBoxForOrientation(&candidate,origin,lo,hi,e->s.number,MASK_PLAYERSOLID,NULL);
 for(int a=0;a<3;++a)if(!isfinite(lo[a])||!isfinite(hi[a])||lo[a]>hi[a]||fabsf(lo[a])>8192||fabsf(hi[a])>8192)return qfalse;
 return qtrue;
}
static qboolean ActorRotateAt(gentity_t*e,const vec3_t origin,const vec3_t angle,const vec3_t contact){
 if(PhysicsProp(e->s.number))return PhysicsRotateAt(e,origin,angle,contact);
 const qboolean translate=!VectorCompare(origin,e->r.currentOrigin);
 if(translate||e->m_pVehicle){vec3_t lo,hi;trace_t tr;if(!VehicleBounds(e,angle,lo,hi))return qfalse;
  trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,origin,e->s.number,PhysicsMoveMask(e),qfalse,0,0);
  if(tr.startsolid||tr.allsolid||tr.fraction<1)return qfalse;
  if(e->m_pVehicle){
   trap->Trace(&tr,origin,lo,hi,origin,e->s.number,MASK_PLAYERSOLID,qfalse,0,0);
   if(tr.startsolid||tr.allsolid)return qfalse;
  }
  VectorCopy(lo,e->r.mins);VectorCopy(hi,e->r.maxs);
 }
 VectorCopy(angle,e->r.currentAngles);VectorCopy(angle,e->s.apos.trBase);VectorCopy(angle,e->s.angles);e->s.apos.trType=TR_INTERPOLATE;
 if(e->client)SetClientViewAngle(e,e->r.currentAngles);VehicleOrientation(e);
 if(translate)Move(e,origin);
 return qtrue;
}
static qboolean ActorRotate(gentity_t*e,const vec3_t angle){
 return ActorRotateAt(e,e->r.currentOrigin,angle,NULL);
}
static qboolean ActorAliveMode(gentity_t *e,qboolean transition){
 if(!e->inuse||!e->client||e->health<=0||e->client->ps.stats[STAT_HEALTH]<=0||
    (e->client->ps.eFlags2&EF2_HELD_BY_MONSTER)||e->client->ps.duelInProgress||
    e->client->ps.fd.forceGripBeingGripped>level.time)return qfalse;
 if(ActorVehicle(e))return VehicleAvailableMode(e,transition);
 if(e->client->ps.m_iVehicleNum)return qfalse;
 return e->s.number>=MAX_CLIENTS||Alive(e);
}
static qboolean ActorAlive(gentity_t*e){return ActorAliveMode(e,qfalse);}
static qboolean ActorEdit(gentity_t *p,int n){
 if(n<0||n>=ENTITYNUM_WORLD||n==p->s.number||!ActorAlive(&g_entities[n]))return qfalse;
 const int gun=g_entities[n].client->ps.emplacedIndex;
 if(gun>=MAX_CLIENTS&&gun<ENTITYNUM_WORLD&&g_entities[gun].activator==&g_entities[n]&&JAR_MP_NativeEmplacedOccupied(&g_entities[gun]))return qfalse;
 if(!VehicleSeatsPermit(p,&g_entities[n]))return qfalse;
 if(n<MAX_CLIENTS)return playerTools.integer&&!players[n].noTools&&level.time>=players[n].noGrabUntil;
 return objects[n].owner?Edit(p,n):mapEditing.integer!=0;
}
static qboolean Manipulate(gentity_t *p,int n){
 if(n<0||n>=ENTITYNUM_WORLD)return qfalse;
 return g_entities[n].client?ActorEdit(p,n):Edit(p,n);
}
static void VehicleRiderPose(gentity_t*r){
 // Updating position must not consume another queued animation/weapon event.
 G_SetOrigin(r,r->client->ps.origin);r->s.pos.trType=TR_INTERPOLATE;VectorClear(r->client->ps.velocity);VectorClear(r->s.pos.trDelta);
}
static void VehicleRiderPublish(gentity_t*r){VehicleRiderPose(r);trap->LinkEntity((sharedEntity_t*)r);}
static void VehicleAttachRiders(gentity_t*e){
 if(!e->m_pVehicle||!VehicleAvailable(e))return;
 Vehicle_t*v=e->m_pVehicle;
 if(v->m_pVehicleInfo->AttachRiders)v->m_pVehicleInfo->AttachRiders(v);
 if(v->m_pPilot)VehicleRiderPublish((gentity_t*)v->m_pPilot);
 // The native MP client locates non-pilot passenger cameras at vehicle origin.
 for(int i=0;i<v->m_iNumPassengers;++i){gentity_t*r=(gentity_t*)v->m_ppPassengers[i];VectorCopy(e->client->ps.origin,r->client->ps.origin);VehicleRiderPublish(r);}
 if(v->m_pDroidUnit&&v->m_iDroidUnitTag>=0&&e->ghoul2){
  mdxaBone_t matrix;vec3_t yaw={0,e->client->ps.viewangles[YAW],0};gentity_t*d=(gentity_t*)v->m_pDroidUnit;
  if(trap->G2API_GetBoltMatrix(e->ghoul2,0,v->m_iDroidUnitTag,&matrix,yaw,e->r.currentOrigin,level.time,NULL,e->modelScale)){
   BG_GiveMeVectorFromMatrix(&matrix,ORIGIN,d->client->ps.origin);VehicleRiderPublish(d);
  }
 }
}
static void ActorPose(gentity_t *e){
 qboolean npc=e->s.eType==ET_NPC;
 VehicleOrientation(e);
 BG_PlayerStateToEntityState(&e->client->ps,&e->s,qfalse);
 if(npc)e->s.eType=ET_NPC;
 VectorCopy(e->client->ps.origin,e->r.currentOrigin);
}
static void ActorSnapshot(gentity_t *e){
 ActorPose(e);
 trap->LinkEntity((sharedEntity_t*)e);
 VehicleAttachRiders(e);
}
static qboolean ActorThrowEligible(gentity_t *e){
 return sandbox.integer&&!level.intermissiontime&&e->s.eType==ET_NPC&&e->NPC&&e->think==NPC_Think&&
        !ActorVehicle(e)&&ActorAlive(e)&&e->client->ps.pm_type==PM_NORMAL&&
        !e->client->noclip&&e->client->ps.gravity>0&&e->waterlevel<2&&
        (!e->next_roff_time||e->next_roff_time<level.time);
}

qboolean JAR_MP_NPCThrowCommand(gentity_t *e,usercmd_t *cmd){
 jarActor_t *actor=&actors[e->s.number];
 if(!actor->thrown)return qfalse;
 if(!ActorThrowEligible(e)||e->client->ps.groundEntityNum!=ENTITYNUM_NONE){
  actor->thrown=qfalse;
  return qfalse;
 }
 // Keep the native NPC command clock while advancing movement every server
 // frame. A stale command would trigger the native 100 ms catch-up step.
 const playerState_t *ps=&e->client->ps;
 e->nextthink=level.time+1;
 memset(cmd,0,sizeof(*cmd));
 cmd->serverTime=level.time-JAR_NPC_COMMAND_TIME_OFFSET;
 cmd->weapon=ps->weapon;
 for(int axis=0;axis<3;++axis)
  cmd->angles[axis]=ANGLE2SHORT(ps->viewangles[axis])-ps->delta_angles[axis];
 return qtrue;
}

static void EndActor(int n,qboolean fling){
 if(n<0||n>=ENTITYNUM_WORLD)return;
 if(!actors[n].active){actors[n].thrown=qfalse;return;}
 jarActor_t saved=actors[n];gentity_t *e=&g_entities[n];
 memset(&actors[n],0,sizeof(actors[n]));
 if(objects[n].heldBy){int c=objects[n].heldBy-1;if(players[c].held==n)ClearGrip(c);}
 objects[n].heldBy=objects[n].frozen=0;
 if(e->inuse&&e->client){
  if(e->client->ps.pm_type==PM_FREEZE)e->client->ps.pm_type=saved.pmType;
  if(!ActorVehicle(e)&&e->client->ps.forceHandExtend==HANDEXTEND_CHOKE){
   e->client->ps.forceHandExtend=saved.handTime>level.time?saved.hand:HANDEXTEND_NONE;
   e->client->ps.forceHandExtendTime=saved.handTime>level.time?saved.handTime:0;
  }
  if(e->health>0&&saved.posed){
   // Only unwind our choke pose; do not overwrite a death/other external animation.
   if(e->client->ps.legsAnim==BOTH_CHOKE3){e->client->ps.legsAnim=saved.legs;e->client->ps.legsTimer=0;}
   if(e->client->ps.torsoAnim==BOTH_CHOKE3){e->client->ps.torsoAnim=saved.torso;e->client->ps.torsoTimer=0;}
  }
  if(fling&&ActorAlive(e)){
   playerState_t *ps=&e->client->ps;
   VectorCopy(objects[n].velocity,ps->velocity);
   if(ActorThrowEligible(e)){
    actors[n].thrown=qtrue;
    ps->fd.forceJumpZStart=0;
    ps->commandTime=level.time-JAR_NPC_COMMAND_TIME_OFFSET;
    e->nextthink=level.time+1;
   }
  }
  else VectorClear(e->client->ps.velocity);
  // Vehicle dynamics resume from the released linear velocity, without old
  // throttle/turn/turbo commands or angular drift accumulated before the grab.
  if(e->m_pVehicle&&VehicleAvailable(e)){
   VehicleStop(e);vec3_t forward;AngleVectors(e->r.currentAngles,forward,NULL,NULL);
   if(VehicleOccupied(e->m_pVehicle)){VectorCopy(e->client->ps.velocity,e->client->ps.jarVehicleDrift);e->client->ps.jarVehicleDrift[2]=0;}
   else e->client->ps.speed=DotProduct(e->client->ps.velocity,forward);
  }
  ActorSnapshot(e);
 }
 VectorClear(objects[n].velocity);
}
static void BeginActorAs(int controller,gentity_t *e){
 if(e->s.number<MAX_CLIENTS)Drop(e->s.number,qfalse);
 jarActor_t *a=&actors[e->s.number];
 a->thrown=qfalse;
 if(!a->active){
  a->pmType=e->client->ps.pm_type;a->hand=e->client->ps.forceHandExtend;a->handTime=e->client->ps.forceHandExtendTime;
  a->legs=e->client->ps.legsAnim;a->torso=e->client->ps.torsoAnim;
  a->active=qtrue;
 }
 a->controller=controller;
}
static void BeginActor(gentity_t*p,gentity_t*e){BeginActorAs(p->s.number+1,e);}
qboolean JAR_MP_ActorControlled(gentity_t *e){
 return e&&e->s.number>=0&&e->s.number<ENTITYNUM_WORLD&&actors[e->s.number].active;
}
qboolean JAR_MP_ActorThink(gentity_t *e,usercmd_t *cmd){
 if(!JAR_MP_ActorControlled(e)){VehicleRiderInput(e,cmd);return qfalse;}
 int n=e->s.number;jarActor_t *a=&actors[n];int c=a->controller-1;
 const qboolean serverOwned=c==MAX_CLIENTS&&objects[n].owner==MAX_CLIENTS+1&&objects[n].frozen&&ActorAlive(e)&&VehicleSeatsPermit(NULL,e);
 if(!sandbox.integer||level.intermissiontime||(!serverOwned&&(c<0||c>=MAX_CLIENTS||!Alive(&g_entities[c])||!ActorEdit(&g_entities[c],n)))){
  EndActor(n,qfalse);return qfalse;
 }
 playerState_t *ps=&e->client->ps;
 ps->pm_type=PM_FREEZE;ps->groundEntityNum=ENTITYNUM_NONE;VectorClear(ps->velocity);VectorClear(ps->moveDir);
 if(ActorVehicle(e))VehicleStop(e);
 else {ps->forceHandExtend=HANDEXTEND_CHOKE;ps->forceHandExtendTime=level.time+200;}
 if(!ActorVehicle(e)&&e->localAnimIndex<=1){
  G_SetAnim(e,NULL,SETANIM_BOTH,BOTH_CHOKE3,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD,0);
  ps->legsTimer=ps->torsoTimer=200;a->posed=qtrue;
 }
 SetClientViewAngle(e,e->r.currentAngles);
 // Acknowledge commands while frozen so release never replays old movement.
 int time=cmd?cmd->serverTime:level.time;if(time>level.time+200)time=level.time+200;
 if(time>ps->commandTime)ps->commandTime=time;
 // The held vehicle's last checked pose already includes every seat.
 if(e->m_pVehicle&&objects[n].heldBy)ActorPose(e);else ActorSnapshot(e);
 return qtrue;
}
void JAR_MP_ClientReset(int c){
 if(c<0||c>=MAX_CLIENTS)return;
 SearchlightClearClient(c);
 VehicleRiderRelease(&g_entities[c]);
 JAR_MP_NativeEmplacedDetachUser(&g_entities[c]);
 Drop(c,qfalse);ForceClientReset(c);
 for(int n=0;n<level.num_entities;++n)if(actors[n].active&&(n==c||actors[n].controller==c+1))EndActor(n,qfalse);
 for(int other=0;other<MAX_CLIENTS;++other){
  if(players[other].selected==c){players[other].selected=ENTITYNUM_NONE;if(g_entities[other].inuse&&g_entities[other].client&&g_entities[other].client->pers.connected==CON_CONNECTED)SelectionEcho(&g_entities[other]);}
  if(players[other].first==c)players[other].first=ENTITYNUM_NONE;
 }
 objects[c].contextIdentity=0;memset(&players[c].propertyUndo,0,sizeof(players[c].propertyUndo));
 players[c].first=players[c].selected=ENTITYNUM_NONE;players[c].freezeLatch=0;players[c].contextOpen=0;
 if(g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)SelectionEcho(&g_entities[c]);
}
static void ActorFrame(void){
 for(int n=0;n<level.num_entities;++n)if(actors[n].active)JAR_MP_ActorThink(&g_entities[n],NULL);
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];
  if(e->inuse&&e->client&&e->m_pVehicle&&VectorLengthSquared(e->client->ps.jarVehicleDrift)>0&&
     (!sandbox.integer||level.intermissiontime||!ActorAliveMode(e,qtrue)||(!playerTools.integer&&VehicleOccupied(e->m_pVehicle))))VehicleDriftClear(e);
 }
}
