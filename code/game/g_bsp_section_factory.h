// Stock Factory arena data. GPL-2.0-only.
#pragma once
#include "g_bsp_section_rift.h"
#include <cstring>

namespace jar { namespace factory {
constexpr unsigned checksum=0x4c083d62u;
constexpr int modelCount=6,entityCount=186,entryCount=29,pickupCount=18;
constexpr int hurtModel=3;
constexpr char baseModel[]="models/map_objects/mp/flag_base.md3";
constexpr char glassSound[]="sound/effects/glassbreak1.wav";
constexpr char glassImpact[]="effects/misc/glass_impact.efx";
struct Flag {const char* name;const char* model;const char* origin;const char* color;};
constexpr Flag flags[]={
    {"team_CTF_redflag","models/flags/r_flag.md3","1548 2592 -320","Red"},
    {"team_CTF_blueflag","models/flags/b_flag.md3","7220 2592 -320","Blue"}
};
inline int FlagTeam(const char* name){
    if(name)for(size_t i=0;i<ARRAY_LEN(flags);++i)if(!strcmp(name,flags[i].name))return int(i)+1;
    return 0;
}
inline int StartTeam(const char* name){
    if(!name)return 0;
    if(!strcmp(name,"team_CTF_redplayer")||!strcmp(name,"team_CTF_redspawn"))return 1;
    if(!strcmp(name,"team_CTF_blueplayer")||!strcmp(name,"team_CTF_bluespawn"))return 2;
    return 0;
}
struct Window {int model,shards;const char* shader;float points[4][3];};
constexpr Window windows[]={
    {1,12,"textures/doomgiver/mapd2",{{5240,1728,-272},{5144,1728,-272},{5144,1696,-208},{5240,1696,-208}}},
    {2,8,"textures/doomgiver/mapd1",{{4980,1692,-216},{4980,1692,-280},{4860,1812,-280},{4860,1812,-216}}},
    {4,12,"textures/doomgiver/mapd2",{{3624,1728,-272},{3528,1728,-272},{3528,1696,-208},{3624,1696,-208}}},
    {5,8,"textures/doomgiver/mapd1",{{3908,1812,-216},{3908,1812,-280},{3788,1692,-280},{3788,1692,-216}}}
};
inline const Window* Glass(int localModel){
    for(const auto& window:windows)if(window.model==localModel)return &window;
    return nullptr;
}
constexpr rift::Pickup pickups[]={
    {"weapon_det_pack",2,40},{"weapon_thermal",2,40},{"weapon_trip_mine",2,40},
    {"weapon_blaster",2,5},{"weapon_repeater",2,5},{"weapon_flechette",2,5},
    {"weapon_disruptor",2,5},{"weapon_bowcaster",2,5},{"weapon_rocket_launcher",1,5},
    {"item_force_boon",1,120}
};
} }
