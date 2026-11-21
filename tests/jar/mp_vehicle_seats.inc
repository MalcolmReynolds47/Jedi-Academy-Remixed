// Production pilot attachment plus real Sandbox commands; only the engine's
// Ghoul2 matrix boundary is deterministic. This is not rendered seat acceptance.
int BG_GetTime(void){return level.time;}
void BG_GiveMeVectorFromMatrix(mdxaBone_t*m,int flags,vec3_t out){assert(flags==ORIGIN);for(int a=0;a<3;++a)out[a]=m->matrix[a][3];}
#include "game/bg_vehicle_rider.inc"
#include "cgame/cg_jar_vehicle_prediction.inc"
static int seatBolts,seatMatrices,seatMissingBolt;
static int SeatBolt(void*g,int model,const char*name){assert(g&&model==0&&!strcmp(name,"*driver"));++seatBolts;return 0;}
static qboolean SeatMatrix(void*g,const int model,const int bolt,mdxaBone_t*m,const vec3_t angle,const vec3_t origin,const int time,qhandle_t*list,vec3_t scale){
 assert(g&&model==0&&time==level.time);++seatMatrices;if(bolt==seatMissingBolt)return qfalse;
 vec3_t f,point;AngleVectors(angle,f,NULL,NULL);VectorMA(origin,8+bolt,f,point);point[2]+=24;memset(m,0,sizeof(*m));for(int a=0;a<3;++a)m->matrix[a][3]=point[a];return qtrue;
}
static gentity_t* SeatVehicle(vehicleType_t type){
 WorldFixture();memset(&copyVehicleInfo,0,sizeof(copyVehicleInfo));copyVehicleInfo.type=type;copyVehicleInfo.length=160;copyVehicleInfo.width=60;copyVehicleInfo.height=80;copyVehicleInfo.maxPassengers=1;copyVehicleInfo.AttachRiders=AttachRidersGeneric;
 gentity_t*e=CopySpawn(3,0,200);CopyReady(e);e->ghoul2=(void*)1;
 Vehicle_t*v=e->m_pVehicle;v->m_pPilot=(bgEntity_t*)&g_entities[1];v->m_pOldPilot=v->m_pPilot;v->m_ppPassengers[0]=(bgEntity_t*)&g_entities[2];v->m_iNumPassengers=1;
 e->client->ps.m_iVehicleNum=2;for(int c=1;c<=2;++c){clients[c].ps.m_iVehicleNum=e->s.number;g_entities[c].r.ownerNum=e->s.number;clients[c].ps.legsAnim=21;clients[c].ps.torsoAnim=22;Link((sharedEntity_t*)&g_entities[c]);}
 seatBolts=seatMatrices=0;seatMissingBolt=-1;return e;
}
static gentity_t* gripSeats[4];
static int gripObstruction,gripSweeps;
static void GripSeatTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(!mins)return;
 for(int i=0;i<ARRAY_LEN(gripSeats);++i)if(gripSeats[i]->s.number==pass){
  for(int j=0;j<ARRAY_LEN(gripSeats);++j)assert(!gripSeats[j]->r.linked);
  if(DistanceSquared(start,end)>.001f){++gripSweeps;if(i==gripObstruction)t->fraction=.5f;}
 }
}
static void VehicleGripClearanceCases(void){
 gentity_t*e=SeatVehicle(VH_FIGHTER);const int n=e->s.number;
 gentity_t*d=ActorNPC(-1,0);d->client->ps.m_iVehicleNum=n;e->m_pVehicle->m_pDroidUnit=(bgEntity_t*)d;e->m_pVehicle->m_iDroidUnitTag=5;
 gripSeats[0]=e;gripSeats[1]=&g_entities[1];gripSeats[2]=&g_entities[2];gripSeats[3]=d;
 // Establish native seating before the player grabs the hull.
 VehicleAttachRiders(e);Unlink((sharedEntity_t*)d);
 VectorAdd(e->r.currentOrigin,((vec3_t){-16,3,10}),physgunFixtureHit);
 VectorSet(clients[0].ps.origin,physgunFixtureHit[0]-200,physgunFixtureHit[1],physgunFixtureHit[2]-24);
 imports.Trace=PhysgunFixtureTrace;
 seatMissingBolt=5;ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE&&!actors[n].active);
 seatMissingBolt=-1;ActorArm(0,n);assert(players[0].held==n);
 assert(e->r.linked&&g_entities[1].r.linked&&g_entities[2].r.linked&&!d->r.linked);
 vec3_t previous[4],facing,point;VectorCopy(e->r.currentAngles,facing);
 for(int i=0;i<4;++i)VectorCopy(gripSeats[i]->r.currentOrigin,previous[i]);
 JAR_GripPoint(&players[0].grip,e->r.currentOrigin,facing,point);point[0]+=20;
 imports.Trace=GripSeatTrace;
 for(gripObstruction=0;gripObstruction<4;++gripObstruction){
  gripSweeps=0;PhysgunMoveNative(e,&players[0].grip,point,facing,.025f);
  assert(gripSweeps==gripObstruction+1&&!VectorLengthSquared(objects[n].velocity));
  for(int i=0;i<4;++i){assert(VectorCompare(previous[i],gripSeats[i]->r.currentOrigin));assert(gripSeats[i]->r.linked==(i<3));}
 }
 gripObstruction=1;
 JAR_GripPoint(&players[0].grip,e->r.currentOrigin,e->r.currentAngles,point);facing[YAW]+=8;
 PhysgunMoveNative(e,&players[0].grip,point,facing,.025f);
 assert(fabsf(AngleSubtract(e->r.currentAngles[YAW],facing[YAW]-8))<.01f);
 gripObstruction=-1;gripSweeps=0;
 PhysgunMoveNative(e,&players[0].grip,point,facing,.025f);
 assert(gripSweeps==4&&fabsf(AngleSubtract(e->r.currentAngles[YAW],facing[YAW]))<.01f);
 assert(VectorCompare(e->r.currentAngles,e->client->ps.vehOrientation));
 for(int i=0;i<4;++i){
  gentity_t*m=gripSeats[i];assert(m->r.linked==(i<3)&&m->health==100);
  assert(VectorCompare(m->r.currentOrigin,m->client->ps.origin));
 }
 assert(VectorCompare(e->r.currentOrigin,g_entities[2].r.currentOrigin));
 JAR_GripPoint(&players[0].grip,e->r.currentOrigin,e->r.currentAngles,point);point[0]+=20;
 PhysgunMoveNative(e,&players[0].grip,point,facing,.025f);
 const float throwSpeed=objects[n].velocity[0];assert(fabsf(throwSpeed-800)<.01f);
 for(int i=0;i<4;++i)assert(!(gripSeats[i]->client->ps.eFlags&EF_TELEPORT_BIT));
 imports.Trace=Trace;Drop(0,qfalse);
 assert(e->client->ps.velocity[0]==throwSpeed&&e->client->ps.jarVehicleDrift[0]==throwSpeed);
}
static void VehicleSeatCases(void){
 imports.G2API_AddBolt=SeatBolt;imports.G2API_GetBoltMatrix=SeatMatrix;
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int kind=0;kind<4;++kind){
  gentity_t*e=SeatVehicle(types[kind]);int n=e->s.number;Vehicle_t*v=e->m_pVehicle;
  assert(ActorAlive(e)&&ActorEdit(&g_entities[0],n)&&CopyOccupied(e));
  players[2].noTools=1;ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE);players[2].noTools=0;
  playerTools.integer=0;ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE);playerTools.integer=1;
  VectorAdd(e->r.currentOrigin,((vec3_t){-16,3,10}),physgunFixtureHit);
  VectorSet(clients[0].ps.origin,physgunFixtureHit[0]-200,physgunFixtureHit[1],physgunFixtureHit[2]-24);
  imports.Trace=PhysgunFixtureTrace;ActorArm(0,n);imports.Trace=Trace;
  assert(players[0].held==n&&actors[n].active&&e->client->ps.pm_type==PM_FREEZE);
  assert(seatBolts&&seatMatrices&&fabsf(clients[1].ps.origin[0]-e->r.currentOrigin[0]-8)<.001f);
  assert(VectorCompare(g_entities[1].r.currentOrigin,clients[1].ps.origin)&&VectorCompare(g_entities[2].r.currentOrigin,e->r.currentOrigin));
  for(int i=0;i<3;++i)ActorTurnInput(&g_entities[0],30,0);
  for(int i=0;i<10;++i)ActorTick();
  assert(fabsf(e->r.currentAngles[YAW]-90)<.01f&&fabsf(clients[1].ps.origin[1]-e->r.currentOrigin[1]-8)<.001f);
  assert(VectorCompare(clients[1].ps.origin,g_entities[1].s.pos.trBase)&&g_entities[1].r.linked);
  for(int c=1;c<=2;++c)assert(clients[c].ps.m_iVehicleNum==n&&clients[c].ps.legsAnim==21&&clients[c].ps.torsoAnim==22&&clients[c].ps.pm_type==PM_NORMAL&&g_entities[c].health==100);
  vec3_t old;VectorCopy(e->r.currentOrigin,old);blocked=1;ActorTick();assert(VectorCompare(old,e->r.currentOrigin));blocked=0;
  VectorSet(objects[n].velocity,40,60,80);Drop(0,qfalse);assert(!actors[n].active&&e->client->ps.velocity[2]==80&&e->client->ps.jarVehicleDrift[2]==0&&e->client->ps.jarVehicleDrift[1]==60&&v->m_pPilot==(bgEntity_t*)&g_entities[1]);
  ActorArm(0,n);JAR_MP_Fire(&g_entities[0],qtrue);assert(objects[n].frozen&&actors[n].active);aimed=n;Command(0,"jar_select","");
  int caps=ContextCaps(&g_entities[0],n);assert((caps&JAR_CONTEXT_HEALTH)&&(caps&JAR_CONTEXT_ROTATE)&&!(caps&JAR_CONTEXT_REMOVE));
  Command(0,"jar_property","health_317");assert(e->health==317&&v->m_iArmor==317);Command(0,"jar_property","property_undo");assert(e->health==100&&v->m_iArmor==100);
  Command(0,"jar_property","rotate");assert(fabsf(e->r.currentAngles[YAW]-105)<.01f);
  CopyAssembly(&g_entities[0],n);assert(!copies[0].count);Undo(&g_entities[0],qtrue);ForceTool(0,n,1,qfalse);assert(e->inuse);
  static jarWorld_t capture;jarAutoCapture_t automatic={0};assert(!WorldCapture(&capture));assert(WorldCaptureMode(&capture,&automatic)&&capture.copy.count==1&&capture.copy.node[0].frozen&&automatic.parked==1);assert(actors[n].active&&v->m_pPilot==(bgEntity_t*)&g_entities[1]);
  Command(2,"jar_release","");assert(!actors[n].active&&e->inuse);ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE);level.time+=1000;
  ActorArm(0,n);Command(1,"jar_allowTools","0");assert(!actors[n].active);ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE);Command(1,"jar_allowTools","1");ActorArm(0,n);assert(actors[n].active);
  usercmd_t command={0};command.serverTime=level.time;command.buttons=BUTTON_USE|BUTTON_ATTACK;command.forwardmove=127;
  assert(!JAR_MP_ActorThink(&g_entities[2],&command)&&!actors[n].active);assert(command.buttons==(BUTTON_USE|BUTTON_ATTACK)&&command.forwardmove==127);level.time+=1000;
  ActorArm(0,n);command.buttons=0;command.upmove=127;JAR_MP_ActorThink(&g_entities[1],&command);assert(!actors[n].active);level.time+=1000;
  // Occupied forces are additive; native throttle and commands are preserved.
  e->client->ps.speed=350;VectorSet(e->client->ps.velocity,350,0,0);VectorClear(e->client->ps.jarVehicleDrift);
  v->m_ucmd.forwardmove=127;v->m_ucmd.rightmove=40;usercmd_t driving=v->m_ucmd;
  ForceTool(0,n,2,qfalse);assert(objects[n].thruster&&JointEndpoint(n)&&ForceMass(n));
  ForceFrame(.05f);assert(e->client->ps.velocity[2]<0&&e->client->ps.jarVehicleDrift[2]==0&&e->client->ps.speed==350&&!memcmp(&driving,&v->m_ucmd,sizeof(driving))&&!JAR_MP_VehicleForces(n));
  ForceTool(0,n,4,qfalse);assert(players[0].first==n);ForceTool(0,ENTITYNUM_WORLD,4,qfalse);int rope=Last(0);assert(g_entities[rope].s.generic1==251);
  v->m_iBoarding=-1;vec3_t entering;VectorCopy(e->client->ps.velocity,entering);ForceFrame(.05f);assert(objects[n].thruster&&g_entities[rope].inuse&&VectorCompare(entering,e->client->ps.velocity));v->m_iBoarding=0;
  Command(2,"jar_release","");assert(!objects[n].thruster&&!g_entities[rope].inuse&&VectorLength(e->client->ps.jarVehicleDrift)==0);level.time+=1000;
  // Both joint kinds project vehicle and native rider positions, retaining speed.
  for(int joint=1;joint<=2;++joint){
   int link=JointMake(n,ENTITYNUM_WORLD,joint);vec3_t displaced;VectorCopy(e->r.currentOrigin,displaced);displaced[1]+=30;Move(e,displaced);VectorSet(e->client->ps.velocity,350,0,0);VectorClear(e->client->ps.jarVehicleDrift);
   imports.Trace=Trace;float before=JointError(link);level.time+=25;JointFrame(.025f,qfalse);
   assert(JointError(link)<before&&e->client->ps.speed==350&&!memcmp(&driving,&v->m_ucmd,sizeof(driving)));
   assert(VectorLength(e->client->ps.jarVehicleDrift)>0&&VectorCompare(g_entities[2].r.currentOrigin,e->r.currentOrigin));
   v->m_iBoarding=-1;VectorCopy(e->r.currentOrigin,entering);JointFrame(.025f,qfalse);ForceFrame(.025f);assert(JointLink(link)&&VectorCompare(entering,e->r.currentOrigin));v->m_iBoarding=0;
   Command(1,"jar_allowTools","0");assert(!JointLink(link));Command(1,"jar_allowTools","1");
  }
  // Server-owned attachments cannot bypass any passenger's opt-out.
  objects[n].owner=MAX_CLIENTS+1;objects[n].thruster=1;objects[n].thrustBy=MAX_CLIENTS+1;players[2].noTools=1;ForceFrame(.025f);assert(!objects[n].thruster);players[2].noTools=0;objects[n].owner=1;
  ForceTool(0,n,2,qfalse);ForceFrame(.05f);ForceImpulseAt(n,(float[3]){1,0,0},100,ForceMass(n),NULL);assert(VectorLength(e->client->ps.jarVehicleDrift)>0);playerTools.integer=0;ActorFrame();ForceFrame(.025f);assert(VectorLength(e->client->ps.jarVehicleDrift)==0&&!objects[n].thruster);playerTools.integer=1;
  ActorArm(0,n);v->m_iBoarding=-1;ActorTick();assert(!actors[n].active);v->m_iBoarding=0;
  ActorArm(0,n);clients[2].ps.m_iVehicleNum=0;ActorTick();assert(!actors[n].active);clients[2].ps.m_iVehicleNum=n;
  ActorArm(0,n);JAR_MP_ClientReset(2);assert(!actors[n].active&&e->inuse);
  ActorArm(0,n);sandbox.integer=0;ActorTick();assert(!actors[n].active);sandbox.integer=1;
  // Disconnect cleanup retains the occupied assembly without retaining a grip.
  ActorArm(0,n);JAR_MP_Disconnect(0);assert(e->inuse&&!actors[n].active&&objects[n].owner==MAX_CLIENTS+1);
  worldEditing.integer=1;ActorArm(0,n);assert(actors[n].active);clients[2].pers.connected=CON_DISCONNECTED;ActorTick();assert(!actors[n].active);
 }
 gentity_t*e=SeatVehicle(VH_SPEEDER);int n=e->s.number;Vehicle_t*v=e->m_pVehicle;
 const bgEntity_t*pilot=v->m_pPilot;v->m_pPilot=(bgEntity_t*)((uintptr_t)g_entities+1);assert(!VehicleAvailable(e));v->m_pPilot=(bgEntity_t*)pilot;
 v->m_iNumPassengers=VEH_MAX_PASSENGERS+1;assert(!VehicleAvailable(e));v->m_iNumPassengers=1;
 v->m_ppPassengers[1]=v->m_ppPassengers[0];assert(!VehicleAvailable(e));v->m_ppPassengers[1]=NULL;
 e->client->ps.m_iVehicleNum=1;assert(!VehicleAvailable(e));e->client->ps.m_iVehicleNum=2;
 // Restored/server-owned frozen vehicles still require all riders' consent.
 objects[n].owner=MAX_CLIENTS+1;objects[n].frozen=1;BeginActorAs(MAX_CLIENTS+1,e);players[2].noTools=1;assert(!JAR_MP_ActorThink(e,NULL)&&!actors[n].active);
 players[2].noTools=0;objects[n].frozen=1;BeginActorAs(MAX_CLIENTS+1,e);assert(JAR_MP_ActorThink(e,NULL));EndActor(n,qfalse);
 // Optional attached droid position follows its native bolt without a second event conversion.
 e=SeatVehicle(VH_FIGHTER);n=e->s.number;v=e->m_pVehicle;gentity_t*d=ActorNPC(-1,0);d->client->ps.m_iVehicleNum=n;v->m_pDroidUnit=(bgEntity_t*)d;v->m_iDroidUnitTag=5;
 ActorArm(0,n);assert(actors[n].active&&fabsf(d->r.currentOrigin[0]-e->r.currentOrigin[0]-13)<.001f&&d->health==100);
 // The client adapter holds authoritative angles instead of letting Pmove's
 // pre-freeze view update turn the vehicle toward the pilot's mouse input.
 pmove_t move={0};move.ps=e->playerState;move.cmd.serverTime=level.time+50;move.cmd.angles[YAW]=ANGLE2SHORT(170);vec3_t angle,position;
 VectorCopy(move.ps->viewangles,angle);VectorCopy(move.ps->origin,position);assert(JAR_FrozenVehiclePrediction(&move,(bgEntity_t*)e,(bgEntity_t*)&g_entities[1]));
 assert(move.ps->commandTime==level.time+50&&VectorCompare(move.ps->viewangles,angle)&&VectorCompare(move.ps->origin,position));
 move.cmd.serverTime=level.time-50;JAR_FrozenVehiclePrediction(&move,(bgEntity_t*)e,(bgEntity_t*)&g_entities[1]);assert(move.ps->commandTime==level.time+50);
 move.ps->pm_type=PM_NORMAL;assert(!JAR_FrozenVehiclePrediction(&move,(bgEntity_t*)e,(bgEntity_t*)&g_entities[1]));
 // NPC occupants retain their own owner/map permissions.
 e=SeatVehicle(VH_SPEEDER);n=e->s.number;v=e->m_pVehicle;gentity_t*r=ActorNPC(1,0);r->client->ps.m_iVehicleNum=n;v->m_ppPassengers[0]=(bgEntity_t*)r;
 assert(ActorAlive(e)&&!ActorEdit(&g_entities[0],n));sharedEditing.integer=1;assert(ActorEdit(&g_entities[0],n));sharedEditing.integer=0;objects[r->s.number].owner=0;
 mapEditing.integer=0;assert(!ActorEdit(&g_entities[0],n));mapEditing.integer=1;assert(ActorEdit(&g_entities[0],n));
 VehicleGripClearanceCases();
 puts("MP occupied vehicles: four hull families, native pilot bolt, passenger/droid positions, consent/revocation, exit input, freeze/rotation/fling/context, client prediction, occupied save/cleanup and invalid seats passed.");
}
