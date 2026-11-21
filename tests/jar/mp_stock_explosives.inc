// Portable explosive fixtures run the production callbacks and world codec.
// The radius boundary may feed one victim through the same permission gate.
typedef struct {
 const char *name;
 unsigned id;
 int kind,health,damage,radius,material;
} stockExplosiveFixture_t;
static const stockExplosiveFixture_t stockExplosiveFixtures[]={
 {"misc_exploding_crate",93146776u,JAR_EXPLODING_CRATE_CONTROLLER,40,50,128,MAT_CRATE1},
 {"misc_gas_tank",84862444u,JAR_GAS_TANK_CONTROLLER,20,32,48,MAT_METAL3}
};
static int stockBlastCalls,stockBlastDamage,stockBlastRadius,stockBlastDepth;
static int stockBlastVictim=ENTITYNUM_NONE,stockBlastOutput=ENTITYNUM_NONE;
static vec3_t stockBlastOrigin;

static void StockExplosiveRadius(const vec3_t origin,gentity_t *source,int damage,int radius){
 assert(source->inuse);
 assert(++stockBlastDepth==1);
 ++stockBlastCalls;stockBlastDamage=damage;stockBlastRadius=radius;VectorCopy(origin,stockBlastOrigin);
 if(stockBlastOutput!=ENTITYNUM_NONE)assert(g_entities[stockBlastOutput].count==1);
 if(stockBlastVictim!=ENTITYNUM_NONE){
  gentity_t *victim=&g_entities[stockBlastVictim];
  if(victim!=source&&!JAR_MP_BreakableRadiusDenied(source,victim))BreakableFixtureDamage(victim,source,damage);
 }
 --stockBlastDepth;
}

static int StockEffectCount(int index){
 int count=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const gentity_t *e=&g_entities[n];
  if(e->inuse&&e->s.eType==ET_EVENTS+EV_PLAY_EFFECT_ID&&e->s.eventParm==index)++count;
 }
 return count;
}

static void StockGasEffectAt(int index,const vec3_t origin,const vec3_t up){
 int matches=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const gentity_t *e=&g_entities[n];
  if(!e->inuse||e->s.eType!=ET_EVENTS+EV_PLAY_EFFECT_ID||e->s.eventParm!=index)continue;
  vec3_t forward;AngleVectors(e->s.angles,forward,NULL,NULL);
  assert(e->freeAfterEvent&&e->r.linked&&DotProduct(up,forward)>.999f);
  assert(Distance(origin,e->s.pos.trBase)<.01f&&Distance(origin,e->s.origin)<.01f);
  ++matches;
 }
 assert(matches==1);
}

static void StockExplosiveDeathEffects(int material){
 int debris=0,blast=0,sound=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const gentity_t *e=&g_entities[n];if(!e->inuse)continue;
  if(e->s.eType==ET_EVENTS+EV_DEBRIS){
   assert(e->freeAfterEvent&&e->s.trickedentindex==material&&e->s.eventParm>0);++debris;
  }
  if(e->s.eType==ET_EVENTS+EV_MISC_MODEL_EXP){assert(e->freeAfterEvent&&e->s.eventParm==material);++blast;}
  if(e->s.eType==ET_EVENTS+EV_GENERAL_SOUND){assert(e->freeAfterEvent&&e->s.eventParm==1);++sound;}
 }
 assert(debris==1&&blast==1&&sound==1);
}

