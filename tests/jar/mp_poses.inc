// Actual pose commands, MD3 parser/cache, Box3D shapes and world/clipboard state.
static byte poseModel[sizeof(md3Header_t)+3*sizeof(md3Frame_t)];static int poseReadAt,poseReads,poseLength;
static int PoseOpen(const char*p,fileHandle_t*f,fsMode_t mode){*f=1;poseReadAt=0;return poseLength;}
static int PoseRead(void*out,int bytes,fileHandle_t f){++poseReads;if(bytes<0||poseReadAt+bytes>poseLength)return 0;memcpy(out,poseModel+poseReadAt,bytes);poseReadAt+=bytes;return bytes;}
static void PoseFixture(void){
 RigidFixture();memset(poseModel,0,sizeof(poseModel));poseLength=sizeof(poseModel);poseReads=0;md3Header_t*h=(md3Header_t*)poseModel;md3Frame_t*f=(md3Frame_t*)(h+1);
 h->ident=MD3_IDENT;h->version=MD3_VERSION;h->numFrames=3;h->ofsFrames=sizeof(*h);h->ofsEnd=sizeof(poseModel);
 for(int i=0;i<3;++i){VectorSet(f[i].bounds[0],-12,-8,-4);VectorSet(f[i].bounds[1],20+i*10,8,12+i*4);}
 imports.FS_Open=PoseOpen;imports.FS_Read=PoseRead;
}
static int poseFeedback[3][3];
static void PoseSend(int client,const char*text){if(client>=0&&client<3)sscanf(text,"jarselection %d %d %d",&poseFeedback[client][0],&poseFeedback[client][1],&poseFeedback[client][2]);}
static void PoseCases(void){
 imports.SendServerCommand=PoseSend;
 PoseFixture();gentity_t*a=CopySpawn(1,0,100),*b=CopySpawn(1,0,300);int n=a->s.number,m=b->s.number;objects[n].noGravity=objects[m].noGravity=1;
 aimed=n;Command(0,"jar_select","");assert(PhysicsPoseCount(n)==3);Command(0,"jar_property","pose_next");assert(a->s.frame==1&&a->r.maxs[0]==30&&a->s.angles2[2]==16&&b->s.frame==0);
 int reads=poseReads;RigidSteps(6);assert(poseReads==reads&&b->r.maxs[0]==20); // same model, two cached frames
 vec3_t low,high;jarMPPose_t pose;assert(JAR_RigidRead(n,&pose,low,high)&&fabsf(high[0]-30)<.001f);
 blocked=1;Command(0,"jar_property","pose_next");assert(a->s.frame==1&&a->r.maxs[0]==30);blocked=0;
 Command(0,"jar_property","pose_undo");assert(a->s.frame==0&&a->r.maxs[0]==20);Command(0,"jar_property","pose_undo");assert(a->s.frame==0);
 // Unauthorized edits, other editors and entity reuse cannot hijack pose Undo.
 players[1].selected=n;Command(1,"jar_property","pose_next");assert(a->s.frame==0);
 Command(0,"jar_property","pose_next");sharedEditing.integer=1;players[1].selected=n;Command(1,"jar_property","pose_next");assert(a->s.frame==2&&poseFeedback[0][1]==2&&poseFeedback[1][1]==2);Command(0,"jar_property","pose_undo");assert(a->s.frame==2);Command(1,"jar_property","pose_undo");assert(a->s.frame==1);sharedEditing.integer=0;
 Command(0,"jar_property","pose_next");Command(0,"jar_property","pose_next");assert(a->s.frame==2);assert(!PhysicsSetPose(a,-1)&&!PhysicsSetPose(a,65536));
 // Bound failure only rejects that pose; cached valid frames remain usable.
 md3Frame_t*f=(md3Frame_t*)((md3Header_t*)poseModel+1);f[2].bounds[1][0]=NAN;PhysicsReset();assert(!PhysicsSetPose(a,2));assert(PhysicsSetPose(a,1));f[2].bounds[1][0]=40;PhysicsReset();
 assert(PhysicsSetPose(a,2));objects[n].velocity[0]=15;objects[n].angular[2]=.5f;RigidSteps(1);
 gentity_t*r=ForceLink(0,n,ENTITYNUM_WORLD,200);VectorSet(objects[r->s.number].anchorB,100,0,250);RigidSteps(1);assert(PhysicsRope(r->s.number));
 // Frame edits release a held prop and recreate its rope/body with motion intact.
 ActorArm(0,n);assert(players[0].held==n);Command(0,"jar_property","pose_prev");assert(players[0].held==ENTITYNUM_NONE&&a->s.frame==1);clients[0].pers.cmd.buttons=0;RigidSteps(1);assert(PhysicsRope(r->s.number));
 // Copy/paste carries the selected frame even after deleting the source.
 G_FreeEntity(r);objects[n].frozen=1;CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].poseFrame==1);
 G_FreeEntity(a);assert(poseFeedback[0][0]==-1);assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){100,0,100},NULL,NULL));a=&g_entities[n];assert(a->s.frame==1&&a->r.maxs[0]>29);players[0].selected=n;Command(0,"jar_property","pose_undo");assert(a->s.frame==1);
 // World JRW4 preserves frame; JRW3 retains mount fields and defaults frame zero.
 static jarWorld_t captured,decoded;static byte bytes[WORLD_BYTES],old[WORLD_BYTES];assert(WorldCapture(&captured));int size=WorldEncode(&captured,bytes);assert(WorldDecode(bytes,size,&decoded)&&decoded.copy.node[0].poseFrame==1);
 int at=WORLD_HEADER+132;WorldPut(bytes,&at,65536);WorldTestHash(bytes,size);decoded.generation=42;assert(!WorldDecode(bytes,size,&decoded)&&decoded.generation==42);
 size=WorldEncode(&captured,bytes);memcpy(old,bytes,WORLD_HEADER);at=0;WorldPut(old,&at,0x3357524au);at=WORLD_HEADER;
 for(int i=0;i<captured.copy.count;++i){memcpy(old+at,bytes+WORLD_HEADER+i*WORLD_NODE,132);at+=132;}for(int r=0;r<captured.copy.ropes;++r){memcpy(old+at,bytes+WORLD_HEADER+captured.copy.count*WORLD_NODE+r*WORLD_ROPE,36);at+=36;}at+=4;WorldTestHash(old,at);
 assert(WorldDecode(old,at,&decoded)&&decoded.copy.node[0].poseFrame==0&&decoded.copy.node[0].thrustForce==3000);
 assert(WorldClear());captured.copy.node[0].poseFrame=3;assert(!PlaceCopy(NULL,&captured.copy,vec3_origin,captured.velocity,captured.angular)&&LiveCount()==0);captured.copy.node[0].poseFrame=1;assert(PlaceCopy(NULL,&captured.copy,vec3_origin,captured.velocity,captured.angular));assert(g_entities[MAX_CLIENTS].s.frame==1);
 // Parser bounds and short reads cannot walk past the asset or create a pose.
 assert(WorldClear());PhysicsReset();((md3Header_t*)poseModel)->ofsFrames=INT_MAX;a=CopySpawn(1,0,100);assert(!a&&LiveCount()==0);PhysicsReset();
 imports.SendServerCommand=Send;
 puts("MP decorative poses: frame cache/bounds, rigid shapes/ropes, held release, permissions, revision-scoped Undo, copy/reuse, JRW1-4 and malformed models passed.");
}

