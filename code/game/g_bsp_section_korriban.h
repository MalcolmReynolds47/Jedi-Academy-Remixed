// Retail Korriban FFA portable section. GPL-2.0-only.
#ifndef G_BSP_SECTION_KORRIBAN_H
#define G_BSP_SECTION_KORRIBAN_H
#include "g_bsp_section_rift.h"

namespace jar { namespace korriban {
constexpr char resource[] = "#mp/ffa2";
constexpr char path[] = "maps/mp/ffa2.bsp";
constexpr unsigned checksum = 0xad56447du;
constexpr int arenaArea = 0;
constexpr int entryCount = 17, entityCount = 311, fireCount = 28, windCount = 7, pickupCount = 47;
constexpr rift::BrushBox fallBoxes[] = {
    {{-4480,1936,-576}, {-4288,2416,64}},
    {{-2784,-384,-576}, {-2464,-128,160}},
    {{-2976,768,-576}, {-2368,1536,-64}},
    {{-4480,848,-576}, {-4032,1520,-64}},
    {{-1632,2880,-576}, {-672,3648,-64}},
    {{-1440,1088,-576}, {-640,1728,-64}},
    {{-1664,-384,-576}, {-1408,256,-64}},
    {{-2912,1920,-576}, {-2336,3168,-64}},
};
constexpr rift::Pickup pickups[] = {
    {"item_medpak_instant",12,30}, {"item_shield_sm_instant",7,20}, {"item_shield_lrg_instant",2,20},
    {"ammo_tripmine",3,40}, {"ammo_thermal",3,40}, {"ammo_powercell",3,40},
    {"ammo_rockets",3,40}, {"ammo_metallic_bolts",3,40}, {"ammo_blaster",3,40},
    {"weapon_bowcaster",2,5}, {"weapon_rocket_launcher",1,5}, {"weapon_flechette",1,5},
    {"weapon_demp2",1,5}, {"weapon_repeater",1,5}, {"weapon_disruptor",1,5}, {"weapon_blaster",1,5},
};
} }
#endif
