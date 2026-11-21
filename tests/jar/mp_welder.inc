// The native radius and event paths use fixture imports for deterministic time and collision.
static int welderTraceMode;
static void WelderFixtureTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,
                              const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mask!=MASK_SHOT||mins||pass<MAX_CLIENTS||ControllerKind(pass)!=JAR_WELDER_CONTROLLER)return;
 t->fraction=1;
 if(welderTraceMode==1){
  t->fraction=.5f;
  VectorSet(t->plane.normal,0,-1,0);
  for(int i=0;i<3;++i)t->endpos[i]=(start[i]+end[i])*.5f;
 }else if(welderTraceMode==2)t->startsolid=qtrue;
}

static gentity_t* WelderFixtureEffect(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t*e=&g_entities[n];
  if(e->inuse&&e->s.eType==ET_EVENTS+EV_PLAY_EFFECT_ID&&e->s.eventParm==FIXTURE_WELDER_EFFECT)return e;
 }
 return NULL;
}

static void WelderFixtureTick(int ms){
 level.previousTime=level.time;
 level.time+=ms;
 JAR_MP_Frame();
}

static void WelderCases(void){
 RigidFixture();
 imports.Trace=WelderFixtureTrace;
 welderTraceMode=welderRadiusCalls=welderRecycleSource=0;
 gentity_t*owner=&g_entities[0],*other=&g_entities[1];
 gentity_t*root=SpawnWithController(owner,JAR_MP_WELDER_ID,(vec3_t){200,0,100},vec3_origin);
 assert(root&&root->s.modelindex==FIXTURE_WELDER_MODEL&&root->s.loopSound==FIXTURE_WELDER_LOOP);
 const int n=root->s.number;
 assert(ControllerKind(n)==JAR_WELDER_CONTROLLER&&PhysicsProp(n)&&!PhysicsSolid(n));
 assert(root->r.contents==CONTENTS_ITEM&&root->r.mins[2]==-16&&root->r.maxs[2]==16);
 assert(!(ContextCaps(owner,n)&JAR_CONTEXT_POSE));
 const vec3_t eye={160,0,100};
 trace_t aim={0};
 aim.entityNum=ENTITYNUM_WORLD;aim.fraction=.75f;VectorSet(aim.endpos,240,0,100);
 NonsolidToolAim(other,eye,&aim);assert(aim.entityNum==ENTITYNUM_WORLD);
 NonsolidToolAim(owner,eye,&aim);assert(aim.entityNum==n);
 root->use(root,other,other);assert(root->s.loopSound==FIXTURE_WELDER_LOOP);

 WelderFrame(root);assert(!WelderFixtureEffect());
 WelderFixtureTick(1);
 gentity_t*effect=WelderFixtureEffect();
 assert(effect&&welderRadiusCalls==1&&welderDamage==10&&welderRadius==45);
 assert(Distance(welderRadiusOrigin,(vec3_t){215,-.7046813f,96.78138f})<.01f);
 assert(VectorCompare(effect->s.origin,welderRadiusOrigin)&&fabsf(effect->s.angles[YAW]-180)<.01f);
 G_FreeEntity(effect);
 playerTools.integer=0;
 assert(JAR_MP_BreakableRadiusDenied(root,other));
 playerTools.integer=1;players[1].noTools=0;
 assert(!JAR_MP_BreakableRadiusDenied(root,other));
 players[1].noTools=1;assert(JAR_MP_BreakableRadiusDenied(root,other));

 players[0].selected=n;
 Command(0,"jar_property","controller_welder_interval_10000");
 WelderFixtureTick(90);assert(welderRadiusCalls==2&&WelderRemaining(root)==10000);
 effect=WelderFixtureEffect();assert(effect);G_FreeEntity(effect);
 Command(0,"jar_property","controller_welder_interval_250");
 Command(0,"jar_property","controller_welder_reach_31750");
 Command(0,"jar_property","controller_welder_radius_25125");
 Command(0,"jar_property","controller_damage_22");
 Command(0,"jar_property","controller_welder_reach_131072001");
 jarControllerState_t state;
 ControllerState(n,&state);
 assert(state.waitMs==250&&state.speed==31750&&state.pushSpeed==25125&&state.damage==22);
 assert(WelderRemaining(root)==9450);

 CopyAssembly(owner,n);
 assert(copies[0].count==1&&!copies[0].node[0].controllerRemaining);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){500,50,130},NULL,NULL));
 gentity_t*copy=&g_entities[players[0].selected];
 assert(copy!=root&&!WelderRemaining(copy));
 WelderFixtureTick(1);assert(welderRadiusCalls==3&&WelderRemaining(copy)==250);
 effect=WelderFixtureEffect();assert(effect);G_FreeEntity(effect);
 Undo(owner,qfalse);assert(!copy->inuse&&root->inuse);

 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==1&&saved.copy.node[0].controllerRemaining==9449);
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 decoded.copy.node[0].controllerRemaining=-1;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 root=StockExplosiveFind(JAR_WELDER_CONTROLLER);
 assert(root&&WelderRemaining(root)==9449);
 WelderFixtureTick(9448);assert(welderRadiusCalls==3&&!WelderFixtureEffect());
 welderTraceMode=1;
 assert(PhysicsSetAngles(root,(vec3_t){0,90,0}));
 WelderFixtureTick(1);
 assert(welderRadiusCalls==4&&welderDamage==22&&fabsf(welderRadius-25.125f)<.001f);
 assert(Distance(welderRadiusOrigin,(vec3_t){200.35234f,15.375f,98.39069f})<.01f);
 effect=WelderFixtureEffect();assert(effect&&VectorCompare(effect->s.origin,welderRadiusOrigin));
 G_FreeEntity(effect);
 welderTraceMode=2;WelderFixtureTick(250);assert(welderRadiusCalls==4&&!WelderFixtureEffect());

 ControllerState(root->s.number,&state);state.inactive=1;
 assert(WelderApply(root->s.number,&state)&&!root->s.loopSound);
 welderTraceMode=0;WelderFixtureTick(2000);assert(welderRadiusCalls==4&&!WelderFixtureEffect());
 state.inactive=0;state.damage=0;
 assert(WelderApply(root->s.number,&state)&&root->s.loopSound==FIXTURE_WELDER_LOOP);
 WelderFixtureTick(1);assert(welderRadiusCalls==4);
 effect=WelderFixtureEffect();assert(effect);G_FreeEntity(effect);
 worldEditing.integer=1;
 root->use(root,owner,owner);assert(!root->s.loopSound&&WelderRemaining(root)==250);
 root->use(root,owner,owner);assert(root->s.loopSound==FIXTURE_WELDER_LOOP&&WelderRemaining(root)==250);
 state.damage=22;assert(WelderApply(root->s.number,&state));
 welderRecycleSource=1;WelderFixtureTick(250);
 assert(welderRadiusCalls==5&&root->health==73&&!root->s.loopSound&&!objects[root->s.number].owner);
 welderRecycleSource=0;G_FreeEntity(root);
 effect=WelderFixtureEffect();assert(effect);G_FreeEntity(effect);
 assert(WorldClear());
 PhysicsReset();
 puts("MP welder: stock sparks and loop, clipping, consent, Context bounds, duplicate, JRW18 deadline and recycled source passed.");
}
