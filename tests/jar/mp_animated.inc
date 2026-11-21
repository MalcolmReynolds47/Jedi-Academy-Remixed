// Run the native animated controller callbacks, model parser and world codec.
// Frame data deliberately places each hull extreme in a different MD3 frame.
static struct {
 md3Header_t header;
 md3Frame_t frames[JAR_MP_BEACON_LOOP_LAST + 1];
} animatedModels[3];
static int animatedModel, animatedReadAt, animatedHullTraces, animatedHullPass;
static qboolean animatedExpectHiddenLink;
static vec3_t animatedTraceMins, animatedTraceMaxs;

static int AnimatedFixtureOpen(const char *path, fileHandle_t *f, fsMode_t mode) {
 if (!strcmp(path, animatedLoop) || !strcmp(path, animatedDisarm) ||
     !strcmp(path,JAR_BUTTON_PRESS_SOUND)||!strcmp(path,JAR_BUTTON_RETURN_SOUND)) {
  *f = 3;
  return 100;
 }
 if (!strcmp(path, "models/map_objects/factory/bomb_new_deact.md3")) animatedModel = 0;
 else if (!strcmp(path, "models/map_objects/wedge/beacon.md3")) animatedModel = 1;
 else if (!strcmp(path,JAR_BUTTON_MODEL)) animatedModel=2;
 else return RigidOpen(path, f, mode);
 *f = 2;
 animatedReadAt = 0;
 return animatedModels[animatedModel].header.ofsEnd;
}

static int AnimatedFixtureRead(void *out, int size, fileHandle_t f) {
 if (f != 2) return RigidRead(out, size, f);
 assert(size >= 0 && animatedReadAt + size <= animatedModels[animatedModel].header.ofsEnd);
 memcpy(out, (const byte *)&animatedModels[animatedModel] + animatedReadAt, size);
 animatedReadAt += size;
 return size;
}

static void AnimatedFixtureTrace(trace_t *tr, const vec3_t start, const vec3_t mins,
                                 const vec3_t maxs, const vec3_t end, int pass,
                                 int mask, int capsule, int flags, int lod) {
 RigidTrace(tr, start, mins, maxs, end, pass, mask, capsule, flags, lod);
 if (!mins) return;
 ++animatedHullTraces;
 animatedHullPass = pass;
 VectorCopy(mins, animatedTraceMins);
 VectorCopy(maxs, animatedTraceMaxs);
}

static void AnimatedFixtureLink(sharedEntity_t *e) {
 if (animatedExpectHiddenLink && JAR_MP_BeaconState(&e->s))
  assert((e->s.eFlags & EF_NODRAW) && (e->r.svFlags & SVF_NOCLIENT) && !e->r.contents);
 RigidLink(e);
}

static void AnimatedFixture(void) {
 RigidFixture();
 imports.FS_Open = AnimatedFixtureOpen;
 imports.FS_Read = AnimatedFixtureRead;
 imports.Trace = AnimatedFixtureTrace;
 imports.LinkEntity = AnimatedFixtureLink;
 imports.SendServerCommand = Send;
 aimed = ENTITYNUM_WORLD;
 animatedHullTraces = mediaEvents = 0;
 animatedExpectHiddenLink = qfalse;
 memset(animatedModels, 0, sizeof(animatedModels));
 for (int i = 0; i < ARRAY_LEN(animatedModels); ++i) {
  const int last = i==2?JAR_BUTTON_LAST:i==1?JAR_MP_BEACON_LOOP_LAST:JAR_MP_BOMB_LAST_FRAME;
  md3Header_t *h = &animatedModels[i].header;
  md3Frame_t *frames = animatedModels[i].frames;
  h->ident = MD3_IDENT;
  h->version = MD3_VERSION;
  h->numFrames = last + 1;
  h->ofsFrames = sizeof(*h);
  h->ofsEnd = h->ofsFrames + h->numFrames * sizeof(*frames);
  for (int frame = 0; frame <= last; ++frame) {
   VectorSet(frames[frame].bounds[0], -12, -8, -4);
   VectorSet(frames[frame].bounds[1], 20, 8, 12);
  }
  frames[1].bounds[0][0] = -13;
  frames[2].bounds[0][1] = -19;
  frames[3].bounds[0][2] = -7;
  frames[last - 1].bounds[1][1] = 15;
  frames[last].bounds[1][0] = 20 + last;
  frames[last].bounds[1][2] = 12 + last * .5f;
 }
}

