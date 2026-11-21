// Data from the retail Coruscant Streets BSP. GPL-2.0-only.
#pragma once
#include "g_bsp_section_ctf.h"
#include "g_bsp_section_rift.h"
namespace jar { namespace coruscant {
constexpr char path[]="maps/mp/ctf4.bsp";
constexpr char resource[]="#mp/ctf4";
constexpr unsigned checksum=0xf4307a50u;
constexpr int models=24, entities=388, starts=52, pickupCount=67;
constexpr int falls[]={9,10,23};
constexpr char fog[]="textures/mp/roofog";
constexpr sectionCTF::Lift lifts[]={{12,32,500,"lift",false},{14,12,500,"lift2",false},{16,16,500,"lift3",false},
    {17,16,500,"t158",false},{19,12,500,"t175",false},{21,32,500,"t217",false}};
constexpr sectionCTF::Pad pads[]={{1,3,200,-1,6},{2,3,200,-1,6},{3,5,200,180,6},{4,4,200,360,6},
    {5,2,200,-1,6},{6,2,200,-1,6},{7,0,200,270,6},{8,1,200,90,6},{11,0,2000,-1,6},
    {13,1,2000,-1,6},{15,2,2000,-1,6},{18,3,2000,-1,6},{20,4,2000,-1,6},{22,5,2000,-1,6}};
constexpr char flyby[]="sound/vehicles/shuttle/flyby.mp3";
struct Sound {const char* file;int count;};
constexpr Sound sounds[]={{flyby,6},{"sound/ambience/narshaddaa/ns_streetint.wav",26},
    {"sound/ambience/narshaddaa/ns_hideint.wav",8},{"sound/ambience/bespin/bespin_wind.wav",1}};
constexpr rift::Pickup pickups[]={{"ammo_rockets",8,40},{"ammo_blaster",12,40},
    {"ammo_metallic_bolts",12,40},{"ammo_powercell",18,40},{"weapon_flechette",2,5},
    {"weapon_thermal",2,40},{"weapon_rocket_launcher",2,5},{"weapon_repeater",2,5},
    {"weapon_bowcaster",2,5},{"weapon_disruptor",2,5},{"weapon_det_pack",2,40},
    {"weapon_trip_mine",2,40},{"item_force_boon",1,120}};
} }
