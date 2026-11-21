// Production MP routing against a controlled swept pillar, plus real Box3D.
static int ropeTestCheckEndpoints;
static int ropeTestA,ropeTestB,ropeTestSolid,ropeTestSweeps;
static int ropeTestPinned=-1;
static void RopeTestTrace(trace_t*t,const vec3_t a,const vec3_t lo,const vec3_t hi,const vec3_t b,int pass,int mask,int capsule,int flags,int lod){
 memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=ENTITYNUM_NONE;VectorCopy(b,t->endpos);
 if(!lo)return;
 if(fabsf(lo[0])<3){++ropeTestSweeps;if(ropeTestCheckEndpoints)assert(!g_entities[ropeTestA].r.linked&&!g_entities[ropeTestB].r.linked);}
 if(pass==ropeTestPinned&&fabsf(lo[0])>=3&&Distance(a,b)>.001f){
  t->fraction=0;t->entityNum=ENTITYNUM_WORLD;VectorCopy(a,t->endpos);VectorSet(t->plane.normal,-1,0,0);return;
 }
 const gentity_t*e=&g_entities[ropeTestSolid];if(pass==ropeTestSolid||!e->inuse||!(e->r.contents&mask)||!e->r.linked)return;
 float enter=0,leave=1,sign=0;int normal=-1,inside=1;
 for(int k=0;k<3;++k){float low=e->r.currentOrigin[k]+e->r.mins[k]-hi[k],high=e->r.currentOrigin[k]+e->r.maxs[k]-lo[k],delta=b[k]-a[k];
  if(a[k]<low||a[k]>high)inside=0;if(fabsf(delta)<.00001f){if(a[k]<low||a[k]>high)return;continue;}
  float x=(low-a[k])/delta,y=(high-a[k])/delta,n=delta>0?-1:1;if(x>y){float v=x;x=y;y=v;}if(x>enter){enter=x;normal=k;sign=n;}leave=fminf(leave,y);
 }
 if(enter>leave||leave<0||enter>1)return;
 if(inside){t->startsolid=t->allsolid=qtrue;t->fraction=0;t->entityNum=ropeTestSolid;VectorCopy(a,t->endpos);return;}
 if(normal>=0&&enter<1){t->fraction=fmaxf(0,enter);t->entityNum=ropeTestSolid;t->plane.normal[normal]=sign;for(int k=0;k<3;++k)t->endpos[k]=a[k]+(b[k]-a[k])*t->fraction;}
}
static void RopeLengthCases(void){
 RigidFixture();imports.EntitiesInBox=FixtureEntitiesInBox;imports.Trace=RopeTestTrace;rigidEnabled.integer=0;
 gentity_t *a=CopySpawn(1,0,0),*b=CopySpawn(1,0,160),*wall=G_Spawn();
 ropeTestA=a->s.number;ropeTestB=b->s.number;ropeTestSolid=wall->s.number;
 wall->r.contents=CONTENTS_SOLID;VectorSet(wall->r.mins,-10,-20,-60);VectorSet(wall->r.maxs,10,20,60);
 Move(a,(vec3_t){0,0,80});Move(b,(vec3_t){160,0,80});Move(wall,(vec3_t){80,0,80});
 int id=ForceLink(0,ropeTestA,ropeTestB,200)->s.number;
 RopePrepare();assert(ropeCache[id].valid&&objects[id].ropePath.count==2);
 const float setLength=ropeCache[id].result.length;
 objects[id].length=setLength;
 Move(a,(vec3_t){-36,0,80});objects[ropeTestA].velocity[0]=-900;
 ropeTestPinned=ropeTestA;level.time+=25;
 JointFrame(.025f,qfalse);ForceRopeVisuals();
 ropeTestPinned=-1;
 assert(ropeCache[id].valid);
 fprintf(stderr,"MP routed rope with one sweep blocked: full path %.4f / %.4f, far end x %.4f.\n",ropeCache[id].result.length,setLength,b->r.currentOrigin[0]);
 assert(ropeCache[id].result.length<=setLength+.02f&&b->r.currentOrigin[0]<130);
 const float before=b->r.currentOrigin[0];
 // Keep driving one end through several server frames. The other end must
 // keep following the route toward the pillar, even behind the corner.
 for(int i=0;i<4;++i){
  objects[ropeTestA].velocity[0]=-80;ActorTick();
  assert(ropeCache[id].valid&&!RopeOverLength(id)&&g_entities[id].s.jarRopeState==4);
 }
 assert(b->r.currentOrigin[0]<before-4);
 imports.Trace=RigidTrace;assert(WorldClear());

 // The moving corner participates in the same constraint. With both ends
 // frozen, a shorter cable can pull a movable prop out of its own path.
 RigidFixture();imports.EntitiesInBox=FixtureEntitiesInBox;
 a=CopySpawn(1,0,0);b=CopySpawn(1,0,160);wall=CopySpawn(1,0,80);
 ropeTestA=a->s.number;ropeTestB=b->s.number;ropeTestSolid=wall->s.number;
 Move(a,(vec3_t){0,0,84});Move(b,(vec3_t){160,0,84});Move(wall,(vec3_t){80,3,84});
 PhysicsHull(wall,wall->r.currentAngles,wall->r.mins,wall->r.maxs);
 objects[ropeTestA].frozen=objects[ropeTestB].frozen=1;
 objects[ropeTestSolid].noGravity=1;
 assert(PhysicsPrepare());imports.Trace=RopeTestTrace;
 id=ForceLink(0,ropeTestA,ropeTestB,200)->s.number;
 RopePrepare();assert(ropeCache[id].valid&&ropeCache[id].result.contactCount==2);
 const float original=ropeCache[id].result.length;
 const float target=original-.4f;
 objects[id].length=target;
 vec3_t cornerBefore;VectorCopy(wall->r.currentOrigin,cornerBefore);
 level.time+=25;JointFrame(.025f,qtrue);ForceRopeVisuals();
 fprintf(stderr,"MP routed rope over a moving prop: full path %.4f / %.4f, corner displacement %.4f.\n",
  ropeCache[id].result.length,target,Distance(cornerBefore,wall->r.currentOrigin));
 assert(ropeCache[id].valid&&!RopeOverLength(id)&&g_entities[id].s.jarRopeState>2);
 assert(Distance(cornerBefore,wall->r.currentOrigin)>.05f&&a->r.currentOrigin[0]==0&&b->r.currentOrigin[0]==160);

 // An impossible fixed path retains its configured length and attachment.
 // Never replicate a visibly stretched tube while there is no feasible pose.
 objects[ropeTestSolid].frozen=1;objects[id].length=120;
 level.time+=25;JointFrame(.025f,qtrue);ForceRopeVisuals();
 assert(g_entities[id].inuse&&objects[id].length==120&&g_entities[id].s.jarRopeState==1);
 objects[id].length=original;level.time+=25;JointFrame(.025f,qtrue);ForceRopeVisuals();
 assert(g_entities[id].s.jarRopeState>2);
 imports.Trace=RigidTrace;assert(WorldClear());
}
static void RopePathCases(void){
 static jarWorld_t world,decoded;
 RigidFixture();gentity_t*a=CopySpawn(1,0,0),*b=CopySpawn(1,0,160);ropeTestA=a->s.number;ropeTestB=b->s.number;
 imports.EntitiesInBox=FixtureEntitiesInBox;
 objects[ropeTestA].frozen=objects[ropeTestB].frozen=0;objects[ropeTestA].noGravity=objects[ropeTestB].noGravity=1;
 assert(PhysicsPrepare());gentity_t*wall=G_Spawn();ropeTestSolid=wall->s.number;wall->r.contents=CONTENTS_SOLID;
 VectorSet(wall->r.currentOrigin,80,0,80);VectorSet(wall->r.mins,-10,-20,-60);VectorSet(wall->r.maxs,10,20,60);Link((sharedEntity_t*)wall);
 gentity_t*rope=ForceLink(0,ropeTestA,ropeTestB,160);int rn=rope->s.number;
 assert(JAR_RigidRope(rn+1,ropeTestA,PhysicsIdentity(ropeTestA),ropeTestB,PhysicsIdentity(ropeTestB),objects[rn].anchorA,objects[rn].anchorB,160));
 ropeTestCheckEndpoints=1;imports.Trace=RopeTestTrace;level.time+=50;RopePrepare();assert(ropeCache[rn].valid&&objects[rn].ropePath.count==2&&!RopeRigid(rn));
 const float routedLength=ropeCache[rn].result.length;objects[rn].length=routedLength;
 assert(a->r.linked&&b->r.linked);
 vec3_t outward;VectorScale(ropeCache[rn].result.pullA,-100,outward);PhysicsVelocity(ropeTestA,outward);
 ForceFrame(.05f);
 // Both velocity changes point toward the same side of the pillar.
 jarMPPose_t pa,pb;vec3_t low,high;assert(JAR_RigidRead(ropeTestA,&pa,low,high)&&JAR_RigidRead(ropeTestB,&pb,low,high));
 assert((pa.linear[1]-outward[1])*pb.linear[1]>0);
 ForceRopeVisuals();assert(rope->s.jarRopeState==4&&rope->s.jarRopeLength==routedLength&&!memcmp(rope->s.jarRopeBends,objects[rn].ropePath.bend,sizeof(rope->s.jarRopeBends)));
 assert(rope->s.otherEntityNum==ropeTestA&&rope->s.otherEntityNum2==ropeTestB);
 int sweeps=ropeTestSweeps;RopePrepare();assert(sweeps==ropeTestSweeps);
 // Budget exhaustion recovers by an entity hull sweep; no stale endpoint force.
 vec3_t accepted;VectorCopy(a->r.currentOrigin,accepted);a->r.currentOrigin[0]-=8;ropeBudgetTime=level.time;ropeBudget=0;
 RopePrepare();assert(!ropeCache[rn].valid&&VectorCompare(accepted,a->r.currentOrigin)&&!VectorLength(objects[ropeTestA].velocity));RopePrepare();assert(ropeCache[rn].valid);
 // A changed solid invalidates stationary anchors and never publishes stale bends.
 level.time+=50;wall->r.mins[1]=wall->r.mins[2]=-10000;wall->r.maxs[1]=wall->r.maxs[2]=10000;RopePrepare();RopePublish(rn);assert(!ropeCache[rn].valid&&rope->s.jarRopeState==1&&!WorldCapture(&world));
 VectorSet(wall->r.mins,-10,-20,-60);VectorSet(wall->r.maxs,10,20,60);level.time+=50;RopePrepare();assert(ropeCache[rn].valid);
 // Full world encoding preserves topology; old JRW17 reads yield zero bends.
 static byte bytes[WORLD_BYTES],legacy[WORLD_BYTES];assert(WorldCapture(&world)&&world.copy.ropes==1);
 int size=WorldEncode(&world,bytes);assert(WorldDecode(bytes,size,&decoded)&&decoded.copy.rope[0].path.count==2);
 assert(!memcmp(&world.copy.rope[0].path,&decoded.copy.rope[0].path,sizeof(jarRopePath_t)));
 const int linkAt=WORLD_HEADER+world.copy.count*WORLD_NODE;memcpy(legacy,bytes,linkAt+72);int at=0;WorldPut(legacy,&at,0x4857524au);WorldTestHash(legacy,linkAt+72+4);
 assert(WorldDecode(legacy,linkAt+72+4,&decoded)&&!decoded.copy.rope[0].path.count);
 for(int field=0;field<3;++field){memcpy(legacy,bytes,size);at=linkAt+72+(field==1?4:field==2?4+2*12:0);WorldPut(legacy,&at,field==0?9:field==1?0x7fc00000u:0x3f800000u);WorldTestHash(legacy,size);decoded.generation=99;assert(!WorldDecode(legacy,size,&decoded)&&decoded.generation==99);}
 // Copies choose a new route in clear space, then reject an obstructed paste
 // transaction. Retained source bends must never be offset into empty space.
 ropeTestCheckEndpoints=0;CopyAssembly(&g_entities[0],ropeTestA);assert(copies[0].ropes==1&&copies[0].rope[0].path.count==2);
 vec3_t destination={0,300,80};int before=LiveCount();assert(PlaceCopy(&g_entities[0],&copies[0],destination,NULL,NULL));int pasted=-1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(RopeLink(n)&&n!=rn)pasted=n;assert(pasted>=0&&!objects[pasted].ropePath.count);Undo(&g_entities[0],qfalse);assert(LiveCount()==before);
 VectorSet(wall->r.currentOrigin,80,300,80);VectorSet(wall->r.mins,-10,-10000,-10000);VectorSet(wall->r.maxs,10,10000,10000);
 assert(!PlaceCopy(&g_entities[0],&copies[0],destination,NULL,NULL)&&LiveCount()==before&&rope->inuse);
 // Removing scenery restores straight-constraint eligibility. Slot reuse and
 // deleting an endpoint erase route history and its attachment together.
 wall->r.contents=0;level.time+=50;RopePrepare();assert(RopeRigid(rn)&&!objects[rn].ropePath.count);
 // A moving NPC/player-shaped body can redirect the route without changing
 // either the obstacle or an endpoint while their hulls overlap.
 wall->r.contents=CONTENTS_BODY;VectorSet(wall->r.mins,-10,-20,-60);VectorSet(wall->r.maxs,10,20,60);
 VectorSet(wall->r.currentOrigin,80,300,80);sweeps=ropeTestSweeps;RopePrepare();assert(sweeps==ropeTestSweeps);
 wall->r.currentOrigin[1]=0;level.time+=50;RopePrepare();RopePublish(rn);
 assert(ropeCache[rn].valid&&rope->s.jarRopeState==4&&objects[rn].ropePath.count==2);
 VectorCopy(a->r.currentOrigin,wall->r.currentOrigin);VectorCopy(a->r.currentOrigin,accepted);objects[ropeTestA].velocity[0]=75;
 level.time+=50;RopePrepare();RopePublish(rn);
 assert(!ropeCache[rn].valid&&rope->s.jarRopeState==1&&VectorCompare(accepted,a->r.currentOrigin)&&objects[ropeTestA].velocity[0]==75);
 assert(WorldCapture(&world)&&world.copy.ropes==1&&!world.copy.rope[0].path.count&&objects[rn].ropePath.count==2);
 wall->r.currentOrigin[1]=300;level.time+=50;RopePrepare();RopePublish(rn);
 assert(RopeRigid(rn)&&rope->s.jarRopeState==2);
 G_FreeEntity(a);assert(!rope->inuse&&!ropeCache[rn].history);imports.Trace=RigidTrace;assert(WorldClear());
 puts("MP rope paths: local cache, moving bodies, safe overlap suspension, real Box3D impulses, snapshot publication, JRW18/17, copy rollback and deletion passed.");
 RigidFixture();imports.EntitiesInBox=FixtureEntitiesInBox;
 a=CopySpawn(1,0,0);b=CopySpawn(1,0,160);wall=CopySpawn(1,1,80);
 ropeTestA=a->s.number;ropeTestB=b->s.number;ropeTestSolid=wall->s.number;
 objects[ropeTestA].frozen=objects[ropeTestB].frozen=1;
 objects[ropeTestSolid].frozen=0;objects[ropeTestSolid].mass=120;objects[ropeTestSolid].noGravity=1;
 Move(wall,(vec3_t){80,6,90});VectorSet(wall->r.mins,-10,-20,-60);VectorSet(wall->r.maxs,10,20,60);
 rope=ForceLink(0,ropeTestA,ropeTestB,250);rn=rope->s.number;
 assert(PhysicsPrepare());imports.Trace=RopeTestTrace;ropeTestCheckEndpoints=1;RopeForget(rn);
 level.time+=50;ForceFrame(.05f);
 const jarRopePathResult_t* route=&ropeCache[rn].result;
 assert(ropeCache[rn].valid&&route->contactCount==2&&route->contacts[0].prop==ropeTestSolid+1);
 assert(!VectorLength(objects[ropeTestSolid].velocity)); // Slack.
 objects[rn].length=160;sharedEditing.integer=0;ForceFrame(.05f);
 assert(!VectorLength(objects[ropeTestSolid].velocity)); // Owner only.
 sharedEditing.integer=1;objects[ropeTestSolid].heldBy=2;ForceFrame(.05f);
 assert(!VectorLength(objects[ropeTestSolid].velocity));
 objects[ropeTestSolid].heldBy=0;objects[ropeTestSolid].frozen=1;ForceFrame(.05f);
 assert(!VectorLength(objects[ropeTestSolid].velocity));
 objects[ropeTestSolid].frozen=0;
 jarRopeContact_t stale=route->contacts[0];++stale.generation;
 assert(!RopeContactMass(objects[rn].owner,&stale));
 PhysicsVelocity(ropeTestSolid,(vec3_t){0,-80,0});ForceFrame(.05f);
 assert(objects[ropeTestSolid].velocity[1]>-80&&fabsf(objects[ropeTestSolid].angular[0])>.001f);
 assert(a->r.currentOrigin[0]==0&&b->r.currentOrigin[0]==160);
 ropeTestCheckEndpoints=0;imports.Trace=RigidTrace;assert(WorldClear());
 puts("MP rope loads: Box3D force and torque, fixed endpoints, slack, shared-edit policy, freeze, hold and stale identity passed.");
}