static void StockExplosiveDefaults(void){
 for(int which=0;which<ARRAY_LEN(stockExplosiveFixtures);++which){
  const stockExplosiveFixture_t *fixture=&stockExplosiveFixtures[which];
  RigidFixture();imports.SendServerCommand=ControllerPropertySend;
  gentity_t *owner=&g_entities[0],*other=&g_entities[1];
  int models=which?tankModelRegistrations:crateModelRegistrations, sounds=stockExplosionSounds;
  int jets=gasJetRegistrations,flames=gasFlameRegistrations;
  assert(ControllerId(fixture->name)==fixture->id);
  const int firstSlot=level.num_entities;
  gentity_t *body=SpawnWithController(owner,fixture->id,(vec3_t){100,0,80},vec3_origin);
  assert(body&&body->s.number==firstSlot&&level.num_entities==firstSlot+1&&!objects[firstSlot].output[0]);
  gentity_t *counter=ControllerAt("target_counter",0,400);
  assert(body&&counter&&(which?tankModelRegistrations:crateModelRegistrations)>models&&stockExplosionSounds>sounds);
  if(which)assert(gasJetRegistrations>jets&&gasFlameRegistrations>flames);
  const int slot=body->s.number;
  assert(ControllerKind(slot)==fixture->kind&&objects[slot].owner==1&&PhysicsProp(slot));
  assert(body->health==fixture->health&&body->splashDamage==fixture->damage&&body->splashRadius==fixture->radius);
  assert(body->material==fixture->material&&body->takedamage&&body->use&&body->die&&body->r.linked&&(body->r.contents&CONTENTS_SOLID));
  assert(body->r.mins[0]==-12&&body->r.maxs[0]==20&&body->r.mins[2]==-4&&body->r.maxs[2]==12);

  ControllerSelect(body);assert(ContextCaps(owner,slot)&JAR_CONTEXT_CONTROLLER);
  Command(0,"jar_property","controller_count_77");assert(body->health==77);
  Command(0,"jar_property","property_undo");assert(body->health==fixture->health);
  Command(0,"jar_property","controller_damage_70");assert(body->splashDamage==70);
  Command(0,"jar_property","property_undo");assert(body->splashDamage==fixture->damage);
  Command(0,"jar_property","controller_radius_90");assert(body->splashRadius==90);
  Command(0,"jar_property","property_undo");assert(body->splashRadius==fixture->radius);
  ControllerLink(owner,slot,qfalse);ControllerLink(owner,counter->s.number,qfalse);
  assert(objects[slot].output[0]==counter->s.number+1);
  players[0].linkOutput=1;ControllerLink(owner,slot,qfalse);ControllerLink(owner,counter->s.number,qfalse);
  assert(!objects[slot].output[1]);players[0].linkOutput=0;

  const int before=stockBlastCalls;
  ControllerShot(other,slot,qfalse);assert(body->inuse&&stockBlastCalls==before);
  players[0].noTools=1;GlobalUse(body,owner,owner);assert(body->inuse);players[0].noTools=0;
  Command(0,"jar_property","controller_enabled");assert(JAR_MP_BreakableDamageBlocked(body));
  BreakableFixtureDamage(body,other,100);assert(body->health==fixture->health);
  GlobalUse(body,owner,owner);assert(body->inuse);Command(0,"jar_property","controller_enabled");
  assert(!JAR_MP_BreakableDamageBlocked(body));
  objects[slot].copyPending=1;assert(JAR_MP_BreakableDamageBlocked(body));GlobalUse(body,owner,owner);
  assert(body->inuse);objects[slot].copyPending=0;

  ActorArm(0,slot);assert(players[0].held==slot);
  BreakableFixtureDamage(body,other,fixture->health);
  const int fuse=body->nextthink-level.time;
  assert(body->inuse&&JAR_MP_BreakableDamageBlocked(body)&&!body->takedamage&&fuse>0&&fuse<=JAR_STOCK_EXPLOSIVE_FUSE);
  assert(stockBlastCalls==before&&counter->count==2);
  vec3_t blastAt;VectorCopy(body->r.currentOrigin,blastAt);blastAt[2]+=which?20:32;
  ControllerTick(body,fuse-1);assert(body->inuse&&stockBlastCalls==before);
  stockBlastOutput=counter->s.number;ControllerTick(body,1);stockBlastOutput=ENTITYNUM_NONE;
  assert(!body->inuse&&counter->count==1&&stockBlastCalls==before+1&&players[0].held==ENTITYNUM_NONE);
  assert(stockBlastDamage==fixture->damage&&stockBlastRadius==fixture->radius&&!objects[slot].owner);
  assert(Distance(blastAt,stockBlastOrigin)<.01f);
  assert(!StockEffectCount(FIXTURE_GAS_FLAME_EFFECT)); // Lethal damage skips Pain.
  StockExplosiveDeathEffects(fixture->material);
  assert(WorldClear());
 }
}

static int StockExplosiveNode(const jarCopy_t *copy,unsigned id){
 for(int i=0;i<copy->count;++i)if(copy->node[i].id==id)return i;
 return -1;
}

static gentity_t *StockExplosiveFind(int kind){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==kind)return &g_entities[n];
 return NULL;
}

