// Native Timer/Random callbacks through actual server tools, context and saves.
static void AlwaysControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 gentity_t *owner=&g_entities[0];
 gentity_t *shot=SpawnWithController(owner,92295601u,(vec3_t){100,0,80},vec3_origin);
 assert(shot&&ControllerKind(shot->s.number)==JAR_ALWAYS_CONTROLLER&&shot->nextthink==level.time+300);
 gentity_t *output=&g_entities[objects[shot->s.number].output[0]-1];
 assert(output->inuse&&ControllerKind(output->s.number)==1);
 assert(shot->s.bolt2==JAR_MP_LOGIC_MARKER_TAG&&output->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 assert(shot->r.contents==CONTENTS_SOLID&&PhysicsSolid(shot->s.number));
 ControllerTick(shot,299);assert(!controllerPrints);
 ControllerTick(shot,1);assert(controllerPrints==1&&shot->inuse&&!shot->nextthink);
 ControllerSelect(shot);Command(0,"jar_property","controller_delay_0");assert(shot->delay==300);
 Command(0,"jar_property","controller_delay_800");assert(shot->delay==800);
 Command(0,"jar_property","property_undo");assert(shot->delay==300);
 Command(0,"jar_property","controller_delay_800");
 ControllerShot(owner,shot->s.number,qfalse);const int due=shot->nextthink;
 assert(due==level.time+800);ControllerTick(shot,90);ControllerActivate(owner,shot->s.number,qfalse);assert(shot->nextthink==due);
 CopyAssembly(owner,shot->s.number);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 int index=0;while(index<decoded.copy.count&&decoded.copy.node[index].id!=92295601u)++index;
 assert(index<decoded.copy.count&&decoded.copy.node[index].controllerRemaining==710);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 shot=NULL;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==JAR_ALWAYS_CONTROLLER)shot=&g_entities[n];
 assert(shot&&shot->delay==800&&shot->nextthink==level.time+710&&shot->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 const int printed=controllerPrints;
 ControllerTick(shot,709);assert(controllerPrints==printed);
 ControllerTick(shot,1);assert(controllerPrints==printed+1&&shot->inuse&&!shot->nextthink);
 assert(WorldCapture(&saved));size_t shots=0;
 for(int i=0;i<saved.copy.count;++i)if(saved.copy.node[i].id==92295601u){++shots;assert(!saved.copy.node[i].controller.active&&!saved.copy.node[i].controllerRemaining);}
 assert(shots==1&&WorldClear());
 assert(PlaceCopy(owner,&copies[0],(vec3_t){500,0,80},NULL,NULL));
 shot=NULL;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==JAR_ALWAYS_CONTROLLER)shot=&g_entities[n];
 assert(shot&&shot->nextthink==level.time+710);
 ControllerSelect(shot);Command(0,"jar_property","controller_enabled");
 assert(!shot->nextthink&&(shot->flags&FL_INACTIVE));ControllerShot(owner,shot->s.number,qfalse);assert(!shot->nextthink);
 Undo(owner,qfalse);assert(!shot->inuse&&WorldClear());
 puts("MP one-shot trigger: automatic output, rearm, delay Undo, pending and spent state, clipboard, journal and cleanup passed.");
}

static int visibleOccluded,visiblePVS;
static qboolean VisibleFixturePVS(const vec3_t eye,const vec3_t marker){return visiblePVS?qtrue:qfalse;}
static void VisibleFixtureTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,
 const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mask==MASK_OPAQUE&&!mins){t->entityNum=ENTITYNUM_WORLD;t->fraction=visibleOccluded?.5f:1.f;}
}

