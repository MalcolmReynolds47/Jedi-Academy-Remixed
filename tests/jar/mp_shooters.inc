// Portable controller lifecycle with a recorded native weapon boundary.
typedef struct {
 const char *name;
 unsigned id;
 int kind,weapon;
} shooterFixture_t;

static const shooterFixture_t shooterFixtures[]={
 {"shooter_plasma",86744182u,JAR_PLASMA_SHOOTER_CONTROLLER,WP_BRYAR_PISTOL},
 {"shooter_grenade",86306646u,JAR_GRENADE_SHOOTER_CONTROLLER,WP_THERMAL},
 {"shooter_rocket",90241160u,JAR_ROCKET_SHOOTER_CONTROLLER,WP_ROCKET_LAUNCHER},
 {"misc_weapon_shooter",98645765u,JAR_BLASTER_SHOOTER_CONTROLLER,WP_BLASTER}
};

static struct {
 int calls,source,weapon;
 qboolean alt,linked;
 vec3_t origin,direction,target;
} shooterShot;

static int shooterMissingAssets;
qboolean JAR_MP_ShooterAssets(int weapon){
 assert(weapon==WP_BRYAR_PISTOL||weapon==WP_THERMAL||weapon==WP_ROCKET_LAUNCHER||weapon==WP_BLASTER);
 return !shooterMissingAssets;
}

qboolean JAR_MP_FireShooter(gentity_t *source,int weapon,qboolean alt,const vec3_t origin,const vec3_t direction,const vec3_t target){
 unsigned epoch;
 assert(JAR_MP_ShooterSource(source,&epoch)&&epoch==JAR_MP_EntityEpoch(source->s.number));
 assert(fabsf(VectorLength(direction)-1)<.001f);
 ++shooterShot.calls;shooterShot.source=source->s.number;shooterShot.weapon=weapon;
 shooterShot.alt=alt;shooterShot.linked=target!=NULL;
 VectorCopy(origin,shooterShot.origin);VectorCopy(direction,shooterShot.direction);
 if(target)VectorCopy(target,shooterShot.target);else VectorClear(shooterShot.target);
 return qtrue;
}

static gentity_t *ShooterSpawn(const shooterFixture_t *fixture){
 assert(ControllerId(fixture->name)==fixture->id);
 const int firstSlot=level.num_entities;
 gentity_t *shooter=SpawnWithController(&g_entities[0],fixture->id,(vec3_t){100,0,80},(vec3_t){15,37,5});
 assert(shooter&&shooter->s.number==firstSlot&&level.num_entities==firstSlot+1);
 assert(!objects[firstSlot].output[0]&&!objects[firstSlot].output[1]);
 assert(ControllerKind(firstSlot)==fixture->kind&&objects[firstSlot].owner==1&&shooter->use&&shooter->r.linked);
 assert(!shooter->client&&!shooter->nextthink&&!shooterShot.calls);
 return shooter;
}

static void ShooterStateCheck(const gentity_t *shooter,int flags,int delay,int random,int active,int inactive){
 jarControllerState_t state;
 ControllerState(shooter->s.number,&state);
 assert(state.flags==flags&&state.waitMs==delay&&state.randomMs==random&&state.active==active&&state.inactive==inactive);
}

static void ShooterShotCheck(const shooterFixture_t *fixture,const gentity_t *shooter,qboolean alt,const gentity_t *target){
 vec3_t expectedDirection;
 assert(shooterShot.source==shooter->s.number&&shooterShot.weapon==fixture->weapon&&shooterShot.alt==alt);
 assert(shooterShot.linked==(target!=NULL));
 assert(Distance(shooterShot.origin,shooter->r.currentOrigin)<.01f);
 if(target){
  assert(Distance(shooterShot.target,target->r.currentOrigin)<.01f);
  VectorSubtract(shooterShot.target,shooterShot.origin,expectedDirection);VectorNormalize(expectedDirection);
 }else AngleVectors(shooter->r.currentAngles,expectedDirection,NULL,NULL);
 assert(DotProduct(expectedDirection,shooterShot.direction)>.99999f);
}

