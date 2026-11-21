// Portable stock gun and ammo racks with native one-shot pickups.
// Shelf membership is private server state. GPL-2.0-only.
typedef struct {
 const char *name;
 itemType_t type;
 int tag, amount;
 float right, up;
} jarRackSlot_t;
static const jarRackSlot_t gunRackSlots[JAR_MP_GUN_RACK_SLOTS] = {
 {"weapon_blaster", IT_WEAPON, WP_BLASTER, 15, -9, 23},
 {"weapon_repeater", IT_WEAPON, WP_REPEATER, 100, 0, 24.5f},
 {"weapon_rocket_launcher", IT_WEAPON, WP_ROCKET_LAUNCHER, 4, 9, 25.5f}
};
static const jarRackSlot_t ammoRackSlots[JAR_MP_AMMO_RACK_SLOTS] = {
 {"ammo_blaster", IT_AMMO, AMMO_BLASTER, 25, -8, 7},
 {"ammo_metallic_bolts", IT_AMMO, AMMO_METAL_BOLTS, 100, 0, 7},
 {"ammo_powercell", IT_AMMO, AMMO_POWERCELL, 100, 8, 7},
 {"weapon_blaster", IT_WEAPON, WP_BLASTER, 15, 7, 25.5f},
 {"item_medpak_instant", IT_HEALTH, 0, 20, -6, 24}
};

typedef struct {
 qboolean live, positioned;
 unsigned epoch;
 int kind, consumed, item[JAR_MP_AMMO_RACK_SLOTS];
 vec3_t origin, angles;
} jarRack_t;
typedef struct {
 int parent, slot;
 unsigned epoch;
 qboolean loose;
} jarRackChild_t;
static jarRack_t racks[MAX_GENTITIES];
static jarRackChild_t rackChildren[MAX_GENTITIES];

static jarRack_t *RackRoot(int n) {
 if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !racks[n].live || !g_entities[n].inuse ||
     racks[n].epoch != controllerEntityEpoch[n]) return NULL;
 return &racks[n];
}

int JAR_MP_RackParent(const gentity_t *item) {
 if (!item || !item->inuse || item->s.eType != ET_ITEM || item->s.generic1 != JAR_MP_RACK_ITEM_TAG ||
     item->s.number < MAX_CLIENTS || item->s.number >= ENTITYNUM_WORLD) return ENTITYNUM_NONE;
 const int n = item->s.number;
 const jarRackChild_t *child = &rackChildren[n];
 const int parent = child->parent - 1;
 const jarRack_t *rack = RackRoot(parent);
 if (!rack || child->slot < 0 || child->slot >= JAR_MP_RackSlotCount(rack->kind) ||
     child->epoch != controllerEntityEpoch[n] || rack->item[child->slot] != n + 1) return ENTITYNUM_NONE;
 return parent;
}

static void RackTake(int parent, int child) {
 const int slot = rackChildren[child].slot;
 racks[parent].item[slot] = 0;
 racks[parent].consumed |= 1 << slot;
 memset(&rackChildren[child], 0, sizeof(rackChildren[child]));
 ++objects[parent].propertyRevision;
 SelectionChanged(parent);
}

qboolean JAR_MP_RackPickup(gentity_t *item) {
 if (!item) return qfalse;
 const int parent = JAR_MP_RackParent(item);
 if (parent != ENTITYNUM_NONE) {
  RackTake(parent, item->s.number);
  return qtrue;
 }
 const int n = item->s.number;
 return n >= MAX_CLIENTS && n < ENTITYNUM_WORLD && rackChildren[n].loose &&
        rackChildren[n].epoch == controllerEntityEpoch[n];
}

static void RackReset(void) {
 memset(racks, 0, sizeof(racks));
 memset(rackChildren, 0, sizeof(rackChildren));
}

static void RackForget(int n) {
 const int parent = JAR_MP_RackParent(&g_entities[n]);
 if (parent != ENTITYNUM_NONE) RackTake(parent, n);
 memset(&rackChildren[n], 0, sizeof(rackChildren[n]));
 jarRack_t *rack = RackRoot(n);
 if (!rack) return;
 for (int slot = 0; slot < JAR_MP_RackSlotCount(rack->kind); ++slot) {
  const int child = rack->item[slot] - 1;
  if (child < MAX_CLIENTS || child >= ENTITYNUM_WORLD || JAR_MP_RackParent(&g_entities[child]) != n) continue;
  memset(&rackChildren[child], 0, sizeof(rackChildren[child]));
  G_FreeEntity(&g_entities[child]);
 }
 memset(rack, 0, sizeof(*rack));
}

