// Extend a host view to include a separately instanced brush model.
// GPL-2.0-only.
#pragma once

static inline float R_BSPFarDepth(const vec3_t bounds[2], const vec3_t origin,
                                const vec3_t axis[3], const vec3_t eye,
                                const vec3_t forward, const cplane_t *sidePlanes)
{
	unsigned visibleSides = 0;
	float depth = 0;
	for (int corner = 0; corner < 8; ++corner)
	{
		vec3_t point, offset;
		VectorCopy(origin, point);
		for (int i = 0; i < 3; ++i)
			VectorMA(point, bounds[(corner >> i) & 1][i], axis[i], point);
		if (sidePlanes)
			for (int side = 0; side < 4; ++side)
				if (DotProduct(point, sidePlanes[side].normal) > sidePlanes[side].dist)
					visibleSides |= 1u << side;
		VectorSubtract(point, eye, offset);
		const float distance = DotProduct(offset, forward);
		if (distance > depth)
			depth = distance;
	}
	if ((sidePlanes && visibleSides != 15) || depth <= 0)
		return 0;
	return depth + 1;
}