static gentity_t *AnimatedMenuSpawn(unsigned id, int kind, int tag, int model) {
 char argument[32];
 const int before = LiveCount();
 snprintf(argument, sizeof(argument), "%u", id);
 Command(0, "jar_spawn", argument);
 const int n = Last(0);
 assert(n >= MAX_CLIENTS && LiveCount() == before + 1 && ControllerKind(n) == kind);
 gentity_t *e = &g_entities[n];
 assert(JAR_MP_AnimatedTag(&e->s) == tag && e->s.modelindex == model && e->s.frame == 0);
 assert(e->r.linked && e->r.contents == CONTENTS_SOLID && !e->takedamage && e->use == AnimatedUse);
 assert(e->r.svFlags & SVF_PLAYER_USABLE);
 assert(!objects[n].output[0] && !PhysicsPoseCount(n) && !PhysicsSetPose(e, 1));
 return e;
}

static void AnimatedBounds(const gentity_t *e, int last, qboolean rotated) {
 vec3_t low = {-13, -19, -7}, high = {20 + last, 15, 12 + last * .5f};
 if (rotated) {
  VectorSet(low, -15, -13, -7);
  VectorSet(high, 19, 20 + last, 12 + last * .5f);
 }
 assert(Distance(low, e->r.mins) < .001f && Distance(high, e->r.maxs) < .001f);
 assert(Distance(low, e->s.origin2) < .001f && Distance(high, e->s.angles2) < .001f);
}

static gentity_t *AnimatedFind(int kind) {
 for (int n = MAX_CLIENTS; n < level.num_entities; ++n)
  if (ControllerKind(n) == kind) return &g_entities[n];
 assert(0);
 return NULL;
}

static int AnimatedNode(const jarCopy_t *copy, unsigned id) {
 for (int i = 0; i < copy->count; ++i)
  if (copy->node[i].id == id) return i;
 assert(0);
 return -1;
}

