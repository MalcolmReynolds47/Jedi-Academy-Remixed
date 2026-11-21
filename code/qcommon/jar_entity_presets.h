// Portable workshop setups for native map entities. GPL-2.0-only.
#ifndef JAR_ENTITY_PRESETS_H
#define JAR_ENTITY_PRESETS_H
#include <cstring>
#include "../../shared/qcommon/jar_logic_marker.h"
namespace jar { namespace entities {
enum { Brush=1, Marker=2, Position=4, Feedback=8, Train=16, Model=32, Directional=64, Rail=128 };
constexpr int helperLimit=8;
struct Preset {const char *name,*spawn,*model; unsigned flags;};
// Empty model selects the stock small Imperial crate as a visible control node.
// Brush setups use that model's actual bounds, not an inline BSP model number.
static const Preset presets[]={
 {"misc_bsp","misc_bsp","maps/mp/duel10.bsp",0},
 {"func_door","func_door","models/map_objects/factory/f_door_b.md3",Brush},
 {"func_plat","func_plat","models/map_objects/rocky_ruins/floor_grate.md3",Brush},
 {"func_button","func_button","models/map_objects/imperial/switch.md3",Brush|Feedback},
 {"func_static","func_static","",Brush},
 {"func_rotating","func_rotating","",Brush},
 {"func_bobbing","func_bobbing","",Brush},
 {"func_pendulum","func_pendulum","",Brush},
 {"func_train","func_train","",Brush|Train},
 {"rail_track","rail_track","",Marker|Rail},
 {"rail_lane","rail_lane","",Marker|Rail},
 {"rail_mover","rail_mover","",Model|Rail},
 {"func_wall","func_wall","models/map_objects/rocky_ruins/cell_wall.md3",Brush},
 {"func_usable","func_usable","",Brush},
 {"func_breakable","func_breakable","",Brush},
 {"func_glass","func_breakable","",Brush},
 {"func_group","func_static","",Brush},
 {"func_timer","func_timer","",Marker|Feedback},
 {"trigger_multiple","trigger_multiple","",Brush|Marker|Feedback},
 {"trigger_once","trigger_once","",Brush|Marker|Feedback},
 {"trigger_push","trigger_push","",Brush|Marker|Position},
 {"trigger_teleport","trigger_teleport","",Brush|Marker|Position},
 {"misc_teleporter","trigger_teleport","",Brush|Marker|Position},
 {"trigger_hurt","trigger_hurt","",Brush|Marker},
 {"trigger_bidirectional","trigger_bidirectional","",Brush|Marker|Directional},
 {"trigger_space","trigger_space","",Brush|Marker},
 {"trigger_shipboundary","trigger_shipboundary","",Brush|Marker},
 {"trigger_location","trigger_location","",Brush|Marker},
 {"trigger_visible","trigger_visible","",Marker|Feedback},
 {"trigger_entdist","trigger_entdist","",Marker|Feedback},
 {"trigger_always","trigger_always","",Marker|Feedback},
 {"target_give","target_give","",Marker},
 {"target_delay","target_delay","",Marker|Feedback},
 {"target_score","target_score","",Marker},
 {"target_print","target_print","",Marker},
 {"target_speaker","target_speaker","",Marker},
 {"target_change_parm","target_change_parm","",Marker},
 {"target_scriptrunner","target_scriptrunner","",Marker},
 {"target_level_change","target_level_change","",Marker},
 {"target_play_music","target_play_music","",Marker},
 {"target_laser","target_laser","",Marker},
 {"target_teleporter","target_teleporter","",Marker|Position},
 {"target_relay","target_relay","",Marker|Feedback},
 {"target_kill","target_kill","",Marker},
 {"target_interest","target_interest","",Marker|Feedback},
 {"misc_siege_item","misc_siege_item","models/map_objects/mp/crystal_blue.md3",0},
 {"info_siege_objective","info_siege_objective","",Marker},
 {"target_siege_end","target_siege_end","",Marker},
 {"target_location","target_location","",Marker},
 {"target_position","target_position","",Marker},
 {"target_push","target_push","",Marker},
 {"target_random","target_random","",Marker|Feedback},
 {"target_counter","target_counter","",Marker|Feedback},
 {"target_activate","target_activate","",Marker|Feedback},
 {"target_deactivate","target_deactivate","",Marker|Feedback},
 {"target_gravity_change","target_gravity_change","",Marker},
 {"target_friction_change","target_friction_change","",Marker},
 {"target_autosave","target_autosave","",Marker},
 {"target_secret","target_secret","",Marker},
 {"info_null","info_notnull","",Marker},
 {"info_notnull","info_notnull","",Marker},
 {"info_player_start","info_notnull","",Marker},
 {"info_player_deathmatch","info_notnull","",Marker},
 {"misc_teleporter_dest","info_notnull","",Marker},
 {"path_corner","path_corner","",Marker},
 {"misc_cubemap","misc_cubemap","",Marker},
 {"misc_skyportal","misc_skyportal","",Marker},
 {"point_combat","point_combat","",Marker},
 {"ref_tag","ref_tag","",Marker},
 {"waypoint","waypoint","",Marker},
 {"waypoint_small","waypoint_small","",Marker},
 {"waypoint_navgoal","waypoint_navgoal","",Marker},
 {"light","misc_dlight","",Marker},
 {"misc_dlight","misc_dlight","",Marker},
 {"misc_model","","",Model},
 {"misc_model_static","","",Model},
 {"misc_model_breakable","misc_model_breakable","",Model},
 {"misc_model_ghoul","misc_model_ghoul","models/players/r2d2/model.glm",Model},
 {"fx_wind_zone","fx_wind_zone","",Brush|Marker},
 {"misc_weather_zone","misc_weather_zone","",Brush|Marker},
 {"fx_rain","fx_rain","",Marker},
 {"fx_snow","fx_snow","",Marker},
 {"fx_puff","fx_puff","",Marker},
 {"fx_wind","fx_wind","",Marker},
 {"fx_cloudlayer","fx_cloudlayer","",Marker},
 {"fx_runner","fx_runner","",Marker},
 {"fx_explosion_trail","fx_explosion_trail","",Marker|Position},
 {"fx_target_beam","fx_target_beam","",Marker|Position},
 {"misc_weapon_shooter","misc_weapon_shooter","",Marker},
 {"shooter_rocket","misc_weapon_shooter","",Marker},
 {"shooter_grenade","misc_weapon_shooter","",Marker},
 {"shooter_plasma","misc_weapon_shooter","",Marker},
 {"misc_model_cargo_small","misc_model_cargo_small","models/map_objects/kejim/cargo_small.md3",Model},
 {"misc_turret","misc_turret","models/map_objects/imp_mine/turret_canon.glm",Model},
 {"misc_ns_turret","misc_ns_turret","models/map_objects/imp_mine/turret_canon.glm",Model},
 {"misc_ion_cannon","misc_ion_cannon","models/map_objects/imp_mine/ion_cannon.glm",Model},
 {"misc_panel_turret","misc_panel_turret","models/map_objects/imp_mine/ladyluck_gun.md3",Model},
 {"misc_trip_mine","misc_trip_mine","models/weapons2/laser_trap/laser_trap.md3",Model},
 {"misc_model_shield_power_converter","misc_shield_floor_unit","models/items/a_shield_converter.md3",Model},
 {"misc_sentry_turret","misc_sentry_turret","models/items/psgun.glm",Model},
 {"misc_security_panel","misc_security_panel","models/map_objects/kejim/sec_panel.md3",Model|Feedback},
 {"misc_replicator_item","misc_replicator_item","",Marker},
 {"misc_camera","misc_camera","models/map_objects/kejim/impcam.md3",Model},
 {"misc_model_gun_rack","misc_model_gun_rack","models/map_objects/kejim/weaponsrack.md3",Model},
 {"misc_model_ammo_rack","misc_model_ammo_rack","models/map_objects/kejim/weaponsrung.md3",Model},
 {"misc_portal_surface","misc_portal_surface","models/map_objects/kejim/sec_panel.md3",Model},
 {"misc_portal_camera","misc_portal_camera","models/map_objects/kejim/impcam.md3",Model},
 {"misc_model_jabba_cam","misc_model_jabba_cam","models/map_objects/kejim/sec_panel.md3",Model},
 {"misc_model_welder","misc_model_welder","models/weapons2/demp2/demp2.md3",Model},
 {"misc_camera_track","misc_camera_track","models/map_objects/kejim/impcam.md3",Model|Marker|Train},
 {"misc_camera_focus","misc_camera_focus","",Marker},
 {"misc_spotlight","misc_spotlight","models/map_objects/kejim/impcam.md3",Model|Position},
 {"misc_laser_arm","misc_laser_arm","models/map_objects/kejim/impcam_base.md3",Model},
 {"misc_maglock","misc_maglock","models/map_objects/kejim/sec_panel.md3",Model},
 {"misc_crystal_crate","misc_crystal_crate","models/map_objects/imperial/crate1.md3",Model},
 {"object_cargo_barrel1","object_cargo_barrel1","models/map_objects/imperial/barrels.md3",Model},
};
inline const Preset* Find(const char*name){for(const auto&p:presets)if(!std::strcmp(p.name,name))return &p;return nullptr;}
inline const char* ModelPath(const Preset&p){return p.model[0]?p.model:JAR_LOGIC_PLACEHOLDER_MODEL;}
inline bool LogicMarker(const char* classname, const char* targetname, const char* model){
 if(!classname||!targetname||std::strncmp(targetname,"jar_sb_",7)||!model||std::strcmp(model,JAR_LOGIC_PLACEHOLDER_MODEL))return false;
 const char* digits=targetname+7;
 if(!*digits)return false;
 for(const char* digit=digits;*digit;++digit)if(*digit<'0'||*digit>'9')return false;
 const auto* preset=Find(classname);
 return preset&&(preset->flags&Marker)&&!preset->model[0];
}
inline unsigned HelperCount(const Preset&p){
 unsigned count=p.flags&(Train|Directional|Rail)?2:p.flags&(Position|Feedback)?1:0;
 if(!std::strcmp(p.name,"misc_security_panel")||!std::strcmp(p.name,"misc_camera"))++count;
 if(!std::strcmp(p.name,"misc_model_gun_rack"))count+=3;
 if(!std::strcmp(p.name,"misc_model_ammo_rack"))count+=5;
 if(!std::strcmp(p.name,"misc_maglock"))++count;
 if(!std::strcmp(p.name,"misc_portal_surface")||!std::strcmp(p.name,"misc_portal_camera"))++count;
 if(!std::strcmp(p.name,"misc_crystal_crate"))++count;
 if(!std::strcmp(p.name,"misc_model_jabba_cam"))count+=2;
 if(!std::strcmp(p.name,"misc_laser_arm"))count+=2;
 if(!std::strcmp(p.name,"misc_spotlight"))++count;
 if(!std::strcmp(p.name,"trigger_entdist"))++count;
 if(!std::strcmp(p.name,"misc_camera_focus"))++count;
 if(!std::strcmp(p.name,"misc_siege_item"))++count;
 return count;
}
}}
#endif