static void VisibleControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 qboolean (*oldPVS)(const vec3_t,const vec3_t)=imports.InPVS;
 imports.InPVS=VisibleFixturePVS;imports.Trace=VisibleFixtureTrace;
 level.numConnectedClients=3;level.maxclients=3;
 visiblePVS=1;visibleOccluded=0;
 gentity_t*owner=&g_entities[0];
 gentity_t*marker=SpawnWithController(owner,94776726u,(vec3_t){100,0,24},vec3_origin);
 assert(marker&&ControllerKind(marker->s.number)==JAR_VISIBLE_CONTROLLER);
 const int n=marker->s.number,output=objects[n].output[0]-1;
 assert(ControllerKind(output)==1&&marker->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 assert(marker->r.contents==CONTENTS_ITEM&&!PhysicsSolid(n));
 trace_t pick;
 PhysgunAim(owner,&pick);
 assert(pick.entityNum==n&&pick.contents==CONTENTS_ITEM);
 clients[0].ps.viewangles[YAW]=180;
 clients[2].sess.sessionTeam=TEAM_SPECTATOR;
 VisibleFrame();assert(!controllerPrints);
 level.time+=JAR_VISIBLE_SAMPLE_MS;visiblePVS=0;VisibleFrame();assert(!controllerPrints);
 level.time+=JAR_VISIBLE_SAMPLE_MS;visiblePVS=1;visibleOccluded=1;VisibleFrame();assert(!controllerPrints);
 ControllerSelect(marker);
 Command(0,"jar_property","controller_visible_range_0");assert(marker->speed==512);
 Command(0,"jar_property","controller_visible_range_4096");assert(marker->speed==4096);
 Command(0,"jar_property","property_undo");assert(marker->speed==512);
 Command(0,"jar_property","controller_visible_range_150");
 Command(0,"jar_property","controller_visible_cone_25");
 Command(0,"jar_property","controller_visible_walls");
 Command(0,"jar_property","controller_visible_sight");
 level.time+=JAR_VISIBLE_SAMPLE_MS;VisibleFrame();assert(!controllerPrints&&marker->genericValue1);
 clients[1].ps.fd.forcePowersActive=1<<FP_SEE;
 Command(0,"jar_property","controller_enabled");
 level.time+=JAR_VISIBLE_SAMPLE_MS;VisibleFrame();assert(!controllerPrints&&marker->genericValue1);
 Command(0,"jar_property","controller_enabled");VisibleFrame();
 assert(controllerPrints==1&&marker->inuse&&!marker->genericValue1&&objects[n].activationClient==2);
 level.time+=JAR_VISIBLE_SAMPLE_MS;VisibleFrame();assert(controllerPrints==1);
 CopyAssembly(owner,n);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
 assert(!copies[0].node[0].controller.active&&copies[0].node[0].controller.damage==25);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);
 assert(size&&WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 marker=NULL;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_VISIBLE_CONTROLLER)marker=&g_entities[i];
 assert(marker&&!marker->genericValue1&&marker->speed==150&&marker->damage==25&&marker->spawnflags==3);
 level.time+=JAR_VISIBLE_SAMPLE_MS;VisibleFrame();assert(controllerPrints==1);
 sharedEditing.integer=worldEditing.integer=1;
 ControllerActivate(owner,marker->s.number,qfalse);assert(marker->genericValue1);
 level.time+=JAR_VISIBLE_SAMPLE_MS;VisibleFrame();assert(controllerPrints==2&&!marker->genericValue1);
 assert(WorldClear());
 imports.InPVS=oldPVS;imports.Trace=RigidTrace;
 puts("MP visibility trigger: view, walls, PVS, Force Sight, spectator exclusion, activator, properties, copy and world restore passed.");
}

static void DistanceControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 level.maxclients=3;
 gentity_t*owner=&g_entities[0];
 clients[1].sess.sessionTeam=clients[2].sess.sessionTeam=TEAM_SPECTATOR;
 rackAllocationFailureCountdown=3;
 assert(!SpawnWithController(owner,JAR_MP_DISTANCE_ID,(vec3_t){100,0,80},vec3_origin));
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)assert(!g_entities[i].inuse);
 gentity_t*marker=SpawnWithController(owner,JAR_MP_DISTANCE_ID,(vec3_t){100,0,80},vec3_origin);
 assert(marker&&marker->r.contents==CONTENTS_ITEM&&!PhysicsSolid(marker->s.number));
 const int n=marker->s.number,inside=objects[n].output[0]-1,outside=objects[n].output[1]-1;
 assert(ControllerKind(n)==JAR_DISTANCE_CONTROLLER&&marker->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 assert(ControllerKind(inside)==1&&ControllerKind(outside)==1&&objects[inside].order==objects[n].order&&objects[outside].order==objects[n].order);
 VectorSet(clients[0].ps.origin,0,0,80);
 ControllerShot(owner,n,qfalse);
 assert(controllerPrints==1&&strstr(printedMessage,"inside range")&&(marker->flags&FL_INACTIVE));
 ControllerShot(owner,n,qfalse);assert(controllerPrints==1);
 ControllerSelect(marker);
 Command(0,"jar_property","controller_distance_range_0");assert(marker->speed==192);
 Command(0,"jar_property","controller_distance_range_4096");assert(marker->speed==4096);
 Command(0,"jar_property","property_undo");assert(marker->speed==192);
 VectorSet(clients[0].ps.origin,-92,0,80); // Exactly 192 units away.
 Command(0,"jar_property","controller_enabled");ControllerShot(owner,n,qfalse);
 assert(controllerPrints==2&&strstr(printedMessage,"outside range"));

 gentity_t*reference=ControllerAt("target_position",0,400);
 gentity_t*npc=CopySpawn(2,0,449);CopyReady(npc);
 const int ref=reference->s.number,subject=npc->s.number;
 Move(reference,(vec3_t){400,0,80});
 Command(0,"jar_setting","output_reference");ControllerLink(owner,n,qfalse);ControllerLink(owner,ref,qfalse);
 Command(0,"jar_setting","output_subject");ControllerLink(owner,n,qfalse);ControllerLink(owner,ref,qfalse);
 assert(objects[n].distanceInput[0]==ref+1&&!objects[n].distanceInput[1]);
 ControllerLink(owner,n,qfalse);ControllerLink(owner,subject,qfalse);
 assert(objects[n].distanceInput[1]==subject+1);
 Command(0,"jar_property","controller_distance_clear_subject");assert(!objects[n].distanceInput[1]);
 Command(0,"jar_property","property_undo");assert(objects[n].distanceInput[1]==subject+1);
 Command(0,"jar_property","controller_distance_range_50");
 Command(0,"jar_property","controller_distance_subjects");assert(marker->spawnflags==JAR_DISTANCE_NPC);
 Command(0,"jar_property","controller_enabled");ControllerShot(owner,n,qfalse);
 assert(controllerPrints==3&&strstr(printedMessage,"inside range"));
 Move(npc,(vec3_t){451,0,80});
 Command(0,"jar_property","controller_enabled");ControllerShot(owner,n,qfalse);
 assert(controllerPrints==4&&strstr(printedMessage,"outside range"));

 CopyAssembly(owner,n);
 assert(copies[0].count==5&&copies[0].node[0].controllerCount>0&&copies[0].node[0].controllerInitial>0);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 int index=0;while(index<decoded.copy.count&&decoded.copy.node[index].id!=JAR_MP_DISTANCE_ID)++index;
 assert(index<decoded.copy.count&&decoded.copy.node[index].controller.inactive);
 jarCopyNode_t*node=&decoded.copy.node[index];
 const int previousSubject=node->controllerInitial;
 node->controllerInitial=node->controllerCount;assert(!CopyControllersValid(&decoded.copy));
 node->controllerInitial=previousSubject;assert(CopyControllersValid(&decoded.copy));
 assert(PlaceCopy(owner,&copies[0],(vec3_t){800,100,80},NULL,NULL));
 const int pasted=players[0].selected;
 assert(pasted!=n&&objects[pasted].distanceInput[0]!=ref+1&&objects[pasted].distanceInput[1]!=subject+1);
 assert(g_entities[pasted].flags&FL_INACTIVE);
 CopyReady(&g_entities[objects[pasted].distanceInput[1]-1]);
 Undo(owner,qfalse);assert(!g_entities[pasted].inuse&&marker->inuse&&npc->inuse);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 marker=NULL;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_DISTANCE_CONTROLLER)marker=&g_entities[i];
 assert(marker&&marker->speed==50&&marker->spawnflags==JAR_DISTANCE_NPC&&(marker->flags&FL_INACTIVE));
 const int loaded=marker->s.number;
 assert(objects[loaded].distanceInput[0]>0&&objects[loaded].distanceInput[1]>0);
 CopyReady(&g_entities[objects[loaded].distanceInput[1]-1]);
 worldEditing.integer=1;ControllerSelect(marker);Command(0,"jar_property","controller_enabled");ControllerShot(owner,loaded,qfalse);
 assert(controllerPrints==5&&strstr(printedMessage,"outside range"));
 G_FreeEntity(&g_entities[objects[loaded].distanceInput[0]-1]);
 assert(objects[loaded].distanceInput[0]==JAR_DISTANCE_LINK_MISSING);
 Command(0,"jar_property","controller_enabled");ControllerShot(owner,loaded,qfalse);
 assert(controllerPrints==5&&!(marker->flags&FL_INACTIVE));
 assert(WorldCapture(&saved)&&WorldEncode(&saved,bytes)>0);
 assert(WorldClear());
 puts("MP distance trigger: native subject selection, both outputs, links, Undo, boundary, missing input and journal passed.");
}