static void ShooterDefaults(void){
 for(int i=0;i<ARRAY_LEN(shooterFixtures);++i){
  const shooterFixture_t *fixture=&shooterFixtures[i];
  RigidFixture();imports.SendServerCommand=ControllerPropertySend;shooterShot.calls=0;
  gentity_t *owner=&g_entities[0],*other=&g_entities[1],*shooter=ShooterSpawn(fixture);
  const int n=shooter->s.number;
  ShooterStateCheck(shooter,0,500,0,0,0);
  ControllerShot(other,n,qfalse);assert(!shooterShot.calls);
  ControllerShot(owner,n,qtrue);assert(!shooterShot.calls&&players[0].selected==n);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==1&&!shooter->nextthink);
  ShooterShotCheck(fixture,shooter,qfalse,NULL);

  Move(shooter,(vec3_t){144,69,121});assert(PhysicsSetAngles(shooter,(vec3_t){-22,104,7}));
  VectorSet(shooter->s.origin,-1,-2,-3);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==2);
  ShooterShotCheck(fixture,shooter,qfalse,NULL);

  // Primary aims the projectile without activating the destination.
  gentity_t *target=ControllerAt("target_counter",0,350);
  SwitchLink(owner,shooter,target,0);
  players[0].linkOutput=1;ControllerLink(owner,n,qfalse);ControllerLink(owner,target->s.number,qfalse);
  assert(!objects[n].output[1]);players[0].linkOutput=0;
  VectorSet(target->s.origin,-1000,1000,200);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==3&&target->count==2);
  ShooterShotCheck(fixture,shooter,qfalse,target);
  Move(target,(vec3_t){200,520,240});
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==4&&target->count==2);
  ShooterShotCheck(fixture,shooter,qfalse,target);

  // Revoked ownership and deleted targets fall back to the source's facing.
  // A pending copy member suppresses the shot until its native pose is ready.
  objects[target->s.number].owner=2;
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==5);
  ShooterShotCheck(fixture,shooter,qfalse,NULL);
  objects[target->s.number].owner=1;objects[target->s.number].copyPending=1;
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==5);
  const int stale=target->s.number;G_FreeEntity(target);
  target=ControllerAt("target_counter",0,350);assert(target->s.number==stale&&!objects[n].output[0]);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==6);
  ShooterShotCheck(fixture,shooter,qfalse,NULL);

  ControllerSelect(shooter);assert(ContextCaps(owner,n)&JAR_CONTEXT_CONTROLLER);
  Command(0,"jar_property","controller_enabled");
  ShooterStateCheck(shooter,0,500,0,0,1);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==6);
  Command(0,"jar_property","property_undo");
  ShooterStateCheck(shooter,0,500,0,0,0);
  objects[n].copyPending=1;ControllerShot(owner,n,qfalse);objects[n].copyPending=0;
  sandbox.integer=0;ControllerShot(owner,n,qfalse);sandbox.integer=1;
  assert(shooterShot.calls==6&&WorldClear());
 }
}

static void ShooterRepeat(void){
 for(int i=0;i<ARRAY_LEN(shooterFixtures);++i){
  const shooterFixture_t *fixture=&shooterFixtures[i];
  RigidFixture();imports.SendServerCommand=ControllerPropertySend;shooterShot.calls=0;
  gentity_t *owner=&g_entities[0],*shooter=ShooterSpawn(fixture);
  const int n=shooter->s.number;
  ControllerSelect(shooter);
  Command(0,"jar_property","controller_shooter_alt");
  ShooterStateCheck(shooter,JAR_SHOOTER_ALT,500,0,0,0);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==1&&!shooter->nextthink);
  ShooterShotCheck(fixture,shooter,qtrue,NULL);
  Command(0,"jar_property","controller_shooter_repeat");
  Command(0,"jar_property","controller_delay_325");
  Command(0,"jar_property","controller_random_175");
  const int flags=JAR_SHOOTER_ALT|JAR_SHOOTER_REPEAT;
  const char *bad[]={"controller_delay_0","controller_delay_2147483648","controller_random_-1","controller_random_2147483647","controller_random_1.5"};
  for(int b=0;b<ARRAY_LEN(bad);++b)Command(0,"jar_property",bad[b]);
  ShooterStateCheck(shooter,flags,325,175,0,0);
  assert(shooterShot.calls==1&&!shooter->nextthink);

  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==2);
  ShooterStateCheck(shooter,flags,325,175,1,0);
  const int deadline=shooter->nextthink-level.time;
  assert(deadline>=325&&deadline<=500);
  ControllerTick(shooter,deadline-1);assert(shooterShot.calls==2);
  Move(shooter,(vec3_t){190,20,90});assert(PhysicsSetAngles(shooter,(vec3_t){10,45,0}));
  ControllerTick(shooter,1);assert(shooterShot.calls==3);
  ShooterShotCheck(fixture,shooter,qtrue,NULL);
  assert(shooter->nextthink-level.time>=325&&shooter->nextthink-level.time<=500);

  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==3&&!shooter->nextthink);
  ShooterStateCheck(shooter,flags,325,175,0,0);
  ControllerTick(shooter,500);assert(shooterShot.calls==3);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==4&&shooter->nextthink>level.time);
  Command(0,"jar_property","controller_shooter_repeat");
  ShooterStateCheck(shooter,JAR_SHOOTER_ALT,325,175,0,0);
  assert(shooterShot.calls==4&&!shooter->nextthink);

  Command(0,"jar_property","controller_shooter_repeat");
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==5&&shooter->nextthink>level.time);
  gentity_t *off=ControllerAt("target_deactivate",0,400),*on=ControllerAt("target_activate",0,600);
  SwitchLink(owner,off,shooter,0);SwitchLink(owner,on,shooter,0);
  ControllerShot(owner,off->s.number,qfalse);
  ShooterStateCheck(shooter,flags,325,175,0,1);
  assert(shooterShot.calls==5&&!shooter->nextthink);
  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==5);
  ControllerShot(owner,on->s.number,qfalse);
  ShooterStateCheck(shooter,flags,325,175,0,0);
  ControllerTick(shooter,500);assert(shooterShot.calls==5&&!shooter->nextthink);

  ControllerShot(owner,n,qfalse);assert(shooterShot.calls==6&&shooter->nextthink>level.time);
  Command(0,"jar_property","controller_enabled");
  ShooterStateCheck(shooter,flags,325,175,0,1);
  assert(shooterShot.calls==6&&!shooter->nextthink);
  Command(0,"jar_property","controller_enabled");
  ShooterStateCheck(shooter,flags,325,175,0,0);
  ControllerTick(shooter,500);assert(shooterShot.calls==6&&!shooter->nextthink);
  assert(WorldClear());
 }
}

