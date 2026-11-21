// Retail Imperial Drop Zone BSP controller data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_ctf.h"
#include "g_bsp_section_rift.h"

namespace jar { namespace dropzone {
constexpr unsigned checksum=0x674a675fu;
constexpr int models=34,entities=282,starts=45,pickupCount=64,speakerCount=14;
struct Mover {
    int model;
    const char* target;
    int speed,angle,flags,damage,delay;
    const char* delayField;
    bool Platform() const {return angle==0;}
};
// Platforms use the native binary-mover delay in milliseconds. Door delay
// keys are in seconds, including the fractional values in the retail BSP.
constexpr Mover movers[]={
    {9,"t175",450,-1,0,2,1000,"1"},
    {11,"t56",600,0,0,2,50,"50"},
    {16,"midlift1",600,0,0,2,50,"50"},
    {27,"blueriser",500,-2,5,5000,200,"0.2"},
    {28,"blueupdoor",500,-2,1,2,200,"0.2"},
    {29,"bluelift2",450,-1,0,2,1000,"1"},
    {30,"bluedowndoor",500,-2,0,2,800,"0.8"},
    {31,"t72",500,-2,0,2,800,".8"},
    {32,"t87",500,-2,1,2,200,"0.2"},
    {33,"t89",500,-2,5,5000,200,"0.2"},
};
constexpr sectionCTF::Pad pads[]={
    {1,9,2000,-1,9},{2,3,2000,-1,9},{3,7,2000,-1,9},{4,6,2000,-1,9},
    {5,5,1000,-1,3},{6,0,1000,-1,4},{7,8,2000,-1,9},{8,4,2000,-1,9},
    {10,0,1000,-1,3},{12,1,100,-1,1},{13,3,2000,-1,9},{14,5,1000,-1,3},
    {15,4,2000,-1,9},{17,2,100,-1,1},{18,6,2000,-1,9},{19,7,2000,-1,9},
    {20,8,2000,-1,9},{21,9,2000,-1,9},
};
constexpr int falls[]={22,23,24,25,26};
struct Ambient {const char* path;int count;};
constexpr Ambient ambience[]={
    {"sound/ambience/artus/artus_shaft.wav",8},{"sound/ambience/artus/artus_hangar.wav",2},
    {"sound/ambience/cairn/cairn_circuits.wav",2},{"sound/ambience/doomgiver/doom_bridge.wav",2},
};
constexpr rift::Pickup pickups[]={
    {"item_shield_sm_instant",2,20},{"item_medpak_instant",22,30},
    {"ammo_powercell",6,40},{"ammo_blaster",4,40},{"ammo_metallic_bolts",4,40},{"ammo_rockets",4,40},
    {"weapon_trip_mine",2,40},{"weapon_thermal",4,40},{"weapon_disruptor",3,5},
    {"weapon_rocket_launcher",3,5},{"weapon_bowcaster",2,5},{"weapon_blaster",2,5},
    {"weapon_demp2",2,5},{"weapon_repeater",2,5},{"weapon_flechette",2,5},
};
} }
