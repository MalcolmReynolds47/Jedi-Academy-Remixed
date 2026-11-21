// A brush can be far from its BSP origin while its size stays bounded.
// GPL-2.0-only.
#ifndef G_BSP_SECTION_BOX_H
#define G_BSP_SECTION_BOX_H
#include "../qcommon/jar_volume.h"

inline bool BSPSectionBoxValid(const vec3_t low, const vec3_t high) {
    for (int axis = 0; axis < 3; ++axis) {
        const float extent = high[axis] - low[axis];
        if (!(low[axis] >= MIN_WORLD_COORD && high[axis] <= MAX_WORLD_COORD &&
              extent >= jar::volume::minimum && extent <= jar::volume::maximum))
            return false;
    }
    return true;
}
#endif
