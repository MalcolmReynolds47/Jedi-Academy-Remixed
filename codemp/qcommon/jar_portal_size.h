// A portable panel's width occupies a world span, and its height travels in eventParm.
#pragma once
#include <limits.h>

static inline int JAR_MP_PortalSizeValid(float width, float height) {
 return width >= 1 && width <= WORLD_SIZE && height >= 1 && height <= USHRT_MAX;
}
