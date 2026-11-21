// Portable crystal crate. The attached crystal has no independent gameplay state.
// GPL-2.0-only.
typedef struct {
 int part;
 unsigned epoch;
} crystalCrateLink_t;

typedef struct {
 int slot;
 unsigned epoch;
 qboolean terminal;
} crystalCrateWork_t;

static crystalCrateLink_t crystalCrateParts[MAX_GENTITIES];
static crystalCrateLink_t crystalCrateParents[MAX_GENTITIES];
static crystalCrateWork_t crystalCrateWork[2 * MAX_GENTITIES];
static int crystalCrateWorkCount;

static void CrystalCrateReset(void) {
 memset(crystalCrateParts, 0, sizeof(crystalCrateParts));
 memset(crystalCrateParents, 0, sizeof(crystalCrateParents));
 crystalCrateWorkCount = 0;
}

static qboolean CrystalCrateLive(int n, unsigned epoch) {
 return n >= MAX_CLIENTS && n < ENTITYNUM_WORLD && g_entities[n].inuse &&
  controllerEntityEpoch[n] == epoch && ControllerKind(n) == JAR_CRYSTAL_CRATE_CONTROLLER;
}

static gentity_t *CrystalCratePart(int n) {
 const crystalCrateLink_t link = crystalCrateParts[n];
 const int part = link.part - 1;
 if (part < MAX_CLIENTS || part >= ENTITYNUM_WORLD || !g_entities[part].inuse ||
     controllerEntityEpoch[part] != link.epoch || crystalCrateParents[part].part != n + 1 ||
     crystalCrateParents[part].epoch != controllerEntityEpoch[n]) return NULL;
 return &g_entities[part];
}

static void CrystalCrateForget(int n) {
 const crystalCrateLink_t link = crystalCrateParents[n];
 const int parent = link.part - 1;
 if (CrystalCrateLive(parent, link.epoch) && CrystalCratePart(parent) == &g_entities[n])
  crystalCrateParts[parent] = (crystalCrateLink_t){0};
 crystalCrateParents[n] = (crystalCrateLink_t){0};

 gentity_t *part = CrystalCratePart(n);
 crystalCrateParts[n] = (crystalCrateLink_t){0};
 if (part) {
  crystalCrateParents[part->s.number] = (crystalCrateLink_t){0};
  G_FreeEntity(part);
 }
}

static void CrystalCrateSync(int n) {
 gentity_t *part = CrystalCratePart(n);
 if (!part) return;

 const gentity_t *root = &g_entities[n];
 const int flags = root->s.eFlags & (EF_TELEPORT_BIT | EF_NODRAW);
 const int identity = (int)PhysicsIdentity(n);
 vec3_t point, up;
 BreakLocalUp(root, JAR_MP_CRYSTAL_CRATE_PART_HEIGHT, point, up);
 if (part->r.linked && part->s.eFlags == flags && part->s.time2 == identity &&
     VectorCompare(part->r.currentOrigin, point) && VectorCompare(part->r.currentAngles, root->r.currentAngles)) return;

 part->s.eFlags = flags;
 part->s.time2 = identity;
 if (flags & EF_NODRAW) part->r.svFlags |= SVF_NOCLIENT;
 else part->r.svFlags &= ~SVF_NOCLIENT;
 G_SetOrigin(part, point);
 VectorCopy(point, part->s.origin);
 VectorCopy(root->r.currentAngles, part->r.currentAngles);
 VectorCopy(root->r.currentAngles, part->s.angles);
 VectorCopy(root->r.currentAngles, part->s.apos.trBase);
 part->s.pos.trType = part->s.apos.trType = TR_INTERPOLATE;
 trap->LinkEntity((sharedEntity_t *)part);
}

static void CrystalCrateState(int n, jarControllerState_t *state) {
 const gentity_t *root = &g_entities[n];
 state->count = root->health;
 state->initial = root->maxHealth;
 state->active = root->genericValue11 == 1;
 state->inactive = (root->flags & FL_INACTIVE) != 0;
 state->flags = root->spawnflags;
 state->damage = root->splashDamage;
 state->speed = root->splashRadius;
 state->repeats = root->damage;
 state->pushSpeed = root->genericValue12;
}

