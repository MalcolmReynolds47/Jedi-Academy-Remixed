// Stock welder assets, snapshot identity and portable controller settings.
// GPL-2.0-only.
#pragma once

#define JAR_MP_WELDER_ID 89954407u
#define JAR_MP_WELDER_MODEL "models/weapons2/demp2/demp2.md3"
#define JAR_MP_WELDER_EFFECT "sparks/spark_nosnd"
#define JAR_MP_WELDER_LOOP "sound/effects/electric_beam_lp.wav"
#define JAR_MP_WELDER_HULL 16.0f
#define JAR_MP_WELDER_LEFT_OFFSET (-0.7046813f)
#define JAR_MP_WELDER_UP_OFFSET (-3.21862f)

enum {
 JAR_MP_WELDER_TAG = 7, JAR_MP_WELDER_MILLI = 1000,
 JAR_MP_WELDER_MIN_INTERVAL = 100, JAR_MP_WELDER_MAX_INTERVAL = 10000,
 JAR_MP_WELDER_MIN_REACH = 1000, JAR_MP_WELDER_MAX_REACH = 256000,
 JAR_MP_WELDER_MIN_RADIUS = 1000, JAR_MP_WELDER_MAX_RADIUS = 256000,
 JAR_MP_WELDER_MAX_DAMAGE = 1000,
 JAR_MP_WELDER_DEFAULT_INTERVAL = 200, JAR_MP_WELDER_DEFAULT_REACH = 15000,
 JAR_MP_WELDER_DEFAULT_RADIUS = 45000, JAR_MP_WELDER_DEFAULT_DAMAGE = 10
};

static inline qboolean JAR_MP_WelderRootState(const entityState_t *state) {
 return state->eType == ET_GENERAL && state->generic1 == 250 && state->bolt2 == JAR_MP_WELDER_TAG;
}
