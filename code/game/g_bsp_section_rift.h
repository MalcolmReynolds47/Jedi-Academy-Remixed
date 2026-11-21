// Authored data for the portable Rift Sanctuary. GPL-2.0-only.
#ifndef G_BSP_SECTION_RIFT_H
#define G_BSP_SECTION_RIFT_H

namespace jar { namespace rift {
constexpr char resource[] = "#mp/ffa4";
constexpr char path[] = "maps/mp/ffa4.bsp";
constexpr unsigned checksum = 0xca78c8d7u;
constexpr int arenaArea = 1;
constexpr int pickupTag = 0x4a525046;
constexpr int pickupCount = 38;

struct BrushBox { float mins[3], maxs[3]; };
constexpr BrushBox fallBoxes[] = {
    {{4608,1536,-2176}, {7296,2688,-512}},
    {{4608,2688,-2176}, {9120,5632,-512}},
    {{8576,960,-2176}, {10240,2688,-512}},
};

struct Pickup { const char* name; int count; int seconds; };
constexpr Pickup pickups[] = {
    {"item_medpak_instant", 8, 30},
    {"item_shield_sm_instant", 5, 20},
    {"item_shield_lrg_instant", 1, 20},
    {"ammo_blaster", 5, 40},
    {"ammo_powercell", 4, 40},
    {"ammo_metallic_bolts", 3, 40},
    {"ammo_rockets", 3, 40},
    {"weapon_thermal", 2, 5},
    {"weapon_repeater", 1, 5},
    {"weapon_bowcaster", 1, 5},
    {"weapon_blaster", 1, 5},
    {"weapon_demp2", 1, 5},
    {"weapon_rocket_launcher", 1, 5},
    {"weapon_flechette", 1, 5},
    {"weapon_disruptor", 1, 5},
};
} }
#endif