static qboolean CrystalCrateApply(int n, const jarControllerState_t *state) {
 gentity_t *root = &g_entities[n];
 if (root->genericValue11 != 1 || !JAR_ControllerStateValid(JAR_CRYSTAL_CRATE_CONTROLLER, state)) return qfalse;

 const int contents = state->flags & JAR_MP_CRYSTAL_CRATE_SHOT_ONLY ? CONTENTS_CORPSE : JAR_MP_CRYSTAL_CRATE_SOLID_CONTENTS;
 if (root->r.contents != contents) {
  if (contents != CONTENTS_CORPSE) {
   trace_t trace;
   trap->Trace(&trace, root->r.currentOrigin, root->r.mins, root->r.maxs, root->r.currentOrigin,
    n, MASK_PLAYERSOLID, qfalse, 0, 0);
   if (trace.startsolid || trace.allsolid) return qfalse;
  }
  if (objects[n].heldBy) Drop(objects[n].heldBy - 1, qfalse);
  PhysicsForget(n);
 }

 root->r.contents = contents;
 root->clipmask = PhysicsMoveMask(root);
 if (state->inactive) root->flags |= FL_INACTIVE;
 else root->flags &= ~FL_INACTIVE;
 root->spawnflags = state->flags;
 root->health = state->count;
 root->maxHealth = state->initial;
 root->splashDamage = state->damage;
 root->splashRadius = state->speed;
 root->damage = state->repeats;
 root->genericValue12 = state->pushSpeed;
 G_ScaleNetHealth(root);
 PhysicsPublish(root);
 trap->LinkEntity((sharedEntity_t *)root);
 return qtrue;
}

static void CrystalCrateFinish(gentity_t *root) {
 const int n = root->s.number;
 const unsigned epoch = controllerEntityEpoch[n];
 const int damage = root->splashDamage, radius = root->splashRadius;
 const vec3_t direction = {0, 0, 1};
 vec3_t point, up;
 BreakLocalUp(root, JAR_MP_CRYSTAL_CRATE_BLAST_HEIGHT, point, up);

 root->think = NULL;
 root->nextthink = 0;
 SandboxDirectedEffect(root->genericValue14, point, direction);
 SandboxDirectedEffect(root->genericValue15, point, direction);
 gentity_t *sound = BreakEvent(root->r.currentOrigin, EV_GENERAL_SOUND);
 if (sound) {
  sound->s.eventParm = root->noise_index;
  sound->s.saberEntityNum = CHAN_AUTO;
 }

 // Primary can remove this crate before its native blast runs.
 G_UseTargets(root, ControllerActivator(root));
 if (CrystalCrateLive(n, epoch) && damage)
  G_RadiusDamage(point, root, damage, radius, root, NULL, MOD_UNKNOWN);
 if (CrystalCrateLive(n, epoch)) G_FreeEntity(root);
}

static void CrystalCrateQueue(gentity_t *root, qboolean terminal);

static void CrystalCrateDeferredFinish(gentity_t *root) {
 CrystalCrateQueue(root, qtrue);
}

static void CrystalCrateQueue(gentity_t *root, qboolean terminal) {
 const int n = root->s.number;
 if (crystalCrateWorkCount >= ARRAY_LEN(crystalCrateWork)) {
  // Reentrant callbacks can recycle slots. Preserve terminal work across
  // frames even if more than one arena of generations is reached at once.
  if (terminal) {
   root->think = CrystalCrateDeferredFinish;
   root->nextthink = level.time + 1;
  }
  return;
 }
 crystalCrateWork[crystalCrateWorkCount++] = (crystalCrateWork_t){n, controllerEntityEpoch[n], terminal};
 if (crystalCrateWorkCount != 1) return;

 // Radius damage and linked outputs can enqueue other crates synchronously.
 // Drain those jobs without nesting another native damage stack for each one.
 for (int i = 0; i < crystalCrateWorkCount; ++i) {
  const crystalCrateWork_t job = crystalCrateWork[i];
  if (!CrystalCrateLive(job.slot, job.epoch)) continue;
  gentity_t *source = &g_entities[job.slot];
  if (job.terminal) {
   if (!source->genericValue11) CrystalCrateFinish(source);
   continue;
  }
  if (source->genericValue11 != 1 || !BreakReady(source)) continue;
  const vec3_t direction = {0, 0, 1};
  vec3_t point, up;
  BreakLocalUp(source, JAR_MP_CRYSTAL_CRATE_HIT_HEIGHT, point, up);
  SandboxDirectedEffect(source->genericValue13, point, direction);
  if (source->damage)
   G_RadiusDamage(point, source, source->damage, (float)source->genericValue12 / JAR_MP_CRYSTAL_CRATE_MILLI, source, NULL, MOD_UNKNOWN);
 }
 crystalCrateWorkCount = 0;
}

static void CrystalCratePain(gentity_t *root, gentity_t *attacker, int damage) {
 if (!BreakReady(root) || root->genericValue11 != 1) return;
 ++objects[root->s.number].propertyRevision;
 SelectionChanged(root->s.number);
 if (root->painDebounceTime > level.time) return;
 root->painDebounceTime = level.time + JAR_MP_CRYSTAL_CRATE_PAIN_INTERVAL;
 CrystalCrateQueue(root, qfalse);
}

static void CrystalCrateDie(gentity_t *root, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
 if (!BreakReady(root) || root->genericValue11 != 1) return;
 const int n = root->s.number;
 ControllerCaptureActivator(root, attacker);
 root->health = 0;
 root->genericValue11 = 0;
 root->takedamage = qfalse;
 root->use = NULL;
 root->pain = NULL;
 root->die = NULL;
 G_ScaleNetHealth(root);
 BreakVisibility(root, qfalse);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
 CrystalCrateQueue(root, qtrue);
}

