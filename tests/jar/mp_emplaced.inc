// The engine's native gun callbacks live in g_weapon.c. These fixture imports
// model only their reciprocal seat fields and spawn defaults.
void JAR_MP_NativeEmplacedInit(gentity_t *gun){
 gun->health=gun->maxHealth=800;
 gun->s.generic1=250;gun->s.bolt2=1;gun->s.speed=60;
 gun->s.weapon=WP_EMPLACED_GUN;gun->s.emplacedOwner=ENTITYNUM_NONE;
 gun->s.modelGhoul2=1;gun->s.g2radius=110;
 gun->takedamage=qtrue;gun->r.svFlags|=SVF_PLAYER_USABLE;
}

qboolean JAR_MP_NativeEWebInit(gentity_t *gun){
 gun->health=gun->maxHealth=250;
 gun->s.generic1=250;gun->s.bolt2=JAR_MP_EWEB_TAG;gun->s.speed=180;
 gun->s.weapon=WP_NONE;gun->s.emplacedOwner=ENTITYNUM_NONE;
 gun->s.modelGhoul2=1;gun->s.g2radius=128;
 gun->count=gun->s.otherEntityNum2=JAR_MP_EWEB_DEFAULT_AMMO;
 gun->takedamage=qtrue;gun->r.svFlags|=SVF_PLAYER_USABLE;
 return qtrue;
}

qboolean JAR_MP_NativeEmplacedUse(gentity_t *gun,gentity_t *user){
 if(gun->activator||gun->health<=0||user->client->ps.emplacedIndex)return qfalse;
 gun->activator=user;gun->s.emplacedOwner=user->s.number;gun->s.activeForcePass=NUM_FORCE_POWERS+1;
 if(JAR_MP_EWebState(&gun->s))gun->genericValue11=user->client->ps.weapon;
 else gun->s.weapon=user->client->ps.weapon;
 user->client->ps.emplacedIndex=gun->s.number;user->r.ownerNum=gun->s.number;
 user->client->ps.weapon=WP_EMPLACED_GUN;
 return qtrue;
}

qboolean JAR_MP_NativeEmplacedOccupied(const gentity_t *gun){
 if(!gun||!gun->inuse||!JAR_MP_EmplacedState(&gun->s)||!gun->activator)return qfalse;
 const gentity_t *user=gun->activator;
 return user->client&&gun->s.emplacedOwner==user->s.number&&gun->s.activeForcePass==NUM_FORCE_POWERS+1&&
        user->client->ps.emplacedIndex==gun->s.number&&user->r.ownerNum==gun->s.number;
}

void JAR_MP_NativeEmplacedDetach(gentity_t *gun){
 if(!JAR_MP_NativeEmplacedOccupied(gun))return;
 gentity_t *user=gun->activator;
 const qboolean eweb=JAR_MP_EWebState(&gun->s);
 user->client->ps.emplacedIndex=0;user->client->ps.weapon=eweb?gun->genericValue11:gun->s.weapon;
 user->r.ownerNum=ENTITYNUM_NONE;
 gun->activator=NULL;gun->s.emplacedOwner=ENTITYNUM_NONE;gun->s.activeForcePass=0;
 gun->s.weapon=eweb?WP_NONE:WP_EMPLACED_GUN;
}

void JAR_MP_NativeEmplacedDetachUser(gentity_t *user){
 if(!user->client)return;
 const int slot=user->client->ps.emplacedIndex;
 if(slot>=MAX_CLIENTS&&slot<ENTITYNUM_WORLD&&g_entities[slot].activator==user)
  JAR_MP_NativeEmplacedDetach(&g_entities[slot]);
}