static void WallControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;
 gentity_t*owner=&g_entities[0];
 gentity_t*wall=SpawnWithController(owner,JAR_WALL_CATALOG_ID,(vec3_t){120,0,80},vec3_origin);
 assert(wall&&wall->r.contents==CONTENTS_SOLID&&!strcmp(wall->model,JAR_WALL_MODEL));
 int n=wall->s.number;
 assert(ControllerKind(n)==JAR_WALL_CONTROLLER&&wall->genericValue1&&PhysicsSolid(n)&&!wall->s.bolt2);
 ControllerSelect(wall);Command(0,"jar_property","controller_wall_visible");
 assert(!wall->genericValue1&&wall->r.contents==CONTENTS_ITEM&&wall->s.bolt2==JAR_MP_WALL_OFF_TAG&&BodyDormant(n)&&!PhysicsSolid(n));
 blocked=1;
 Command(0,"jar_property","property_undo");assert(!wall->genericValue1&&ContextUndoReady(owner,n));
 blocked=0;Command(0,"jar_property","property_undo");assert(wall->genericValue1&&PhysicsSolid(n));
 ControllerShot(owner,n,qfalse);assert(!wall->genericValue1&&!PhysicsSolid(n));
 Command(0,"jar_property","controller_enabled");
 ControllerShot(owner,n,qfalse);assert(!wall->genericValue1&&(wall->flags&FL_INACTIVE));
 CopyAssembly(owner,n);assert(copies[0].count==1&&!copies[0].node[0].controller.active);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){500,0,80},NULL,NULL));
 const int pasted=players[0].selected;
 assert(pasted!=n&&BodyDormant(pasted)&&g_entities[pasted].r.mins[0]==-16);
 Undo(owner,qfalse);assert(!g_entities[pasted].inuse&&wall->inuse);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);
 assert(size&&WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_WALL_CONTROLLER){
  wall=&g_entities[i];n=i;break;
 }
 assert(wall->inuse&&BodyDormant(n)&&(wall->flags&FL_INACTIVE)&&!wall->genericValue1);
 worldEditing.integer=1;ControllerSelect(wall);Command(0,"jar_property","controller_enabled");
 ControllerShot(owner,n,qfalse);assert(wall->genericValue1&&PhysicsSolid(n)&&wall->r.mins[0]==-12);
 assert(WorldClear());
 puts("MP wall: stock geometry, solid and off states, obstruction, Undo, activation, clipboard and journal passed.");
}

static void SlidingDoorTick(gentity_t*door,int ms){
 level.previousTime=level.time;
 level.time+=ms;
 SlidingDoorFrame(door);
}

