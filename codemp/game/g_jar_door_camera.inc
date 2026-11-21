// Stock telescoping camera with an authoritative pose and private parts.
// GPL-2.0-only.
enum { DOOR_CAMERA_PARTS = 2, DOOR_CAMERA_HALF_EXTENT = 14 };

typedef struct {
 qboolean live, published, extended;
 unsigned epoch, publishedIdentity;
 int part[DOOR_CAMERA_PARTS], travel, speed, servo, lastTime;
 float extension, publishedExtension;
 vec3_t origin, angles;
} jarDoorCamera_t;

typedef struct {
 int root, role;
 unsigned epoch;
} jarDoorCameraPart_t;

static jarDoorCamera_t doorCameras[MAX_GENTITIES];
static jarDoorCameraPart_t doorCameraParts[MAX_GENTITIES];

static jarDoorCamera_t *DoorCameraData(int n) {
 if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !g_entities[n].inuse ||
     !doorCameras[n].live || doorCameras[n].epoch != controllerEntityEpoch[n]) return NULL;
 return &doorCameras[n];
}

static int DoorCameraRoot(int n) {
 if (DoorCameraData(n)) return n;
 if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !g_entities[n].inuse) return ENTITYNUM_NONE;
 const gentity_t *entity = &g_entities[n];
 const jarDoorCameraPart_t *part = &doorCameraParts[n];
 const int root = part->root - 1, slot = part->role - 1;
 const jarDoorCamera_t *camera = DoorCameraData(root);
 if (!camera || slot < 0 || slot >= DOOR_CAMERA_PARTS || part->epoch != controllerEntityEpoch[n] ||
     camera->part[slot] != n + 1 || entity->s.eType != ET_GENERAL ||
     entity->s.generic1 != JAR_MP_DOOR_CAMERA_PART_TAG || entity->s.bolt2 != part->role) return ENTITYNUM_NONE;
 return root;
}

static gentity_t *DoorCameraPart(int root, int role) {
 const int n = doorCameras[root].part[role - 1] - 1;
 return n >= MAX_CLIENTS && n < ENTITYNUM_WORLD && DoorCameraRoot(n) == root ? &g_entities[n] : NULL;
}

static void DoorCameraBounds(const vec3_t angles, float extension, vec3_t mins, vec3_t maxs) {
 matrix3_t axis;
 AnglesToAxis(angles, axis);
 const float halfSpan = (JAR_MP_DOOR_CAMERA_CLOSED + extension) * .5f;
 for (int a = 0; a < 3; ++a) {
  const float center = axis[0][a] * halfSpan;
  const float extent = fabsf(axis[0][a]) * (halfSpan + DOOR_CAMERA_HALF_EXTENT) +
                       (fabsf(axis[1][a]) + fabsf(axis[2][a])) * DOOR_CAMERA_HALF_EXTENT;
  mins[a] = center - extent;
  maxs[a] = center + extent;
 }
}

static qboolean DoorCameraHull(int n, const vec3_t angles, vec3_t mins, vec3_t maxs) {
 const jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera) return qfalse;
 DoorCameraBounds(angles, camera->extension, mins, maxs);
 return qtrue;
}

static qboolean DoorCameraPickBounds(int n, vec3_t mins, vec3_t maxs) {
 const int root = DoorCameraRoot(n);
 if (root == ENTITYNUM_NONE) return qfalse;
 // The native enclosing hull is for moving the assembly. Picking uses each
 // stock MD3's local frame bounds, with only the shaft's Z length scaled.
 if (n == root) {
  VectorSet(mins, 0, -8.34375f, -6.03125f);
  VectorSet(maxs, 7.21875f, 8.34375f, 6.03125f);
 } else if (doorCameraParts[n].role == JAR_MP_DOOR_CAMERA_SHAFT) {
  VectorSet(mins, -5.5f, -6.34375f, -(JAR_MP_DOOR_CAMERA_CLOSED + doorCameras[root].extension));
  VectorSet(maxs, 5.484375f, 6.359375f, 0);
 } else {
  VectorSet(mins, -9.28125f, -4.890625f, -7.640625f);
  VectorSet(maxs, 9.375f, 4.90625f, 2.921875f);
 }
 return qtrue;
}