static void EWebCases(void){
 static jarWorld_t captured,decoded;
 static byte bytes[WORLD_BYTES];
 RigidFixture();
 imports.JAR_ModelPoseBounds=SkeletalBounds;imports.SendServerCommand=ControllerPropertySend;
 gentity_t *owner=&g_entities[0];
 const unsigned id=96166652u;
 int catalog;
 const jarMPEntry_t *entry=Entry(id,&catalog);
 assert(entry&&entry->kind==5&&!strcmp(entry->name,"emplaced_eweb")&&
        !strcmp(entry->model,"models/map_objects/hoth/eweb_model.glm"));
 VectorSet(owner->client->ps.origin,0,0,80);owner->client->ps.viewangles[YAW]=45;
 char key[16];snprintf(key,sizeof(key),"%u",id);
 aimed=ENTITYNUM_WORLD;Command(0,"jar_spawn",key);
 const int slot=Last(0);
 assert(slot>=MAX_CLIENTS&&ControllerKind(slot)==JAR_EWEB_CONTROLLER);
 gentity_t *gun=&g_entities[slot];
 assert(gun->r.linked&&objects[slot].frozen&&gun->pos1[YAW]==45&&JAR_MP_EWebState(&gun->s));
 assert(!JAR_MP_ChairState(&gun->s)&&gun->s.weapon==WP_NONE&&gun->health==250&&gun->maxHealth==250);
 assert(gun->count==999&&gun->s.otherEntityNum2==999&&JAR_MP_EWebAmmo(&gun->s)==999);
 assert(JAR_MP_EmplacedConstraint(&gun->s)==180&&VectorCompare(gun->s.origin2,gun->r.mins));
 assert(!SpawnAtPose(owner,id,(float[3]){700,0,120},vec3_origin,1));

 ControllerSelect(gun);
 assert(strstr(controllerEcho," 38 0 0 0 0 250 250 0 - 180 1 0 999"));
 Command(0,"jar_property","controller_ammo_123");
 Command(0,"jar_property","controller_ammo_1500");
 assert(gun->count==1500&&JAR_MP_EWebAmmo(&gun->s)==1500);
 Command(0,"jar_property","property_undo");
 Command(0,"jar_property","controller_ammo_16384");
 Command(0,"jar_property","controller_yaw_90");
 assert(gun->count==123&&gun->s.otherEntityNum2==123&&JAR_MP_EWebAmmo(&gun->s)==123);
 assert(JAR_MP_EmplacedConstraint(&gun->s)==90&&gun->s.origin2[0]!=90);

 owner->client->ps.weapon=WP_BLASTER;
 owner->client->ps.stats[STAT_HOLDABLE_ITEMS]=1<<HI_EWEB;
 objects[slot].heldBy=1;gun->use(gun,owner,owner);assert(!JAR_MP_NativeEmplacedOccupied(gun));
 objects[slot].heldBy=0;gun->use(gun,owner,owner);
 assert(JAR_MP_NativeEmplacedOccupied(gun)&&gun->s.weapon==WP_NONE&&owner->client->ps.weapon==WP_EMPLACED_GUN);
 assert(!WorldCapture(&captured)&&!WorldClear());
 CopyAssembly(owner,slot);assert(!copies[0].count);
 Command(0,"jar_property","controller_enabled");
 assert(!JAR_MP_NativeEmplacedOccupied(gun)&&!owner->client->ps.emplacedIndex);
 assert(owner->client->ps.weapon==WP_BLASTER&&owner->client->ps.stats[STAT_HOLDABLE_ITEMS]==(1<<HI_EWEB));
 Command(0,"jar_property","controller_enabled");

 gun->health=0;gun->genericValue4=1;gun->s.time=level.time+50;EmplacedEnabled(gun);
 CopyAssembly(owner,slot);
 assert(copies[0].count==1&&copies[0].node[0].controllerRemaining==50);
 assert(WorldCapture(&captured));
 const int size=WorldEncode(&captured,bytes);
 assert(size==WORLD_HEADER+WORLD_NODE+4&&WorldDecode(bytes,size,&decoded));
 jarCopyNode_t *node=&decoded.copy.node[0];
 assert(node->id==id&&node->controller.active==2&&node->controllerRemaining==50);
 assert(node->controller.pushSpeed==123&&node->controller.speed==90);
 node->controllerRemaining=51;assert(!CopyControllersValid(&decoded.copy));node->controllerRemaining=50;
 assert(WorldClear());level.time+=200;
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 gun=&g_entities[slot];
 assert(gun->inuse&&JAR_MP_EWebState(&gun->s)&&JAR_MP_EWebAmmo(&gun->s)==123);
 assert(gun->genericValue4==1&&gun->s.time-level.time==50&&!gun->takedamage);
 assert(!gun->activator&&gun->s.weapon==WP_NONE);
 assert(WorldClear()&&PlaceCopy(owner,&copies[0],(float[3]){1000,0,120},NULL,NULL));
 gun=&g_entities[players[0].selected];
 assert(JAR_MP_EWebState(&gun->s)&&gun->genericValue4==1&&gun->s.time-level.time==50);
 assert(JAR_MP_EWebAmmo(&gun->s)==123&&JAR_MP_EmplacedConstraint(&gun->s)==90);

 gun->genericValue4=2;gun->s.time=-1;EmplacedEnabled(gun);
 ControllerSelect(gun);Command(0,"jar_property","controller_repair");
 assert(gun->health==250&&gun->takedamage&&!gun->genericValue4&&!gun->s.time);
 assert(gun->r.contents&CONTENTS_SOLID);
 assert(JAR_MP_EWebAmmo(&gun->s)==123&&gun->s.weapon==WP_NONE);
 assert(WorldClear());
 PhysicsReset();imports.JAR_ModelPoseBounds=NULL;
 puts("MP persistent E-Web: catalog, ammo and yaw Context, seat protections, pending JRW18 and clipboard restore, and wreck repair passed.");
}

