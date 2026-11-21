// Actual native use decisions with the owned geometry/physics adapter.
// Engine tracing and damage dispatch are imports; this is not a rendered test.
static void UsableCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0];
 gentity_t*e=ControllerAt("func_usable",0,100),*relay=ControllerAt("target_relay",0,400),*counter=ControllerAt("target_counter",0,700);
 assert(e&&ControllerKind(e->s.number)==29&&e->use==UsableUse&&e->r.contents==CONTENTS_SOLID&&(e->r.svFlags&SVF_PLAYER_USABLE));
 int n=e->s.number;ControllerLink(p,relay->s.number,qfalse);ControllerLink(p,n,qfalse);ControllerLink(p,n,qfalse);ControllerLink(p,counter->s.number,qfalse);
 players[0].linkOutput=1;ControllerLink(p,n,qfalse);ControllerLink(p,counter->s.number,qfalse);players[0].linkOutput=0;
 RigidSteps(1);assert(JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));
 ControllerShot(p,relay->s.number,qfalse);assert(BodyDormant(n)&&counter->count==1&&!e->r.contents&&!JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));
 vec3_t at;VectorCopy(e->r.currentOrigin,at);objects[n].frozen=0;objects[n].thruster=1;objects[n].thrustBy=1;RigidSteps(8);assert(Distance(at,e->r.currentOrigin)<.001f&&!VectorLength(objects[n].velocity)&&!rigidFailed);
 blocked=1;GlobalUse(e,p,p);assert(BodyDormant(n)&&e->genericValue11==1&&e->think==UsableThink&&counter->count==1);StationTick(e,FRAMETIME);assert(BodyDormant(n));blocked=0;StationTick(e,FRAMETIME);assert(!BodyDormant(n)&&!e->genericValue11&&counter->count==0&&e->r.contents==CONTENTS_SOLID);
 objects[n].thruster=0;RigidSteps(1);assert(JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));
 // Toggle while held releases the Physgun; dormant fallback bodies stay still.
 ActorArm(0,n);assert(objects[n].heldBy);GlobalUse(e,p,p);assert(BodyDormant(n)&&!objects[n].heldBy&&players[0].held==ENTITYNUM_NONE);rigidEnabled.integer=0;ActorTick();objects[n].frozen=0;VectorCopy(e->r.currentOrigin,at);ActorTick();assert(Distance(at,e->r.currentOrigin)<.001f);
 ControllerSelect(e);Command(0,"jar_property","controller_visible");assert(!BodyDormant(n));Command(0,"jar_property","property_undo");assert(BodyDormant(n));
 blocked=1;Command(0,"jar_property","controller_visible");assert(BodyDormant(n));blocked=0;Command(0,"jar_property","controller_visible");assert(!BodyDormant(n));
 Command(0,"jar_property","controller_count_100");assert(e->health==100&&e->takedamage);e->health=90;e->pain(e,p,10);assert(BodyDormant(n)&&e->health==90&&!ContextUndoReady(p,n));
 e->health=-10;e->die(e,p,p,100,MOD_BLASTER);assert(!BodyDormant(n)&&e->health==0&&!e->takedamage);
 Command(0,"jar_property","controller_rearm");assert(e->health==100&&e->takedamage);Command(0,"jar_property","controller_count_0");assert(!e->health&&!e->takedamage);
 Command(0,"jar_property","controller_usablemode");assert((e->spawnflags&8)&&e->wait==1);GlobalUse(e,p,p);assert(!BodyDormant(n)&&e->genericValue11==2&&!(e->r.svFlags&SVF_PLAYER_USABLE));
 int deadline=e->nextthink;GlobalUse(e,p,p);assert(e->nextthink==deadline);Command(0,"jar_property","controller_enabled");StationTick(e,1100);assert(e->genericValue11==2);Command(0,"jar_property","controller_enabled");StationTick(e,FRAMETIME);assert(!e->genericValue11&&(e->r.svFlags&SVF_PLAYER_USABLE));
 Command(0,"jar_property","controller_delay_0");GlobalUse(e,p,p);assert(e->genericValue11==3&&!e->think);Command(0,"jar_property","controller_rearm");assert(!e->genericValue11);Command(0,"jar_property","controller_playeruse");assert(!(e->r.svFlags&SVF_PLAYER_USABLE));
 const char*bad[]={"controller_count_-1","controller_count_2147483648","controller_delay_-1","controller_charge_3","controller_visible"};jarControllerState_t before,after;ControllerState(n,&before);for(int j=0;j<5;++j)Command(0,"jar_property",bad[j]);ControllerState(n,&after);assert(!memcmp(&before,&after,sizeof(before)));
 // Direct NPC-only and current player consent apply; links remain usable.
 Command(0,"jar_property","controller_usablemode");Command(0,"jar_property","controller_npconly");GlobalUse(e,p,p);assert(!BodyDormant(n));GlobalUse(e,relay,p);assert(BodyDormant(n));Command(0,"jar_property","controller_visible");
 gentity_t*npc=CopySpawn(2,0,1100);CopyReady(npc);GlobalUse(e,npc,npc);assert(BodyDormant(n));Command(0,"jar_property","controller_npconly");players[0].noTools=1;GlobalUse(e,p,p);assert(BodyDormant(n));players[0].noTools=0;GlobalUse(e,p,p);assert(!BodyDormant(n));G_FreeEntity(npc);
 // Waiting-to-reappear, cooldown and spent states retain native timing through
 // copies/JRW17. Restore never replays outputs or borrows a reused activator.
 for(int state=1;state<=3;++state){
  jarControllerState_t s={0};s.flags=16|64|(state>1?8:0);s.active=state>1;s.repeats=state;s.waitMs=state>1?1000:0;s.count=75;s.initial=100;assert(UsableApply(n,&s));if(state==2)e->nextthink=level.time+321;
  CopyAssembly(p,n);assert(CopyControllersValid(&copies[0]));static jarWorld_t world,decoded;static byte data[WORLD_BYTES];assert(WorldCapture(&world));int length=WorldEncode(&world,data);assert(WorldDecode(data,length,&decoded));
  int index=-1;for(int i=0;i<world.copy.count;++i)if(world.copy.node[i].id==jarMPCatalog[objects[n].catalog-1].id)index=i;assert(index>=0&&world.copy.node[index].controller.repeats==state);if(state==2)assert(world.copy.node[index].controllerRemaining==321);
  decoded.copy.node[index].controllerRemaining=state==3?1:0;assert(!CopyControllersValid(&decoded.copy));assert(WorldDecode(data,length,&decoded));assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
  e=NULL;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==29){e=&g_entities[i];n=i;}assert(e&&e->genericValue11==state&&e->health==75&&!e->activator);
  if(state==1){assert(BodyDormant(n));StationTick(e,FRAMETIME);assert(!BodyDormant(n)&&!e->genericValue11);}
  if(state==2){assert(e->nextthink-level.time==321);StationTick(e,321);assert(!e->genericValue11);}objects[n].owner=1;ControllerSelect(e);
 }
 assert(WorldClear());
 // Hiding suspends both rigid and mixed attachments without deleting them.
 // Showing restores those links, and deleting the body cleans them normally.
 for(int rigid=0;rigid<=1;++rigid){
  RigidFixture();rigidEnabled.integer=rigid;e=ControllerAt("func_usable",0,100);n=e->s.number;gentity_t*b=CopySpawn(1,0,300);int bn=b->s.number;objects[n].noGravity=objects[bn].noGravity=1;objects[n].frozen=0;
  gentity_t*rope=ForceLink(0,n,bn,100);int rn=rope->s.number;int joint=JointMake(n,bn,1);imports.Trace=RigidTrace;
  GlobalUse(e,p,p);assert(BodyDormant(n));VectorCopy(e->r.currentOrigin,at);ActorTick();assert(rope->inuse&&g_entities[joint].inuse&&!PhysicsRope(rn)&&!PhysicsRope(joint)&&Distance(at,e->r.currentOrigin)<.001f&&!VectorLength(objects[bn].velocity));
  GlobalUse(e,p,p);assert(!BodyDormant(n));ActorTick();assert(rope->inuse&&g_entities[joint].inuse&&!rigidFailed);if(rigid)assert(PhysicsRope(rn)&&PhysicsRope(joint));G_FreeEntity(e);assert(!rope->inuse&&!g_entities[joint].inuse);assert(WorldClear());
 }
 PhysicsReset();puts("MP usable: native toggle/button callbacks, linked outputs, blocked return, Box3D/fallback dormancy and attachments, health callbacks, consent, typed edits/Undo and JRW17 pending states passed.");
}
