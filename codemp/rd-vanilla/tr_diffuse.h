// Vertex lighting for the multiplayer Classic renderer. GPL-2.0-only.
#pragma once

#include "qcommon/q_shared.h"
#include <cstring>
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace diffuse {
#if defined(__SSE2__) || defined(_M_X64)
inline __m128i Channel(__m128 incoming, float ambient, float directed) {
	const __m128 value = _mm_add_ps(_mm_set1_ps(ambient), _mm_mul_ps(incoming, _mm_set1_ps(directed)));
	// The scalar path limits the upper end and stores the low byte.
	const __m128i channel = _mm_cvttps_epi32(value);
	const __m128i maxByte = _mm_set1_epi32(255);
	return _mm_and_si128(_mm_or_si128(channel, _mm_cmpgt_epi32(channel, maxByte)), maxByte);
}
#endif

inline void Colors(unsigned char* out, const vec4_t* normals, int count, const vec3_t direction,
                   const vec3_t ambient, const vec3_t directed, int packedAmbient) {
	int i = 0;
#if defined(__SSE2__) || defined(_M_X64)
	for (; i + 4 <= count; i += 4) {
		__m128 x = _mm_loadu_ps(normals[i]), y = _mm_loadu_ps(normals[i+1]);
		__m128 z = _mm_loadu_ps(normals[i+2]), w = _mm_loadu_ps(normals[i+3]);
		_MM_TRANSPOSE4_PS(x, y, z, w);
		const __m128 incoming = _mm_add_ps(_mm_add_ps(_mm_mul_ps(x, _mm_set1_ps(direction[0])),
		                                             _mm_mul_ps(y, _mm_set1_ps(direction[1]))),
		                                  _mm_mul_ps(z, _mm_set1_ps(direction[2])));
		const __m128i unlit = _mm_castps_si128(_mm_cmple_ps(incoming, _mm_setzero_ps()));
		const __m128i red = Channel(incoming, ambient[0], directed[0]);
		const __m128i green = Channel(incoming, ambient[1], directed[1]);
		const __m128i blue = Channel(incoming, ambient[2], directed[2]);
		const __m128i rgb = _mm_or_si128(red, _mm_or_si128(_mm_slli_epi32(green, 8), _mm_slli_epi32(blue, 16)));
		const __m128i lit = _mm_or_si128(rgb, _mm_slli_epi32(_mm_set1_epi32(255), 24));
		const __m128i rgba = _mm_or_si128(_mm_and_si128(unlit, _mm_set1_epi32(packedAmbient)),
		                                 _mm_andnot_si128(unlit, lit));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(&out[4*i]), rgba);
	}
#endif
	for (; i < count; ++i) {
		const float incoming = DotProduct(normals[i], direction);
		if (incoming <= 0) {
			std::memcpy(&out[4*i], &packedAmbient, 4);
			continue;
		}
		for (int channel = 0; channel < 3; ++channel) {
			const int value = Q_ftol(ambient[channel] + incoming * directed[channel]);
			out[4*i+channel] = value > 255 ? 255 : value;
		}
		out[4*i+3] = 255;
	}
}
}
