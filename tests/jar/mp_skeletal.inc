// Production game bridge/cache/lifetime paths; renderer traversal tested separately.
static int skeletalCalls;
static qboolean SkeletalBounds(const char*name,int frame,int*frames,vec3_t low,vec3_t high){
 assert(SkeletalModel(name));++skeletalCalls;*frames=6;if(frame<0||frame>=6||frame==4)return qfalse;
 VectorSet(low,-10,-20,-30);VectorSet(high,20+frame,30,40);if(frame==5)high[0]=2000;return qtrue;
}
static void SkeletalCases(void){
 PoseFixture();imports.JAR_ModelPoseBounds=SkeletalBounds;skeletalCalls=0;unsigned id=0;
 for(int i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==1&&SkeletalModel(jarMPCatalog[i].model)){id=jarMPCatalog[i].id;break;}assert(id);
 gentity_t*a=SpawnAtPose(&g_entities[0],id,(float[3]){100,0,100},vec3_origin,2);assert(a&&a->s.modelGhoul2&&a->s.frame==2&&a->r.maxs[0]==22);int n=a->s.number,identity=a->s.time2;
 assert(PhysicsPoseCount(n)==6&&skeletalCalls==1);objects[n].noGravity=1;RigidSteps(3);assert(skeletalCalls==1);
 aimed=n;Command(0,"jar_select","");Command(0,"jar_property","pose_next");assert(a->s.frame==3&&skeletalCalls==2&&a->s.time2==identity);Command(0,"jar_property","pose_next");assert(a->s.frame==3&&skeletalCalls==3);Command(0,"jar_property","pose_next");assert(skeletalCalls==3);
 Command(0,"jar_property","pose_undo");assert(a->s.frame==2&&skeletalCalls==3);
 Command(0,"jar_property","pose_set_2");assert(a->s.frame==1);Command(0,"jar_property","pose_undo");assert(a->s.frame==2);
 for(const char**bad=(const char*[]){"pose_set_0","pose_set_65537","pose_set_-1","pose_set_abc",NULL};*bad;++bad){Command(0,"jar_property",*bad);assert(a->s.frame==2);}
 const int cachedCalls=skeletalCalls; // Copying the selected frame uses cached bounds.
 CopyAssembly(&g_entities[0],n);G_FreeEntity(a);assert(PlaceCopy(&g_entities[0],&copies[0],(float[3]){300,0,100},NULL,NULL));a=&g_entities[n];assert(a->s.modelGhoul2&&a->s.frame==2&&a->s.time2!=identity&&skeletalCalls==cachedCalls);
 jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes);assert(WorldDecode(bytes,size,&decoded));assert(WorldClear());assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));assert(g_entities[n].s.modelGhoul2&&g_entities[n].s.frame==2);RigidSteps(1);
 int count=LiveCount();assert(!SpawnAtPose(&g_entities[0],id,(float[3]){500,0,100},vec3_origin,5)&&LiveCount()==count);RigidSteps(1);assert(!rigidFailed);
 assert(WorldClear());
 gentity_t *mapProp=SpawnAtPose(&g_entities[0],91157969u,(vec3_t){600,0,100},vec3_origin,3);
 assert(mapProp&&!strcmp(mapProp->classname,"misc_model_ghoul")&&mapProp->s.modelGhoul2&&mapProp->s.frame==3);
 assert(!strcmp(mapProp->model,"models/players/r2d2/model.glm")&&mapProp->r.maxs[0]==23);
 assert(objects[mapProp->s.number].frozen&&PhysicsPoseCount(mapProp->s.number)==6);
 assert(WorldClear());PhysicsReset();imports.JAR_ModelPoseBounds=NULL;
 puts("MP skeletal props: native bounds/count cache, physics, pose Undo/refusal, generation snapshots, copy/save reconstruction and oversized-pose isolation passed.");
}
