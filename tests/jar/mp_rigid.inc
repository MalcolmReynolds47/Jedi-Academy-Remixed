// Production MP adapter + real Box3D; only filesystem, map and engine traces are fixtures.
static byte rigidModel[sizeof(md3Header_t)+sizeof(md3Frame_t)];static int rigidReadAt,rigidBrushReads,rigidBrushFail;
static int RigidOpen(const char*path,fileHandle_t*f,fsMode_t mode){*f=1;rigidReadAt=0;return sizeof(rigidModel);}
static int RigidRead(void*out,int count,fileHandle_t f){assert(f==1&&count>=0&&rigidReadAt+count<=sizeof(rigidModel));memcpy(out,rigidModel+rigidReadAt,count);rigidReadAt+=count;return count;}
static int RigidMap(int model,int mask,void*ctx,qboolean(*emit)(void*,int,const float*,const float*)){
 if(model){
  const float floor[]={-64,-64,0,64,-64,0,64,64,0,-64,64,0},normal[]={0,0,1};assert((model==7||model==8)&&mask==(CONTENTS_SOLID|CONTENTS_BODY));++rigidBrushReads;
  return !rigidBrushFail&&emit(ctx,4,floor,normal)?1:-1;
 }
 const float floor[]={-2048,-2048,0,2048,-2048,0,2048,2048,0,-2048,2048,0},normal[]={0,0,1};assert(model==0&&mask==CONTENTS_SOLID);return emit(ctx,4,floor,normal)?1:-1;
}
static void RigidLink(sharedEntity_t*e){Link(e);for(int k=0;k<3;++k){e->r.absmin[k]=e->r.currentOrigin[k]+e->r.mins[k];e->r.absmax[k]=e->r.currentOrigin[k]+e->r.maxs[k];}}
static void RigidTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=ENTITYNUM_WORLD;t->plane.normal[2]=1;VectorCopy(end,t->endpos);t->startsolid=blocked;if(!mins){t->fraction=.5f;t->entityNum=aimed;}
 if(mins&&start[2]+mins[2]<-.01f)t->startsolid=qtrue;
 if(mins&&end[2]+mins[2]<0){t->fraction=(start[2]+mins[2])/(start[2]-end[2]);for(int k=0;k<3;++k)t->endpos[k]=start[k]+t->fraction*(end[k]-start[k]);}
}
static void RigidFixture(void){
 rigidBrushReads=rigidBrushFail=0;
 imports.FS_Open=RigidOpen;imports.FS_Read=RigidRead;imports.FS_Close=Close;imports.Print=Print;imports.JAR_CollisionPolygons=RigidMap;imports.LinkEntity=RigidLink;imports.Trace=RigidTrace;
 memset(rigidModel,0,sizeof(rigidModel));md3Header_t*h=(md3Header_t*)rigidModel;md3Frame_t*f=(md3Frame_t*)(rigidModel+sizeof(*h));h->ident=MD3_IDENT;h->version=MD3_VERSION;h->numFrames=1;h->ofsFrames=sizeof(*h);h->ofsEnd=sizeof(rigidModel);VectorSet(f->bounds[0],-12,-8,-4);VectorSet(f->bounds[1],20,8,12);
 ActorFixture();Q_strncpyz(level.mapname,"mp/ffa3",sizeof(level.mapname));worldTried=qtrue;worldAutoSave.integer=0;g_gravity.value=800;blockAt=0;
}
static qboolean DiagonalModelHull(const char* model,int frame,void* context,void (*emit)(void*,const float*)){
 assert(model&&frame==0);
 for(int along=-1;along<=1;along+=2)for(int across=-1;across<=1;across+=2)
  for(int z=-1;z<=1;z+=2){
   const vec3_t p={along*16.f+across*2.f,-along*16.f+across*2.f,z*4.f};
   emit(context,p);
  }
 return qtrue;
}
static void RigidModelHullCases(void){
 RigidFixture();
 imports.JAR_VisitModelHull=DiagonalModelHull;
 gentity_t* prop=CopySpawn(1,0,100);
 const int n=prop->s.number;
 PhysicsPublish(prop);
 assert(JAR_MP_ModelHullState(&prop->s));
 vec3_t low,high;
 const vec3_t angle={0,45,0};
 PhysicsHull(prop,angle,low,high);
 assert(high[0]-low[0]>44&&high[1]-low[1]<6);
 VectorCopy(angle,prop->r.currentAngles);
 assert(PhysicsPrepare());
 jarMPPose_t pose;
 assert(JAR_RigidRead(n,&pose,low,high)&&high[0]-low[0]>44&&high[1]-low[1]<6);
 vec3_t origin,actor,normal,delta={-1,-1,-1},extent={1,1,1};float depth=0;
 VectorCopy(prop->r.currentOrigin,origin);
 VectorSet(actor,origin[0]+10,origin[1]+10,origin[2]);
 assert(JAR_RigidModelActor(prop->model,0,angle,origin,origin,actor,delta,extent,normal,&depth)==0);
 VectorCopy(origin,actor);
 assert(JAR_RigidModelActor(prop->model,0,angle,origin,origin,actor,delta,extent,normal,&depth)==1&&depth>0);
 imports.JAR_VisitModelHull=NULL;
 PhysicsReset();
}
static void PortableMapModelCases(void){
 const unsigned ids[]={88708799u,99343814u};
 const char *classes[]={"misc_model","misc_model_static"};
 for(int i=0;i<ARRAY_LEN(ids);++i){
  RigidFixture();
  gentity_t *owner=&g_entities[0],*model=Spawn(owner,ids[i]);
  assert(model&&!strcmp(model->classname,classes[i]));
  const int n=model->s.number;
  assert(model->r.linked&&model->r.contents==CONTENTS_SOLID&&model->s.modelindex&&!model->takedamage);
  assert(!strcmp(model->model,"models/map_objects/imperial/crate1.md3"));
  assert(PhysicsProp(n)&&objects[n].frozen&&objects[n].mass>0&&!ControllerKind(n));
  const int caps=ContextCaps(owner,n);
  assert((caps&(JAR_CONTEXT_RELEASE|JAR_CONTEXT_ROTATE|JAR_CONTEXT_MASS))==
         (JAR_CONTEXT_RELEASE|JAR_CONTEXT_ROTATE|JAR_CONTEXT_MASS));
  CopyAssembly(owner,n);
  assert(copies[0].count==1&&copies[0].node[0].id==ids[i]);
  assert(PlaceCopy(owner,&copies[0],(vec3_t){900,100,80},NULL,NULL));
  const gentity_t *copy=&g_entities[players[0].selected];
  assert(copy!=model&&copy->inuse&&!strcmp(copy->classname,classes[i]));
  assert(copy->s.modelindex==model->s.modelindex&&objects[copy->s.number].frozen);
  assert(WorldClear());
 }
 puts("MP map-model cards: retail collision body, Context controls and class-preserving duplication passed.");
}
static void RigidSteps(int frames){for(int i=0;i<frames;++i)ActorTick();assert(JAR_RigidActive());}

