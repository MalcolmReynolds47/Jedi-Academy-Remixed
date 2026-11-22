// Multiplayer Classic Ghoul2 vertex skinning. GPL-2.0-only.
#pragma once

#include "rd-common/tr_types.h"
#include <cstring>
#if defined(__SSE2__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace ghoul2_skin {
// Columns keep the three output coordinates adjacent during each operation.
// The fourth lane is the padding already present in the tessellation buffer.
struct alignas(16) Column {
#if defined(__SSE2__) || defined(_M_X64)
	__m128 lanes;
	Column() = default;
	explicit Column(__m128 v) : lanes(v) {}
	Column(float x, float y, float z, float w) : lanes(_mm_setr_ps(x, y, z, w)) {}
	Column operator+(Column b) const { return Column(_mm_add_ps(lanes, b.lanes)); }
	Column operator-(Column b) const { return Column(_mm_sub_ps(lanes, b.lanes)); }
	Column operator*(float s) const { return Column(_mm_mul_ps(lanes, _mm_set1_ps(s))); }
#else
	float x, y, z, pad;

	Column operator+(const Column& b) const { return {x+b.x, y+b.y, z+b.z, pad+b.pad}; }
	Column operator-(const Column& b) const { return {x-b.x, y-b.y, z-b.z, pad-b.pad}; }
	Column operator*(float s) const { return {x*s, y*s, z*s, pad*s}; }
#endif
};

struct Bone {
	Column axis[3], origin;

	Column Rotate(const float v[3]) const {
		return axis[0]*v[0] + axis[1]*v[1] + axis[2]*v[2];
	}

	Column Transform(const float v[3]) const {
		return Rotate(v) + origin;
	}
};

// Evaluation marks bones as rendered and can apply smoothing. Resolve only
// bones actually used by this surface, in their original first-use order.
template<class BoneCache>
void SkinSurface(BoneCache& cache, const int* references, const mdxmVertex_t* vertices,
                 int count, vec4_t* positions, vec4_t* normals) {
	Bone palette[iMAX_G2_BONEREFS_PER_SURFACE];
	unsigned int used = 0;
	auto boneFor = [&](int slot) -> const Bone& {
		const unsigned int bit = 1u << slot;
		Bone& bone = palette[slot];
		if (!(used & bit)) {
			const auto& m = cache.EvalRender(references[slot]).matrix;
			for (int i = 0; i < 3; ++i)
				bone.axis[i] = {m[0][i], m[1][i], m[2][i], 0};
			bone.origin = {m[0][3], m[1][3], m[2][3], 0};
			used |= bit;
		}
		return bone;
	};

	for (int i = 0; i < count; ++i) {
		const mdxmVertex_t* v = &vertices[i];
		const Bone& first = boneFor(G2_GetVertBoneIndex(v, 0));
		const Column normal = first.Rotate(v->normal);
		Column point = first.Transform(v->vertCoords);
		const int weights = G2_GetVertWeights(v);
		if (weights > 1) {
			float total = 0;
			const float firstWeight = G2_GetVertBoneWeight(v, 0, total, weights);
			if (weights == 2) {
				const Column other = boneFor(G2_GetVertBoneIndex(v, 1)).Transform(v->vertCoords);
				point = (point - other)*firstWeight + other;
			} else {
				point = point*firstWeight;
				for (int j = 1; j < weights; ++j) {
					const Bone& bone = boneFor(G2_GetVertBoneIndex(v, j));
					point = point + bone.Transform(v->vertCoords)*G2_GetVertBoneWeight(v, j, total, weights);
				}
			}
		}
		static_assert(sizeof(Column) == sizeof(vec4_t), "Tessellation vector size");
		std::memcpy(positions[i], &point, sizeof(point));
		std::memcpy(normals[i], &normal, sizeof(normal));
	}
}
}
