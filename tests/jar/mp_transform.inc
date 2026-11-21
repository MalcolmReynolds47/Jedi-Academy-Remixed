// Exact placement through the real server command, transaction and world codec.
static void TransformCases(void){
 imports.FS_Open=Open;imports.FS_Read=Read;imports.FS_Close=Close;imports.Trace=Trace;imports.LinkEntity=Link;imports.SendServerCommand=ContextSend;blockAt=0;WorldFixture();
 gentity_t*e=CopySpawn(1,0,100);int n=e->s.number;aimed=n;Command(0,"jar_select","");
 assert(ContextCaps(&g_entities[0],n)&JAR_CONTEXT_TRANSFORM);vec3_t old;VectorCopy(e->r.currentOrigin,old);
 Command(0,"jar_property","mass_120");jarPropertyUndo_t undo=players[0].propertyUndo;
 for(const char**s=(const char*[]){"transform_","transform_1_2_3_4_5","transform_nan_2_3_4_5_6","transform_1_2_3_4_inf_6","transform_65536_2_3_4_5_6","transform_1_2_3_4_0x1p2_6","transform_1_2_3_4_5_6_extra",NULL};*s;++s){
  Command(0,"jar_property",*s);assert(VectorCompare(old,e->r.currentOrigin)&&!memcmp(&undo,&players[0].propertyUndo,sizeof(undo)));
 }
 blocked=1;Command(0,"jar_property","transform_-125.25_50_200_0_-3690_7200");blocked=0;
 assert(VectorCompare(old,e->r.currentOrigin)&&e->r.linked&&!memcmp(&undo,&players[0].propertyUndo,sizeof(undo)));
 VectorSet(objects[n].velocity,10,20,30);VectorSet(objects[n].angular,.1f,.2f,.3f);int flags=e->s.eFlags;
 Command(0,"jar_property","transform_-125.25_50_200_0_-3690_7200");
 assert(e->r.currentOrigin[0]==-125.25f&&e->r.currentOrigin[1]==50&&e->r.currentOrigin[2]==200&&e->r.currentAngles[1]==270&&e->r.currentAngles[2]==0);
 assert(e->s.eFlags==(flags^EF_TELEPORT_BIT)&&VectorCompare(e->s.pos.trBase,e->r.currentOrigin)&&objects[n].velocity[2]==30&&objects[n].angular[2]==.3f&&!objects[n].frozen);
 assert(ContextUndoReady(&g_entities[0],n));undo=players[0].propertyUndo;
 blocked=1;Command(0,"jar_property","property_undo");blocked=0;assert(e->r.currentOrigin[0]==-125.25f&&!memcmp(&undo,&players[0].propertyUndo,sizeof(undo)));
 Command(0,"jar_property","property_undo");assert(VectorCompare(old,e->r.currentOrigin)&&VectorCompare(e->r.currentAngles,vec3_origin)&&!players[0].propertyUndo.field);
 // A competing edit, movement, pose change or Physgun cycle expires Undo.
 Command(0,"jar_property","transform_400_100_200_0_90_0");sharedEditing.integer=1;players[1].selected=n;Command(1,"jar_property","transform_500_100_200_0_90_0");Command(1,"jar_property","transform_400_100_200_0_90_0");
 Command(0,"jar_property","property_undo");assert(e->r.currentOrigin[0]==400);sharedEditing.integer=0;
 Command(0,"jar_property","transform_401_100_200_0_90_0");e->r.currentOrigin[0]+=1;assert(!ContextUndoReady(&g_entities[0],n));
 Command(0,"jar_property","transform_403_100_200_0_90_0");++objects[n].poseRevision;assert(!ContextUndoReady(&g_entities[0],n));
 Command(0,"jar_property","transform_404_100_200_0_90_0");ActorArm(0,n);assert(!(ContextCaps(&g_entities[0],n)&JAR_CONTEXT_TRANSFORM));
 Command(0,"jar_property","transform_999_100_200_0_90_0");assert(e->r.currentOrigin[0]==404);Drop(0,qfalse);assert(!ContextUndoReady(&g_entities[0],n));
 Command(0,"jar_property","freeze");Command(0,"jar_property","transform_405_100_200_0_90_0");Command(0,"jar_property","release");Command(0,"jar_property","freeze");assert(!ContextUndoReady(&g_entities[0],n));
 Command(0,"jar_property","transform_406_100_200_0_90_0");for(int i=0;i<24;++i)Command(0,"jar_property","rotate");assert(!ContextUndoReady(&g_entities[0],n));
 // Serialization and duplication preserve the new pose with existing JRW6.
 Command(0,"jar_property","transform_407_101_201_0_90_0");static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world));int size=WorldEncode(&world,bytes);assert(WorldDecode(bytes,size,&decoded));
 assert(decoded.copy.count==1&&decoded.copy.node[0].offset[0]==407&&decoded.copy.node[0].angles[1]==90);
 CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].angles[1]==90);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));e=&g_entities[MAX_CLIENTS];assert(e->r.currentOrigin[0]==407&&e->r.currentAngles[1]==90);
 // Live NPCs must be frozen. Players keep tool consent and self release.
 WorldFixture();e=CopySpawn(2,0,100);CopyReady(e);n=e->s.number;aimed=n;Command(0,"jar_select","");assert(!(ContextCaps(&g_entities[0],n)&JAR_CONTEXT_TRANSFORM));
 Command(0,"jar_property","transform_700_0_200_0_45_0");assert(e->r.currentOrigin[0]==100);Command(0,"jar_property","freeze");
 Command(0,"jar_property","transform_700_0_200_0_45_0");assert(e->r.currentOrigin[0]==700&&e->client->ps.origin[0]==700&&e->client->ps.viewangles[1]==45&&e->health==100&&JAR_MP_ActorControlled(e));
 Command(0,"jar_property","property_undo");assert(e->r.currentOrigin[0]==100);Command(0,"jar_property","release");assert(!JAR_MP_ActorControlled(e));
 aimed=1;Command(0,"jar_select","");Command(0,"jar_property","freeze");Command(0,"jar_property","transform_800_0_200_0_90_0");assert(clients[1].ps.origin[0]==800&&clients[1].ps.pm_type==PM_FREEZE);
 Command(1,"jar_allowTools","0");Command(0,"jar_property","transform_900_0_200_0_90_0");assert(clients[1].ps.origin[0]==800&&!JAR_MP_ActorControlled(&g_entities[1]));
 // Occupied vehicle placement validates the body, driver and passenger before
 // changing any native snapshot; failure restores original link flags and Undo.
 imports.G2API_AddBolt=SeatBolt;imports.G2API_GetBoltMatrix=SeatMatrix;
 const vehicleType_t kinds[]={VH_SPEEDER,VH_WALKER,VH_FIGHTER,VH_ANIMAL};
 for(int kind=0;kind<4;++kind){
  e=SeatVehicle(kinds[kind]);n=e->s.number;aimed=n;Command(0,"jar_select","");Command(0,"jar_property","freeze");
  vec3_t original[3];VectorCopy(e->r.currentOrigin,original[0]);VectorCopy(g_entities[1].r.currentOrigin,original[1]);VectorCopy(g_entities[2].r.currentOrigin,original[2]);
  for(int obstacle=1;obstacle<=3;++obstacle){traceCalls=0;blockAt=obstacle;Command(0,"jar_property","transform_500_200_300_0_90_0");blockAt=0;
   assert(VectorCompare(original[0],e->r.currentOrigin)&&VectorCompare(original[1],g_entities[1].r.currentOrigin)&&VectorCompare(original[2],g_entities[2].r.currentOrigin));
   assert(e->r.linked&&g_entities[1].r.linked&&g_entities[2].r.linked&&!players[0].propertyUndo.field);
  }
  e->m_pVehicle->m_iBoarding=-1;Command(0,"jar_property","transform_500_200_300_0_90_0");assert(VectorCompare(original[0],e->r.currentOrigin));e->m_pVehicle->m_iBoarding=0;Command(0,"jar_select","");
  Command(0,"jar_property","transform_500_200_300_0_90_0");assert(e->r.currentOrigin[0]==500&&fabsf(clients[1].ps.origin[1]-208)<.001f&&clients[1].ps.origin[2]==324&&VectorCompare(clients[2].ps.origin,e->r.currentOrigin));
  for(int c=1;c<=2;++c)assert(clients[c].ps.m_iVehicleNum==n&&clients[c].ps.legsAnim==21&&clients[c].ps.torsoAnim==22&&g_entities[c].health==100);
  Command(0,"jar_property","property_undo");assert(VectorCompare(original[0],e->r.currentOrigin));
  players[2].noTools=1;Command(0,"jar_property","transform_800_0_200_0_90_0");assert(VectorCompare(original[0],e->r.currentOrigin));
 }
 // Droid helpers use their native bolt, also checked before the vehicle moves.
 e=SeatVehicle(VH_FIGHTER);n=e->s.number;gentity_t*droid=CopySpawn(2,0,700);CopyReady(droid);droid->client->ps.m_iVehicleNum=n;
 e->m_pVehicle->m_pDroidUnit=(bgEntity_t*)droid;e->m_pVehicle->m_iDroidUnitTag=2;aimed=n;Command(0,"jar_select","");Command(0,"jar_property","freeze");VectorCopy(e->r.currentOrigin,old);
 trap->UnlinkEntity((sharedEntity_t*)droid);traceCalls=0;blockAt=4;Command(0,"jar_property","transform_600_200_300_15_90_20");blockAt=0;
 assert(VectorCompare(e->r.currentOrigin,old)&&!droid->r.linked&&e->r.linked);trap->LinkEntity((sharedEntity_t*)droid);
 Command(0,"jar_property","transform_600_200_300_15_90_20");assert(e->r.currentOrigin[0]==600&&e->r.currentAngles[0]==15&&e->r.currentAngles[2]==20);
 assert(fabsf(droid->r.currentOrigin[1]-210)<.001f&&droid->r.currentOrigin[2]==324&&fabsf(clients[1].ps.origin[1]-208)<.001f);
 // The real rigid solver adopts the exact placement instead of snapping back.
 RigidFixture();e=CopySpawn(1,0,100);n=e->s.number;aimed=n;Command(0,"jar_select","");Command(0,"jar_property","freeze");RigidSteps(1);
 Command(0,"jar_property","transform_700_100_300_0_90_0");RigidSteps(3);assert(Distance(e->r.currentOrigin,(float[3]){700,100,300})<.001f&&fabsf(AngleSubtract(e->r.currentAngles[1],90))<.001f);
 Command(0,"jar_property","property_undo");RigidSteps(1);assert(Distance(e->r.currentOrigin,(float[3]){100,0,80})<.001f);PhysicsReset();
 puts("MP transform: staged command bounds, atomic collision rejection, motion retention, conflict/pose/control Undo, JRW6/clipboard, frozen actors, consent, occupied seats and real solver placement passed.");
}
