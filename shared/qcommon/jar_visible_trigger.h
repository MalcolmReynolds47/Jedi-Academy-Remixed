// Shared Sandbox visibility trigger settings and sight cone. GPL-2.0-only.
#pragma once
#include <math.h>

enum {
    JAR_VISIBLE_RANGE_MIN = 32, JAR_VISIBLE_RANGE_MAX = 4096, JAR_VISIBLE_RANGE_DEFAULT = 512,
    JAR_VISIBLE_CONE_MIN = 5, JAR_VISIBLE_CONE_MAX = 85, JAR_VISIBLE_CONE_DEFAULT = 45,
    JAR_VISIBLE_IGNORE_WALLS = 1, JAR_VISIBLE_FORCE_SIGHT = 2,
    JAR_VISIBLE_SAMPLE_MS = 100
};

typedef struct {
    int range, cone, flags;
} jarVisibleSettings_t;

static inline int JAR_VisibleSettingsValid(jarVisibleSettings_t settings)
{
    return settings.range >= JAR_VISIBLE_RANGE_MIN && settings.range <= JAR_VISIBLE_RANGE_MAX &&
           settings.cone >= JAR_VISIBLE_CONE_MIN && settings.cone <= JAR_VISIBLE_CONE_MAX &&
           !(settings.flags & ~(JAR_VISIBLE_IGNORE_WALLS | JAR_VISIBLE_FORCE_SIGHT));
}

static inline int JAR_VisibleFacing(const float *marker, const float *eye, const float *forward,
                                   int range, float cosine)
{
    const float x = marker[0] - eye[0], y = marker[1] - eye[1], z = marker[2] - eye[2];
    const float distanceSquared = x*x + y*y + z*z;
    const float projection = x*forward[0] + y*forward[1] + z*forward[2];
    return distanceSquared > 0 && distanceSquared <= (float)range*range && projection > 0 &&
           projection*projection >= distanceSquared*cosine*cosine;
}

static inline float JAR_VisibleCosine(int cone)
{
    return cosf(cone * (3.14159265358979323846f / 180.0f));
}
