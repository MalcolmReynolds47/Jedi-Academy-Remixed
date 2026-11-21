// Exercise the camera's owned parts, scoped picking, controls and journal.
static float doorCameraTraceFraction=1;
static void DoorCameraFixtureTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,
                                   const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mask==(MASK_SOLID|CONTENTS_BODY)&&mins&&mins[0]==-14&&maxs[0]==14){
  t->fraction=doorCameraTraceFraction;
  vec3_t delta;VectorSubtract(end,start,delta);
  VectorMA(start,t->fraction,delta,t->endpos);
 }
}

static void DoorCameraFixtureTick(gentity_t *camera,int ms){
 level.time+=ms;
 DoorCameraFrame(camera);
}

static void DoorCameraFixturePose(gentity_t *camera,float extension){
 const int n=camera->s.number;
 gentity_t *shaft=DoorCameraPart(n,JAR_MP_DOOR_CAMERA_SHAFT),*head=DoorCameraPart(n,JAR_MP_DOOR_CAMERA_HEAD);
 assert(shaft&&head&&DoorCameraRoot(shaft->s.number)==n&&DoorCameraRoot(head->s.number)==n);
 assert(shaft->s.time2==camera->s.time2&&head->s.time2==camera->s.time2);
 assert(shaft->s.modelindex==FIXTURE_DOOR_CAMERA_SHAFT&&head->s.modelindex==FIXTURE_DOOR_CAMERA_HEAD);
 assert(shaft->r.contents==CONTENTS_ITEM&&head->r.contents==CONTENTS_ITEM);
 vec3_t axis[3],shaftAxis[3],expected,low,high;
 AnglesToAxis(camera->r.currentAngles,axis);AnglesToAxis(shaft->r.currentAngles,shaftAxis);
 const float span=JAR_MP_DOOR_CAMERA_CLOSED+extension;
 VectorMA(camera->r.currentOrigin,span,axis[0],expected);
 assert(Distance(expected,head->r.currentOrigin)<.002f);
 assert(Distance(camera->r.currentOrigin,shaft->r.currentOrigin)<.002f);
 assert(fabsf(shaft->s.speed-span/JAR_MP_DOOR_CAMERA_SHAFT_LENGTH)<.002f);
 assert(DotProduct(axis[0],shaftAxis[2])<-.999f&&DotProduct(axis[1],shaftAxis[1])>.999f);
 DoorCameraBounds(camera->r.currentAngles,extension,low,high);
 assert(Distance(low,camera->r.mins)<.002f&&Distance(high,camera->r.maxs)<.002f);
}

