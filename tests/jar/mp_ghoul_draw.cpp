// Compare with the original scalar formulas, including their order of rounding.
#include "rd-vanilla/tr_ghoul2_skin.h"
#include "rd-vanilla/tr_diffuse.h"
#include "test_support.h"
#include <array>
#include <vector>

namespace {
struct TestCache {
	std::array<mdxaBone_t, 32> bones;
	std::vector<int> calls;
	const mdxaBone_t& EvalRender(int index) {
		calls.push_back(index);
		return bones.at(index);
	}
};

void CheckSkin() {
	TestCache cache{};
	int references[32];
	for (int b = 0; b < 32; ++b) {
		references[b] = 31-b;
		for (int axis = 0; axis < 3; ++axis)
			for (int k = 0; k < 4; ++k)
				cache.bones[b].matrix[axis][k] = ((b*17 + axis*7 + k*13) % 47 - 23)*.173f;
	}
	mdxmVertex_t vertices[64]{};
	vec4_t positions[64], normals[64];
	for (int i = 0; i < 64; ++i) {
		auto& v = vertices[i];
		const int count = 1 + i%4;
		v.uiNmWeightsAndBoneIndexes = unsigned(count-1)<<30;
		for (int j = 0; j < count; ++j) {
			const int slot = (i%3 == 0) ? 31 : (i*5 + j*11)%16;
			const int weight = j == 0 ? (i%4==1 && i%3==0 ? 1023 : 273) : 188;
			v.uiNmWeightsAndBoneIndexes |= unsigned(slot) << (5*j);
			v.uiNmWeightsAndBoneIndexes |= unsigned(weight>>8) << (20+2*j);
			v.BoneWeightings[j] = static_cast<byte>(weight);
		}
		for (int axis = 0; axis < 3; ++axis) {
			v.vertCoords[axis] = (i*3 - axis*21)*.147f;
			v.normal[axis] = (axis*5 - i)*.027f;
		}
	}
	ghoul2_skin::SkinSurface(cache, references, vertices, 0, positions, normals);
	CHECK(cache.calls.empty());
	for (int pass = 0; pass < 2; ++pass) {
		cache.calls.clear();
		cache.bones[0].matrix[1][3] += 1.25f;
		ghoul2_skin::SkinSurface(cache, references, vertices, 64, positions, normals);
		std::vector<int> expectedCalls;
		bool used[32]{};
		for (int i = 0; i < 64; ++i) {
			const auto* v = &vertices[i];
			const int count = G2_GetVertWeights(v);
			float total = 0, weights[4];
			vec3_t points[4];
			for (int j = 0; j < count; ++j) {
				const int bone = references[G2_GetVertBoneIndex(v, j)];
				if (!used[bone]) {
					expectedCalls.push_back(bone);
					used[bone] = true;
				}
				weights[j] = G2_GetVertBoneWeight(v, j, total, count);
				for (int axis = 0; axis < 3; ++axis) {
					const auto& row = cache.bones[bone].matrix[axis];
					points[j][axis] = DotProduct(row, v->vertCoords) + row[3];
					if (!j)
						CHECK(normals[i][axis] == DotProduct(row, v->normal));
				}
			}
			for (int axis = 0; axis < 3; ++axis) {
				float expected = points[0][axis];
				if (count == 2)
					expected = weights[0]*(expected-points[1][axis]) + points[1][axis];
				if (count > 2) {
					expected *= weights[0];
					for (int j = 1; j < count; ++j)
						expected += weights[j]*points[j][axis];
				}
				CHECK(positions[i][axis] == expected);
			}
		}
		CHECK(cache.calls == expectedCalls);
	}
}

void CheckLight() {
	vec4_t normals[17]{};
	const vec3_t direction = {-.61f, .13f, .47f};
	const vec3_t ambient = {5.9f, 260.4f, -1.2f}, directed = {74.4f, -58.7f, 2.1f};
	const int packedAmbient = 0x71ab036c;
	for (int i = 0; i < 17; ++i)
		for (int axis = 0; axis < 3; ++axis)
			normals[i][axis] = ((i*i*(axis+1))%19 - i)*.91f;
	for (int count = 0; count <= 17; ++count) {
		std::array<byte, 17*4+2> expected{}, actual{};
		for (int i = 0; i < count; ++i) {
			byte* color = expected.data()+1+i*4;
			const float incoming = DotProduct(normals[i], direction);
			if (incoming <= 0) {
				std::memcpy(color, &packedAmbient, 4);
			} else {
				for (int c = 0; c < 3; ++c) {
					const int value = Q_ftol(ambient[c]+incoming*directed[c]);
					color[c] = value > 255 ? 255 : value;
				}
				color[3] = 255;
			}
		}
		diffuse::Colors(actual.data()+1, normals, count, direction, ambient, directed, packedAmbient);
		CHECK(actual == expected);
	}
}
}

void TestGhoulDraw() {
	CheckSkin();
	CheckLight();
}