static void EmplacedCases(void){
 static jarWorld_t captured,decoded;
 static byte bytes[WORLD_BYTES];
 RigidFixture();
 imports.JAR_ModelPoseBounds=SkeletalBounds;imports.SendServerCommand=ControllerPropertySend;
 gentity_t *owner=&g_entities[0],*visitor=&g_entities[1];
 const unsigned id=93840071u;
 int catalog;
 const jarMPEntry_t *entry=Entry(id,&catalog);
 assert(entry&&entry->kind==5&&!strcmp(entry->name,"emplaced_gun")&&
        !strcmp(entry->model,"models/map_objects/mp/turret_chair.glm"));
 char key[16];snprintf(key,sizeof(key),"%u",id);
 VectorSet(owner->client->ps.origin,0,0,80);owner->client->ps.viewangles[YAW]=45;
 aimed=ENTITYNUM_WORLD;Command(0,"jar_spawn",key);
 const int slot=Last(0);
 assert(slot>=MAX_CLIENTS&&ControllerKind(slot)==JAR_EMPLACED_CONTROLLER);
 gentity_t *gun=&g_entities[slot];
 assert(PhysicsProp(slot)&&objects[slot].frozen&&gun->r.linked&&gun->s.modelGhoul2&&gun->s.g2radius==110);
 assert(!strcmp(gun->model,entry->model)&&gun->r.currentAngles[YAW]==45&&gun->pos1[YAW]==45);
 assert(gun->health==800&&gun->maxHealth==800&&gun->takedamage&&(gun->r.svFlags&SVF_PLAYER_USABLE));
 assert(JAR_MP_EmplacedState(&gun->s)&&JAR_MP_EmplacedConstraint(&gun->s)==60);

 ControllerSelect(gun);
 assert(ContextCaps(owner,slot)&JAR_CONTEXT_CONTROLLER);
 assert(strstr(controllerEcho," 37 0 0 0 0 800 800 0 - 60 1 0 0"));
 Command(0,"jar_property","controller_yaw_90");
 Command(0,"jar_property","controller_count_900");
 Command(0,"jar_property","transform_400_50_120_0_90_0");
 assert(VectorCompare(gun->s.origin,((float[3]){400,50,120}))&&VectorCompare(gun->s.origin,gun->r.currentOrigin));
 assert(gun->s.angles[YAW]==90&&gun->pos1[YAW]==90&&gun->s.apos.trBase[YAW]==90);
 assert(VectorCompare(gun->s.origin2,gun->r.mins)&&VectorCompare(gun->s.angles2,gun->r.maxs));
 assert(gun->s.origin2[0]!=90&&JAR_MP_EmplacedConstraint(&gun->s)==90);
 assert(gun->health==900&&gun->maxHealth==900);

 CopyAssembly(owner,slot);
 assert(copies[0].count==1&&copies[0].node[0].id==id&&copies[0].node[0].controller.speed==90);
 const jarCopyNode_t copied=copies[0].node[0];
 owner->client->ps.weapon=WP_BLASTER;
 VectorSet(owner->client->ps.origin,400,10,120);
 VectorSet(owner->client->ps.viewangles,0,90,0);
 assert(!JAR_MP_EmplacedAllowsSeat(gun,visitor));
 gun->use(gun,visitor,owner);
 assert(!JAR_MP_NativeEmplacedOccupied(gun));
 aimed=ENTITYNUM_WORLD;Command(0,"jar_property","activate");
 assert(!JAR_MP_NativeEmplacedOccupied(gun)&&!owner->client->ps.emplacedIndex);
 aimed=slot;Command(0,"jar_property","activate");
 assert(JAR_MP_NativeEmplacedOccupied(gun)&&JAR_MP_EmplacedCanFire(gun));
 assert(owner->client->ps.emplacedIndex==slot&&gun->s.emplacedOwner==0);
 assert(!(ContextCaps(owner,slot)&(JAR_CONTEXT_FREEZE|JAR_CONTEXT_TRANSFORM|JAR_CONTEXT_REMOVE)));
 CopyAssembly(owner,slot);
 assert(copies[0].count==1&&!memcmp(&copied,&copies[0].node[0],sizeof(copied)));
 assert(!WorldCapture(&captured)&&!WorldClear());
 jarAutoCapture_t automatic={0};
 assert(WorldCaptureMode(&captured,&automatic)&&automatic.parked==1&&captured.copy.count==1);
 assert(captured.copy.node[0].frozen&&JAR_MP_NativeEmplacedOccupied(gun));
 Undo(owner,qfalse);Undo(owner,qtrue);
 assert(gun->inuse&&JAR_MP_NativeEmplacedOccupied(gun));
 Command(0,"jar_property","controller_enabled");
 assert(!gun->activator&&gun->s.emplacedOwner==ENTITYNUM_NONE&&!gun->s.activeForcePass);
 assert(!owner->client->ps.emplacedIndex&&owner->client->ps.weapon==WP_BLASTER);
 assert(!gun->takedamage&&!(gun->r.svFlags&SVF_PLAYER_USABLE)&&!JAR_MP_EmplacedCanFire(gun));
 Command(0,"jar_property","controller_enabled");
 assert(gun->takedamage&&(gun->r.svFlags&SVF_PLAYER_USABLE));

 // Native death has already scheduled its warning. Exercise the Sandbox's
 // pending-time capture, validation and restoration around that native phase.
 gun->health=0;gun->genericValue4=1;gun->s.time=level.time+1375;EmplacedEnabled(gun);
 assert(WorldCapture(&captured));
 const int size=WorldEncode(&captured,bytes);
 assert(size==WORLD_HEADER+WORLD_NODE+4&&WorldDecode(bytes,size,&decoded));
 jarCopyNode_t *node=&decoded.copy.node[0];
 assert(node->id==id&&node->controller.active==2&&node->controller.count==0&&node->controller.initial==900);
 assert(node->controllerRemaining==1375&&node->controller.speed==90);
 node->controllerRemaining=3001;assert(!CopyControllersValid(&decoded.copy));node->controllerRemaining=1375;
 assert(WorldClear());level.time+=200;
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 gun=&g_entities[slot];
 assert(gun->inuse&&objects[slot].owner==MAX_CLIENTS+1&&JAR_MP_EmplacedState(&gun->s));
 assert(gun->genericValue4==1&&gun->s.time-level.time==1375&&!gun->takedamage);
 assert(!gun->activator&&gun->s.emplacedOwner==ENTITYNUM_NONE&&!gun->s.activeForcePass&&gun->s.weapon==WP_EMPLACED_GUN);
 assert(gun->pos1[YAW]==90&&VectorCompare(gun->s.origin2,gun->r.mins)&&JAR_MP_EmplacedConstraint(&gun->s)==90);

 // A wreck retains its collision. Context Repair restores its own max HP.
 gun->genericValue4=2;gun->s.time=-1;EmplacedEnabled(gun);
 worldEditing.integer=1;ControllerSelect(gun);Command(0,"jar_property","controller_repair");
 assert(!gun->genericValue4&&!gun->s.time&&gun->health==900&&gun->takedamage&&(gun->r.svFlags&SVF_PLAYER_USABLE));
 assert(gun->r.contents&CONTENTS_SOLID);
 assert(WorldClear());
 PhysicsReset();imports.JAR_ModelPoseBounds=NULL;
 puts("MP emplaced gun: catalog and wire hull, typed Context, occupied protections, parked autosave, pending JRW18 and wreck repair passed.");
 EWebCases();
}
