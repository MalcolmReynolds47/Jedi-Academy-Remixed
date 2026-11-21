// Occupied vehicle control keeps native seat links and attachment callbacks.
// Included by g_jar_actor.inc. GPL-2.0-only.
static gentity_t* VehicleSeatEntity(const bgEntity_t*seat){
 if(!seat)return NULL;
 const uintptr_t address=(uintptr_t)seat,base=(uintptr_t)g_entities;
 if(address<base||address>=base+sizeof(gentity_t)*ENTITYNUM_WORLD||(address-base)%sizeof(gentity_t))return NULL;
 gentity_t*r=&g_entities[(address-base)/sizeof(gentity_t)];
 return r->inuse&&r->client&&r->s.number==(int)(r-g_entities)?r:NULL;
}
static qboolean VehicleSeatValid(gentity_t*e,const bgEntity_t*seat){
 gentity_t*r=VehicleSeatEntity(seat);
 if(!r||r==e||r->playerState!=&r->client->ps||r->health<=0||r->client->ps.stats[STAT_HEALTH]<=0||r->client->ps.m_iVehicleNum!=e->s.number||
    r->m_pVehicle||actors[r->s.number].active||(r->client->ps.eFlags2&EF2_HELD_BY_MONSTER)||
    r->client->ps.duelInProgress||r->client->ps.fd.forceGripBeingGripped>level.time)return qfalse;
 return r->s.number>=MAX_CLIENTS||(r->client->pers.connected==CON_CONNECTED&&r->client->sess.sessionTeam!=TEAM_SPECTATOR);
}
static qboolean VehicleHasRider(gentity_t*e,gentity_t*r){
 if(!e||!e->client||!e->m_pVehicle||!r)return qfalse;
 const Vehicle_t*v=e->m_pVehicle;if(v->m_pPilot==(bgEntity_t*)r||v->m_pOldPilot==(bgEntity_t*)r)return qtrue;
 for(int i=0;i<VEH_MAX_PASSENGERS;++i)if(v->m_ppPassengers[i]==(bgEntity_t*)r)return qtrue;
 return qfalse;
}
static qboolean VehicleSeatsValidMode(gentity_t*e,qboolean transition){
 const Vehicle_t*v=e->m_pVehicle;
 // Boarding, ejecting and pilot exchanges remain native until settled.
 if((!transition&&(v->m_iBoarding||(v->m_pOldPilot&&v->m_pOldPilot!=v->m_pPilot)))||v->m_iNumPassengers<0||v->m_iNumPassengers>VEH_MAX_PASSENGERS||v->m_iNumPassengers>v->m_pVehicleInfo->maxPassengers)return qfalse;
 if(v->m_pOldPilot&&!VehicleSeatValid(e,v->m_pOldPilot))return qfalse;
 if(v->m_pPilot){
  if(!VehicleSeatValid(e,v->m_pPilot)||e->client->ps.m_iVehicleNum!=v->m_pPilot->s.number+1)return qfalse;
 }else if(e->client->ps.m_iVehicleNum)return qfalse;
 int count=0;
 for(int i=0;i<VEH_MAX_PASSENGERS;++i)if(v->m_ppPassengers[i]){
  if(i>=v->m_iNumPassengers||!VehicleSeatValid(e,v->m_ppPassengers[i])||v->m_ppPassengers[i]==v->m_pPilot)return qfalse;
  for(int j=0;j<i;++j)if(v->m_ppPassengers[j]==v->m_ppPassengers[i])return qfalse;
  ++count;
 }
 if(count!=v->m_iNumPassengers)return qfalse;
 if((v->m_pPilot||count)&&(!e->ghoul2||e->playerState!=&e->client->ps||!v->m_pVehicleInfo->AttachRiders))return qfalse;
 // Native AttachRiders also dereferences a vehicle's optional droid helper.
 if(v->m_pDroidUnit){gentity_t*d=VehicleSeatEntity(v->m_pDroidUnit);if(!d||d==e||d->s.number<MAX_CLIENTS||d->client->ps.m_iVehicleNum!=e->s.number)return qfalse;}
 return qtrue;
}
static qboolean VehicleSeatPermission(gentity_t*p,const bgEntity_t*seat){
 if(!seat)return qtrue;gentity_t*r=VehicleSeatEntity(seat);if(!r)return qfalse;int n=r->s.number;
 if(n<MAX_CLIENTS)return playerTools.integer&&!players[n].noTools&&level.time>=players[n].noGrabUntil;
 return objects[n].owner?(p?Edit(p,n):objects[n].owner==MAX_CLIENTS+1):mapEditing.integer!=0;
}
static qboolean VehicleSeatsPermit(gentity_t*p,gentity_t*e){
 const Vehicle_t*v=e->m_pVehicle;if(!v)return qtrue;
 if(!VehicleSeatPermission(p,v->m_pPilot)||!VehicleSeatPermission(p,v->m_pOldPilot))return qfalse;
 for(int i=0;i<VEH_MAX_PASSENGERS;++i)if(!VehicleSeatPermission(p,v->m_ppPassengers[i]))return qfalse;
 return qtrue;
}
static void VehicleRiderRelease(gentity_t*r){
 if(!r||!r->client||r->m_pVehicle)return;
 int n=r->client->ps.m_iVehicleNum;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||!g_entities[n].inuse||!VehicleHasRider(&g_entities[n],r))return;
 EndActor(n,qfalse);ForceDetach(n);
 VehicleDriftClear(&g_entities[n]);
}
static void VehicleRiderInput(gentity_t*r,const usercmd_t*cmd){
 if(!r||!r->client||r->m_pVehicle||!cmd)return;
 int n=r->client->ps.m_iVehicleNum;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||!actors[n].active||!VehicleHasRider(&g_entities[n],r))return;
 // Release first so native USE/jump dismount handling runs normally. Input is
 // never rewritten, and a rider cannot be immediately re-grabbed on that tick.
 if((cmd->buttons&BUTTON_USE)||cmd->upmove>0){
  if(r->s.number<MAX_CLIENTS)players[r->s.number].noGrabUntil=level.time+1000;
  VehicleRiderRelease(r);
 }
}
