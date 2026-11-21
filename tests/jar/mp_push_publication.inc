static void VehiclePushPublicationCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int family=0;family<4;++family){
  ArrivalFixture(types[family]);imports.Trace=Trace;gentity_t*v=teleportAssembly[0],*pilot=&g_entities[1],*passenger=&g_entities[2],*e=TransportSpawn("trigger_push",1200),*h=TriggerHelper(e->s.number);
  TriggerPlaceActor(v,e);v->client->ps.clientNum=v->s.number;pilot->client->ps.clientNum=1;
  TriggerSync(e->s.number);assert(JAR_PushVolumePilotAllowed(&h->s,1)&&!JAR_PushVolumePilotAllowed(&h->s,2)&&!JAR_PushVolumeAllowed(&h->s,1)&&!(h->r.svFlags&SVF_NOCLIENT));
  for(int mode=0;mode<3;++mode){
   e->spawnflags=mode==0?4:mode==1?20:0;e->wait=0;e->painDebounceTime=level.time-1000;TriggerSync(e->s.number);playerState_t predicted=v->client->ps;predicted.commandTime=level.time;jarPushPrediction_t pending={h->s.time,qfalse};
   assert(JAR_PushVolumePredictVehicle(&h->s,&pending,&predicted,&pilot->client->ps,v->r.mins,v->r.maxs));TriggerEnter(e,v);
   assert(Distance(predicted.velocity,v->client->ps.velocity)<.001f&&VectorCompare(predicted.jarVehicleDrift,v->client->ps.jarVehicleDrift)&&predicted.speed==v->client->ps.speed);
  }
  for(int refusal=0;refusal<9;++refusal){
   switch(refusal){case 0:players[1].noTools=1;break;case 1:players[2].noTools=1;break;case 2:players[2].noGrabUntil=level.time+1000;break;case 3:v->m_pVehicle->m_iBoarding=1;break;case 4:actors[v->s.number].active=1;break;case 5:passenger->client->ps.m_iVehicleNum=0;break;case 6:teleportAssembly[3]->client->ps.m_iVehicleNum=0;break;case 7:playerTools.integer=0;break;case 8:sharedEditing.integer=0;objects[v->s.number].owner=2;break;}
   TriggerSync(e->s.number);assert(!JAR_PushVolumePilotAllowed(&h->s,1));
   players[1].noTools=players[2].noTools=players[2].noGrabUntil=0;v->m_pVehicle->m_iBoarding=0;actors[v->s.number].active=0;passenger->client->ps.m_iVehicleNum=v->s.number;teleportAssembly[3]->client->ps.m_iVehicleNum=v->s.number;playerTools.integer=1;sharedEditing.integer=1;objects[v->s.number].owner=1;
   TriggerSync(e->s.number);assert(JAR_PushVolumePilotAllowed(&h->s,1));
  }
  e->flags|=FL_INACTIVE;TriggerSync(e->s.number);assert(!h->s.trickedentindex3&&!h->s.trickedentindex4&&(h->r.svFlags&SVF_NOCLIENT));e->flags&=~FL_INACTIVE;
  // Even if every connected player is seated, a permitted pilot needs the sensor.
  players[0].noTools=1;TriggerSync(e->s.number);assert(!h->s.trickedentindex&&!h->s.trickedentindex2&&h->s.trickedentindex3==2&&!(h->r.svFlags&SVF_NOCLIENT));players[0].noTools=0;
  // The last client slot uses the high bit of the second pilot field.
  static gclient_t highPilot;
  highPilot=*pilot->client;highPilot.ps.clientNum=31;
  g_entities[31]=*pilot;g_entities[31].s.number=31;g_entities[31].client=&highPilot;g_entities[31].playerState=&highPilot.ps;
  v->m_pVehicle->m_pPilot=v->m_pVehicle->m_pOldPilot=(bgEntity_t*)&g_entities[31];v->client->ps.m_iVehicleNum=32;TriggerSync(e->s.number);
  assert(!h->s.trickedentindex3&&h->s.trickedentindex4==32768&&JAR_PushVolumePilotAllowed(&h->s,31));
  PhysicsReset();
 }
 puts("MP vehicle push publication: native touch parity across four families, independent pilot masks, passenger consent, boarding/hold/owner refusals and invalid droid seat removal passed.");
}
static void PushPublicationCases(void){
 VehiclePushPublicationCases();
 RigidFixture();gentity_t*p=&g_entities[0],*e=TransportSpawn("trigger_push",100),*h=TriggerHelper(e->s.number),*dest=TransportDestination(e);
 TriggerSync(e->s.number);assert(JAR_PushVolumeValid(&h->s)&&!(h->r.svFlags&SVF_NOCLIENT)&&h->s.bolt2==1&&JAR_PushVolumeAllowed(&h->s,0)&&h->s.otherEntityNum==e->s.number);
 vec3_t authoritative,predicted,center,angles;
 for(int mode=0;mode<3;++mode){
  e->spawnflags=mode==0?0:mode==1?4:20;e->wait=0;e->painDebounceTime=level.time-60000;VectorSet(e->r.currentAngles,20,45,0);VectorCopy(e->r.currentOrigin,p->r.currentOrigin);p->r.currentOrigin[0]+=16;VectorCopy(p->r.currentOrigin,p->client->ps.origin);TriggerSync(e->s.number);
  assert(TransportPushVector(e,p->r.currentOrigin,p,authoritative)==1);JAR_PushVolumePose(&h->s,level.time,center,angles);assert(JAR_PushVolumeVelocity(h->s.frame,center,angles,h->s.bolt2?h->s.origin2:NULL,p->client->ps.origin,h->s.speed,h->s.angles2[0],predicted)==1&&Distance(authoritative,predicted)<.0001f);
  playerState_t ps=p->client->ps;ps.commandTime=level.time;ps.stats[STAT_HEALTH]=100;ps.clientNum=0;ps.pm_type=PM_NORMAL;jarPushPrediction_t pending={h->s.time,qfalse};assert(JAR_PushVolumePredict(&h->s,&pending,&ps,p->r.mins,p->r.maxs)&&Distance(ps.velocity,predicted)<.0001f);
 }
 players[0].noTools=1;TriggerSync(e->s.number);assert(!JAR_PushVolumeAllowed(&h->s,0));players[0].noTools=0;actors[0].active=1;TriggerSync(e->s.number);assert(!JAR_PushVolumeAllowed(&h->s,0));actors[0].active=0;playerTools.integer=0;TriggerSync(e->s.number);assert(h->r.svFlags&SVF_NOCLIENT);playerTools.integer=1;
 objects[dest->s.number].owner=2;sharedEditing.integer=0;TriggerSync(e->s.number);assert(h->r.svFlags&SVF_NOCLIENT);objects[dest->s.number].owner=1;TriggerSync(e->s.number);assert(!(h->r.svFlags&SVF_NOCLIENT));
 e->flags|=FL_INACTIVE;TriggerSync(e->s.number);assert(!h->r.linked&&(h->r.svFlags&SVF_NOCLIENT)&&!h->s.trickedentindex);e->flags&=~FL_INACTIVE;
 objects[e->s.number].frozen=0;VectorSet(objects[e->s.number].velocity,60,70,80);VectorSet(e->s.apos.trDelta,0,90,0);TriggerSync(e->s.number);assert(h->s.pos.trTime==level.time&&h->s.pos.trDelta[1]==70&&h->s.apos.trDelta[1]==90);
 objects[e->s.number].frozen=1;TriggerSync(e->s.number);assert(!VectorLength(h->s.pos.trDelta)&&!VectorLength(h->s.apos.trDelta));
 // Preserve the native normalized helper payload only during a touch; then
 // republish the full target/geometry/timing contract for client prediction.
 e->spawnflags=4;e->wait=500;TriggerPlaceActor(p,e);TriggerEnter(e,p);assert(h->s.time==e->painDebounceTime&&h->s.time2==500&&VectorCompare(h->s.origin2,dest->r.currentOrigin));
 gentity_t*prop=CopySpawn(1,0,1400);objects[prop->s.number].frozen=0;e->painDebounceTime=level.time-1000;assert(TriggerPushProp(e,prop,e->r.currentOrigin));assert(h->s.time==level.time&&h->s.time2==500);
 assert(WorldClear());PhysicsReset();puts("MP push publication: server/client velocity parity, current actor/owner consent, disabled removal, motion snapshots and post-touch payload restoration passed.");
}