static void SlidingDoorCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 gentity_t*owner=&g_entities[0];
 gentity_t*door=SpawnWithController(owner,JAR_SLIDING_DOOR_ID,(vec3_t){120,0,80},vec3_origin);
 assert(door&&door->s.bolt2==JAR_MP_SLIDING_DOOR_TAG&&door->r.contents==CONTENTS_SOLID);
 int n=door->s.number;
 assert(ControllerKind(n)==JAR_SLIDING_DOOR_CONTROLLER&&objects[n].frozen&&PhysicsSolid(n));
 gentity_t*opened=ControllerAt("target_print",0,480),*closed=ControllerAt("target_print",0,620);
 ControllerLink(owner,n,qfalse);ControllerLink(owner,opened->s.number,qfalse);
 Command(0,"jar_setting","output_secondary");ControllerLink(owner,n,qfalse);ControllerLink(owner,closed->s.number,qfalse);
 assert(objects[n].output[0]==opened->s.number+1&&objects[n].output[1]==closed->s.number+1);
 ControllerSelect(door);
 Command(0,"jar_property","controller_slide_travel_0");assert(door->genericValue3==JAR_SLIDE_TRAVEL);
 Command(0,"jar_property","controller_slide_travel_60");
 Command(0,"jar_property","controller_slide_wait_250");
 ControllerShot(owner,n,qfalse);
 SlidingDoorTick(door,100);
 assert(door->genericValue6==10000&&door->r.currentOrigin[2]==90&&door->s.loopSound);
 Command(0,"jar_property","controller_slide_speed_200");SlidingDoorTick(door,100);
 assert(door->genericValue6==30000&&door->r.currentOrigin[2]==110);
 Command(0,"jar_property","property_undo");assert(door->speed==100&&door->genericValue6==30000);
 Command(0,"jar_property","controller_slide_direction");assert(door->genericValue2==JAR_SLIDE_UP);
 blocked=1;SlidingDoorTick(door,100);
 assert(door->genericValue6==30000&&door->genericValue5==JAR_SLIDE_BLOCKED&&!door->s.loopSound);
 blocked=0;
 for(int i=0;i<3;++i)SlidingDoorTick(door,100);
 assert(door->genericValue6==60000&&door->genericValue4==250&&controllerPrints==1);
 SlidingDoorTick(door,100);assert(door->genericValue4==150);
 CopyAssembly(owner,n);assert(copies[0].count==3&&copies[0].node[0].controllerRemaining==150);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 int index=0;while(index<decoded.copy.count&&decoded.copy.node[index].id!=JAR_SLIDING_DOOR_ID)++index;
 assert(index<decoded.copy.count&&decoded.copy.node[index].controller.pushSpeed==60000&&decoded.copy.node[index].controllerRemaining==150);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 door=NULL;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_SLIDING_DOOR_CONTROLLER)door=&g_entities[i];
 assert(door&&door->r.currentOrigin[2]==140&&door->genericValue4==150);
 SlidingDoorTick(door,100);assert(door->genericValue4==50&&door->genericValue6==60000);
 blocked=1;SlidingDoorTick(door,50);
 assert(!door->genericValue1&&door->genericValue6==60000&&door->genericValue5==JAR_SLIDE_BLOCKED);
 blocked=0;
 for(int i=0;i<6;++i){level.previousTime=level.time;level.time+=100;JAR_MP_Frame();}
 assert(door->genericValue6==0&&fabsf(door->r.currentOrigin[2]-80)<.001f&&controllerPrints==2);
 assert(WorldClear());
 door=SpawnWithController(owner,JAR_SLIDING_DOOR_ID,(vec3_t){120,0,80},vec3_origin);
 assert(door);
 ControllerSelect(door);
 Command(0,"jar_property","controller_slide_travel_60");
 Command(0,"jar_property","controller_slide_wait_0");
 ControllerShot(owner,door->s.number,qfalse);
 for(int i=0;i<2;++i)SlidingDoorTick(door,100);
 assert(door->genericValue6==20000&&door->s.loopSound);
 Command(0,"jar_property","controller_slide_travel_20");
 SlidingDoorTick(door,100);
 assert(door->genericValue6==20000&&door->genericValue5==JAR_SLIDE_REST&&!door->s.loopSound);
 assert(WorldClear());
 puts("MP sliding door: native hull, both outputs, obstruction retry, Context Undo, exact-travel sound stop, auto-close and world restore passed.");
}

