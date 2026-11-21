// Production tools, rigid/native solvers, permissions and versioned persistence.
static void JointClick(int client,int endpoint,const vec3_t point,const vec3_t normal){aimed=endpoint;VectorCopy(point,attachPoint);VectorCopy(normal,attachNormal);imports.Trace=AttachTrace;Tool(&g_entities[client],qfalse);}
static int JointMake(int first,int second,int kind){
 players[0].tool=kind+4;players[0].first=ENTITYNUM_NONE;vec3_t point={100,0,80},other={200,0,80};
 if(first!=ENTITYNUM_WORLD)VectorCopy(g_entities[first].r.currentOrigin,point);
 if(second==ENTITYNUM_WORLD)VectorMA(point,-20,(float[3]){0,0,1},other);
 if(second!=ENTITYNUM_WORLD)VectorCopy(g_entities[second].r.currentOrigin,other);
 ToolTarget(&g_entities[0],qfalse,&(trace_t){.fraction=.5f,.entityNum=first,
  .endpos={point[0],point[1],point[2]},.plane={.normal={0,0,1}}});
 ToolTarget(&g_entities[0],qfalse,&(trace_t){.fraction=.5f,.entityNum=second,
  .endpos={other[0],other[1],other[2]},.plane={.normal={0,0,1}}});
 VectorCopy(other,attachPoint);
 int n=Last(0);assert(JointLink(n)&&objects[n].jointKind==kind);return n;
}
static float JointError(int n){vec3_t a,b;Endpoint(objects[n].first-1,objects[n].anchorA,a);Endpoint(objects[n].second-1,objects[n].anchorB,b);return Distance(a,b);}
static void ConstraintLoadCases(void){
 // The body starts taut well above the floor with a large outward velocity.
 RigidFixture();
 gentity_t *prop=CopySpawn(1,0,100);
 int slot=prop->s.number;
 Move(prop,(vec3_t){100,0,256});
 objects[slot].mass=25000;
 objects[slot].velocity[2]=-1400;
 int rope=ForceLink(0,slot,ENTITYNUM_WORLD,128)->s.number;
 VectorSet(objects[rope].anchorB,100,0,384);
 float maximum=0;
 for(int i=0;i<30;++i){RigidSteps(1);maximum=fmaxf(maximum,JointError(rope));}
 // Native actors advance before constraints in a server frame. Simulate a
 // 100 ms frame that carries a heavy NPC beyond a taut rope.
 RigidFixture();
 gentity_t *actor=CopySpawn(2,0,300);
 CopyReady(actor);
 slot=actor->s.number;
 Move(actor,(vec3_t){230,0,120});
 objects[slot].mass=25000;
 VectorSet(actor->client->ps.velocity,1300,0,0);
 rope=ForceLink(0,slot,ENTITYNUM_WORLD,100)->s.number;
 VectorSet(objects[rope].anchorB,0,0,120);
 level.previousTime=level.time;level.time+=100;
 JAR_MP_Frame();
 const float nativeDistance=JointError(rope);
 assert(fabsf(actor->client->ps.velocity[0])<.02f);
 // A rope connected to a weld must resolve both constraints in the same
 // projection, including when a fast native actor pulls a light welded prop.
 RigidFixture();actor=CopySpawn(2,0,300);CopyReady(actor);Move(actor,(vec3_t){430,0,120});
 gentity_t *light=CopySpawn(1,0,200),*heavy=CopySpawn(1,0,100);
 Move(light,(vec3_t){200,0,120});Move(heavy,(vec3_t){100,0,120});
 int lightSlot=light->s.number,heavySlot=heavy->s.number;
 objects[lightSlot].mass=25;objects[heavySlot].mass=25000;objects[actor->s.number].mass=25000;
 objects[lightSlot].noGravity=objects[heavySlot].noGravity=1;
 int mixedWeld=JointMake(lightSlot,heavySlot,1);
 rope=ForceLink(0,lightSlot,actor->s.number,100)->s.number;
 actor->client->ps.velocity[0]=1300;
 level.previousTime=level.time;level.time+=100;JAR_MP_Frame();
 const float mixedDistance=JointError(rope),mixedWeldError=JointError(mixedWeld);
 assert(JointNative(rope)&&JointNative(mixedWeld)&&mixedDistance<=100.02f&&mixedWeldError<.02f);
 assert(heavy->r.currentOrigin[0]>100&&actor->r.currentOrigin[0]<430);
 // Fast linear and angular impulses must not deform a heavy fixed weld or
 // pull a hinge off its pivot. The hinge can still turn about the chosen axis.
 float jointErrors[2]={0};
 for(int kind=1;kind<=2;++kind){
  RigidFixture();prop=CopySpawn(1,0,100);slot=prop->s.number;
  objects[slot].mass=25000;
  VectorSet(objects[slot].velocity,850,0,-1200);
  VectorSet(objects[slot].angular,1,0,3);
  int link=JointMake(slot,ENTITYNUM_WORLD,kind);
  for(int i=0;i<30;++i){RigidSteps(1);jointErrors[kind-1]=fmaxf(jointErrors[kind-1],JointError(link));}
  assert(fabsf(prop->r.currentAngles[PITCH])<.1f&&fabsf(prop->r.currentAngles[ROLL])<.1f);
  assert(kind==1?fabsf(prop->r.currentAngles[YAW])<.1f:fabsf(prop->r.currentAngles[YAW])>20);
 }
 fprintf(stderr,"MP constraint stress: maximum Box3D length %.4f / 128, native and mixed lengths %.4f and %.4f / 100, weld, mixed weld and hinge errors %.4f, %.4f and %.4f.\n",
  maximum,nativeDistance,mixedDistance,jointErrors[0],mixedWeldError,jointErrors[1]);
 assert(maximum<=128.02f&&nativeDistance<=100.02f&&jointErrors[0]<.03f&&jointErrors[1]<.03f);
}
static void JointCases(void){
 // Rigid welds preserve the initial relative transform under a real impulse.
 RigidFixture();gentity_t*a=CopySpawn(1,0,100),*b=CopySpawn(1,0,200);int x=a->s.number,y=b->s.number;objects[x].noGravity=objects[y].noGravity=1;
 int link=JointMake(x,y,1);assert(g_entities[link].r.svFlags&SVF_NOCLIENT);assert(PhysicsPrepare()&&PhysicsRope(link)&&!JointNative(link));
 vec3_t p;VectorCopy(a->r.currentOrigin,p);assert(JAR_RigidImpulse(x,(float[3]){0,2000,0},p));RigidSteps(50);
 assert(JointError(link)<.1f&&Distance(a->r.currentOrigin,b->r.currentOrigin)>99.8f&&Distance(a->r.currentOrigin,b->r.currentOrigin)<100.2f);
 assert(fabsf(AngleSubtract(a->r.currentAngles[YAW],b->r.currentAngles[YAW]))<.1f);
 // Copy captures immutable frames and the graph; Undo removes the pasted group.
 CopyAssembly(&g_entities[0],x);assert(copies[0].count==2&&copies[0].ropes==1&&copies[0].rope[0].kind==1);jarCopyRope_t record=copies[0].rope[0];
 G_FreeEntity(a);assert(!g_entities[link].inuse);G_FreeEntity(b);assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){300,0,100},NULL,NULL));
 int restored=Last(0);assert(g_entities[restored].inuse);int restoredLink=-1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(JointLink(n))restoredLink=n;
 assert(restoredLink>=0&&!memcmp(objects[restoredLink].jointA,record.ra,sizeof(record.ra)));RigidSteps(12);assert(JointError(restoredLink)<.2f);Undo(&g_entities[0],qfalse);assert(!JointLink(restoredLink));
 // A world weld arrests gravity; a hinge allows only the chosen axial rotation.
 RigidFixture();a=CopySpawn(1,0,100);x=a->s.number;link=JointMake(x,ENTITYNUM_WORLD,1);
 assert(Distance(objects[link].anchorB,attachPoint)<.01f&&fabsf(objects[link].anchorA[2]+20)<.01f);
 VectorCopy(a->r.currentOrigin,p);RigidSteps(100);assert(Distance(p,a->r.currentOrigin)<.03f&&JointError(link)<.03f);
 Undo(&g_entities[0],qfalse);assert(a->inuse&&!JointLink(link));RigidSteps(100);assert(a->r.currentOrigin[2]<20);
 RigidFixture();a=CopySpawn(1,0,100);x=a->s.number;objects[x].noGravity=1;objects[x].angular[2]=1;link=JointMake(ENTITYNUM_WORLD,x,2);RigidSteps(60);
 assert(fabsf(a->r.currentAngles[YAW])>20&&fabsf(a->r.currentAngles[PITCH])<.05f&&fabsf(a->r.currentAngles[ROLL])<.05f&&JointError(link)<.1f);
 jarObject_t immutable=objects[link];objects[x].frozen=1;RigidSteps(10);objects[x].frozen=0;RigidSteps(10);assert(!memcmp(immutable.jointA,objects[link].jointA,sizeof(immutable.jointA))&&!memcmp(immutable.jointB,objects[link].jointB,sizeof(immutable.jointB)));
 // Save format 6 retains both frames, rejects corrupt frames transactionally,
 // and still reads old v4 ropes without interpreting trailing memory as joints.
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));assert(decoded.copy.rope[0].kind==2);
 const int offsets[]={36,40,52};const unsigned invalid[]={3,0x7fc00000u,0};
 for(int k=0;k<3;++k){memcpy(bad,bytes,size);at=WORLD_HEADER+saved.copy.count*WORLD_NODE+offsets[k];WorldPut(bad,&at,invalid[k]);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);}
 assert(WorldClear()&&PlaceCopy(NULL,&saved.copy,vec3_origin,saved.velocity,saved.angular));RigidSteps(30);int restoredHinge=-1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(JointLink(n))restoredHinge=n;assert(restoredHinge>=0&&JointError(restoredHinge)<.1f);
 RigidFixture();a=CopySpawn(1,0,100);x=a->s.number;ForceLink(0,x,ENTITYNUM_WORLD,50);assert(WorldCapture(&saved));size=WorldEncode(&saved,bytes);memcpy(bad,bytes,WORLD_HEADER+136);memcpy(bad+WORLD_HEADER+136,bytes+WORLD_HEADER+WORLD_NODE,36);at=0;WorldPut(bad,&at,0x3457524au);size=WORLD_HEADER+136+36+4;WorldTestHash(bad,size);assert(WorldDecode(bad,size,&decoded)&&decoded.copy.rope[0].kind==0);
 // A mixed graph uses the native solver as a whole. A frozen NPC is a movable
 // editor anchor without losing its native animation, health or controller.
 RigidFixture();a=CopySpawn(1,0,100);b=CopySpawn(1,0,200);gentity_t*npc=CopySpawn(2,0,300);CopyReady(npc);x=a->s.number;y=b->s.number;int actor=npc->s.number;objects[x].noGravity=objects[y].noGravity=1;
 int rigidLink=JointMake(x,y,1);link=JointMake(y,actor,1);BeginActor(&g_entities[0],npc);objects[actor].frozen=1;RigidSteps(1);
 assert(JointNative(link)&&JointNative(rigidLink)&&!PhysicsRope(link)&&!PhysicsRope(rigidLink));int legs=npc->client->ps.legsAnim,events=snapshotCalls;
 VectorSet(p,330,30,100);Move(npc,p);vec3_t angle={0,25,0};assert(ActorRotate(npc,angle));RigidSteps(50);
 assert(JointError(link)<.1f&&JointError(rigidLink)<.1f&&npc->health==100&&npc->client->ps.legsAnim==legs&&actors[actor].active);
 // Holding a member drives the assembly without turning it into an infinite
 // mass anchor that can tear the graph apart.
 ActorArm(0,x);clients[0].ps.origin[0]=700;RigidSteps(40);assert(JointError(link)<.2f&&JointError(rigidLink)<.2f);clients[0].pers.cmd.buttons=0;RigidSteps(1);
 // Collision rejects the projection instead of teleporting a connected member.
 VectorCopy(a->r.currentOrigin,p);Move(npc,(float[3]){350,30,100});blocked=1;JointFrame(.025f,qtrue);blocked=0;assert(Distance(a->r.currentOrigin,p)<.001f);
 assert(snapshotCalls>=events);EndActor(actor,qfalse);assert(!actors[actor].active&&npc->health==100);
 // Pending native restoration is inert; an unrelated world anchor remains rigid.
 objects[actor].copyPending=1;gentity_t*independent=CopySpawn(1,0,600);int separate=JointMake(independent->s.number,ENTITYNUM_WORLD,1);assert(PhysicsPrepare());assert(!JointNative(separate)&&PhysicsRope(separate));objects[actor].copyPending=0;
 // Native/mixed hinges retain axial freedom, while held actors still obey a
 // world weld. The actor's controller and animation state remain native.
 RigidFixture();a=CopySpawn(1,0,100);npc=CopySpawn(2,0,300);CopyReady(npc);x=a->s.number;actor=npc->s.number;objects[x].noGravity=1;objects[x].angular[2]=1;BeginActor(&g_entities[0],npc);objects[actor].frozen=1;link=JointMake(x,actor,2);RigidSteps(60);
 assert(JointNative(link)&&fabsf(a->r.currentAngles[YAW])>20&&fabsf(a->r.currentAngles[PITCH])<.1f&&fabsf(a->r.currentAngles[ROLL])<.1f&&JointError(link)<.2f);
 RigidFixture();npc=CopySpawn(2,0,100);CopyReady(npc);actor=npc->s.number;link=JointMake(actor,ENTITYNUM_WORLD,1);VectorCopy(npc->r.currentOrigin,p);ActorArm(0,actor);clients[0].ps.origin[0]=700;for(int i=0;i<40;++i)ActorTick();assert(JointError(link)<.2f&&Distance(p,npc->r.currentOrigin)<.2f&&npc->health==100);
 // Player opt-out/self-release/disconnect remove links without damaging players.
 RigidFixture();a=CopySpawn(1,0,100);x=a->s.number;link=JointMake(x,1,1);assert(JointLink(link));assert(!WorldCapture(&saved));jarAutoCapture_t automatic={0};assert(WorldCaptureMode(&saved,&automatic)&&automatic.transient==1&&saved.copy.ropes==0);
 Command(1,"jar_allowTools","0");assert(!JointLink(link)&&g_entities[1].health==100);Command(1,"jar_allowTools","1");link=JointMake(x,1,2);Command(1,"jar_release","");assert(!JointLink(link));level.time+=1100;link=JointMake(x,1,1);JAR_MP_Disconnect(1);assert(!JointLink(link)&&a->inuse);
 // Tool switching and alternate fire cancel pending anchors; ownership is checked
 // again on the second click, and duplicate links do not consume entity slots.
 RigidFixture();a=CopySpawn(1,0,100);b=CopySpawn(1,1,200);x=a->s.number;y=b->s.number;players[0].tool=5;VectorCopy(a->r.currentOrigin,p);JointClick(0,x,p,(float[3]){0,0,1});assert(players[0].first==x);Command(0,"jar_tool","6");assert(players[0].first==ENTITYNUM_NONE);
 JointClick(0,x,p,(float[3]){0,0,1});Tool(&g_entities[0],qtrue);assert(players[0].first==ENTITYNUM_NONE);
 JointClick(0,x,p,(float[3]){0,0,1});VectorCopy(b->r.currentOrigin,p);JointClick(0,y,p,(float[3]){0,0,1});assert(!JointLink(Last(0)));sharedEditing.integer=1;link=JointMake(x,y,1);int live=LiveCount();JointMake(x,y,1);assert(LiveCount()==live);sharedEditing.integer=0;
 ForceFrame(.025f);assert(!JointLink(link));PhysicsReset();imports.Trace=Trace;
 puts("MP Weld/Hinge: rigid motion, axial freedom, mixed graph projection, collision refusal, immutable frames, copy/Undo, JRW1-5, consent and lifecycle passed.");
}