static void RackState(int n, jarControllerState_t *state) {
 const jarRack_t *rack = RackRoot(n);
 if (rack) state->flags = rack->consumed;
}

static void RackAngles(matrix3_t axis, vec3_t angles) {
 const float horizontal = sqrtf(axis[0][0] * axis[0][0] + axis[0][1] * axis[0][1]);
 const float degrees = 180.0f / M_PI;
 angles[PITCH] = atan2f(-axis[0][2], horizontal) * degrees;
 if (horizontal > .0001f) {
  angles[YAW] = atan2f(axis[0][1], axis[0][0]) * degrees;
  angles[ROLL] = atan2f(axis[1][2], axis[2][2]) * degrees;
 } else {
  angles[YAW] = atan2f(-axis[1][0], axis[1][1]) * degrees;
  angles[ROLL] = 0;
 }
}

static void RackSync(int n) {
 jarRack_t *rack = RackRoot(n);
 if (!rack) return;
 const gentity_t *root = &g_entities[n];
 if (rack->positioned && VectorCompare(rack->origin, root->r.currentOrigin) &&
     VectorCompare(rack->angles, root->r.currentAngles)) return;
 VectorCopy(root->r.currentOrigin, rack->origin);
 VectorCopy(root->r.currentAngles, rack->angles);
 rack->positioned = qtrue;

 // Use the centers of Raven's stock jitter ranges. The shelf state then
 // restores the same pose with only the consumed bits in the journal.
 matrix3_t rootAxis, localAxis, itemAxis;
 vec3_t itemAngles[2];
 const vec3_t localAngles[2] = {{-75, 180, 0}, {0, 90, 0}};
 const qboolean ammoRack = rack->kind == JAR_AMMO_RACK_CONTROLLER;
 const jarRackSlot_t *slots = ammoRack ? ammoRackSlots : gunRackSlots;
 AnglesToAxis(rack->angles, rootAxis);
 for (int pose = 0; pose < (ammoRack ? 2 : 1); ++pose) {
  AnglesToAxis(localAngles[pose], localAxis);
  MatrixMultiply(localAxis, rootAxis, itemAxis);
  RackAngles(itemAxis, itemAngles[pose]);
 }
 for (int slot = 0; slot < JAR_MP_RackSlotCount(rack->kind); ++slot) {
  const int child = rack->item[slot] - 1;
  if (child < MAX_CLIENTS || child >= ENTITYNUM_WORLD || JAR_MP_RackParent(&g_entities[child]) != n) continue;
  gentity_t *item = &g_entities[child];
  vec3_t origin;
  // AnglesToAxis's second row is left, while stock shelf offsets use right.
  VectorMA(rack->origin, -slots[slot].right, rootAxis[1], origin);
  VectorMA(origin, slots[slot].up, rootAxis[2], origin);
  G_SetOrigin(item, origin);
  VectorCopy(origin, item->s.origin);
  item->s.pos.trType = TR_INTERPOLATE;
  item->s.apos.trType = TR_INTERPOLATE;
  const float *angles = itemAngles[slots[slot].type != IT_WEAPON];
  VectorCopy(angles, item->s.angles);
  VectorCopy(angles, item->s.apos.trBase);
  VectorCopy(angles, item->r.currentAngles);
  trap->LinkEntity((sharedEntity_t *)item);
 }
}

static gitem_t *RackStockItem(const jarRackSlot_t *stock) {
 gitem_t *item = BG_FindItem(stock->name);
 const qboolean duel = level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL;
 const int disabled = duel ? g_duelWeaponDisable.integer : g_weaponDisable.integer;
 if (!item || item->giType != stock->type || item->giTag != stock->tag || G_ItemDisabled(item)) return NULL;
 if (level.gametype != GT_JEDIMASTER && ((stock->type == IT_WEAPON && (disabled & (1u << stock->tag))) ||
     (stock->type == IT_AMMO && HasSetSaberOnly()))) return NULL;
 if (stock->type == IT_HEALTH && duel) return NULL;
 const char *model = stock->type == IT_HEALTH ? JAR_MP_CARGO_DROP_MODEL : item->world_model[0];
 if (!model || !*model) return NULL;
 fileHandle_t file = 0;
 const int size = trap->FS_Open(model, &file, FS_READ);
 if (file) trap->FS_Close(file);
 return file && size > 0 ? item : NULL;
}