static void DoorCameraCases(void){
 RigidFixture();imports.Trace=DoorCameraFixtureTrace;doorCameraTraceFraction=1;
 gentity_t *owner=&g_entities[0],*observer=&g_entities[1];
 gentity_t *camera=SpawnWithController(owner,JAR_MP_DOOR_CAMERA_ID,(vec3_t){200,0,100},vec3_origin);
 assert(camera&&RackFixtureEntities()==3);
 const int n=camera->s.number;
 assert(ControllerKind(n)==JAR_DOOR_CAMERA_CONTROLLER&&camera->s.modelindex==FIXTURE_DOOR_CAMERA_BASE);
 assert(PhysicsProp(n)&&!PhysicsSolid(n)&&camera->r.contents==CONTENTS_ITEM&&objects[n].mass==20);
 assert(!(ContextCaps(owner,n)&(JAR_CONTEXT_MASS|JAR_CONTEXT_POSE)));
 DoorCameraFixturePose(camera,0);
 const gentity_t *head=DoorCameraPart(n,JAR_MP_DOOR_CAMERA_HEAD);
 vec3_t from;VectorCopy(head->r.currentOrigin,from);from[1]-=40;
 trace_t picked={0};
 picked.entityNum=ENTITYNUM_WORLD;picked.fraction=.5f;VectorCopy(from,picked.endpos);picked.endpos[1]+=80;
 trace_t hidden=picked;hidden.endpos[1]-=60;
 NonsolidToolAim(owner,from,&hidden);assert(hidden.entityNum==ENTITYNUM_WORLD);
 NonsolidToolAim(observer,from,&picked);assert(picked.entityNum==ENTITYNUM_WORLD);
 NonsolidToolAim(owner,from,&picked);
 assert(picked.entityNum==n&&picked.fraction<.5f&&picked.plane.normal[1]<-.99f);

 gentity_t *box=CopySpawn(1,0,600);
 assert(box&&PhysicsSolid(box->s.number)&&PhysicsPrepare());
 VectorSet(owner->client->ps.origin,150,0,76);
 ActorArm(0,n);assert(players[0].held==n);
 ActorTurnInput(owner,15,0);
 for(int i=0;i<3;++i)ActorTick();
 assert(players[0].held==n&&fabsf(camera->r.currentAngles[YAW]-15)<.1f);
 DoorCameraFixturePose(camera,0);
 Drop(0,qtrue);
 owner->client->pers.cmd.buttons=0;
 G_FreeEntity(box);

 players[0].selected=n;
 Command(0,"jar_property","activate");
 DoorCameraFixtureTick(camera,200);
 DoorCameraFixturePose(camera,24);
 assert(camera->s.loopSound==FIXTURE_DOOR_CAMERA_SERVO&&camera->s.time==JAR_MP_DOOR_CAMERA_MOVING);
 doorCameraTraceFraction=.5f;
 DoorCameraFixtureTick(camera,100);
 DoorCameraFixturePose(camera,30);
 assert(!camera->s.loopSound&&camera->s.time==JAR_MP_DOOR_CAMERA_BLOCKED);
 doorCameraTraceFraction=0;
 DoorCameraFixtureTick(camera,100);
 DoorCameraFixturePose(camera,30);
 doorCameraTraceFraction=1;
 DoorCameraFixtureTick(camera,50);
 DoorCameraFixturePose(camera,36);

 Command(0,"jar_property","controller_camera_speed_80125");
 DoorCameraFixtureTick(camera,100);
 DoorCameraFixturePose(camera,44.0125f);
 assert(ContextUndoReady(owner,n));
 Command(0,"jar_property","property_undo");
 jarControllerState_t state;ControllerState(n,&state);
 assert(state.speed==120000&&state.damage==48000&&abs(state.pushSpeed-44013)<=1);
 Command(0,"jar_property","controller_camera_travel_300000");
 ControllerState(n,&state);assert(state.damage==300000);
 Command(0,"jar_property","property_undo");
 Command(0,"jar_property","controller_camera_travel_0");
 ControllerState(n,&state);assert(state.damage==48000);
 Command(0,"jar_property","controller_enabled");
 DoorCameraFixtureTick(camera,200);
 DoorCameraFixturePose(camera,44.0125f);
 assert(!camera->s.loopSound&&camera->s.time==JAR_MP_DOOR_CAMERA_REST);
 assert(ContextTransformApply(n,(vec3_t){300,60,130},(vec3_t){12,90,24}));
 DoorCameraFixturePose(camera,44.0125f);
 Command(0,"jar_property","controller_enabled");

 CopyAssembly(owner,n);
 assert(copies[0].count==1&&copies[0].node[0].controller.pushSpeed==state.pushSpeed);
 assert(PlaceCopy(owner,&copies[0],(vec3_t){500,60,130},NULL,NULL));
 gentity_t *copy=&g_entities[players[0].selected];
 assert(copy!=camera&&ControllerKind(copy->s.number)==JAR_DOOR_CAMERA_CONTROLLER&&!copy->s.loopSound);
 DoorCameraFixturePose(copy,44.0125f);
 Undo(owner,qfalse);
 assert(!copy->inuse&&camera->inuse&&RackFixtureEntities()==3);

 static jarWorld_t saved,decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved)&&saved.copy.count==1);
 assert(saved.copy.node[0].controller.active&&saved.copy.node[0].controller.pushSpeed==state.pushSpeed);
 const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 decoded.copy.node[0].controller.pushSpeed=48001;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear()&&RackFixtureEntities()==0);
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 camera=StockExplosiveFind(JAR_DOOR_CAMERA_CONTROLLER);
 assert(camera&&RackFixtureEntities()==3&&!camera->s.loopSound);
 DoorCameraFixturePose(camera,44.0125f);
 DoorCameraFixtureTick(camera,100);DoorCameraFixturePose(camera,48);
 assert(!camera->s.loopSound&&camera->s.time==JAR_MP_DOOR_CAMERA_REST);
 assert(WorldClear()&&RackFixtureEntities()==0);

 rackAllocationFailureCountdown=3;
 assert(!SpawnWithController(owner,JAR_MP_DOOR_CAMERA_ID,(vec3_t){200,0,100},vec3_origin));
 assert(!rackAllocationFailureCountdown&&RackFixtureEntities()==0);
 PhysicsReset();
 puts("MP door camera: stock parts and servo, scoped pick, obstruction/retry, edit Undo, Physgun, full pose, copy, JRW18 and rollback passed.");
}
