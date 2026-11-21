// Oriented volume against an axis-aligned entity hull. GPL-2.0-only.
#pragma once
#include <math.h>

static inline qboolean JAR_BoxAxesContact(const vec3_t center, vec3_t axes[3],
    const vec3_t extent, const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
    vec3_t delta, half;
    for (int i = 0; i < 3; ++i) {
        delta[i] = origin[i] + .5f * (mins[i] + maxs[i]) - center[i];
        half[i] = .5f * (maxs[i] - mins[i]);
    }

    vec3_t tests[15];
    int count = 0;
    for (int world = 0; world < 3; ++world) {
        vec3_t unit = {0, 0, 0};
        unit[world] = 1;
        VectorCopy(unit, tests[count]);
        ++count;
        VectorCopy(axes[world], tests[count]);
        ++count;
        for (int local = 0; local < 3; ++local) {
            CrossProduct(axes[local], unit, tests[count]);
            ++count;
        }
    }
    for (int i = 0; i < count; ++i) {
        float reach = 0;
        for (int axis = 0; axis < 3; ++axis) {
            reach += half[axis] * fabsf(tests[i][axis]);
            reach += extent[axis] * fabsf(DotProduct(axes[axis], tests[i]));
        }
        if (fabsf(DotProduct(delta, tests[i])) > reach + .001f)
            return qfalse;
    }
    return qtrue;
}

static inline qboolean JAR_BoxVolumeContact(const vec3_t center, const vec3_t angles,
    const vec3_t extent, const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
    vec3_t axes[3];
    AnglesToAxis(angles, axes);
    return JAR_BoxAxesContact(center, axes, extent, origin, mins, maxs);
}
