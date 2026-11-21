// Actual frame/Box3D/fallback routes; engine area queries are a bounded fixture.
static int TriggerPropBoxes(const vec3_t mins,const vec3_t maxs,int*list,int limit){
 int count=0;for(int n=MAX_CLIENTS;n<level.num_entities&&count<limit;++n){gentity_t*e=&g_entities[n];if(!e->inuse||!e->r.linked)continue;int a=0;for(;a<3;++a)if(e->r.absmin[a]>maxs[a]||e->r.absmax[a]<mins[a])break;if(a==3)list[count++]=n;}return count;
}
static void TriggerPropAt(gentity_t*e,const vec3_t point){Move(e,point);VectorClear(objects[e->s.number].velocity);VectorClear(e->s.pos.trDelta);PhysicsVelocity(e->s.number,vec3_origin);}
static void TriggerPropCases(void){
 for(int rigid=0;rigid<2;++rigid){
  RigidFixture();imports.EntitiesInBox=TriggerPropBoxes;rigidEnabled.integer=rigid;
  gentity_t*push=TransportSpawn("trigger_push",100),*dest=TransportDestination(push),*p=CopySpawn(1,0,120);assert(push&&dest&&p);
  int root=push->s.number,n=p->s.number;objects[root].frozen=objects[dest->s.number].frozen=1;objects[n].noGravity=1;objects[n].mass=500;objects[n].angular[2]=1;
  vec3_t point={120,40,80};TriggerPropAt(p,point);ActorTick();assert(fabsf(objects[n].velocity[0]-800)<.001f&&fabsf(objects[n].velocity[2]-600)<.001f&&VectorCompare(objects[n].velocity,p->s.pos.trDelta));
  jarMPPose_t pose;vec3_t low,high;if(rigid){assert(JAR_RigidRead(n,&pose,low,high)&&VectorCompare(pose.linear,objects[n].velocity)&&fabsf(pose.angular[2])>.9f);}
  // Disabling the volume leaves the launched prop moving on subsequent frames.
  push->flags|=FL_INACTIVE;vec3_t before;VectorCopy(p->r.currentOrigin,before);ActorTick();assert(p->r.currentOrigin[0]>before[0]+15&&p->r.currentOrigin[2]>before[2]+10);push->flags&=~FL_INACTIVE;
  TriggerPropAt(p,point);objects[n].mass=5;ActorTick();assert(fabsf(objects[n].velocity[0]-800)<.001f); // speed is mass-independent
  // Frozen, held, unfinished copies, ownership and disabled sources cannot push.
  for(int refusal=0;refusal<6;++refusal){TriggerPropAt(p,point);objects[n].frozen=refusal==0;objects[n].heldBy=refusal==1?1:0;objects[n].copyPending=refusal==2;objects[n].owner=refusal==3?2:1;push->flags=refusal==4?FL_INACTIVE:0;objects[root].triggerActive=refusal==5?0:1;TriggerPropBegin();TriggerFrame();assert(!VectorLength(objects[n].velocity));}
  objects[n].frozen=objects[n].heldBy=objects[n].copyPending=0;objects[n].owner=1;push->flags=0;objects[root].triggerActive=1;TriggerSync(root);
  objects[n].owner=2;sharedEditing.integer=1;TriggerPropBegin();TriggerFrame();assert(fabsf(objects[n].velocity[0]-800)<.001f);sharedEditing.integer=0;objects[n].owner=1;
  objects[dest->s.number].owner=2;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();assert(!VectorLength(objects[n].velocity));objects[dest->s.number].owner=1;
  // Relative and arc modes use the same validated trajectory as actors.
  push->spawnflags=20;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();vec3_t expected;VectorSubtract(dest->r.currentOrigin,point,expected);VectorNormalize(expected);VectorScale(expected,1000,expected);assert(Distance(expected,objects[n].velocity)<.001f);
  push->spawnflags=0;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();assert(fabsf(objects[n].velocity[0]-128/sqrtf(.24f))<.001f&&fabsf(objects[n].velocity[2]-sqrtf(153600))<.001f);
  dest->r.currentOrigin[2]=push->r.currentOrigin[2]-1;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();assert(!VectorLength(objects[n].velocity));dest->r.currentOrigin[2]=push->r.currentOrigin[2]+96;
  // Native shared cooldown includes prop and player contacts; MULTIPLE permits
  // the same frame, once disarms after one prop, and invalid attempts consume none.
  push->spawnflags=4;push->wait=500;push->painDebounceTime=level.time-500;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();int deadline=push->painDebounceTime+500;assert(deadline==level.time+500);
  TriggerPlaceActor(&g_entities[0],push);VectorClear(clients[0].ps.velocity);TriggerEnter(push,&g_entities[0]);assert(!VectorLength(clients[0].ps.velocity));
  push->spawnflags|=2048;TriggerEnter(push,&g_entities[0]);assert(fabsf(clients[0].ps.velocity[0]-800)<.001f);++level.time;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();assert(!VectorLength(objects[n].velocity)&&push->painDebounceTime+500==deadline);
  level.time=deadline;TriggerPropBegin();TriggerFrame();assert(fabsf(objects[n].velocity[0]-800)<.001f);
  push->spawnflags=4;push->wait=-1;++level.time;TriggerPropAt(p,point);TriggerPropBegin();TriggerFrame();assert(!objects[root].triggerActive&&!TriggerHelper(root)->r.linked&&fabsf(objects[n].velocity[0]-800)<.001f);
  // Prop state, spent volume and destination survive the actual world codec.
  static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&world));int length=WorldEncode(&world,bytes);assert(WorldDecode(bytes,length,&decoded));assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
  int props=0,volumes=0;for(int i=MAX_CLIENTS;i<level.num_entities;++i){if(ControllerKind(i)==22){assert(!objects[i].triggerActive&&TransportDestination(&g_entities[i]));++volumes;}else if(PhysicsProp(i)&&!ControllerKind(i)){assert(fabsf(objects[i].velocity[0]-800)<.001f&&fabsf(objects[i].angular[2])>.9f);++props;}}assert(props==1&&volumes==1);PhysicsReset();
 }
 // Fast translation across a complete volume still triggers, but a rotated
 // broad-phase corner does not. Slot reuse cannot inherit another prop's path.
 RigidFixture();imports.EntitiesInBox=TriggerPropBoxes;gentity_t*push=TransportSpawn("trigger_push",100),*p=CopySpawn(1,0,-100);int root=push->s.number,n=p->s.number;
 vec3_t start={-100,40,80},end={300,40,80};TriggerPropAt(p,start);TriggerPropBegin();Move(p,end);TriggerFrame();assert(fabsf(objects[n].velocity[0]-800)<.001f);
 assert(PhysicsSetAngles(push,(float[3]){0,45,0}));vec3_t corner={180,80,80};TriggerPropAt(p,corner);TriggerPropBegin();TriggerFrame();assert(!VectorLength(objects[n].velocity));
 assert(PhysicsSetAngles(push,vec3_origin));TriggerPropAt(p,start);TriggerPropBegin();G_FreeEntity(p);p=CopySpawn(1,0,300);assert(p->s.number==n);TriggerPropAt(p,end);TriggerFrame();assert(!VectorLength(objects[n].velocity));
 // Frozen destination/body never propel themselves or consume a one-shot.
 objects[root].frozen=0;gentity_t*dest=TransportDestination(push);objects[dest->s.number].frozen=0;Move(dest,push->r.currentOrigin);push->wait=-1;TriggerPropBegin();TriggerFrame();assert(objects[root].triggerActive&&!VectorLength(objects[root].velocity)&&!VectorLength(objects[dest->s.number].velocity));
 // Reset removes pending contact work; no simulation means no duplicate touch.
 TriggerPropBegin();TriggerPropReset();TriggerFrame();assert(objects[root].triggerActive);imports.EntitiesInBox=FixtureEntitiesInBox;PhysicsReset();
 puts("MP prop push volumes: real Box3D and fallback frames, mass-independent speed, retained spin/momentum, swept rotated contacts, consent/freeze/hold/copy restrictions, shared actor cooldown/once and JRW16 motion restore passed.");
}