static int liftCeilingActor=-1;
static int bobFloorDeck=-1;
static float bobFloorZ;
static void LiftTraceFixture(trace_t* t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,
                             int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mins&&start[2]!=end[2]&&pass==liftCeilingActor)t->fraction=.5f;
 if(mins&&pass==bobFloorDeck&&end[2]<start[2]&&end[2]<bobFloorZ){
  t->fraction=.5f;t->entityNum=ENTITYNUM_WORLD;VectorSet(t->plane.normal,0,0,1);
 }
}
static void LiftCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;imports.EntitiesInBox=RigidNearbyBoxes;
 imports.Trace=LiftTraceFixture;controllerPrints=0;
 gentity_t* owner=&g_entities[0],*rider=&g_entities[1];
 playerTools.integer=1;sharedEditing.integer=1;
 gentity_t* deck=SpawnWithController(owner,JAR_LIFT_ID,(vec3_t){120,0,80},vec3_origin);
 assert(deck&&deck->s.bolt2==JAR_MP_LIFT_TAG&&ControllerKind(deck->s.number)==JAR_LIFT_CONTROLLER);
 const int n=deck->s.number;assert(deck->genericValue3==128&&deck->wait==1000);
 gentity_t* cargo=SpawnAt(owner,1001,(vec3_t){136,0,96},vec3_origin);assert(cargo);
 objects[cargo->s.number].frozen=0;
 VectorSet(rider->r.mins,-4,-4,-8);VectorSet(rider->r.maxs,4,4,8);
 rider->r.contents=CONTENTS_BODY;
 Move(rider,(vec3_t){115,0,100});rider->client->ps.groundEntityNum=n;
 gentity_t* raised=ControllerAt("target_print",0,480),*lowered=ControllerAt("target_print",0,620);
 Command(0,"jar_setting","output_primary");ControllerLink(owner,n,qfalse);ControllerLink(owner,raised->s.number,qfalse);
 Command(0,"jar_setting","output_secondary");ControllerLink(owner,n,qfalse);ControllerLink(owner,lowered->s.number,qfalse);
 ControllerSelect(deck);
 Command(0,"jar_property","controller_slide_travel_30");
 Command(0,"jar_property","controller_slide_speed_50");
 Command(0,"jar_property","property_undo");assert(deck->speed==100);
 Command(0,"jar_property","controller_slide_wait_250");
 ControllerShot(owner,n,qfalse);SlidingDoorTick(deck,100);
 assert(deck->r.currentOrigin[2]==90&&rider->client->ps.origin[2]==110&&cargo->r.currentOrigin[2]==106);
 liftCeilingActor=rider->s.number;SlidingDoorTick(deck,100);
 assert(deck->genericValue5==JAR_SLIDE_BLOCKED&&deck->genericValue6==10000);
 assert(rider->r.currentOrigin[2]==110&&cargo->r.currentOrigin[2]==106&&deck->r.linked&&rider->r.linked&&cargo->r.linked);
 liftCeilingActor=-1;players[1].noTools=1;SlidingDoorTick(deck,100);
 assert(deck->genericValue6==10000);
 players[1].noTools=0;SlidingDoorTick(deck,100);SlidingDoorTick(deck,100);
 assert(deck->genericValue6==30000&&deck->genericValue4==250&&rider->client->ps.origin[2]==130&&controllerPrints==1);
 SlidingDoorTick(deck,100);assert(deck->genericValue4==150);
 static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world));const int size=WorldEncode(&world,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 int index=0;while(index<decoded.copy.count&&decoded.copy.node[index].id!=JAR_LIFT_ID)++index;
 assert(index<decoded.copy.count&&decoded.copy.node[index].controller.pushSpeed==30000&&decoded.copy.node[index].controllerRemaining==150);
 ControllerShot(owner,n,qfalse);
 for(int i=0;i<3;++i)SlidingDoorTick(deck,100);
 assert(deck->genericValue6==0&&rider->client->ps.origin[2]==100&&cargo->r.currentOrigin[2]==96&&controllerPrints==2);
 assert(WorldClear());imports.Trace=RigidTrace;
 puts("MP lift: editable motion, both endpoint links, atomic actor/prop carriage, obstruction and consent rollback, and saved progress passed.");
}

static void BobbingCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;imports.EntitiesInBox=RigidNearbyBoxes;
 imports.Trace=LiftTraceFixture;playerTools.integer=1;sharedEditing.integer=1;
 gentity_t* owner=&g_entities[0],*rider=&g_entities[1];
 gentity_t* deck=SpawnWithController(owner,JAR_MP_BOBBING_ID,(vec3_t){120,0,80},vec3_origin);
 assert(deck&&deck->s.bolt2==JAR_MP_BOBBING_TAG&&deck->r.contents==CONTENTS_SOLID);
 const int n=deck->s.number;
 assert(ControllerKind(n)==JAR_BOBBING_CONTROLLER&&objects[n].frozen&&deck->genericValue2==JAR_BOB_BOTTOM);
 gentity_t* cargo=SpawnAt(owner,1001,(vec3_t){130,0,150},vec3_origin);assert(cargo);
 const float top=deck->r.currentOrigin[2]+deck->r.maxs[2];
 Move(cargo,(vec3_t){130,0,top-cargo->r.mins[2]});
 objects[cargo->s.number].frozen=0;
 VectorSet(rider->r.mins,-4,-4,-8);VectorSet(rider->r.maxs,4,4,8);
 rider->r.contents=CONTENTS_BODY;
 Move(rider,(vec3_t){112,0,top+8});rider->client->ps.groundEntityNum=n;
 const float riderZ=rider->r.currentOrigin[2],cargoZ=cargo->r.currentOrigin[2];
 ControllerSelect(deck);
 assert(!(ContextCaps(owner,n)&JAR_CONTEXT_POSE));
 Command(0,"jar_property","controller_bob_period_0");assert(deck->delay==JAR_BOB_PERIOD);
 Command(0,"jar_property","controller_bob_period_1000");
 Command(0,"jar_property","controller_bob_amplitude_20");
 for(int i=0;i<2;++i){level.time+=100;BobbingFrame(deck);}
 const float raised=deck->r.currentOrigin[2]-80;
 assert(raised>13&&fabsf(rider->client->ps.origin[2]-riderZ-raised)<.001f&&fabsf(cargo->r.currentOrigin[2]-cargoZ-raised)<.001f);
 const int phase=deck->genericValue2;
 liftCeilingActor=rider->s.number;level.time+=100;BobbingFrame(deck);
 assert(deck->genericValue3==JAR_BOB_BLOCKED&&deck->genericValue2==phase&&deck->r.currentOrigin[2]==80+raised);
 assert(deck->r.linked&&rider->r.linked&&cargo->r.linked);
 liftCeilingActor=-1;players[1].noTools=1;level.time+=100;BobbingFrame(deck);
 assert(deck->genericValue2==phase);
 players[1].noTools=0;
 Command(0,"jar_property","property_undo");
 assert(deck->speed==JAR_BOB_AMPLITUDE&&deck->genericValue2==phase&&deck->r.currentOrigin[2]==80+raised);
 level.time+=100;BobbingFrame(deck);assert(deck->genericValue2!=phase&&deck->genericValue3==JAR_BOB_MOVING);
 ControllerShot(owner,n,qfalse);level.time+=100;BobbingFrame(deck);
 assert(deck->genericValue3==JAR_BOB_STOPPED&&!deck->genericValue1);
 const int parkedPhase=deck->genericValue2;
 const float parkedZ=deck->r.currentOrigin[2];
 static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world));const int size=WorldEncode(&world,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 CopyAssembly(owner,n);assert(copies[0].count==1);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){430,0,100},NULL,NULL));
 const int duplicate=players[0].selected;
 assert(duplicate!=n&&g_entities[duplicate].genericValue2==parkedPhase&&!g_entities[duplicate].genericValue1);
 Undo(owner,qfalse);assert(!g_entities[duplicate].inuse);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 deck=&g_entities[n];assert(ControllerKind(n)==JAR_BOBBING_CONTROLLER&&deck->genericValue2==parkedPhase&&deck->r.currentOrigin[2]==parkedZ);
 level.time+=100;BobbingFrame(deck);assert(deck->r.currentOrigin[2]==parkedZ);
 worldEditing.integer=1;
 ControllerShot(owner,n,qfalse);level.time+=100;BobbingFrame(deck);assert(deck->genericValue2!=parkedPhase);
 deck->genericValue2=JAR_BOB_CYCLE/2;deck->genericValue5=0;
 const float floorPose=deck->r.currentOrigin[2];
 bobFloorDeck=n;bobFloorZ=floorPose-.25f;
 level.time+=100;BobbingFrame(deck);
 assert(deck->genericValue2==JAR_BOB_BOTTOM&&deck->r.currentOrigin[2]==floorPose);
 level.time+=100;BobbingFrame(deck);
 assert(deck->genericValue3==JAR_BOB_MOVING&&deck->r.currentOrigin[2]>floorPose);
 bobFloorDeck=-1;
 assert(WorldClear());imports.Trace=RigidTrace;
 puts("MP bobbing platform: periodic rider/cargo motion, obstruction and consent, Context undo, phase-preserving copies and journal passed.");
}