static void AnimatedBombAndBeacon(void) {
 AnimatedFixture();
 gentity_t *p = &g_entities[0], *other = &g_entities[1];
 gentity_t *bomb = AnimatedMenuSpawn(JAR_MP_BOMB_ID, JAR_BOMB_CONTROLLER, JAR_MP_BOMB_TAG, FIXTURE_BOMB_MODEL);
 const int bn = bomb->s.number;
 assert(bomb->s.time == JAR_MP_BOMB_ARMED && bomb->s.loopSound == FIXTURE_ANIMATED_LOOP && !bomb->nextthink);
 AnimatedBounds(bomb, JAR_MP_BOMB_LAST_FRAME, qfalse);
 assert(PhysicsSetAngles(bomb, (float[3]){0, 90, 0}));
 AnimatedBounds(bomb, JAR_MP_BOMB_LAST_FRAME, qtrue);
 gentity_t *counter = ControllerAt("target_counter", 0, 400);
 ControllerLink(p, bn, qfalse);
 ControllerLink(p, counter->s.number, qfalse);
 assert(objects[bn].output[0] == counter->s.number + 1);
 GlobalUse(bomb, other, other);
 assert(bomb->s.time == JAR_MP_BOMB_ARMED && !mediaEvents && counter->count == 2);
 ControllerSelect(bomb);
 Command(0, "jar_property", "controller_enabled");
 assert(bomb->flags & FL_INACTIVE);
 assert(!bomb->s.loopSound && !(bomb->r.svFlags & SVF_PLAYER_USABLE));
 GlobalUse(bomb, p, p);
 assert(bomb->s.time == JAR_MP_BOMB_ARMED && !mediaEvents && counter->count == 2);
 Command(0, "jar_property", "property_undo");
 assert(!(bomb->flags & FL_INACTIVE) && bomb->s.loopSound == FIXTURE_ANIMATED_LOOP);
 GlobalUse(bomb, p, p);
 assert(bomb->s.time == JAR_MP_BOMB_DISARMING && bomb->s.frame == 0 && !bomb->s.loopSound);
 assert(mediaEvents == 1 && mediaEventEntity == bn && mediaEventType == EV_GENERAL_SOUND);
 assert(mediaEventParm == FIXTURE_BOMB_DISARM && counter->count == 1);
 const int deadline = bomb->nextthink;
 GlobalUse(bomb, p, p);
 assert(bomb->nextthink == deadline && mediaEvents == 1 && counter->count == 1);
 ControllerTick(bomb, 399);
 assert(bomb->s.time == JAR_MP_BOMB_DISARMING && bomb->s.frame == 7 && AnimatedRemaining(bomb) == 1);
 ControllerTick(bomb, 1);
 assert(bomb->s.time == JAR_MP_BOMB_DISARMED && bomb->s.frame == 8 && !bomb->nextthink);
 GlobalUse(bomb, p, p);
 assert(mediaEvents == 1 && counter->count == 1);
 AnimatedBounds(bomb, JAR_MP_BOMB_LAST_FRAME, qtrue);

 gentity_t *beacon = AnimatedMenuSpawn(JAR_MP_BEACON_ID, JAR_BEACON_CONTROLLER, JAR_MP_BEACON_TAG, FIXTURE_BEACON_MODEL);
 const int sn = beacon->s.number;
 assert(beacon->s.time == JAR_MP_BEACON_DEPLOYING && !beacon->s.loopSound && AnimatedRemaining(beacon) == 50);
 assert(PhysicsSetAngles(beacon, (float[3]){0, 90, 0}));
 AnimatedBounds(beacon, JAR_MP_BEACON_LOOP_LAST, qtrue);
 ControllerTick(beacon, 1499);
 assert(beacon->s.time == JAR_MP_BEACON_DEPLOYING && beacon->s.frame == 29 && !beacon->s.loopSound);
 ControllerTick(beacon, 1);
 assert(beacon->s.frame == 30 && !beacon->s.loopSound);
 ControllerTick(beacon, 50);
 assert(beacon->s.time == JAR_MP_BEACON_SIGNALLING && beacon->s.frame == 31 && beacon->s.loopSound == FIXTURE_ANIMATED_LOOP);
 ControllerTick(beacon, 1450);
 assert(beacon->s.frame == 60 && beacon->s.loopSound == FIXTURE_ANIMATED_LOOP);
 ControllerTick(beacon, 50);
 assert(beacon->s.frame == 31 && beacon->s.loopSound == FIXTURE_ANIMATED_LOOP);
 AnimatedBounds(beacon, JAR_MP_BEACON_LOOP_LAST, qtrue);
 ActorArm(0, sn);
 assert(players[0].held == sn);
 GlobalUse(beacon, p, p);
 assert(BodyDormant(sn) && !beacon->r.contents && !beacon->s.loopSound && !beacon->nextthink);
 assert((beacon->r.svFlags & SVF_NOCLIENT) && !(beacon->r.svFlags & SVF_PLAYER_USABLE));
 assert(beacon->s.frame == 0 && players[0].held == ENTITYNUM_NONE && !objects[sn].heldBy);
 clients[0].pers.cmd.buttons = 0;
 const int traces = animatedHullTraces;
 blocked = 1;
 GlobalUse(beacon, p, p);
 assert(BodyDormant(sn) && animatedHullTraces == traces + 1 && animatedHullPass == sn);
 assert(VectorCompare(animatedTraceMins, beacon->r.mins) && VectorCompare(animatedTraceMaxs, beacon->r.maxs));
 blocked = 0;
 GlobalUse(beacon, p, p);
 assert(!BodyDormant(sn) && beacon->r.contents == CONTENTS_SOLID && beacon->s.time == JAR_MP_BEACON_DEPLOYING);
 assert(beacon->s.frame == 0 && beacon->nextthink - level.time == 50 && mediaEvents == 1);
 AnimatedBounds(beacon, JAR_MP_BEACON_LOOP_LAST, qtrue);
 assert(WorldClear());
}

