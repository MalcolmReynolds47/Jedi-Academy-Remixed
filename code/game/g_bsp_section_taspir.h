// Stock Taspir arena data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_rift.h"

namespace jar { namespace taspir {
constexpr unsigned checksum=0x0d933db6u;
constexpr int modelCount=17,entityCount=186,entryCount=16,pickupCount=42,sceneryCount=84;
constexpr int fallModel=7,trainModel=2;
constexpr int doors[]={3,4,5,6,8,12,13,14,15,16};
struct Platform {int model,height;const char* lip;};
constexpr Platform platforms[]={{1,656,"0"},{9,520,"0"},{10,640,""},{11,640,""}};
struct Corner {const char* name;int height,speed,wait;};
constexpr Corner corners[]={
    {"floor1_stop",-16,0,3}, {"floor1",-16,150,0},
    {"floor2_stop",624,0,3}, {"floor2",624,150,0},
    {"floor3_stop",1264,0,3}, {"floor3",1264,150,0},
    {"floor2a_stop",624,0,3}, {"floor2a",624,150,0}
};
constexpr int helperCount=int(ARRAY_LEN(doors)+ARRAY_LEN(platforms));
constexpr char sceneryPrefix[]="models/map_objects/taspir/";
struct Scenery { const char* model; int count; };
constexpr Scenery scenery[]={
    {"lava_ring.md3",48},{"strut.md3",9},{"lamp_rotated.md3",7},{"lamp2_double.md3",6},
    {"airpurifier.md3",5},{"control_station.md3",5},{"wall_pipe.md3",4}
};
struct SoundSet {const char* stock;const char* runtime;const char* files[3];};
constexpr SoundSet sounds[]={
    {"impdoor1","@jar_taspir_door",{"sound/movers/doors/door1start.wav","sound/movers/doors/door1move.wav","sound/movers/doors/door1stop.wav"}},
    {"platform","@jar_taspir_platform",{"sound/movers/platforms/platform_start.mp3","sound/movers/platforms/platform_move_lp.wav","sound/movers/platforms/platform_stop.mp3"}},
    {"pedestal","@jar_taspir_pedestal",{"sound/movers/platforms/pedestal_start.mp3","sound/movers/platforms/pedestal_move_lp.wav","sound/movers/platforms/pedestal_stop.mp3"}}
};
constexpr rift::Pickup pickups[]={
    {"item_medpak_instant",4,30},{"item_shield_sm_instant",3,20},{"item_shield_lrg_instant",1,20},
    {"ammo_metallic_bolts",5,40},{"ammo_blaster",5,40},{"ammo_rockets",4,40},{"ammo_powercell",4,40},
    {"ammo_tripmine",1,40},{"ammo_detpack",1,40},{"ammo_thermal",1,40},
    {"weapon_thermal",1,40},{"weapon_det_pack",1,40},{"weapon_trip_mine",1,40},
    {"weapon_rocket_launcher",1,5},{"weapon_disruptor",1,5},{"weapon_flechette",1,5},{"weapon_demp2",1,5},
    {"weapon_repeater",1,5},{"weapon_bowcaster",1,5},{"weapon_blaster",1,5},
    {"item_force_enlighten_light",1,120},{"item_force_enlighten_dark",1,120},{"item_force_boon",1,120}
};
} }
