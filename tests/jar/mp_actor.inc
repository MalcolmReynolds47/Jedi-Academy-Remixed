// Actual MP actor lifecycle and command paths; native animation/snapshot imports
// are bounded fixture adapters, not a graphical-client acceptance test.
static gclient_t npcClients[2];
void NPC_Think(gentity_t *npc){assert(0 && "The actor fixture prepares native commands without running NPC AI.");}
static void ActorFixture(void){
 bg_numItems=3;bg_itemlist[2].classname="weapon_toolgun";bg_itemlist[2].giType=IT_WEAPON;bg_itemlist[2].giTag=WP_TOOLGUN;
 memset(g_entities,0,sizeof(g_entities));memset(clients,0,sizeof(clients));memset(npcClients,0,sizeof(npcClients));
 memset(&level,0,sizeof(level));level.num_entities=MAX_CLIENTS;level.time=1000;blocked=0;JAR_MP_Init();sharedEditing.integer=worldEditing.integer=0;cleanupDisconnect.integer=1;
 for(int c=0;c<3;++c){gentity_t*p=&g_entities[c];p->s.number=c;p->s.eType=ET_PLAYER;p->inuse=qtrue;p->health=100;p->client=&clients[c];
  clients[c].pers.connected=CON_CONNECTED;clients[c].sess.sessionTeam=TEAM_FREE;clients[c].ps.clientNum=c;
  clients[c].ps.stats[STAT_HEALTH]=100;clients[c].ps.fd.forcePower=100;clients[c].ps.viewheight=24;p->playerState=&clients[c].ps;
 }
}
static gentity_t *ActorNPC(int owner,int custom){
 gentity_t*e=G_Spawn();e->client=&npcClients[custom];e->health=e->client->ps.stats[STAT_HEALTH]=100;
 e->playerState=&e->client->ps;
 e->client->ps.fd.forcePower=100;e->client->ps.clientNum=e->s.number;e->s.eType=ET_NPC;e->localAnimIndex=custom?2:0;
 if(owner>=0){Track(e,owner,-1);}vec3_t pos={100,0,40};Move(e,pos);return e;
}
static void ActorArm(int c,int n){aimed=n;clients[c].ps.weapon=WP_PHYSGUN;clients[c].pers.cmd.buttons=BUTTON_ATTACK;players[c].freezeLatch=0;JAR_MP_Fire(&g_entities[c],qfalse);}
static void ActorTick(void){level.previousTime=level.time;level.time+=25;JAR_MP_Frame();}
static void NPCThrowCases(void){
 ActorFixture();
 gentity_t *npc=ActorNPC(0,0);
 npc->NPC=&copyNpcs[npc->s.number];npc->think=NPC_Think;
 playerState_t *ps=&npc->client->ps;
 ps->gravity=800;ps->weapon=WP_BLASTER;
 const vec3_t launch={640,24,380};
 const int n=npc->s.number;
 ActorArm(0,n);
 VectorCopy(launch,objects[n].velocity);
 clients[0].pers.cmd.buttons=0;
 ActorTick();
 assert(actors[n].thrown&&!actors[n].active&&players[0].held==ENTITYNUM_NONE);
 assert(VectorCompare(ps->velocity,launch)&&npc->nextthink==level.time+1);
 assert(ps->commandTime==level.time-JAR_NPC_COMMAND_TIME_OFFSET);
 for(int i=0;i<4;++i){
  level.time+=25;
  usercmd_t cmd;
  memset(&cmd,0xff,sizeof(cmd));
  assert(JAR_MP_NPCThrowCommand(npc,&cmd));
  assert(cmd.serverTime==ps->commandTime+25&&npc->nextthink==level.time+1);
  assert(!cmd.forwardmove&&!cmd.rightmove&&!cmd.upmove&&!cmd.buttons&&cmd.weapon==WP_BLASTER);
  assert(VectorCompare(ps->velocity,launch));
  ps->commandTime=cmd.serverTime;
 }
 usercmd_t cmd={0};
 ps->groundEntityNum=ENTITYNUM_WORLD;
 assert(!JAR_MP_NPCThrowCommand(npc,&cmd)&&!actors[n].thrown);
 ActorArm(0,n);EndActor(n,qtrue);assert(actors[n].thrown);
 ActorArm(0,n);assert(!actors[n].thrown&&actors[n].active);
 EndActor(n,qtrue);assert(actors[n].thrown);
 G_FreeEntity(npc);assert(!actors[n].thrown);
 puts("MP NPC Physgun: release momentum, per-frame commands, landing, re-grab and deletion passed.");
}
static void ActorTurnInput(gentity_t*p,float yaw,float pitch){
 usercmd_t*command=&p->client->pers.cmd;
 float yawRemainder=0,pitchRemainder=0;
 command->buttons=BUTTON_ATTACK|BUTTON_USE|BUTTON_JAR_PHYSGUN_TURN;
 command->angles[ROLL]=JAR_PackPhysgunTurn(yaw,pitch,&yawRemainder,&pitchRemainder);
 JAR_MP_ReceivePhysgunInput(p,command,qtrue);
}
static void ActorCases(void){
 ActorFixture();gentity_t*npc=ActorNPC(0,0);int n=npc->s.number;
 JAR_MP_Fire(npc,qfalse);assert(!actors[n].active);
 ActorArm(1,n);assert(players[1].held==ENTITYNUM_NONE); // Ownership, then shared permission.
 sharedEditing.integer=1;ActorArm(1,n);assert(players[1].held==n);Drop(1,qfalse);sharedEditing.integer=0;
 VectorSet(physgunFixtureHit,84,0,24);imports.Trace=PhysgunFixtureTrace;
 ActorArm(0,n);imports.Trace=Trace;
 assert(players[0].held==n&&actors[n].active&&npc->client->ps.pm_type==PM_FREEZE);
 assert(npc->s.eType==ET_NPC&&npc->s.number==n&&npc->client->ps.torsoAnim==BOTH_CHOKE3);
 ActorArm(2,n);assert(players[2].held==ENTITYNUM_NONE);
 for(int i=0;i<200;++i)ActorTick();
 assert(npc->health==100&&npc->client->ps.stats[STAT_HEALTH]==100&&npc->client->ps.fd.forcePower==100);
 assert(npc->client->ps.fd.forcePowersActive==0&&npc->client->ps.fd.forceGripBeingGripped==0);
 clients[0].ps.viewangles[YAW]=30;for(int i=0;i<3;++i)ActorTick();
 assert(fabsf(npc->r.currentAngles[YAW]-30)<.01f&&fabsf(npc->client->ps.viewangles[YAW]-30)<.01f);
 assert(VectorLength(objects[n].velocity)>0&&VectorLength(objects[n].velocity)<=1500.01f);
 clients[0].pers.cmd.buttons=0;ActorTick();assert(!actors[n].active&&npc->client->ps.pm_type==PM_NORMAL&&VectorLength(npc->client->ps.velocity)>0);
 assert(npc->client->ps.forceHandExtend==HANDEXTEND_NONE&&npc->client->ps.legsTimer==0);
 // Hull obstruction produces no teleport and no stale fling velocity.
 ActorArm(0,n);blocked=1;vec3_t position;VectorCopy(npc->r.currentOrigin,position);ActorTick();assert(VectorCompare(position,npc->r.currentOrigin)&&VectorLength(objects[n].velocity)==0);blocked=0;
 JAR_MP_Fire(&g_entities[0],qtrue);assert(players[0].held==ENTITYNUM_NONE&&objects[n].frozen&&actors[n].active);
 ActorTick();assert(VectorCompare(position,npc->r.currentOrigin));
 aimed=n;Command(0,"jar_select","");Command(0,"jar_property","rotate");assert(npc->r.currentAngles[YAW]==45);
 Command(0,"jar_property","release");assert(!actors[n].active&&!objects[n].frozen);
 // Toolgun freeze/release is available for actors without exposing player deletion.
 players[0].tool=0;Tool(&g_entities[0],qfalse);assert(actors[n].active);Tool(&g_entities[0],qtrue);assert(!actors[n].active);
 Command(0,"jar_property","freeze");assert(actors[n].active);sharedEditing.integer=0;JAR_MP_ClientReset(0);assert(!actors[n].active);
 // Map actors have a separate server permission; nonhumanoids keep their own pose.
 gentity_t*mapNPC=ActorNPC(-1,1);int mapNum=mapNPC->s.number;mapNPC->client->ps.torsoAnim=17;
 mapEditing.integer=0;ActorArm(0,mapNum);assert(players[0].held==ENTITYNUM_NONE);
 mapEditing.integer=1;ActorArm(0,mapNum);assert(players[0].held==mapNum&&mapNPC->client->ps.torsoAnim==17);Drop(0,qfalse);
 ActorArm(0,n);G_FreeEntity(npc);assert(players[0].held==ENTITYNUM_NONE&&!actors[n].active);gentity_t*reuse=G_Spawn();assert(reuse->s.number==n&&!actors[n].active);

 ActorFixture();ActorArm(1,0);assert(players[1].held==0);Drop(1,qfalse);assert(!actors[0].active);
 gentity_t*target=&g_entities[1];
 ActorArm(0,1);assert(players[0].held==1&&target->client->ps.pm_type==PM_FREEZE);
 usercmd_t cmd={0};cmd.serverTime=level.time+10000;cmd.buttons=BUTTON_ATTACK|BUTTON_USE;cmd.forwardmove=127;
 assert(JAR_MP_ActorThink(target,&cmd)&&target->client->ps.commandTime==level.time+200);
 assert(cmd.buttons==(BUTTON_ATTACK|BUTTON_USE)&&cmd.forwardmove==127); // No persistent input mutation.
 ActorArm(2,1);assert(players[2].held==ENTITYNUM_NONE); // One holder.
 Command(1,"jar_release","");assert(!actors[1].active&&players[0].held==ENTITYNUM_NONE);
 ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);level.time+=1000;ActorArm(0,1);assert(players[0].held==1);
 // Revocation bypasses the ordinary request throttle and works while frozen.
 players[1].nextRequest=level.time+9999;Command(1,"jar_allowTools","0");assert(players[1].noTools&&!actors[1].active);
 ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);Command(1,"jar_allowTools","0;quit");assert(players[1].noTools);
 Command(1,"jar_allowTools","1");assert(!players[1].noTools);ActorArm(0,1);assert(players[0].held==1);
 playerTools.integer=0;ActorTick();assert(!actors[1].active&&target->client->ps.pm_type==PM_NORMAL);playerTools.integer=1;
 ActorArm(0,1);JAR_MP_Fire(&g_entities[0],qtrue);assert(actors[1].active&&objects[1].frozen);Command(1,"jar_release","");assert(!actors[1].active);
 level.time+=1000;aimed=1;players[0].tool=1;Tool(&g_entities[0],qfalse);assert(target->inuse); // Remove cannot free player slots.
 // Self, spectator, death, vehicle/duel/native Force-grip conflicts cannot be grabbed.
 ActorArm(0,0);assert(players[0].held==ENTITYNUM_NONE);
 clients[1].sess.sessionTeam=TEAM_SPECTATOR;ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);clients[1].sess.sessionTeam=TEAM_FREE;
 clients[1].ps.m_iVehicleNum=99;ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);clients[1].ps.m_iVehicleNum=0;
 clients[1].ps.duelInProgress=qtrue;ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);clients[1].ps.duelInProgress=qfalse;
 clients[1].ps.fd.forceGripBeingGripped=level.time+50;ActorArm(0,1);assert(players[0].held==ENTITYNUM_NONE);clients[1].ps.fd.forceGripBeingGripped=0;
 ActorArm(0,1);target->health=0;target->client->ps.legsAnim=BOTH_DEATH1;ActorTick();assert(!actors[1].active&&target->client->ps.legsAnim==BOTH_DEATH1);target->health=100;
 ActorArm(0,1);players[2].selected=players[2].first=1;JAR_MP_ClientReset(1);assert(!actors[1].active&&players[0].held==ENTITYNUM_NONE);
 assert(players[2].selected==ENTITYNUM_NONE&&players[2].first==ENTITYNUM_NONE);
 Command(2,"jar_property","freeze");assert(!actors[1].active);
 ActorArm(0,1);JAR_MP_Disconnect(1);assert(!actors[1].active&&players[0].held==ENTITYNUM_NONE&&!players[1].noTools);
 ActorArm(0,1);JAR_MP_Disconnect(0);assert(!actors[1].active&&target->client->ps.pm_type==PM_NORMAL);
 ActorArm(0,1);sandbox.integer=0;ActorTick();assert(!actors[1].active);sandbox.integer=1;
 ActorArm(0,1);level.intermissiontime=level.time;ActorTick();assert(!actors[1].active);level.intermissiontime=0;
 // Taking a player releases what that player was holding; no control cycles.
 npc=ActorNPC(1,0);ActorArm(1,npc->s.number);assert(players[1].held==npc->s.number);
 ActorArm(0,1);assert(players[0].held==1&&players[1].held==ENTITYNUM_NONE&&!actors[npc->s.number].active);
 Command(1,"jar_property","freeze");assert(!actors[0].active);
 JAR_MP_Init();assert(!actors[1].active&&players[0].held==ENTITYNUM_NONE); // Map reset clears session control.
 puts("MP actors: holds, native-pose adapter, snapshots, rotation/fling, freeze/release, permissions, self-release and lifecycle passed.");
}
