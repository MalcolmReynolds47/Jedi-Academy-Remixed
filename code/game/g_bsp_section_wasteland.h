// Retail Hoth Wasteland controller data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_ctf.h"
#include "g_bsp_section_rift.h"

namespace jar { namespace wasteland {
constexpr char resource[]="#mp/ctf2";
constexpr char path[]="maps/mp/ctf2.bsp";
constexpr char fog[]="textures/mp/hoth_ctf_fog";
constexpr unsigned checksum=0xc5964d20u;
constexpr int models=18,entities=269,starts=25,pickupCount=54,helpers=3;

struct WeatherVolume {int model;float mins[3],maxs[3];};
constexpr WeatherVolume weatherVolumes[]={
    {1,{-656,1504,-112},{1296,2528,800}},
    {2,{616,2768,-304},{3216,5864,688}},
    {3,{-2528,-1848,-304},{-16,1216,712}},
};
constexpr char weatherMaterial[]="textures/system/weatherzone";
constexpr const char* snowOrigins[]={"1552 3408 336","-944 656 336"};
constexpr int snowCount=1000;

constexpr char doorTeam[]="blue_door1";
struct DoorLeaf {int model,angle;};
constexpr DoorLeaf doorLeaves[]={{5,180},{6,360},{15,180},{16,360}};
constexpr int platforms[]={4,17};
constexpr int platformHeight=384;
constexpr sectionCTF::Lift lifts[]={{10,-380,250,"t216",false},{11,-380,250,"t215",false}};
constexpr sectionCTF::Pad liftPads[]={{9,0,200,-1,6},{12,1,200,-1,6}};
struct Fall {int model,brushes,damage,flags;};
constexpr Fall falls[]={{7,2,9999,12},{8,1,-1,0},{13,1,-1,0},{14,2,9999,12}};

constexpr int gunRespawn=15000;
constexpr int gunHealth=320,gunAmmo=999,gunTag=0x4a485747;
constexpr char gunModel[]="models/map_objects/imp_mine/turret_chair.glm";
constexpr char gunSound[]="sound/weapons/emplaced/emplaced_mount.mp3";
constexpr char gunExitSound[]="sound/weapons/emplaced/emplaced_dismount.mp3";
constexpr char gunMoveSound[]="sound/weapons/emplaced/emplaced_move_lp.wav";
constexpr char gunExplosion[]="effects/emplaced/explode.efx";
constexpr char snowImage[]="gfx/effects/snowflake1.tga";
constexpr char crateModel[]="models/map_objects/hoth/crate_stackable.md3";
struct Gun {const char* origin;const char* heading;};
constexpr Gun guns[]={{"-1056 236 128","90"},{"1664 3828 128","270"}};
constexpr const char* flagOrigins[]={"1856 5600 156","-1248 -1536 156"};
struct Scenery {const char* origin;const char* heading;bool pedestal;};
constexpr Scenery scenery[]={
    {"1824 4464 128","",false},{"-1152 -464 128","180",false},
    {"-1248 -1536 128","",true},{"1856 5600 128","",true},
    {"2336 5048 128","",false},{"-1728 -984 128","180",false},
};
constexpr rift::Pickup pickups[]={
    {"item_medpak_instant",14,30},{"item_shield_sm_instant",4,20},
    {"ammo_metallic_bolts",4,40},{"ammo_rockets",4,40},{"ammo_blaster",4,40},
    {"ammo_powercell",4,40},{"ammo_thermal",4,40},{"weapon_thermal",4,40},
    {"weapon_rocket_launcher",2,5},{"weapon_flechette",2,5},{"weapon_demp2",2,5},
    {"weapon_repeater",2,5},{"weapon_blaster",2,5},{"weapon_disruptor",2,5},
};
} }