static void AnimatedPersistence(void) {
 AnimatedFixture();
 gentity_t *p = &g_entities[0];
 gentity_t *bomb = AnimatedMenuSpawn(JAR_MP_BOMB_ID, JAR_BOMB_CONTROLLER, JAR_MP_BOMB_TAG, FIXTURE_BOMB_MODEL);
 gentity_t *beacon = AnimatedMenuSpawn(JAR_MP_BEACON_ID, JAR_BEACON_CONTROLLER, JAR_MP_BEACON_TAG, FIXTURE_BEACON_MODEL);
 gentity_t *counter = ControllerAt("target_counter", 0, 400);
 Move(bomb, (float[3]){100, 0, 80});
 Move(beacon, (float[3]){700, 0, 80});
 ControllerLink(p, bomb->s.number, qfalse);
 ControllerLink(p, counter->s.number, qfalse);
 ControllerTick(beacon, 1670);
 assert(beacon->s.frame == 33 && AnimatedRemaining(beacon) == 30);
 players[0].selected = beacon->s.number;
 assert(ContextController(p, "controller_enabled"));
 assert((beacon->flags & FL_INACTIVE) && !beacon->s.loopSound && !beacon->nextthink);
 assert(AnimatedRemaining(beacon) == 30);
 ControllerTick(beacon, 1000);
 assert(beacon->s.frame == 33 && AnimatedRemaining(beacon) == 30);
 GlobalUse(bomb, p, p);
 ControllerTick(bomb, 123);
 assert(bomb->s.frame == 2 && AnimatedRemaining(bomb) == 27);
 static jarCopy_t bombCopy;
 CopyAssembly(p, bomb->s.number);
 bombCopy = copies[0];
 assert(bombCopy.count == 2 && bombCopy.node[0].poseFrame == 2 && bombCopy.node[0].controllerRemaining == 27);
 CopyAssembly(p, beacon->s.number);
 assert(copies[0].count == 1 && copies[0].node[0].poseFrame == 33 && copies[0].node[0].controllerRemaining == 30);

 static jarWorld_t saved, decoded;
 static byte data[WORLD_BYTES];
 assert(WorldCapture(&saved) && saved.copy.count == 3);
 const int size = WorldEncode(&saved, data);
 int at = 0;
 assert(size && WorldGet(data, &at) == 0x4957524au && WorldDecode(data, size, &decoded));
 const int b = AnimatedNode(&decoded.copy, JAR_MP_BOMB_ID), s = AnimatedNode(&decoded.copy, JAR_MP_BEACON_ID);
 jarCopyNode_t *bombNode = &decoded.copy.node[b], *beaconNode = &decoded.copy.node[s];
 assert(bombNode->controller.active == JAR_MP_BOMB_DISARMING && bombNode->poseFrame == 2 && bombNode->controllerRemaining == 27);
 assert(beaconNode->controller.active == JAR_MP_BEACON_SIGNALLING && beaconNode->controller.inactive);
 assert(beaconNode->poseFrame == 33 && beaconNode->controllerRemaining == 30);
 bombNode->poseFrame = 8;
 assert(!CopyControllersValid(&decoded.copy));
 bombNode->poseFrame = 2;
 beaconNode->controllerRemaining = 0;
 assert(!CopyControllersValid(&decoded.copy));
 beaconNode->controllerRemaining = JAR_MP_ANIM_TICK_MS + 1;
 assert(!CopyControllersValid(&decoded.copy));
 assert(WorldDecode(data, size, &decoded) && WorldClear());
 assert(PlaceCopy(NULL, &decoded.copy, vec3_origin, decoded.velocity, decoded.angular));
 bomb = AnimatedFind(JAR_BOMB_CONTROLLER);
 beacon = AnimatedFind(JAR_BEACON_CONTROLLER);
 counter = AnimatedFind(4);
 assert(bomb->s.frame == 2 && bomb->nextthink - level.time == 27 && counter->count == 1 && mediaEvents == 1);
 assert(objects[bomb->s.number].output[0] == counter->s.number + 1);
 assert(beacon->s.frame == 33 && !beacon->nextthink && !beacon->s.loopSound && AnimatedRemaining(beacon) == 30);
 ControllerTick(bomb, 277);
 assert(bomb->s.frame == 8 && bomb->s.time == JAR_MP_BOMB_DISARMED && counter->count == 1 && mediaEvents == 1);
 assert(WorldClear());

 assert(PlaceCopy(p, &bombCopy, (float[3]){100, 0, 80}, NULL, NULL));
 bomb = AnimatedFind(JAR_BOMB_CONTROLLER);
 counter = AnimatedFind(4);
 assert(bomb->s.frame == 2 && bomb->nextthink - level.time == 27 && counter->count == 1 && mediaEvents == 1);
 assert(PlaceCopy(p, &copies[0], (float[3]){700, 0, 80}, NULL, NULL));
 beacon = AnimatedFind(JAR_BEACON_CONTROLLER);
 assert(beacon->s.frame == 33 && !beacon->nextthink && !beacon->s.loopSound && AnimatedRemaining(beacon) == 30);
 assert(ContextController(p, "controller_enabled"));
 assert(beacon->nextthink - level.time == 30 && beacon->s.loopSound == FIXTURE_ANIMATED_LOOP);
 ControllerTick(beacon, 29);
 assert(beacon->s.frame == 33);
 ControllerTick(beacon, 1);
 assert(beacon->s.frame == 34 && mediaEvents == 1 && counter->count == 1);
 assert(WorldClear());
}

