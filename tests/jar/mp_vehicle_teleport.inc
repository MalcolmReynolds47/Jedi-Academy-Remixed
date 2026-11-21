// Real controller commands, native TeleportPlayer and native seat attachment.
// Trace/Ghoul2 imports model deterministic map and bolt boundaries, not rendering.
static gentity_t*teleportAssembly[4];
static gentity_t*liftDeck;
static int liftSupportHit,liftSupportTraces,liftBlockedRider;
static void VehicleLiftTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(!mins)return;
 if(liftDeck->r.linked){
  assert(pass==teleportAssembly[0]->s.number&&end[2]<start[2]);
  ++liftSupportTraces;t->fraction=.25f;t->entityNum=liftSupportHit;
  return;
 }
 if(liftSupportHit==liftDeck->s.number)
  for(int i=0;i<4;++i)assert(!teleportAssembly[i]->r.linked);
 if(pass==liftBlockedRider)t->fraction=.5f;
}
static void VehicleLiftCases(void){
 const vehicleType_t types[]={VH_WALKER,VH_SPEEDER,VH_FIGHTER};
 for(int mode=0;mode<ARRAY_LEN(types);++mode){
  RigidFixture();gentity_t* hull=SeatVehicle(types[mode]);Vehicle_t* v=hull->m_pVehicle;
  gentity_t* droid=ActorNPC(-1,0);droid->client->ps.m_iVehicleNum=hull->s.number;v->m_pDroidUnit=(bgEntity_t*)droid;
  teleportAssembly[0]=hull;teleportAssembly[1]=&g_entities[1];teleportAssembly[2]=&g_entities[2];teleportAssembly[3]=droid;
  playerTools.integer=1;sharedEditing.integer=1;imports.EntitiesInBox=RigidNearbyBoxes;
  liftDeck=SpawnWithController(&g_entities[0],JAR_LIFT_ID,(vec3_t){120,0,80},vec3_origin);assert(liftDeck);
  const int deck=liftDeck->s.number;
  const float top=liftDeck->r.currentOrigin[2]+liftDeck->r.maxs[2];
  const float gap=mode==1?12:mode==2?6:0;
  VectorSet(hull->r.mins,-6,-4,-8);VectorSet(hull->r.maxs,6,4,8);hull->r.contents=CONTENTS_BODY;
  for(int i=0;i<4;++i){
   gentity_t* e=teleportAssembly[i];Move(e,(vec3_t){120,0,top+8+gap+i*12});
   e->client->ps.velocity[0]=7;e->client->ps.eventSequence=17;e->client->ps.groundEntityNum=i==0&&mode==0?deck:ENTITYNUM_NONE;
  }
  Unlink((sharedEntity_t*)droid);
  copyVehicleInfo.hoverHeight=mode==1?32:0;copyVehicleInfo.landingHeight=mode==2?80:0;
  v->m_LandTrace.entityNum=deck;v->m_LandTrace.fraction=.1f;v->m_LandTrace.plane.normal[2]=1;v->m_LandTrace.endpos[2]=top;
  liftSupportHit=deck;liftSupportTraces=0;liftBlockedRider=g_entities[2].s.number;imports.Trace=VehicleLiftTrace;
  assert(!LiftTranslate(liftDeck,10)&&liftDeck->r.currentOrigin[2]==80&&hull->r.currentOrigin[2]==top+8+gap);
  liftBlockedRider=-1;players[2].noTools=1;assert(!LiftTranslate(liftDeck,10));players[2].noTools=0;
  v->m_iBoarding=1;assert(!LiftTranslate(liftDeck,10));v->m_iBoarding=0;
  if(mode){
   liftSupportHit=ENTITYNUM_WORLD;
   assert(LiftTranslate(liftDeck,4)&&hull->r.currentOrigin[2]==top+8+gap);
   assert(LiftTranslate(liftDeck,-4));liftSupportHit=deck;
  }
  objects[deck].owner=MAX_CLIENTS+1;sharedEditing.integer=0;assert(!LiftTranslate(liftDeck,10));sharedEditing.integer=1;
  assert(LiftTranslate(liftDeck,10)&&liftDeck->r.currentOrigin[2]==90&&v->m_LandTrace.endpos[2]==top+10);
  assert((liftSupportTraces>0)==(mode!=0));
  for(int i=0;i<4;++i){
   const gentity_t* e=teleportAssembly[i];const playerState_t* ps=&e->client->ps;
   assert(e->r.linked==(i!=3)&&e->r.currentOrigin[2]==top+18+gap+i*12&&ps->origin[2]==e->r.currentOrigin[2]);
   assert(ps->eventSequence==17&&ps->velocity[0]==7&&(i==0||ps->m_iVehicleNum==hull->s.number));
  }
  assert(LiftTranslate(liftDeck,-10)&&liftDeck->r.currentOrigin[2]==80&&v->m_LandTrace.endpos[2]==top);
 }
 imports.Trace=RigidTrace;
 puts("MP lift vehicles: grounded, hovered and landed hulls, native seats, obstruction, consent and link-state preservation passed.");
}
static int arrivalTraces,arrivalBlock,arrivalMatrixFail,arrivalBoltFail,arrivalAllSolid;
static int ArrivalBolt(void*g,int model,const char*name){return arrivalBoltFail?-1:SeatBolt(g,model,name);}
static qboolean ArrivalMatrix(void*g,const int model,const int bolt,mdxaBone_t*m,const vec3_t angle,const vec3_t origin,const int time,qhandle_t*list,vec3_t scale){
 if(arrivalMatrixFail)return qfalse;return SeatMatrix(g,model,bolt,m,angle,origin,time,list,scale);
}
static void ArrivalTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mins){
  ++arrivalTraces;for(int i=0;i<4;++i)assert(!teleportAssembly[i]->r.linked);
  t->startsolid=arrivalTraces==arrivalBlock;t->allsolid=arrivalAllSolid;
 }
}
static gentity_t*ArrivalFixture(vehicleType_t type){
 RigidFixture();gentity_t*e=SeatVehicle(type);Vehicle_t*v=e->m_pVehicle;
 gentity_t*d=ActorNPC(-1,0);d->client->ps.m_iVehicleNum=e->s.number;v->m_pDroidUnit=(bgEntity_t*)d;v->m_iDroidUnitTag=5;
 teleportAssembly[0]=e;teleportAssembly[1]=&g_entities[1];teleportAssembly[2]=&g_entities[2];teleportAssembly[3]=d;
 for(int i=1;i<4;++i){gentity_t*r=teleportAssembly[i];VectorSet(r->r.mins,-16,-16,-24);VectorSet(r->r.maxs,16,16,40);r->client->ps.eventSequence=17;r->client->ps.entityEventSequence=11;}
 gentity_t*tele=TransportSpawn("target_teleporter",900);assert(tele);gentity_t*dest=TransportDestination(tele);assert(dest);
 VectorSet(dest->r.currentAngles,20,90,5);
 for(int i=0;i<4;++i){RigidLink((sharedEntity_t*)teleportAssembly[i]);teleportAssembly[i]->client->ps.eFlags=EF_TELEPORT_BIT;teleportAssembly[i]->s.eFlags|=EF_TELEPORT_BIT;}
 sharedEditing.integer=1;arrivalTraces=arrivalBlock=arrivalMatrixFail=arrivalBoltFail=arrivalAllSolid=0;blockAt=0;
 imports.Trace=ArrivalTrace;imports.G2API_AddBolt=ArrivalBolt;imports.G2API_GetBoltMatrix=ArrivalMatrix;
 return tele;
}
static void VehicleTeleportCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int kind=0;kind<4;++kind){
  gentity_t*tele=ArrivalFixture(types[kind]),*e=teleportAssembly[0],*dest=TransportDestination(tele);Vehicle_t*v=e->m_pVehicle;
  e->client->ps.speed=1200;VectorSet(e->client->ps.jarVehicleDrift,30,40,0);v->m_vAngularVelocity=50;v->m_ucmd.forwardmove=127;v->m_iTurboTime=level.time+2000;
  vec3_t lo,hi;assert(VehicleBounds(e,dest->r.currentAngles,lo,hi));
  int events=teleportEvents,kills=teleportKillBoxes,snapshots=snapshotCalls;
  aimed=tele->s.number;Command(1,"jar_select","");assert(players[1].selected==tele->s.number);
  Command(1,"jar_property","activate");
  assert(arrivalTraces==4&&teleportEvents==events+2&&teleportKillBoxes==kills+1&&snapshotCalls==snapshots+1);
  assert(e->r.currentOrigin[0]==dest->r.currentOrigin[0]&&e->r.currentOrigin[2]==dest->r.currentOrigin[2]+dest->r.maxs[2]-lo[2]+5);
  assert(VectorCompare(e->r.mins,lo)&&VectorCompare(e->r.maxs,hi)&&VectorCompare(e->r.currentAngles,dest->r.currentAngles));
  assert(e->s.eType==ET_NPC&&VectorCompare(e->client->ps.vehOrientation,e->r.currentAngles)&&e->client->ps.speed==400&&fabsf(VectorLength(e->client->ps.velocity)-400)<.001f);
  assert(!VectorLength(e->client->ps.jarVehicleDrift)&&!v->m_vAngularVelocity&&!v->m_ucmd.forwardmove&&v->m_iTurboTime<=level.time&&v->m_LandTrace.fraction==1);
  assert(VectorCompare(e->r.currentAngles,v->m_vPrevOrientation)&&VectorCompare(e->r.currentAngles,v->m_vPrevRiderViewAngles));
  for(int i=0;i<4;++i){gentity_t*r=teleportAssembly[i];assert(r->r.linked&&r->health==100&&!(r->client->ps.eFlags&EF_TELEPORT_BIT));
   assert(VectorCompare(r->r.currentOrigin,r->client->ps.origin)&&r->client->ps.groundEntityNum==ENTITYNUM_NONE);
   if(i){assert(!(r->s.eFlags&EF_TELEPORT_BIT)&&r->client->ps.m_iVehicleNum==e->s.number&&r->client->ps.eventSequence==17&&r->client->ps.entityEventSequence==11);}
  }
  assert(fabsf(clients[1].ps.origin[1]-e->r.currentOrigin[1]-8)<.001f&&clients[1].ps.origin[2]==e->r.currentOrigin[2]+24);
  assert(VectorCompare(clients[2].ps.origin,e->r.currentOrigin)&&fabsf(teleportAssembly[3]->r.currentOrigin[1]-e->r.currentOrigin[1]-13)<.001f);
  assert(v->m_pPilot==(bgEntity_t*)&g_entities[1]&&v->m_pOldPilot==v->m_pPilot&&v->m_ppPassengers[0]==(bgEntity_t*)&g_entities[2]&&v->m_iNumPassengers==1);
  vec3_t pilot;VectorCopy(clients[1].ps.origin,pilot);AttachRidersGeneric(v);assert(VectorCompare(pilot,clients[1].ps.origin));
  // A passenger can use C too; unrelated equipment and property writes stay gated.
  Command(2,"jar_select","");assert(players[2].selected==tele->s.number);int active=LiveCount();Command(2,"jar_spawn","1");assert(LiveCount()==active);
  int flags=tele->spawnflags;Command(2,"jar_property","controller_flags_1");assert(tele->spawnflags==flags);
  arrivalTraces=0;Command(2,"jar_property","activate");assert(arrivalTraces==4&&teleportEvents==events+4&&(clients[1].ps.eFlags&EF_TELEPORT_BIT));
  // Push can now calculate an occupied vehicle trajectory without moving it.
  vec3_t velocity={12,34,56};gentity_t*push=G_Spawn();int ci;assert(Entry(ControllerId("target_push"),&ci));Track(push,0,ci);
  assert(JAR_MP_PushVector(push,e,velocity)==1&&velocity[0]==0);G_FreeEntity(push);
 }
 // Each refusal leaves all transforms, snapshots, velocities and seat links intact.
 for(int refusal=0;refusal<25;++refusal){
  gentity_t*tele=ArrivalFixture(VH_FIGHTER),*e=teleportAssembly[0],*dest=TransportDestination(tele);Vehicle_t*v=e->m_pVehicle;
  switch(refusal){
   case 0:players[1].noTools=1;break;case 1:players[2].noTools=1;break;
   case 2:playerTools.integer=0;break;case 3:players[2].noGrabUntil=level.time+1000;break;
   case 4:objects[e->s.number].owner=2;sharedEditing.integer=0;break;
   case 5:v->m_iBoarding=-1;break;case 6:clients[2].ps.m_iVehicleNum=0;break;
   case 7:clients[2].pers.connected=CON_DISCONNECTED;break;
   case 8:actors[e->s.number].active=qtrue;break;
   case 9:arrivalBoltFail=1;break;case 10:arrivalMatrixFail=1;break;
   case 11:v->m_iDroidUnitTag=-1;break;
   case 12:arrivalBlock=1;break;case 13:arrivalBlock=2;break;case 14:arrivalBlock=3;break;case 15:arrivalBlock=4;break;
   case 16:arrivalAllSolid=1;break;case 17:dest->r.currentAngles[0]=NAN;break;
   case 18:dest->r.currentOrigin[2]=65535;break;
   case 19:imports.Trace=Trace;CopyLink(0,e->s.number,ENTITYNUM_WORLD);imports.Trace=ArrivalTrace;break;
   case 20:imports.Trace=Trace;CopyLink(0,2,ENTITYNUM_WORLD);imports.Trace=ArrivalTrace;break;
   case 21:objects[dest->s.number].owner=2;sharedEditing.integer=0;break;
   case 22:teleportAssembly[3]->client->ps.stats[STAT_HEALTH]=0;break;
   case 23:arrivalBlock=3;Unlink((sharedEntity_t*)teleportAssembly[2]);break;
   case 24:for(int n=level.num_entities;n<ENTITYNUM_WORLD;++n)g_entities[n].inuse=qtrue;break;
  }
  static gentity_t before[4];static gclient_t beforeClient[4];Vehicle_t vehicleBefore=*v;
  for(int i=0;i<4;++i){before[i]=*teleportAssembly[i];beforeClient[i]=*teleportAssembly[i]->client;}
  int events=teleportEvents,kills=teleportKillBoxes,snapshots=snapshotCalls;
  assert(JAR_MP_TeleportTarget(tele,&g_entities[1]));
  assert(teleportEvents==events&&teleportKillBoxes==kills&&snapshotCalls==snapshots&&!memcmp(&vehicleBefore,v,sizeof(vehicleBefore)));
  for(int i=0;i<4;++i){assert(!memcmp(&before[i],teleportAssembly[i],sizeof(gentity_t)));assert(!memcmp(&beforeClient[i],teleportAssembly[i]->client,sizeof(gclient_t)));}
 }
 // Delayed activation resolves the current vehicle, but still rechecks consent.
 gentity_t*delayedTele=ArrivalFixture(VH_FIGHTER);imports.Trace=Trace;
 gentity_t*delay=ControllerAt("target_delay",0,1800);ControllerLink(&g_entities[0],delay->s.number,qfalse);ControllerLink(&g_entities[0],delayedTele->s.number,qfalse);imports.Trace=ArrivalTrace;
 aimed=delay->s.number;Command(1,"jar_select","");Command(1,"jar_property","activate");int delayedEvents=teleportEvents;
 players[2].noTools=1;ControllerTick(delay,1000);assert(teleportEvents==delayedEvents);players[2].noTools=0;
 Command(1,"jar_property","activate");ControllerTick(delay,1000);assert(teleportEvents==delayedEvents+2&&arrivalTraces==4);
 // Native callbacks can also target an empty vehicle without inventing riders.
 gentity_t*tele=ArrivalFixture(VH_SPEEDER),*e=teleportAssembly[0];Vehicle_t*v=e->m_pVehicle;
 v->m_pPilot=v->m_pOldPilot=v->m_ppPassengers[0]=v->m_pDroidUnit=NULL;v->m_iNumPassengers=0;e->client->ps.m_iVehicleNum=0;
 imports.Trace=Trace;int events=teleportEvents;ControllerInvoke(tele,tele,e);assert(teleportEvents==events+2&&e->client->ps.speed==400);
 // Stale context identity and inactive controllers cannot teleport the assembly.
 tele=ArrivalFixture(VH_SPEEDER);aimed=tele->s.number;Command(1,"jar_select","");level.time+=110;
 strcpy(argument,"activate");strcpy(contextTokenArgument,"4294967295");events=teleportEvents;assert(JAR_MP_Command(&g_entities[1],"jar_property")&&teleportEvents==events);
 tele->flags|=FL_INACTIVE;Command(1,"jar_property","activate");assert(teleportEvents==events);
 imports.Trace=Trace;imports.G2API_AddBolt=SeatBolt;imports.G2API_GetBoltMatrix=SeatMatrix;PhysicsReset();
 puts("MP vehicle teleport: four hull families, seated C activation, native driver/passenger/droid placement, unchanged seats/events, collision/consent/ownership/boarding/missing-bolt rollback, attachments, empty vehicles and stale context rejection passed.");
}
