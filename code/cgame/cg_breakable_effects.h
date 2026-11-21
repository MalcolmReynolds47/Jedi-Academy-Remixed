// Native SP breakable debris shared by the game and cgame. GPL-2.0-only.
#ifndef CG_BREAKABLE_EFFECTS_H
#define CG_BREAKABLE_EFFECTS_H

#include "../game/g_shared.h"

struct BreakableDebrisFrame
{
	vec3_t origin;
	vec3_t axis[3];

	void ToWorld(vec3_t point) const
	{
		vec3_t world;
		VectorCopy(origin, world);
		for (int i = 0; i < 3; ++i)
		{
			VectorMA(world, point[i], axis[i], world);
		}
		VectorCopy(world, point);
	}
};

void CG_MiscModelExplosion(const vec3_t mins, const vec3_t maxs, int size,
						   material_t chunkType, const BreakableDebrisFrame *frame = nullptr);
void CG_Chunks(int owner, const vec3_t origin, const vec3_t normal, const vec3_t mins, const vec3_t maxs,
			   float speed, int numChunks, material_t chunkType, int customChunk, float baseScale,
			   int customSound = 0, const BreakableDebrisFrame *frame = nullptr);

#endif
