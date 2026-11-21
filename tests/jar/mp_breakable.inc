// Production callbacks, effects, persistence and physics. Engine weapon damage
// and radius falloff are imports; native server checks cover scheduled teardown.
static void BreakableCases(void){
 const char*names[]={"func_breakable","misc_model_breakable","func_glass"};
 for(int kind=0;kind<3;++kind){
  RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0],*e=ControllerAt(names[kind],0,100),*counter=ControllerAt("target_counter",0,700);int n=e->s.number;
  assert(ControllerKind(n)==30+kind&&e->health==40&&e->takedamage&&e->use==BreakUse&&e->die==BreakDie);
  objects[n].output[0]=objects[n].output[1]=counter->s.number+1;
  e->health=30;e->pain(e,p,10);assert(counter->count==1);e->pain(e,p,1);assert(counter->count==1);
  ControllerSelect(e);Command(0,"jar_property","controller_count_100");assert(e->health==100&&ContextUndoReady(p,n));
  Command(0,"jar_property","controller_delay_500");assert(e->delay==500);
  Command(0,"jar_property","controller_playeruse");assert(e->r.svFlags&SVF_PLAYER_USABLE);
  Command(0,"jar_property","controller_damagepolicy");assert(!e->takedamage);Command(0,"jar_property","controller_damagepolicy");assert(e->takedamage&&(e->flags&FL_DMG_BY_SABER_ONLY));Command(0,"jar_property","controller_damagepolicy");assert(e->flags&FL_DMG_BY_HEAVY_WEAP_ONLY);Command(0,"jar_property","controller_damagepolicy");assert(!(e->flags&(FL_DMG_BY_SABER_ONLY|FL_DMG_BY_HEAVY_WEAP_ONLY)));
  Command(0,"jar_property","controller_damage_120");Command(0,"jar_property","controller_radius_300");Command(0,"jar_property","controller_chunks_400");
  jarControllerState_t before,after;ControllerState(n,&before);const char*bad[]={"controller_count_0","controller_count_2147483648","controller_damage_32768","controller_radius_131073","controller_chunks_-1","controller_delay_-1","controller_random_-1"};
  for(int i=0;i<7;++i)Command(0,"jar_property",bad[i]);ControllerState(n,&after);assert(!memcmp(&before,&after,sizeof(before)));
  players[0].noTools=1;GlobalUse(e,p,p);assert(e->genericValue11==1);players[0].noTools=0;
  Command(0,"jar_property","controller_enabled");assert(JAR_MP_BreakableDamageBlocked(e));GlobalUse(e,p,p);assert(e->genericValue11==1);Command(0,"jar_property","controller_enabled");assert(!JAR_MP_BreakableDamageBlocked(e));
  playerTools.integer=0;assert(JAR_MP_BreakableRadiusDenied(e,p));playerTools.integer=1;assert(!JAR_MP_BreakableRadiusDenied(e,p));players[0].noTools=1;assert(JAR_MP_BreakableRadiusDenied(e,p));players[0].noTools=0;
  sharedEditing.integer=0;gentity_t*other=CopySpawn(1,1,1400);assert(JAR_MP_BreakableRadiusDenied(e,other));sharedEditing.integer=1;assert(!JAR_MP_BreakableRadiusDenied(e,other));G_FreeEntity(other);
  RigidSteps(1);assert(JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));ActorArm(0,n);
  int calls=breakRadiusCalls;GlobalUse(e,p,p);assert(e->genericValue11==2&&!e->health&&!e->takedamage&&!BodyDormant(n)&&JAR_MP_BreakableDamageBlocked(e));int deadline=e->nextthink;GlobalUse(e,p,p);assert(e->nextthink==deadline);
  StationTick(e,500);assert(!e->genericValue11&&BodyDormant(n)&&!e->r.contents&&!e->s.solid&&!objects[n].heldBy&&counter->count==0&&breakRadiusCalls==calls+1);
  assert(!JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));int debris=0,fx=0;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse){gentity_t*t=&g_entities[i];if(t->s.eType==ET_EVENTS+EV_DEBRIS){++debris;assert(t->freeAfterEvent&&t->s.eventParm>=72&&t->s.eventParm<=100&&t->s.owner==i&&t->s.trickedentindex==e->material);}if(t->s.eType==ET_EVENTS+EV_MISC_MODEL_EXP)++fx;}assert(debris==1&&fx==1);
  vec3_t at;VectorCopy(e->r.currentOrigin,at);objects[n].frozen=0;objects[n].thruster=1;objects[n].thrustBy=1;RigidSteps(4);assert(Distance(at,e->r.currentOrigin)<.001f);rigidEnabled.integer=0;ActorTick();assert(Distance(at,e->r.currentOrigin)<.001f);
  ControllerSelect(e);blocked=1;Command(0,"jar_property","controller_repair");assert(BodyDormant(n));blocked=0;Command(0,"jar_property","controller_repair");assert(e->health==100&&!BodyDormant(n));Command(0,"jar_property","property_undo");assert(BodyDormant(n));Command(0,"jar_property","controller_repair");
  // At zero delay, destruction is queued so splash chains cannot recurse.
  Command(0,"jar_property","controller_delay_0");GlobalUse(e,p,p);assert(e->genericValue11==2&&e->nextthink==level.time+1);breakAllocationFail=1;StationTick(e,1);breakAllocationFail=0;assert(BodyDormant(n));
  Command(0,"jar_property","controller_repair");Command(0,"jar_property","controller_usemode");GlobalUse(e,p,p);assert(e->genericValue11==1);
  // Intact, broken and pending destruction survive typed world round trips.
  for(int state=0;state<3;++state){
   jarControllerState_t s={0};s.initial=100;s.count=state==1?75:0;s.active=state;s.waitMs=1000;s.randomMs=500;s.pushSpeed=100;s.repeats=MAT_GLASS;s.inactive=state==2;assert(BreakApply(n,&s));if(state==2)e->nextthink=level.time+321;e->painDebounceTime=level.time+123;
   static jarWorld_t world,decoded;static byte data[WORLD_BYTES];assert(WorldCapture(&world));int length=WorldEncode(&world,data);assert(WorldDecode(data,length,&decoded));int idx=-1;for(int i=0;i<world.copy.count;++i)if(world.copy.node[i].id==jarMPCatalog[objects[n].catalog-1].id)idx=i;assert(idx>=0);jarCopyNode_t*node=&decoded.copy.node[idx];assert(node->controller.active==state&&node->stationUseRemaining==123&&(state!=2||node->controllerRemaining==321));
   node->controllerRemaining=state==2?0:1;assert(!CopyControllersValid(&decoded.copy));assert(WorldDecode(data,length,&decoded));assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
   for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==30+kind){e=&g_entities[i];n=i;}assert(e->genericValue11==state&&BodyDormant(n)==(state==0)&&e->painDebounceTime-level.time==123&&ControllerActivator(e)==e);objects[n].owner=1;ControllerSelect(e);
   if(state==2){assert(e->nextthink-level.time==211);StationTick(e,211);assert(e->genericValue11==2);Command(0,"jar_property","controller_enabled");StationTick(e,FRAMETIME);assert(BodyDormant(n));}
  }
  assert(WorldClear());
 }
 PhysicsReset();puts("MP breakables: health callbacks, policy, bounded effects, nonrecursive destruction, linked actions, repair/Undo, physics dormancy, typed settings and JRW17 persistence passed.");
}
