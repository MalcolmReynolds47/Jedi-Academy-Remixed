// Transactional vehicle/seat arrival for owned Sandbox teleport controllers.
// Authored map teleporters continue through their native path. GPL-2.0-only.
typedef struct {
 gentity_t*entity;
 vec3_t origin,mins,maxs;
 qboolean linked;
} jarVehiclePoseMember_t;

static qboolean TeleportVector(const vec3_t value,float limit){
 for(int a=0;a<3;++a)if(!isfinite(value[a])||fabsf(value[a])>limit)return qfalse;
 return qtrue;
}
static qboolean VehiclePoseMemberAdd(jarVehiclePoseMember_t*members,int*count,gentity_t*e,const vec3_t origin){
 if(*count>=VEH_MAX_PASSENGERS+3||!e||!e->client||!TeleportVector(origin,65535)||
    !TeleportVector(e->r.mins,8192)||!TeleportVector(e->r.maxs,8192))return qfalse;
 for(int a=0;a<3;++a)if(e->r.mins[a]>e->r.maxs[a])return qfalse;
 for(int i=0;i<*count;++i)if(members[i].entity==e)return qfalse;
 jarVehiclePoseMember_t*m=&members[(*count)++];m->entity=e;m->linked=e->r.linked;
 VectorCopy(origin,m->origin);VectorCopy(e->r.mins,m->mins);VectorCopy(e->r.maxs,m->maxs);return qtrue;
}
static qboolean VehiclePosePlan(gentity_t*e,const vec3_t origin,const vec3_t angles,const vec3_t lo,const vec3_t hi,
                                jarVehiclePoseMember_t*members,int*count){
 Vehicle_t*v=e->m_pVehicle;*count=0;
 if(!VehicleAvailable(e)||!VehiclePoseMemberAdd(members,count,e,origin))return qfalse;
 VectorCopy(lo,members[0].mins);VectorCopy(hi,members[0].maxs);
 // These are the native MP seat locations: yaw-only driver/droid bolts and
 // vehicle-origin passenger cameras. Evaluate once before moving any member.
 for(int i=-1;i<=v->m_iNumPassengers;++i){
  gentity_t*r=i<0?(gentity_t*)v->m_pPilot:i==v->m_iNumPassengers?(gentity_t*)v->m_pDroidUnit:(gentity_t*)v->m_ppPassengers[i];
  if(!r)continue;
  if(!VehicleSeatValid(e,(bgEntity_t*)r))return qfalse;
  vec3_t point;VectorCopy(origin,point);
  if(i<0||i==v->m_iNumPassengers){
   if(!e->ghoul2)return qfalse;
   int bolt=i<0?trap->G2API_AddBolt(e->ghoul2,0,"*driver"):v->m_iDroidUnitTag;
   mdxaBone_t matrix;vec3_t yaw={0,angles[YAW],0};
   if(bolt<0||!trap->G2API_GetBoltMatrix(e->ghoul2,0,bolt,&matrix,yaw,origin,level.time,NULL,e->modelScale))return qfalse;
   BG_GiveMeVectorFromMatrix(&matrix,ORIGIN,point);
  }
  if(!VehiclePoseMemberAdd(members,count,r,point))return qfalse;
 }
 return qtrue;
}
static qboolean TeleportVehiclePlan(gentity_t*e,gentity_t*dest,jarVehiclePoseMember_t*members,int*count){
 vec3_t lo,hi,origin;
 if(!TeleportVector(dest->r.currentAngles,3600)||!VehicleBounds(e,dest->r.currentAngles,lo,hi))return qfalse;
 VectorCopy(dest->r.currentOrigin,origin);origin[2]+=dest->r.maxs[2]-lo[2]+5;
 return VehiclePosePlan(e,origin,dest->r.currentAngles,lo,hi,members,count);
}
static qboolean TeleportVehicleAttached(const jarVehiclePoseMember_t*members,int count){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&g_entities[n].s.generic1==251&&objects[n].owner){
  for(int i=0;i<count;++i){int id=members[i].entity->s.number+1;if(objects[n].first==id||objects[n].second==id)return qtrue;}
 }
 return qfalse;
}
static qboolean TeleportVehicleClear(jarVehiclePoseMember_t*members,int count){
 for(int i=0;i<count;++i)trap->UnlinkEntity((sharedEntity_t*)members[i].entity);
 qboolean clear=qtrue;
 for(int i=0;i<count&&clear;++i){
  jarVehiclePoseMember_t*m=&members[i];trace_t tr;
  trap->Trace(&tr,m->origin,m->mins,m->maxs,m->origin,m->entity->s.number,MASK_PLAYERSOLID,qfalse,0,0);
  clear=!tr.startsolid&&!tr.allsolid;
 }
 for(int i=0;i<count;++i)if(members[i].linked)trap->LinkEntity((sharedEntity_t*)members[i].entity);
 return clear;
}
static void TeleportVehicle(gentity_t*source,gentity_t*e,gentity_t*dest,gentity_t*activator){
 jarVehiclePoseMember_t members[VEH_MAX_PASSENGERS+3];int count;
 if(!TeleportVehiclePlan(e,dest,members,&count)){TransportMessage(activator,"Vehicle or seat arrival could not be resolved. Nothing teleported.");return;}
 if(TeleportVehicleAttached(members,count)){TransportMessage(activator,"Remove ropes, welds or hinges from the vehicle and riders before teleporting.");return;}
 if(!TeleportVehicleClear(members,count)){TransportMessage(activator,"Destination obstructs the vehicle or a rider. Nothing teleported.");return;}
 if(!Room()){TransportMessage(activator,"Entity reserve reached; no teleport effects allocated.");return;}
 G_ActivateBehavior(source,BSET_USE);
 // Keep every seat unlinked until native KillBox has finished. Preflight above
 // rejects occupants/geometry instead of telefragging at a Sandbox destination.
 for(int i=0;i<count;++i)trap->UnlinkEntity((sharedEntity_t*)members[i].entity);
 VectorCopy(members[0].mins,e->r.mins);VectorCopy(members[0].maxs,e->r.maxs);
 VectorCopy(dest->r.currentAngles,e->r.currentAngles);VectorCopy(e->r.currentAngles,e->s.angles);
 VehicleStop(e);VehicleOrientation(e);e->client->ps.groundEntityNum=ENTITYNUM_NONE;
 vec3_t origin;VectorCopy(members[0].origin,origin);origin[2]-=1;
 TeleportPlayer(e,origin,dest->r.currentAngles);
 e->client->ps.speed=400;e->s.pos.trType=TR_INTERPOLATE;
 e->m_pVehicle->m_LandTrace.fraction=1;
 VectorCopy(e->r.currentAngles,e->m_pVehicle->m_vPrevRiderViewAngles);
 for(int i=1;i<count;++i){
  gentity_t*r=members[i].entity;
  VectorCopy(members[i].origin,r->client->ps.origin);SetClientViewAngle(r,e->r.currentAngles);
  VectorCopy(e->r.currentAngles,r->r.currentAngles);VectorCopy(e->r.currentAngles,r->s.apos.trBase);r->s.apos.trType=TR_INTERPOLATE;
  r->client->ps.groundEntityNum=ENTITYNUM_NONE;r->client->ps.eFlags^=EF_TELEPORT_BIT;
  r->s.eFlags=(r->s.eFlags&~EF_TELEPORT_BIT)|(r->client->ps.eFlags&EF_TELEPORT_BIT);
  VehicleRiderPublish(r);
 }
 ++objects[e->s.number].propertyRevision;
 TransportMessage(activator,"Vehicle and occupants teleported to the linked destination.");
}
