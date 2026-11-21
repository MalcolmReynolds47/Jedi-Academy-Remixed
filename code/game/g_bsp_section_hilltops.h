// Data from the retail Yavin Hilltops BSP. GPL-2.0-only.
#pragma once
#include "g_bsp_section_ctf.h"
#include "g_bsp_section_rift.h"

namespace jar { namespace hilltops {
constexpr unsigned checksum=0x45686774u;
constexpr int models=25,entities=180,starts=55,pickupCount=54;
constexpr int falls[]={9,10,17,18};
constexpr sectionCTF::Lift lifts[]={{12,16,600,"t5",true},{13,16,400,"t4",true},{16,16,600,"t3",true},
    {19,16,600,"bluesidelift2",true},{22,16,400,"bluesidelift",true},{23,16,600,"bluesidelift3",true}};
constexpr sectionCTF::Pad pads[]={{1,2,2000,-1,4},{2,0,2000,90,4},{3,1,2000,270,6},{4,1,2000,90,6},
    {5,4,2000,90,6},{6,4,2000,270,6},{7,3,2000,-1,4},{8,5,2000,270,4},
    {11,0,2000,-1,4},{14,1,2000,-1,6},{15,2,2000,-1,4},{20,3,2000,-1,4},{21,4,2000,-1,6},{24,5,2000,-1,4}};
constexpr char exterior[]="sound/ambience/yavin/yavin_ext.wav";
constexpr char wind[]="sound/ambience/prototype/windmix1.wav";
constexpr char exteriorSet[]="yavin_exterior";
constexpr const char* birdCalls[]={"sound/ambience/yavin/bird1.mp3","sound/ambience/yavin/bird2.mp3",
    "sound/ambience/yavin/bird3.mp3","sound/ambience/yavin/bird4.mp3","sound/ambience/yavin/bird5.mp3",
    "sound/ambience/yavin/bird6.mp3","sound/ambience/yavin/bird7.mp3"};
constexpr int callMin=20000,callMax=50000;
constexpr int birdSpeakerTag=0x4a594253;
constexpr rift::Pickup pickups[]={{"item_medpak_instant",14,30},{"item_shield_sm_instant",4,20},
    {"item_shield_lrg_instant",2,20},{"ammo_metallic_bolts",6,40},{"ammo_rockets",5,40},{"ammo_powercell",3,40},
    {"ammo_blaster",3,40},{"ammo_thermal",2,40},{"weapon_thermal",4,40},{"weapon_disruptor",2,5},
    {"weapon_blaster",2,5},{"weapon_repeater",2,5},{"weapon_demp2",2,5},{"weapon_flechette",2,5},
    {"weapon_rocket_launcher",1,5}};
} }
