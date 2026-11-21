// Ammo rack stock, clip policy and consumed slots through the native fixture.
static void AmmoRackFixture(void){
 RackFixture();
 md3Frame_t *frame=(md3Frame_t*)(rigidModel+sizeof(md3Header_t));
 VectorSet(frame->bounds[0],-5.515625f,-13.8125f,.0625f);
 VectorSet(frame->bounds[1],7.359375f,13.546875f,47.28125f);
 bg_numItems=6;
 bg_itemlist[1]=(gitem_t){.classname="ammo_blaster",.world_model={"models/items/energy_cell.md3"},.giType=IT_AMMO,.giTag=AMMO_BLASTER};
 bg_itemlist[2]=(gitem_t){.classname="ammo_metallic_bolts",.world_model={"models/items/metallic_bolts.md3"},.giType=IT_AMMO,.giTag=AMMO_METAL_BOLTS};
 bg_itemlist[3]=(gitem_t){.classname="ammo_powercell",.world_model={"models/items/power_cell.md3"},.giType=IT_AMMO,.giTag=AMMO_POWERCELL};
 bg_itemlist[4]=(gitem_t){.classname="weapon_blaster",.world_model={"models/weapons2/blaster_r/blaster_w.glm"},.giType=IT_WEAPON,.giTag=WP_BLASTER};
 bg_itemlist[5]=(gitem_t){.classname="item_medpak_instant",.world_model={"models/map_objects/mp/medpac.md3"},.giType=IT_HEALTH};
}

static void AmmoRackFixturePose(const gentity_t *rack,int mask){
 static const int amount[]={25,100,100,15,20};
 static const float shelfRight[]={-8,0,8,7,-6},shelfUp[]={7,7,7,25.5f,24};
 vec3_t forward,right,up;
 AngleVectors(rack->r.currentAngles,forward,right,up);
 for(int slot=0;slot<JAR_MP_AMMO_RACK_SLOTS;++slot){
  const gentity_t *item=RackFixtureChild(rack->s.number,slot);
  if(mask&(1<<slot)){assert(!item);continue;}
  assert(item&&item->count==amount[slot]&&item->s.eType==ET_ITEM&&item->r.linked);
  assert(item->s.generic1==JAR_MP_RACK_ITEM_TAG&&item->s.otherEntityNum==rack->s.number);
  assert(item->s.modelindex2==(slot==4?FIXTURE_CARGO_MEDPACK_MODEL:1));
  assert(item->s.pos.trType==TR_INTERPOLATE&&item->s.apos.trType==TR_INTERPOLATE);
  assert(item->r.mins[2]==(slot==4?-3:slot==3?-2:0));
  assert(item->r.maxs[2]==(slot==3?12:16));
  vec3_t offset;VectorSubtract(item->r.currentOrigin,rack->r.currentOrigin,offset);
  assert(fabsf(DotProduct(offset,forward))<.02f);
  assert(fabsf(DotProduct(offset,right)-shelfRight[slot])<.02f);
  assert(fabsf(DotProduct(offset,up)-shelfUp[slot])<.02f);
 }
}

static void AmmoRackCases(void){
 AmmoRackFixture();
 gentity_t *owner=&g_entities[0],*observer=&g_entities[1];
 const int n=level.num_entities;
 Command(0,"jar_spawn","98056028");
 gentity_t *rack=&g_entities[n];
 assert(ControllerId("misc_model_ammo_rack")==JAR_MP_AMMO_RACK_ID);
 assert(ControllerKind(n)==JAR_AMMO_RACK_CONTROLLER&&RackFixtureEntities()==6&&PhysicsSolid(n));
 assert(!rack->use&&JAR_MP_AmmoRackClip(&rack->s)&&rack->r.contents==JAR_MP_AMMO_RACK_CONTENTS);
 assert(JAR_MP_AmmoRackClipTrace(MASK_PLAYERSOLID)&&JAR_MP_AmmoRackClipTrace(MASK_NPCSOLID));
 assert(JAR_MP_AmmoRackClipTrace(MASK_DEADSOLID)&&JAR_MP_AmmoRackClipTrace(CONTENTS_BOTCLIP));
 assert(JAR_MP_AmmoRackClipTrace(MASK_SHOT)&&JAR_MP_AmmoRackClipTrace(MASK_SHOT|CONTENTS_LIGHTSABER));
 assert(!JAR_MP_AmmoRackClipTrace(MASK_SOLID)&&!JAR_MP_AmmoRackClipTrace(MASK_OPAQUE));
 const int useMask=MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE;
 assert(!JAR_MP_AmmoRackClipTrace(useMask));
 entityState_t unrelated=rack->s;unrelated.eType=ET_ITEM;assert(!JAR_MP_AmmoRackClip(&unrelated));
 AmmoRackFixturePose(rack,0);
 assert(PhysicsSetAngles(rack,(vec3_t){9,127,-12}));
 Move(rack,(vec3_t){320,100,100});
 AmmoRackFixturePose(rack,0);

 gentity_t *ammo=RackFixtureChild(n,0);
 Touch_Item(ammo,observer,NULL);
 assert(pickupEffects==1&&pickupEvents==1&&!RackFixtureChild(n,0));
 G_FreeEntity(ammo);

 gentity_t *medpak=RackFixtureChild(n,4);
 VectorCopy(medpak->r.currentOrigin,owner->client->ps.origin);
 owner->client->ps.origin[0]-=160;
 g_knockback.value=1000;
 assert(JAR_MP_RackPull(owner,medpak));
 assert(medpak->s.generic1==JAR_MP_CARGO_DROP_TAG&&medpak->s.modelindex2==FIXTURE_CARGO_MEDPACK_MODEL);
 assert(medpak->count==20&&medpak->s.pos.trType==TR_GRAVITY&&medpak->nextthink==level.time+30000);
 assert(objects[medpak->s.number].transientPickup);
 AmmoRackFixturePose(rack,17);

 CopyAssembly(owner,n);
 assert(copies[0].count==1&&copies[0].node[0].controller.flags==17);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){800,80,100},NULL,NULL));
 const int copy=players[0].selected;
 assert(copy!=n&&JAR_MP_AmmoRackClip(&g_entities[copy].s));
 AmmoRackFixturePose(&g_entities[copy],17);
 Undo(owner,qfalse);assert(!g_entities[copy].inuse&&medpak->inuse);

 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==1&&saved.copy.node[0].controller.flags==17);
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 decoded.copy.node[0].controller.flags=32;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear()&&RackFixtureEntities()==0);
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 rack=StockExplosiveFind(JAR_AMMO_RACK_CONTROLLER);
 assert(rack&&RackFixtureEntities()==4&&JAR_MP_AmmoRackClip(&rack->s));
 AmmoRackFixturePose(rack,17);
 assert(WorldClear()&&RackFixtureEntities()==0);

 rackAllocationFailureCountdown=5;
 assert(!SpawnWithController(owner,JAR_MP_AMMO_RACK_ID,(vec3_t){300,0,100},vec3_origin));
 assert(!rackAllocationFailureCountdown&&RackFixtureEntities()==0);
 PhysicsReset();
 puts("MP ammo rack: five stock slots, clip policy, pickup, Force Pull, copy, JRW18 and rollback passed.");
}
