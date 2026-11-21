// Real native trigger callbacks, oriented sensor adapter, commands and save codec.
// Broad-phase linking, damage delivery and native actor animation are imports.
static void TriggerTick(gentity_t*e,int ms){level.time+=ms;if(e->nextthink>0&&e->nextthink<=level.time){e->nextthink=0;if(e->think)e->think(e);}TriggerSync(e->s.number);}
static void TriggerEnter(gentity_t*e,gentity_t*a){gentity_t*h=TriggerHelper(e->s.number);assert(h&&h->touch==TriggerTouch);h->touch(h,a,NULL);}
static void TriggerPlaceActor(gentity_t*a,gentity_t*e){G_SetOrigin(a,e->r.currentOrigin);VectorCopy(e->r.currentOrigin,a->client->ps.origin);VectorSet(a->r.mins,-8,-8,-8);VectorSet(a->r.maxs,8,8,8);}
static void TriggerControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=hurtCalls=0;gentity_t*p=&g_entities[0];p->takedamage=qtrue;
 gentity_t*repeat=SpawnWithController(p,ControllerId("trigger_multiple"),(float[3]){100,0,80},vec3_origin);assert(repeat);
 int n=repeat->s.number;gentity_t*h=TriggerHelper(n),*print=&g_entities[objects[n].output[0]-1];
 assert(h&&h->r.linked&&(h->r.svFlags&SVF_NOCLIENT)&&h->r.contents==CONTENTS_TRIGGER&&!objects[h->s.number].owner&&repeat->r.contents==CONTENTS_SOLID&&!repeat->touch&&repeat->use==Use_Multi&&PhysicsProp(n));
 assert(objects[n].order==objects[print->s.number].order&&!repeat->target2&&repeat->speed==1000);
 TriggerPlaceActor(p,repeat);TriggerEnter(repeat,p);assert(controllerPrints==1&&repeat->nextthink==level.time+1000&&!repeat->think);
 TriggerEnter(repeat,p);assert(controllerPrints==1);TriggerTick(repeat,999);TriggerEnter(repeat,p);assert(controllerPrints==1);TriggerTick(repeat,1);TriggerEnter(repeat,p);assert(controllerPrints==2);
 // Manual disarm/rearm Undo keeps an existing cooldown instead of restarting it.
 ControllerSelect(repeat);int deadline=repeat->nextthink;Command(0,"jar_property","controller_playing");assert(!h->r.linked&&!repeat->use&&repeat->nextthink==deadline);
 Command(0,"jar_property","property_undo");assert(h->r.linked&&repeat->use==Use_Multi&&repeat->nextthink==deadline);
 Command(0,"jar_property","controller_extent_128");assert(objects[n].triggerExtent==128&&h->r.maxs[0]==128);Command(0,"jar_property","property_undo");assert(objects[n].triggerExtent==64);
 const char*bad[]={"controller_extent_0","controller_extent_65537","controller_extent_nan","controller_delay_0","controller_random_1001",NULL};for(int i=0;bad[i];++i)Command(0,"jar_property",bad[i]);assert(objects[n].triggerExtent==64&&repeat->wait==1&&!repeat->random);
 Command(0,"jar_property","controller_delay_1500");Command(0,"jar_property","controller_random_250");assert(fabsf(repeat->wait-1.5f)<.0001f&&fabsf(repeat->random-.25f)<.0001f);Command(0,"jar_property","controller_random_0");Command(0,"jar_property","controller_delay_1000");
 // Native facing and USE/FIRE filtering operate on the touch actor, not owner.
 TriggerTick(repeat,1000);Command(0,"jar_property","controller_facing");VectorSet(p->client->ps.viewangles,0,180,0);TriggerEnter(repeat,p);assert(controllerPrints==2);
 VectorClear(p->client->ps.viewangles);Command(0,"jar_property","controller_input");TriggerEnter(repeat,p);assert(controllerPrints==2);p->client->pers.cmd.buttons=BUTTON_USE;TriggerEnter(repeat,p);assert(controllerPrints==3);
 TriggerTick(repeat,1000);p->client->ps.weaponTime=0;Command(0,"jar_property","controller_input");TriggerEnter(repeat,p);assert(controllerPrints==3);p->client->pers.cmd.buttons=BUTTON_ALT_ATTACK;TriggerEnter(repeat,p);assert(controllerPrints==4);
 TriggerTick(repeat,1000);Command(0,"jar_property","controller_input");TriggerEnter(repeat,p);assert(controllerPrints==4);p->client->pers.cmd.buttons=BUTTON_USE|BUTTON_ATTACK;TriggerEnter(repeat,p);assert(controllerPrints==5);
 TriggerTick(repeat,1000);p->client->ps.weaponTime=0;Command(0,"jar_property","controller_input");Command(0,"jar_property","controller_facing");p->client->pers.cmd.buttons=0;
 gentity_t*npc=ActorNPC(0,0);gNPC_t npcData={0};npc->NPC=&npcData;TriggerPlaceActor(npc,repeat);
 Command(0,"jar_property","controller_subjects");TriggerEnter(repeat,npc);assert(controllerPrints==5);Command(0,"jar_property","controller_subjects");TriggerEnter(repeat,p);assert(controllerPrints==5);TriggerEnter(repeat,npc);assert(controllerPrints==6);
 TriggerTick(repeat,1000);Command(0,"jar_property","controller_subjects");players[0].noTools=1;TriggerEnter(repeat,p);assert(controllerPrints==6);players[0].noTools=0;
 Command(0,"jar_property","controller_enabled");assert(!h->r.linked);TriggerEnter(repeat,p);assert(controllerPrints==6);Command(0,"jar_property","property_undo");assert(h->r.linked);
 // Rotated broad-phase corners are rejected; translated boxes stay centered.
 assert(PhysicsSetAngles(repeat,(float[3]){0,45,0}));TriggerFrame();assert(fabsf(h->r.maxs[0]-64*sqrtf(2))<.01f);
 VectorSet(p->client->ps.origin,repeat->r.currentOrigin[0]+80,80,80);assert(!TriggerContact(repeat,p));TriggerEnter(repeat,p);assert(controllerPrints==6);
 VectorSet(p->client->ps.origin,repeat->r.currentOrigin[0]+75,0,80);assert(TriggerContact(repeat,p));TriggerEnter(repeat,p);assert(controllerPrints==7);
 Move(repeat,(float[3]){500,200,80});TriggerFrame();assert(VectorCompare(h->r.currentOrigin,repeat->r.currentOrigin));TriggerPlaceActor(p,repeat);assert(TriggerContact(repeat,p));
 // Secondary fires one second after the volume clears, then native cooldown.
 gentity_t*secondary=ControllerAt("target_print",0,800);players[0].linkOutput=1;ControllerLink(p,n,qfalse);ControllerLink(p,secondary->s.number,qfalse);players[0].linkOutput=0;
 TriggerTick(repeat,1000);TriggerEnter(repeat,p);assert(controllerPrints==8&&repeat->think==trigger_cleared_fire);
 TriggerTick(repeat,500);TriggerEnter(repeat,p);assert(repeat->nextthink==level.time+1000);TriggerTick(repeat,999);assert(controllerPrints==8);TriggerTick(repeat,1);assert(controllerPrints==9&&!repeat->think&&repeat->nextthink==level.time+1000);
 // Rejected native Use must not replace the delayed/clear activator.
 TriggerTick(repeat,1000);secondary->use=ControllerUseSpy;TriggerPlaceActor(npc,repeat);TriggerEnter(repeat,npc);sharedEditing.integer=1;ControllerShot(&g_entities[1],n,qfalse);sharedEditing.integer=0;assert(ControllerActivator(repeat)==npc);
 int slot=npc->s.number;G_FreeEntity(npc);npc=ActorNPC(0,0);assert(npc->s.number==slot);TriggerTick(repeat,1000);assert(controllerActivator==n);G_FreeEntity(npc);
 // One-shot bodies retain tools/selection and can be deliberately rearmed.
 gentity_t*once=SpawnWithController(p,ControllerId("trigger_once"),(float[3]){1100,0,80},vec3_origin);assert(once);TriggerPlaceActor(p,once);int printed=controllerPrints;TriggerEnter(once,p);assert(controllerPrints==printed+1&&!once->use&&!TriggerHelper(once->s.number)->r.linked);
 TriggerEnter(once,p);assert(controllerPrints==printed+1);ControllerSelect(once);Command(0,"jar_property","controller_playing");assert(once->use);Command(0,"jar_property","property_undo");assert(!once->use);Command(0,"jar_property","controller_playing");++level.time;TriggerEnter(once,p);assert(controllerPrints==printed+2&&!once->use);
 // Native Hurt rate, current consent and activation attribution.
 gentity_t*hurt=ControllerAt("trigger_hurt",0,1400);assert(hurt&&hurt->damage==5);TriggerPlaceActor(p,hurt);TriggerEnter(hurt,p);assert(hurtCalls==1&&hurtAmount==5&&hurtAttacker==hurt->s.number);
 TriggerEnter(hurt,p);assert(hurtCalls==1);TriggerTick(hurt,FRAMETIME);TriggerEnter(hurt,p);assert(hurtCalls==2);
 ControllerSelect(hurt);Command(0,"jar_property","controller_slow");Command(0,"jar_property","controller_damage_20");TriggerTick(hurt,FRAMETIME);TriggerEnter(hurt,p);assert(hurtCalls==3&&hurtAmount==20&&hurt->timestamp==level.time+1000);
 Command(0,"jar_property","controller_damage_32768");assert(hurt->damage==20);ControllerShot(p,hurt->s.number,qfalse);assert(!TriggerHelper(hurt->s.number)->r.linked);TriggerTick(hurt,1000);TriggerEnter(hurt,p);assert(hurtCalls==3);ControllerShot(p,hurt->s.number,qfalse);TriggerEnter(hurt,p);assert(hurtCalls==4&&hurtAttacker==0);
 TriggerTick(hurt,1000);players[0].noTools=1;TriggerEnter(hurt,p);assert(hurtCalls==4);players[0].noTools=0;playerTools.integer=0;TriggerPlaceActor(&g_entities[1],hurt);g_entities[1].takedamage=qtrue;TriggerEnter(hurt,&g_entities[1]);assert(hurtCalls==4);playerTools.integer=1;
 Command(0,"jar_property","controller_damage_0");TriggerEnter(hurt,p);assert(hurtCalls==4);Command(0,"jar_property","property_undo");TriggerEnter(hurt,p);assert(hurtCalls==5);
 // Spent once, hurt cooldown, repeated-clear and remaining cooldown round-trip.
 TriggerPlaceActor(p,repeat);TriggerTick(repeat,1000);TriggerEnter(repeat,p);assert(repeat->think==trigger_cleared_fire);ControllerSelect(repeat);Command(0,"jar_property","controller_playing");assert(!repeat->use&&repeat->think==trigger_cleared_fire);
 gentity_t*cool=ControllerAt("trigger_multiple",0,1700);TriggerPlaceActor(p,cool);TriggerEnter(cool,p);assert(!cool->think&&cool->nextthink>level.time);
 hurt->timestamp=level.time+750;CopyAssembly(p,n);assert(CopyControllersValid(&copies[0]));static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 int sensors=0;for(int i=0;i<decoded.copy.count;++i){int index;const jarMPEntry_t*entry=Entry(decoded.copy.node[i].id,&index);if(entry->kind==5&&JAR_ControllerTrigger(ControllerClass(entry->name)))++sensors;}assert(sensors==4);
 // Refuse impossible states before allocating helpers or touching the world.
 static jarCopy_t invalid;invalid=decoded.copy;for(int i=0;i<invalid.count;++i){int index;const jarMPEntry_t*entry=Entry(invalid.node[i].id,&index);if(entry->kind==5&&ControllerClass(entry->name)==20){invalid.node[i].controller.initial=invalid.node[i].controllerInitial=1;invalid.node[i].controllerRemaining=1;break;}}assert(!CopyControllersValid(&invalid));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));int helpers=0,spent=0,pending=0,cooldown=0,hurtTimer=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i){gentity_t*e=&g_entities[i];int kind=ControllerKind(i);if(JAR_ControllerTrigger(kind)){++helpers;assert(TriggerHelper(i)&&VectorCompare(TriggerHelper(i)->r.currentOrigin,e->r.currentOrigin));if(kind==20){++spent;assert(!e->use&&!TriggerHelper(i)->r.linked);}if(kind==19&&e->think){++pending;assert(!e->use&&e->think==trigger_cleared_fire&&!TriggerHelper(i)->r.linked);}if(kind==19&&!e->think){++cooldown;assert(e->nextthink>level.time);}if(kind==21){++hurtTimer;assert(e->timestamp==level.time+750);}}}
 assert(helpers==4&&spent==1&&pending==1&&cooldown==1&&hurtTimer==1);assert(WorldClear());
 assert(PlaceCopy(p,&copies[0],(float[3]){2000,0,80},NULL,NULL));int pasted=players[0].selected;assert(TriggerHelper(pasted));Undo(p,qfalse);assert(!g_entities[pasted].inuse);for(int i=MAX_CLIENTS;i<level.num_entities;++i)assert(!triggerParent[i]);
 // A removed sensor is never mistaken for a replacement entity in its slot.
 repeat=ControllerAt("trigger_multiple",0,100);n=repeat->s.number;h=TriggerHelper(n);slot=h->s.number;G_FreeEntity(h);gentity_t*replacement=G_Spawn();assert(replacement->s.number==slot&&!TriggerHelper(n));TriggerFrame();assert(!objects[n].triggerActive&&!replacement->touch);G_FreeEntity(repeat);assert(replacement->inuse);G_FreeEntity(replacement);
 // Preflight reserves space for both bodies and derived trigger helpers.
 int occupied=0;for(int i=MAX_CLIENTS;i<ENTITYNUM_WORLD-JAR_MP_ENTITY_RESERVE-1;++i)if(!g_entities[i].inuse){g_entities[i].inuse=qtrue;++occupied;}int live=LiveCount();assert(!PlaceCopy(p,&copies[0],vec3_origin,NULL,NULL)&&LiveCount()==live);assert(occupied);PhysicsReset();
 puts("MP triggers: native Repeat/Once/Hurt callbacks, filters, oriented contact, permissions, Undo deadlines, actor lifetime, JRW15/copy and helper cleanup passed.");
}
static void SpaceControllerCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;spaceDamage=spaceSounds=0;
 gentity_t*p=&g_entities[0],*other=&g_entities[1];p->takedamage=other->takedamage=qtrue;
 const vec3_t position={200,100,80},angles={0,45,0};
 gentity_t*space=SpawnWithController(p,87701002u,position,angles);assert(space);
 const int n=space->s.number;
 gentity_t*sensor=TriggerHelper(n);assert(sensor&&sensor->r.linked&&space->r.contents==CONTENTS_ITEM);
 assert(space->s.bolt2==JAR_MP_LOGIC_MARKER_TAG&&ControllerKind(n)==JAR_SPACE_CONTROLLER&&!objects[n].output[0]);
 assert(sensor->r.mins[2]==0&&sensor->r.maxs[2]==128);
 TriggerPlaceActor(p,space);p->client->ps.origin[2]-=1;
 TriggerEnter(space,p);assert(JAR_MP_SpaceGravity(p)==-1);
 TriggerPlaceActor(p,space);p->client->ps.origin[0]+=80;p->client->ps.origin[1]+=80;
 TriggerEnter(space,p);assert(JAR_MP_SpaceGravity(p)==-1); // enclosing AABB corner
 TriggerPlaceActor(p,space);TriggerEnter(space,p);assert(JAR_MP_SpaceGravity(p)==0);
 const int deadline=p->client->inSpaceSuffocation;assert(deadline==level.time+5000);
 level.time=deadline-1;JAR_MP_SpaceFrame(p);assert(!spaceDamage&&!spaceSounds);
 TriggerEnter(space,p);assert(p->client->inSpaceSuffocation==deadline);
 level.time=deadline;JAR_MP_SpaceFrame(p);assert(spaceDamage>=20&&spaceDamage<=40&&spaceSounds==1);
 assert(p->client->ps.forceHandExtend==HANDEXTEND_CHOKE);
 const int inherited=p->client->inSpaceSuffocation;
 gentity_t*overlap=SpawnWithController(p,87701002u,position,vec3_origin);assert(overlap);
 TriggerEnter(overlap,p);assert(p->client->inSpaceIndex==TriggerHelper(overlap->s.number)->s.number);
 Move(overlap,(float[3]){position[0]+200,position[1],position[2]});TriggerSync(overlap->s.number);
 assert(JAR_MP_SpaceGravity(p)==0&&p->client->inSpaceIndex==sensor->s.number&&p->client->inSpaceSuffocation==inherited);
 Move(overlap,position);TriggerSync(overlap->s.number);TriggerEnter(overlap,p);
 Undo(p,qfalse);
 assert(!overlap->inuse&&JAR_MP_SpaceGravity(p)==0&&p->client->inSpaceIndex==sensor->s.number&&p->client->inSpaceSuffocation==inherited);
 TriggerPlaceActor(other,space);TriggerEnter(space,other);assert(JAR_MP_SpaceGravity(other)==0);
 players[1].noTools=1;assert(JAR_MP_SpaceGravity(other)==-1&&!other->client->inSpaceSuffocation);
 players[1].noTools=0;
 gentity_t*droid=CopySpawn(2,0,500);assert(droid);CopyReady(droid);
 droid->client->NPC_class=CLASS_R2D2;droid->takedamage=qtrue;
 TriggerPlaceActor(droid,space);TriggerEnter(space,droid);assert(JAR_MP_SpaceGravity(droid)==0);
 level.time+=5000;JAR_MP_SpaceFrame(droid);assert(spaceSounds==1);
 gentity_t*vehicle=CopySpawn(3,0,550);assert(vehicle);CopyReady(vehicle);
 vehicleInfo_t enclosed=*vehicle->m_pVehicle->m_pVehicleInfo;enclosed.hideRider=qtrue;
 vehicle->m_pVehicle->m_pVehicleInfo=&enclosed;other->client->ps.m_iVehicleNum=vehicle->s.number;
 TriggerEnter(space,other);assert(JAR_MP_SpaceGravity(other)==-1);
 enclosed.hideRider=qfalse;TriggerEnter(space,other);assert(JAR_MP_SpaceGravity(other)==0);
 other->client->ps.m_iVehicleNum=0;
 ControllerSelect(space);
 Command(0,"jar_property","controller_space_width_192");
 Command(0,"jar_property","controller_space_depth_48");
 Command(0,"jar_property","controller_space_height_81");
 assert(sensor->r.mins[2]==0&&sensor->r.maxs[2]==81);
 Command(0,"jar_property","controller_space_height_131073");assert(sensor->r.maxs[2]==81);
 Command(0,"jar_property","property_undo");assert(sensor->r.maxs[2]==128);
 Command(0,"jar_property","controller_space_height_81");
 vec3_t axis[3];AnglesToAxis(angles,axis);
 TriggerPlaceActor(p,space);VectorMA(position,90,axis[0],p->client->ps.origin);
 TriggerEnter(space,p);assert(JAR_MP_SpaceGravity(p)==0);
 VectorMA(position,30,axis[1],p->client->ps.origin);assert(JAR_MP_SpaceGravity(p)==-1);
 TriggerPlaceActor(p,space);
 Command(0,"jar_property","controller_playing");assert(!sensor->r.linked&&JAR_MP_SpaceGravity(p)==-1);
 Command(0,"jar_property","property_undo");assert(sensor->r.linked&&sensor->r.maxs[2]==81);
 TriggerEnter(space,p);assert(JAR_MP_SpaceGravity(p)==0);
 const int slot=sensor->s.number;G_FreeEntity(sensor);
 gentity_t*replacement=G_Spawn();assert(replacement->s.number==slot&&!p->client->inSpaceIndex);
 assert(JAR_MP_SpaceGravity(p)==-1);
 G_FreeEntity(replacement);Command(0,"jar_property","controller_playing");assert(TriggerHelper(n)&&TriggerHelper(n)->r.linked);
 CopyAssembly(p,n);assert(copies[0].count==1&&CopyControllersValid(&copies[0]));
 assert(PlaceCopy(p,&copies[0],(float[3]){900,0,80},NULL,NULL));
 const int pasted=players[0].selected;assert(pasted!=n&&TriggerHelper(pasted)&&TriggerHelper(pasted)->r.maxs[2]==81);
 Undo(p,qfalse);assert(!g_entities[pasted].inuse&&space->inuse);
 G_FreeEntity(droid);G_FreeEntity(vehicle);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));const int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 int restored=0;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_SPACE_CONTROLLER){
  ++restored;assert(objects[i].triggerExtent==192&&g_entities[i].genericValue1==48&&TriggerHelper(i)->r.maxs[2]==81&&TriggerHelper(i)->r.linked);
 }
 assert(restored==1&&WorldClear());
 // Version 1.204 stored a centered cube using the original half-size slot.
 assert(decoded.copy.count==1);
 jarControllerState_t*legacy=&decoded.copy.node[0].controller;
 legacy->flags=0;legacy->speed=96;legacy->damage=legacy->pushSpeed=0;
 const int legacySize=WorldEncode(&decoded,bytes);assert(legacySize&&WorldDecode(bytes,legacySize,&saved));
 assert(PlaceCopy(p,&saved.copy,vec3_origin,saved.velocity,saved.angular));
 const int migrated=players[0].selected;
 sensor=TriggerHelper(migrated);assert(sensor&&sensor->r.mins[2]==-96&&sensor->r.maxs[2]==96);
 ControllerSelect(&g_entities[migrated]);Command(0,"jar_property","controller_space_width_300");
 jarControllerState_t migratedState;ControllerState(migrated,&migratedState);
 int sizeAfterEdit[3];JAR_SpaceDimensions(&migratedState,sizeAfterEdit);
 assert(sizeAfterEdit[0]==300&&sizeAfterEdit[1]==192&&sizeAfterEdit[2]==192&&sensor->r.mins[2]==-96);
 assert(WorldClear());
 // The stock map brush retains its native gravity and damage cadence.
 gentity_t*authored=G_Spawn();authored->classname="trigger_space";authored->r.contents=CONTENTS_TRIGGER;
 VectorSet(authored->r.mins,-64,-64,-64);VectorSet(authored->r.maxs,64,64,64);
 RigidLink((sharedEntity_t*)authored);TriggerPlaceActor(p,authored);
 p->client->inSpaceIndex=authored->s.number;p->client->inSpaceSuffocation=level.time;
 const int damageBefore=spaceDamage;
 assert(JAR_MP_SpaceGravity(p)==1);JAR_MP_SpaceFrame(p);
 assert(spaceDamage-damageBefore>=50&&spaceDamage-damageBefore<=70&&spaceSounds==2);
 p->client->ps.origin[0]+=100;assert(JAR_MP_SpaceGravity(p)==-1);
 G_FreeEntity(authored);
 puts("MP space volume: oriented entry, consent, exposure, enclosed vehicles, Context, Undo and world restore passed.");
}

