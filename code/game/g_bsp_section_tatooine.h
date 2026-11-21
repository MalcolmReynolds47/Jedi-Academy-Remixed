// Stock Tatooine FFA section data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_sentinel.h"

namespace jar { namespace tatooine {
constexpr unsigned checksum = 0xdd9d694au;
constexpr int modelCount = 9, entityCount = 220, entryCount = 16, pickupCount = 40;
constexpr int platform = 1;
// Model 4 extends below the world model at its authored bottom position.
constexpr float minimumTravelZ = -376.0f;
constexpr sentinel::Lift lifts[] = {
    {4,1,5,"42","piston"}, {5,0,1,"-264","piston"}, {6,0,5,"-264","piston"},
    {7,0,1,"-264","piston"}, {8,0,1,"-264","piston"}
};
constexpr sentinel::Pad pads[] = {{2,"lift1",6}, {3,"lift2",6}};
constexpr rift::Pickup pickups[] = {
    {"item_medpak_instant",6,30}, {"item_shield_sm_instant",2,20}, {"item_shield_lrg_instant",2,20},
    {"ammo_blaster",4,40}, {"ammo_metallic_bolts",6,40}, {"ammo_powercell",4,40}, {"ammo_rockets",2,40},
    {"weapon_trip_mine",2,40}, {"weapon_thermal",2,40}, {"weapon_det_pack",1,40},
    {"weapon_blaster",1,5}, {"weapon_disruptor",1,5}, {"weapon_bowcaster",1,5}, {"weapon_repeater",1,5},
    {"weapon_rocket_launcher",1,5}, {"weapon_flechette",1,5},
    {"item_force_enlighten_light",1,120}, {"item_force_enlighten_dark",1,120}, {"item_force_boon",1,120}
};
} }
