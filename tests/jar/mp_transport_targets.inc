// Production target callbacks and native TeleportPlayer; collision/network events
// are deterministic imports. This is not a graphical or human multiplayer test.
static gentity_t*TransportSpawn(const char*name,float x){return SpawnWithController(&g_entities[0],ControllerId(name),(float[3]){x,0,100},vec3_origin);}
static void PlayerStartCases(void){
 RigidFixture();level.gametype=GT_FFA;
 gentity_t *player=&g_entities[0];player->client->sess.sessionTeam=TEAM_FREE;player->client->ps.pm_type=PM_NORMAL;
 const vec3_t mins={-15,-15,-24},maxs={15,15,32};
 VectorCopy(mins,player->r.mins);VectorCopy(maxs,player->r.maxs);
 gentity_t *start=SpawnWithController(player,89857924u,(vec3_t){100,0,24},(vec3_t){0,90,0});
 gentity_t *respawn=SpawnWithController(player,88940811u,(vec3_t){400,200,24},(vec3_t){0,180,0});
 assert(start&&respawn&&strcmp(start->classname,"info_player_start")&&strcmp(respawn->classname,"info_player_deathmatch"));
 assert(start->r.contents==CONTENTS_ITEM&&!PhysicsSolid(start->s.number)&&start->s.bolt2==JAR_MP_LOGIC_MARKER_TAG);
 vec3_t origin,angles;
 assert(!JAR_MP_SelectPortableStart(player,qtrue,mins,maxs,origin,angles));
 ControllerSelect(start);Command(0,"jar_property","controller_text_48616e676172");assert(!strcmp(start->message,"Hangar"));
 Command(0,"jar_property","controller_start_spawn");
 assert(JAR_MP_SelectPortableStart(player,qtrue,mins,maxs,origin,angles)==start&&origin[0]==100&&origin[2]==53&&angles[YAW]==90);
 Command(0,"jar_property","property_undo");assert(!JAR_MP_SelectPortableStart(player,qtrue,mins,maxs,origin,angles));
 Command(0,"jar_property","controller_start_spawn");
 player->client->ps.origin[1]=200;ControllerSelect(respawn);Command(0,"jar_property","controller_start_spawn");
 assert(JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles)==respawn&&origin[0]==400&&angles[YAW]==180);
 blocked=1;assert(!JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles));blocked=0;
 objects[respawn->s.number].frozen=0;assert(!JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles));objects[respawn->s.number].frozen=1;
 level.gametype=GT_CTF;assert(!JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles));level.gametype=GT_TEAM;
 assert(JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles)==respawn);
 const int before=teleportEvents;ControllerShot(player,start->s.number,qfalse);
 assert(teleportEvents==before+2&&player->client->ps.origin[0]==100&&player->client->ps.origin[2]==53);
 gentity_t*tele=SpawnAt(player,ControllerId("target_teleporter"),(vec3_t){600,0,30},vec3_origin);
 ControllerLink(player,tele->s.number,qfalse);ControllerLink(player,respawn->s.number,qfalse);
 assert(TransportDestination(tele)==respawn);ControllerShot(player,tele->s.number,qfalse);
 assert(teleportEvents==before+4&&player->client->ps.origin[0]==400&&player->client->ps.origin[2]==53);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));
 const int size=WorldEncode(&saved,bytes);assert(size>0&&WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 start=JAR_MP_SelectPortableStart(player,qtrue,mins,maxs,origin,angles);
 assert(start&&!strcmp(start->message,"Hangar")&&origin[0]==100);
 respawn=JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles);
 assert(respawn&&origin[0]==400&&angles[YAW]==180);
 assert(WorldClear()&&!JAR_MP_SelectPortableStart(player,qfalse,mins,maxs,origin,angles));
 puts("MP start markers: isolated native class, nonblocking body, visit and spatial links, opt-in entry/respawn, geometry preflight, Undo and world restore passed.");
}