static void DoorCameraPlacePart(gentity_t *part, const vec3_t origin, const vec3_t angles,
                               unsigned identity) {
 G_SetOrigin(part, (float *)origin);
 VectorCopy(origin, part->s.origin);
 VectorCopy(angles, part->r.currentAngles);
 VectorCopy(angles, part->s.angles);
 VectorCopy(angles, part->s.apos.trBase);
 part->s.pos.trType = part->s.apos.trType = TR_INTERPOLATE;
 part->s.time2 = (int)identity;
 trap->LinkEntity((sharedEntity_t *)part);
}

static void DoorCameraSync(int n) {
 jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera) return;
 gentity_t *root = &g_entities[n];
 const unsigned identity = PhysicsIdentity(n);
 if (camera->published && camera->publishedIdentity == identity && camera->publishedExtension == camera->extension &&
     VectorCompare(camera->origin, root->r.currentOrigin) && VectorCompare(camera->angles, root->r.currentAngles)) return;
 camera->published = qtrue;
 camera->publishedIdentity = identity;
 camera->publishedExtension = camera->extension;
 VectorCopy(root->r.currentOrigin, camera->origin);
 VectorCopy(root->r.currentAngles, camera->angles);

 DoorCameraBounds(camera->angles, camera->extension, root->r.mins, root->r.maxs);
 VectorCopy(root->r.mins, root->s.origin2);
 VectorCopy(root->r.maxs, root->s.angles2);
 VectorCopy(camera->origin, root->s.origin);
 VectorCopy(camera->angles, root->s.angles);
 VectorCopy(camera->angles, root->s.apos.trBase);
 root->s.apos.trType = TR_INTERPOLATE;
 root->s.generic1 = 250;
 root->s.bolt2 = JAR_MP_DOOR_CAMERA_ROOT_TAG;
 root->s.time2 = (int)identity;

 matrix3_t axis;
 AnglesToAxis(camera->angles, axis);
 const float length = JAR_MP_DOOR_CAMERA_CLOSED + camera->extension;
 gentity_t *head = DoorCameraPart(n, JAR_MP_DOOR_CAMERA_HEAD);
 if (head) {
  vec3_t position;
  VectorMA(camera->origin, length, axis[0], position);
  DoorCameraPlacePart(head, position, camera->angles, identity);
 }
 gentity_t *shaft = DoorCameraPart(n, JAR_MP_DOOR_CAMERA_SHAFT);
 if (shaft) {
  // The MD3 shaft extends down local Z. Keep root roll when turning that
  // local negative Z axis into the root's forward direction.
  vec3_t angles;
  matrix3_t zeroRoll;
  vectoangles(axis[2], angles);
  AnglesToAxis(angles, zeroRoll);
  angles[ROLL] = atan2f(DotProduct(axis[1], zeroRoll[2]), DotProduct(axis[1], zeroRoll[1])) * (180.0f / M_PI);
  shaft->s.speed = length / JAR_MP_DOOR_CAMERA_SHAFT_LENGTH;
  VectorCopy(root->r.mins, shaft->r.mins);
  VectorCopy(root->r.maxs, shaft->r.maxs);
  DoorCameraPlacePart(shaft, camera->origin, angles, identity);
 }
 trap->LinkEntity((sharedEntity_t *)root);
}

static qboolean DoorCameraReady(const gentity_t *root) {
 const int n = root->s.number;
 return sandbox.integer && !(root->flags & FL_INACTIVE) && !objects[n].copyPending &&
        DoorCameraPart(n, JAR_MP_DOOR_CAMERA_SHAFT) && DoorCameraPart(n, JAR_MP_DOOR_CAMERA_HEAD);
}

