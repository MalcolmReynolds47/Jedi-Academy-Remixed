// Production tool requests, native velocity handoff and attachment lifecycle.
// Movement/collision imports are adapters; real-server motion is tested separately.
static gentity_t*ForceLink(int owner,int a,int b,float length){
 gentity_t*r=CopyLink(owner,a,b);jarObject_t*o=&objects[r->s.number];VectorClear(o->anchorA);VectorClear(o->anchorB);o->length=length;return r;
}
static void ForceTool(int c,int target,int tool,qboolean alt){players[c].tool=tool;aimed=target;Tool(&g_entities[c],alt);}
static void ForceCases(void){
 // A rope transmits equal/opposite impulses, apportioned by actual prop/actor mass.
 ActorFixture();gentity_t*p=CopySpawn(1,0,0),*npc=CopySpawn(2,0,200);CopyReady(npc);
 int a=p->s.number,b=npc->s.number;objects[a].mass=20;objects[b].mass=200;gentity_t*r=ForceLink(0,a,b,100);
 int snapshots=snapshotCalls;npc->s.event=EV_FIRE_WEAPON;npc->client->ps.entityEventSequence=17;
 npc->client->ps.velocity[0]=100;ForceFrame(.025f);
 assert(snapshotCalls==snapshots&&npc->s.event==EV_FIRE_WEAPON&&npc->client->ps.entityEventSequence==17);
 assert(VectorCompare(npc->client->ps.velocity,npc->s.pos.trDelta));
 assert(objects[a].velocity[0]>0&&fabsf(npc->client->ps.velocity[0]-objects[a].velocity[0])<.01f);
 assert(fabsf(objects[a].velocity[0]*20+npc->client->ps.velocity[0]*200-20000)<.01f);
 assert(npc->s.eType==ET_NPC&&npc->health==100&&npc->client->ps.forceHandExtend==HANDEXTEND_NONE&&!actors[b].active);
 ForceRopeVisuals();assert(VectorCompare(r->s.pos.trBase,p->r.currentOrigin)&&VectorCompare(r->s.origin2,npc->r.currentOrigin));
 // Slack ropes never push endpoints apart, and frozen endpoints are immovable.
 VectorClear(objects[a].velocity);VectorClear(npc->client->ps.velocity);objects[r->s.number].length=300;ForceFrame(.05f);assert(VectorLength(objects[a].velocity)==0&&VectorLength(npc->client->ps.velocity)==0);
 objects[r->s.number].length=100;objects[a].frozen=1;VectorSet(objects[a].velocity,999,0,0);npc->client->ps.velocity[0]=100;
 ForceFrame(.05f);assert(objects[a].velocity[0]==999&&fabsf(npc->client->ps.velocity[0])<.01f);objects[a].frozen=0;
 ForceDetach(a);assert(!r->inuse);objects[a].thruster=0;
 // Real Toolgun path admits NPC/player rope anchors, applies permission to BOTH
 // ends, and rechecks first-anchor consent if it changes between clicks.
 ForceTool(0,1,4,qfalse);assert(players[0].first==1);ForceTool(0,b,4,qfalse);int rope=Last(0);assert(g_entities[rope].s.generic1==251);
 Command(1,"jar_release","");assert(!g_entities[rope].inuse);level.time+=1000;
 ForceTool(0,1,4,qfalse);assert(players[0].first==1);Command(1,"jar_allowTools","0");ForceTool(0,b,4,qfalse);assert(players[0].first==ENTITYNUM_NONE);Command(1,"jar_allowTools","1");
 ForceTool(0,ENTITYNUM_WORLD,4,qfalse);ForceTool(0,1,4,qfalse);rope=Last(0);assert(g_entities[rope].s.generic1==251);JAR_MP_ClientReset(1);assert(!g_entities[rope].inuse);
 // Thrusters use the selected entity's local up axis, retain native poses,
 // hand off velocity rather than moving through walls, and disable with alt fire.
 ForceTool(0,b,2,qfalse);assert(objects[b].thruster&&objects[b].thrustBy==1);
 VectorClear(npc->client->ps.velocity);VectorSet(npc->r.currentAngles,90,0,0);vec3_t old;VectorCopy(npc->r.currentOrigin,old);ForceFrame(.1f);
 assert(npc->client->ps.velocity[0]<-47.9f&&fabsf(npc->client->ps.velocity[2])<.001f&&VectorCompare(old,npc->r.currentOrigin));
 assert(npc->client->ps.legsAnim==0&&npc->health==100);ForceTool(0,b,2,qtrue);assert(!objects[b].thruster);
 ForceTool(0,1,2,qfalse);assert(objects[1].thruster);Command(1,"jar_allowTools","0");assert(!objects[1].thruster);Command(1,"jar_allowTools","1");
 ForceTool(0,1,2,qfalse);playerTools.integer=0;ForceFrame(.05f);assert(!objects[1].thruster);playerTools.integer=1;
 // Context detaches selected attachments; deletion/reuse cannot inherit them.
 ForceTool(0,b,2,qfalse);r=ForceLink(0,a,b,100);aimed=b;Command(0,"jar_select","");Command(0,"jar_property","detach");assert(!objects[b].thruster&&!r->inuse);
 r=ForceLink(0,a,b,100);G_FreeEntity(npc);assert(!r->inuse);npc=CopySpawn(2,0,200);assert(npc->s.number==b&&!objects[b].thruster);CopyReady(npc);
 // Map actor permissions and shared-edit revocation apply continuously.
 gentity_t*map=ActorNPC(-1,0);ForceTool(0,map->s.number,2,qfalse);assert(objects[map->s.number].thruster);mapEditing.integer=0;ForceFrame(.05f);assert(!objects[map->s.number].thruster);mapEditing.integer=1;
 sharedEditing.integer=1;ForceTool(1,b,2,qfalse);r=ForceLink(1,a,b,100);sharedEditing.integer=0;ForceFrame(.05f);assert(!objects[b].thruster&&!r->inuse);
 // No force while held/frozen, paused or with zero elapsed time.
 ForceTool(0,b,2,qfalse);VectorClear(npc->client->ps.velocity);BeginActor(&g_entities[0],npc);objects[b].frozen=1;ForceFrame(.05f);assert(VectorLength(npc->client->ps.velocity)==0);
 EndActor(b,qfalse);sandbox.integer=0;ForceFrame(.05f);assert(VectorLength(npc->client->ps.velocity)==0);sandbox.integer=1;
 level.intermissiontime=1;ForceFrame(.05f);assert(VectorLength(npc->client->ps.velocity)==0);level.intermissiontime=0;ForceFrame(0);assert(VectorLength(npc->client->ps.velocity)==0);
 // Pending native copies keep their ropes and force settings inert until ready.
 ActorFixture();p=CopySpawn(1,0,0);npc=CopySpawn(2,0,200);CopyReady(npc);a=p->s.number;b=npc->s.number;ForceTool(0,b,2,qfalse);ForceLink(0,a,b,100);CopyAssembly(&g_entities[0],a);assert(copies[0].count==2&&copies[0].node[1].thruster);
 PasteAssembly(&g_entities[0]);unsigned group=objects[players[0].selected].order;int pending=-1,newRope=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].order==group){if(objects[n].copyPending)pending=n;if(g_entities[n].s.generic1==251)newRope=n;}
 assert(pending>=0&&newRope>=0);ForceFrame(.025f);assert(g_entities[newRope].inuse&&objects[pending].thruster&&VectorLength(g_entities[pending].client->ps.velocity)==0);
 CopyReady(&g_entities[pending]);ForceFrame(.025f);assert(g_entities[pending].client->ps.velocity[2]<0);
 // Playerless persistent worlds retain NPC attachments, while departing players
 // lose tools on their own bodies before connection slots can be reused.
 cleanupDisconnect.integer=0;JAR_MP_Disconnect(0);assert(g_entities[b].inuse&&objects[b].owner==MAX_CLIENTS+1&&objects[b].thrustBy==MAX_CLIENTS+1);
 clients[0].pers.connected=CON_DISCONNECTED;ForceFrame(.025f);assert(objects[b].thruster);
 // Boarding suspends attachments until native seats settle. Death removes them.
 const vehicleType_t vehicleTypes[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int kind=0;kind<4;++kind){
  ActorFixture();copyVehicleInfo.type=vehicleTypes[kind];gentity_t*v=CopySpawn(3,0,200);CopyReady(v);int n=v->s.number;objects[n].mass=500;assert(!JAR_MP_VehicleForces(n));ForceTool(0,n,2,qfalse);assert(JAR_MP_VehicleForces(n));r=ForceLink(0,n,ENTITYNUM_WORLD,20);VectorSet(objects[r->s.number].anchorB,500,0,80);
  v->client->ps.velocity[0]=-100;
  ForceFrame(.025f);assert(v->client->ps.velocity[0]>-1&&v->client->ps.velocity[2]<0&&v->s.eType==ET_NPC);
  v->m_pVehicle->m_iBoarding=-1;assert(!JAR_MP_VehicleForces(n));vec3_t entering;VectorCopy(v->client->ps.velocity,entering);ForceFrame(.025f);assert(objects[n].thruster&&r->inuse&&v->inuse&&VectorCompare(entering,v->client->ps.velocity));v->m_pVehicle->m_iBoarding=0;ForceDetach(n);
  ForceTool(0,n,2,qfalse);r=ForceLink(0,n,ENTITYNUM_WORLD,20);v->health=0;ForceFrame(.025f);assert(!objects[n].thruster&&!r->inuse);
 }
 // A fixed-time thruster pulse has equal impulse at 10, 20, 40 and 120 Hz.
 for(int hz=10;hz<=120;hz=hz==10?20:hz==20?40:hz==40?120:121){
  ActorFixture();Command(0,"jar_setting","force_3000");npc=ActorNPC(0,0);ForceTool(0,npc->s.number,2,qfalse);for(int i=0;i<hz;++i)ForceFrame(1.f/hz);assert(fabsf(npc->client->ps.velocity[2]+1200)<.05f);
 }
 // Let an actor cross the remaining slack in one step. A taut cable keeps
 // the full length available and stops outward motion without bouncing.
 float resting[4];const int rates[]={10,20,40,120};
 for(int k=0;k<4;++k){
  ActorFixture();npc=ActorNPC(0,0);Move(npc,(vec3_t){60,0,0});npc->client->ps.velocity[0]=1400;
  ForceLink(0,npc->s.number,ENTITYNUM_WORLD,100);
  float dt=1.f/rates[k];
  for(int i=0;i<rates[k];++i){
   level.previousTime=level.time;level.time+=1000/rates[k];
   VectorMA(npc->client->ps.origin,dt,npc->client->ps.velocity,npc->client->ps.origin);
   VectorCopy(npc->client->ps.origin,npc->r.currentOrigin);ForceFrame(dt);JointFrame(dt,qfalse);
   assert(VectorLength(npc->r.currentOrigin)<=100.02f&&npc->client->ps.velocity[0]>=-.01f);
  }
  resting[k]=npc->r.currentOrigin[0];assert(fabsf(resting[k]-100)<.02f&&fabsf(npc->client->ps.velocity[0])<.02f);
 }
 puts("MP native ropes/thrusters: mass, timestep, consent, vehicles, copies, native handoff and lifecycle passed.");
}