static void StockGasPulse(void){
 RigidFixture();gentity_t *owner=&g_entities[0],*other=&g_entities[1];
 gentity_t *tank=ControllerAt("misc_gas_tank",0,100);assert(tank);
 const int slot=tank->s.number,initial=tank->nextthink-level.time,before=stockBlastCalls;
 assert(initial>=6000&&initial<=18000);
 vec3_t angles={30,15,45},up,at;
 assert(PhysicsSetAngles(tank,angles));AngleVectors(angles,NULL,NULL,up);
 ControllerTick(tank,initial-1);assert(!StockEffectCount(FIXTURE_GAS_JET_EFFECT));
 ControllerTick(tank,1);assert(stockBlastCalls==before);
 VectorMA(tank->r.currentOrigin,50,up,at);StockGasEffectAt(FIXTURE_GAS_JET_EFFECT,at,up);
 const int idle=tank->nextthink-level.time;assert(idle>=12000&&idle<=JAR_GAS_TANK_IDLE_MAX);

 assert(!JAR_MP_BreakableRadiusDenied(tank,other));
 players[1].noTools=1;assert(JAR_MP_BreakableRadiusDenied(tank,other));players[1].noTools=0;
 BreakableFixtureDamage(tank,other,2);
 assert(tank->health==18&&stockBlastCalls==before+1&&stockBlastDamage==32&&stockBlastRadius==32);
 VectorMA(tank->r.currentOrigin,78,up,at);assert(Distance(at,stockBlastOrigin)<.01f);
 VectorMA(tank->r.currentOrigin,46,up,at);StockGasEffectAt(FIXTURE_GAS_FLAME_EFFECT,at,up);
 BreakableFixtureDamage(tank,other,1);
 assert(tank->health==17&&stockBlastCalls==before+1&&StockEffectCount(FIXTURE_GAS_FLAME_EFFECT)==1);

 level.time+=20;
 const int remaining=tank->nextthink-level.time;
 CopyAssembly(owner,slot);assert(copies[0].count==1&&CopyControllersValid(&copies[0]));
 jarCopyNode_t *copied=&copies[0].node[0];
 assert(copied->id==84862444u&&copied->controllerRemaining==remaining&&copied->controller.flags==JAR_GAS_TANK_PULSE_SPENT);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 int index=StockExplosiveNode(&decoded.copy,84862444u);assert(index>=0);
 jarCopyNode_t *node=&decoded.copy.node[index];
 assert(node->controllerRemaining==remaining&&node->controller.flags==JAR_GAS_TANK_PULSE_SPENT&&node->controller.count==17);
 node->controllerRemaining=JAR_GAS_TANK_IDLE_MAX+1;assert(!CopyControllersValid(&decoded.copy));
 node->controllerRemaining=0;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 tank=StockExplosiveFind(JAR_GAS_TANK_CONTROLLER);assert(tank&&tank->health==17&&tank->nextthink-level.time==remaining);
 BreakableFixtureDamage(tank,other,1);assert(tank->health==16&&stockBlastCalls==before+1);
 assert(WorldClear()&&PlaceCopy(owner,&copies[0],(float[3]){900,0,80},NULL,NULL));
 tank=&g_entities[players[0].selected];assert(ControllerKind(tank->s.number)==JAR_GAS_TANK_CONTROLLER&&tank->nextthink-level.time==remaining);
 BreakableFixtureDamage(tank,other,1);assert(tank->health==16&&stockBlastCalls==before+1);

 ControllerSelect(tank);Command(0,"jar_property","controller_enabled");
 assert(tank->flags&FL_INACTIVE);
 ControllerTick(tank,tank->nextthink-level.time);
 assert(tank->inuse&&StockEffectCount(FIXTURE_GAS_JET_EFFECT)==1&&stockBlastCalls==before+1);
 Command(0,"jar_property","controller_enabled");assert(!(tank->flags&FL_INACTIVE));
 const int due=tank->nextthink-level.time;
 ControllerTick(tank,due>0?due:0);
 assert(StockEffectCount(FIXTURE_GAS_JET_EFFECT)==2&&stockBlastCalls==before+1&&WorldClear());
}

