// Actual Toolgun, world codec and Box3D joint/impulse paths.
static vec3_t attachPoint,attachNormal;
static void AttachTrace(trace_t*t,const vec3_t s,const vec3_t lo,const vec3_t hi,const vec3_t e,int pass,int mask,int cap,int flags,int lod){
 RigidTrace(t,s,lo,hi,e,pass,mask,cap,flags,lod);if(!lo){VectorCopy(attachPoint,t->endpos);VectorCopy(attachNormal,t->plane.normal);}
}
static void AttachAim(gentity_t*e,const vec3_t local,const vec3_t normal){Endpoint(e->s.number,local,attachPoint);VectorCopy(normal,attachNormal);aimed=e->s.number;imports.Trace=AttachTrace;}
static void AttachmentCases(void){
 RigidFixture();Command(0,"jar_setting","show");Command(1,"jar_setting","show");assert(players[0].ropeSlack==1.1f&&players[1].thrustForce==5000);
 Command(0,"jar_setting","force_more");assert(players[0].thrustForce==6000&&players[1].thrustForce==5000);
 assert(JAR_MP_Command(&g_entities[0],"jar_setting")&&players[0].thrustForce==6000); // same-tick rate limit
 Command(0,"jar_setting","force_99900");Command(0,"jar_setting","force_more");assert(players[0].thrustForce==100900);
 Command(0,"jar_setting","force_200");Command(0,"jar_setting","force_less");assert(players[0].thrustForce==-800);
 Command(0,"jar_setting","force_0");Command(0,"jar_setting","show");assert(players[0].thrustForce==0);
 Command(0,"jar_setting","slack_2.95");Command(0,"jar_setting","slack_more");assert(fabsf(players[0].ropeSlack-3.05f)<.0001f);
 Command(0,"jar_setting","slack_1.01");Command(0,"jar_setting","slack_less");assert(fabsf(players[0].ropeSlack-.91f)<.0001f);
 Command(0,"jar_setting","slack_more;quit");assert(fabsf(players[0].ropeSlack-.91f)<.0001f);
 Command(0,"jar_setting","rotation_-740.5");assert(players[0].rotationStep==-740.5f);
 Command(0,"jar_setting","rotation_1e999");assert(players[0].rotationStep==-740.5f);
 Command(0,"jar_setting","force_reset");assert(players[0].thrustForce==5000);
 // Mount at a surface point: power and local frame survive body rotation.
 gentity_t*a=CopySpawn(1,0,100);int n=a->s.number;objects[n].noGravity=1;
 const vec3_t local={20,4,4},normal={0,0,1};AttachAim(a,local,normal);players[0].tool=2;Tool(&g_entities[0],qfalse);
 assert(objects[n].thrustMounted&&objects[n].thruster&&objects[n].thrustForce==5000&&Distance(local,objects[n].thrustAnchor)<.001f);
 assert(PhysicsPrepare());ForceFrame(.025f);jarMPPose_t pose;vec3_t low,high;assert(JAR_RigidRead(n,&pose,low,high)&&pose.linear[2]<0&&VectorLength(pose.angular)>1&&VectorLength(pose.angular)<=8.001f);
 vec3_t old;VectorCopy(pose.linear,old);objects[n].heldBy=1;ForceFrame(.025f);assert(JAR_RigidRead(n,&pose,low,high)&&Distance(old,pose.linear)<.001f);objects[n].heldBy=0;
 objects[n].thrustForce=100000;objects[n].mass=5;assert(PhysicsPrepare());ForceFrame(.1f);assert(JAR_RigidRead(n,&pose,low,high)&&VectorLength(pose.linear)<=1500.01f&&VectorLength(pose.angular)<=8.001f);
 objects[n].thrustForce=4500;objects[n].thruster=0;VectorClear(objects[n].velocity);VectorClear(objects[n].angular);
 const vec3_t angle={0,90,0};assert(PhysicsSetAngles(a,angle));vec3_t point,axis;ThrusterFrame(n,point,axis);assert(fabsf(point[0]-96)<.001f&&fabsf(point[1]-20)<.001f&&axis[2]==1);
 // Copy is a value snapshot; deletion/slot reuse cannot change mounted settings.
 CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].thrustMounted&&copies[0].node[0].thrustForce==4500);
 G_FreeEntity(a);assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){100,0,100},NULL,NULL));a=&g_entities[MAX_CLIENTS];n=a->s.number;
 assert(objects[n].thrustMounted&&objects[n].thrustForce==4500&&Distance(local,objects[n].thrustAnchor)<.001f);
 // JRW3 round-trip, old JRW1/JRW2 default mounts, malformed fields are transactional.
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&saved));int length=WorldEncode(&saved,bytes);assert(WorldDecode(bytes,length,&decoded)&&decoded.copy.node[0].thrustForce==4500&&decoded.copy.node[0].thrustMounted);
 const int offsets[]={100,104,108,120};const unsigned invalid[]={2,0x7fc00000u,0x47000000u,0x40000000u};
 for(int k=0;k<4;++k){memcpy(bad,bytes,length);int at=WORLD_HEADER+offsets[k];WorldPut(bad,&at,invalid[k]);WorldTestHash(bad,length);decoded.generation=42;assert(!WorldDecode(bad,length,&decoded)&&decoded.generation==42);}
 for(int version=1;version<=2;++version){int stride=version==1?88:100;memcpy(bad,bytes,WORLD_HEADER+stride);int at=0;WorldPut(bad,&at,version==1?0x3157524au:0x3257524au);int size=WORLD_HEADER+stride+4;WorldTestHash(bad,size);assert(WorldDecode(bad,size,&decoded)&&!decoded.copy.node[0].thrustMounted&&decoded.copy.node[0].thrustForce==3000&&decoded.copy.node[0].thrustAxis[2]==1);}
 AttachAim(a,local,normal);players[0].tool=2;Tool(&g_entities[0],qtrue);assert(!objects[n].thrustMounted&&!objects[n].thruster);
 Command(0,"jar_setting","force_more");Command(0,"jar_setting","thrust_out");assert(players[0].thrustReverse&&!players[1].thrustReverse);
 Tool(&g_entities[0],qfalse);assert(objects[n].thrustForce==6000&&objects[n].thrustMounted&&objects[n].thrustAxis[2]==-1);
 ForceRopeVisuals();assert(g_entities[thrusterVisual[n]-1].s.origin2[2]==-1);
 assert(PhysicsPrepare());ForceFrame(.025f);assert(JAR_RigidRead(n,&pose,low,high)&&pose.linear[2]>0);
 assert(WorldCapture(&saved));length=WorldEncode(&saved,bytes);
 assert(WorldDecode(bytes,length,&decoded)&&decoded.copy.node[0].thrustAxis[2]==-1);
 CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].thrustAxis[2]==-1);
 Command(0,"jar_setting","thrust_in");assert(!players[0].thrustReverse&&objects[n].thrustAxis[2]==-1);
 // A real fixed rope catches a falling prop at its local surface anchor.
 RigidFixture();a=CopySpawn(1,0,100);n=a->s.number;gentity_t*r=ForceLink(0,n,ENTITYNUM_WORLD,50);int rope=r->s.number;
 VectorSet(objects[rope].anchorA,20,0,4);VectorSet(objects[rope].anchorB,120,0,150);RigidSteps(200);
 Endpoint(n,objects[rope].anchorA,point);assert(PhysicsRope(rope)&&Distance(point,objects[rope].anchorB)<50.6f&&a->r.currentOrigin[2]>75&&fabsf(a->r.currentAngles[PITCH])>10);
 // Held drag must respect the same joint, instead of stretching the visible rope.
 ActorArm(0,n);clients[0].ps.origin[0]=600;RigidSteps(100);Endpoint(n,objects[rope].anchorA,point);assert(Distance(point,objects[rope].anchorB)<52);
 clients[0].pers.cmd.buttons=0;RigidSteps(1);objects[n].frozen=1;RigidSteps(1);VectorCopy(a->r.currentOrigin,old);RigidSteps(20);assert(Distance(old,a->r.currentOrigin)<.001f);objects[n].frozen=0;RigidSteps(20);
 G_FreeEntity(r);assert(!PhysicsRope(rope));RigidSteps(160);assert(a->r.currentOrigin[2]<25);
 // Duplicated members share an undo group but must have distinct physics IDs.
 RigidFixture();a=CopySpawn(1,0,100);gentity_t*b=CopySpawn(1,0,200);n=a->s.number;int m=b->s.number;objects[m].order=objects[n].order;objects[n].noGravity=objects[m].noGravity=1;
 r=ForceLink(0,n,m,80);rope=r->s.number;VectorSet(objects[rope].anchorA,4,0,4);VectorSet(objects[rope].anchorB,4,0,4);RigidSteps(40);assert(PhysicsRope(rope)&&PhysicsIdentity(n)!=PhysicsIdentity(m));
 Endpoint(n,objects[rope].anchorA,point);Endpoint(m,objects[rope].anchorB,axis);assert(Distance(point,axis)<81);
 G_FreeEntity(a);assert(!r->inuse);a=CopySpawn(1,0,100);assert(a->s.number==n);RigidSteps(1);assert(!PhysicsRope(rope));
 // Legacy unmounted force remains pure linear acceleration on an offset model.
 RigidFixture();a=CopySpawn(1,0,100);n=a->s.number;objects[n].noGravity=1;objects[n].mass=200;objects[n].thruster=1;assert(!objects[n].thrustMounted&&PhysicsPrepare());ForceFrame(.1f);
 assert(JAR_RigidRead(n,&pose,low,high)&&fabsf(pose.linear[2]+48)<.001f&&VectorLength(pose.angular)==0);
 // Mixed native/rigid ropes use point velocity and impart torque, preserving AI.
 RigidFixture();a=CopySpawn(1,0,100);b=CopySpawn(2,0,250);CopyReady(b);n=a->s.number;m=b->s.number;objects[n].noGravity=1;r=ForceLink(0,n,m,40);VectorSet(objects[r->s.number].anchorA,4,0,12);assert(PhysicsPrepare());assert(!PhysicsRope(r->s.number));
 b->client->ps.velocity[0]=100;ForceFrame(.025f);
 assert(JAR_RigidRead(n,&pose,low,high)&&fabsf(pose.angular[1])>.01f&&b->client->ps.velocity[0]<100&&b->health==100&&!actors[m].active);
 PhysicsReset();imports.Trace=Trace;puts("MP mounted attachments: settings, torque/limits, hold, JRW1/2/3, copy/reuse, rigid anchors, mixed ropes and cleanup passed.");
}