static void AnimatedHiddenRestore(void) {
 AnimatedFixture();
 gentity_t *p = &g_entities[0];
 gentity_t *e = AnimatedMenuSpawn(JAR_MP_BEACON_ID, JAR_BEACON_CONTROLLER, JAR_MP_BEACON_TAG, FIXTURE_BEACON_MODEL);
 GlobalUse(e, p, p);
 assert(BodyDormant(e->s.number));
 const vec3_t position = {700, 0, 80};
 Move(e, position);
 const vec3_t from = {500, 0, 80}, before = {650, 0, 80}, after = {900, 0, 80};
 assert(ContextHiddenBeacon(p, from, after) == e->s.number);
 assert(ContextHiddenBeacon(p, from, before) == ENTITYNUM_NONE);
 assert(ContextHiddenBeacon(&g_entities[1], from, after) == ENTITYNUM_NONE);
 CopyAssembly(p, e->s.number);
 assert(copies[0].count == 1 && CopyHiddenBeacon(&copies[0].node[0]));
 static jarWorld_t world, decoded;
 static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world) && world.copy.count == 1);
 int size = WorldEncode(&world, bytes);
 assert(size && WorldDecode(bytes, size, &decoded) && WorldClear());

 // A blocker at the saved transform permits dormant placement and prevents
 // the first reveal. Neither restore publishes a solid frame at that pose.
 blocked = 1;
 animatedExpectHiddenLink = qtrue;
 const int traces = animatedHullTraces;
 assert(PlaceCopy(NULL, &decoded.copy, vec3_origin, decoded.velocity, decoded.angular));
 e = AnimatedFind(JAR_BEACON_CONTROLLER);
 assert(VectorCompare(e->r.currentOrigin, position) && BodyDormant(e->s.number));
 assert(!e->r.contents && !e->s.loopSound && !e->nextthink && animatedHullTraces == traces);
 assert(WorldClear());
 assert(PlaceCopy(p, &copies[0], position, NULL, NULL));
 e = AnimatedFind(JAR_BEACON_CONTROLLER);
 assert(VectorCompare(e->r.currentOrigin, position) && BodyDormant(e->s.number) && animatedHullTraces == traces);
 GlobalUse(e, p, p);
 assert(BodyDormant(e->s.number) && animatedHullTraces == traces + 1);
 blocked = 0;
 animatedExpectHiddenLink = qfalse;
 GlobalUse(e, p, p);
 assert(!BodyDormant(e->s.number) && e->s.time == JAR_MP_BEACON_DEPLOYING && !e->s.frame && !mediaEvents);
 assert(WorldClear());
}

static void AnimatedSeatedContext(void) {
 AnimatedFixture();
 gentity_t *vehicle=SeatVehicle(VH_ANIMAL),*pilot=&g_entities[1];
 sharedEditing.integer=1;
 gentity_t *bomb=AnimatedMenuSpawn(JAR_MP_BOMB_ID,JAR_BOMB_CONTROLLER,JAR_MP_BOMB_TAG,FIXTURE_BOMB_MODEL);
 aimed=bomb->s.number;
 Command(1,"jar_select","");
 assert(players[1].selected==bomb->s.number);
 GlobalUse(bomb,pilot,pilot);
 assert(bomb->s.time==JAR_MP_BOMB_ARMED&&!mediaEvents);
 Command(1,"jar_property","activate");
 assert(bomb->s.time==JAR_MP_BOMB_DISARMING&&mediaEvents==1);
 assert(pilot->client->ps.m_iVehicleNum==vehicle->s.number);
 vehicle->m_pVehicle->m_pPilot=vehicle->m_pVehicle->m_pOldPilot=vehicle->m_pVehicle->m_ppPassengers[0]=NULL;
 vehicle->m_pVehicle->m_iNumPassengers=0;
 clients[1].ps.m_iVehicleNum=clients[2].ps.m_iVehicleNum=vehicle->client->ps.m_iVehicleNum=0;
 assert(WorldClear());
}

