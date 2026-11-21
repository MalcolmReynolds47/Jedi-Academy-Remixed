// Native crystal callbacks and ownership with deterministic combat and traces.
static int crystalHitPulses,crystalBlasts,crystalCounter,crystalChainTarget,crystalRadiusDepth;
static float crystalHitDamage,crystalHitRadius,crystalBlastDamage,crystalBlastRadius;
static vec3_t crystalHitOrigin,crystalBlastOrigin;

static void CrystalFixtureRadius(const vec3_t origin,gentity_t*source,float damage,float radius){
 assert(++crystalRadiusDepth==1);
 if(source->takedamage){
  ++crystalHitPulses;crystalHitDamage=damage;crystalHitRadius=radius;
  VectorCopy(origin,crystalHitOrigin);
 }else{
  ++crystalBlasts;crystalBlastDamage=damage;crystalBlastRadius=radius;
  VectorCopy(origin,crystalBlastOrigin);
  if(crystalCounter!=ENTITYNUM_NONE)assert(g_entities[crystalCounter].count==1);
 }
 if(crystalChainTarget!=ENTITYNUM_NONE){
  gentity_t*target=&g_entities[crystalChainTarget];
  crystalChainTarget=ENTITYNUM_NONE;
  assert(!JAR_MP_BreakableRadiusDenied(source,target));
  BreakableFixtureDamage(target,source,1);
 }
 --crystalRadiusDepth;
}

static gentity_t* CrystalFixturePart(const gentity_t*root){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t*e=&g_entities[n];
  if(e->inuse&&e->s.eType==ET_GENERAL&&e->s.generic1==JAR_MP_CRYSTAL_CRATE_PART_TAG&&
     e->s.otherEntityNum==root->s.number)return e;
 }
 return NULL;
}

static void CrystalFixturePose(const gentity_t*root){
 const gentity_t*part=CrystalFixturePart(root);
 assert(part&&part->s.modelindex==FIXTURE_CRYSTAL_PART&&part->r.linked&&!part->r.contents);
 assert(part->s.time2==root->s.time2&&!objects[part->s.number].owner&&!part->takedamage);
 vec3_t up,expected;
 AngleVectors(root->r.currentAngles,NULL,NULL,up);
 VectorMA(root->r.currentOrigin,JAR_MP_CRYSTAL_CRATE_PART_HEIGHT,up,expected);
 assert(Distance(expected,part->r.currentOrigin)<.01f);
 assert(VectorCompare(root->r.currentAngles,part->r.currentAngles));
 assert((part->s.eFlags&EF_TELEPORT_BIT)==(root->s.eFlags&EF_TELEPORT_BIT));
}

