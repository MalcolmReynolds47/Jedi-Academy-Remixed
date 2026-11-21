// Portable emplaced guns retain the prop hull in origin2/angles2.
// The tagged gun publishes its native yaw limit in speed. GPL-2.0-only.
#pragma once

enum { JAR_MP_CHAIR_TAG = 1, JAR_MP_EWEB_TAG = 2, JAR_MP_EWEB_DEFAULT_AMMO = 999,
       JAR_MP_EWEB_WIRE_AMMO_MAX = (1 << GENTITYNUM_BITS) - 1 };
#define JAR_MP_EWEB_SHOT_CLASS "jar_eweb_shot"

static inline qboolean JAR_MP_EmplacedTag(const entityState_t *state, int tag) {
 return state->number >= MAX_CLIENTS && state->number < ENTITYNUM_WORLD &&
        state->eType == ET_GENERAL && state->generic1 == 250 && state->bolt2 == tag;
}

static inline qboolean JAR_MP_ChairState(const entityState_t *state) {
 return JAR_MP_EmplacedTag(state, JAR_MP_CHAIR_TAG);
}

static inline qboolean JAR_MP_EWebState(const entityState_t *state) {
 return JAR_MP_EmplacedTag(state, JAR_MP_EWEB_TAG);
}

static inline qboolean JAR_MP_EmplacedState(const entityState_t *state) {
 return JAR_MP_ChairState(state) || JAR_MP_EWebState(state);
}

static inline int JAR_MP_EWebAmmo(const entityState_t *state) {
 return JAR_MP_EWebState(state) ? Com_Clampi(0, JAR_MP_EWEB_WIRE_AMMO_MAX, state->otherEntityNum2) : 0;
}

static inline float JAR_MP_EmplacedConstraint(const entityState_t *state) {
 return JAR_MP_EmplacedState(state) ? state->speed : state->origin2[0];
}
