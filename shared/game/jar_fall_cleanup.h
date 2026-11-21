// Shared Sandbox out-of-map policy. GPL-2.0-only.
#pragma once
#include <math.h>

static inline float JAR_FallFloor(float mapBottom) {
	return isfinite(mapBottom) ? fmaxf(MIN_WORLD_COORD, mapBottom - 256.f) : MIN_WORLD_COORD;
}

static inline qboolean JAR_OutsideMap(const vec3_t origin, float localTop, float fallFloor) {
	for (int axis = 0; axis < 3; ++axis) {
		if (!isfinite(origin[axis])) return qfalse;
	}
	for (int axis = 0; axis < 3; ++axis) {
		if (origin[axis] <= MIN_WORLD_COORD || origin[axis] >= MAX_WORLD_COORD) return qtrue;
	}
	return isfinite(localTop) && origin[2] + localTop < fallFloor ? qtrue : qfalse;
}
