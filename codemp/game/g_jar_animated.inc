// Portable stock bomb and beacon. The server publishes their current MD3
// frame, phase and sound so late joiners see the same state. GPL-2.0-only.
static const char animatedLoop[] = "sound/interface/ammocon_run.wav";
static const char animatedDisarm[] = "sound/weapons/overchargeend.wav";
static void AnimatedThink(gentity_t *e);

static qboolean AnimatedReady(const gentity_t *e) {
 return sandbox.integer && !(e->flags & FL_INACTIVE) && !objects[e->s.number].copyPending;
}

static int AnimatedRemaining(const gentity_t *e) {
 if (!JAR_MP_AnimatedFrameRuns(e->s.bolt2, e->genericValue11)) return 0;
 const int next = e->genericValue8;
 return Com_Clampi(1, JAR_MP_ANIM_TICK_MS, next < 0 ? -next : next - level.time);
}

static void AnimatedSetEnabled(gentity_t *e) {
 const int kind = ControllerKind(e->s.number), phase = e->genericValue11;
 const qboolean bomb = kind == JAR_BOMB_CONTROLLER;
 const qboolean ready = AnimatedReady(e);
 const qboolean looping = bomb ? phase == JAR_MP_BOMB_ARMED : phase == JAR_MP_BEACON_SIGNALLING;
 const qboolean usable = bomb ? phase == JAR_MP_BOMB_ARMED : phase != JAR_MP_BEACON_HIDDEN;
 e->s.loopSound = ready && looping ? e->genericValue13 : 0;
 if (ready && usable) e->r.svFlags |= SVF_PLAYER_USABLE;
 else e->r.svFlags &= ~SVF_PLAYER_USABLE;

 if (!JAR_MP_AnimatedFrameRuns(e->s.bolt2, phase)) {
  e->genericValue8 = e->nextthink = 0;
  e->think = NULL;
  return;
 }
 if (!e->genericValue8) e->genericValue8 = level.time + JAR_MP_ANIM_TICK_MS;
 if (ready && e->genericValue8 < 0) e->genericValue8 = level.time - e->genericValue8;
 if (!ready && e->genericValue8 > 0) e->genericValue8 = -AnimatedRemaining(e);
 e->think = AnimatedThink;
 e->nextthink = ready ? e->genericValue8 : 0;
}

static void AnimatedFrame(gentity_t *e, int frame, int remaining) {
 e->s.time = e->genericValue11;
 e->s.frame = frame;
 e->genericValue8 = remaining ? level.time + remaining : 0;
 AnimatedSetEnabled(e);
}