// Large/offset models must use the selected hull before allocation, including
// frame zero and a nonzero pose that fits where the default pose does not.
static vec3_t placementLow,placementHigh,placementPoint;static int placementLimit,placementPass,placementFlags;
static void PlacementTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=ENTITYNUM_WORLD;VectorCopy(end,t->endpos);
 if(!mins){assert(!flags);t->fraction=.5f;VectorSet(t->endpos,600,0,300);VectorSet(t->plane.normal,.6f,0,.8f);}
 else {VectorCopy(mins,placementLow);VectorCopy(maxs,placementHigh);VectorCopy(start,placementPoint);placementPass=pass;placementFlags=flags;t->startsolid=placementLimit&&maxs[0]-mins[0]>100;}
}
static void LargePlacementCases(void){
 PoseFixture();imports.Trace=PlacementTrace;placementLimit=0;
 md3Frame_t*f=(md3Frame_t*)((md3Header_t*)poseModel+1);VectorSet(f[0].bounds[0],-300,-80,-30);VectorSet(f[0].bounds[1],200,100,380);PhysicsReset();
 unsigned id=CopyId(1);gentity_t*e=Spawn(&g_entities[0],id);assert(e&&placementPass==ENTITYNUM_NONE&&placementFlags==JAR_TRACE_SPAWN_OVERLAP&&objects[e->s.number].mass==1500);
 // Plane support, including the model's offset origin, leaves a four-unit gap.
 assert(fabsf(.6f*(e->r.currentOrigin[0]-600-300)+.8f*(e->r.currentOrigin[2]-300-30)-4)<.001f);
 assert(placementLow[0]==-300&&placementHigh[2]==380&&e->s.origin2[0]==-300);
 objects[e->s.number].noGravity=1;objects[e->s.number].frozen=1;RigidSteps(1);jarMPPose_t pose;vec3_t low,high;
 assert(JAR_RigidRead(e->s.number,&pose,low,high)&&fabsf(low[0]+300)<.01f&&fabsf(high[2]-380)<.01f);
 CopyAssembly(&g_entities[0],e->s.number);G_FreeEntity(e);placementLimit=1;
 // Stale/tiny saved bounds cannot bypass frame-zero collision preflight.
 VectorSet(copies[0].node[0].mins,-1,-1,-1);VectorSet(copies[0].node[0].maxs,1,1,1);
 assert(!PlaceCopy(&g_entities[0],&copies[0],(float[3]){100,0,100},NULL,NULL)&&LiveCount()==0);
 copies[0].node[0].poseFrame=1;
 assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){100,0,100},NULL,NULL));e=&g_entities[MAX_CLIENTS];assert(e->s.frame==1&&e->r.maxs[0]==30);RigidSteps(1);assert(!rigidFailed);
 // A bad new pose cannot create an entity or disable already-running props.
 f[2].bounds[1][0]=NAN;int count=LiveCount();assert(!SpawnAtPose(&g_entities[0],id,(float[3]){500,0,100},vec3_origin,2)&&LiveCount()==count);RigidSteps(1);assert(!rigidFailed);
 assert(WorldClear());placementLimit=0;e=SpawnAt(&g_entities[0],id,(float[3]){500,0,100},(float[3]){0,90,0});assert(e&&fabsf(e->r.mins[0]+100)<.001f&&fabsf(e->r.maxs[1]-200)<.001f);
 assert(WorldClear());PhysicsReset();imports.Trace=Trace;
 puts("MP large props: full rotated hull preflight, surface support, frame-zero restore validation, direct selected-frame creation and isolated bad-pose refusal passed.");
}
