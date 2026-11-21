// Sandbox ownership and persistent state for the native MP emplaced gun.
// The operator and transient gun events are never stored. GPL-2.0-only.
static void EmplacedSyncPose(gentity_t *gun) {
 if (!JAR_MP_EmplacedState(&gun->s)) return;
 VectorCopy(gun->r.currentOrigin, gun->s.origin);
 VectorCopy(gun->r.currentAngles, gun->s.angles);
 VectorCopy(gun->r.currentAngles, gun->pos1);
}

qboolean JAR_MP_EmplacedAllowsSeat(gentity_t *gun, gentity_t *user) {
 const int n = gun->s.number, c = user->s.number;
 if (c < 0 || c >= MAX_CLIENTS || n < MAX_CLIENTS || n >= ENTITYNUM_WORLD ||
     !JAR_MP_EmplacedState(&gun->s) || gun->health <= 0 || gun->genericValue4 ||
     !sandbox.integer || !ActorAlive(user) || JAR_MP_ActorControlled(user) ||
     !Edit(user, n) || !gun->r.linked || (gun->flags & FL_INACTIVE) ||
     objects[n].copyPending || objects[n].heldBy || !objects[n].frozen) return qfalse;
 return qtrue;
}

qboolean JAR_MP_EmplacedCanFire(const gentity_t *gun) {
 return gun->activator && JAR_MP_EmplacedAllowsSeat((gentity_t *)gun, gun->activator) && Room();
}

void JAR_MP_EmplacedChanged(gentity_t *gun) {
 if (!JAR_MP_EmplacedState(&gun->s)) return;
 ++objects[gun->s.number].propertyRevision;
 SelectionChanged(gun->s.number);
}

static void EmplacedUse(gentity_t *gun, gentity_t *other, gentity_t *activator) {
 if (activator && other == activator && JAR_MP_EmplacedAllowsSeat(gun, activator))
  JAR_MP_NativeEmplacedUse(gun, activator);
}

static void EmplacedEnabled(gentity_t *gun) {
 const qboolean ready = !(gun->flags & FL_INACTIVE) && gun->health > 0 && !gun->genericValue4;
 gun->takedamage = ready;
 if (ready) gun->r.svFlags |= SVF_PLAYER_USABLE;
 else {
  gun->r.svFlags &= ~SVF_PLAYER_USABLE;
  JAR_MP_NativeEmplacedDetach(gun);
 }
}

static qboolean EmplacedInit(gentity_t *gun, int kind) {
 if (kind == JAR_EWEB_CONTROLLER) {
  if (!JAR_MP_NativeEWebInit(gun)) return qfalse;
 } else JAR_MP_NativeEmplacedInit(gun);
 gun->use = EmplacedUse;
 return qtrue;
}

static void EmplacedState(int n, jarControllerState_t *state) {
 const gentity_t *gun = &g_entities[n];
 state->initial = gun->maxHealth;
 state->count = Com_Clampi(0, gun->maxHealth, gun->health);
 state->active = gun->genericValue4 == 1 ? 2 : gun->genericValue4 == 2 ? 0 : 1;
 state->speed = (int)gun->s.speed;
 if (JAR_MP_EWebState(&gun->s)) state->pushSpeed = gun->count;
 state->inactive = !!(gun->flags & FL_INACTIVE);
}

static qboolean EmplacedApply(int n, const jarControllerState_t *state) {
 const int kind = ControllerKind(n);
 if (!JAR_ControllerStateValid(kind, state)) return qfalse;
 gentity_t *gun = &g_entities[n];
 const int phase = state->active == 2 ? 1 : state->active == 1 ? 0 : 2;
 if (gun->genericValue4 != phase) {
  gun->s.time = phase == 1 ? level.time + 1 : phase == 2 ? -1 : 0;
  gun->genericValue2 = gun->genericValue3 = gun->genericValue5 = 0;
 }
 gun->genericValue4 = phase;
 gun->maxHealth = state->initial;
 gun->health = state->count;
 G_ScaleNetHealth(gun);
 gun->s.speed = state->speed;
 if (kind == JAR_EWEB_CONTROLLER) gun->s.otherEntityNum2 = gun->count = state->pushSpeed;
 if (state->inactive) gun->flags |= FL_INACTIVE;
 else gun->flags &= ~FL_INACTIVE;
 EmplacedEnabled(gun);
 return qtrue;
}