static void AngularMoverCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;imports.EntitiesInBox=RigidNearbyBoxes;
 imports.Trace=LiftTraceFixture;playerTools.integer=1;sharedEditing.integer=1;
 gentity_t* owner=&g_entities[0],*rider=&g_entities[1];
 gentity_t* spinner=SpawnWithController(owner,JAR_MP_SPINNER_ID,(vec3_t){120,0,80},vec3_origin);
 gentity_t* pendulum=SpawnWithController(owner,JAR_MP_PENDULUM_ID,(vec3_t){300,0,80},vec3_origin);
 assert(spinner&&pendulum&&spinner->s.bolt2==JAR_MP_ANGULAR_TAG);
 const int n=spinner->s.number;
 assert(!spinner->genericValue1&&pendulum->genericValue1&&pendulum->genericValue2==JAR_ANGULAR_FORWARD);
 const float top=spinner->r.currentOrigin[2]+spinner->r.maxs[2];
 gentity_t* cargo=SpawnAt(owner,1001,(vec3_t){100,0,150},vec3_origin);assert(cargo);
 Move(cargo,(vec3_t){100,0,top-cargo->r.mins[2]});
 objects[cargo->s.number].frozen=0;
 VectorSet(rider->r.mins,-4,-4,-8);VectorSet(rider->r.maxs,4,4,8);rider->r.contents=CONTENTS_BODY;
 Move(rider,(vec3_t){132,0,top+8});rider->client->ps.groundEntityNum=n;
 ControllerSelect(spinner);
 assert(!(ContextCaps(owner,n)&JAR_CONTEXT_POSE));
 Command(0,"jar_property","controller_angular_rate_0");assert(spinner->speed==0);
 Command(0,"jar_property","controller_angular_rate_-120");assert(spinner->speed==-120);
 ControllerShot(owner,n,qfalse);
 pendulum->timestamp=level.time;
 assert(PhysicsPrepare());
 level.time+=50;AngularFrame(spinner);AngularFrame(pendulum);
 assert(fabsf(AngleSubtract(spinner->r.currentAngles[YAW],-6))<.01f);
 assert(rider->r.currentOrigin[1]<-1&&rider->client->ps.origin[1]==rider->r.currentOrigin[1]);
 assert(cargo->r.currentOrigin[1]>.8f&&AngleSubtract(cargo->r.currentAngles[YAW],0)<-5);
 jarMPPose_t rigid;
 vec3_t low,high;
 assert(JAR_RigidRead(cargo->s.number,&rigid,low,high)&&Distance(rigid.position,cargo->r.currentOrigin)<.001f);
 assert(pendulum->r.currentAngles[ROLL]>2&&pendulum->genericValue4==50);
 const float turn=spinner->r.currentAngles[YAW];
 players[1].noTools=1;level.time+=50;AngularFrame(spinner);
 assert(spinner->r.currentAngles[YAW]==turn&&spinner->genericValue3==JAR_ANGULAR_BLOCKED);
 players[1].noTools=0;
 liftCeilingActor=rider->s.number;
 const float riderHeight=rider->r.currentOrigin[2];
 // The first rotation padded the contact gap. Test a positive vertical sweep.
 spinner->genericValue2=JAR_ANGULAR_FORWARD;
 level.time+=50;AngularFrame(spinner);
 assert(spinner->genericValue3==JAR_ANGULAR_BLOCKED&&rider->r.currentOrigin[2]==riderHeight);
 assert(spinner->r.linked&&rider->r.linked&&cargo->r.linked);
 assert(JAR_RigidRead(cargo->s.number,&rigid,low,high)&&Distance(rigid.position,cargo->r.currentOrigin)<.001f);
 liftCeilingActor=-1;
 ControllerSelect(pendulum);
 Command(0,"jar_property","controller_angular_period_1000");assert(pendulum->delay==1000&&pendulum->genericValue4==25);
 pendulum->timestamp=level.time;
 level.time+=50;AngularFrame(pendulum);
 Command(0,"jar_property","property_undo");assert(pendulum->delay==2000&&pendulum->genericValue4==150);
 Command(0,"jar_property","controller_angular_running");assert(!pendulum->genericValue1);
 const int phase=pendulum->genericValue4;
 const float swing=pendulum->r.currentAngles[ROLL];
 static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world));const int size=WorldEncode(&world,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 CopyAssembly(owner,pendulum->s.number);assert(copies[0].count==1);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){430,0,100},NULL,NULL));
 gentity_t* clone=&g_entities[players[0].selected];assert(clone!=pendulum&&clone->genericValue4==phase&&!clone->genericValue1);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 gentity_t* restored=NULL;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_PENDULUM_CONTROLLER)restored=&g_entities[i];
 assert(restored&&restored->genericValue4==phase&&restored->r.currentAngles[ROLL]==swing);
 assert(WorldClear());imports.Trace=RigidTrace;
 puts("MP angular movers: controls, riders, cargo, consent, blocked rollback, duplicate and journal passed.");
}