static void TransportTargetCases(void){
 RigidFixture();imports.SendServerCommand=Send;g_gravity.value=800;gentity_t*p=&g_entities[0];p->client->ps.pm_type=PM_NORMAL;VectorSet(p->r.mins,-16,-16,-24);VectorSet(p->r.maxs,16,16,40);
 gentity_t*push=TransportSpawn("target_push",100);assert(push&&push->use==Use_target_push);int root=push->s.number,marker=objects[root].output[0]-1;assert(ControllerKind(marker)==5&&objects[marker].order==objects[root].order);
 vec3_t hull;VectorCopy(push->s.origin2,hull);ControllerShot(p,root,qfalse);assert(fabsf(p->client->ps.velocity[0]-480)<.001f&&fabsf(p->client->ps.velocity[2]-360)<.001f&&VectorCompare(hull,push->s.origin2));
 ControllerSelect(push);Command(0,"jar_property","controller_speed_1200");ControllerShot(p,root,qfalse);assert(fabsf(p->client->ps.velocity[0]-960)<.001f);
 Command(0,"jar_property","controller_speed_4001");assert(push->speed==4001);Command(0,"jar_property","controller_speed_49");assert(push->speed==49);Command(0,"jar_property","controller_speed_1200");
 Command(0,"jar_property","controller_speed_1000");Command(0,"jar_property","property_undo");assert(push->speed==1200);
 Command(0,"jar_property","controller_flags_0");ControllerShot(p,root,qfalse);assert(fabsf(p->client->ps.velocity[2]-sqrtf(2*800*96))<.001f);
 vec3_t velocity;VectorCopy(p->client->ps.velocity,velocity);g_entities[marker].r.currentOrigin[2]=push->r.currentOrigin[2]-1;ControllerShot(p,root,qfalse);assert(VectorCompare(velocity,p->client->ps.velocity));
 g_entities[marker].r.currentOrigin[2]=push->r.currentOrigin[2]+96;g_gravity.value=0;ControllerShot(p,root,qfalse);assert(VectorCompare(velocity,p->client->ps.velocity));g_gravity.value=800;
 Command(0,"jar_property","controller_flags_3");assert(push->noise_index);ControllerShot(p,root,qfalse);assert(push->speed==1200&&p->fly_sound_debounce_time>level.time);
 // Destination links are a typed port; secondary/activation destinations fail.
 gentity_t*print=ControllerAt("target_print",0,600);ControllerLink(p,root,qfalse);ControllerLink(p,print->s.number,qfalse);assert(objects[root].output[0]==marker+1);
 players[0].linkOutput=1;ControllerLink(p,root,qfalse);ControllerLink(p,marker,qfalse);assert(!objects[root].output[1]);players[0].linkOutput=0;
 // Unlink switches to the native angle direction without overwriting prop bounds.
 ForceDetach(root);VectorSet(push->r.currentAngles,0,90,0);ControllerShot(p,root,qfalse);assert(fabsf(p->client->ps.velocity[1]-1200)<.001f&&VectorCompare(hull,push->s.origin2));
 ControllerLink(p,root,qfalse);ControllerLink(p,marker,qfalse);
 gentity_t*tele=TransportSpawn("target_teleporter",900);assert(tele&&tele->use==target_teleporter_use);int t=tele->s.number,d=objects[t].output[0]-1;gentity_t*dest=&g_entities[d];assert(ControllerKind(d)==8);
 VectorSet(dest->r.currentAngles,0,90,0);int events=teleportEvents,killBoxes=teleportKillBoxes,bit=p->client->ps.eFlags&EF_TELEPORT_BIT;
 ControllerShot(p,t,qfalse);assert(teleportEvents==events+2&&teleportKillBoxes==killBoxes+1&&(p->client->ps.eFlags&EF_TELEPORT_BIT)!=bit);
 assert(p->client->ps.origin[0]==dest->r.currentOrigin[0]&&p->client->ps.origin[2]==dest->r.currentOrigin[2]+dest->r.maxs[2]-p->r.mins[2]+5&&p->client->ps.viewangles[1]==90&&fabsf(p->client->ps.velocity[1]-400)<.001f&&p->client->ps.pm_time==160);
 vec3_t origin;VectorCopy(p->client->ps.origin,origin);events=teleportEvents;blocked=1;ControllerShot(p,t,qfalse);assert(VectorCompare(origin,p->client->ps.origin)&&teleportEvents==events);blocked=0;
 players[0].noTools=1;ControllerShot(p,t,qfalse);assert(teleportEvents==events);players[0].noTools=0;p->client->ps.m_iVehicleNum=999;ControllerShot(p,t,qfalse);assert(teleportEvents==events);p->client->ps.m_iVehicleNum=0;
 // Consent is checked on the activator, including one arriving through a Delay.
 gentity_t*delay=ControllerAt("target_delay",0,1300);ControllerLink(p,delay->s.number,qfalse);ControllerLink(p,t,qfalse);ControllerShot(p,delay->s.number,qfalse);players[0].noTools=1;ControllerTick(delay,1000);assert(teleportEvents==events);players[0].noTools=0;
 // Another player's marker becomes unusable if sharing is subsequently revoked.
 objects[d].owner=2;ControllerShot(p,t,qfalse);assert(teleportEvents==events);sharedEditing.integer=1;ControllerShot(p,t,qfalse);assert(teleportEvents==events+2);sharedEditing.integer=0;objects[d].owner=1;
 // Exact native NPC teleport keeps ET_NPC and updates native origin/angles.
 gentity_t*npc=CopySpawn(2,0,1600);CopyReady(npc);VectorSet(npc->r.mins,-16,-16,-24);VectorSet(npc->r.maxs,16,16,40);npc->client->ps.pm_type=PM_NORMAL;
 ControllerInvoke(tele,p,npc);assert(npc->s.eType==ET_NPC&&npc->r.currentOrigin[0]==dest->r.currentOrigin[0]);G_FreeEntity(npc);
 // Saved links remap to destination objects; clipboard state is immutable.
 ControllerSelect(push);Command(0,"jar_property","controller_speed_1234");CopyAssembly(p,root);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));int loaded=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){int kind=ControllerKind(n);if(kind==6){++loaded;assert(g_entities[n].speed==1234&&TransportDestination(&g_entities[n])&&g_entities[n].spawnflags==3);}if(kind==7){++loaded;assert(TransportDestination(&g_entities[n]));events=teleportEvents;ControllerInvoke(&g_entities[n],p,p);assert(teleportEvents==events+2);}}
 assert(loaded==2&&WorldClear());assert(PlaceCopy(p,&copies[0],(float[3]){2000,0,100},NULL,NULL));assert(ControllerKind(players[0].selected)==6&&g_entities[players[0].selected].speed==1234&&TransportDestination(&g_entities[players[0].selected]));Undo(p,qfalse);assert(WorldClear());
 // Wrong destination type, secondary spatial edge and corrupt speed are rejected.
 static jarCopy_t malformed;malformed=copies[0];malformed.node[0].output[1]=malformed.node[0].output[0];assert(!CopyControllersValid(&malformed));malformed=copies[0];malformed.node[1].id=ControllerId("target_print");assert(!CopyControllersValid(&malformed));
 memcpy(bad,bytes,size);at=WORLD_HEADER+260;WorldPut(bad,&at,WORLD_SIZE+1);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);
 // Deletion before slot reuse disconnects references rather than reusing a target.
 tele=TransportSpawn("target_teleporter",100);t=tele->s.number;d=objects[t].output[0]-1;G_FreeEntity(&g_entities[d]);ControllerAt("misc_teleporter_dest",0,700);assert(!objects[t].output[0]);events=teleportEvents;ControllerShot(p,t,qfalse);assert(teleportEvents==events);assert(WorldClear());
 // Transactional helper creation and one-group Undo for a normal menu spawn.
 int live=LiveCount();imports.Trace=Trace;blockAt=traceCalls+2;assert(!TransportSpawn("target_push",100)&&LiveCount()==live);blockAt=0;imports.Trace=RigidTrace;
 tele=TransportSpawn("target_teleporter",100);t=tele->s.number;d=objects[t].output[0]-1;Undo(p,qfalse);assert(!g_entities[t].inuse&&!g_entities[d].inuse);
 // Authored-map native paths retain origin2 and G_PickTarget semantics.
 gentity_t*map=G_Spawn();VectorSet(map->s.origin2,11,22,33);Use_target_push(map,map,p);assert(VectorCompare(p->client->ps.velocity,map->s.origin2));
 authoredDestination=G_Spawn();VectorSet(authoredDestination->s.origin,50,60,70);VectorSet(authoredDestination->s.angles,0,180,0);target_teleporter_use(map,map,p);assert(p->client->ps.origin[0]==50&&p->client->ps.origin[2]==71&&p->client->ps.viewangles[1]==180);authoredDestination=NULL;
 PhysicsReset();puts("MP spatial targets: native push/teleport, helpers, typed links, settings, arc bounds, consent, obstruction, native NPC snapshots, exact copy/JRW9, slot reuse, allocation rollback and authored-map preservation passed.");
}
