// Actual capture/codec/journal/native-readiness paths; no human driving claim.
static gentity_t* MotionLoaded(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner==MAX_CLIENTS+1&&g_entities[n].m_pVehicle)return &g_entities[n];
 assert(0);return NULL;
}
static void VehicleMotionCases(void){
 imports.FS_Open=WorldTestOpen;imports.FS_Read=WorldTestRead;imports.FS_Write=WorldTestWrite;imports.FS_Close=WorldTestClose;imports.Print=WorldTestPrint;imports.Cvar_VariableStringBuffer=WorldTestCvar;imports.Argc=WorldTestArgc;
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 const vec3_t velocity={137.25f,-44.5f,63.75f},drift={23.75f,-18.25f,0};
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],old[WORLD_BYTES],bad[WORLD_BYTES];
 for(int family=0;family<4;++family){
  memset(worldFiles,0,sizeof(worldFiles));worldFileCount=0;WorldFixture();copyVehicleInfo.type=types[family];
  gentity_t*e=CopySpawn(3,0,200);CopyReady(e);int n=e->s.number;
  VectorCopy(velocity,e->client->ps.velocity);VectorCopy(drift,e->client->ps.jarVehicleDrift);e->client->ps.speed=111.5f;
  CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&!copies[0].node[0].vehicleMotion&&VectorLengthSquared(copies[0].node[0].vehicleDrift)==0);
  assert(WorldCapture(&saved)&&saved.copy.node[0].vehicleMotion&&saved.copy.node[0].vehicleSpeed==111.5f&&VectorCompare(saved.copy.node[0].vehicleDrift,drift));
  int size=WorldEncode(&saved,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
  assert(VectorCompare(decoded.velocity[0],velocity)&&VectorCompare(decoded.copy.node[0].vehicleDrift,drift)&&decoded.copy.node[0].vehicleSpeed==111.5f);
  for(int cycle=0;cycle<3;++cycle){
   if(!WorldSave("motion")){fprintf(stderr,"motion save: %s\n",worldMessage);assert(0);} assert(WorldClear());if(!WorldLoad("motion")){fprintf(stderr,"motion load: %s\n",worldMessage);assert(0);}e=MotionLoaded();n=e->s.number;assert(objects[n].copyPending&&objects[n].copyVehicleMotion&&!VectorLengthSquared(e->client->ps.jarVehicleDrift));
   CopyReady(e);assert(!objects[n].copyPending&&VectorCompare(e->client->ps.velocity,velocity)&&VectorCompare(e->client->ps.jarVehicleDrift,drift)&&e->client->ps.speed==111.5f);
   vec3_t base;VectorSubtract(e->client->ps.velocity,e->client->ps.jarVehicleDrift,base);assert(base[0]==113.5f&&base[1]==-26.25f&&base[2]==63.75f);
  }
  // Named map-start restore passes the same state through deferred NPC setup.
  assert(WorldClear());WorldReset();Q_strncpyz(worldLoad.string,"motion",sizeof(worldLoad.string));WorldFrame();e=MotionLoaded();CopyReady(e);assert(VectorCompare(e->client->ps.jarVehicleDrift,drift)&&e->client->ps.speed==111.5f);worldLoad.string[0]=0;assert(WorldClear());
  // Every older revision keeps its original velocity/derived-speed semantics.
  const int strides[]={88,100,132,136,136,156,180,260,264,268,268};
  for(int version=1;version<=11;++version){
   memcpy(old,bytes,WORLD_HEADER);at=0;WorldPut(old,&at,version==11?0x4257524au:version==10?0x4157524au:0x3057524au+(unsigned)version*0x1000000u);memcpy(old+WORLD_HEADER,bytes+WORLD_HEADER,strides[version-1]);int oldSize=WORLD_HEADER+strides[version-1]+4;WorldTestHash(old,oldSize);
   assert(WorldDecode(old,oldSize,&decoded));if(version>=6)assert(decoded.copy.node[0].vehicleMotion&&VectorCompare(decoded.copy.node[0].vehicleDrift,drift));else assert(!decoded.copy.node[0].vehicleMotion&&!VectorLengthSquared(decoded.copy.node[0].vehicleDrift));
   assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));e=MotionLoaded();CopyReady(e);assert(VectorCompare(e->client->ps.velocity,velocity));if(version>=6)assert(VectorCompare(e->client->ps.jarVehicleDrift,drift)&&e->client->ps.speed==111.5f);else assert(VectorLengthSquared(e->client->ps.jarVehicleDrift)==0&&e->client->ps.speed==velocity[0]);assert(WorldClear());
  }
  // Even a recomputed checksum cannot admit NaN/oversize/vertical drift,
  // unknown flags, nonvehicle payloads, or moving frozen-vehicle records.
  const int offsets[]={136,136,140,140,148,152,152,4,0};
  const unsigned values[]={2,0,0x7fc00000u,0x461c4000u,0x3f800000u,0x7f800000u,0x48800000u,1,CopyId(1)};
  for(int k=0;k<9;++k){memcpy(bad,bytes,size);at=WORLD_HEADER+offsets[k];WorldPut(bad,&at,values[k]);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);}
  assert(!WorldDecode(bytes,size-1,&decoded)&&!WorldDecode(bytes,size+1,&decoded));
  // Copy/paste remains stationary; world motion never leaks into a clipboard.
  assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){300,0,100},NULL,NULL));e=&g_entities[players[0].selected];CopyReady(e);assert(VectorLengthSquared(e->client->ps.velocity)==0&&VectorLengthSquared(e->client->ps.jarVehicleDrift)==0&&e->client->ps.speed==0);assert(WorldClear());
  e=SeatVehicle(types[family]);VectorCopy(velocity,e->client->ps.velocity);VectorCopy(drift,e->client->ps.jarVehicleDrift);e->client->ps.speed=111.5f;
  jarAutoCapture_t automatic={0};assert(!WorldCapture(&saved));assert(WorldCaptureMode(&saved,&automatic)&&automatic.parked==1&&saved.copy.node[0].frozen);
  assert(saved.copy.node[0].vehicleMotion&&VectorLengthSquared(saved.copy.node[0].vehicleDrift)==0&&saved.copy.node[0].vehicleSpeed==0&&VectorLengthSquared(saved.velocity[0])==0);
  assert(VectorCompare(e->client->ps.jarVehicleDrift,drift)&&VectorCompare(e->client->ps.velocity,velocity)&&e->client->ps.speed==111.5f&&!WorldClear());
 }
 puts("MP JRW6 vehicle motion: four families, exact repeated journals, deferred readiness, map restore, JRW1-5 compatibility, malformed records, stationary copies and occupied snapshot isolation passed.");
}
