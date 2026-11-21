// Controlled collision and visibility services for the actual trigger loop.
cgs_t cgs;
static snapshot_t teleportSnap;
static int teleportTraces, teleportPVSChecks, teleportBlockAt, teleportHideAt;
static qboolean teleportVisible, teleportBlocked;
static qboolean TeleportPVS(const vec3_t from,const vec3_t to,byte*mask){
 assert(mask==teleportSnap.areamask&&from==teleportSnap.ps.origin);
 return ++teleportPVSChecks!=teleportHideAt&&teleportVisible;
}
static qboolean CG_JAR_TeleportArrivalClear(const vec3_t origin,const vec3_t mins,const vec3_t maxs,int vehicle,int pilot){
 const entityState_t *portal=&volume.currentState;
 assert(vehicle==90&&pilot==cg.predictedPlayerState.clientNum);
 if(++teleportTraces==1){
  assert(VectorCompare(origin,portal->origin)&&VectorCompare(mins,portal->pos.trDelta)&&VectorCompare(maxs,portal->apos.trDelta));
 }else{
  assert(VectorCompare(origin,portal->angles)&&VectorCompare(mins,portal->boneAngles1)&&VectorCompare(maxs,portal->boneAngles2));
 }
 return teleportTraces!=teleportBlockAt;
}
void BG_EvaluateTrajectory(const trajectory_t *tr,int atTime,vec3_t result){
 assert(tr->trType==TR_STATIONARY);
 VectorCopy(tr->trBase,result);
}
static void TeleportRiderTrace(trace_t *trace,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,
                              clipHandle_t model,int mask,const vec3_t origin,const vec3_t angles,int capsule){
 assert(start==cg.predictedPlayerState.origin&&VectorCompare(start,end)&&mins==cg_pmove.mins&&maxs==cg_pmove.maxs);
 assert(model==1&&mask==-1&&!capsule&&origin[0]==500&&angles[YAW]==90);
 memset(trace,0,sizeof(*trace));trace->startsolid=contacts;
}
static void TeleportTrace(trace_t*t,const vec3_t a,const vec3_t mins,const vec3_t maxs,const vec3_t b,int pass,int mask){
 assert(VectorCompare(a,b)&&mins==cg_pmove.mins&&maxs==cg_pmove.maxs&&pass==cg.predictedPlayerState.clientNum&&mask==MASK_PLAYERSOLID);
 memset(t,0,sizeof(*t));t->startsolid=teleportBlocked;++teleportTraces;
}
static entityState_t*TeleportReset(void){
 Reset();cg.snap=&teleportSnap;imports.R_InPVS=TeleportPVS;cg_pmove.trace=TeleportTrace;memset(&cg_pmove.cmd,0,sizeof(cg_pmove.cmd));
 jarTeleportPredicted=jarVehicleTeleportPredicted=qfalse;
 teleportTraces=teleportPVSChecks=teleportBlockAt=teleportHideAt=0;teleportVisible=qtrue;teleportBlocked=qfalse;volume.currentValid=qtrue;
 entityState_t*s=&volume.currentState;s->eType=ET_TELEPORT_TRIGGER;s->generic1=JAR_MP_TELEPORT_VOLUME;s->otherEntityNum=90;s->time=1000;s->speed=12.5f;
 VectorSet(s->origin2,400.25f,50,32);VectorSet(s->angles2,0,90,0);return s;
}
static entityState_t *VehicleTeleportReset(vehicleType_t type,int client){
 TeleportReset();VehicleReset(type);cg.snap=&teleportSnap;
 cgs.jarMPVersion=JAR_MP_VEHICLE_TELEPORT_FEATURE;cgs.numInlineModels=10;
 imports.CM_TransformedTrace=TeleportRiderTrace;
 playerState_t *pilot=&cg.predictedPlayerState,*hull=&cg.predictedVehicleState;
 pilot->clientNum=driven.currentState.owner=client;
 hull->m_iVehicleNum=driven.currentState.m_iVehicleNum=client+1;
 pilot->groundEntityNum=42;pilot->origin[0]=500;hull->origin[0]=100;
 VectorSet(hull->jarVehicleDrift,2,3,4);VectorSet(pilot->velocity,5,6,7);
 teleportSnap.ps=*pilot;teleportSnap.vps=*hull;
 volume.currentValid=qtrue;entityState_t *s=&volume.currentState;
 s->eType=ET_TELEPORT_TRIGGER;s->generic1=JAR_MP_TELEPORT_VOLUME;s->time=1000;
 s->otherEntityNum=91;s->otherEntityNum2=90;
 s->trickedentindex3=client<16?1u<<client:0;s->trickedentindex4=client>=16?1u<<(client-16):0;
 VectorSet(s->origin2,400,50,32);VectorSet(s->angles2,0,90,0);
 VectorSet(s->origin,400,50,78);VectorSet(s->angles,405,52,102);
 VectorSet(s->pos.trDelta,-28,-76,-42);VectorSet(s->apos.trDelta,28,76,42);
 VectorSet(s->boneAngles1,-15,-15,-24);VectorSet(s->boneAngles2,15,15,32);
 drivenMove.cmd.angles[YAW]=500;
 return s;
}

