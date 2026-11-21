// Jedi Academy Remixed attachment encoding. GPL-2.0-only; see LICENSE.txt.
#ifndef JAR_BOLT_H
#define JAR_BOLT_H
#include "jar_capacity.h"
#include <cstdint>

#define ENTITY_WIDTH JAR_ENTITY_BITS
#define MODEL_WIDTH 8
#define BOLT_WIDTH 10
#define MODEL_AND ((1 << MODEL_WIDTH) - 1)
#define BOLT_AND ((1 << BOLT_WIDTH) - 1)
#define ENTITY_AND ((1 << ENTITY_WIDTH) - 1)
#define BOLT_SHIFT 0
#define MODEL_SHIFT BOLT_WIDTH
#define ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)

namespace jar {
inline bool PackEntityBolt(int& result, int entity, int model, int bolt)
{
    if (entity < 0 || entity >= JAR_ENTITY_SLOTS - 2 ||
        model < 0 || model > MODEL_AND || bolt < 0 || bolt > BOLT_AND)
        return false;
    const uint32_t packed = (uint32_t(entity) << ENTITY_SHIFT) |
        (uint32_t(model) << MODEL_SHIFT) | uint32_t(bolt);
    // Preserve the existing signed API without overflowing a signed shift.
    result = packed <= INT32_MAX ? int(packed) : -1 - int(UINT32_MAX - packed);
    return true;
}
}
#endif
