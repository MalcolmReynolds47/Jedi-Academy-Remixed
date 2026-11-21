// Prediction grants expire quickly and do not bypass authoritative touch gates.
// GPL-2.0-only.
enum { TELEPORT_VEHICLE_GRANT_WORK = 6, TELEPORT_GRANT_NEIGHBORS = 128 };

static qboolean TeleportPilotGrantConflict(const gentity_t *helper,const playerState_t *pilot){
 const vec3_t hullReach={80,80,104}; // Two native G_TouchTriggers query radii.
 const float riderReach=512; // Match the vehicle's lead beyond the source extent.
 vec3_t mins,maxs;
 for(int a=0;a<3;++a){
  mins[a]=fminf(helper->r.currentOrigin[a]+helper->r.mins[a]-hullReach[a],pilot->origin[a]-riderReach);
  maxs[a]=fmaxf(helper->r.currentOrigin[a]+helper->r.maxs[a]+hullReach[a],pilot->origin[a]+riderReach);
 }
 int found[TELEPORT_GRANT_NEIGHBORS];
 const int count=trap->EntitiesInBox(mins,maxs,found,ARRAY_LEN(found));
 if(count<0||count>=ARRAY_LEN(found))return qtrue;
 for(int i=0;i<count;++i){
  const int n=found[i];
  if(n<MAX_CLIENTS||n>=level.num_entities||n==helper->s.number)continue;
  const gentity_t*other=&g_entities[n];
  if(other->inuse&&other->r.linked&&(other->r.contents&CONTENTS_TRIGGER)&&other->touch&&
     (other->s.eType==ET_PUSH_TRIGGER||other->s.eType==ET_TELEPORT_TRIGGER))return qtrue;
 }
 return qfalse;
}

static qboolean TeleportPilotGrantClear(const entityState_t *s, const gentity_t *pilot,
                                        const jarVehiclePoseMember_t members[2]){
 for(int i=0;i<2;++i){
  const jarVehiclePoseMember_t*m=&members[i];
  if(JAR_TeleportVolumeContact(s,m->origin,m->mins,m->maxs)||
     !trap->InPVS(pilot->client->ps.origin,m->origin))return qfalse;
  // The commit can unlink the assembly. Publication instead declines close
  // destinations where its read-only traces might encounter either member.
  for(int j=0;j<2;++j){
   const gentity_t*current=members[j].entity;
   qboolean overlaps=qtrue;
   for(int a=0;a<3;++a)if(m->origin[a]+m->maxs[a]<current->r.absmin[a]||
                          m->origin[a]+m->mins[a]>current->r.absmax[a])overlaps=qfalse;
   if(overlaps)return qfalse;
  }
  trace_t tr;
  trap->Trace(&tr,m->origin,m->mins,m->maxs,m->origin,m->entity->s.number,MASK_PLAYERSOLID,qfalse,0,0);
  if(tr.startsolid||tr.allsolid)return qfalse;
 }
 return qtrue;
}

static void TeleportPublishPilot(gentity_t *source,gentity_t *dest,gentity_t *helper,float reach){
 if(!trap->G2API_AddBolt||!trap->G2API_GetBoltMatrix||!trap->EntitiesInBox)return;
 entityState_t*s=&helper->s;
 for(int c=0;c<MAX_CLIENTS&&teleportGrantBudget>=TELEPORT_VEHICLE_GRANT_WORK;++c){
  gentity_t*p=&g_entities[c];
  if(!p->inuse||!p->r.linked||!p->client||p->client->noclip||!p->client->ps.m_iVehicleNum||
     (p->client->pers.cmd.buttons&(BUTTON_USE|BUTTON_USE_HOLDABLE)))continue;
  gentity_t*e=TransportMover(p);
  if(!e||!e->r.linked||!e->m_pVehicle||e->s.eType!=ET_NPC||e->s.NPC_class!=CLASS_VEHICLE||e->s.owner!=c)continue;
  Vehicle_t*v=e->m_pVehicle;
  const playerState_t*pilot=&p->client->ps,*hull=&e->client->ps;
  if(v->m_pPilot!=(bgEntity_t*)p||v->m_iNumPassengers||v->m_pDroidUnit||
     pilot->clientNum!=c||hull->clientNum!=e->s.number||v->m_iRemovedSurfaces||v->m_iTurboTime>level.time||
     !JAR_TeleportVolumeVehicleState(hull,pilot,level.time)||
     !JAR_TeleportVolumeHullValid(hull->origin,e->r.mins,e->r.maxs)||
     !JAR_TeleportVolumeHullValid(pilot->origin,p->r.mins,p->r.maxs)||
     DistanceSquared(hull->origin,s->pos.trBase)>reach*reach||
     !TransportActor(source,e,qtrue)||e->client->noclip)continue;
  const vehicleInfo_t*info=v->m_pVehicleInfo;
  if(!isfinite(info->length)||!isfinite(info->width)||!isfinite(info->height)||
     info->length<=0||info->width<=0||info->height<=0)continue;
  if(teleportGrantRoom<0)teleportGrantRoom=Room();
  if(!teleportGrantRoom)return;
  // Charge for the neighborhood query, driver matrix, two PVS checks and two traces.
  teleportGrantBudget-=TELEPORT_VEHICLE_GRANT_WORK;
  if(TeleportPilotGrantConflict(helper,pilot))continue;
  jarVehiclePoseMember_t members[VEH_MAX_PASSENGERS+3];int count;
  if(!TeleportVehiclePlan(e,dest,members,&count)||count!=2||
     TeleportVehicleAttached(members,count)||!TeleportPilotGrantClear(s,p,members))continue;
  s->otherEntityNum2=e->s.number;
  VectorCopy(members[0].origin,s->origin);VectorCopy(members[1].origin,s->angles);
  VectorCopy(members[0].mins,s->pos.trDelta);VectorCopy(members[0].maxs,s->apos.trDelta);
  VectorCopy(members[1].mins,s->boneAngles1);VectorCopy(members[1].maxs,s->boneAngles2);
  if(c<16)s->trickedentindex3=1u<<c;else s->trickedentindex4=1u<<(c-16);
  return;
 }
}