static void DirectionalMove(gentity_t*actor,const vec3_t start,const vec3_t end,int teleportBit){
 VectorCopy(start,actor->client->oldOrigin);VectorCopy(end,actor->client->ps.origin);
 JAR_MP_DirectionalCrossings(actor,teleportBit);
}

static void DirectionalCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 gentity_t*p=&g_entities[0];vec3_t origin={200,100,80},angles={0,90,0};
 gentity_t*root=SpawnWithController(p,89652619u,origin,angles);assert(root);
 int n=root->s.number;gentity_t*sensor=TriggerHelper(n);assert(sensor&&sensor->r.linked&&directionalSensors==1);
 assert(root->s.bolt2==JAR_MP_LOGIC_MARKER_TAG&&root->r.contents==CONTENTS_ITEM);
 vec3_t before={200,0,104},after={200,200,104};
 DirectionalMove(p,before,after,0);assert(controllerPrints==1&&strstr(printedMessage,"Forward crossing"));
 DirectionalMove(p,after,before,0);assert(controllerPrints==1);
 level.time+=1001;DirectionalMove(p,after,before,0);assert(controllerPrints==2&&strstr(printedMessage,"Reverse crossing"));
 level.time+=1001;
 DirectionalMove(p,(float[3]){270,0,104},(float[3]){270,200,104},0);
 DirectionalMove(p,before,after,EF_TELEPORT_BIT);DirectionalMove(p,before,before,0);assert(controllerPrints==2);
 players[0].noTools=1;DirectionalMove(p,before,after,0);players[0].noTools=0;assert(controllerPrints==2);
 ControllerSelect(root);Command(0,"jar_property","controller_extent_32");assert(objects[n].triggerExtent==32);
 Command(0,"jar_property","property_undo");assert(objects[n].triggerExtent==64);
 Command(0,"jar_property","controller_directional_uses_2");
 Command(0,"jar_property","controller_directional_wait_250");
 DirectionalMove(p,before,after,0);assert(root->count==1&&controllerPrints==3);
 const int deadline=root->painDebounceTime;
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 assert(decoded.copy.node[0].controllerRemaining==deadline-level.time&&decoded.copy.node[0].controller.count==1);
 CopyAssembly(p,n);assert(copies[0].count==3&&CopyControllersValid(&copies[0]));
 assert(PlaceCopy(p,&copies[0],(float[3]){600,100,80},NULL,NULL));
 const int copy=players[0].selected;assert(copy!=n&&TriggerHelper(copy)&&directionalSensors==2);
 assert(objects[copy].output[0]!=objects[n].output[0]&&objects[copy].output[1]!=objects[n].output[1]);
 Undo(p,qfalse);assert(!g_entities[copy].inuse&&directionalSensors==1);
 level.time=deadline;DirectionalMove(p,after,before,0);
 assert(root->count==0&&!sensor->r.linked&&root->genericValue3==1&&controllerPrints==4);
 ControllerSelect(root);Command(0,"jar_property","controller_playing");assert(root->count==2&&sensor->r.linked);
 Command(0,"jar_property","property_undo");assert(root->count==0&&!sensor->r.linked&&root->genericValue3==1);
 assert(WorldClear()&&!directionalSensors&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 assert(directionalSensors==1);worldEditing.integer=1;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==JAR_DIRECTIONAL_CONTROLLER){
  assert(TriggerHelper(i)&&TriggerHelper(i)->r.linked&&g_entities[i].count==1);
  DirectionalMove(p,after,before,0);assert(g_entities[i].count==1);
  level.time=g_entities[i].painDebounceTime;DirectionalMove(p,after,before,0);assert(g_entities[i].count==0);
 }
 assert(WorldClear()&&!directionalSensors);
 puts("MP directional trigger: swept crossings, consent, cooldown, uses, Context Undo, helper copies and world restore passed.");
}
