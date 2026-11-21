// Full command/link/context/copy/world route and actual native Touch_Item.
static unsigned ActionItemId(const char*name){for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==4&&!strcmp(jarMPCatalog[i].name,name))return jarMPCatalog[i].id;assert(0);return 0;}
static void HoldableCatalogCases(void){
 const struct {const char* alias;const char* native;int tag;} items[]={
  {"item_bacta","item_medpac",HI_MEDPAC},
  {"item_la_goggles","item_binoculars",HI_BINOCULARS},
  {"item_enviro","item_shield",HI_SHIELD}
 };
 for(int i=0;i<ARRAY_LEN(items);++i){
  RigidFixture();gentity_t*p=&g_entities[0];
  bg_numItems=2;bg_itemlist[1]=(gitem_t){0};
  gitem_t*native=&bg_itemlist[1];native->classname=items[i].native;native->giType=IT_HOLDABLE;native->giTag=items[i].tag;
  const unsigned newId=ActionItemId(items[i].alias),oldId=ActionItemId(items[i].native);
  const unsigned ids[]={newId,oldId};
  int index;
  assert(JAR_MPCatalogVisible(Entry(newId,&index))&&!JAR_MPCatalogVisible(Entry(oldId,&index)));
  for(int j=0;j<ARRAY_LEN(ids);++j){
   char id[32];snprintf(id,sizeof(id),"%u",ids[j]);aimed=ENTITYNUM_WORLD;
   const int oldItems=registeredItems;
   itemDisabled=1;Command(0,"jar_spawn",id);assert(!LiveCount()&&registeredItems==oldItems);
   itemDisabled=0;Command(0,"jar_spawn",id);
   int slot=Last(0);assert(slot>=MAX_CLIENTS&&g_entities[slot].item==native);
   assert(jarMPCatalog[objects[slot].catalog-1].id==ids[j]);
   CopyAssembly(p,slot);assert(copies[0].count==1&&copies[0].node[0].id==ids[j]);
   static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
   assert(WorldCapture(&saved));const int length=WorldEncode(&saved,bytes);
   assert(WorldDecode(bytes,length,&decoded)&&decoded.copy.node[0].id==ids[j]);
   assert(WorldClear()&&PlaceCopy(p,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
   slot=players[0].selected;assert(slot>=MAX_CLIENTS&&g_entities[slot].item==native);
   assert(jarMPCatalog[objects[slot].catalog-1].id==ids[j]);
   Command(0,"jar_undo","");assert(!LiveCount());
  }
 }
 PhysicsReset();
 puts("MP holdables: shared catalog IDs, native item admission, legacy IDs, world restore, copy and Undo passed.");
}
static void ActionControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0];pickupAllowed=1;pickupEffects=pickupEvents=scoreCalls=actionKillCalls=itemDisabled=0;
 bg_numItems=3;bg_itemlist[1]=(gitem_t){0};bg_itemlist[1].classname="item_medpak_instant";bg_itemlist[1].giType=IT_HEALTH;bg_itemlist[1].quantity=25;
 bg_itemlist[2]=(gitem_t){0};bg_itemlist[2].classname="weapon_stun_baton";bg_itemlist[2].giType=IT_WEAPON;bg_itemlist[2].giTag=WP_STUN_BATON;
 p->client->pers.predictItemPickup=qtrue;p->client->ps.stats[STAT_HEALTH]=p->health=100;
 gentity_t*give=SpawnWithController(p,ControllerId("target_give"),(float[3]){100,0,80},vec3_origin);assert(give&&give->use==Use_Target_Give);gentity_t*health=&g_entities[objects[give->s.number].output[0]-1];assert(health->item&&health->item->giType==IT_HEALTH&&objects[health->s.number].order==objects[give->s.number].order);
 // Actual delayed native initialization must start at the requested point, not world zero.
 assert(health->s.origin[0]==228);FinishSpawningItem(health);assert(health->r.currentOrigin[0]==228&&health->r.currentOrigin[2]==0&&health->touch==Touch_Item);
 gentity_t*score=ControllerAt("target_score",0,400),*kill=ControllerAt("target_kill",0,600),*delay=ControllerAt("target_delay",0,800);
 assert(score&&kill&&score->count==1&&score->use==Use_Target_Score&&kill->use==target_kill_use);
 // Invalid destinations and ordinary authored map names never enter owned wiring.
 int original=objects[give->s.number].output[0];ControllerLink(p,give->s.number,qfalse);ControllerLink(p,score->s.number,qfalse);assert(objects[give->s.number].output[0]==original);
 pickupAllowed=0;ControllerShot(p,give->s.number,qfalse);assert(!pickupEffects&&!objects[health->s.number].itemConsumed&&health->r.linked);pickupAllowed=1;
 players[0].noTools=1;ControllerShot(p,give->s.number,qfalse);assert(!pickupEffects);players[0].noTools=0;
 itemDisabled=1;ControllerShot(p,give->s.number,qfalse);assert(!pickupEffects);itemDisabled=0;
 ControllerSelect(give);Command(0,"jar_property","controller_enabled");ControllerShot(p,give->s.number,qfalse);assert(!pickupEffects);Command(0,"jar_property","controller_enabled");
 // Two distinct linked items, one owned by another player, with live permission.
 gentity_t*weapon=SpawnAt(&g_entities[1],ActionItemId("weapon_stun_baton"),(float[3]){1000,0,80},vec3_origin);assert(weapon);
 sharedEditing.integer=1;players[0].linkOutput=1;ControllerLink(p,give->s.number,qfalse);ControllerLink(p,weapon->s.number,qfalse);players[0].linkOutput=0;sharedEditing.integer=0;
 ControllerShot(p,give->s.number,qfalse);assert(pickupEffects==1&&pickupEvents==1&&objects[health->s.number].itemConsumed&&!health->r.linked&&!health->touch&&!health->nextthink&&!objects[weapon->s.number].itemConsumed);
 sharedEditing.integer=1;g_weaponDisable.integer=1<<WP_STUN_BATON;ControllerShot(p,give->s.number,qfalse);assert(pickupEffects==1);g_weaponDisable.integer=0;
 ControllerShot(p,give->s.number,qfalse);assert(pickupEffects==2&&pickupEvents==2&&objects[weapon->s.number].itemConsumed);ControllerShot(p,give->s.number,qfalse);assert(pickupEffects==2);
 // Consumed pickup validation is specific to item nodes; no arbitrary state on props.
 static jarCopy_t invalid;CopyAssembly(p,give->s.number);invalid=copies[0];invalid.node[0].itemConsumed=1;assert(!CopyControllersValid(&invalid));
 // A duplicate output must never grant the same pickup twice.
 gentity_t*extra=SpawnAt(p,ActionItemId("item_medpak_instant"),(float[3]){1200,0,80},vec3_origin);assert(extra);gentity_t*single=ControllerAt("target_give",0,1300);objects[single->s.number].output[0]=objects[single->s.number].output[1]=extra->s.number+1;ControllerShot(p,single->s.number,qfalse);assert(pickupEffects==3&&pickupEvents==3);G_FreeEntity(extra);assert(!objects[single->s.number].output[0]&&!objects[single->s.number].output[1]);G_FreeEntity(single);pickupEffects=pickupEvents=2;
 // Signed score setting and conflict-aware property Undo; no integer wrap.
 ControllerSelect(score);Command(0,"jar_property","controller_points_-50");assert(score->count==-50);Command(0,"jar_property","property_undo");assert(score->count==1);Command(0,"jar_property","controller_points_25");ControllerShot(p,score->s.number,qfalse);assert(scoreCalls==1&&scorePoints==25&&scoreClient==0);
 Command(0,"jar_property","controller_points_32768");assert(score->count==25);p->client->ps.persistant[PERS_SCORE]=INT_MAX;ControllerShot(p,score->s.number,qfalse);assert(scoreCalls==1);p->client->ps.persistant[PERS_SCORE]=0;
 Command(0,"jar_property","controller_points_-50");ControllerShot(p,score->s.number,qfalse);assert(scoreCalls==2&&scorePoints==-50);
 int oldGametype=level.gametype;level.gametype=GT_TEAM;p->client->ps.persistant[PERS_TEAM]=TEAM_RED;level.teamScores[TEAM_RED]=INT_MIN;ControllerShot(p,score->s.number,qfalse);assert(scoreCalls==2);level.teamScores[TEAM_RED]=0;level.gametype=oldGametype;
 // MP electrical rendering state, falling damage route and consent changes.
 ControllerSelect(kill);Command(0,"jar_property","controller_flags_2");ControllerShot(p,kill->s.number,qfalse);assert(actionKillCalls==1&&actionKillVictim==0&&actionKillMod==MOD_DEMP2&&p->client->ps.electrifyTime==level.time+4000);
 Command(0,"jar_property","controller_flags_1");ControllerShot(p,kill->s.number,qfalse);assert(actionKillCalls==2&&actionKillMod==MOD_FALLING);players[0].noTools=1;ControllerShot(p,kill->s.number,qfalse);assert(actionKillCalls==2);players[0].noTools=0;
 Command(0,"jar_property","controller_flags_0");ControllerShot(p,kill->s.number,qfalse);assert(actionKillCalls==3&&actionKillMod==MOD_UNKNOWN);Command(0,"jar_property","controller_flags_1");actionKillCalls=2;
 // Delay retains the activating connection; a reused slot receives no action.
 ControllerLink(p,delay->s.number,qfalse);ControllerLink(p,kill->s.number,qfalse);ControllerShot(p,delay->s.number,qfalse);++controllerClientEpoch[0];ControllerTick(delay,1000);assert(actionKillCalls==2);
 // State survives an independent clipboard, deletion, JRW14 and server restore.
 CopyAssembly(p,give->s.number);assert(copies[0].count==3&&CopyControllersValid(&copies[0]));
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 int consumed=0;for(int i=0;i<saved.copy.count;++i)consumed+=saved.copy.node[i].itemConsumed;assert(consumed==2);
 decoded.copy.node[0].itemConsumed=2;assert(!WorldValid(&decoded));assert(WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));consumed=0;give=NULL;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse){if(objects[n].itemConsumed){++consumed;assert(g_entities[n].item&&!g_entities[n].touch&&!g_entities[n].nextthink&&!g_entities[n].r.linked);}if(ControllerKind(n)==16)give=&g_entities[n];if(ControllerKind(n)==17)assert(g_entities[n].count==-50);if(ControllerKind(n)==18)assert(g_entities[n].spawnflags==1);}
 assert(consumed==2&&give);worldEditing.integer=1;ControllerShot(p,give->s.number,qfalse);assert(pickupEffects==2);
 assert(WorldClear()&&PlaceCopy(p,&copies[0],(float[3]){1500,0,80},NULL,NULL));int pasted=players[0].selected;ControllerShot(p,pasted,qfalse);assert(pickupEffects==2);Undo(p,qfalse);assert(!g_entities[pasted].inuse);
 PhysicsReset();puts("MP actions: native pickup dispatch, typed links, live permissions, signed score/context Undo, kill modes/consent, connection reuse, consumed-item JRW14/copy/cleanup passed.");
}
