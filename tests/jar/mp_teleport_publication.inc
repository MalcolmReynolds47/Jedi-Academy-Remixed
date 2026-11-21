// Actual portable sensor publication and native TeleportPlayer touch.
static qboolean teleportPVS;
static qboolean TeleportInPVS(const vec3_t start,const vec3_t end){return teleportPVS;}

static int pilotGrantPVS,pilotGrantTraces,pilotGrantHide,pilotGrantBlock;
static int PilotGrantEntities(const vec3_t mins,const vec3_t maxs,int*found,int capacity){
 int count=0;
 for(int n=0;n<level.num_entities&&count<capacity;++n){
  const gentity_t*e=&g_entities[n];
  if(e->inuse&&e->r.linked&&PhysicsBoxesOverlap(mins,maxs,e->r.absmin,e->r.absmax))found[count++]=n;
 }
 return count;
}
static qboolean PilotGrantInPVS(const vec3_t from,const vec3_t to){
 return ++pilotGrantPVS!=pilotGrantHide;
}
static void PilotGrantTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,
                            const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 assert(VectorCompare(start,end)&&mask==MASK_PLAYERSOLID);
 assert(teleportAssembly[0]->r.linked&&teleportAssembly[1]->r.linked);
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 t->startsolid=++pilotGrantTraces==pilotGrantBlock;
}
static void TeleportVehiclePublicationCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 int (*priorEntitiesInBox)(const vec3_t,const vec3_t,int*,int)=imports.EntitiesInBox;
 static gclient_t highPilot;
 for(int family=0;family<4;++family){
  ArrivalFixture(types[family]);imports.Trace=Trace;
  gentity_t*e=teleportAssembly[0],*pilot=teleportAssembly[1];
  gentity_t*source=TransportSpawn("trigger_teleport",1500),*dest=TransportDestination(source);
  assert(source&&dest);
  gentity_t*h=TriggerHelper(source->s.number);
  const int n=source->s.number;
  Vehicle_t*v=e->m_pVehicle;
  e->s.NPC_class=CLASS_VEHICLE;e->s.owner=1;e->client->ps.clientNum=e->s.number;
  Move(dest,(vec3_t){2200.25f,12.5f,100});
  VectorSet(dest->r.currentAngles,20,90,5);
  vec3_t hullSource,pilotSource,mins,maxs;
  VectorCopy(source->r.currentOrigin,hullSource);hullSource[0]+=90;
  VectorCopy(source->r.currentOrigin,pilotSource);pilotSource[0]+=108;
  assert(VehicleBounds(e,e->r.currentAngles,mins,maxs));
  VectorCopy(mins,e->r.mins);VectorCopy(maxs,e->r.maxs);
  Move(e,hullSource);Move(pilot,pilotSource);
  e->client->ps.commandTime=pilot->client->ps.commandTime=level.time;
  e->client->ps.groundEntityNum=pilot->client->ps.groundEntityNum=8;
  e->client->ps.speed=1200;VectorSet(e->client->ps.jarVehicleDrift,80,50,0);
  VectorSet(e->client->ps.velocity,1000,400,200);VectorSet(pilot->client->ps.velocity,10,5,1);
  e->client->pers.cmd.angles[PITCH]=100;
  e->client->pers.cmd.angles[YAW]=200;
  e->client->pers.cmd.angles[ROLL]=300;
  pilot->client->pers.cmd=e->client->pers.cmd;
  players[0].noTools=players[2].noTools=1;
  imports.InPVS=PilotGrantInPVS;imports.EntitiesInBox=PilotGrantEntities;
  TriggerSync(n);assert((h->r.svFlags&SVF_NOCLIENT)&&!h->s.trickedentindex3);
  v->m_iNumPassengers=0;v->m_ppPassengers[0]=NULL;
  TriggerSync(n);assert((h->r.svFlags&SVF_NOCLIENT)&&!h->s.trickedentindex3);
  v->m_pDroidUnit=NULL;
  imports.Trace=PilotGrantTrace;
  pilotGrantPVS=pilotGrantTraces=pilotGrantHide=pilotGrantBlock=0;
  const int beforeEvents=teleportEvents,beforeKills=teleportKillBoxes;
  gentity_t beforeHull=*e,beforePilot=*pilot;
  TriggerSync(n);
  assert(!(h->r.svFlags&SVF_NOCLIENT)&&h->s.otherEntityNum2==e->s.number&&h->s.trickedentindex3==2);
  assert(!h->s.trickedentindex&&!h->s.trickedentindex2&&!h->s.trickedentindex4);
  assert(pilotGrantPVS==2&&pilotGrantTraces==2&&teleportGrantBudget==64-TELEPORT_VEHICLE_GRANT_WORK);
  assert(!memcmp(e,&beforeHull,sizeof(*e))&&!memcmp(pilot,&beforePilot,sizeof(*pilot)));
  assert(teleportEvents==beforeEvents&&teleportKillBoxes==beforeKills);
  const entityState_t snapshot=h->s;
  playerState_t predictedHull=e->client->ps,predictedPilot=pilot->client->ps;
  jarVehicleTeleportArrival_t arrival;
  assert(!JAR_TeleportVolumeNativeContact(&snapshot,pilotSource,pilot->r.mins,pilot->r.maxs));
  assert(JAR_TeleportVolumeVehicleCandidate(&snapshot,&predictedHull,&predictedPilot,
     &pilot->client->pers.cmd,e->r.mins,e->r.maxs,&arrival));
  JAR_TeleportVolumeApplyVehicle(&snapshot,&predictedHull,&predictedPilot,&pilot->client->pers.cmd,&arrival);

  // Visibility and collision on either member suppress a previously published grant.
  for(int member=1;member<=2;++member){
   pilotGrantPVS=pilotGrantTraces=0;pilotGrantHide=member;
   TriggerSync(n);assert((h->r.svFlags&SVF_NOCLIENT)&&!h->s.trickedentindex3&&h->s.otherEntityNum2==ENTITYNUM_NONE);
   assert(pilotGrantPVS==member&&pilotGrantTraces==member-1);pilotGrantHide=0;
   pilotGrantPVS=pilotGrantTraces=0;pilotGrantBlock=member;
   TriggerSync(n);assert((h->r.svFlags&SVF_NOCLIENT)&&!h->s.trickedentindex3&&h->s.otherEntityNum2==ENTITYNUM_NONE);
   assert(pilotGrantPVS==member&&pilotGrantTraces==member);pilotGrantBlock=0;
  }
  // The new payload clears on permission or linkage change.
  players[1].noTools=1;TriggerSync(n);
  assert(!h->s.trickedentindex3&&!VectorLength(h->s.origin)&&!VectorLength(h->s.boneAngles1));
  players[1].noTools=0;pilot->client->pers.cmd.buttons=BUTTON_USE;TriggerSync(n);assert(!h->s.trickedentindex3);
  pilot->client->pers.cmd.buttons=0;v->m_iBoarding=1;TriggerSync(n);assert(!h->s.trickedentindex3);v->m_iBoarding=0;
  v->m_iTurboTime=level.time+100;TriggerSync(n);assert(!h->s.trickedentindex3);v->m_iTurboTime=level.time;
  e->client->ps.vehSurfaces=1;TriggerSync(n);assert(!h->s.trickedentindex3);e->client->ps.vehSurfaces=0;
  e->s.owner=2;TriggerSync(n);assert(!h->s.trickedentindex3);e->s.owner=1;
  arrivalBoltFail=1;TriggerSync(n);assert(!h->s.trickedentindex3);arrivalBoltFail=0;
  arrivalMatrixFail=1;TriggerSync(n);assert(!h->s.trickedentindex3);arrivalMatrixFail=0;
  imports.Trace=Trace;
  gentity_t*competing=G_Spawn();
  competing->s.eType=ET_TELEPORT_TRIGGER;competing->r.contents=CONTENTS_TRIGGER;
  competing->r.svFlags=SVF_NOCLIENT;competing->touch=TriggerTouch;
  VectorSet(competing->r.mins,-32,-32,-32);VectorSet(competing->r.maxs,32,32,32);
  Move(competing,(vec3_t){3500,0,100});TriggerSync(n);assert(h->s.trickedentindex3==2);
  Move(competing,hullSource);TriggerSync(n);assert(!h->s.trickedentindex3);G_FreeEntity(competing);
  gentity_t*rope=CopyLink(0,e->s.number,ENTITYNUM_WORLD);
  assert(rope);TriggerSync(n);assert(!h->s.trickedentindex3);G_FreeEntity(rope);
  teleportGrantBudget=TELEPORT_VEHICLE_GRANT_WORK-1;TriggerSync(n);assert(!h->s.trickedentindex3);

  // A server touch still commits through the normal teleport and seat plan.
  imports.InPVS=NULL;
  TriggerEnter(source,e);
  assert(teleportEvents==beforeEvents+2&&teleportKillBoxes==beforeKills+1);
  assert(VectorCompare(snapshot.origin,e->client->ps.origin)&&VectorCompare(snapshot.angles,pilot->client->ps.origin));
  assert(VectorCompare(snapshot.pos.trDelta,e->r.mins)&&VectorCompare(snapshot.apos.trDelta,e->r.maxs));
  assert(VectorCompare(snapshot.boneAngles1,pilot->r.mins)&&VectorCompare(snapshot.boneAngles2,pilot->r.maxs));
  const playerState_t*server[]={&e->client->ps,&pilot->client->ps};
  const playerState_t*predicted[]={&predictedHull,&predictedPilot};
  for(int member=0;member<2;++member){
   const playerState_t*state=server[member],*guess=predicted[member];
   assert(VectorCompare(state->origin,guess->origin)&&VectorCompare(state->velocity,guess->velocity));
   assert(VectorCompare(state->viewangles,guess->viewangles)&&VectorCompare(state->vehOrientation,guess->vehOrientation));
   assert(VectorCompare(state->jarVehicleDrift,guess->jarVehicleDrift)&&state->speed==guess->speed);
   assert(!memcmp(state->delta_angles,guess->delta_angles,sizeof(state->delta_angles)));
   assert(state->eFlags==guess->eFlags&&state->groundEntityNum==guess->groundEntityNum&&
          state->pm_flags==guess->pm_flags&&state->pm_time==guess->pm_time);
  }
  assert(e->s.owner==1&&e->r.linked&&pilot->r.linked);

  // Reuse the fixture with a high client slot to verify the second mask.
  Move(e,hullSource);Move(pilot,pilotSource);
  g_entities[31]=*pilot;highPilot=*pilot->client;
  g_entities[31].s.number=31;g_entities[31].client=&highPilot;g_entities[31].playerState=&highPilot.ps;
  highPilot.ps.clientNum=31;
  v->m_pPilot=v->m_pOldPilot=(bgEntity_t*)&g_entities[31];
  pilot->client->ps.m_iVehicleNum=0;players[1].noTools=1;
  e->s.owner=31;e->client->ps.m_iVehicleNum=32;
  ++level.time;imports.InPVS=PilotGrantInPVS;
  TriggerSync(n);assert(!h->s.trickedentindex3&&h->s.trickedentindex4==32768&&h->s.otherEntityNum2==e->s.number);
  imports.InPVS=NULL;PhysicsReset();
 }
 imports.EntitiesInBox=priorEntitiesInBox;
 puts("MP vehicle teleport grants: four hulls, exact pilot and hull arrivals, read-only checks, consent, lifetime, budget, obstruction and high pilot mask passed.");
}