static void DoorCameraUseFlags(gentity_t *root, qboolean usable) {
 const int flag = usable ? SVF_PLAYER_USABLE : 0;
 root->r.svFlags = (root->r.svFlags & ~SVF_PLAYER_USABLE) | flag;
 for (int role = 1; role <= DOOR_CAMERA_PARTS; ++role) {
  gentity_t *part = DoorCameraPart(root->s.number, role);
  if (part) part->r.svFlags = (part->r.svFlags & ~SVF_PLAYER_USABLE) | flag;
 }
}

static void DoorCameraSweep(const gentity_t *root, const gentity_t *head,
                            float extension, trace_t *trace) {
 vec3_t forward, end;
 AngleVectors(root->r.currentAngles, forward, NULL, NULL);
 VectorMA(root->r.currentOrigin, JAR_MP_DOOR_CAMERA_CLOSED + extension, forward, end);
 trap->Trace(trace, head->r.currentOrigin, head->r.mins, head->r.maxs, end,
             root->s.number, MASK_SOLID | CONTENTS_BODY, qfalse, 0, 0);
}

static void DoorCameraFrame(gentity_t *root) {
 const int n = root->s.number;
 jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera) return;
 const int64_t elapsed = (int64_t)level.time - camera->lastTime;
 camera->lastTime = level.time;
 const qboolean ready = DoorCameraReady(root);
 DoorCameraUseFlags(root, ready);
 const int previousStatus = root->s.time;
 if (!ready) {
  root->s.loopSound = 0;
  root->s.time = JAR_MP_DOOR_CAMERA_REST;
 } else if (elapsed > 0) {
  const float target = camera->extended ? (float)camera->travel / JAR_MP_DOOR_CAMERA_MILLI : 0;
  const float step = ((float)camera->speed / JAR_MP_DOOR_CAMERA_MILLI) * ((float)elapsed * .001f);
  const float before = camera->extension;
  const float next = target > before ? fminf(target, before + step) : fmaxf(target, before - step);
  root->s.loopSound = 0;
  root->s.time = JAR_MP_DOOR_CAMERA_REST;
  if (next != before) {
   trace_t trace;
   DoorCameraSweep(root, DoorCameraPart(n, JAR_MP_DOOR_CAMERA_HEAD), next, &trace);
   const qboolean blocked = trace.startsolid || trace.allsolid || trace.fraction < 1;
   if (!trace.startsolid && !trace.allsolid)
    camera->extension = blocked ? before + (next - before) * Com_Clamp(0, 1, trace.fraction) : next;
   if (camera->extension != before) DoorCameraSync(n);
   if (blocked) root->s.time = JAR_MP_DOOR_CAMERA_BLOCKED;
   else if (camera->extension != target) {
    root->s.time = JAR_MP_DOOR_CAMERA_MOVING;
    if (fabsf(camera->extension - before) > .001f) root->s.loopSound = camera->servo;
   }
  }
 }
 if (previousStatus != root->s.time) SelectionChanged(n);
}

static void DoorCameraUse(gentity_t *root, gentity_t *other, gentity_t *activator) {
 const int n = root->s.number;
 jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera || !DoorCameraReady(root)) return;
 if (other == activator && activator && activator->client) {
  if (activator->s.number < MAX_CLIENTS) {
   if (!Edit(activator, n) || (!controllerCalling[n] && !Alive(activator))) return;
  } else if (!ActionActivator(root, activator)) return;
 }
 camera->extended = !camera->extended;
 camera->lastTime = level.time;
 root->s.loopSound = 0;
 root->s.time = JAR_MP_DOOR_CAMERA_REST;
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void DoorCameraPartUse(gentity_t *part, gentity_t *other, gentity_t *activator) {
 const int root = DoorCameraRoot(part->s.number);
 if (root != ENTITYNUM_NONE) DoorCameraUse(&g_entities[root], other, activator);
}

static int DoorCameraExtent(const jarDoorCamera_t *camera) {
 return Com_Clampi(0, camera->travel, (int)lroundf(camera->extension * JAR_MP_DOOR_CAMERA_MILLI));
}