static void StockExplosivePersistence(void){
 for(int which=0;which<ARRAY_LEN(stockExplosiveFixtures);++which){
  const stockExplosiveFixture_t *fixture=&stockExplosiveFixtures[which];
  RigidFixture();gentity_t *owner=&g_entities[0];
  gentity_t *body=ControllerAt(fixture->name,0,100),*counter=ControllerAt("target_counter",0,400);assert(body&&counter);
  ControllerLink(owner,body->s.number,qfalse);ControllerLink(owner,counter->s.number,qfalse);
  if(which)BreakableFixtureDamage(body,owner,1); // Persist the already spent pulse through pending death.
  const int before=stockBlastCalls;
  BreakableFixtureDamage(body,owner,fixture->health);
  const int remaining=body->nextthink-level.time;assert(remaining>0&&remaining<=JAR_STOCK_EXPLOSIVE_FUSE);
  CopyAssembly(owner,body->s.number);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
  static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
  assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);
  assert(size>0&&WorldDecode(bytes,size,&decoded));
  const int index=StockExplosiveNode(&decoded.copy,fixture->id);assert(index>=0);
  jarCopyNode_t *node=&decoded.copy.node[index];
  assert(node->controller.active==2&&node->controller.count==0&&node->controllerRemaining==remaining);
  assert(node->controller.flags==(which?JAR_GAS_TANK_PULSE_SPENT:0));
  node->controllerRemaining=JAR_STOCK_EXPLOSIVE_FUSE+1;assert(!CopyControllersValid(&decoded.copy));
  node->controllerRemaining=remaining;node->stationUseRemaining=1;assert(!CopyControllersValid(&decoded.copy));
  assert(WorldDecode(bytes,size,&decoded)&&WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
  body=StockExplosiveFind(fixture->kind);assert(body&&body->nextthink-level.time==remaining&&ControllerActivator(body)==body);
  const int output=objects[body->s.number].output[0]-1;assert(ControllerKind(output)==4);
  ControllerTick(body,remaining-1);assert(body->inuse&&stockBlastCalls==before);
  ControllerTick(body,1);assert(!body->inuse&&g_entities[output].count==1&&stockBlastCalls==before+1);

  assert(WorldClear()&&PlaceCopy(owner,&copies[0],(float[3]){1000,0,80},NULL,NULL));
  body=&g_entities[players[0].selected];assert(ControllerKind(body->s.number)==fixture->kind);
  const int pastedOutput=objects[body->s.number].output[0]-1;assert(ControllerKind(pastedOutput)==4);
  assert(body->nextthink-level.time==remaining);
  GlobalUse(body,owner,owner);
  assert(body->inuse&&body->nextthink-level.time==remaining&&stockBlastCalls==before+1);
  ControllerTick(body,remaining);
  assert(!body->inuse&&g_entities[pastedOutput].count==1&&stockBlastCalls==before+2&&WorldClear());
 }
}

static void StockExplosiveChain(void){
 RigidFixture();gentity_t *owner=&g_entities[0];
 gentity_t *crate=ControllerAt("misc_exploding_crate",0,100),*tank=ControllerAt("misc_gas_tank",1,170);
 assert(crate&&tank&&JAR_MP_BreakableRadiusDenied(crate,tank));
 stockBlastVictim=tank->s.number;
 const int before=stockBlastCalls;
 GlobalUse(crate,owner,owner);
 assert(!crate->inuse&&tank->health==20&&stockBlastCalls==before+1);
 crate=ControllerAt("misc_exploding_crate",0,100);assert(crate);
 sharedEditing.integer=1;assert(!JAR_MP_BreakableRadiusDenied(crate,tank));
 objects[tank->s.number].copyPending=1;assert(JAR_MP_BreakableRadiusDenied(crate,tank));objects[tank->s.number].copyPending=0;
 GlobalUse(crate,owner,owner);stockBlastVictim=ENTITYNUM_NONE;
 assert(!crate->inuse&&tank->inuse&&!tank->takedamage&&stockBlastCalls==before+2);
 const int fuse=tank->nextthink-level.time;assert(fuse>0&&fuse<=JAR_STOCK_EXPLOSIVE_FUSE);
 breakAllocationFail=1;ControllerTick(tank,fuse);breakAllocationFail=0;
 assert(!tank->inuse&&stockBlastCalls==before+3&&WorldClear());

 // A nonlethal splash can ignite another gas tank in the same tick. The
 // radius mock rejects re-entry so the native queue must flatten the chain.
 RigidFixture();
 gentity_t *first=ControllerAt("misc_gas_tank",0,100),*second=ControllerAt("misc_gas_tank",0,140);
 assert(first&&second);ControllerSelect(second);
 Command(0,"jar_property","controller_count_50");assert(second->health==50);
 stockBlastVictim=second->s.number;
 BreakableFixtureDamage(first,owner,1);
 stockBlastVictim=ENTITYNUM_NONE;
 assert(first->health==19&&second->health==18&&stockBlastCalls==before+5&&StockEffectCount(FIXTURE_GAS_FLAME_EFFECT)==2);
 BreakableFixtureDamage(first,owner,1);BreakableFixtureDamage(second,owner,1);
 assert(stockBlastCalls==before+5&&WorldClear());
}

static void StockExplosiveCases(void){
 StockExplosiveDefaults();StockGasPulse();StockExplosivePersistence();StockExplosiveChain();PhysicsReset();
 puts("MP stock explosive crate and gas tank: defaults, owned death outputs, terminal cleanup, gas pulse and idle FX, permissions, bounded chains, and pending world/duplicate restore passed.");
}