static void TeleportPublicationCases(void){
 RigidFixture();
 gentity_t*source=TransportSpawn("trigger_teleport",700),*dest=TransportDestination(source),*p=&g_entities[0];
 assert(source&&dest);int n=source->s.number,d=dest->s.number;gentity_t*h=TriggerHelper(n);
 Move(dest,(vec3_t){975.5f,12.25f,100});VectorSet(dest->r.currentAngles,17.5f,64.75f,0);
 TriggerPlaceActor(p,source);p->client->ps.pm_type=PM_NORMAL;p->client->ps.commandTime=level.time;p->client->ps.groundEntityNum=19;
 imports.InPVS=TeleportInPVS;teleportPVS=qtrue;
 TriggerSync(n);assert(!(h->r.svFlags&SVF_NOCLIENT)&&JAR_TeleportVolumeValid(&h->s)&&JAR_PushVolumeAllowed(&h->s,0));
 entityState_t snapshot=h->s;playerState_t predicted=p->client->ps;usercmd_t cmd={0};vec3_t origin,final;
 assert(JAR_TeleportVolumeCandidate(&snapshot,&predicted,&cmd,p->r.mins,p->r.maxs,origin,final));JAR_TeleportVolumeApply(&snapshot,&predicted,&cmd,origin);
 int events=teleportEvents,kills=teleportKillBoxes;blocked=1;TriggerEnter(source,p);
 assert(VectorCompare(p->client->ps.origin,source->r.currentOrigin)&&teleportEvents==events&&teleportKillBoxes==kills&&(h->r.svFlags&SVF_NOCLIENT));
 blocked=0;TriggerEnter(source,p);
 assert(teleportEvents==events+2&&teleportKillBoxes==kills+1);
 assert(VectorCompare(predicted.origin,p->client->ps.origin)&&VectorCompare(predicted.velocity,p->client->ps.velocity)&&VectorCompare(predicted.viewangles,p->client->ps.viewangles));
 assert(predicted.pm_time==p->client->ps.pm_time&&predicted.pm_flags==p->client->ps.pm_flags&&predicted.eFlags==p->client->ps.eFlags&&predicted.groundEntityNum==p->client->ps.groundEntityNum);
 TriggerPlaceActor(p,source);players[0].noTools=1;TriggerSync(n);assert(!JAR_PushVolumeAllowed(&h->s,0));players[0].noTools=0;
 objects[d].heldBy=1;TriggerSync(n);assert(h->r.svFlags&SVF_NOCLIENT);objects[d].heldBy=0;
 objects[n].frozen=0;TriggerSync(n);assert(h->r.svFlags&SVF_NOCLIENT);objects[n].frozen=1;
 teleportPVS=qfalse;TriggerSync(n);assert(h->r.svFlags&SVF_NOCLIENT);teleportPVS=qtrue;
 source->spawnflags=1;TriggerSync(n);assert(h->r.svFlags&SVF_NOCLIENT);source->spawnflags=0;
 teleportGrantBudget=0;TriggerSync(n);assert(h->r.svFlags&SVF_NOCLIENT);++level.time;TriggerSync(n);assert(!(h->r.svFlags&SVF_NOCLIENT));
 imports.InPVS=NULL;assert(WorldClear());PhysicsReset();
 puts("MP teleport grants: actual native motion parity, snapshot cannot bypass newly obstructed arrival, consent, frozen endpoints, visibility and frame work limit passed.");
 TeleportVehiclePublicationCases();
}
