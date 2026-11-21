// Stock Imperial Shaft section data. GPL-2.0-only.
#ifndef G_BSP_SECTION_SHAFT_H
#define G_BSP_SECTION_SHAFT_H

namespace jar { namespace shaft {
constexpr unsigned checksum = 1406961046u;
constexpr char liftSoundSet[] = "@jar_shaft_platform";
constexpr const char* liftSounds[] = {
    "sound/movers/platforms/normalplat_start.mp3",
    "sound/movers/platforms/normalplat_move.wav",
    "sound/movers/platforms/normalplat_stop.mp3"
};
constexpr const char* sceneryModels[] = {
    "models/map_objects/taspir/lava_ring.md3",
    "models/map_objects/taspir/lava_tube.md3",
    "models/map_objects/taspir/tank_top.md3"
};
} }
#endif
