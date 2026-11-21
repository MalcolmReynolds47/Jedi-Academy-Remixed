// Actual context commands, state echo, ownership, native mirrors and persistence.
static char contextFeedback[3][256];static int contextFeedbackCount;
static void ContextSend(int c,const char*s){if(c>=0&&c<3&&!strncmp(s,"jarselection ",13)){Q_strncpyz(contextFeedback[c],s,sizeof(contextFeedback[c]));++contextFeedbackCount;}}
static void ContextRaw(int c,const char*action,const char*token){level.time+=110;Q_strncpyz(argument,action,sizeof(argument));Q_strncpyz(contextTokenArgument,token,sizeof(contextTokenArgument));assert(JAR_MP_Command(&g_entities[c],"jar_property"));}
static void PlayerContextCases(void){
 WorldFixture();
 gentity_t*host=&g_entities[0];gentity_t*guest=&g_entities[1];
 host->client->ps.pm_type=guest->client->ps.pm_type=PM_NORMAL;
 const int oldShared=sharedEditing.integer;
 sharedEditing.integer=0;playerTools.integer=0;
 Command(0,"jar_select","self");
 assert(players[0].selected==0&&(ContextCaps(host,0)&JAR_CONTEXT_ARMOR));
 Command(0,"jar_property","health_75");Command(0,"jar_property","armor_60");
 assert(host->health==75&&host->client->ps.stats[STAT_HEALTH]==75&&host->client->ps.stats[STAT_ARMOR]==60);
 Command(0,"jar_property","property_undo");assert(host->client->ps.stats[STAT_ARMOR]==0);
 aimed=1;Command(0,"jar_select","");assert(players[0].selected==ENTITYNUM_NONE);
 playerTools.integer=1;Command(0,"jar_select","");assert(players[0].selected==1);
 Command(0,"jar_property","armor_48");assert(guest->client->ps.stats[STAT_ARMOR]==48);
 Command(0,"jar_property","armor_-1");Command(0,"jar_property","armor_32768");
 assert(guest->client->ps.stats[STAT_ARMOR]==48);
 char token[32];snprintf(token,sizeof(token),"%u",ContextIdentity(1));
 Command(1,"jar_allowTools","0");ContextRaw(0,"armor_12",token);ContextRaw(0,"property_undo",token);
 assert(guest->client->ps.stats[STAT_ARMOR]==48&&players[0].selected==ENTITYNUM_NONE);
 Command(1,"jar_select","self");Command(1,"jar_property","armor_33");
 assert(guest->client->ps.stats[STAT_ARMOR]==33);
 Command(1,"jar_allowTools","1");Command(0,"jar_select","");Command(0,"jar_property","freeze");
 assert(JAR_MP_ActorControlled(guest));
 Command(1,"jar_select","self");Command(1,"jar_property","health_64");Command(1,"jar_property","armor_24");
 assert(JAR_MP_ActorControlled(guest)&&guest->health==64&&guest->client->ps.stats[STAT_ARMOR]==24);
 Command(1,"jar_property","release");assert(!JAR_MP_ActorControlled(guest));
 level.time+=1100;aimed=1;Command(0,"jar_select","");
 playerTools.integer=0;ContextRaw(0,"health_50",token);
 assert(guest->health==64&&players[0].selected==ENTITYNUM_NONE);
 sandbox.integer=0;Command(1,"jar_property","health_25");assert(guest->health==64);
 sandbox.integer=1;playerTools.integer=1;sharedEditing.integer=oldShared;
 puts("MP player context: self and remote health/armor, held self actions, native value bounds, Undo, opt-out and server policy passed.");
}
static void MovementContextCases(void){
 WorldFixture();gentity_t*p=&g_entities[0];p->client->ps.pm_type=PM_NORMAL;
 const float mapGravity=g_gravity.value;g_gravity.value=800;
 gentity_t*gravity=SpawnAt(p,95709066,(float[3]){100,0,100},vec3_origin);
 gentity_t*friction=SpawnAt(p,92295820,(float[3]){300,0,100},vec3_origin);
 assert(gravity&&friction&&gravity->r.contents==CONTENTS_ITEM&&friction->r.contents==CONTENTS_ITEM);
 assert(gravity->s.bolt2==JAR_MP_LOGIC_MARKER_TAG&&friction->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 players[0].selected=gravity->s.number;
 Command(0,"jar_property","controller_movement_value_3201");assert(gravity->speed==3201);
 Command(0,"jar_property","property_undo");assert(gravity->speed==400);
 Command(0,"jar_property","controller_movement_value_125");assert(gravity->speed==125);
 Command(0,"jar_property","property_undo");assert(gravity->speed==400);
 Command(0,"jar_property","controller_movement_value_125");
 players[0].noTools=1;ControllerActivate(p,gravity->s.number,qfalse);assert(!p->client->ps.stats[STAT_JAR_ACTOR_GRAVITY]);
 players[0].noTools=0;ControllerActivate(p,gravity->s.number,qfalse);assert(p->client->ps.gravity==125);
 players[0].selected=friction->s.number;
 Command(0,"jar_property","controller_movement_value_0");
 Command(0,"jar_property","controller_enabled");ControllerActivate(p,friction->s.number,qfalse);assert(!p->client->ps.stats[STAT_JAR_ACTOR_FRICTION]);
 Command(0,"jar_property","property_undo");ControllerActivate(p,friction->s.number,qfalse);
 assert(JAR_ActorMovementValue(&p->client->ps,STAT_JAR_ACTOR_FRICTION,JAR_MOVEMENT_MAX_FRICTION)==0);
 players[0].selected=0;Command(0,"jar_property","actor_map_gravity");assert(p->client->ps.gravity==800);
 Command(0,"jar_property","property_undo");assert(p->client->ps.gravity==125);
 Command(0,"jar_property","actor_friction_7");assert(p->client->ps.stats[STAT_JAR_ACTOR_FRICTION]==8);
 p->client->ps.m_iVehicleNum=99;assert(JAR_ActorMovementValue(&p->client->ps,STAT_JAR_ACTOR_FRICTION,JAR_MOVEMENT_MAX_FRICTION)==-1);p->client->ps.m_iVehicleNum=0;

 gentity_t*delay=SpawnAt(p,ControllerId("target_delay"),(float[3]){500,0,100},vec3_origin);
 assert(delay);ControllerLink(p,delay->s.number,qfalse);ControllerLink(p,friction->s.number,qfalse);
 assert(objects[delay->s.number].output[0]==friction->s.number+1);
 ControllerActivate(p,delay->s.number,qfalse);level.time=delay->nextthink;delay->think(delay);
 assert(p->client->ps.stats[STAT_JAR_ACTOR_FRICTION]==1);
 gentity_t*npc=CopySpawn(2,0,700);CopyReady(npc);npc->client->ps.pm_type=PM_NORMAL;
 players[0].selected=npc->s.number;
 Command(0,"jar_property","actor_gravity_0");Command(0,"jar_property","actor_friction_10");
 assert(npc->client->ps.gravity==0&&npc->client->ps.stats[STAT_JAR_ACTOR_FRICTION]==11);
 CopyAssembly(p,npc->s.number);assert(copies[0].count==1&&copies[0].node[0].actorGravity==1&&copies[0].node[0].actorFriction==11);
 assert(PlaceCopy(p,&copies[0],(float[3]){900,0,100},NULL,NULL));
 gentity_t*pasted=&g_entities[players[0].selected];CopyReady(pasted);
 assert(pasted!=npc&&pasted->client->ps.stats[STAT_JAR_ACTOR_GRAVITY]==1&&pasted->client->ps.stats[STAT_JAR_ACTOR_FRICTION]==11);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));
 int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==WORLD_JRW24&&WorldDecode(bytes,size,&decoded));
 assert(decoded.copy.node[0].controller.speed==125&&decoded.copy.node[1].controller.speed==0);
 players[0].selected=npc->s.number;
 Command(0,"jar_property","actor_gravity_12000");Command(0,"jar_property","actor_friction_317");
 assert(npc->client->ps.gravity==12000&&npc->client->ps.stats[STAT_JAR_ACTOR_FRICTION]==318);
 assert(WorldCapture(&saved));
 size=WorldEncode(&saved,bytes);at=0;
 assert(size&&WorldGet(bytes,&at)==WORLD_JRW25&&WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 int restored=0,wide=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&g_entities[n].NPC){
  CopyReady(&g_entities[n]);int value=g_entities[n].client->ps.stats[STAT_JAR_ACTOR_FRICTION];assert(value==11||value==318);wide+=value==318;++restored;
 }
 assert(restored==2&&wide==1);assert(WorldClear());g_gravity.value=mapGravity;
 puts("MP movement: native marker activation, consent, links, settings Undo, actor movement, NPC copy and JRW24/JRW25 restore passed.");
}
static void ContextCases(void){
 imports.FS_Open=Open;imports.FS_Read=Read;imports.FS_Close=Close;imports.SendServerCommand=ContextSend;WorldFixture();
 gentity_t*prop=CopySpawn(1,0,100);int n=prop->s.number;aimed=n;Command(0,"jar_select","");unsigned identity=ContextIdentity(n);
 assert(strstr(contextFeedback[0],"jarselection ")&&ContextKind(n)==JAR_CONTEXT_PROP);
 assert((ContextCaps(&g_entities[0],n)&(JAR_CONTEXT_MASS|JAR_CONTEXT_GRAVITY|JAR_CONTEXT_POSE))==(JAR_CONTEXT_MASS|JAR_CONTEXT_GRAVITY|JAR_CONTEXT_POSE));
 char token[32];snprintf(token,sizeof(token),"%u",identity);float original=objects[n].mass;
 for(const char**s=(const char*[]){"","0","-1","4294967296","1;quit",NULL};*s;++s){ContextRaw(0,"heavy",*s);assert(objects[n].mass==original);}
 for(const char**s=(const char*[]){"mass_","mass_nan","mass_inf","mass_0","mass_-4","mass_1e99","mass_0x1p2","mass_ 20","mass_20bad","health_100","quantity_20",NULL};*s;++s){ContextRaw(0,*s,token);assert(objects[n].mass==original&&!players[0].propertyUndo.field);}
 Command(0,"jar_property","mass_432.5");assert(objects[n].mass==432.5f&&(ContextCaps(&g_entities[0],n)&JAR_CONTEXT_UNDO));
 Command(0,"jar_property","property_undo");assert(objects[n].mass==original&&!players[0].propertyUndo.field);
 Command(0,"jar_property","mass_432.5");Command(0,"jar_property","gravity");assert(objects[n].noGravity);
 Command(0,"jar_property","property_undo");assert(!objects[n].noGravity&&objects[n].mass==432.5f);
 // Another editor invalidates Undo, even if that editor returns to the same value.
 Command(0,"jar_property","heavy");sharedEditing.integer=1;players[1].selected=n;
 Command(1,"jar_property","normal");Command(1,"jar_property","heavy");Command(0,"jar_property","property_undo");assert(objects[n].mass==200);
 sharedEditing.integer=0;Command(1,"jar_property","light");assert(objects[n].mass==200&&players[1].selected==ENTITYNUM_NONE);
 // Real entity slot reuse must reject a queued command with the retired identity.
 G_FreeEntity(prop);prop=CopySpawn(1,0,100);assert(prop->s.number==n);aimed=n;Command(0,"jar_select","");original=objects[n].mass;
 assert(ContextIdentity(n)!=identity);ContextRaw(0,"heavy",token);assert(objects[n].mass==original);
 Command(0,"jar_property","mass_.125");assert(objects[n].mass==.125f);
 Command(0,"jar_property","mass_2.5e4");assert(objects[n].mass==25000.f);
 Command(0,"jar_property","gravity");Command(0,"jar_property","freeze");
 gentity_t*npc=CopySpawn(2,0,500);CopyReady(npc);int a=npc->s.number;aimed=a;Command(0,"jar_select","");
 assert(ContextKind(a)==JAR_CONTEXT_NPC&&!(ContextCaps(&g_entities[0],a)&JAR_CONTEXT_MASS));
 for(const char**s=(const char*[]){"health_0","health_32768","health_3.5","health_nan","mass_80",NULL};*s;++s){Command(0,"jar_property",*s);assert(npc->health==100);}
 Command(0,"jar_property","health_32500");assert(npc->health==32500);
 Command(0,"jar_property","health_173");assert(npc->health==173&&npc->client->ps.stats[STAT_HEALTH]==173);
 npc->health=npc->client->ps.stats[STAT_HEALTH]=150;Command(0,"jar_property","property_undo");assert(npc->health==150); // damage cannot be undone as an old property edit
 Command(0,"jar_property","health_333");Command(0,"jar_property","property_undo");assert(npc->health==150);
 Command(0,"jar_property","health_333");
 gentity_t*vehicle=CopySpawn(3,0,900);CopyReady(vehicle);int v=vehicle->s.number;aimed=v;Command(0,"jar_select","");
 assert(ContextKind(v)==JAR_CONTEXT_VEHICLE);Command(0,"jar_property","health_444");assert(vehicle->health==444&&vehicle->client->ps.stats[STAT_HEALTH]==444&&vehicle->m_pVehicle->m_iArmor==444);
 vehicle->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[1];Command(0,"jar_property","health_999");assert(vehicle->health==444);vehicle->m_pVehicle->m_pPilot=NULL;
 unsigned baton=0;for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(!strcmp(jarMPCatalog[i].name,"weapon_stun_baton")){baton=jarMPCatalog[i].id;break;}assert(baton);
 gentity_t*pickup=SpawnAt(&g_entities[0],baton,(float[3]){1300,0,100},(float[3]){0,0,0});assert(pickup&&pickup->item);int item=pickup->s.number;aimed=item;Command(0,"jar_select","");
 assert(ContextKind(item)==JAR_CONTEXT_PICKUP&&(ContextCaps(&g_entities[0],item)&JAR_CONTEXT_QUANTITY)&&!(ContextCaps(&g_entities[0],item)&JAR_CONTEXT_MASS));
 pickup->item->giType=IT_ARMOR;assert(!(ContextCaps(&g_entities[0],item)&JAR_CONTEXT_QUANTITY));Command(0,"jar_property","quantity_99");assert(!pickup->count);
 pickup->item->giType=IT_HOLDABLE;assert(!(ContextCaps(&g_entities[0],item)&JAR_CONTEXT_QUANTITY));pickup->item->giType=IT_WEAPON;
 Command(0,"jar_property","quantity_17");assert(pickup->count==17);Command(0,"jar_property","property_undo");assert(pickup->count==0);
 Command(0,"jar_property","quantity_2147483648");assert(!pickup->count);
 Command(0,"jar_property","quantity_16777217");assert(pickup->count==16777217);
 Command(0,"jar_property","quantity_16777216");Command(0,"jar_property","property_undo");
 assert(pickup->count==16777217);
 static jarWorld_t world,decoded;static byte encoded[WORLD_BYTES];assert(WorldCapture(&world));int size=WorldEncode(&world,encoded);assert(size&&WorldDecode(encoded,size,&decoded));
 assert(decoded.copy.count==4&&decoded.copy.node[0].mass==25000.f&&decoded.copy.node[0].noGravity&&decoded.copy.node[0].frozen&&decoded.copy.node[1].health==333&&decoded.copy.node[2].health==444&&decoded.copy.node[3].count==16777217);
 CopyAssembly(&g_entities[0],item);assert(copies[0].node[0].count==16777217);Command(0,"jar_property","remove");assert(!pickup->inuse);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 for(int at=MAX_CLIENTS;at<level.num_entities;++at)if(objects[at].copyPending)CopyReady(&g_entities[at]);
 assert(WorldCapture(&world)&&world.copy.count==4&&world.copy.node[0].mass==25000.f&&world.copy.node[0].noGravity&&world.copy.node[1].health==333&&world.copy.node[2].health==444&&world.copy.node[3].count==16777217);
 assert(WorldClear());
 // Consenting players use health only; their native body cannot be removed.
 aimed=1;Command(0,"jar_select","");assert(ContextKind(1)==JAR_CONTEXT_PLAYER&&!(ContextCaps(&g_entities[0],1)&JAR_CONTEXT_REMOVE));
 Command(0,"jar_property","health_123");assert(g_entities[1].health==123&&clients[1].ps.stats[STAT_HEALTH]==123);
 Command(0,"jar_property","remove");assert(g_entities[1].inuse);Command(1,"jar_allowTools","0");Command(0,"jar_property","health_1");assert(g_entities[1].health==123);
 Command(1,"jar_allowTools","1");level.time+=1000;Command(0,"jar_select","");identity=ContextIdentity(1);JAR_MP_ClientReset(1);assert(!objects[1].contextIdentity&&players[0].selected==ENTITYNUM_NONE);
 Command(0,"jar_select","");assert(ContextIdentity(1)!=identity);
 // Open context gets native changes; closing bypasses request throttling and
 // stops periodic messages without affecting selection on another connection.
 int count=contextFeedbackCount;g_entities[1].health=122;level.time+=501;ContextFrame();assert(contextFeedbackCount>count&&strstr(contextFeedback[0]," 122 "));
 Q_strncpyz(argument,"close",sizeof(argument));players[0].nextRequest=level.time+10000;assert(JAR_MP_Command(&g_entities[0],"jar_select"));assert(players[0].selected==ENTITYNUM_NONE);
 count=contextFeedbackCount;level.time+=501;ContextFrame();assert(contextFeedbackCount==count);
 puts("MP context: typed values, applicability, echoes, scalar Undo/conflicts, stale identities, consent, boarding, pickup removal and persistence passed.");
}