static qboolean RackInit(gentity_t *root, int consumedMask) {
 const int n = root->s.number, kind = ControllerKind(n), count = JAR_MP_RackSlotCount(kind);
 if (!count || (consumedMask & ~((1 << count) - 1))) return qfalse;
 const int remaining = JAR_MP_RackSlotsRemaining(kind, consumedMask);
 if (remaining && !RoomExtra(remaining)) return qfalse;
 const jarRackSlot_t *slots = kind == JAR_AMMO_RACK_CONTROLLER ? ammoRackSlots : gunRackSlots;
 gitem_t *goods[JAR_MP_AMMO_RACK_SLOTS] = {0};
 gentity_t *items[JAR_MP_AMMO_RACK_SLOTS] = {0};
 qboolean hasMedpak = qfalse;
 for (int slot = 0; slot < count; ++slot) {
  if (consumedMask & (1 << slot)) continue;
  goods[slot] = RackStockItem(&slots[slot]);
  if (!goods[slot]) return qfalse;
  if (slots[slot].type == IT_HEALTH) hasMedpak = qtrue;
 }
 if (hasMedpak && !ModelRoom(JAR_MP_CARGO_DROP_MODEL)) return qfalse;
 // Allocate every required slot before linking or registering any item.
 for (int slot = 0; slot < count; ++slot) {
  if (!goods[slot]) continue;
  items[slot] = G_TrySpawn();
  if (!items[slot]) goto fail;
 }
 const int medpakModel = hasMedpak ? G_ModelIndex(JAR_MP_CARGO_DROP_MODEL) : 0;
 if (hasMedpak && !medpakModel) goto fail;
 jarRack_t *rack = &racks[n];
 memset(rack, 0, sizeof(*rack));
 rack->live = qtrue;
 rack->epoch = controllerEntityEpoch[n];
 rack->kind = kind;
 rack->consumed = consumedMask;
 for (int slot = 0; slot < count; ++slot) {
  gentity_t *item = items[slot];
  if (!item) continue;
  const qboolean weapon = slots[slot].type == IT_WEAPON, medpak = slots[slot].type == IT_HEALTH;
  const int child = item->s.number;
  rack->item[slot] = child + 1;
  rackChildren[child] = (jarRackChild_t){n + 1, slot, controllerEntityEpoch[child], qfalse};
  item->item = goods[slot];
  item->classname = item->item->classname;
  item->count = slots[slot].amount;
  item->flags = FL_DROPPED_ITEM;
  item->s.eType = ET_ITEM;
  item->s.modelindex = item->item - bg_itemlist;
  item->s.modelindex2 = medpak ? medpakModel : 1;
  if (weapon) item->s.eFlags |= EF_DROPPEDWEAPON;
  item->s.generic1 = JAR_MP_RACK_ITEM_TAG;
  item->s.otherEntityNum = n;
  item->s.groundEntityNum = ENTITYNUM_WORLD;
  item->r.contents = CONTENTS_TRIGGER;
  item->touch = Touch_Item;
  item->physicsBounce = .1f;
  if (weapon) {
   VectorSet(item->r.mins, -10, -10, -2);
   VectorSet(item->r.maxs, 10, 10, 12);
  } else {
   VectorSet(item->r.mins, -8, -8, medpak ? -3 : 0);
   VectorSet(item->r.maxs, 8, 8, 16);
  }
  RegisterItem(item->item);
 }
 if (remaining) SaveRegisteredItems();
 RackSync(n);
 return qtrue;

fail:
 for (int slot = 0; slot < count; ++slot) if (items[slot]) G_FreeEntity(items[slot]);
 return qfalse;
}

static qboolean RackEditable(gentity_t *caster, int parent) {
 return sandbox.integer && Alive(caster) && !JAR_MP_ActorControlled(caster) && Edit(caster, parent);
}

int JAR_MP_RackAimItem(gentity_t *caster, int parent, const vec3_t from, const vec3_t end) {
 const jarRack_t *rack = RackRoot(parent);
 if (!rack || !RackEditable(caster, parent)) return ENTITYNUM_NONE;
 vec3_t direction;
 VectorSubtract(end, from, direction);
 const float length = VectorNormalize(direction);
 int selected = ENTITYNUM_NONE;
 float best = 16 * 16;
 for (int slot = 0; slot < JAR_MP_RackSlotCount(rack->kind); ++slot) {
  const int n = rack->item[slot] - 1;
  if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || JAR_MP_RackParent(&g_entities[n]) != parent) continue;
  vec3_t delta, nearest;
  VectorSubtract(g_entities[n].r.currentOrigin, from, delta);
  const float distance = DotProduct(delta, direction);
  if (distance < 0 || distance > length) continue;
  VectorMA(delta, -distance, direction, nearest);
  const float miss = VectorLengthSquared(nearest);
  if (miss < best) { best = miss; selected = n; }
 }
 return selected;
}

