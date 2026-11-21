// Stock Hoth Canyon's detached sky geometry. GPL-2.0-only.
#ifndef JAR_HOTH_SKY_H
#define JAR_HOTH_SKY_H
#include "jar_hoth_sky_camera.h"

namespace jar { namespace hothSky {

constexpr char resource[] = "#mp/duel9";
constexpr unsigned checksum = 0xb0b505a2u;
constexpr int arenaArea = 2;
constexpr int surfaceCount = 627;

// The stock sky camera's PVS consists of these two surface ranges.
// They are disjoint from the playable canyon's surfaces.
constexpr bool BackdropSurface(int index)
{
    return (index >= 333 && index <= 337) || (index >= 412 && index <= 511);
}

} }
#endif