static void VehicleTeleportPredictionCases(void){
 const vehicleType_t families[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 playerState_t *pilot=&cg.predictedPlayerState,*hull=&cg.predictedVehicleState;
 for(int family=0;family<ARRAY_LEN(families);++family){
  entityState_t *s=VehicleTeleportReset(families[family],family==3?31:0);
  const playerState_t oldHull=*hull,oldPilot=*pilot;
  const usercmd_t command=drivenMove.cmd;
  assert(!CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
  CG_TouchTriggerPrediction();assert(!jarTeleportPredicted&&!teleportTraces);
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(jarTeleportPredicted&&jarVehicleTeleportPredicted&&teleportTraces==2&&teleportPVSChecks==2);
  assert(VectorCompare(hull->origin,s->origin)&&VectorCompare(pilot->origin,s->angles));
  assert(fabsf(hull->velocity[YAW]-400)<.01f&&hull->speed==400&&!VectorLength(hull->jarVehicleDrift));
  assert(!VectorLength(pilot->velocity)&&VectorCompare(hull->vehOrientation,s->angles2));
  assert(hull->groundEntityNum==ENTITYNUM_NONE&&pilot->groundEntityNum==ENTITYNUM_NONE);
  assert(pilot->delta_angles[YAW]==15884&&hull->delta_angles[YAW]==15884);
  assert(hull->eventSequence==oldHull.eventSequence&&pilot->eventSequence==oldPilot.eventSequence);
  assert(!memcmp(&drivenMove.cmd,&command,sizeof(command))&&CG_JAR_VehiclePortalDiscontinuity(&oldHull,qfalse));

  const playerState_t warpedHull=*hull,warpedPilot=*pilot;
  *hull=oldHull;*pilot=oldPilot;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(teleportTraces==2&&!memcmp(hull,&oldHull,sizeof(*hull))&&!memcmp(pilot,&oldPilot,sizeof(*pilot)));
  jarTeleportPredicted=jarVehicleTeleportPredicted=qfalse;teleportTraces=0;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(!memcmp(hull,&warpedHull,sizeof(*hull))&&!memcmp(pilot,&warpedPilot,sizeof(*pilot)));
  assert(!CG_JAR_VehiclePortalDiscontinuity(&warpedHull,qtrue));
  *hull=oldHull;*pilot=oldPilot;jarTeleportPredicted=jarVehicleTeleportPredicted=qfalse;
  s->trickedentindex3=s->trickedentindex4=0;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(CG_JAR_VehiclePortalDiscontinuity(&warpedHull,qtrue)&&!jarVehicleTeleportPredicted);
 }

 for(int refusal=0;refusal<18;++refusal){
  entityState_t *s=VehicleTeleportReset(VH_SPEEDER,0);
  switch(refusal){
   case 0: --cgs.jarMPVersion;break;
   case 1: driven.currentState.m_iVehicleNum=0;break;
   case 2: teleportSnap.vps.m_iVehicleNum=0;break;
   case 3: s->otherEntityNum2=92;break;
   case 4: s->trickedentindex3=0;s->trickedentindex=1;break;
   case 5: hull->commandTime=s->time+101;break;
   case 6: pilot->commandTime=s->time-1;break;
   case 7: drivenMove.cmd.buttons=BUTTON_USE;break;
   case 8: cg_pmove.cmd.buttons=BUTTON_USE_HOLDABLE;break;
   case 9: drivenVehicle.m_iTurboTime=1001;break;
   case 10: cg.hyperspace=qtrue;break;
   case 11: volume.currentValid=qfalse;break;
   case 12: hull->origin[0]=110;break; // Long hull reaches outside native trigger query.
   case 13: s->boneAngles2[2]=NAN;break;
   case 14: VectorClear(s->angles);break; // Pilot would arrive back in the same trigger.
   case 15: native=volume;native.currentState.number=101;cg_triggerEntities[1]=&native;cg_numTriggerEntities=2;break;
   case 16: driven.currentState.owner=2;break;
   case 17: teleportSnap.ps.m_iVehicleNum=0;break;
  }
  const playerState_t oldHull=*hull,oldPilot=*pilot;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(!jarVehicleTeleportPredicted&&!teleportTraces&&!teleportPVSChecks);
  assert(!memcmp(hull,&oldHull,sizeof(*hull))&&!memcmp(pilot,&oldPilot,sizeof(*pilot)));
 }

 for(int endpoint=1;endpoint<=2;++endpoint){
  VehicleTeleportReset(VH_FIGHTER,0);teleportHideAt=endpoint;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(!jarVehicleTeleportPredicted&&!teleportTraces&&teleportPVSChecks==endpoint);
  VehicleTeleportReset(VH_FIGHTER,0);teleportBlockAt=endpoint;
  CG_JAR_PredictVehicleTeleportVolumes(&drivenMove,&driven);
  assert(!jarVehicleTeleportPredicted&&teleportTraces==endpoint&&teleportPVSChecks==2);
 }

 entityState_t *s=VehicleTeleportReset(VH_SPEEDER,0);
 pilot->origin[0]=0;assert(CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 pilot->origin[0]=500;
 native=volume;native.currentState.number=101;VectorCopy(pilot->origin,native.currentState.pos.trBase);
 cg_triggerEntities[1]=&native;cg_numTriggerEntities=2;
 assert(CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 native.currentState.eType=ET_PUSH_TRIGGER;native.currentState.generic1=JAR_MP_PUSH_VOLUME;
 native.currentState.speed=800;native.currentState.frame=4;native.currentState.angles2[0]=800;
 assert(CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 native.currentState.generic1=0;native.currentState.solid=SOLID_BMODEL;native.currentState.modelindex=9;
 native.currentState.pos.trType=native.currentState.apos.trType=TR_STATIONARY;native.currentState.apos.trBase[YAW]=90;
 contacts=0;assert(!CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 contacts=1;assert(CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 s->trickedentindex3=0;assert(!CG_JAR_VehicleTeleportRiderBlocked(pilot,cg_pmove.mins,cg_pmove.maxs));
 contacts=0;
 puts("MP vehicle teleport prediction: four native families, exact paired arrival, native rider ordering, independent grants, replay rollback, stale/ambiguous refusal, and blocked/invisible exits passed.");
}

static void TeleportPredictionCases(void){
 entityState_t*s=TeleportReset();playerState_t*p=&cg.predictedPlayerState,baseline=*p;
 p->groundEntityNum=42;cg_pmove.cmd.angles[YAW]=500;
 CG_TouchTriggerPrediction();assert(jarTeleportPredicted&&teleportTraces==1&&p->origin[0]==400.25f&&p->origin[2]==73.5f);
 assert(p->eFlags&EF_TELEPORT_BIT);assert(p->groundEntityNum==ENTITYNUM_NONE&&p->pm_time==160&&(p->pm_flags&PMF_TIME_KNOCKBACK));
 assert(fabsf(p->velocity[YAW]-400)<.01f&&p->delta_angles[YAW]==15884&&p->viewangles[YAW]==90&&CG_JAR_PortalDiscontinuity(&baseline,qfalse));
 playerState_t previous=*p;*p=baseline;CG_TouchTriggerPrediction();assert(!VectorLength(p->origin)&&teleportTraces==1);
 jarTeleportPredicted=qfalse;CG_TouchTriggerPrediction();assert(VectorCompare(p->origin,previous.origin)&&!CG_JAR_PortalDiscontinuity(&previous,qtrue));
 *p=baseline;jarTeleportPredicted=qfalse;s->trickedentindex=0;CG_TouchTriggerPrediction();assert(!VectorLength(p->origin)&&CG_JAR_PortalDiscontinuity(&previous,qtrue));
 s=TeleportReset();teleportBlocked=qtrue;CG_TouchTriggerPrediction();assert(!VectorLength(p->origin)&&teleportTraces==1);
 teleportBlocked=qfalse;teleportVisible=qfalse;CG_TouchTriggerPrediction();assert(!VectorLength(p->origin)&&teleportTraces==1);
 s=TeleportReset();p->commandTime=s->time+101;CG_TouchTriggerPrediction();assert(!teleportTraces);
 p->commandTime=s->time-1;CG_TouchTriggerPrediction();assert(!teleportTraces);
 p->commandTime=s->time;p->m_iVehicleNum=125;CG_TouchTriggerPrediction();assert(!teleportTraces);
 p->m_iVehicleNum=0;p->pm_type=PM_SPECTATOR;CG_TouchTriggerPrediction();assert(!teleportTraces);
 p->pm_type=PM_NORMAL;cg_pmove.cmd.buttons=BUTTON_USE;CG_TouchTriggerPrediction();assert(!teleportTraces);
 cg_pmove.cmd.buttons=0;p->duelInProgress=qtrue;CG_TouchTriggerPrediction();assert(!teleportTraces);p->duelInProgress=qfalse;
 s->angles2[YAW]=NAN;CG_TouchTriggerPrediction();assert(!teleportTraces);s->angles2[YAW]=90;
 s->apos.trBase[YAW]=45;VectorSet(p->origin,95,95,0);CG_TouchTriggerPrediction();assert(!teleportTraces);VectorClear(p->origin);
 VectorSet(s->origin2,0,0,-20);CG_TouchTriggerPrediction();assert(!teleportTraces);
 s=TeleportReset();native=volume;native.currentState.number=101;cg_triggerEntities[1]=&native;cg_numTriggerEntities=2;CG_TouchTriggerPrediction();assert(!teleportTraces);
 cg_numTriggerEntities=1;p->clientNum=31;s->trickedentindex=0;s->trickedentindex2=1u<<15;CG_TouchTriggerPrediction();assert(jarTeleportPredicted&&teleportTraces==1);
 puts("MP teleport prediction: actual trigger loop, arrival motion, command replay, stale/denied grants, consent bit 31, rotated contact, blocked/invisible exit and ambiguous overlap passed.");
 VehicleTeleportPredictionCases();
}