static void DoorCameraState(int n, jarControllerState_t *state) {
 const jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera) return;
 state->damage = camera->travel;
 state->speed = camera->speed;
 state->pushSpeed = DoorCameraExtent(camera);
 state->active = camera->extended;
 state->inactive = !!(g_entities[n].flags & FL_INACTIVE);
}

static qboolean DoorCameraApply(int n, const jarControllerState_t *state) {
 jarDoorCamera_t *camera = DoorCameraData(n);
 if (!camera) return qfalse;
 jarControllerState_t next = *state;
 // Settings edits and Undo leave the moving pose alone. Only a reduced
 // travel maximum can shorten it, and that path still has to be clear.
 next.pushSpeed = Com_Clampi(0, next.damage, DoorCameraExtent(camera));
 if (!JAR_ControllerStateValid(JAR_DOOR_CAMERA_CONTROLLER, &next)) return qfalse;
 gentity_t *root = &g_entities[n];
 const float travel = (float)next.damage / JAR_MP_DOOR_CAMERA_MILLI;
 if (camera->extension > travel) {
  const gentity_t *head = DoorCameraPart(n, JAR_MP_DOOR_CAMERA_HEAD);
  if (!head) return qfalse;
  trace_t trace;
  DoorCameraSweep(root, head, travel, &trace);
  if (trace.startsolid || trace.allsolid || trace.fraction < 1) return qfalse;
  camera->extension = travel;
 }
 camera->travel = next.damage;
 camera->speed = next.speed;
 camera->extended = next.active;
 camera->lastTime = level.time;
 if (next.inactive) root->flags |= FL_INACTIVE;
 else root->flags &= ~FL_INACTIVE;
 root->s.loopSound = 0;
 root->s.time = JAR_MP_DOOR_CAMERA_REST;
 DoorCameraUseFlags(root, DoorCameraReady(root));
 DoorCameraSync(n);
 return qtrue;
}

static void DoorCameraForget(int n) {
 const int root = DoorCameraRoot(n);
 if (root == ENTITYNUM_NONE) return;
 jarDoorCamera_t *camera = &doorCameras[root];
 g_entities[root].s.loopSound = 0;
 g_entities[root].s.time = JAR_MP_DOOR_CAMERA_REST;
 if (root != n) {
  camera->part[doorCameraParts[n].role - 1] = 0;
  memset(&doorCameraParts[n], 0, sizeof(doorCameraParts[n]));
  DoorCameraUseFlags(&g_entities[root], qfalse);
  return;
 }
 for (int role = 1; role <= DOOR_CAMERA_PARTS; ++role) {
  gentity_t *part = DoorCameraPart(root, role);
  if (!part) continue;
  camera->part[role - 1] = 0;
  memset(&doorCameraParts[part->s.number], 0, sizeof(doorCameraParts[part->s.number]));
  G_FreeEntity(part);
 }
 memset(camera, 0, sizeof(*camera));
}

static void DoorCameraReset(void) {
 memset(doorCameras, 0, sizeof(doorCameras));
 memset(doorCameraParts, 0, sizeof(doorCameraParts));
}

static qboolean DoorCameraModelsAvailable(const char *const models[DOOR_CAMERA_PARTS]) {
 qboolean registered[DOOR_CAMERA_PARTS] = {qfalse};
 int freeSlots = 0, needed = 0;
 for (int i = 1; i < MAX_MODELS; ++i) {
  char path[MAX_QPATH];
  trap->GetConfigstring(CS_MODELS + i, path, sizeof(path));
  if (!*path) ++freeSlots;
  else for (int part = 0; part < DOOR_CAMERA_PARTS; ++part)
   if (!Q_stricmp(path, models[part])) registered[part] = qtrue;
 }
 for (int part = 0; part < DOOR_CAMERA_PARTS; ++part) if (!registered[part]) ++needed;
 return !needed || freeSlots >= 64 + needed;
}

