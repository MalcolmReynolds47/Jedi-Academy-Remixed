// Validate fixed Sandbox attachment points against the static map. GPL-2.0-only.
#pragma once

typedef void (*jarWorldAnchorTrace_t)(trace_t*, const vec3_t, const vec3_t);

static inline qboolean JAR_WorldAnchorHit(const vec3_t point, const vec3_t direction,
                                         jarWorldAnchorTrace_t trace, vec3_t normal) {
	vec3_t start, end;
	VectorMA(point, 2.f, direction, start);
	VectorMA(point, -2.f, direction, end);
	trace_t hit = {0};
	trace(&hit, start, end);
	if (hit.startsolid || hit.allsolid || hit.fraction < 0 || hit.fraction >= 1 ||
	    hit.entityNum != ENTITYNUM_WORLD || DistanceSquared(hit.endpos, point) > .25f) return qfalse;
	if (!(VectorLengthSquared(hit.plane.normal) > .99f)) return qfalse;
	VectorCopy(hit.plane.normal, normal);
	return qtrue;
}

static inline qboolean JAR_WorldAnchorSurface(const vec3_t point, const vec3_t facing,
                                             jarWorldAnchorTrace_t trace, vec3_t normal) {
	if (VectorLengthSquared(facing) > .5f) {
		if (JAR_WorldAnchorHit(point, facing, trace, normal) && DotProduct(facing, normal) >= .98f)
			return qtrue;
		return qfalse;
	}
	// A hinge axis need not align with the surface. Probe both sides of each axis.
	for (int axis = 0; axis < 3; ++axis) {
		for (int sign = 1; sign >= -1; sign -= 2) {
			vec3_t direction = {0, 0, 0};
			direction[axis] = (float)sign;
			if (JAR_WorldAnchorHit(point, direction, trace, normal)) return qtrue;
		}
	}
	return qfalse;
}
