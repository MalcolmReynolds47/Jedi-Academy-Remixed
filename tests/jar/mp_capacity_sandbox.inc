// Admission, persistence and atomic overflow at the actual fixed Sandbox limits.
static void CapacitySandboxCases(void){
 ActorFixture();imports.Cvar_VariableStringBuffer=WorldTestCvar;Q_strncpyz(level.mapname,"capacity",sizeof(level.mapname));
 for(int i=0;i<JAR_MP_PROP_LIMIT;++i){gentity_t*e=CopySpawn(1,0,100);assert(e);objects[e->s.number].frozen=1;}
 int live=LiveCount();assert(live==JAR_MP_PROP_LIMIT&&!CopySpawn(1,0,100)&&LiveCount()==live);
 static jarWorld_t world,decoded;static byte data[WORLD_BYTES];assert(WorldCapture(&world)&&world.copy.count==4096);int bytes=WorldEncode(&world,data);assert(bytes>65535&&WorldDecode(data,bytes,&decoded)&&decoded.copy.count==4096);
 vec3_t zero={0,0,0};assert(!PlaceCopy(&g_entities[0],&world.copy,zero,NULL,NULL)&&LiveCount()==live);
 assert(WorldClear()&&LiveCount()==0);assert(PlaceCopy(NULL,&decoded.copy,zero,decoded.velocity,decoded.angular)&&LiveCount()==4096);assert(WorldClear());
 ActorFixture();Q_strncpyz(level.mapname,"capacity",sizeof(level.mapname));
 for(int i=0;i<513;++i){gentity_t*e=CopySpawn(2,0,100);assert(e);CopyReady(e);objects[e->s.number].frozen=1;}
 assert(CopyCount(0)==513);
 assert(WorldCapture(&world)&&world.copy.count==513);bytes=WorldEncode(&world,data);assert(WorldDecode(data,bytes,&decoded)&&decoded.copy.count==513);
 ActorFixture();

 // The real MP Box3D adapter must admit map-anchored joints after every
 // prop slot is occupied. Give the static test props separate hulls.
 const vec3_t quad[]={ {-512,-512,0},{512,-512,0},{512,512,0},{-512,512,0} };
 const vec3_t up={0,0,1},half={8,8,8};
 JAR_RigidReset();assert(JAR_RigidPolygon(4,&quad[0][0],up)&&JAR_RigidStart());
 for(unsigned i=0;i<JAR_MP_PROP_LIMIT;++i){
  jarMPPose_t pose={0};VectorSet(pose.position,(i%64)*32,(i/64)*32,80);
  assert(JAR_RigidBody(MAX_CLIENTS+i,i+1,half,zero,&pose,qtrue,80,1));
 }
 const vec3_t anchor={0,0,160};
 JAR_RigidBeginRopes();
 assert(JAR_RigidRope(1,MAX_CLIENTS,1,-1,0,zero,anchor,80));
 for(int kind=1;kind<=2;++kind){
  jarMPJoint_t joint={0};joint.id=kind+1;joint.kind=kind;
  joint.first=kind+1;joint.ra[3]=joint.rb[3]=1;
  VectorSet(joint.b,kind*32,0,80);
  assert(JAR_RigidJoint(&joint,MAX_CLIENTS+kind,-1));
 }
 JAR_RigidEndRopes();
 JAR_RigidReset();
}