static void FallenObjectCases(void){
 RigidFixture();
 mapFallBottom=0;
 imports.Trace=Trace;
 gentity_t *prop=CopySpawn(1,0,4096), *safe=CopySpawn(1,0,100);
 assert(prop&&safe);
 const int slot=prop->s.number;
 Move(prop,(float[3]){4096,0,-250});
 objects[slot].velocity[2]=-500;
 gentity_t *rope=CopyLink(0,slot,ENTITYNUM_WORLD);
 VectorCopy(prop->r.currentOrigin,objects[rope->s.number].anchorB);
 objects[rope->s.number].length=200;
 gentity_t *marker=G_Spawn();
 Move(marker,(float[3]){0,0,-500});
 players[0].selected=slot;
 StatsEcho(&g_entities[0]);
 assert(!strcmp(spawnStatsEcho[0],"jarstats 2 2 1 0"));
 RigidSteps(5);
 assert(!prop->inuse&&!rope->inuse&&safe->inuse&&marker->inuse);
 assert(players[0].selected==ENTITYNUM_NONE&&!objects[slot].owner&&!JAR_RigidOutOfBounds(slot));
 assert(!strcmp(spawnStatsEcho[0],"jarstats 1 1 1 0"));
 assert(!strcmp(spawnStatsEcho[1],"jarstats 0 1 1 0"));

 prop=CopySpawn(1,0,100);
 assert(prop);
 Move(prop,(float[3]){65530,0,80});
 objects[prop->s.number].velocity[0]=1000;
 RigidSteps(1);
 assert(!prop->inuse&&safe->inuse);

 RigidFixture();
 mapFallBottom=0;
 rigidEnabled.integer=0;
 imports.Trace=Trace;
 prop=CopySpawn(1,0,100);
 assert(prop);
 gentity_t *section=G_Spawn();
 Track(section,0,-1);
 section->s.eType=ET_MOVER;
 section->s.generic1=SECTION_MARKER;
 section->r.bmodel=qtrue;
 VectorSet(section->r.mins,-32,-32,-80);
 VectorSet(section->r.maxs,32,32,80);
 Move(section,(float[3]){0,0,-400});
 Move(prop,(float[3]){100,0,-500});
 ActorTick();
 assert(section->inuse&&prop->inuse&&fallFloor==-736);
 G_FreeEntity(section);
 ActorTick();
 assert(!prop->inuse);

 prop=CopySpawn(1,0,100);
 assert(prop);
 Move(prop,(float[3]){100,0,-266});
 objects[prop->s.number].velocity[2]=-500;
 ActorTick();
 assert(!prop->inuse&&!JAR_RigidActive());

 gentity_t *vehicle=CopySpawn(3,0,100);
 assert(vehicle);
 CopyReady(vehicle);
 StatsEcho(&g_entities[0]);
 assert(!strcmp(spawnStatsEcho[0],"jarstats 1 0 0 1"));
 vehicle->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[0];
 Move(vehicle,(float[3]){100,0,-500});
 CullFallenObjects();
 assert(vehicle->inuse);
 vehicle->m_pVehicle->m_pPilot=NULL;
 CullFallenObjects();
 assert(!vehicle->inuse);
}
static void RigidProxyTurnover(void){
 JAR_RigidReset();assert(RigidMap(0,CONTENTS_SOLID,NULL,PhysicsPolygon)>0&&JAR_RigidStart());
 for(unsigned model=7;model<=8;++model){JAR_RigidBeginGeometry();assert(RigidMap(model,CONTENTS_SOLID|CONTENTS_BODY,NULL,PhysicsPolygon)>0&&JAR_RigidAddProxyMesh(model));}
 const jarMPPose_t pose={0};const vec3_t half={4,4,4};
 JAR_RigidBeginProxies();
 assert(JAR_RigidMeshProxy(1,1,7,&pose)&&JAR_RigidMeshProxy(2,1,8,&pose)&&JAR_RigidMeshProxy(3,1,8,&pose));
 for(unsigned n=4;n<=JAR_MP_RIGID_PROXY_LIMIT;++n)assert(JAR_RigidProxy(n,1,half,vec3_origin,&pose));
 assert(!JAR_RigidProxy(JAR_MP_RIGID_PROXY_LIMIT+1,1,half,vec3_origin,&pose));
 JAR_RigidEndProxies();

 JAR_RigidBeginProxies();
 JAR_RigidKeepProxy(1,1,7); // same lifetime and model
 JAR_RigidKeepProxy(2,2,8); // recycled slot, same model
 JAR_RigidKeepProxy(3,1,0); // same slot now needs a native box
 JAR_RigidEndProxies();
 assert(JAR_RigidHasProxyMesh(7)&&!JAR_RigidHasProxyMesh(8));
 for(unsigned n=JAR_MP_RIGID_PROXY_LIMIT+1;n<JAR_MP_RIGID_PROXY_LIMIT*2;++n)assert(JAR_RigidProxy(n,1,half,vec3_origin,&pose));
 assert(!JAR_RigidProxy(JAR_MP_RIGID_PROXY_LIMIT*2,1,half,vec3_origin,&pose));
 JAR_RigidReset();
}
static int RigidNearbyBoxes(const vec3_t mins,const vec3_t maxs,int *entities,int capacity){
 int count=0;
 for(int n=0;n<level.num_entities&&count<capacity;++n){
  const gentity_t *e=&g_entities[n];
  if(e->inuse&&e->r.linked&&PhysicsBoxesOverlap(mins,maxs,e->r.absmin,e->r.absmax))entities[count++]=n;
 }
 return count;
}
static void RigidCityNearCases(void){
 RigidFixture();
 gentity_t *left=CopySpawn(1,0,-1900),*right=CopySpawn(1,0,1900);
 assert(left&&right);
 gentity_t brush={0};
 VectorSet(brush.r.mins,-16,984,64);VectorSet(brush.r.maxs,16,1016,96);
 VectorSet(brush.r.currentAngles,0,90,0);
 VectorSet(brush.r.absmin,-1030,-1030,-1030);VectorSet(brush.r.absmax,1030,1030,1030);
 const vec3_t pivotLow={-272,-272,-192},pivotHigh={272,272,352};
 assert(PhysicsBoxesOverlap(brush.r.absmin,brush.r.absmax,pivotLow,pivotHigh));
 assert(!PhysicsSectionBrushNear(&brush,pivotLow,pivotHigh));
 const vec3_t unionLow={-2200,-272,-192},unionHigh={2200,272,352};
 int (*previous)(const vec3_t,const vec3_t,int*,int)=imports.EntitiesInBox;
 imports.EntitiesInBox=RigidNearbyBoxes;
 assert(!PhysicsSectionBrushNear(&brush,unionLow,unionHigh)); // Empty space between props.
 vec3_t contact={-1000,0,80};Move(left,contact);
 assert(PhysicsSectionBrushNear(&brush,unionLow,unionHigh));
 imports.EntitiesInBox=previous;
}
static void RigidCases(void){
 RigidModelHullCases();
 RigidProxyTurnover();RigidCityNearCases();
 RigidFixture();gentity_t*a=CopySpawn(1,0,100);int n=a->s.number;assert(a&&a->r.mins[0]==-12&&a->r.maxs[0]==20&&a->s.generic1==250&&a->s.origin2[2]==-4);
 RigidSteps(160);assert(JAR_RigidTriangles()==2&&JAR_RigidSteps()>=479);assert(fabsf(a->r.currentOrigin[2]-4)<.5f&&fabsf(objects[n].velocity[2])<1);
 gentity_t*b=CopySpawn(1,0,100);RigidSteps(180);assert(fabsf(b->r.currentOrigin[2]-20)<.6f&&fabsf(a->r.currentOrigin[2]-4)<.5f); // actual solver stacking
 G_FreeEntity(b);G_FreeEntity(a);assert(!JAR_RigidRead(n,&(jarMPPose_t){0},(float[3]){0},(float[3]){0}));
 a=CopySpawn(1,0,100);assert(a->s.number==n);players[0].selected=n;Command(0,"jar_property","mass_200");Command(0,"jar_property","gravity");RigidSteps(30);assert(fabsf(a->r.currentOrigin[2]-80)<.01f);
 // Editor mass controls real acceleration. A center impulse cannot introduce spin.
 vec3_t push={2000,0,0},point={104,0,84},low,high;jarMPPose_t pose;assert(JAR_RigidImpulse(n,push,point));assert(JAR_RigidRead(n,&pose,low,high)&&fabsf(pose.linear[0]-10)<.01f);
 VectorCopy(pose.linear,objects[n].velocity);ForceImpulseAt(n,(float[3]){1,0,0},2000,200,point);assert(JAR_RigidRead(n,&pose,low,high)&&fabsf(pose.linear[0]-20)<.01f); // force starts from published velocity
 objects[n].frozen=1;RigidSteps(1);vec3_t position;VectorCopy(a->r.currentOrigin,position);RigidSteps(12);assert(Distance(a->r.currentOrigin,position)<.001f);
 vec3_t angle={0,90,0};assert(PhysicsSetAngles(a,angle));RigidSteps(1);assert(fabsf(a->r.mins[0]+8)<.01f&&fabsf(a->r.maxs[1]-20)<.01f);
 blocked=1;angle[YAW]=45;assert(!PhysicsSetAngles(a,angle)&&fabsf(a->r.currentAngles[YAW]-90)<.01f);blocked=0;
 // Physgun goes through the actual command/frame path, including USE rotation and fling.
 VectorAdd(a->r.currentOrigin,((vec3_t){-8,5,6}),physgunFixtureHit);
 VectorSet(clients[0].ps.origin,physgunFixtureHit[0]-200,physgunFixtureHit[1],physgunFixtureHit[2]-24);
 imports.Trace=PhysgunFixtureTrace;ActorArm(0,n);imports.Trace=RigidTrace;
 RigidSteps(10);assert(Distance(g_entities[players[0].beam].s.origin2,physgunFixtureHit)<.01f);
 clients[0].ps.viewangles[YAW]=30;clients[0].ps.viewangles[PITCH]=-25;RigidSteps(60);
 assert(fabsf(AngleSubtract(a->r.currentAngles[YAW],120))<1&&fabsf(a->r.currentAngles[PITCH])<.01f);
 assert(fabsf(g_entities[players[0].beam].s.origin2[2]-physgunFixtureHit[2]-200*sinf(25*M_PI/180))<.1f);
 // Horizontal USE motion rotates around camera up, even while looking above the prop.
 vec3_t cameraUp,propForward,beamBefore;
 AngleVectors(clients[0].ps.viewangles,NULL,NULL,cameraUp);
 AngleVectors(a->r.currentAngles,propForward,NULL,NULL);
 const float beforeTurn=DotProduct(cameraUp,propForward);
 VectorCopy(g_entities[players[0].beam].s.origin2,beamBefore);
 ActorTurnInput(&g_entities[0],15,0);
 RigidSteps(60);
 AngleVectors(a->r.currentAngles,propForward,NULL,NULL);
 assert(fabsf(DotProduct(cameraUp,propForward)-beforeTurn)<.01f);
 assert(a->r.currentAngles[PITCH]>5&&fabsf(AngleSubtract(a->r.currentAngles[YAW],120))>10);
 assert(clients[0].ps.viewangles[YAW]==30&&clients[0].ps.viewangles[PITCH]==-25);
 assert(Distance(g_entities[players[0].beam].s.origin2,beamBefore)<.1f);
 clients[0].pers.cmd.buttons=BUTTON_ATTACK;clients[0].ps.origin[0]+=80;RigidSteps(1);clients[0].pers.cmd.buttons=0;RigidSteps(1);assert(players[0].held==ENTITYNUM_NONE&&VectorLength(objects[n].velocity)>10);
 // A server-owned spinning prop restores the codec angular state into the solver.
 RigidFixture();a=CopySpawn(1,0,100);n=a->s.number;objects[n].noGravity=1;objects[n].angular[2]=1;
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int length=WorldEncode(&saved,bytes);assert(WorldDecode(bytes,length,&decoded)&&decoded.angular[0][2]==1);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));n=MAX_CLIENTS;a=&g_entities[n];RigidSteps(20);assert(a->r.currentAngles[YAW]>20&&fabsf(objects[n].angular[2])>.8f);
 // Disabling the solver freezes current props and safely permits later reinitialization.
 rigidEnabled.integer=0;ActorTick();assert(!JAR_RigidActive()&&objects[n].frozen);rigidEnabled.integer=1;RigidSteps(1);
 // Native player support transports without converting/consuming a second event snapshot.
 RigidFixture();a=CopySpawn(1,0,100);n=a->s.number;objects[n].noGravity=1;objects[n].mass=200;objects[n].velocity[0]=40;gentity_t*actor=&g_entities[0];
 VectorSet(actor->r.mins,-4,-4,0);VectorSet(actor->r.maxs,4,4,40);actor->r.contents=CONTENTS_BODY;vec3_t feet={100,0,92};Move(actor,feet);actor->client->ps.groundEntityNum=n;
 int events=snapshotCalls;RigidSteps(1);assert(actor->r.currentOrigin[0]>100&&snapshotCalls==events&&actor->health==100);
 // Blocked rider transport also rolls back rotation; no actor is partially moved.
 VectorCopy(a->r.currentOrigin,position);VectorCopy(actor->r.currentOrigin,feet);VectorCopy(a->r.currentAngles,angle);objects[n].angular[2]=1;pose=PhysicsPose(n);assert(JAR_RigidCorrect(n,&pose));blocked=1;RigidSteps(1);blocked=0;
 assert(Distance(position,a->r.currentOrigin)<.001f&&Distance(feet,actor->r.currentOrigin)<.001f&&fabsf(AngleSubtract(angle[YAW],a->r.currentAngles[YAW]))<.001f&&VectorLength(objects[n].angular)==0);
 // A prop cannot tunnel through a living actor; contact pushes both by mass.
 actor->client->ps.groundEntityNum=ENTITYNUM_NONE;feet[0]=150;feet[2]=80;Move(actor,feet);objects[n].velocity[0]=1000;PhysicsVelocity(n,objects[n].velocity);RigidSteps(3);assert(actor->r.currentOrigin[0]>150&&actor->client->ps.velocity[0]>0&&actor->health==100);
 // Non-actor solid entities use lifecycle-scoped proxies; removal/reuse is safe.
 gentity_t*wall=G_Spawn();wall->r.contents=CONTENTS_SOLID;VectorSet(wall->r.mins,-8,-20,-20);VectorSet(wall->r.maxs,8,20,20);vec3_t wallpos={400,0,80};Move(wall,wallpos);RigidSteps(1);int slot=wall->s.number;unsigned generation=rigidGeneration[slot];G_FreeEntity(wall);assert(rigidGeneration[slot]!=generation);wall=G_Spawn();assert(wall->s.number==slot);RigidSteps(1);
 // Native brush geometry uses entity origin/angles, never its padded broad-phase box.
 RigidFixture();a=CopySpawn(1,0,100);n=a->s.number;wall=G_Spawn();wall->r.bmodel=qtrue;wall->s.modelindex=7;wall->r.contents=CONTENTS_SOLID;
 VectorSet(wall->r.mins,-65,-65,-1);VectorSet(wall->r.maxs,65,65,100);VectorSet(wall->r.currentAngles,0,90,0);VectorSet(wallpos,100,0,40);Move(wall,wallpos);
 RigidSteps(80);assert(rigidBrushReads==1&&JAR_RigidHasProxyMesh(7));assert(fabsf(a->r.currentOrigin[2]-44)<.5f);
 for(int step=0;step<40;++step){wallpos[2]+=.25f;Move(wall,wallpos);RigidSteps(1);}RigidSteps(40);fprintf(stderr,"MP brush floor %.4f reads %d\n",a->r.currentOrigin[2],rigidBrushReads);assert(rigidBrushReads==1&&fabsf(a->r.currentOrigin[2]-54)<.5f);
 // Replacing its inline model rebuilds only that mesh; unlinking releases it.
 wall->s.modelindex=8;RigidSteps(1);assert(rigidBrushReads==2&&!JAR_RigidHasProxyMesh(7)&&JAR_RigidHasProxyMesh(8));
 G_FreeEntity(wall);RigidSteps(80);assert(!JAR_RigidHasProxyMesh(8)&&fabsf(a->r.currentOrigin[2]-4)<.5f);
 // A failed brush export freezes props, with no silently substituted solid box.
 wall=G_Spawn();wall->r.bmodel=qtrue;wall->s.modelindex=7;wall->r.contents=CONTENTS_SOLID;VectorSet(wall->r.mins,-65,-65,-1);VectorSet(wall->r.maxs,65,65,100);VectorSet(wallpos,100,0,0);Move(wall,wallpos);rigidBrushFail=1;
 ActorTick();assert(!JAR_RigidActive()&&objects[n].frozen&&rigidFailed);
 PhysicsReset();puts("MP rigid adapter: real floor/stack, mass, gravity, rotation, fling, spin persistence, actor push/support, brush transforms/cache/failure, snapshots and teardown passed.");
}