static void TriggerPublishTeleport(gentity_t*e,gentity_t*h){
 entityState_t*s=&h->s;int n=e->s.number;jarObject_t*o=&objects[n];
 h->r.svFlags|=SVF_NOCLIENT;s->generic1=JAR_MP_TELEPORT_VOLUME;
 s->trickedentindex=s->trickedentindex2=s->trickedentindex3=s->trickedentindex4=0;
 s->otherEntityNum2=ENTITYNUM_NONE;
 s->apos.trType=TR_STATIONARY;
 VectorClear(s->origin);VectorClear(s->angles);VectorClear(s->pos.trDelta);VectorClear(s->apos.trDelta);
 VectorClear(s->boneAngles1);VectorClear(s->boneAngles2);
 if(!playerTools.integer||!trap->InPVS||e->spawnflags||!o->frozen||o->heldBy||e->behaviorSet[BSET_USE])return;
 gentity_t*dest=TransportDestination(e);
 if(!dest||!objects[dest->s.number].frozen||objects[dest->s.number].heldBy)return;
 s->time=level.time;s->modelindex=o->triggerExtent;s->otherEntityNum=n;s->speed=TransportTop(dest);
 VectorCopy(e->r.currentOrigin,s->pos.trBase);VectorCopy(e->r.currentAngles,s->apos.trBase);
 VectorCopy(dest->r.currentOrigin,s->origin2);VectorCopy(dest->r.currentAngles,s->angles2);
 if(!JAR_TeleportVolumeValid(s))return;
 if(teleportGrantTime!=level.time){teleportGrantTime=level.time;teleportGrantBudget=64;teleportGrantRoom=-1;}
 if(teleportGrantRoom==0||!teleportGrantBudget)return;
 // Cull distant players before visibility/trace work, but allow a brief
 // movement lead so an ordinary player approaching the edge can predict it.
 const float reach=(float)o->triggerExtent*1.733f+192;
 for(int c=0;c<MAX_CLIENTS&&teleportGrantBudget;++c){
  gentity_t*p=&g_entities[c];vec3_t origin,final;
  if(!p->client||p->client->noclip||!TransportActor(e,p,qfalse)||
     !JAR_TeleportVolumePlayer(&p->client->ps,level.time)||DistanceSquared(p->client->ps.origin,s->pos.trBase)>reach*reach||
     !JAR_TeleportVolumeArrival(s,p->r.mins,p->r.maxs,origin,final))continue;
  --teleportGrantBudget;
  if(!trap->InPVS(p->client->ps.origin,final))continue;
  if(teleportGrantRoom<0)teleportGrantRoom=Room();
  if(!teleportGrantRoom)break;
  trace_t tr;trap->Trace(&tr,final,p->r.mins,p->r.maxs,final,c,MASK_PLAYERSOLID,qfalse,0,0);
  if(tr.startsolid||tr.allsolid)continue;
  if(c<16)s->trickedentindex|=1u<<c;else s->trickedentindex2|=1u<<(c-16);
 }
 if(teleportGrantRoom!=0&&teleportGrantBudget>=TELEPORT_VEHICLE_GRANT_WORK)
  TeleportPublishPilot(e,dest,h,reach+320);
 if(s->trickedentindex||s->trickedentindex2||s->trickedentindex3||s->trickedentindex4)
  h->r.svFlags&=~SVF_NOCLIENT;
}