static void CrystalCrateUse(gentity_t *root, gentity_t *other, gentity_t *activator) {
 const int n = root->s.number;
 if (other == activator && activator && activator->client) {
  if (activator->s.number < MAX_CLIENTS) {
   if (!Edit(activator, n) || (!controllerCalling[n] && !Alive(activator))) return;
  } else if (!ActionActivator(root, activator)) return;
 }
 CrystalCrateDie(root, other, activator, 0, MOD_UNKNOWN);
}

static qboolean CrystalCrateEffectsAvailable(const char *const *paths, int count) {
 int available = 0, found = 0;
 char value[MAX_QPATH];
 for (int i = 0; i < count; ++i) if (!MediaValidate(10, paths[i])) return qfalse;
 for (int i = 1; i < MAX_FX; ++i) {
  trap->GetConfigstring(CS_EFFECTS + i, value, sizeof(value));
  if (!*value) ++available;
  else for (int effect = 0; effect < count; ++effect)
   if (!strcmp(value, paths[effect])) found |= 1 << effect;
 }
 int needed = 0;
 for (int i = 0; i < count; ++i) if (!(found & (1 << i))) ++needed;
 return !needed || available >= 16 + needed;
}

static qboolean CrystalCrateInit(gentity_t *root) {
 static const char *const effects[] = {
  JAR_MP_CRYSTAL_CRATE_HIT_EFFECT, JAR_MP_CRYSTAL_CRATE_BLAST_EFFECT, JAR_MP_CRYSTAL_CRATE_DEBRIS_EFFECT
 };
 if (!RoomExtra(1) || !ModelRoom(JAR_MP_CRYSTAL_CRATE_PART_MODEL) ||
     !MediaValidate(9, JAR_MP_CRYSTAL_CRATE_SOUND) || !CrystalCrateEffectsAvailable(effects, ARRAY_LEN(effects))) return qfalse;

 fileHandle_t handle = 0;
 const int length = trap->FS_Open(JAR_MP_CRYSTAL_CRATE_PART_MODEL, &handle, FS_READ);
 if (handle) trap->FS_Close(handle);
 if (!handle || length <= 0) return qfalse;

 gentity_t *part = G_TrySpawn();
 if (!part) return qfalse;
 part->s.modelindex = G_ModelIndex(JAR_MP_CRYSTAL_CRATE_PART_MODEL);
 root->genericValue13 = G_EffectIndex(effects[0]);
 root->genericValue14 = G_EffectIndex(effects[1]);
 root->genericValue15 = G_EffectIndex(effects[2]);
 root->noise_index = G_SoundIndex(JAR_MP_CRYSTAL_CRATE_SOUND);
 if (!part->s.modelindex || !root->genericValue13 || !root->genericValue14 || !root->genericValue15 || !root->noise_index) {
  G_FreeEntity(part);
  return qfalse;
 }

 const int n = root->s.number, child = part->s.number;
 part->classname = "jar_crystal_crate_part";
 part->s.eType = ET_GENERAL;
 part->s.generic1 = JAR_MP_CRYSTAL_CRATE_PART_TAG;
 part->s.otherEntityNum = n;
 crystalCrateParts[n] = (crystalCrateLink_t){child + 1, controllerEntityEpoch[child]};
 crystalCrateParents[child] = (crystalCrateLink_t){n + 1, controllerEntityEpoch[n]};
 VectorSet(part->r.mins, -7, -7, -13);
 VectorSet(part->r.maxs, 7, 7, 13);

 root->genericValue11 = 1;
 root->material = MAT_CRATE2;
 root->takedamage = qtrue;
 root->r.svFlags |= SVF_PLAYER_USABLE;
 root->pain = CrystalCratePain;
 root->die = CrystalCrateDie;
 root->use = CrystalCrateUse;
 root->painDebounceTime = 0;
 objects[n].mass = JAR_MP_CRYSTAL_CRATE_DEFAULT_MASS;

 jarControllerState_t defaults = {0};
 defaults.count = defaults.initial = JAR_MP_CRYSTAL_CRATE_DEFAULT_HEALTH;
 defaults.active = 1;
 defaults.flags = root->r.contents == CONTENTS_CORPSE ? JAR_MP_CRYSTAL_CRATE_SHOT_ONLY : 0;
 defaults.damage = JAR_MP_CRYSTAL_CRATE_DEFAULT_BLAST_DAMAGE;
 defaults.speed = JAR_MP_CRYSTAL_CRATE_DEFAULT_BLAST_RADIUS;
 defaults.repeats = JAR_MP_CRYSTAL_CRATE_DEFAULT_HIT_DAMAGE;
 defaults.pushSpeed = JAR_MP_CRYSTAL_CRATE_DEFAULT_HIT_RADIUS;
 if (CrystalCrateApply(n, &defaults)) return qtrue;
 CrystalCrateForget(n);
 return qfalse;
}
