// Portable rack state through real Sandbox commands, native pickup dispatch,
// placement callbacks and the world codec.
static const int rackWeapons[]={WP_BLASTER,WP_REPEATER,WP_ROCKET_LAUNCHER};
static const int rackAmmo[]={15,100,4};
static const float rackHeights[]={23,24.5f,25.5f};

static void RackFixture(void){
 RigidFixture();
 md3Frame_t *frame=(md3Frame_t*)(rigidModel+sizeof(md3Header_t));
 VectorSet(frame->bounds[0],-5.515625f,-16.015625f,-.015625f);
 VectorSet(frame->bounds[1],7.375f,16.125f,47.1875f);
 imports.SendServerCommand=ControllerPropertySend;
 itemDisabled=0;pickupAllowed=1;pickupEffects=pickupEvents=0;
 g_entities[0].client->pers.predictItemPickup=qtrue;
 g_entities[1].client->pers.predictItemPickup=qtrue;
 bg_numItems=4;
 bg_itemlist[1]=(gitem_t){.classname="weapon_blaster",.world_model={"models/weapons2/blaster_r/blaster_w.glm"},.giType=IT_WEAPON,.giTag=WP_BLASTER};
 bg_itemlist[2]=(gitem_t){.classname="weapon_repeater",.world_model={"models/weapons2/heavy_repeater/heavy_repeater_w.glm"},.giType=IT_WEAPON,.giTag=WP_REPEATER};
 bg_itemlist[3]=(gitem_t){.classname="weapon_rocket_launcher",.world_model={"models/weapons2/merr_sonn/merr_sonn_w.glm"},.giType=IT_WEAPON,.giTag=WP_ROCKET_LAUNCHER};
}

static int RackFixtureEntities(void){
 int count=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse)++count;
 return count;
}

static gentity_t *RackFixtureChild(int parent,int slot){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t *e=&g_entities[n];
  if(e->inuse&&e->item==&bg_itemlist[slot+1]&&JAR_MP_RackParent(e)==parent)return e;
 }
 return NULL;
}

static void RackFixturePose(const gentity_t *rack,int mask){
 vec3_t forward,right,up;
 AngleVectors(rack->r.currentAngles,forward,right,up);
 for(int slot=0;slot<JAR_MP_GUN_RACK_SLOTS;++slot){
  gentity_t *item=RackFixtureChild(rack->s.number,slot);
  if(mask&(1<<slot)){assert(!item);continue;}
  assert(item&&item->item->giTag==rackWeapons[slot]&&item->count==rackAmmo[slot]);
  assert(item->s.eType==ET_ITEM&&item->s.generic1==JAR_MP_RACK_ITEM_TAG&&item->s.modelindex2==1);
  assert(item->s.otherEntityNum==rack->s.number&&item->r.linked&&(item->flags&FL_DROPPED_ITEM));
  assert(item->s.pos.trType==TR_INTERPOLATE&&item->s.apos.trType==TR_INTERPOLATE);
  vec3_t offset;VectorSubtract(item->r.currentOrigin,rack->r.currentOrigin,offset);
  assert(fabsf(DotProduct(offset,forward))<=2.01f);
  assert(fabsf(DotProduct(offset,right)-(slot-1)*9)<=2.01f);
  assert(fabsf(DotProduct(offset,up)-rackHeights[slot])<.02f);
 }
}