static void ButtonCases(void){
 AnimatedFixture();imports.SendServerCommand=ControllerPropertySend;controllerPrints=0;
 gentity_t* owner=&g_entities[0];
 const int previous=LiveCount();
 Command(0,"jar_spawn","96653372");
 gentity_t* button=NULL;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==JAR_BUTTON_CONTROLLER)button=&g_entities[n];
 assert(button&&LiveCount()==previous+2&&objects[button->s.number].output[0]);
 assert(JAR_MP_AnimatedTag(&button->s)==JAR_MP_BUTTON_TAG&&button->r.contents==CONTENTS_SOLID&&!PhysicsPoseCount(button->s.number));
 AnimatedBounds(button,JAR_BUTTON_LAST,qfalse);
 gentity_t* released=ControllerAt("target_print",0,700);
 Command(0,"jar_setting","output_secondary");
 ControllerLink(owner,button->s.number,qfalse);ControllerLink(owner,released->s.number,qfalse);
 assert(objects[button->s.number].output[1]==released->s.number+1);
 ControllerSelect(button);
 Command(0,"jar_property","controller_button_wait_30001");assert(button->delay==30001);
 Command(0,"jar_property","property_undo");assert(button->delay==JAR_BUTTON_WAIT);
 Command(0,"jar_property","controller_button_wait_125");
 ControllerShot(owner,button->s.number,qfalse);
 level.time+=100;ButtonTick(button);assert(button->s.frame==2&&button->s.time==JAR_BUTTON_PRESSING&&!controllerPrints);
 ControllerShot(owner,button->s.number,qfalse);assert(button->s.frame==2);
 level.time+=100;ButtonTick(button);assert(button->s.frame==4&&button->s.time==JAR_BUTTON_HELD&&controllerPrints==1);
 Command(0,"jar_property","controller_enabled");
 level.time+=100;ButtonTick(button);assert(button->genericValue8==125);
 Command(0,"jar_property","property_undo");assert(!(button->flags&FL_INACTIVE));
 level.time+=80;ButtonTick(button);assert(button->genericValue8==45);
 static jarWorld_t world,restored;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&world));const int len=WorldEncode(&world,bytes);assert(len&&WorldDecode(bytes,len,&restored));
 assert(WorldClear()&&PlaceCopy(NULL,&restored.copy,vec3_origin,restored.velocity,restored.angular));
 button=AnimatedFind(JAR_BUTTON_CONTROLLER);
 assert(button&&button->s.frame==4&&button->s.time==JAR_BUTTON_HELD&&button->genericValue8==45);
 level.time+=95;ButtonTick(button);assert(button->s.frame==3&&button->s.time==JAR_BUTTON_RETURNING);
 for(int i=0;i<3;++i){level.previousTime=level.time;level.time+=50;JAR_MP_Frame();}
 assert(button->s.frame==0&&button->s.time==JAR_BUTTON_IDLE&&controllerPrints==2);
 assert(WorldClear());
 puts("MP Imperial button: Q spawn, stable hull, both outputs, bounded hold, pause, Undo and exact native world restore passed.");
}

static void AnimatedCases(void) {
 AnimatedBombAndBeacon();
 AnimatedPersistence();
 AnimatedHiddenRestore();
 AnimatedSeatedContext();
 ButtonCases();
 PhysicsReset();
 imports.FS_Open = RigidOpen;
 imports.FS_Read = RigidRead;
 imports.Trace = RigidTrace;
 imports.LinkEntity = RigidLink;
 puts("MP animated props: stock Q spawn, fixed rotated hull, native sound and one-shot link, 50ms phases, pause and exact JRW18/clipboard restore, and blocked hidden beacon reveal passed.");
}
