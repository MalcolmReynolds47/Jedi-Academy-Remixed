// Native hull policy and production control paths, with engine boundary adapters.
static void VehicleCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int kind=0;kind<4;++kind){
  ActorFixture();memset(&copyVehicleInfo,0,sizeof(copyVehicleInfo));copyVehicleInfo.type=types[kind];copyVehicleInfo.length=160;copyVehicleInfo.width=60;copyVehicleInfo.height=80;
  gentity_t*e=CopySpawn(3,0,200);assert(e);CopyReady(e);int n=e->s.number;Vehicle_t*v=e->m_pVehicle;playerState_t*ps=&e->client->ps;
  ps->legsAnim=17;ps->torsoAnim=18;ps->speed=400;v->m_iTurboTime=level.time+500;v->m_vAngularVelocity=40;VectorSet(v->m_vFullAngleVelocity,1,2,3);v->m_ucmd.buttons=BUTTON_ATTACK;v->m_ucmd.forwardmove=127;
  ActorArm(1,n);assert(players[1].held==ENTITYNUM_NONE);
  VectorAdd(e->r.currentOrigin,((vec3_t){-16,3,10}),physgunFixtureHit);
  VectorSet(clients[0].ps.origin,physgunFixtureHit[0]-200,physgunFixtureHit[1],physgunFixtureHit[2]-24);
  imports.Trace=PhysgunFixtureTrace;ActorArm(0,n);imports.Trace=Trace;
  assert(players[0].held==n&&ps->pm_type==PM_FREEZE);
  assert(ps->legsAnim==17&&ps->torsoAnim==18&&ps->forceHandExtend==HANDEXTEND_NONE&&ps->speed==0&&v->m_iTurboTime<=level.time&&v->m_vAngularVelocity==0&&VectorLength(v->m_vFullAngleVelocity)==0&&v->m_ucmd.buttons==0);
  ActorTurnInput(&g_entities[0],30,-30);ActorTurnInput(&g_entities[0],30,0);ActorTurnInput(&g_entities[0],30,0);
  for(int i=0;i<10;++i)ActorTick();
  assert(fabsf(e->r.currentAngles[YAW]-90)<.01f&&fabsf(e->r.currentAngles[PITCH]+30)<.01f&&VectorCompare(e->r.currentAngles,ps->vehOrientation)&&VectorCompare(e->r.currentAngles,v->m_vPrevOrientation));
  assert(e->s.eType==ET_NPC&&e->health==100);if(types[kind]==VH_FIGHTER)assert(e->r.maxs[1]>e->r.maxs[0]);
  // Collision rejects the rotation itself, preserving both hull and orientation.
  vec3_t angle,lo,hi;VectorCopy(e->r.currentAngles,angle);VectorCopy(e->r.mins,lo);VectorCopy(e->r.maxs,hi);angle[YAW]+=35;blocked=1;
  assert(!ActorRotate(e,angle)&&VectorCompare(lo,e->r.mins)&&VectorCompare(hi,e->r.maxs)&&fabsf(e->r.currentAngles[YAW]-90)<.01f);blocked=0;
  VectorSet(objects[n].velocity,120,35,40);Drop(0,qfalse);assert(!actors[n].active&&ps->pm_type==PM_NORMAL&&ps->velocity[0]==120&&ps->velocity[2]==40);
  vec3_t forward;AngleVectors(e->r.currentAngles,forward,NULL,NULL);assert(fabsf(ps->speed-DotProduct(ps->velocity,forward))<.01f);
  ActorArm(0,n);JAR_MP_Fire(&g_entities[0],qtrue);assert(objects[n].frozen&&actors[n].active);for(int i=0;i<20;++i)ActorTick();assert(ps->legsAnim==17&&ps->forceHandExtend==HANDEXTEND_NONE);
  aimed=n;Command(0,"jar_select","");Command(0,"jar_property","rotate");assert(fabsf(e->r.currentAngles[YAW]-105)<.01f&&ps->vehOrientation[YAW]==e->r.currentAngles[YAW]);
  Command(0,"jar_property","release");assert(!actors[n].active);players[0].tool=0;Tool(&g_entities[0],qfalse);assert(actors[n].active);Tool(&g_entities[0],qtrue);assert(!actors[n].active);
  // Boarding or inconsistent seat links release immediately on the next native/
  // frame check and refuse grab/removal/Undo/cleanup during the transition.
  ActorArm(0,n);v->m_iBoarding=-1;assert(!JAR_MP_ActorThink(e,NULL)&&!actors[n].active&&players[0].held==ENTITYNUM_NONE);
  ActorArm(0,n);assert(players[0].held==ENTITYNUM_NONE);players[0].tool=1;Tool(&g_entities[0],qfalse);Undo(&g_entities[0],qtrue);assert(e->inuse);v->m_iBoarding=0;
  v->m_ppPassengers[0]=(bgEntity_t*)&g_entities[1];assert(CopyOccupied(e)&&!ActorAlive(e));Tool(&g_entities[0],qfalse);assert(e->inuse);v->m_ppPassengers[0]=NULL;
  v->m_pOldPilot=(bgEntity_t*)&g_entities[1];assert(CopyOccupied(e)&&!ActorAlive(e));v->m_pOldPilot=NULL;
  v->m_pPilot=(bgEntity_t*)&g_entities[1];assert(!ActorAlive(e));v->m_pPilot=NULL;
  ActorArm(0,n);v->m_iDieTime=level.time+1000;ActorTick();assert(!actors[n].active&&e->inuse);v->m_iDieTime=0;
  // Frozen copies apply native hulls/orientation/armor at actual readiness.
  Command(0,"jar_select","");Command(0,"jar_property","freeze");e->health=230;CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].frozen);
  PasteAssembly(&g_entities[0]);int pasted=players[0].selected;assert(pasted!=n);CopyReady(&g_entities[pasted]);assert(actors[pasted].active&&objects[pasted].frozen&&g_entities[pasted].m_pVehicle->m_iArmor==230);
  assert(VectorCompare(g_entities[pasted].r.currentAngles,g_entities[pasted].client->ps.vehOrientation));Undo(&g_entities[0],qfalse);assert(!g_entities[pasted].inuse);
  sharedEditing.integer=1;ActorArm(1,n);assert(players[1].held==n);sharedEditing.integer=0;ActorTick();assert(!actors[n].active);
  objects[n].owner=MAX_CLIENTS+1;assert(!ActorEdit(&g_entities[0],n));worldEditing.integer=1;ActorArm(0,n);assert(players[0].held==n);JAR_MP_ClientReset(0);assert(!actors[n].active);
  objects[n].owner=1;ActorArm(0,n);JAR_MP_Disconnect(0);assert(!e->inuse&&!actors[n].active&&players[0].held==ENTITYNUM_NONE);
 }
 memset(&copyVehicleInfo,0,sizeof(copyVehicleInfo));
 puts("MP vehicles: native hull families, hold/rotation/freeze/release, unchanged animations, velocity handoff, boarding and occupied cleanup, copied armor and ownership passed.");
}
