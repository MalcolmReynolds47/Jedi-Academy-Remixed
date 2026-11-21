// Stock Vjun Sentinel section data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_rift.h"

namespace jar { namespace sentinel {
constexpr unsigned checksum=0x9aeac74fu;
constexpr int modelCount=17,entityCount=229,entryCount=14,pickupCount=45,fallModel=5;
constexpr char pistonSet[]="@jar_sentinel_piston";
constexpr const char* pistonSounds[]={
    "sound/movers/platforms/piston_start.mp3",
    "sound/movers/platforms/piston_move_lp.wav",
    "sound/movers/platforms/piston_stop.mp3"
};
struct Lift { int model,group,flags; const char* lip; const char* sound; };
constexpr Lift lifts[]={
    {1,0,1,"-360","piston"},{2,0,1,"-348",""},{3,0,1,"-348",""},
    {4,0,1,"-348",""},{6,0,5,"-348","medium_platform"},
    {11,3,5,"-488","medium_platform"},{13,4,5,"-408","medium_platform"},
    {14,1,1,"","medium_platform"},{15,2,1,"","medium_platform"},{16,3,1,"-488",""}
};
struct Pad { int model; const char* target; int wait; };
constexpr Pad pads[]={{7,"t90",6},{8,"t63",9},{9,"dplat1",9},{10,"t67",7},{12,"t118",7}};
constexpr rift::Pickup pickups[]={
    {"item_medpak_instant",13,30},{"item_shield_sm_instant",5,20},{"item_shield_lrg_instant",1,20},
    {"ammo_powercell",4,40},{"ammo_blaster",4,40},{"ammo_rockets",3,40},{"ammo_metallic_bolts",3,40},
    {"weapon_thermal",2,40},{"weapon_det_pack",2,40},{"weapon_concussion_rifle",1,5},
    {"weapon_blaster",1,5},{"weapon_bowcaster",1,5},{"weapon_disruptor",1,5},
    {"weapon_flechette",1,5},{"weapon_rocket_launcher",1,5},{"weapon_demp2",1,5},{"weapon_repeater",1,5}
};
} }