static gentity_t *ShooterFind(int kind){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==kind)return &g_entities[n];
 return NULL;
}

static void ShooterPersistence(const shooterFixture_t *fixture){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;shooterShot.calls=0;
 gentity_t *owner=&g_entities[0],*shooter=ShooterSpawn(fixture),*target=ControllerAt("target_position",0,390);
 const int source=shooter->s.number;
 SwitchLink(owner,shooter,target,0);
 ControllerSelect(shooter);
 Command(0,"jar_property","controller_shooter_alt");
 Command(0,"jar_property","controller_shooter_repeat");
 const int flags=JAR_SHOOTER_ALT|JAR_SHOOTER_REPEAT;
 ControllerShot(owner,source,qfalse);assert(shooterShot.calls==1);
 ShooterShotCheck(fixture,shooter,qtrue,target);
 ControllerTick(shooter,177);assert(shooterShot.calls==1);
 const int remaining=323;
 assert(shooter->nextthink-level.time==remaining);

 CopyAssembly(owner,source);
 assert(copies[0].count==2&&CopyControllersValid(&copies[0]));
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);
 assert(size>0&&WorldDecode(bytes,size,&decoded));
 jarCopyNode_t *node=NULL;
 for(int i=0;i<decoded.copy.count;++i)if(decoded.copy.node[i].id==fixture->id)node=&decoded.copy.node[i];
 assert(node&&node->controller.flags==flags&&node->controller.active==1&&node->controllerRemaining==remaining);
 assert(node->output[0]&&decoded.copy.node[node->output[0]-1].id==ControllerId("target_position")&&!node->output[1]);
 node->controllerRemaining=0;assert(!CopyControllersValid(&decoded.copy));
 node->controllerRemaining=node->controller.waitMs+1;assert(!CopyControllersValid(&decoded.copy));
 node->controllerRemaining=remaining;node->output[1]=node->output[0];assert(!CopyControllersValid(&decoded.copy));
 node->output[1]=0;node->controller.flags=JAR_SHOOTER_ALT;assert(!CopyControllersValid(&decoded.copy));
 node->controller.flags=flags;node->controller.inactive=node->controllerInactive=1;assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(bytes,size,&decoded)&&WorldClear());
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 shooter=ShooterFind(fixture->kind);assert(shooter&&shooterShot.calls==1&&shooter->nextthink-level.time==remaining);
 assert(objects[shooter->s.number].owner==MAX_CLIENTS+1&&objects[shooter->s.number].output[0]);
 ShooterStateCheck(shooter,flags,500,0,1,0);
 target=&g_entities[objects[shooter->s.number].output[0]-1];
 ControllerTick(shooter,remaining-1);assert(shooterShot.calls==1);
 objects[target->s.number].copyPending=1;
 ControllerTick(shooter,1);assert(shooterShot.calls==1&&shooter->nextthink-level.time==500);
 objects[target->s.number].copyPending=0;
 Move(target,(vec3_t){440,180,100});
 ControllerTick(shooter,500);assert(shooterShot.calls==2);
 ShooterShotCheck(fixture,shooter,qtrue,target);

 assert(WorldClear()&&PlaceCopy(owner,&copies[0],(vec3_t){950,100,80},NULL,NULL));
 shooter=ShooterFind(fixture->kind);assert(shooter&&shooterShot.calls==2&&shooter->nextthink-level.time==remaining);
 assert(shooter==&g_entities[players[0].selected]&&objects[shooter->s.number].owner==1);
 target=&g_entities[objects[shooter->s.number].output[0]-1];
 ControllerTick(shooter,remaining);assert(shooterShot.calls==3);
 ShooterShotCheck(fixture,shooter,qtrue,target);
 Undo(owner,qfalse);assert(!shooter->inuse&&!target->inuse&&LiveCount()==0);
}

