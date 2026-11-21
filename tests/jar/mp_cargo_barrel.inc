// Production spawn, activation and damage callbacks. The radius import
// delivers one selected victim after running the production permission check.
static int cargoBlastCalls, cargoBlastDamage, cargoBlastRadius;
static int cargoBlastVictim=ENTITYNUM_NONE;
static qboolean cargoReplaceSource;

static void BreakableFixtureDamage(gentity_t *target,gentity_t *attacker,int damage){
 if(!target->takedamage||JAR_MP_BreakableDamageBlocked(target))return;
 target->health-=damage;
 if(target->health>0){if(target->pain)target->pain(target,attacker,damage);}
 else target->die(target,attacker,attacker,damage,MOD_UNKNOWN);
}

static void CargoBarrelRadius(const vec3_t origin,gentity_t *source,int damage,int radius){
 assert(source->inuse&&!source->takedamage&&VectorCompare(origin,source->r.currentOrigin));
 ++cargoBlastCalls;cargoBlastDamage=damage;cargoBlastRadius=radius;
 if(cargoBlastVictim!=ENTITYNUM_NONE){
  gentity_t *victim=&g_entities[cargoBlastVictim];
  if(!JAR_MP_BreakableRadiusDenied(source,victim))BreakableFixtureDamage(victim,source,damage);
 }
 if(cargoReplaceSource){
  const int slot=source->s.number;
  G_FreeEntity(source);
  gentity_t *replacement=G_Spawn();assert(replacement->s.number==slot);
  replacement->classname="cargo_blast_replacement";
 }
}

static int CargoBarrelEffects(const vec3_t origin,const vec3_t angles){
 int count=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i){
  const gentity_t *effect=&g_entities[i];
  if(!effect->inuse||effect->s.eType!=ET_EVENTS+EV_PLAY_EFFECT_ID)continue;
  assert(effect->freeAfterEvent&&effect->s.eventParm==1);
  assert(VectorCompare(origin,effect->s.pos.trBase)&&VectorCompare(origin,effect->s.origin));
  assert(VectorCompare(angles,effect->s.angles));
  ++count;
 }
 return count;
}

