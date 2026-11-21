// Actual native trigger touch/jump-pad/teleport callbacks with deterministic
// engine collision, damage-free arrival, seat-bolt and network-event imports.
#include "mp_teleport_publication.inc"
static void TriggerTransportCases(void){
 TeleportPublicationCases();
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0],*second=&g_entities[1];
 gentity_t*push=TransportSpawn("trigger_push",100),*dest=TransportDestination(push);assert(push&&dest&&ControllerKind(dest->s.number)==5);
 int root=push->s.number;gentity_t*h=TriggerHelper(root);assert(h&&h->r.linked&&h->s.eType==ET_PUSH_TRIGGER&&!(h->r.svFlags&SVF_NOCLIENT)&&push->s.eType==ET_GENERAL&&!push->nextthink&&!push->think);
 vec3_t hull;VectorCopy(push->s.origin2,hull);TriggerPlaceActor(p,push);TriggerEnter(push,p);assert(fabsf(p->client->ps.velocity[0]-800)<.001f&&fabsf(p->client->ps.velocity[2]-600)<.001f&&VectorCompare(hull,push->s.origin2));
 ControllerSelect(push);Command(0,"jar_property","controller_pushspeed_1500");TriggerEnter(push,p);assert(fabsf(p->client->ps.velocity[0]-1200)<.001f);Command(0,"jar_property","controller_pushspeed_49");assert(push->speed==49);Command(0,"jar_property","controller_pushspeed_4001");assert(push->speed==4001);Command(0,"jar_property","controller_pushspeed_1500");
 Command(0,"jar_property","controller_pushspeed_1000");Command(0,"jar_property","property_undo");assert(push->speed==1500);Command(0,"jar_property","controller_pushspeed_1000");
 // Linear wait and native MULTIPLE semantics: first touch, or all in that frame.
 Command(0,"jar_property","controller_delay_500");TriggerEnter(push,p);TriggerPlaceActor(second,push);VectorClear(second->client->ps.velocity);TriggerEnter(push,second);assert(!VectorLength(second->client->ps.velocity));
 Command(0,"jar_property","controller_multiple");level.time+=500;TriggerEnter(push,p);TriggerEnter(push,second);assert(fabsf(second->client->ps.velocity[0]-800)<.001f);VectorClear(second->client->ps.velocity);++level.time;TriggerEnter(push,second);assert(!VectorLength(second->client->ps.velocity));level.time+=499;TriggerEnter(push,second);assert(fabsf(second->client->ps.velocity[0]-800)<.001f);
 Command(0,"jar_property","controller_playing");int deadline=push->painDebounceTime+500;assert(!h->r.linked);Command(0,"jar_property","property_undo");assert(h->r.linked&&push->painDebounceTime+500==deadline);
 Command(0,"jar_property","controller_multiple");Command(0,"jar_property","controller_delay_-1");++level.time;TriggerEnter(push,p);assert(!objects[root].triggerActive&&!h->r.linked&&h->touch==TriggerTouch);
 Command(0,"jar_property","controller_playing");assert(h->r.linked);Command(0,"jar_property","property_undo");assert(!h->r.linked);Command(0,"jar_property","controller_playing");Command(0,"jar_property","controller_delay_0");
 // Relative launch originates at the actor; arc uses native jump-pad state.
 Command(0,"jar_property","controller_pushmode");assert(push->spawnflags==20);TriggerPlaceActor(p,push);p->r.currentOrigin[0]+=32;VectorCopy(p->r.currentOrigin,p->client->ps.origin);TriggerEnter(push,p);assert(fabsf(p->client->ps.velocity[0]-sqrtf(500000))<.002f&&fabsf(p->client->ps.velocity[2]-sqrtf(500000))<.002f);
 Command(0,"jar_property","controller_pushmode");assert(push->spawnflags==0&&!push->wait);p->client->ps.fd.forcePowersActive|=1<<FP_LEVITATION;p->client->ps.pmove_framecount=42;TriggerEnter(push,p);assert(fabsf(p->client->ps.velocity[0]-128/sqrtf(.24f))<.001f&&fabsf(p->client->ps.velocity[2]-sqrtf(153600))<.001f&&p->client->ps.jumppad_ent==h->s.number&&p->client->ps.jumppad_frame==42&&!(p->client->ps.fd.forcePowersActive&(1<<FP_LEVITATION)));
 Command(0,"jar_property","controller_delay_100");assert(!push->wait);vec3_t before;VectorCopy(p->client->ps.velocity,before);dest->r.currentOrigin[2]=push->r.currentOrigin[2]-1;TriggerEnter(push,p);assert(VectorCompare(before,p->client->ps.velocity));dest->r.currentOrigin[2]=push->r.currentOrigin[2]+96;
 Command(0,"jar_property","controller_pushmode");assert(push->spawnflags==4);ForceDetach(root);VectorSet(push->r.currentAngles,-90,0,0);TriggerSync(root);TriggerEnter(push,p);assert(fabsf(p->client->ps.velocity[2]-1000)<.001f&&VectorCompare(hull,push->s.origin2));
 ControllerLink(p,root,qfalse);ControllerLink(p,dest->s.number,qfalse);objects[dest->s.number].owner=2;VectorCopy(p->client->ps.velocity,before);TriggerEnter(push,p);assert(VectorCompare(before,p->client->ps.velocity));objects[dest->s.number].owner=1;
 // A native NPC travels through the same touch route, with owned permission.
 gentity_t*npc=CopySpawn(2,0,600);CopyReady(npc);TriggerPlaceActor(npc,push);TriggerEnter(push,npc);assert(fabsf(npc->client->ps.velocity[0]-800)<.001f);objects[npc->s.number].owner=2;VectorClear(npc->client->ps.velocity);TriggerEnter(push,npc);assert(!VectorLength(npc->client->ps.velocity));G_FreeEntity(npc);
 // The retail misc_teleporter card shares the native touch controller and destination.
 assert(ControllerId("misc_teleporter")==86167446u);
 gentity_t*tele=TransportSpawn("misc_teleporter",900),*exit=TransportDestination(tele);assert(tele&&exit&&ControllerKind(tele->s.number)==23&&ControllerKind(exit->s.number)==8&&TriggerHelper(tele->s.number)->s.eType==ET_TELEPORT_TRIGGER);
 VectorSet(exit->r.currentAngles,0,90,0);TriggerPlaceActor(p,tele);int events=teleportEvents,kills=teleportKillBoxes;TriggerEnter(tele,p);assert(teleportEvents==events+2&&teleportKillBoxes==kills+1&&p->client->ps.origin[0]==exit->r.currentOrigin[0]&&p->client->ps.viewangles[1]==90);
 TriggerPlaceActor(p,tele);VectorCopy(p->client->ps.origin,before);events=teleportEvents;blocked=1;TriggerEnter(tele,p);assert(VectorCompare(before,p->client->ps.origin)&&teleportEvents==events);blocked=0;
 players[0].noTools=1;TriggerEnter(tele,p);assert(teleportEvents==events);players[0].noTools=0;actors[0].active=1;TriggerEnter(tele,p);assert(teleportEvents==events);actors[0].active=0;
 ControllerSelect(tele);Command(0,"jar_property","controller_enabled");TriggerEnter(tele,p);assert(teleportEvents==events&&!TriggerHelper(tele->s.number)->r.linked);Command(0,"jar_property","property_undo");assert(TriggerHelper(tele->s.number)->r.linked);
 // MP's spectator-only native mode is usable and never emits killbox/events.
 second->client->sess.sessionTeam=TEAM_SPECTATOR;second->client->ps.pm_type=PM_SPECTATOR;TriggerPlaceActor(second,tele);kills=teleportKillBoxes;TriggerEnter(tele,second);assert(second->client->ps.origin[0]==exit->r.currentOrigin[0]&&teleportEvents==events&&teleportKillBoxes==kills);
 Command(0,"jar_property","controller_spectators");TriggerEnter(tele,p);assert(teleportEvents==events);TriggerPlaceActor(second,tele);second->client->ps.pm_flags|=PMF_FOLLOW;VectorCopy(second->client->ps.origin,before);TriggerEnter(tele,second);assert(VectorCompare(before,second->client->ps.origin));second->client->ps.pm_flags&=~PMF_FOLLOW;
 players[1].noTools=1;TriggerEnter(tele,second);assert(VectorCompare(before,second->client->ps.origin));players[1].noTools=0;TriggerEnter(tele,second);assert(second->client->ps.origin[0]==exit->r.currentOrigin[0]);second->client->sess.sessionTeam=TEAM_FREE;second->client->ps.pm_type=PM_NORMAL;
 // Save active settings, remaining native wait, spent single-push state, links.
 ControllerSelect(push);Command(0,"jar_property","controller_delay_750");TriggerPlaceActor(p,push);level.time+=1000;TriggerEnter(push,p);Command(0,"jar_property","controller_playing");assert(!objects[root].triggerActive);
 gentity_t*once=TransportSpawn("trigger_push",1400);ControllerSelect(once);Command(0,"jar_property","controller_delay_-1");TriggerPlaceActor(p,once);TriggerEnter(once,p);assert(!objects[once->s.number].triggerActive);
 int remaining=push->painDebounceTime+750-level.time;assert(remaining>0);CopyAssembly(p,root);assert(copies[0].count==2&&CopyControllersValid(&copies[0]));static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&world));int size=WorldEncode(&world,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 int pushed=-1;for(int i=0;i<world.copy.count;++i)if(world.copy.node[i].controller.pushSpeed&&world.copy.node[i].controller.waitMs==750)pushed=i;assert(pushed>=0&&world.copy.node[pushed].controllerRemaining==remaining);
 memcpy(bad,bytes,size);at=WORLD_HEADER+pushed*WORLD_NODE+276;WorldPut(bad,&at,WORLD_SIZE+1);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);assert(WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));int count=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i){int kind=ControllerKind(i);if(kind==22){++count;assert(TriggerHelper(i)&&!TriggerHelper(i)->r.linked&&!objects[i].triggerActive&&g_entities[i].speed==1000);if(g_entities[i].wait==750)assert(g_entities[i].painDebounceTime==level.time+remaining-750);else assert(g_entities[i].wait==-1);}if(kind==23){++count;assert(jarMPCatalog[objects[i].catalog-1].id==86167446u&&g_entities[i].spawnflags==1&&TransportDestination(&g_entities[i])&&TriggerHelper(i)->s.eType==ET_TELEPORT_TRIGGER);}}assert(count==3&&WorldClear());
 assert(PlaceCopy(p,&copies[0],(float[3]){1800,0,100},NULL,NULL));int copied=players[0].selected;assert(ControllerKind(copied)==22&&TriggerHelper(copied)&&g_entities[copied].speed==1000);Undo(p,qfalse);assert(!g_entities[copied].inuse&&WorldClear());
 // JRW14/15 files retain the former layout and default the new speed field.
 gentity_t*repeat=TransportSpawn("trigger_multiple",100);assert(repeat&&WorldCapture(&world));size=WorldEncode(&world,bytes);
 for(unsigned version=0x4557524au;version<=0x4657524au;version+=0x01000000u){memcpy(bad,bytes,WORLD_HEADER);at=0;WorldPut(bad,&at,version);at=WORLD_HEADER;for(int i=0;i<world.copy.count;++i){memcpy(bad+at,bytes+WORLD_HEADER+i*WORLD_NODE,276);at+=276;}assert(!world.copy.ropes);at+=4;WorldTestHash(bad,at);assert(WorldDecode(bad,at,&decoded)&&decoded.copy.count==world.copy.count);for(int i=0;i<decoded.copy.count;++i)assert(!decoded.copy.node[i].controller.pushSpeed);}
 assert(WorldClear());
 // A missing destination placement rolls back both the body and its sensor.
 int live=LiveCount();imports.Trace=Trace;blockAt=traceCalls+2;assert(!TransportSpawn("trigger_push",100)&&LiveCount()==live);blockAt=0;imports.Trace=RigidTrace;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)assert(!triggerParent[i]);
 static jarCopy_t malformed;malformed=copies[0];malformed.node[0].output[1]=malformed.node[0].output[0];assert(!CopyControllersValid(&malformed));
 // Authored trigger callbacks still use native vectors/map destinations.
 gentity_t*map=G_Spawn();map->spawnflags=4;map->speed=77;VectorSet(map->s.origin2,0,1,0);trigger_push_touch(map,p,NULL);assert(p->client->ps.velocity[1]==77);
 authoredDestination=G_Spawn();VectorSet(authoredDestination->s.origin,50,60,70);VectorSet(authoredDestination->s.angles,0,180,0);trigger_teleporter_touch(map,p,NULL);assert(p->client->ps.origin[0]==50&&p->client->ps.origin[2]==71);authoredDestination=NULL;PhysicsReset();
 puts("MP transport triggers: native linear/relative/arc push, touch wait/once, actor consent, misc_teleporter card, spectator teleport, obstructed arrival, JRW16 and legacy 14/15, copies and authored paths passed.");
}
static void TriggerVehicleTransportCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int family=0;family<4;++family){
  gentity_t*old=ArrivalFixture(types[family]),*e=teleportAssembly[0];imports.Trace=Trace;
  gentity_t*push=TransportSpawn("trigger_push",1200),*tele=TransportSpawn(family%2?"misc_teleporter":"trigger_teleport",1500);assert(push&&tele);Vehicle_t*v=e->m_pVehicle;
  TriggerPlaceActor(e,push);v->m_ucmd.forwardmove=127;e->client->ps.speed=700;players[2].noTools=1;vec3_t before;VectorCopy(e->client->ps.velocity,before);TriggerEnter(push,e);assert(VectorCompare(before,e->client->ps.velocity));players[2].noTools=0;
  TriggerEnter(push,e);assert(fabsf(e->client->ps.velocity[0]-800)<.001f&&fabsf(e->client->ps.velocity[2]-600)<.001f&&e->client->ps.jarVehicleDrift[0]==e->client->ps.velocity[0]&&!e->client->ps.speed&&v->m_ucmd.forwardmove==127&&v->m_pPilot==(bgEntity_t*)&g_entities[1]);
  TriggerPlaceActor(e,tele);imports.Trace=ArrivalTrace;arrivalTraces=0;arrivalBlock=2;VectorCopy(e->client->ps.origin,before);int events=teleportEvents;TriggerEnter(tele,e);assert(VectorCompare(before,e->client->ps.origin)&&teleportEvents==events);
  arrivalBlock=0;arrivalTraces=0;TriggerEnter(tele,e);assert(arrivalTraces==4&&teleportEvents==events+2&&e->r.currentOrigin[0]==TransportDestination(tele)->r.currentOrigin[0]);
  assert(v->m_pPilot==(bgEntity_t*)&g_entities[1]&&v->m_ppPassengers[0]==(bgEntity_t*)&g_entities[2]&&v->m_pDroidUnit==(bgEntity_t*)teleportAssembly[3]);for(int i=1;i<4;++i)assert(teleportAssembly[i]->client->ps.m_iVehicleNum==e->s.number&&teleportAssembly[i]->r.linked);PhysicsReset();
 }
 puts("MP transport trigger vehicles: four native families, driver/passenger/droid consent, retained push momentum, transactional obstructed arrival and unchanged seat links passed.");
}