static void ShooterDamagePolicy(const shooterFixture_t *fixture){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;shooterShot.calls=0;
 gentity_t *owner=&g_entities[0],*other=&g_entities[1],*shooter=ShooterSpawn(fixture);
 const int n=shooter->s.number;
 unsigned epoch;
 assert(JAR_MP_ShooterSource(shooter,&epoch)&&epoch==JAR_MP_EntityEpoch(n));
 static gentity_t missile;
 memset(&missile,0,sizeof(missile));
 assert(!JAR_MP_ShooterDamageDenied(&missile,other));
 missile.jarShooterSource=n+1;missile.jarShooterEpoch=epoch;
 assert(!JAR_MP_ShooterDamageDenied(&missile,owner)&&!JAR_MP_ShooterDamageDenied(&missile,other));
 assert(JAR_MP_ShooterDamageDenied(&missile,shooter));
 playerTools.integer=0;assert(JAR_MP_ShooterDamageDenied(&missile,other));playerTools.integer=1;
 players[1].noTools=1;assert(JAR_MP_ShooterDamageDenied(&missile,other));players[1].noTools=0;
 players[1].noGrabUntil=level.time+1000;assert(JAR_MP_ShooterDamageDenied(&missile,other));players[1].noGrabUntil=0;
 objects[n].copyPending=1;assert(!JAR_MP_ShooterSource(shooter,NULL)&&JAR_MP_ShooterDamageDenied(&missile,other));objects[n].copyPending=0;
 owner->client->pers.connected=CON_DISCONNECTED;
 assert(!JAR_MP_ShooterSource(shooter,NULL)&&JAR_MP_ShooterDamageDenied(&missile,other));
 owner->client->pers.connected=CON_CONNECTED;

 gentity_t *prop=CopySpawn(1,1,500);assert(prop);
 assert(JAR_MP_ShooterDamageDenied(&missile,prop));
 sharedEditing.integer=1;assert(!JAR_MP_ShooterDamageDenied(&missile,prop));
 objects[prop->s.number].copyPending=1;assert(JAR_MP_ShooterDamageDenied(&missile,prop));
 objects[prop->s.number].copyPending=0;sharedEditing.integer=0;
 G_FreeEntity(shooter);
 assert(!JAR_MP_ShooterSource(shooter,NULL)&&JAR_MP_ShooterDamageDenied(&missile,other));
 shooter=ControllerAt(fixture->name,0,100);
 assert(shooter&&shooter->s.number==n&&JAR_MP_ShooterSource(shooter,NULL)&&JAR_MP_EntityEpoch(n)!=epoch);
 assert(JAR_MP_ShooterDamageDenied(&missile,other));
 assert(WorldClear()&&LiveCount()==0);

 shooterMissingAssets=1;
 assert(!SpawnWithController(owner,fixture->id,(vec3_t){100,0,80},vec3_origin)&&LiveCount()==0);
 shooterMissingAssets=0;
}

static void ShooterCases(void){
 ShooterDefaults();ShooterRepeat();
 for(int i=0;i<ARRAY_LEN(shooterFixtures);++i){
  ShooterPersistence(&shooterFixtures[i]);ShooterDamagePolicy(&shooterFixtures[i]);
 }
 PhysicsReset();
 puts("MP shooters: stock weapon mapping, live Primary aim, Alt and repeat timing, Disable, copy/world state, projectile permissions and source slot reuse passed.");
}