static void CrystalCrateCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;
 crystalHitPulses=crystalBlasts=crystalRadiusDepth=0;
 crystalCounter=crystalChainTarget=ENTITYNUM_NONE;
 gentity_t*owner=&g_entities[0],*observer=&g_entities[1];
 gentity_t*root=SpawnWithController(owner,JAR_MP_CRYSTAL_CRATE_ID,(vec3_t){200,0,100},vec3_origin);
 assert(root&&JAR_MP_CrystalCrateRootState(&root->s));
 const int n=root->s.number;
 assert(ControllerKind(n)==JAR_CRYSTAL_CRATE_CONTROLLER&&PhysicsSolid(n)&&root->takedamage);
 assert(root->health==80&&objects[n].mass==40&&root->material==MAT_CRATE2);
 assert(root->r.contents==JAR_MP_CRYSTAL_CRATE_SOLID_CONTENTS&&!objects[n].output[0]);
 assert(VectorCompare(root->r.mins,(vec3_t){-17,-11,0})&&VectorCompare(root->r.maxs,(vec3_t){17,11,38}));
 assert(!(ContextCaps(owner,n)&JAR_CONTEXT_POSE));
 CrystalFixturePose(root);
 assert(ContextTransformApply(n,(vec3_t){240,20,130},(vec3_t){25,61,-30}));
 CrystalFixturePose(root);
 vec3_t up,point;AngleVectors(root->r.currentAngles,NULL,NULL,up);
 VectorMA(root->r.currentOrigin,38,up,point);
 BreakableFixtureDamage(root,observer,13);
 assert(root->health==67&&crystalHitPulses==1&&crystalHitDamage==16&&crystalHitRadius==32);
 assert(Distance(point,crystalHitOrigin)<.01f);
 StockGasEffectAt(FIXTURE_WELDER_EFFECT,point,(vec3_t){0,0,1});
 BreakableFixtureDamage(root,observer,1);assert(root->health==66&&crystalHitPulses==1);

 ControllerSelect(root);
 Command(0,"jar_property","controller_crystal_hit_damage_21");
 Command(0,"jar_property","controller_crystal_hit_radius_900125");
 Command(0,"jar_property","controller_crystal_blast_radius_125");
 Command(0,"jar_property","controller_damage_35");
 Command(0,"jar_property","controller_crystal_hit_radius_131072001");
 Command(0,"jar_property","controller_crystal_solid");
 assert(root->r.contents==CONTENTS_CORPSE&&!PhysicsSolid(n)&&PhysicsMoveMask(root)==MASK_SOLID);
 assert(JAR_MP_CrystalCrateShotOnly(&root->s));
 assert((root->r.contents&MASK_SHOT)&&!(root->r.contents&MASK_PLAYERSOLID));
 blocked=1;Command(0,"jar_property","controller_crystal_solid");assert(root->r.contents==CONTENTS_CORPSE);blocked=0;
 const vec3_t from={160,20,146};
 trace_t aim={0};aim.entityNum=ENTITYNUM_WORLD;aim.fraction=1;VectorSet(aim.endpos,300,20,146);
 NonsolidToolAim(observer,from,&aim);assert(aim.entityNum==ENTITYNUM_WORLD);
 NonsolidToolAim(owner,from,&aim);assert(aim.entityNum==n);
 jarControllerState_t state;ControllerState(n,&state);
 assert(state.count==66&&state.initial==80&&state.flags==JAR_MP_CRYSTAL_CRATE_SHOT_ONLY);
 assert(state.repeats==21&&state.pushSpeed==900125&&state.damage==35&&state.speed==125);
 Command(0,"jar_property","controller_enabled");assert(JAR_MP_BreakableDamageBlocked(root));
 Command(0,"jar_property","activate");assert(root->inuse);
 Command(0,"jar_property","property_undo");assert(!JAR_MP_BreakableDamageBlocked(root));
 Command(0,"jar_property","controller_count_92");assert(root->health==92);
 Command(0,"jar_property","property_undo");assert(root->health==66);

 BreakableFixtureDamage(root,observer,1);assert(root->health==65&&root->painDebounceTime>level.time);
 CopyAssembly(owner,n);
 assert(copies[0].count==1&&!copies[0].node[0].stationUseRemaining);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){520,45,150},NULL,NULL));
 gentity_t*copy=&g_entities[players[0].selected];
 assert(copy!=root&&copy->health==65&&copy->r.contents==CONTENTS_CORPSE&&!copy->painDebounceTime);
 CrystalFixturePose(copy);Undo(owner,qfalse);assert(!copy->inuse&&root->inuse);

 gentity_t*counter=ControllerAt("target_counter",0,800);
 ControllerLink(owner,n,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 assert(objects[n].output[0]==counter->s.number+1&&counter->count==2);
 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==2&&saved.copy.node[0].controller.count==65&&saved.copy.node[0].output[0]==2);
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 decoded.copy.node[0].stationUseRemaining=1;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 root=StockExplosiveFind(JAR_CRYSTAL_CRATE_CONTROLLER);
 assert(root&&root->health==65&&root->r.contents==CONTENTS_CORPSE&&!root->painDebounceTime);
 CrystalFixturePose(root);
 worldEditing.integer=1;
 counter=StockExplosiveFind(4);assert(counter);
 crystalCounter=counter->s.number;
 assert(objects[root->s.number].output[0]==crystalCounter+1&&counter->count==2);
 ControllerSelect(root);
 vec3_t blast;
 AngleVectors(root->r.currentAngles,NULL,NULL,up);
 VectorMA(root->r.currentOrigin,19,up,blast);
 gentity_t*part=CrystalFixturePart(root);
 Command(0,"jar_property","activate");
 assert(!root->inuse&&!part->inuse&&counter->count==1&&crystalBlasts==1);
 assert(crystalBlastDamage==35&&crystalBlastRadius==125&&Distance(crystalBlastOrigin,blast)<.01f);
 StockGasEffectAt(FIXTURE_CRYSTAL_BLAST,blast,(vec3_t){0,0,1});
 StockGasEffectAt(FIXTURE_CRYSTAL_DEBRIS,blast,(vec3_t){0,0,1});
 assert(WorldClear());

 crystalCounter=ENTITYNUM_NONE;
 root=SpawnAt(owner,JAR_MP_CRYSTAL_CRATE_ID,(vec3_t){100,0,80},vec3_origin);
 copy=SpawnAt(owner,JAR_MP_CRYSTAL_CRATE_ID,(vec3_t){150,0,80},vec3_origin);
 assert(root&&copy);copy->health=1;
 crystalChainTarget=copy->s.number;
 BreakableFixtureDamage(root,observer,1);
 assert(root->inuse&&!copy->inuse&&crystalBlasts==2&&crystalChainTarget==ENTITYNUM_NONE);
 assert(WorldClear());
 PhysicsReset();
 puts("MP crystal crate: stock child and rotated hull, damage debounce, shot-only selection, Context, duplicate, JRW18, Primary and bounded chain passed.");
}