static void GunRackCases(void){
 RackFixture();
 gentity_t *owner=&g_entities[0],*observer=&g_entities[1];
 owner->client->ps.viewangles[YAW]=90;
 const int first=level.num_entities;
 Command(0,"jar_spawn","96929416");
 gentity_t *rack=&g_entities[first];
 assert(ControllerId("misc_model_gun_rack")==JAR_MP_GUN_RACK_ID);
 assert(ControllerKind(first)==JAR_GUN_RACK_CONTROLLER&&PhysicsProp(first));
 assert(rack->r.currentAngles[YAW]==90&&RackFixtureEntities()==4);
 assert(!rack->use&&!objects[first].output[0]&&!objects[first].output[1]);
 RackFixturePose(rack,0);

 ControllerSelect(rack);
 Command(0,"jar_property","activate");
 Command(0,"jar_property","controller_enabled");
 Command(0,"jar_property","controller_flags_7");
 jarControllerState_t state;
 ControllerState(first,&state);assert(!state.flags&&!state.inactive);
 RackFixturePose(rack,0);
 assert(PhysicsSetAngles(rack,(vec3_t){18,63,11}));
 Move(rack,(vec3_t){360,40,100});
 RackFixturePose(rack,0);

 gentity_t *blaster=RackFixtureChild(first,0);
 pickupAllowed=0;
 Touch_Item(blaster,observer,NULL);
 assert(RackFixtureChild(first,0)==blaster&&!pickupEffects);
 pickupAllowed=1;
 Touch_Item(blaster,observer,NULL);
 assert(pickupEffects==1&&pickupEvents==1&&!RackFixtureChild(first,0));
 assert(!blaster->r.contents&&(blaster->s.eFlags&EF_NODRAW)&&blaster->freeAfterEvent);
 Touch_Item(blaster,observer,NULL);assert(pickupEffects==1);
 G_FreeEntity(blaster);
 ControllerState(first,&state);assert(state.flags==1);
 RackFixturePose(rack,1);

 CopyAssembly(owner,first);
 assert(copies[0].count==1&&copies[0].node[0].controller.flags==1);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){800,80,100},NULL,NULL));
 const int copy=players[0].selected;
 assert(copy!=first&&ControllerKind(copy)==JAR_GUN_RACK_CONTROLLER);
 RackFixturePose(&g_entities[copy],1);
 Undo(owner,qfalse);
 assert(!g_entities[copy].inuse&&RackFixtureEntities()==3);
 RackFixturePose(rack,1);

 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==1);
 assert(saved.copy.node[0].id==JAR_MP_GUN_RACK_ID&&saved.copy.node[0].controller.flags==1);
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 decoded.copy.node[0].controller.flags=8;assert(!CopyControllersValid(&decoded.copy));
 decoded.copy.node[0].controller.flags=1;
 decoded.copy.node[0].controller.active=1;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear()&&RackFixtureEntities()==0);
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 assert(RackFixtureEntities()==3);
 rack=StockExplosiveFind(JAR_GUN_RACK_CONTROLLER);assert(rack);
 RackFixturePose(rack,1);
 assert(WorldClear()&&RackFixtureEntities()==0);

 rack=SpawnWithController(owner,JAR_MP_GUN_RACK_ID,(vec3_t){300,0,100},vec3_origin);
 assert(rack);
 const int parent=rack->s.number;
 gentity_t *repeater=RackFixtureChild(parent,1);
 assert(repeater);
 vec3_t from,to;
 VectorCopy(repeater->r.currentOrigin,to);
 VectorCopy(to,from);from[0]-=160;
 VectorCopy(from,owner->client->ps.origin);
 VectorCopy(from,observer->client->ps.origin);
 assert(JAR_MP_RackAimItem(observer,parent,from,to)==ENTITYNUM_NONE);
 assert(JAR_MP_RackAimItem(owner,parent,from,to)==repeater->s.number);
 assert(!JAR_MP_RackPull(observer,repeater));
 blocked=1;assert(!JAR_MP_RackPull(owner,repeater));blocked=0;
 g_knockback.value=1000;
 assert(JAR_MP_RackPull(owner,repeater));
 assert(JAR_MP_RackParent(repeater)==ENTITYNUM_NONE&&repeater->s.pos.trType==TR_GRAVITY);
 assert(repeater->count==100&&repeater->nextthink==level.time+30000);
 assert(objects[repeater->s.number].transientPickup&&objects[repeater->s.number].order==objects[parent].order);
 assert(VectorLength(repeater->s.pos.trDelta)>200);
 RackFixturePose(rack,2);
 assert(WorldCapture(&saved)&&saved.copy.count==1&&saved.copy.node[0].controller.flags==2);
 Undo(owner,qfalse);
 assert(!repeater->inuse&&!rack->inuse&&RackFixtureEntities()==0);

 itemDisabled=1;
 assert(!SpawnWithController(owner,JAR_MP_GUN_RACK_ID,(vec3_t){300,0,100},vec3_origin));
 assert(RackFixtureEntities()==0);
 itemDisabled=0;
 rackAllocationFailureCountdown=3;
 assert(!SpawnWithController(owner,JAR_MP_GUN_RACK_ID,(vec3_t){300,0,100},vec3_origin));
 assert(!rackAllocationFailureCountdown&&RackFixtureEntities()==0);
 PhysicsReset();
 puts("MP gun rack: stock shelf transforms, native one-shot pickup, Force Pull, copy, JRW18, Undo, and transactional stock admission passed.");
}
