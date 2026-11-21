// Production SP routing, forces, mesh, restore and rigid switching. Native
// brush arithmetic is tested separately; here traces also enforce entity links.
{
 auto previousTrace=gi.trace;
 static int routeSweeps=0;static bool checkEndpointLinks=false,posedCollision=false;
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){
  CHECK(mins&&maxs);*t={};t->fraction=1;t->entityNum=ENTITYNUM_NONE;VectorCopy(end,t->endpos);
  if(std::fabs(mins[0])<3){++routeSweeps;if(checkEndpointLinks)CHECK(!g_entities[1].linked&&!g_entities[2].linked);}
  for(int n:{1,2,20,21,22,23,24,25,26,27}){const auto&e=g_entities[n];if(n>=globals.num_entities||n==pass||!e.inuse||!e.linked||!(e.contents&mask))continue;
   // Two collision poses with the same broad-phase bounds, as with a
   // symmetric rotated model or a frame that moves geometry inside its bounds.
   if(n==20&&posedCollision&&!e.s.frame&&e.currentAngles[YAW]==0)continue;
   vec3_t low,high;for(int a=0;a<3;++a){low[a]=e.currentOrigin[a]+e.mins[a]-maxs[a];high[a]=e.currentOrigin[a]+e.maxs[a]-mins[a];}
   float enter=0,leave=1;int axis=-1;float sign=0;bool inside=true,miss=false;
   for(int a=0;a<3;++a){const float d=end[a]-start[a];if(start[a]<low[a]||start[a]>high[a])inside=false;
    if(std::fabs(d)<.00001f){if(start[a]<low[a]||start[a]>high[a]){miss=true;break;}continue;}
    float near=(low[a]-start[a])/d,far=(high[a]-start[a])/d;const float normal=d>0?-1.f:1.f;
    if(near>far)std::swap(near,far);if(near>enter){enter=near;axis=a;sign=normal;}leave=std::min(leave,far);
   }
   if(miss||enter>leave||leave<0||enter>1)continue;
   if(inside){t->startsolid=qtrue;t->allsolid=qtrue;t->fraction=0;t->entityNum=n;VectorCopy(start,t->endpos);return;}
   if(enter<t->fraction&&axis>=0){t->fraction=std::max(0.f,enter);t->entityNum=n;VectorClear(t->plane.normal);t->plane.normal[axis]=sign;for(int a=0;a<3;++a)t->endpos[a]=start[a]+(end[a]-start[a])*t->fraction;}
  }
 };
 Reset();level.time=10000;
 auto&first=Entity(1,true,0,80);auto&second=Entity(2,true,100,160);auto&pillar=g_entities[20];
 first.currentOrigin[2]=second.currentOrigin[2]=40;first.contents=second.contents=CONTENTS_SOLID;first.linked=second.linked=qtrue;
 pillar.inuse=pillar.linked=pillar.bmodel=qtrue;pillar.s.number=20;pillar.contents=CONTENTS_SOLID;
 VectorSet(pillar.currentOrigin,50,0,40);VectorSet(pillar.mins,-10,-12,-40);VectorSet(pillar.maxs,10,12,40);
 checkEndpointLinks=true;Rope(first,second,100);CHECK(state.joints[0].path.count==2&&first.linked&&second.linked);
 first.client->ps.velocity[0]=-60;second.client->ps.velocity[0]=60;
 attachmentTick(.05f);const auto id=state.joints[0].id;
 CHECK(ropeStates[id].valid&&state.joints[0].path.count==2);
 CHECK(first.client->ps.velocity[1]*second.client->ps.velocity[1]>0); // Scenery takes the lateral reaction.
 CHECK(first.currentOrigin[0]>0&&second.currentOrigin[0]<100&&first.linked&&second.linked);
 const jarRopeVertex_t* mesh=nullptr;
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 for(int i=0;i<JAR_ROPE_VERTICES;++i)for(float x:mesh[i].xyz)CHECK(std::isfinite(x));
 // Unchanged scenery/anchors use the validated path without spending sweeps.
 const int beforeCache=routeSweeps;attachmentPrepareRopes();CHECK(routeSweeps==beforeCache);
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES&&routeSweeps==beforeCache);
 const auto saved=Encode(state);State restored;CHECK(Decode(saved.data(),saved.size(),restored,ENTITYNUM_WORLD));
 const float side=state.joints[0].path.bend[0][1];state=restored;ropeReset();attachmentPrepareRopes();
 CHECK(state.joints[0].path.bend[0][1]*side>0&&ropeStates[id].valid&&ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 // Exhausted global work quota rejects an attempted move using a swept restore,
 // then the old validated path can be reused at the recovered endpoint poses.
 const auto good=ropeStates[id];first.currentOrigin[0]-=8;VectorCopy(first.currentOrigin,first.client->ps.origin);
 ropeBudgetTime=level.time;ropeTraceBudget=0;first.client->ps.velocity[0]=-100;attachmentPrepareRopes();
 CHECK(std::fabs(first.currentOrigin[0]-good.poseA.position.x)<.001f&&VectorLength(first.client->ps.velocity)==0);
 attachmentPrepareRopes();CHECK(ropeStates[id].valid);
 // A moving/changing solid invalidates the cache even with stationary anchors.
 level.time+=50;pillar.mins[1]=pillar.mins[2]=-10000;pillar.maxs[1]=pillar.maxs[2]=10000;
 first.currentOrigin[0]-=4;VectorCopy(first.currentOrigin,first.client->ps.origin);attachmentPrepareRopes();
 CHECK(!ropeStates[id].valid&&state.joints[0].path.count==2&&VectorLength(first.client->ps.velocity)==0);
 CHECK(ropeMesh(state.joints[0],mesh)==0&&status.find("obstructed")!=std::string::npos);
 level.time+=50;pillar.contents=0;attachmentPrepareRopes();CHECK(ropeStates[id].valid&&!state.joints[0].path.count);
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 // Far-away body motion keeps the route cached. An entering collision body
 // bends it, and an actor crossing the anchor cannot rewind either endpoint.
 auto& passer=Entity(21,true,50,80);passer.contents=CONTENTS_BODY;passer.linked=qtrue;
 VectorSet(passer.currentOrigin,50,240,40);const int beforePass=routeSweeps;
 attachmentPrepareRopes();CHECK(routeSweeps==beforePass);
 passer.currentOrigin[1]=0;level.time+=50;attachmentPrepareRopes();
 CHECK(ropeStates[id].valid&&state.joints[0].path.count==2&&ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 VectorCopy(first.currentOrigin,passer.currentOrigin);first.client->ps.velocity[0]=70;
 const float beforeOverlap=first.currentOrigin[0];level.time+=50;attachmentPrepareRopes();
 CHECK(!ropeStates[id].valid&&first.client->ps.velocity[0]==70&&first.currentOrigin[0]==beforeOverlap);
 passer.currentOrigin[1]=240;level.time+=50;attachmentPrepareRopes();
 CHECK(ropeStates[id].valid&&!state.joints[0].path.count);
 passer.linked=passer.inuse=qfalse;passer.client=nullptr;
 // Angle and frame changes alone can move the collision hull into the rope.
 pillar.classname=const_cast<char*>("jar_sandbox_prop");pillar.bmodel=qfalse;pillar.contents=CONTENTS_BODY;
 VectorSet(pillar.mins,-10,-12,-40);VectorSet(pillar.maxs,10,12,40);
 posedCollision=true;attachmentPrepareRopes();CHECK(ropeStates[id].valid&&!state.joints[0].path.count);
 pillar.currentAngles[YAW]=90;attachmentPrepareRopes();CHECK(ropeStates[id].valid&&state.joints[0].path.count==2);
 pillar.currentAngles[YAW]=0;attachmentPrepareRopes();CHECK(ropeStates[id].valid&&!state.joints[0].path.count);
 pillar.s.frame=1;attachmentPrepareRopes();CHECK(ropeStates[id].valid&&state.joints[0].path.count==2);
 pillar.s.frame=0;posedCollision=false;
 // A floor below the physical path can block visual sag without changing its bends.
 pillar.bmodel=qtrue;pillar.contents=CONTENTS_SOLID;
 VectorSet(pillar.currentOrigin,50,0,0);VectorSet(pillar.mins,-90,-90,-15);VectorSet(pillar.maxs,90,90,0);
 state.joints[0].length=240;attachmentPrepareRopes();CHECK(!state.joints[0].path.count);
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 float clippedZ=10000;for(int i=0;i<JAR_ROPE_VERTICES;++i)clippedZ=std::min(clippedZ,mesh[i].xyz[2]);
 CHECK(clippedZ>0);
 const int beforeFloor=routeSweeps;
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES&&routeSweeps==beforeFloor);
 pillar.currentOrigin[2]=-200;attachmentPrepareRopes();
 CHECK(routeSweeps==beforeFloor&&ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES&&routeSweeps>beforeFloor);
 float openZ=10000;for(int i=0;i<JAR_ROPE_VERTICES;++i)openZ=std::min(openZ,mesh[i].xyz[2]);
 CHECK(openZ<clippedZ-20);
 CHECK(removeJoint(id)&&ropeStates.empty());
 // Two real Box3D props switch away from a formerly restored straight joint.
 Reset();level.time=11000;auto&rigidA=Entity(1,false,0,80);auto&rigidB=Entity(2,false,100,80);
 rigidA.currentOrigin[2]=rigidB.currentOrigin[2]=40;rigidA.linked=rigidB.linked=qtrue;
 rigidA.contents=rigidB.contents=CONTENTS_SOLID;pillar.contents=CONTENTS_SOLID;
 VectorSet(pillar.currentOrigin,50,0,40);
 VectorSet(pillar.mins,-10,-12,-40);VectorSet(pillar.maxs,10,12,40);
 CHECK(Own(state,1,1001)&&Own(state,2,1002));owned(1)->frozen=owned(2)->frozen=0;
 CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000}}));
 for(auto*e:{&rigidA,&rigidB}){jar::physics::Pose p;p.position=pv(e->currentOrigin);p.linear.x=e==&rigidA?-60:60;CHECK(physicsWorld.Prop(e->s.number,owned(e->s.number)->id,{8,8,8},{},p,false));}
 Rope(rigidA,rigidB,100);auto straight=state.joints[0];straight.path={};
 CHECK(physicsWorld.Joint(straight,1,2)&&physicsWorld.Statistics().joints==1);
 attachmentPrepareRopes();CHECK(!attachmentBoxJoint(state.joints[0])&&physicsWorld.Statistics().joints==0);
 CHECK(!physicsWorld.Joint(state.joints[0],1,2));attachmentTick(.05f);
 jar::physics::Pose motionA,motionB;CHECK(physicsWorld.Read(1,motionA)&&physicsWorld.Read(2,motionB));
 CHECK(motionA.linear.y*motionB.linear.y>0&&motionA.position.x>0&&motionB.position.x<100);
 CHECK(std::fabs(motionA.position.x-rigidA.currentOrigin[0])<.001f&&std::fabs(motionB.position.x-rigidB.currentOrigin[0])<.001f);
 // Removing the obstacle permits straight rigid constraints again.
 pillar.contents=0;level.time+=50;attachmentPrepareRopes();CHECK(!state.joints[0].path.count&&attachmentBoxJoint(state.joints[0]));
 CHECK(physicsWorld.Joint(state.joints[0],1,2));
 Forget(state,1);physicsWorld.Forget(1);attachmentPrepareRopes();CHECK(ropeStates.empty()&&!state.jointCount);
 // Two fixed anchors squeeze a movable prop. The off-center load spins the
 // real Box3D body, while slack and frozen/held props keep their old behavior.
 Reset();level.time=12000;
 auto& endA=Entity(1,false,0,80);auto& endB=Entity(2,false,100,80);
 endA.currentOrigin[2]=endB.currentOrigin[2]=40;endA.linked=endB.linked=qtrue;
 endA.contents=endB.contents=CONTENTS_BODY;pillar.contents=CONTENTS_BODY;pillar.bmodel=qfalse;
 VectorSet(pillar.currentOrigin,50,6,50);
 CHECK(Own(state,1,1001)&&Own(state,2,1002)&&Own(state,20,1001));
 owned(20)->frozen=0;
 CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000}}));
 for(auto* e:{&endA,&endB,&pillar}){
  jar::physics::Pose p;p.position=pv(e->currentOrigin);
  CHECK(physicsWorld.Prop(e->s.number,owned(e->s.number)->id,attachmentHalf(*e),{},p,e!=&pillar));
 }
 Rope(endA,endB,180);attachmentTick(.05f);
 const auto& load=ropeStates[state.joints[0].id].result;
 CHECK(load.contactCount==2&&load.contacts[0].prop==21&&load.contacts[1].prop==21);
 jar::physics::Pose target;CHECK(physicsWorld.Read(20,target)&&target.linear.y==0);
 state.joints[0].length=100;held=20;attachmentTick(.05f);
 CHECK(physicsWorld.Read(20,target)&&target.linear.y==0);
 held=noEntity;owned(20)->frozen=1;attachmentTick(.05f);
 CHECK(physicsWorld.Read(20,target)&&target.linear.y==0);
 owned(20)->frozen=0;attachmentTick(.05f);
 CHECK(physicsWorld.Read(20,target)&&target.linear.y>0&&std::fabs(target.angular.x)>.001f);
 CHECK(endA.currentOrigin[0]==0&&endB.currentOrigin[0]==100);
 // Use all eight saved bend slots through the actual Workshop paths.
 Reset();level.time=13000;globals.num_entities=28;
 auto& nearAnchor=Entity(1,true,0,80);auto& farAnchor=Entity(2,true,172,80);
 for(auto* anchor:{&nearAnchor,&farAnchor}){
  anchor->currentOrigin[2]=40;VectorCopy(anchor->currentOrigin,anchor->client->ps.origin);
  anchor->contents=CONTENTS_BODY;anchor->linked=qtrue;
 }
 const vec3_t centers[]={{86,0,-20},{86,0,100},{86,-40,40},{86,40,40},{32,-4,40},{68,4,40},{104,-4,40},{140,4,40}};
 const vec3_t halves[]={{300,300,20},{300,300,20},{300,20,40},{300,20,40},{3,16,40},{3,16,40},{3,16,40},{3,16,40}};
 for(int i=0;i<8;++i){
  auto& wall=g_entities[20+i];wall.s.number=20+i;wall.client=nullptr;
  wall.inuse=wall.linked=wall.bmodel=qtrue;wall.contents=CONTENTS_SOLID;
  VectorCopy(centers[i],wall.currentOrigin);VectorCopy(halves[i],wall.maxs);VectorNegate(wall.maxs,wall.mins);
 }
 Rope(nearAnchor,farAnchor,300);attachmentPrepareRopes();
 const auto narrow=state.joints[0];CHECK(narrow.path.count==8&&ropeStates[narrow.id].valid);
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 const auto narrowSave=Encode(state);
 State narrowRestored;CHECK(Decode(narrowSave.data(),narrowSave.size(),narrowRestored,ENTITYNUM_WORLD));
 CHECK(!memcmp(&narrow.path,&narrowRestored.joints[0].path,sizeof(narrow.path)));
 state=narrowRestored;ropeReset();attachmentPrepareRopes();
 CHECK(ropeStates[narrow.id].valid&&state.joints[0].path.count==8);
 CHECK(ropeMesh(state.joints[0],mesh)==JAR_ROPE_VERTICES);
 CHECK(removeJoint(narrow.id)&&ropeStates.empty());
 for(int i=20;i<28;++i)g_entities[i].inuse=g_entities[i].linked=qfalse;
 checkEndpointLinks=false;gi.trace=previousTrace;Reset();
 std::puts("PASS: SP authoritative rope paths, bounded recovery, saved topology, mesh, Box3D switching and force/torque on a movable bend prop");
}
