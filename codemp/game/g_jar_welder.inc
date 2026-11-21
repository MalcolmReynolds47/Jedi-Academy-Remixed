// Portable DEMP2 welder with stock sparks and native radial damage.
// GPL-2.0-only.
static qboolean WelderEmitting(const gentity_t *root) {
 return BreakReady(root) && root->genericValue11;
}

static void WelderSetEnabled(gentity_t *root) {
 const qboolean usable = BreakReady(root);
 root->s.loopSound = usable && root->genericValue11 ? root->genericValue14 : 0;
 if (usable) root->r.svFlags |= SVF_PLAYER_USABLE;
 else root->r.svFlags &= ~SVF_PLAYER_USABLE;
}

static void WelderState(int n, jarControllerState_t *state) {
 const gentity_t *root = &g_entities[n];
 state->waitMs = root->delay;
 state->speed = root->genericValue12;
 state->pushSpeed = root->genericValue15;
 state->damage = root->damage;
 state->active = root->genericValue11;
 state->inactive = !!(root->flags & FL_INACTIVE);
}

static qboolean WelderApply(int n, const jarControllerState_t *state) {
 if (!JAR_ControllerStateValid(JAR_WELDER_CONTROLLER, state)) return qfalse;
 gentity_t *root = &g_entities[n];
 root->delay = state->waitMs;
 root->genericValue12 = state->speed;
 root->genericValue15 = state->pushSpeed;
 root->damage = state->damage;
 root->genericValue11 = state->active;
 if (state->inactive) root->flags |= FL_INACTIVE;
 else root->flags &= ~FL_INACTIVE;
 // Edits and toggles retain the current pulse deadline, even if the new
 // interval is shorter. The next actual pulse will start the new interval.
 WelderSetEnabled(root);
 return qtrue;
}

static int WelderRemaining(const gentity_t *root) {
 const int64_t remaining = (int64_t)root->genericValue8 - level.time;
 if (remaining <= 0) return 0;
 return remaining > INT_MAX ? INT_MAX : (int)remaining;
}

static void WelderRestoreDeadline(gentity_t *root, int remaining) {
 root->genericValue8 = JAR_NumberDeadline(level.time, remaining);
}

static void WelderUse(gentity_t *root, gentity_t *other, gentity_t *activator) {
 const int n = root->s.number;
 if (!BreakReady(root)) return;
 if (other == activator && activator && activator->client) {
  if (activator->s.number < MAX_CLIENTS) {
   if (!Edit(activator, n) || (!controllerCalling[n] && !Alive(activator))) return;
  } else if (!ActionActivator(root, activator)) return;
 }
 jarControllerState_t state = {0};
 WelderState(n, &state);
 state.active = !state.active;
 if (WelderApply(n, &state)) {
  ++objects[n].propertyRevision;
  SelectionChanged(n);
 }
}

static void WelderFrame(gentity_t *root) {
 WelderSetEnabled(root);
 if (!WelderEmitting(root) || root->genericValue8 > level.time ||
     root->genericValue9 == level.time) return;
 // Schedule before damage, and emit at most once after a slow frame. A
 // controller created in a recycled slot starts on the following frame.
 root->genericValue8 = JAR_NumberDeadline(level.time, root->delay);
 matrix3_t axis;
 vec3_t tip, direction;
 AnglesToAxis(root->r.currentAngles, axis);
 VectorMA(root->r.currentOrigin, (float)root->genericValue12 / JAR_MP_WELDER_MILLI, axis[0], tip);
 VectorMA(tip, JAR_MP_WELDER_LEFT_OFFSET, axis[1], tip);
 VectorMA(tip, JAR_MP_WELDER_UP_OFFSET, axis[2], tip);
 trace_t trace = {0};
 trap->Trace(&trace, root->r.currentOrigin, NULL, NULL, tip, root->s.number, MASK_SHOT, qfalse, 0, 0);
 if (trace.startsolid || trace.allsolid) return;
 VectorCopy(trace.endpos, tip);
 if (trace.fraction < 1) {
  VectorCopy(trace.plane.normal, direction);
  VectorMA(tip, .5f, direction, tip);
 } else VectorScale(axis[0], -1, direction);
 SandboxDirectedEffect(root->genericValue13, tip, direction);
 // Native falloff and line of sight apply with owned-source consent. A
 // damage callback may free or reuse the root, so do not access it afterward.
 if (root->damage)
  G_RadiusDamage(tip, root, root->damage, (float)root->genericValue15 / JAR_MP_WELDER_MILLI,
                 root, NULL, MOD_UNKNOWN);
}

static qboolean WelderInit(gentity_t *root) {
 if (!MediaValidate(10, JAR_MP_WELDER_EFFECT) || !MediaValidate(9, JAR_MP_WELDER_LOOP)) return qfalse;
 root->genericValue13 = G_EffectIndex(JAR_MP_WELDER_EFFECT);
 root->genericValue14 = G_SoundIndex(JAR_MP_WELDER_LOOP);
 if (!root->genericValue13 || !root->genericValue14) return qfalse;
 root->r.contents = CONTENTS_ITEM;
 root->takedamage = qfalse;
 root->use = WelderUse;
 root->s.bolt2 = JAR_MP_WELDER_TAG;
 root->genericValue9 = level.time;
 VectorSet(root->r.mins, -JAR_MP_WELDER_HULL, -JAR_MP_WELDER_HULL, -JAR_MP_WELDER_HULL);
 VectorSet(root->r.maxs, JAR_MP_WELDER_HULL, JAR_MP_WELDER_HULL, JAR_MP_WELDER_HULL);
 jarControllerState_t state = {0};
 state.waitMs = JAR_MP_WELDER_DEFAULT_INTERVAL;
 state.speed = JAR_MP_WELDER_DEFAULT_REACH;
 state.pushSpeed = JAR_MP_WELDER_DEFAULT_RADIUS;
 state.damage = JAR_MP_WELDER_DEFAULT_DAMAGE;
 state.active = 1;
 return WelderApply(root->s.number, &state);
}
