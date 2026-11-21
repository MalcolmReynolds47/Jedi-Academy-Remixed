static int controllerActivator;
static void ControllerUseSpy(gentity_t*self,gentity_t*other,gentity_t*activator){controllerActivator=activator->s.number;}
static unsigned ControllerId(const char*name){for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==5&&!strcmp(jarMPCatalog[i].name,name))return jarMPCatalog[i].id;assert(0);return 0;}
static gentity_t*ControllerAt(const char*name,int owner,float x){vec3_t point={x,0,80},angle={0,0,0};return SpawnAt(&g_entities[owner],ControllerId(name),point,angle);}
static void ControllerTick(gentity_t*e,int milliseconds){level.time+=milliseconds;if(e->nextthink>0&&e->nextthink<=level.time){e->nextthink=0;e->think(e);}}
static void ControllerShot(gentity_t*p,int n,qboolean alt){level.time+=110;ControllerActivate(p,n,alt);}
static void StaticBrushCases(void){
 const char* classes[]={"func_static","func_group"};
 for(int i=0;i<ARRAY_LEN(classes);++i){
  RigidFixture();imports.SendServerCommand=Send;controllerPrints=0;
  gentity_t*p=&g_entities[0];const unsigned id=ControllerId(classes[i]);
  aimed=ENTITYNUM_WORLD;gentity_t*brush=Spawn(p,id);assert(brush);
  const int n=brush->s.number;
  assert(ControllerKind(n)==JAR_STATIC_BRUSH_CONTROLLER&&PhysicsSolid(n)&&objects[n].frozen);
  assert(brush->use==StaticBrushUse&&(brush->r.svFlags&SVF_PLAYER_USABLE)&&LiveCount()==1);
  gentity_t*notice=ControllerAt("target_print",0,700);const int target=notice->s.number;
  players[0].linkOutput=1;ControllerLink(p,n,qfalse);ControllerLink(p,target,qfalse);assert(!objects[n].output[1]);
  players[0].linkOutput=0;ControllerLink(p,n,qfalse);ControllerLink(p,target,qfalse);assert(objects[n].output[0]==target+1);
  void (*const noticeUse)(gentity_t*,gentity_t*,gentity_t*)=notice->use;
  notice->use=ControllerUseSpy;controllerActivator=-1;
  brush->use(brush,p,&g_entities[1]);assert(controllerActivator==1);
  ControllerShot(p,n,qfalse);assert(controllerActivator==0);
  aimed=n;Command(0,"jar_select","");Command(0,"jar_property","controller_enabled");
  assert(brush->flags&FL_INACTIVE);controllerActivator=-1;ControllerShot(p,n,qfalse);assert(controllerActivator==-1);
  Command(0,"jar_property","property_undo");assert(!(brush->flags&FL_INACTIVE));
  notice->use=noticeUse;CopyAssembly(p,n);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
  static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];
  assert(WorldCapture(&world));int length=WorldEncode(&world,bytes);assert(WorldDecode(bytes,length,&decoded));
  assert(WorldClear()&&PlaceCopy(p,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
  const int restored=players[0].selected;assert(ControllerKind(restored)==JAR_STATIC_BRUSH_CONTROLLER&&objects[restored].output[0]);
  ControllerShot(p,restored,qfalse);assert(controllerPrints==1);
  Undo(p,qfalse);assert(!LiveCount());
 }
 PhysicsReset();puts("MP static brush and brush group: direct spawn, collision, activation, Context, linked copy, world codec and Undo passed.");
}
static void ControllerCases(void){
 RigidFixture();imports.SendServerCommand=Send;controllerPrints=0;gentity_t*p=&g_entities[0];gentity_t*relay=ControllerAt("target_relay",0,100),*print=ControllerAt("target_print",0,300),*counter=ControllerAt("target_counter",0,500),*delay=ControllerAt("target_delay",0,700);
 assert(relay&&print&&counter&&delay&&objects[relay->s.number].frozen&&PhysicsProp(relay->s.number)&&relay->use==target_relay_use&&counter->count==2&&delay->wait==1);
 int r=relay->s.number,t=print->s.number,c=counter->s.number,d=delay->s.number;
 ControllerLink(p,r,qfalse);assert(players[0].first==r);ControllerLink(p,t,qfalse);assert(objects[r].output[0]==t+1);
 ControllerShot(p,r,qfalse);assert(controllerPrints==1);ControllerShot(p,r,qtrue);assert(controllerPrints==1&&players[0].selected==r);
 ControllerLink(&g_entities[1],r,qfalse);assert(players[1].first==ENTITYNUM_NONE);
 ControllerLink(p,c,qfalse);ControllerLink(p,t,qfalse);ControllerShot(p,c,qfalse);assert(counter->count==1&&controllerPrints==1);ControllerShot(p,c,qfalse);assert(counter->count==0&&controllerPrints==2);
 // Secondary output fires while a counter still has outstanding activations.
 counter->count=2;Command(0,"jar_setting","output_secondary");ControllerLink(p,c,qfalse);ControllerLink(p,t,qfalse);ControllerShot(p,c,qfalse);assert(counter->count==1&&controllerPrints==3);assert(objects[c].output[1]==t+1);
 Command(0,"jar_setting","output_primary");ControllerLink(p,d,qfalse);ControllerLink(p,r,qfalse);ControllerShot(p,d,qfalse);assert(delay->nextthink==level.time+1000);ControllerTick(delay,999);assert(controllerPrints==3);ControllerTick(delay,1);assert(controllerPrints==4);
 // Cycles are rejected through both outputs, and map entities cannot be rewired.
 ControllerLink(p,r,qfalse);ControllerLink(p,d,qfalse);assert(objects[r].output[0]==t+1);gentity_t*map=G_Spawn();map->classname="target_relay";ControllerLink(p,map->s.number,qfalse);assert(players[0].first==ENTITYNUM_NONE);
 ControllerLink(p,r,qfalse);ControllerLink(p,t,qtrue);assert(players[0].first==ENTITYNUM_NONE);
 // Logical links join an assembly from either end; immutable capture uses indices.
 ControllerShot(p,d,qfalse);ControllerTick(delay,300);CopyAssembly(p,t);assert(copies[0].count==4&&CopyControllersValid(&copies[0]));int pending=0;for(int i=0;i<4;++i)if(copies[0].node[i].controllerRemaining==700)++pending;assert(pending==1);
 static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&world));int size=WorldEncode(&world,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 // Recomputed checksums do not admit dangling/self links, cycles or bad timers.
 const int offsets[]={156,164,168,172,176};const unsigned invalid[]={MAX_GENTITIES+1,0x80000000u,0x80000000u,0x80000000u,2};
 for(int i=0;i<5;++i){memcpy(bad,bytes,size);at=WORLD_HEADER+offsets[i];WorldPut(bad,&at,invalid[i]);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);}
 // Deletion severs incoming references before the slot can become a new target.
 G_FreeEntity(print);assert(!objects[r].output[0]&&!objects[c].output[0]&&!objects[c].output[1]);print=ControllerAt("target_print",0,300);assert(print->s.number==t);ControllerShot(p,r,qfalse);assert(controllerPrints==4);
 assert(WorldClear());assert(PlaceCopy(NULL,&world.copy,vec3_origin,world.velocity,world.angular));gentity_t*loadedDelay=NULL;int loadedCounter=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==3)loadedDelay=&g_entities[n];else if(ControllerKind(n)==4)loadedCounter=n;
 assert(loadedDelay&&loadedCounter>=0&&g_entities[loadedCounter].count==1&&objects[loadedCounter].output[1]);ControllerTick(loadedDelay,699);assert(controllerPrints==4);ControllerTick(loadedDelay,1);assert(controllerPrints==5);
 // Native use dispatcher keeps authored entities outside the Sandbox route.
 assert(!JAR_MP_UseTargets(map,p,"ordinary_map_target"));
 assert(WorldClear());assert(PlaceCopy(p,&copies[0],(float[3]){1000,0,80},NULL,NULL));int root=players[0].selected;assert(root>=MAX_CLIENTS);Undo(p,qfalse);assert(!g_entities[root].inuse);
 // Normal menu spawning provides a feedback target with one Undo group.
 aimed=ENTITYNUM_WORLD;gentity_t*spawned=Spawn(p,ControllerId("target_delay"));assert(spawned&&objects[spawned->s.number].output[0]);int child=objects[spawned->s.number].output[0]-1;assert(objects[child].order==objects[spawned->s.number].order);ControllerShot(p,spawned->s.number,qfalse);ControllerTick(spawned,1000);assert(controllerPrints==6);Undo(p,qfalse);assert(!spawned->inuse&&!g_entities[child].inuse);
 // Invalid graph data is rejected before a paste can allocate any entities.
 int live=LiveCount();static jarCopy_t invalidGraph;invalidGraph=world.copy;invalidGraph.node[0].output[0]=4;assert(!CopyControllersValid(&invalidGraph)&&!PlaceCopy(p,&invalidGraph,vec3_origin,NULL,NULL)&&LiveCount()==live);
 // Shared editing can be revoked after linking; subsequent activations recheck it.
 relay=ControllerAt("target_relay",0,100);print=ControllerAt("target_print",1,300);sharedEditing.integer=1;ControllerLink(p,relay->s.number,qfalse);ControllerLink(p,print->s.number,qfalse);sharedEditing.integer=0;int printed=controllerPrints;ControllerShot(p,relay->s.number,qfalse);assert(controllerPrints==printed);
 // Retained delayed events cannot impersonate a new occupant of a client slot.
 delay=ControllerAt("target_delay",0,500);gentity_t*ownedPrint=ControllerAt("target_print",0,700);ControllerLink(p,delay->s.number,qfalse);ControllerLink(p,ownedPrint->s.number,qfalse);ControllerShot(p,delay->s.number,qfalse);ownedPrint->use=ControllerUseSpy;cleanupDisconnect.integer=0;JAR_MP_Disconnect(0);controllerActivator=-1;ControllerTick(delay,1000);assert(controllerActivator==delay->s.number);
 PhysicsReset();puts("MP native controllers: real callbacks, primary/secondary links, delayed use, cycle/ownership rejection, deletion/reuse, immutable duplication and JRW7 restore passed.");
}
