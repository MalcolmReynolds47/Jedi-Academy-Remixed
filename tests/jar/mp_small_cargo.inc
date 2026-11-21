// Exercise the real crate controller and journal. The wire probe covers the
// native missile, drop allocation, gravity and player pickup code end to end.
static void SmallCargoDamage(gentity_t *crate,gentity_t *projectile,gentity_t *attacker,int damage,int flags,int mod){
 if(!crate->takedamage||JAR_MP_BreakableDamageBlocked(crate)||
    JAR_MP_SmallCargoDamageBlocked(crate,projectile,flags,mod))return;
 crate->health-=damage;
 if(crate->health<=0)crate->die(crate,projectile,attacker,damage,mod);
 else crate->pain(crate,attacker,damage);
}

static gentity_t *SmallCargoPickup(void){
 gentity_t *pickup=NULL;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t *e=&g_entities[n];
  if(!e->inuse||!objects[n].transientPickup)continue;
  assert(!pickup);
  pickup=e;
 }
 return pickup;
}

static void SmallCargoCases(void){
 RigidFixture();
 imports.SendServerCommand=ControllerPropertySend;
 bg_numItems=3;
 bg_itemlist[1]=(gitem_t){.classname="item_medpak_instant",.world_model={"models/map_objects/mp/medpac.md3"},.quantity=25,.giType=IT_HEALTH};
 gentity_t *owner=&g_entities[0],*observer=&g_entities[1];
 const int oldRegistrations=registeredItems;
 owner->client->ps.viewangles[YAW]=90;
 const int first=level.num_entities;
 Command(0,"jar_spawn","87961440");
 gentity_t *crate=&g_entities[first];
 assert(crate->inuse&&level.num_entities==first+1);
 assert(ControllerId("misc_model_cargo_small")==JAR_MP_SMALL_CARGO_ID);
 assert(ControllerKind(first)==JAR_SMALL_CARGO_CONTROLLER&&PhysicsProp(first));
 assert(crate->r.currentAngles[YAW]==90&&crate->r.linked&&crate->r.contents&CONTENTS_SOLID);
 assert(crate->health==40&&crate->splashDamage==1&&crate->splashRadius==96);
 assert(crate->material==MAT_CRATE1&&!crate->use&&!objects[first].output[0]);
 assert(registeredItems>oldRegistrations&&!SmallCargoPickup());

 ControllerSelect(crate);
 Command(0,"jar_property","controller_count_75");assert(crate->health==75);
 Command(0,"jar_property","property_undo");assert(crate->health==40);
 Command(0,"jar_property","activate");
 assert(crate->health==40&&crate->takedamage&&!SmallCargoPickup());
 gentity_t *relay=ControllerAt("target_relay",0,500);
 ControllerLink(owner,relay->s.number,qfalse);ControllerLink(owner,first,qfalse);
 assert(!objects[relay->s.number].output[0]);
 G_FreeEntity(relay);
 gentity_t *counter=ControllerAt("target_counter",0,700);
 ControllerLink(owner,first,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 assert(objects[first].output[0]==counter->s.number+1);
 players[0].linkOutput=1;
 ControllerLink(owner,first,qfalse);ControllerLink(owner,counter->s.number,qfalse);
 assert(!objects[first].output[1]);
 players[0].linkOutput=0;

 gentity_t projectile={.inuse=qtrue};
 projectile.s.eType=ET_MISSILE;
 projectile.methodOfDeath=MOD_TURBLAST;
 SmallCargoDamage(crate,&projectile,observer,40,0,MOD_BLASTER);
 SmallCargoDamage(crate,&projectile,observer,40,0,MOD_ROCKET);
 SmallCargoDamage(crate,&projectile,observer,40,0,MOD_SABER);
 SmallCargoDamage(crate,&projectile,observer,40,0,MOD_VEHICLE);
 assert(JAR_MP_SmallCargoDamageBlocked(crate,NULL,0,MOD_TURBLAST));
 projectile.dflags=DAMAGE_JAR_CARGO_WALKER_SHOT;
 assert(!JAR_MP_SmallCargoDamageBlocked(crate,&projectile,0,MOD_VEHICLE));
 projectile.dflags=DAMAGE_HEAVY_WEAP_CLASS;
 assert(!JAR_MP_SmallCargoDamageBlocked(crate,&projectile,0,MOD_VEHICLE));
 assert(JAR_MP_SmallCargoDamageBlocked(crate,&projectile,DAMAGE_RADIUS,MOD_VEHICLE));
 projectile.dflags=0;
 SmallCargoDamage(crate,&projectile,observer,40,DAMAGE_RADIUS,MOD_TURBLAST);
 assert(crate->health==40);
 Command(0,"jar_property","controller_enabled");
 SmallCargoDamage(crate,&projectile,observer,40,0,MOD_TURBLAST);
 assert(crate->health==40);
 Command(0,"jar_property","controller_enabled");
 SmallCargoDamage(crate,&projectile,observer,13,0,MOD_TURBLAST);
 assert(crate->health==27);

 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==2);
 int index=StockExplosiveNode(&saved.copy,JAR_MP_SMALL_CARGO_ID);
 assert(index>=0&&saved.copy.node[index].controller.count==27);
 SmallCargoDamage(crate,&projectile,observer,27,0,MOD_TURBLAST);
 assert(crate->inuse&&!crate->takedamage&&!SmallCargoPickup());
 const int remaining=crate->nextthink-level.time;
 assert(remaining>0&&remaining<=JAR_STOCK_EXPLOSIVE_FUSE);
 assert(WorldCapture(&saved));
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 index=StockExplosiveNode(&decoded.copy,JAR_MP_SMALL_CARGO_ID);
 assert(index>=0&&decoded.copy.node[index].controllerRemaining==remaining);
 jarCopyNode_t *node=&decoded.copy.node[index];
 assert(node->controller.active==2&&!node->controller.count&&!node->output[1]);
 node->controllerRemaining=JAR_STOCK_EXPLOSIVE_FUSE+1;
 assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 crate=StockExplosiveFind(JAR_SMALL_CARGO_CONTROLLER);
 assert(crate&&crate->nextthink-level.time==remaining&&!SmallCargoPickup());
 const int output=objects[crate->s.number].output[0]-1;
 const int ownerSlot=objects[crate->s.number].owner;
 const unsigned group=objects[crate->s.number].order;
 vec3_t origin;VectorCopy(crate->r.currentOrigin,origin);
 const int blasts=breakRadiusCalls;
 ControllerTick(crate,remaining-1);
 assert(crate->inuse&&!SmallCargoPickup()&&g_entities[output].count==2);
 ControllerTick(crate,1);
 assert(!crate->inuse&&breakRadiusCalls==blasts+1&&g_entities[output].count==1);
 gentity_t *pickup=SmallCargoPickup();
 assert(pickup&&pickup->item==&bg_itemlist[1]&&pickup->s.eType==ET_ITEM);
 assert(pickup->flags&FL_DROPPED_ITEM);
 assert(pickup->count==20&&pickup->s.generic1==JAR_MP_CARGO_DROP_TAG);
 assert(pickup->s.modelindex2==FIXTURE_CARGO_MEDPACK_MODEL&&VectorCompare(pickup->s.angles,vec3_origin));
 assert(VectorCompare(pickup->r.mins,((vec3_t){-8,-8,-3})));
 assert(VectorCompare(pickup->r.maxs,((vec3_t){8,8,16})));
 assert(objects[pickup->s.number].owner==ownerSlot&&objects[pickup->s.number].order==group);
 for(int axis=0;axis<2;++axis){
  const float offset=pickup->r.currentOrigin[axis]-origin[axis];
  assert(offset>=8&&offset<=24);
 }
 assert(pickup->r.currentOrigin[2]==origin[2]+16);
 assert(WorldCapture(&saved)&&saved.copy.count==1&&saved.copy.node[0].id==ControllerId("target_counter"));
 assert(WorldClear()&&!pickup->inuse&&!SmallCargoPickup());

 crate=ControllerAt("misc_model_cargo_small",0,100);
 assert(crate);
 breakAllocationFail=1;
 SmallCargoDamage(crate,&projectile,owner,40,0,MOD_TURBLAST);
 ControllerTick(crate,JAR_STOCK_EXPLOSIVE_FUSE);
 assert(!crate->inuse&&!SmallCargoPickup());
 breakAllocationFail=0;
 assert(WorldClear());
 PhysicsReset();
 puts("MP small cargo crate: Q spawn, heavy-only damage, no activation, Primary, pending save, tracked transient pickup and exhaustion passed.");
}