static void AnimatedChanged(int n) {
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void AnimatedThink(gentity_t *e) {
 const int n = e->s.number, kind = ControllerKind(n), oldPhase = e->genericValue11;
 const int elapsed = level.time - e->genericValue8;
 if (!AnimatedReady(e) || e->genericValue8 <= 0 || elapsed < 0) {
  AnimatedSetEnabled(e);
  return;
 }
 // Catch up after a long server frame without a per-frame loop or replaying
 // sound events. Retain the remaining part of the 50 ms step.
 int frame = e->s.frame + 1 + elapsed / JAR_MP_ANIM_TICK_MS;
 const int remaining = JAR_MP_ANIM_TICK_MS - elapsed % JAR_MP_ANIM_TICK_MS;
 if (kind == JAR_BOMB_CONTROLLER) {
  if (frame >= JAR_MP_BOMB_LAST_FRAME) {
   frame = JAR_MP_BOMB_LAST_FRAME;
   e->genericValue11 = JAR_MP_BOMB_DISARMED;
  }
 } else if (frame > JAR_MP_BEACON_DEPLOY_LAST) {
  const int loopLength = JAR_MP_BEACON_LOOP_LAST - JAR_MP_BEACON_LOOP_FIRST + 1;
  frame = JAR_MP_BEACON_LOOP_FIRST + (frame - JAR_MP_BEACON_LOOP_FIRST) % loopLength;
  e->genericValue11 = JAR_MP_BEACON_SIGNALLING;
 }
 AnimatedFrame(e, frame, remaining);
 if (oldPhase != e->genericValue11) AnimatedChanged(n);
}

static qboolean AnimatedRevealClear(const gentity_t *e) {
 trace_t trace;
 trap->Trace(&trace, e->r.currentOrigin, e->r.mins, e->r.maxs,
             e->r.currentOrigin, e->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
 return !trace.startsolid && !trace.allsolid;
}

static void AnimatedVisibility(gentity_t *e, qboolean visible, qboolean publish) {
 if (visible) {
  e->s.eFlags &= ~EF_NODRAW;
  e->r.svFlags &= ~SVF_NOCLIENT;
  e->r.contents = CONTENTS_SOLID;
  e->clipmask = MASK_PLAYERSOLID;
 } else {
  const int n = e->s.number;
  if (objects[n].heldBy) Drop(objects[n].heldBy - 1, qfalse);
  e->s.eFlags |= EF_NODRAW;
  e->r.svFlags |= SVF_NOCLIENT;
  e->s.solid = e->r.contents = e->clipmask = 0;
  VectorClear(objects[n].velocity);
  VectorClear(objects[n].angular);
  VectorClear(e->s.pos.trDelta);
  VectorClear(e->s.apos.trDelta);
 }
 if (publish) {
  PhysicsForget(e->s.number);
  PhysicsPublish(e);
  trap->LinkEntity((sharedEntity_t *)e);
 }
}

static void AnimatedUse(gentity_t *e, gentity_t *other, gentity_t *activator) {
 if (!AnimatedReady(e)) return;
 const int n = e->s.number, kind = ControllerKind(n);
 // Native USE bypasses the linked-controller graph and must check the user.
 if (other == activator && activator && activator->client) {
  if (activator->s.number < MAX_CLIENTS) {
   if (!Edit(activator, n) || (!controllerCalling[n] && !Alive(activator))) return;
  } else if (!ActionActivator(e, activator)) return;
 }

 if (kind == JAR_BOMB_CONTROLLER) {
  if (e->genericValue11 != JAR_MP_BOMB_ARMED) return;
  e->genericValue11 = JAR_MP_BOMB_DISARMING;
  AnimatedFrame(e, 0, JAR_MP_ANIM_TICK_MS);
  e->s.saberEntityNum = CHAN_AUTO;
  G_AddEvent(e, EV_GENERAL_SOUND, e->genericValue14);
  AnimatedChanged(n);
  ControllerCaptureActivator(e, activator);
  G_UseTargets(e, activator ? activator : e);
 } else if (kind == JAR_BEACON_CONTROLLER) {
  const qboolean visible = e->genericValue11 == JAR_MP_BEACON_HIDDEN;
  if (visible && !AnimatedRevealClear(e)) return;
  e->genericValue11 = visible ? JAR_MP_BEACON_DEPLOYING : JAR_MP_BEACON_HIDDEN;
  AnimatedFrame(e, 0, visible ? JAR_MP_ANIM_TICK_MS : 0);
  AnimatedVisibility(e, visible, qtrue);
  AnimatedChanged(n);
 }
}

static void AnimatedState(int n, jarControllerState_t *state) {
 const gentity_t *e = &g_entities[n];
 state->active = e->genericValue11;
 state->inactive = !!(e->flags & FL_INACTIVE);
}

static qboolean AnimatedApply(int n, const jarControllerState_t *state) {
 const int kind = ControllerKind(n);
 if (!JAR_ControllerStateValid(kind, state)) return qfalse;
 gentity_t *e = &g_entities[n];
 const qboolean wasVisible = !(e->s.eFlags & EF_NODRAW);
 const qboolean visible = kind != JAR_BEACON_CONTROLLER || state->active != JAR_MP_BEACON_HIDDEN;
 if (visible && !wasVisible && !AnimatedRevealClear(e)) return qfalse;
 if (state->inactive) e->flags |= FL_INACTIVE;
 else e->flags &= ~FL_INACTIVE;
 if (e->genericValue11 == state->active) {
  AnimatedSetEnabled(e);
 } else {
  e->genericValue11 = state->active;
  int frame = 0;
  if (kind == JAR_BOMB_CONTROLLER && state->active == JAR_MP_BOMB_DISARMED) frame = JAR_MP_BOMB_LAST_FRAME;
  if (kind == JAR_BEACON_CONTROLLER && state->active == JAR_MP_BEACON_SIGNALLING) frame = JAR_MP_BEACON_LOOP_FIRST;
  AnimatedFrame(e, frame, JAR_MP_ANIM_TICK_MS);
 }
 if (visible != wasVisible) AnimatedVisibility(e, visible, qtrue);
 return qtrue;
}

static qboolean AnimatedSounds(qboolean bomb) {
 if (!MediaValidate(9, animatedLoop) || (bomb && !MediaValidate(9, animatedDisarm))) return qfalse;
 int freeSlots = 0, needed = bomb ? 2 : 1;
 for (int i = 1; i < MAX_SOUNDS; ++i) {
  char value[MAX_QPATH];
  trap->GetConfigstring(CS_SOUNDS + i, value, sizeof(value));
  if (!*value) ++freeSlots;
  else if (!strcmp(value, animatedLoop) || (bomb && !strcmp(value, animatedDisarm))) --needed;
 }
 return needed <= 0 || freeSlots >= 64 + needed;
}

static qboolean AnimatedInit(gentity_t *e, int kind, qboolean hiddenBeacon) {
 const qboolean bomb = kind == JAR_BOMB_CONTROLLER;
 if ((!bomb && kind != JAR_BEACON_CONTROLLER) || (bomb && hiddenBeacon) || !AnimatedSounds(bomb)) return qfalse;
 e->genericValue13 = G_SoundIndex(animatedLoop);
 e->genericValue14 = bomb ? G_SoundIndex(animatedDisarm) : 0;
 if (!e->genericValue13 || (bomb && !e->genericValue14)) return qfalse;
 e->s.bolt2 = bomb ? JAR_MP_BOMB_TAG : JAR_MP_BEACON_TAG;
 e->takedamage = qfalse;
 e->use = AnimatedUse;
 if (bomb) e->genericValue11 = JAR_MP_BOMB_ARMED;
 else e->genericValue11 = hiddenBeacon ? JAR_MP_BEACON_HIDDEN : JAR_MP_BEACON_DEPLOYING;
 AnimatedVisibility(e, !hiddenBeacon, qfalse);
 AnimatedFrame(e, 0, JAR_MP_ANIM_TICK_MS);
 return qtrue;
}
