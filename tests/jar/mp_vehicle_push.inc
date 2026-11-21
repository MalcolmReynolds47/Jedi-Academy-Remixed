// Native target callback, owned controller activation and actual world codec.
// Driving integration is additionally exercised by mp_vehicle_drift.c.
static gentity_t*PushVehicleFixture(vehicleType_t type){
 RigidFixture();SeatVehicle(type);imports.Trace=Trace;blockAt=0;sharedEditing.integer=1;
 gentity_t*push=TransportSpawn("target_push",900);assert(push&&TransportDestination(push));return push;
}
static void VehiclePushCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int family=0;family<4;++family){
  gentity_t*push=PushVehicleFixture(types[family]),*e=(gentity_t*)copyVehicles[MAX_CLIENTS].m_pParentEntity;
  assert(e&&e->m_pVehicle);Vehicle_t*v=e->m_pVehicle;Vehicle_t vehicleBefore=*v;
  playerState_t*ps=&e->client->ps;ps->speed=700;VectorSet(ps->velocity,750,100,-50);VectorSet(ps->jarVehicleDrift,50,100,0);ps->groundEntityNum=ENTITYNUM_WORLD;
  v->m_ucmd.forwardmove=127;v->m_ucmd.rightmove=45;v->m_ucmd.buttons=BUTTON_ATTACK;vehicleBefore=*v;
  static gclient_t riders[2];riders[0]=clients[1];riders[1]=clients[2];int snapshots=snapshotCalls,events=teleportEvents;
  vec3_t origin,angles;VectorCopy(e->r.currentOrigin,origin);VectorCopy(e->r.currentAngles,angles);
  aimed=push->s.number;Command(1,"jar_select","");assert(players[1].selected==push->s.number);Command(1,"jar_property","activate");
  assert(fabsf(ps->velocity[0]-480)<.001f&&ps->velocity[1]==0&&fabsf(ps->velocity[2]-360)<.001f);
  assert(ps->jarVehicleDrift[0]==ps->velocity[0]&&ps->jarVehicleDrift[1]==0&&ps->jarVehicleDrift[2]==0&&ps->speed==0&&ps->groundEntityNum==ENTITYNUM_NONE);
  assert(VectorCompare(e->s.pos.trDelta,ps->velocity)&&e->s.groundEntityNum==ENTITYNUM_NONE);
  assert(!memcmp(&vehicleBefore,v,sizeof(vehicleBefore))&&!memcmp(&riders[0],&clients[1],sizeof(gclient_t))&&!memcmp(&riders[1],&clients[2],sizeof(gclient_t)));
  assert(VectorCompare(e->r.currentOrigin,origin)&&VectorCompare(e->r.currentAngles,angles)&&snapshotCalls==snapshots&&teleportEvents==events);
  assert(e->health==100&&e->fly_sound_debounce_time==level.time+1500);
  // Repeated activation replaces travel velocity instead of stacking a fling.
  Command(2,"jar_select","");Command(2,"jar_property","activate");assert(fabsf(ps->velocity[0]-480)<.001f&&ps->speed==0);
  // Arc mode keeps the stock source-to-destination ballistic vector.
  push->spawnflags=0;gentity_t*dest=TransportDestination(push);VectorCopy(push->r.currentOrigin,dest->r.currentOrigin);dest->r.currentOrigin[0]+=128;dest->r.currentOrigin[2]+=96;
  Command(1,"jar_property","activate");assert(fabsf(ps->velocity[0]-128/sqrtf(.24f))<.001f&&fabsf(ps->velocity[2]-sqrtf(153600))<.001f);
  // Full controller speed is retained, including pure vertical launches.
  objects[push->s.number].output[0]=0;push->speed=4000;VectorSet(push->r.currentAngles,0,90,0);
  Command(1,"jar_property","activate");assert(fabsf(ps->velocity[1]-4000)<.001f&&fabsf(ps->jarVehicleDrift[1]-4000)<.001f);
  VectorSet(push->r.currentAngles,-90,0,0);Command(1,"jar_property","activate");assert(fabsf(ps->velocity[2]-4000)<.001f&&fabsf(ps->jarVehicleDrift[2])==0);
  // Force tools must not abruptly truncate an existing controller impulse.
  VectorSet(push->r.currentAngles,0,90,0);Command(1,"jar_property","activate");vec3_t same;VectorCopy(ps->velocity,same);JAR_VehicleDriftVelocity(ps,same);assert(fabsf(ps->velocity[1]-4000)<.001f);
  playerTools.integer=0;ActorFrame();assert(VectorLength(ps->jarVehicleDrift)==0&&VectorLength(ps->velocity)<.01f);playerTools.integer=1;
  // Empty-vehicle motion round-trips at the new range, while old files retain
  // their old validation and clipboard copies still start without motion.
  v->m_pPilot=v->m_pOldPilot=v->m_ppPassengers[0]=NULL;v->m_iNumPassengers=0;ps->m_iVehicleNum=0;clients[1].ps.m_iVehicleNum=clients[2].ps.m_iVehicleNum=0;
  Use_target_push(push,push,e);assert(fabsf(ps->jarVehicleDrift[1]-4000)<.001f);
  static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES],legacy[WORLD_BYTES];assert(WorldCapture(&world));int node=-1;
  for(int i=0;i<world.copy.count;++i)if(world.copy.node[i].vehicleMotion)node=i;assert(node>=0&&world.copy.node[node].vehicleDrift[1]==4000);
  int size=WorldEncode(&world,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
  memcpy(legacy,bytes,WORLD_HEADER);at=0;WorldPut(legacy,&at,0x4157524au);at=WORLD_HEADER;
  for(int i=0;i<world.copy.count;++i){memcpy(legacy+at,bytes+WORLD_HEADER+i*WORLD_NODE,268);at+=268;}
  for(int r=0;r<world.copy.ropes;++r){memcpy(legacy+at,bytes+WORLD_HEADER+world.copy.count*WORLD_NODE+r*WORLD_ROPE,72);at+=72;}at+=4;
  WorldTestHash(legacy,at);assert(!WorldDecode(legacy,at,&decoded));int header=0;WorldPut(legacy,&header,0x4257524au);WorldTestHash(legacy,at);assert(WorldDecode(legacy,at,&decoded)&&decoded.copy.node[node].vehicleDrift[1]==4000);
  assert(WorldClear()&&PlaceCopy(NULL,&world.copy,vec3_origin,world.velocity,world.angular));e=MotionLoaded();CopyReady(e);
  assert(e->client->ps.speed==0&&e->client->ps.jarVehicleDrift[1]==4000&&e->client->ps.velocity[1]==4000);assert(WorldClear());
 }
 for(int refusal=0;refusal<17;++refusal){
  gentity_t*push=PushVehicleFixture(VH_SPEEDER),*e=(gentity_t*)copyVehicles[MAX_CLIENTS].m_pParentEntity,*dest=TransportDestination(push);Vehicle_t*v=e->m_pVehicle;
  switch(refusal){
   case 0:sandbox.integer=0;break;case 1:players[1].noTools=1;break;case 2:players[2].noTools=1;break;
   case 3:playerTools.integer=0;break;case 4:players[2].noGrabUntil=level.time+1000;break;
   case 5:sharedEditing.integer=0;objects[e->s.number].owner=2;break;
   case 6:v->m_iBoarding=-1;break;case 7:clients[2].ps.m_iVehicleNum=0;break;case 8:clients[2].pers.connected=CON_DISCONNECTED;break;
   case 9:actors[e->s.number].active=qtrue;break;case 10:e->client->ps.pm_type=PM_FREEZE;break;
   case 11:dest->flags|=FL_INACTIVE;break;case 12:push->spawnflags=0;dest->r.currentOrigin[2]=push->r.currentOrigin[2]-1;break;
   case 13:push->spawnflags=0;g_gravity.value=0;break;case 14:push->speed=10000;break;
   case 15:dest->r.currentOrigin[0]=NAN;break;case 16:push->flags|=FL_INACTIVE;break;
  }
  gentity_t before=*e;gclient_t clientBefore=*e->client;Vehicle_t vehicleBefore=*v;int snapshots=snapshotCalls;
  ControllerInvoke(push,&g_entities[1],&g_entities[1]);
  assert(!memcmp(&before,e,sizeof(before))&&!memcmp(&clientBefore,e->client,sizeof(clientBefore))&&!memcmp(&vehicleBefore,v,sizeof(vehicleBefore))&&snapshotCalls==snapshots);
 }
 // Delayed activation rechecks seat consent at the time the impulse is applied.
 gentity_t*push=PushVehicleFixture(VH_FIGHTER),*e=(gentity_t*)copyVehicles[MAX_CLIENTS].m_pParentEntity;
 gentity_t*delay=ControllerAt("target_delay",0,1800);ControllerLink(&g_entities[0],delay->s.number,qfalse);ControllerLink(&g_entities[0],push->s.number,qfalse);
 aimed=delay->s.number;Command(1,"jar_select","");Command(1,"jar_property","activate");players[2].noTools=1;ControllerTick(delay,1000);assert(!VectorLength(e->client->ps.jarVehicleDrift));
 players[2].noTools=0;Command(1,"jar_property","activate");ControllerTick(delay,1000);assert(fabsf(e->client->ps.jarVehicleDrift[0]-480)<.001f);
 PhysicsReset();puts("MP vehicle Push: four families, seated C/Delay activation, native callback, full-speed and arc impulses, preserved seats/commands/events, 17 refusal cases, consent revocation and exact JRWB motion restore passed.");
}