qboolean JAR_MP_RackPullLine(const gentity_t *item, const trace_t *hit, const vec3_t end) {
 const int parent = JAR_MP_RackParent(item);
 if (parent == ENTITYNUM_NONE || hit->entityNum != parent || hit->startsolid || hit->allsolid) return qfalse;
 // The root's broad collision hull surrounds its shelf. Bypass only that
 // hull, continuing the original line to check for other scene geometry.
 trace_t remainder;
 trap->Trace(&remainder, hit->endpos, NULL, NULL, end, parent, MASK_SHOT, qfalse, 0, 0);
 return !remainder.startsolid && !remainder.allsolid &&
        (remainder.fraction >= 1 || remainder.entityNum == item->s.number);
}

static qboolean RackPullExit(const gentity_t *item, const gentity_t *root, const vec3_t direction, vec3_t origin) {
 // The new native item would start inside its parent hull. Clear that hull
 // before applying gravity, with a swept check that still hits scenery.
 VectorCopy(item->r.currentOrigin, origin);
 float distance = 128;
 for (int a = 0; a < 3; ++a) {
  const float low = root->r.currentOrigin[a] + root->r.mins[a] - item->r.maxs[a];
  const float high = root->r.currentOrigin[a] + root->r.maxs[a] - item->r.mins[a];
  if (origin[a] < low || origin[a] > high) return qtrue;
  if (direction[a] > .0001f) distance = fminf(distance, (high - origin[a]) / direction[a]);
  else if (direction[a] < -.0001f) distance = fminf(distance, (low - origin[a]) / direction[a]);
 }
 if (distance >= 128) return qfalse;
 vec3_t end;
 VectorMA(origin, distance + .25f, direction, end);
 trace_t sweep;
 trap->Trace(&sweep, origin, item->r.mins, item->r.maxs, end, root->s.number,
             MASK_PLAYERSOLID & ~CONTENTS_BODY, qfalse, 0, 0);
 if (sweep.startsolid || sweep.allsolid || sweep.fraction < 1) return qfalse;
 VectorCopy(end, origin);
 return qtrue;
}

qboolean JAR_MP_RackPull(gentity_t *caster, gentity_t *item) {
 const int parent = JAR_MP_RackParent(item);
 if (parent == ENTITYNUM_NONE || !RackEditable(caster, parent)) return qfalse;
 const int n = item->s.number, owner = objects[parent].owner;
 const unsigned group = objects[parent].order;
 vec3_t aim, direction, origin;
 VectorCopy(caster->client->ps.origin, aim);
 aim[2] += caster->r.maxs[2] / 3;
 VectorSubtract(aim, item->r.currentOrigin, direction);
 const float knockback = fminf(VectorNormalize(direction), 200);
 if (knockback < .01f || !RackPullExit(item, &g_entities[parent], direction, origin)) return qfalse;

 RackTake(parent, n);
 rackChildren[n].loose = qtrue;
 rackChildren[n].epoch = controllerEntityEpoch[n];
 item->s.generic1 = item->item->giType == IT_HEALTH ? JAR_MP_CARGO_DROP_TAG : 0;
 item->s.otherEntityNum = ENTITYNUM_NONE;
 item->s.eFlags ^= EF_TELEPORT_BIT;
 item->s.groundEntityNum = ENTITYNUM_NONE;
 G_SetOrigin(item, origin);
 VectorCopy(origin, item->s.origin);
 item->s.pos.trType = TR_GRAVITY;
 item->s.pos.trTime = level.time;
 item->s.apos.trType = TR_STATIONARY;
 VectorSet(item->s.angles, 0, item->s.angles[YAW], item->item->giType == IT_WEAPON ? -90 : 0);
 VectorCopy(item->s.angles, item->s.apos.trBase);
 VectorCopy(item->s.angles, item->r.currentAngles);
 VectorScale(direction, g_knockback.value * knockback / 200 * (g_gravity.value > 0 ? .8f : 1), item->s.pos.trDelta);
 if (g_gravity.value > 0)
  item->s.pos.trDelta[2] = fmaxf(knockback, direction[2] * g_knockback.value * knockback / 200 * 1.5f);
 item->physicsObject = qtrue;
 item->think = G_FreeEntity;
 item->nextthink = level.time + 30000;
 TrackTemporaryPickup(item, owner, group);
 trap->LinkEntity((sharedEntity_t *)item);
 return qtrue;
}