static void CargoBarrelSpawnAndDamage(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;
 gentity_t *owner=&g_entities[0],*other=&g_entities[1];
 const int stockModels=cargoModelRegistrations,stockEffects=cargoEffectRegistrations;
 assert(ControllerId("object_cargo_barrel1")==91859725u);
 gentity_t *barrel=ControllerAt("object_cargo_barrel1",0,100);
 gentity_t *counter=ControllerAt("target_counter",0,400);
 assert(barrel&&counter&&cargoModelRegistrations>stockModels&&cargoEffectRegistrations>stockEffects);
 const int slot=barrel->s.number;
 assert(ControllerKind(slot)==JAR_CARGO_BARREL_CONTROLLER&&objects[slot].owner==1);
 assert(barrel->takedamage&&barrel->health==40&&barrel->splashDamage==100&&barrel->splashRadius==200);
 assert(barrel->die&&barrel->use&&barrel->r.linked&&(barrel->r.contents&CONTENTS_SOLID));
 assert(barrel->r.mins[0]==-12&&barrel->r.maxs[0]==20&&barrel->r.mins[2]==-4&&barrel->r.maxs[2]==12);
 assert(PhysicsProp(slot));
 ControllerSelect(barrel);assert(ContextCaps(owner,slot)&JAR_CONTEXT_CONTROLLER);
 Command(0,"jar_property","controller_count_120");assert(barrel->health==120);
 Command(0,"jar_property","controller_damage_75");assert(barrel->splashDamage==75);
 Command(0,"jar_property","controller_radius_125");assert(barrel->splashRadius==125);
 Command(0,"jar_property","property_undo");assert(barrel->splashRadius==200);
 Command(0,"jar_property","controller_count_40");Command(0,"jar_property","controller_damage_100");

 ControllerLink(owner,slot,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 assert(objects[slot].output[0]==counter->s.number+1);
 players[0].linkOutput=1;
 ControllerLink(owner,slot,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 assert(!objects[slot].output[1]);players[0].linkOutput=0;
 ControllerShot(other,slot,qfalse);assert(barrel->inuse&&barrel->health==40);
 players[0].noTools=1;GlobalUse(barrel,owner,owner);assert(barrel->inuse);players[0].noTools=0;
 playerTools.integer=0;assert(JAR_MP_BreakableRadiusDenied(barrel,owner));
 GlobalUse(barrel,owner,owner);assert(barrel->inuse);playerTools.integer=1;
 assert(!JAR_MP_BreakableRadiusDenied(barrel,owner));
 players[1].noTools=1;assert(JAR_MP_BreakableRadiusDenied(barrel,other));players[1].noTools=0;
 objects[slot].copyPending=1;assert(JAR_MP_BreakableDamageBlocked(barrel));
 GlobalUse(barrel,owner,owner);assert(barrel->inuse);objects[slot].copyPending=0;
 Command(0,"jar_property","controller_enabled");assert(JAR_MP_BreakableDamageBlocked(barrel));
 GlobalUse(barrel,owner,owner);assert(barrel->inuse);
 Command(0,"jar_property","controller_enabled");assert(!JAR_MP_BreakableDamageBlocked(barrel));

 BreakableFixtureDamage(barrel,other,11);
 assert(barrel->health==29&&barrel->takedamage&&counter->count==2&&!barrel->nextthink);
 ActorArm(0,slot);assert(players[0].held==slot);
 VectorSet(barrel->r.currentAngles,15,90,0);
 vec3_t origin,angles;VectorCopy(barrel->r.currentOrigin,origin);VectorCopy(barrel->r.currentAngles,angles);
 int before=cargoBlastCalls;
 BreakableFixtureDamage(barrel,other,30);
 const int fuse=barrel->nextthink-level.time;
 assert(fuse>=100&&fuse<=500&&barrel->think&&!barrel->takedamage&&JAR_MP_BreakableDamageBlocked(barrel));
 assert(cargoBlastCalls==before&&counter->count==2&&barrel->inuse);
 ControllerTick(barrel,fuse-1);assert(barrel->inuse&&cargoBlastCalls==before);
 ControllerTick(barrel,1);
 assert(!barrel->inuse&&players[0].held==ENTITYNUM_NONE&&counter->count==1);
 assert(cargoBlastCalls==before+1&&cargoBlastDamage==100&&cargoBlastRadius==200&&CargoBarrelEffects(origin,angles)==1);
 assert(!objects[slot].owner&&!objects[slot].output[0]);
 assert(!JAR_RigidRead(slot,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));
}

static void CargoBarrelChain(void){
 RigidFixture();gentity_t *owner=&g_entities[0];
 gentity_t *source=ControllerAt("object_cargo_barrel1",0,100);
 gentity_t *victim=ControllerAt("object_cargo_barrel1",1,240);
 assert(source&&victim&&JAR_MP_BreakableRadiusDenied(source,victim));
 sharedEditing.integer=1;assert(!JAR_MP_BreakableRadiusDenied(source,victim));
 objects[victim->s.number].copyPending=1;assert(JAR_MP_BreakableRadiusDenied(source,victim));
 objects[victim->s.number].copyPending=0;
 cargoBlastVictim=victim->s.number;
 const int before=cargoBlastCalls;
 // The explicit Context action detonates immediately and the radius callback
 // schedules the second barrel instead of recursively detonating it.
 ControllerSelect(source);Command(0,"jar_property","activate");
 assert(!source->inuse&&victim->inuse&&!victim->takedamage&&cargoBlastCalls==before+1);
 const int fuse=victim->nextthink-level.time;assert(fuse>=100&&fuse<=500);
 cargoBlastVictim=ENTITYNUM_NONE;
 breakAllocationFail=1;ControllerTick(victim,fuse);breakAllocationFail=0;
 assert(!victim->inuse&&cargoBlastCalls==before+2);

 RigidFixture();source=ControllerAt("object_cargo_barrel1",0,100);
 gentity_t *counter=ControllerAt("target_counter",0,400);assert(source&&counter);
 ControllerLink(owner,source->s.number,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 cargoReplaceSource=qtrue;GlobalUse(source,owner,owner);cargoReplaceSource=qfalse;
 assert(source->inuse&&!strcmp(source->classname,"cargo_blast_replacement")&&counter->count==2);
}

static int CargoBarrelCopyIndex(const jarCopy_t *copy){
 for(int i=0;i<copy->count;++i)if(copy->node[i].id==91859725u)return i;
 return -1;
}

static void CargoBarrelPersistence(void){
 RigidFixture();gentity_t *owner=&g_entities[0];
 gentity_t *barrel=ControllerAt("object_cargo_barrel1",0,100);
 gentity_t *counter=ControllerAt("target_counter",0,400);
 assert(barrel&&counter);
 ControllerLink(owner,barrel->s.number,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 BreakableFixtureDamage(barrel,owner,40);
 level.time+=30;
 const int remaining=barrel->nextthink-level.time;assert(remaining>=70&&remaining<=470);
 const int before=cargoBlastCalls;
 CopyAssembly(owner,barrel->s.number);
 assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
 int index=CargoBarrelCopyIndex(&copies[0]);assert(index>=0&&copies[0].node[index].controllerRemaining==remaining);
 static jarWorld_t world,decoded;static byte data[WORLD_BYTES];
 assert(WorldCapture(&world));const int size=WorldEncode(&world,data);
 assert(size>0&&WorldDecode(data,size,&decoded));
 index=CargoBarrelCopyIndex(&decoded.copy);assert(index>=0);
 jarCopyNode_t *node=&decoded.copy.node[index];
 assert(node->controller.active==2&&node->controller.count==0&&node->controllerRemaining==remaining&&!node->stationUseRemaining);
 node->controllerRemaining=501;assert(!CopyControllersValid(&decoded.copy));
 node->controllerRemaining=remaining;node->stationUseRemaining=1;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(data,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 barrel=NULL;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_CARGO_BARREL_CONTROLLER)barrel=&g_entities[i];
 assert(barrel&&barrel->nextthink-level.time==remaining&&ControllerActivator(barrel)==barrel);
 const int output=objects[barrel->s.number].output[0]-1;assert(ControllerKind(output)==4);
 ControllerTick(barrel,remaining-1);assert(barrel->inuse&&cargoBlastCalls==before);
 ControllerTick(barrel,1);assert(!barrel->inuse&&cargoBlastCalls==before+1&&g_entities[output].count==1);

 assert(WorldClear()&&PlaceCopy(owner,&copies[0],(float[3]){1000,0,80},NULL,NULL));
 const int pasted=players[0].selected;assert(ControllerKind(pasted)==JAR_CARGO_BARREL_CONTROLLER);
 barrel=&g_entities[pasted];
 assert(barrel->nextthink-level.time==remaining&&JAR_MP_BreakableDamageBlocked(barrel));
 const int pastedOutput=objects[pasted].output[0]-1;assert(ControllerKind(pastedOutput)==4);
 GlobalUse(barrel,owner,owner);
 assert(!barrel->inuse&&g_entities[pastedOutput].count==1&&cargoBlastCalls==before+2);
 assert(WorldClear());
}

static void CargoBarrelCases(void){
 CargoBarrelSpawnAndDamage();CargoBarrelChain();CargoBarrelPersistence();PhysicsReset();
 puts("MP cargo barrel: stock asset and bounds, timed and direct blast, native event, authorization, nonrecursive splash, slot reuse, and pending world/duplicate restoration passed.");
}