static void ReflectionCases(void){
 RigidFixture();gentity_t* p=&g_entities[0];
 gentity_t* e=ControllerAt("misc_cubemap",0,100);assert(e);
 const int n=e->s.number;const unsigned owner=(unsigned)e->s.time2;
 assert(ControllerKind(n)==JAR_REFLECTION_CONTROLLER&&owner&&e->s.bolt2==JAR_MP_PROBE_TAG);
 assert(e->r.contents==CONTENTS_ITEM&&(e->r.svFlags&SVF_BROADCAST)&&!PhysicsSolid(n)&&e->s.speed==512&&e->s.time==1);
 ControllerSelect(e);Command(0,"jar_property","controller_probe_radius_32768");assert(e->s.speed==32768);
 Command(0,"jar_property","controller_probe_radius_131073");assert(e->s.speed==32768);
 Command(0,"jar_property","property_undo");assert(e->s.speed==512);
 Command(0,"jar_property","controller_probe_radius_1000");Command(0,"jar_property","controller_probe_refresh");
 assert(e->s.speed==1000&&e->s.time==2&&(unsigned)e->s.time2==owner);
 Command(0,"jar_property","controller_probe_active");assert(!e->s.time);
 Command(0,"jar_property","property_undo");assert(e->s.time==2);
 ControllerShot(p,n,qfalse);assert(!e->s.time);
 ControllerShot(p,n,qfalse);assert(e->s.time==2);
 aimed=n;Command(1,"jar_select","");Command(1,"jar_property","controller_probe_radius_800");assert(e->s.speed==1000);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes);
 assert(size&&WorldDecode(bytes,size,&decoded)&&decoded.copy.node[0].controller.speed==1000);
 CopyAssembly(p,n);assert(copies[0].count==1&&PlaceCopy(p,&copies[0],(float[3]){400,0,80},NULL,NULL));
 int duplicate=players[0].selected;assert(duplicate!=n&&g_entities[duplicate].s.speed==1000&&g_entities[duplicate].s.time==1);
 Undo(p,qfalse);assert(!g_entities[duplicate].inuse);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 assert(ControllerKind(n)==JAR_REFLECTION_CONTROLLER&&g_entities[n].s.speed==1000&&g_entities[n].s.time==1);
 assert((unsigned)g_entities[n].s.time2!=owner);
 for(int i=1;i<JAR_PROBE_LIMIT;++i)assert(ControllerAt("misc_cubemap",0,1000+i*64));
 assert(!ControllerAt("misc_cubemap",0,8000)&&ReflectionCount(-1)==JAR_PROBE_LIMIT);
 assert(WorldClear()&&!ReflectionCount(-1));
 puts("MP reflection probes: broadcast identity, settings, undo, activation, permissions, copy, save and map cap passed.");
}
static void LogicControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;imports.Print=Print;controllerPrints=0;gentity_t*p=&g_entities[0];
 gentity_t*random=ControllerAt("target_random",0,100),*timer=ControllerAt("func_timer",0,300),*a=ControllerAt("target_print",0,500),*b=ControllerAt("target_print",0,700);
 assert(random&&timer&&a&&b&&random->use==target_random_use&&timer->use==func_timer_use&&timer->think==func_timer_think&&!(timer->r.svFlags&SVF_NOCLIENT)&&!timer->nextthink);
 for(const char**name=(const char*[]){"info_null","info_notnull",NULL};*name;++name){gentity_t*point=ControllerAt(*name,0,900);assert(point&&ControllerKind(point->s.number)==5);ControllerSelect(point);Command(0,"jar_property","controller_text_4c6f63616c");assert(!strcmp(point->message,"Local"));}
 ControllerLink(p,random->s.number,qfalse);ControllerLink(p,a->s.number,qfalse);players[0].linkOutput=1;ControllerLink(p,random->s.number,qfalse);ControllerLink(p,b->s.number,qfalse);players[0].linkOutput=0;
 for(int i=0;i<32;++i){int old=controllerPrints;ControllerShot(p,random->s.number,qfalse);assert(controllerPrints==old+1);}
 // Only one eligible output fires, even when both outputs name the same object.
 objects[random->s.number].output[1]=a->s.number+1;int old=controllerPrints;ControllerShot(p,random->s.number,qfalse);assert(controllerPrints==old+1);objects[random->s.number].output[1]=b->s.number+1;
 ControllerSelect(random);Command(0,"jar_property","controller_flags_1");old=controllerPrints;ControllerShot(p,random->s.number,qfalse);assert(!random->use&&controllerPrints==old+1);ControllerShot(p,random->s.number,qfalse);assert(controllerPrints==old+1);
 Command(0,"jar_property","controller_playing");assert(random->use==target_random_use);Command(0,"jar_property","property_undo");assert(!random->use);
 ControllerLink(p,timer->s.number,qfalse);ControllerLink(p,a->s.number,qfalse);players[0].linkOutput=1;ControllerLink(p,timer->s.number,qfalse);ControllerLink(p,b->s.number,qfalse);assert(!objects[timer->s.number].output[1]);players[0].linkOutput=0;
 ControllerShot(p,timer->s.number,qfalse);assert(timer->nextthink==level.time+1000);old=controllerPrints;ControllerTick(timer,999);assert(controllerPrints==old);ControllerTick(timer,1);assert(controllerPrints==old+1&&timer->nextthink==level.time+1000);
 ControllerSelect(timer);int deadline=timer->nextthink;Command(0,"jar_property","controller_delay_500");Command(0,"jar_property","controller_random_100");assert(timer->wait==.5f&&timer->random==.1f&&timer->nextthink==deadline);
 Command(0,"jar_property","controller_random_501");assert(timer->random==.1f);Command(0,"jar_property","property_undo");assert(timer->random==0&&timer->wait==.5f);
 Command(0,"jar_property","controller_enabled");old=controllerPrints;ControllerTick(timer,1000);assert(controllerPrints==old&&timer->nextthink==level.time+500);Command(0,"jar_property","controller_enabled");ControllerTick(timer,500);assert(controllerPrints==old+1);
 Command(0,"jar_property","controller_playing");assert(!timer->nextthink);Command(0,"jar_property","property_undo");assert(timer->nextthink==level.time+500);
 // World and clipboard retain running timers, remaining time and spent randoms.
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 int timerIndex=-1;for(int i=0;i<saved.copy.count;++i)if(saved.copy.node[i].id==ControllerId("func_timer"))timerIndex=i;assert(timerIndex>=0&&saved.copy.node[timerIndex].controller.active&&saved.copy.node[timerIndex].controllerRemaining==500);
 jarCopyNode_t*node=&decoded.copy.node[timerIndex];node->controllerRemaining=0;assert(!CopyControllersValid(&decoded.copy));node->controllerRemaining=500;node->controller.active=0;assert(!CopyControllersValid(&decoded.copy));
 CopyAssembly(p,timer->s.number);assert(copies[0].count>=2&&CopyControllersValid(&copies[0]));assert(WorldClear());assert(PlaceCopy(NULL,&saved.copy,vec3_origin,saved.velocity,saved.angular));
 timer=random=NULL;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==15)timer=&g_entities[n];else if(ControllerKind(n)==14)random=&g_entities[n];assert(timer&&random&&!random->use&&timer->nextthink==level.time+500);
 old=controllerPrints;ControllerTick(timer,499);assert(controllerPrints==old);ControllerTick(timer,1);assert(controllerPrints==old+1);
 assert(WorldClear());assert(PlaceCopy(p,&copies[0],(float[3]){1500,0,80},NULL,NULL));int pasted=players[0].selected;Undo(p,qfalse);assert(!g_entities[pasted].inuse);
 // A foreign output is checked again at fire time after shared editing changes.
 random=ControllerAt("target_random",0,100);a=ControllerAt("target_print",1,300);sharedEditing.integer=1;ControllerLink(p,random->s.number,qfalse);ControllerLink(p,a->s.number,qfalse);sharedEditing.integer=0;old=controllerPrints;ControllerShot(p,random->s.number,qfalse);assert(controllerPrints==old);
 timer=ControllerAt("func_timer",0,500);a=ControllerAt("target_print",0,700);ControllerLink(p,timer->s.number,qfalse);ControllerLink(p,a->s.number,qfalse);ControllerShot(p,timer->s.number,qfalse);a->use=ControllerUseSpy;cleanupDisconnect.integer=0;JAR_MP_Disconnect(0);controllerActivator=-1;ControllerTick(timer,1000);assert(controllerActivator==timer->s.number);
 PhysicsReset();puts("MP Timer/Random: native callbacks, owned outputs, timing/properties/Undo, spent state, permission revocation, connection reuse, JRW13, independent copies and cleanup passed.");
}
