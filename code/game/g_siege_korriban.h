// Retail Siege Korriban's authored targets and entry groups. GPL-2.0-only.
#pragma once

namespace jar { namespace siegeKorriban {
constexpr char resource[]="#mp/siege_korriban";
constexpr char bsp[]="maps/mp/siege_korriban.bsp";
constexpr char definition[]="maps/mp/siege_korriban.siege";
constexpr unsigned checksum=0xf9a2d444u;
constexpr int models=70,entities=691,starts=95;
constexpr char rancorClass[]="NPC_Monster_Rancor";
constexpr char rancorDefinition[]="ext_data/npcs/rancor.npc";
constexpr char rancorModel[]="models/players/rancor/model.glm";
constexpr char rancorOrigin[]="-1568 13088 -320";
constexpr int rancorTag=0x4a534b52;
constexpr char waypointModel[]="@jar_siege_korriban_waypoint";
struct Waypoint {const char* name;const char* target;const char* origin;};
constexpr Waypoint waypoints[]={
    {"t21","t22","-1512 12952 -392"},
    {"t22","t23","-1360 13000 -392"},
    {"t23","t24","-1168 12944 -392"},
    {"t24","t25","-1016 12888 -392"},
    {"t25","t26","-832 12856 -392"},
    {"t26","t27","-680 12936 -392"},
    {"t27","t28","-520 12984 -392"},
    {"t28","t29","-496 13128 -392"},
    {"t29","t30","-320 13112 -392"},
    {"t30","t31","-352 13320 -392"},
    {"t31","t32","-536 13288 -392"},
    {"t32","t33","-632 13416 -392"},
    {"t33","t34","-776 13544 -392"},
    {"t34","t35","-984 13552 -392"},
    {"t35","t36","-1216 13536 -392"},
    {"t36","t37","-1352 13464 -392"},
    {"t37","t38","-1440 13344 -392"},
    {"t38","t21","-1568 13088 -392"},
};
constexpr char ending[]="siegeend";
constexpr int roundDuration=1800*1000;
enum class Round {Waiting,Active,Won,Lost};
// The .siege announcement paths are absent from the retail PK3 archives.
// Reuse the retail game's success and failure cues.
constexpr char winSound[]="music/goodsmall.mp3";
constexpr char lossSound[]="music/badsmall.mp3";
constexpr int locationTag=0x4a534b4c;
constexpr char advanceCrystals[]="spawnrelay1";
constexpr char crystalCounter[]="cyrstalsinplacecounter";
struct Objective {int number;const char* name;const char* output;const char* announcement;bool startsOff;};
// Objectives 1 and 6 have extra outputs in the .siege file. The final
// objective's optional obj1_complete_imperial output has no BSP recipient.
constexpr Objective objectives[]={
    {1,"gateopen",advanceCrystals,"@SIEGE_3_KOR_LS_OBJ1_COMP_TEAM1",false},
    {2,"crystalcomplete1","","@SIEGE_3_KOR_LS_OBJ2_COMP_TEAM1",true},
    {3,"crystalcomplete2","","@SIEGE_3_KOR_LS_OBJ3_COMP_TEAM1",true},
    {4,"crystalcomplete3","","@SIEGE_3_KOR_LS_OBJ4_COMP_TEAM1",true},
    {5,"jedi_obj_5","","@SIEGE_3_KOR_LS_OBJ5_COMP_TEAM1",true},
    {6,"jedi_obj_6",ending,"@SIEGE_3_KOR_LS_OBJ6_COMP_TEAM1",true},
};
struct Delivery {int model;const char* trigger;const char* target;const char* itemModel;const char* itemTarget;};
constexpr Delivery deliveries[]={
    {65,"redcrystaldelivery","crystalcomplete1","models/map_objects/mp/crystal_red.md3","redcrystalspawn"},
    {66,"greencrystaldelivery","crystalcomplete2","models/map_objects/mp/crystal_green.md3","greencrystalspawn"},
    {67,"bluecrystaldelivery","crystalcomplete3","models/map_objects/mp/crystal_blue.md3","bluecrystalspawn"},
    {69,"staffplace","coffinguard_relay","models/map_objects/mp/mp_scepter.md3","jedi_obj_5"},
};
constexpr char coffinModel[]="models/map_objects/korriban/coffin_slab.md3";
constexpr char lavaLoop[]="sound/ambience/artus/artus_lava.wav";
constexpr char torchLoop[]="sound/effects/torch_burning_lp.wav";
constexpr char lavaSoundSet[]="artus_lava",torchSoundSet[]="yavintorch_lp";
constexpr int sceneryTag=0x4a534b4d,speakerTag=0x4a534b53,effectTag=0x4a534b46;
constexpr int sceneryCount=189,speakerCount=45,torchCount=175;
constexpr float mediaRange=8192;
struct TorchEffect {const char* file;int count;};
constexpr TorchEffect torchEffects[]={
    {"env/small_fire_lod",169},
    {"env/small_fire_blue",2},
    {"env/small_fire_green",2},
    {"env/small_fire_red",2},
};
struct NamedEffect {const char* name;const char* file;const char* target;int flags;};
constexpr NamedEffect namedEffects[]={
    {"coffinguard_shield","env/dome","",0},
    {"crystal_shield","env/red_cyc","",0},
    {"crystalsinplacelaser","mp/crystalbeamred","t18",1},
    {"c1","mp/crystalbeamred","t9",0},
    {"t12","mp/crystalbeamred","t11",0},
    {"t17","mp/crystalbeamred","t16",0},
};
constexpr char doorSoundSet[]="@jar_siege_korriban_stone";
constexpr const char* doorSounds[]={
    "sound/movers/doors/door_stone_start.mp3",
    "sound/movers/doors/door_stone_move.wav",
    "sound/movers/doors/door_stone_stop.mp3",
};
struct Door {
    int model,flags,angle,lip,speed,wait,delay,damage;
    const char* name;
    const char* opens;
    bool sound;
};
constexpr Door doors[]={
    {25,2,180,64,400,-1,0,2,"","",false},
    {26,2,180,64,400,-1,0,2,"","",false},
    {27,9,180,32,90,2,0,2,"scepterstairs","",true},
    {28,2,-2,16,50,2,0,2,"stairbutton","scepterstairs",true},
    {47,3,-2,64,400,-1,0,2,"","",true},
    {48,3,-2,64,400,-1,0,2,"","",true},
    {49,3,-2,64,400,-1,0,2,"","",true},
    {54,2,-2,16,50,2,1,2,"","crystalroom1_plat",true},
    {57,4,-2,0,80,2,0,999,"crusher_2","",true},
    {58,4,-2,0,80,2,0,999,"crusher_1","",true},
    {59,2,180,8,400,-1,0,2,"","",false},
    {60,2,180,8,400,-1,0,2,"","",false},
    {61,12,-2,648,110,2,3,9999,"crystalroom1_plat","",true},
    {62,2,-2,16,50,2,1,2,"","crystalroom1_plat",true},
};
struct Pressure {int model;const char* target;};
constexpr Pressure pressurePads[]={{52,"crusher_2"},{53,"crusher_1"},{55,"crusher_1"},{56,"crusher_2"}};
constexpr int lavaModels[]={1,2,3,4};
constexpr int hazardModels[]={21,22,23,63};
constexpr const char* groupNames[]={"spawnset1","spawnset2","spawnset3"};
constexpr int team1GroupCounts[]={15,15,16};
constexpr int team2GroupCounts[]={16,16,16};
} }