static qboolean DoorCameraInit(gentity_t *root, const jarControllerState_t *saved) {
 const int n = root->s.number;
 const char *const models[DOOR_CAMERA_PARTS] = {JAR_MP_DOOR_CAMERA_SHAFT_MODEL, JAR_MP_DOOR_CAMERA_HEAD_MODEL};
 jarControllerState_t initial = {0};
 initial.damage = JAR_MP_DOOR_CAMERA_DEFAULT_TRAVEL;
 initial.speed = JAR_MP_DOOR_CAMERA_DEFAULT_SPEED;
 if (saved) initial = *saved;
 if (!JAR_ControllerStateValid(JAR_DOOR_CAMERA_CONTROLLER, &initial) || !RoomExtra(DOOR_CAMERA_PARTS) ||
     !DoorCameraModelsAvailable(models) || !MediaValidate(9, JAR_MP_DOOR_CAMERA_SERVO)) return qfalse;
 for (int part = 0; part < DOOR_CAMERA_PARTS; ++part) {
  fileHandle_t file = 0;
  const int size = trap->FS_Open(models[part], &file, FS_READ);
  if (file) trap->FS_Close(file);
  if (!file || size <= 0) return qfalse;
 }

 gentity_t *parts[DOOR_CAMERA_PARTS] = {0};
 int modelIndex[DOOR_CAMERA_PARTS] = {0};
 for (int part = 0; part < DOOR_CAMERA_PARTS; ++part) {
  parts[part] = G_TrySpawn();
  if (!parts[part]) goto fail;
 }
 for (int part = 0; part < DOOR_CAMERA_PARTS; ++part) {
  modelIndex[part] = G_ModelIndex(models[part]);
  if (!modelIndex[part]) goto fail;
 }
 const int servo = G_SoundIndex(JAR_MP_DOOR_CAMERA_SERVO);
 if (!servo) goto fail;

 jarDoorCamera_t *camera = &doorCameras[n];
 memset(camera, 0, sizeof(*camera));
 camera->live = qtrue;
 camera->epoch = controllerEntityEpoch[n];
 camera->servo = servo;
 camera->lastTime = level.time;
 camera->travel = initial.damage;
 camera->speed = initial.speed;
 camera->extension = (float)initial.pushSpeed / JAR_MP_DOOR_CAMERA_MILLI;
 camera->extended = initial.active;
 for (int slot = 0; slot < DOOR_CAMERA_PARTS; ++slot) {
  gentity_t *part = parts[slot];
  const int child = part->s.number, role = slot + 1;
  camera->part[slot] = child + 1;
  doorCameraParts[child] = (jarDoorCameraPart_t){n + 1, role, controllerEntityEpoch[child]};
  part->classname = "jar_door_camera_part";
  part->model = (char *)models[slot];
  part->s.eType = ET_GENERAL;
  part->s.generic1 = JAR_MP_DOOR_CAMERA_PART_TAG;
  part->s.bolt2 = role;
  part->s.modelindex = modelIndex[slot];
  part->s.otherEntityNum = n;
  part->r.contents = CONTENTS_ITEM;
  part->use = DoorCameraPartUse;
  VectorSet(part->r.mins, -DOOR_CAMERA_HALF_EXTENT, -DOOR_CAMERA_HALF_EXTENT, -DOOR_CAMERA_HALF_EXTENT);
  VectorSet(part->r.maxs, DOOR_CAMERA_HALF_EXTENT, DOOR_CAMERA_HALF_EXTENT, DOOR_CAMERA_HALF_EXTENT);
 }
 root->r.contents = CONTENTS_ITEM;
 root->takedamage = qfalse;
 root->use = DoorCameraUse;
 root->s.loopSound = 0;
 root->s.time = JAR_MP_DOOR_CAMERA_REST;
 if (initial.inactive) root->flags |= FL_INACTIVE;
 else root->flags &= ~FL_INACTIVE;
 objects[n].mass = 20;
 DoorCameraUseFlags(root, DoorCameraReady(root));
 DoorCameraSync(n);
 return qtrue;

fail:
 for (int part = 0; part < DOOR_CAMERA_PARTS; ++part) if (parts[part]) G_FreeEntity(parts[part]);
 return qfalse;
}
