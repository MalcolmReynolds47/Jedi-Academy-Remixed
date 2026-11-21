#include "../../shared/qcommon/jar_thruster.h"
// Original JA:R carried weapon choices and bounded owned Sandbox. GPL-2.0-only.
#include "../cgame/cg_local.h"
#include "g_target_music.h"
#include "g_skyportal.h"
#include "g_scene_camera.h"
#include "g_target_scriptrunner.h"
#include "g_local.h"
#include "g_carried_weapon_art.h"
#include "g_level_change.h"
#include "g_script_parms.h"
#include "g_workshop.h"
#include "g_workshop_placement.h"
#include "g_subbsp_ownership.h"
#include "g_spawn_transaction.h"
#include "g_npc_context.h"
#include "g_workshop_npc_loadout.h"
#include "g_workshop_npc_order.h"
#include "g_workshop_vehicle_status.h"
#include "g_workshop_vehicle_thruster.h"
#include "g_workshop_actor_spawn.h"
#include "g_workshop_pickup_quantity.h"
#include "g_portable_shield.h"
#include "g_rail_portable.h"
#include "g_space.h"
#include "g_shipboundary.h"
#include "g_cloud.h"
#include "g_reflection_probe.h"
#include "g_laser_arm.h"
#include "g_sandbox_laser.h"
#include "g_sandbox_secret.h"
#include "g_sandbox_replicator.h"
#include "g_sandbox_station.h"
#include "g_sandbox_breakable.h"
#include "g_sandbox_usable.h"
#include "g_portal.h"
#include "g_spotlight.h"
#include "g_welder.h"
#include "g_door_camera.h"
#include "g_crystal_crate.h"
#include "g_mover_motion.h"
#include "g_bsp_section.h"
#include "g_siege_item.h"
#include "g_bsp_section_copy.h"
#include "g_weather.h"
#include "g_bidirectional.h"
#include "g_target_markers.h"
#include "g_reference_tags.h"
#include "g_combat_points.h"
#include "g_waypoints.h"
#include "g_volume_geometry.h"
#include "g_always_trigger.h"
#include "g_distance_trigger.h"
#include "g_wall_sandbox.h"
#include "g_sliding_door.h"
#include "g_bobbing_sandbox.h"
#include "g_sandbox_angular.h"
#include "g_angular_cargo.h"
#include "g_movement_settings.h"
#include "g_logic_settings.h"
#include "g_contact_trigger.h"
#include "g_contact_clocks.h"
#include "g_sandbox_push.h"
#include "g_sandbox_teleport.h"
#include "g_sandbox_vehicle_teleport.h"
#include "g_teleport_prop_motion.h"
#include "g_teleport_mover_motion.h"
#include "g_push_prop_motion.h"
#include "g_button_sandbox.h"
#include "g_start_marker.h"
#include "g_randomizer.h"
#include "g_workshop_fx_aim.h"
#include "g_vehicles.h"
#include "b_local.h"
#include "g_functions.h"
extern void AI_DeleteSelfFromGroup(gentity_t*);
extern qboolean G_ValidEnemy(gentity_t*,gentity_t*);
#include "jar_workshop_policy.h"
#include "wp_saber.h"
#include "qcommon/jar_savegame.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <cstdlib>
#include <cmath>
#include <array>
#include <bitset>
#include <map>
#include <set>
#include <thread>
#include <unordered_set>
#include "jar_physics.h"
#include "qcommon/jar_context.h"
#include "qcommon/jar_duplicate.h"
#include "jar_actor_contact.h"
#include "../../shared/qcommon/jar_rope_mesh.h"
#include "../../shared/qcommon/jar_tool_style.h"
#include "../cgame/FxUtil.h"
#include "qcommon/qfiles.h"
#include "../../shared/qcommon/jar_md3_surface.h"
#include "rd-common/mdx_format.h"
extern Vehicle_t *G_IsRidingVehicle(gentity_t*);
extern qboolean PM_LockedAnim(int);
extern qboolean PM_InSaberLock(int);
extern qboolean PM_SuperBreakLoseAnim(int);
extern qboolean PM_SuperBreakWinAnim(int);
extern qboolean PM_InKnockDown(playerState_t*);
extern qboolean PM_InRoll(playerState_t*);
extern void CG_ChangeWeapon(int);
extern void WP_SetSaber(gentity_t*,int,const char*);
extern void WP_RemoveSaber(gentity_t*,int);
extern void WP_SaberFreeStrings(saberInfo_t&);
extern gentity_t *NPC_Spawn_Do(gentity_t*,qboolean);
extern void NPC_PrecacheAnimationCFG(const char*);
extern void NPC_PrecacheByClassName(const char*);
extern char VehicleParms[];
extern bool G_JAR_SpawnWorkshopEntity(gentity_t*,gentity_t**,int&,bool);
#include "qcommon/jar_entity_presets.h"
#include "g_workshop_undo_save.h"
extern gentity_t *LaunchItem(gitem_t*,const vec3_t,const vec3_t,char*);
extern int numSpawnVars;extern qboolean spawning;
namespace {
using namespace jar::workshop;
State state;
bool ready=false,readDone=false,wasEnabled=false;
constexpr uint32_t noEntity=ENTITYNUM_NONE;
#include "g_workshop_grip.inc"
uint32_t held=noEntity,selected=noEntity,holdToken=0,holdSequence=0;
bool physgunConsumedUse=false;
void clearHoldToken(){holdToken=0;physgunGrip={};gi.cvar_set("jar_physgunHeld","0");}
float holdDistance=144;int toolShotUntil=0;vec3_t toolShotEnd{};
bool removeJoint(unsigned);
void releaseHeld(bool freeze=false);void nativeReleaseAll();void nativeRelease(unsigned,bool);
int lastTime=0,page=0;
uint32_t activeTool=0;int toolMode=0,toolButtons=0,previousFamily=0;bool toolRelease=false,toolRestorePending=false;
const char*toolModes[]={"select","rotate","freeze","release","remove","weld","hinge","unlink","duplicate","rope","thruster","activate","link"};
constexpr int toolModeCount=sizeof(toolModes)/sizeof(toolModes[0]);
uint32_t linkFirst=0;
uint32_t jointFirst=0;
int jointPendingMode=0;
void stopTool(){releaseHeld();jointFirst=0;activeTool=0;held=noEntity;toolRelease=true;toolRestorePending=false;toolButtons=0;}
std::vector<uint8_t> prepared;
const char *families[]={"None","Saber","Blaster pistol","E-11 blaster","Disruptor","Bowcaster","Repeater","DEMP2","Flechette","Rocket launcher","Thermal detonator","Trip mine","Det pack","Concussion rifle","Fists"};
struct Prop {uint32_t id;const char *label,*model;int bytes;uint32_t hash;float radius,height,bottom=0;bool rigid=true;};
const std::vector<Prop> props=[](){std::vector<Prop> out={
 {1001,"Small supply crate","models/map_objects/imperial/crate1.md3",4964,1022355355u,21,22},
 {1002,"Banded crate","models/map_objects/imperial/crate_banded.md3",4236,1069308185u,19,34},
 {1003,"Tall crate","models/map_objects/imperial/crate_01.md3",2396,3986609362u,24,43,-1},
 {1004,"Cargo crate","models/map_objects/imperial/crate_02.md3",4924,1454713447u,25,46,-1},
 {1005,"Square crate","models/map_objects/imperial/crate_04.md3",4396,1083384200u,38,28,-18},
 {1006,"Imperial cargo","models/map_objects/imperial/crate.md3",3740,2559623622u,48,55,0},
 {1007,"Barrel pair","models/map_objects/imperial/barrels.md3",8236,2618944792u,39,48,-1},
 {1008,"Factory crate","models/map_objects/factory/s_crate_01.md3",6020,4035183939u,44,64,0},
 {1009,"Factory case","models/map_objects/factory/s_crate_03.md3",7092,2368991634u,41,64,0},
 {1010,"Snow crate","models/map_objects/hoth/crate_snow.md3",3740,912413888u,48,55,0},
 {1011,"Tall snow crate","models/map_objects/hoth/crate_snow2.md3",1772,683805093u,46,93,-1},
 {1012,"Small snow crate","models/map_objects/hoth/crate_snow3.md3",2636,3631113437u,29,40,0},
 // Retail JA carries this Outcast mesh and texture under the Imperial path.
 {1013,"Kejim crate","models/map_objects/imperial/crate_01.md3",2396,3986609362u,24,43,-1},
 {1014,"Mine cargo","models/map_objects/imp_mine/crate.md3",3740,1371904333u,48,55,0},
 {1015,"Rooftop crate","models/map_objects/roof_top/crate3.md3",4836,231865862u,39,73,0},
 {1016,"Red cargo box","models/map_objects/imperial/crate_xplode.md3",4676,1028658004u,35,67,0},
 {1017,"Large factory crate","models/map_objects/factory/s_crate_01_2x.md3",6020,454949764u,88,128,0},
 {1018,"Large factory case","models/map_objects/factory/s_crate_03_2x.md3",7092,223397796u,81,129,0},
 {1019,"Outdoor snow crate","models/map_objects/hoth/crate_snow_outside.md3",3740,2903581380u,48,55,0},
 {1020,"Outdoor small crate","models/map_objects/hoth/crate_snow3_outside.md3",2636,485976641u,29,40,0}
};for(const auto&e:jar::catalog::entries)if(e.kind==1)out.push_back({e.id,e.name,e.model,e.bytes,e.hash,e.radius,e.height,e.bottom,e.rigid});return out;}();
struct Actor {uint32_t id;const char *name,*model;uint32_t npcHash,vehicleHash,alternateHash;float radius,height;bool vehicle;uint32_t npcAlternate=0;};
const std::vector<Actor> actors=[](){std::vector<Actor> out={
 {3001,"stormtrooper","models/players/stormtrooper/model.glm",390902072u,0u,0u,16,64,false},
 {3002,"rebel","models/players/rebel/model.glm",646539101u,0u,0u,16,64,false},
 {3003,"r2d2","models/players/r2d2/model.glm",3719913568u,0u,0u,12,40,false},
 {3004,"imperial","models/players/imperial/model.glm",1997719464u,0u,0u,16,64,false},
 {3005,"rodian","models/players/rodian/model.glm",1059373696u,0u,0u,16,64,false},
 {3006,"gran","models/players/gran/model.glm",3668815691u,0u,0u,16,64,false},
 {3007,"trandoshan","models/players/trandoshan/model.glm",1122137031u,0u,0u,16,64,false},
 {3008,"weequay","models/players/weequay/model.glm",1001443733u,0u,0u,16,64,false},
 {3009,"r5d2","models/players/r5d2/model.glm",1249566534u,0u,0u,12,40,false},
 {3010,"protocol","models/players/protocol/model.glm",2341722717u,0u,0u,12,65,false},
 {3011,"jedi","models/players/jedi/model.glm",3800641957u,0u,0u,16,64,false},
 {3012,"human_merc","models/players/human_merc/model.glm",772473957u,0u,0u,16,64,false},
 {4001,"swoop","models/players/swoop/model.glm",2681780958u,710687386u,140738443u,64,96,true},
 {4002,"tauntaun","models/players/tauntaun/model.glm",2539802644u,463015785u,0u,64,96,true}
};for(const auto&e:jar::catalog::entries)if(e.kind==2||e.kind==3)out.push_back({e.id,e.name,e.model,e.npcHash,e.vehicleHash,0,e.radius,e.height,e.kind==3,e.npcAlternate});return out;}();
struct Supply {uint32_t id;const char*name;itemType_t type;};
const std::vector<Supply> supplies=[](){std::vector<Supply> out={{2501,"item_medpak_instant",IT_HEALTH},{2502,"item_shield_sm_instant",IT_ARMOR},{2503,"item_battery",IT_BATTERY},{2504,"item_shield_lrg_instant",IT_ARMOR}};
 for(const auto&e:jar::catalog::entries)if(e.kind==4){const char*types[]={"IT_BAD","IT_WEAPON","IT_AMMO","IT_ARMOR","IT_HEALTH","IT_HOLDABLE","IT_BATTERY","IT_HOLOCRON","IT_POWERUP"};
  for(unsigned i=1;i<ARRAY_LEN(types);++i)if(!std::strcmp(types[i],e.itemType)){out.push_back({e.id,e.name,itemType_t(i)});break;}}
 return out;}();
const Supply*supply(uint32_t id){static const auto index=[](){std::map<uint32_t,const Supply*> result;for(const auto&s:supplies)result.emplace(s.id,&s);return result;}();const auto found=index.find(id);return found==index.end()?nullptr:found->second;}
gitem_t*supplyItem(const Supply&s){for(int i=1;i<bg_numItems;++i)if(bg_itemlist[i].classname && !std::strcmp(bg_itemlist[i].classname,s.name) && bg_itemlist[i].giType==s.type)return &bg_itemlist[i];return nullptr;}
const Actor *actor(uint32_t id){static const auto index=[](){std::map<uint32_t,const Actor*> result;for(const auto&a:actors)result.emplace(a.id,&a);return result;}();const auto found=index.find(id);return found==index.end()?nullptr:found->second;}
bool enabled(){return gi.cvar("jar_sandbox","0",CVAR_ARCHIVE)->integer==1;}
unsigned propLimit(){return jar::physics::propLimit;}
unsigned propCount(){unsigned count=0;for(unsigned i=0;i<state.ownedCount;++i)count+=jar::workshop::PropCatalog(state.owned[i].catalog);return count;}
void message(const char *s){gi.cvar_set("jar_workshopStatus",s);
 if(gi.JAR_Log)gi.JAR_Log(1,"workshop","status","%s",s);
}
void undoFeedback(){
 cgi_S_StartLocalSound(cgi_S_RegisterSound("sound/interface/esc.mp3"),CHAN_LOCAL);
 gi.cvar_set("jar_undoPopupStatus",gi.cvar("jar_workshopStatus","",0)->string);
 gi.cvar_set("jar_undoPopupUntil",va("%d",cg.time+4500));
}
bool number(const char*s,uint32_t& n){if(!s||!*s)return false;uint64_t v=0;for(;*s;++s){if(*s<'0'||*s>'9')return false;v=v*10+(*s-'0');if(v>UINT32_MAX)return false;}n=uint32_t(v);return true;}
gentity_t *workshopPlayer(){return ready && g_entities[0].inuse && g_entities[0].client ? &g_entities[0]:nullptr;}
bool protectedAnim(int anim){return PM_LockedAnim(anim)||PM_InSaberLock(anim)||PM_SuperBreakLoseAnim(anim)||PM_SuperBreakWinAnim(anim);}
bool usable(){auto*p=workshopPlayer();if(!p)return false;const auto&ps=p->client->ps;
 return NativeControlAllowed(ps,p->health,level.time,in_camera||cg.overrides.active,G_IsRidingVehicle(p)!=nullptr,
   (p->flags&FL_LOCK_PLAYER_WEAPONS)!=0,p->client->clientInfo.animFileIndex==0,
   protectedAnim(ps.torsoAnim)||protectedAnim(ps.legsAnim));
}
bool equipReady(){auto*p=workshopPlayer();return p && NativeEquipAllowed(p->client->ps,usable(),
 PM_InKnockDown(&p->client->ps)!=qfalse,PM_InRoll(&p->client->ps)!=qfalse);}
void reconcile(){if(!workshopPlayer())return;auto&ps=workshopPlayer()->client->ps;state.active=0;
 for(uint32_t f=2;f<=14;++f)if(ps.stats[STAT_WEAPONS]&(1u<<f)){Weapon w;w.family=f;auto id=Add(state,w);if(ps.weapon==int(f))state.active=id;}
 if(ps.stats[STAT_WEAPONS]&(1u<<WP_SABER)){auto w=G_JAR_WorkshopCurrentWeapon();if(w.family==WP_SABER){auto id=Add(state,w);if(ps.weapon==WP_SABER)state.active=id;}}
 for(unsigned f=WP_PHYSGUN;f<=WP_TOOLGUN;++f)if(ps.stats[STAT_WEAPONS]&(1u<<f)){Weapon w;w.family=f;w.tool=f==WP_PHYSGUN?1:2;w.sandbox=1;auto id=Add(state,w);if(ps.weapon==int(f))state.active=id;}
 if(auto*w=Find(state,activeTool))if(enabled() && cg.weaponSelect==int(w->family))state.active=activeTool;
}
bool saberValid(const char *name,bool secondary=false,bool *twoHanded=nullptr){
 if(!NameValid(name,false))return false;saberInfo_t test{};
 bool ok=WP_SaberParseParms(name,&test,qfalse)!=qfalse;
 if(ok){const size_t n=test.model?std::strlen(test.model):0;
  // Native attachment copies into 64 bytes and rewrites MD3 paths. Only
  // directly named Ghoul2 models qualify for transactional inventory equips.
  ok=n>4 && n<64 && !Q_stricmp(test.model+n-4,".glm");
  if(ok){void*bytes=nullptr;const int len=gi.FS_ReadFile(test.model,&bytes);if(bytes)gi.FS_FreeFile(bytes);ok=len>0;}
  if(ok){CGhoul2Info_v probe;ok=gi.G2API_InitGhoul2Model(probe,test.model,0,NULL_HANDLE,NULL_HANDLE,0,0)>=0;gi.G2API_CleanGhoul2Models(probe);}
 }
 if(ok && test.skin && test.skin[0]){void*bytes=nullptr;const int len=gi.FS_ReadFile(test.skin,&bytes);if(bytes)gi.FS_FreeFile(bytes);ok=len>0 && gi.RE_RegisterSkin(test.skin)!=0;}
 if(twoHanded)*twoHanded=(test.saberFlags&SFL_TWO_HANDED)!=0;
 if(secondary && (test.saberFlags&SFL_TWO_HANDED))ok=false;
 WP_SaberFreeStrings(test);return ok;
}
bool choiceValid(const Weapon&w){if(w.family!=WP_SABER)return FamilyValid(w.family);
 bool two=false;if(!saberValid(w.saber[0],false,&two))return false;
 return !w.saber[1][0] || (!two && saberValid(w.saber[1],true));
}
const char* saberLabel(const char*s){if(!std::strcmp(s,"jar_shortblade"))return "Shortblade";if(!std::strcmp(s,"jar_pike"))return "Energy pike";if(!std::strcmp(s,"sith_sword"))return "Sith sword";return s;}
bool spawnCountDirty=true;
void publishSpawnCount(){
 if(!spawnCountDirty)return;
 spawnCountDirty=false;
 unsigned actors=0,props=0,total=0;
 for(const auto& record:state.owned){
  if(record.parent)continue;
  ++total;
  if(g_entities[record.entity].NPC)++actors;
  else if(PropCatalog(record.catalog))++props;
 }
 gi.cvar_set("jar_sandboxCount",va("%u spawned | %u NPCs/vehicles | %u props",total,actors,props));
}
void list(){reconcile();page=std::max(0,std::min(page,int((state.count?state.count-1:0)/8)));
 gi.cvar_set("jar_inventoryPageIndex",va("%d",page));
 gi.cvar_set("jar_inventoryLastPage",va("%u",(state.count?state.count-1:0)/8));
 gi.cvar_set("jar_inventoryPage",va("Page %d / %d | %u carried choices",page+1,std::max(1,int((state.count+7)/8)),state.count));
 for(unsigned row=0;row<8;++row){const unsigned index=page*8+row;const Weapon*w=index<state.count?&state.weapons[index]:nullptr;
  gi.cvar_set(va("jar_inventoryId%u",row),w?va("%u",w->id):"0");
  const bool possessed=w && workshopPlayer() && (workshopPlayer()->client->ps.stats[STAT_WEAPONS]&(1u<<w->family));
  const char *status=!w?"":w->tool&&!enabled()?" [Sandbox off]":!possessed?" [family unavailable]":!choiceValid(*w)?" [asset unavailable]":w->id==state.active?" [equipped]":"";
  char text[240]={};if(w){char label[160];Com_sprintf(label,sizeof(label),"%s%s%s",w->tool==1?"Physics gun":w->tool==2?"Toolgun":w->family==1?saberLabel(w->saber[0]):families[w->family],
    w->saber[1][0]?" + ":"",w->saber[1][0]?saberLabel(w->saber[1]):"");
   if(std::strlen(label)>54){label[51]=0;std::strcat(label,"...");}Com_sprintf(text,sizeof(text),"%s%s",label,status);}
  gi.cvar_set(va("jar_inventoryLabel%u",row),text);
 }
 publishSpawnCount();
}
void equip(uint32_t id){reconcile();auto*w=Find(state,id);auto*p=workshopPlayer();
 if(!w||!p){message("That carried choice is unavailable.");return;}
 if(w->tool && !enabled()){message("Enable Sandbox before selecting a tool weapon.");return;}
  if(!equipReady()){if(gi.JAR_Log)gi.JAR_Log(1,"inventory","deferred","usable=%d inFlight=%d weaponTime=%d weaponState=%d torso=%d legs=%d knockdown=%d roll=%d",usable(),p->client->ps.saberInFlight,p->client->ps.weaponTime,p->client->ps.weaponstate,p->client->ps.torsoAnim,p->client->ps.legsAnim,PM_InKnockDown(&p->client->ps),PM_InRoll(&p->client->ps));message("Finish the current action before changing weapons.");return;}
 if(!(p->client->ps.stats[STAT_WEAPONS]&(1u<<w->family))){message("This mission has not granted that weapon family.");return;}
 if(!choiceValid(*w)){message("Required weapon definition or model is unavailable; current loadout kept.");return;}
 // All definitions validated before the native setter removes any old model.
 if(w->family==WP_SABER){
  WP_SaberFreeStrings(p->client->ps.saber[0]);WP_SaberFreeStrings(p->client->ps.saber[1]);
  WP_SetSaber(p,0,w->saber[0]);
  if(w->saber[1][0])WP_SetSaber(p,1,w->saber[1]);else WP_RemoveSaber(p,1);
  for(int s=0;s<2;++s)for(int b=0;b<8;++b)p->client->ps.saber[s].blade[b].color=saber_colors_t(w->colors[s][b]);
  gi.cvar_set("g_saber",w->saber[0]);gi.cvar_set("g_saber2",w->saber[1]);
  const char*colors[]={"red","orange","yellow","green","blue","purple"};
  gi.cvar_set("g_saber_color",colors[w->colors[0][0]]);gi.cvar_set("g_saber2_color",colors[w->colors[1][0]]);
 }
 // Use ordinary player selection; do not force ps.weapon or refill ammo.
 const uint32_t requestedTool=w->tool?w->id:0;
  if(requestedTool && !activeTool)previousFamily=p->client->ps.weapon;
  if(activeTool || requestedTool)stopTool();
  CG_ChangeWeapon(int(w->family));
 if(cg.weaponSelect!=int(w->family)){message("That weapon cannot currently be selected with its available ammo.");return;}
 activeTool=requestedTool;
  message(w->tool==1?"Physics gun: hold primary to grab; wheel changes distance; release to throw; secondary freezes.":w->tool==2?"Toolgun: primary applies the mode to your aimed object; secondary uses the selected tool alternate action.":"Changing weapon.");
 if(gi.JAR_Log)gi.JAR_Log(1,"inventory","equip_requested","record=%u family=%u first=%s second=%s",w->id,w->family,w->saber[0],w->saber[1]);
 list();
}
const Prop *prop(uint32_t id){static const auto index=[](){std::map<uint32_t,const Prop*> result;for(const auto&p:props)result.emplace(p.id,&p);return result;}();const auto found=index.find(id);return found==index.end()?nullptr:found->second;}
bool propValid(const Prop&p){void*data=nullptr;int bytes=gi.FS_ReadFile(p.model,&data);uint32_t hash=2166136261u;
 if(bytes==p.bytes && data)for(int i=0;i<bytes;++i)hash=(hash^static_cast<uint8_t*>(data)[i])*16777619u;
 if(data)gi.FS_FreeFile(data);return bytes==p.bytes && hash==p.hash;
}
Owned *owned(uint32_t entity){return OwnedEntity(state,entity);}
const char* workshopMelee(uint32_t);
bool matches(const Owned&o){if(o.entity>=ENTITYNUM_WORLD)return false;auto&e=g_entities[o.entity];
 if(o.catalog==consoleCatalog)return e.inuse;
 if(!e.inuse||!e.targetname||std::strcmp(e.targetname,va("jar_sb_%u",o.id)))return false;
 if(const auto*p=prop(o.catalog)){
  if(e.s.eType!=ET_GENERAL||!e.classname||std::strcmp(e.classname,"jar_sandbox_prop")||!e.model)return false;
  // A save made with the original model installed still owns the same crate.
  return !std::strcmp(e.model,p->model)||(o.catalog==1013&&!std::strcmp(e.model,"models/map_objects/kejim/crate_01.md3"));
 }
 if(const auto*a=actor(o.catalog))return e.client && e.NPC && e.NPC_type && !Q_stricmp(e.NPC_type,a->name) && bool(e.m_pVehicle)==a->vehicle;
  if(const auto*s=supply(o.catalog))return e.s.eType==ET_ITEM && e.item && e.item==supplyItem(*s);
 if(const char*name=workshopMelee(o.catalog))return e.s.eType==ET_ITEM&&e.item&&e.item->giType==IT_WEAPON&&e.item->giTag==WP_SABER&&e.NPC_type&&!strcmp(e.NPC_type,name);
 if(const auto*d=jar::catalog::Find(o.catalog))if(d->kind==5){
  if(const auto*section=jar::bspSection::Catalog(d->name))return jar::bspSection::MatchesCard(e,*section);
  return e.classname&&!std::strcmp(e.classname,d->name);
 }
  return e.s.eType==ET_ITEM && e.item && e.item->giType==IT_WEAPON && uint32_t(e.item->giTag)==o.catalog-2000;
}
bool nativeOwnedControl(unsigned);
bool occupied(const gentity_t&e){
 if(g_entities[0].client && g_entities[0].client->ps.viewEntity>0 && g_entities[0].client->ps.viewEntity==e.s.number)return true;
 for(int i=0;i<globals.num_entities;++i)if(g_entities[i].inuse && g_entities[i].client && g_entities[i].owner==&e && (g_entities[i].client->ps.eFlags&EF_LOCKED_TO_WEAPON))return true;
 if(e.client){
  if(e.s.m_iVehicleNum || (e.owner && e.owner->inuse && e.owner->m_pVehicle))return true;
  if((e.client->ps.eFlags&EF_FORCE_GRIPPED)&&!nativeOwnedControl(e.s.number))return true;
  if(e.client->ps.eFlags&(EF_LOCKED_TO_WEAPON|EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE|EF_FORCE_DRAINED))return true;
  if(g_entities[0].client && g_entities[0].client->ps.viewEntity>0 && g_entities[0].client->ps.viewEntity==e.s.number)return true;
 }
 if(!e.m_pVehicle)return false;
 if(e.m_pVehicle->m_pPilot || e.m_pVehicle->m_pOldPilot || e.m_pVehicle->m_pDroidUnit || e.m_pVehicle->m_iBoarding)return true;
 for(int i=0;i<globals.num_entities;++i){const auto&r=g_entities[i];if(r.inuse && r.client && (r.owner==&e || r.s.m_iVehicleNum==e.s.number))return true;}
 return false;
}
bool removalOccupied(const gentity_t& e){return occupied(e)||G_BSPSectionSupportsActors(e);}
#include "g_workshop_remove.inc"
void cleanup(){releaseHeld();nativeReleaseAll();held=selected=noEntity;jointFirst=0;
 while(state.jointCount)removeJoint(state.joints[0].id);
 for(int i=int(state.ownedCount)-1;i>=0;--i)if(unsigned(i)<state.ownedCount&&removalRoot(state.owned[i]))remove(state.owned[i].entity);
 message(state.ownedCount?"Cleanup deferred for occupied or controlled actors. Exit or release them safely.":"Sandbox objects cleaned up. Carried inventory retained.");
}
// Only a qualified actor's native helper is removed; arbitrary owner pointers
// never turn authored entities into Sandbox objects.
void freeActorHelpers(gentity_t*e){
 if(!e->NPC)return;
 G_WorkshopFreeActorSaber(*e);
 auto*goal=e->NPC->tempGoal;e->NPC->tempGoal=nullptr;
 if(goal && goal->inuse && goal->owner==e && goal->classname && !std::strcmp(goal->classname,"NPC_goal"))G_FreeEntity(goal);
 if(e->NPC->combatPoint>=0){NPC_FreeCombatPoint(e->NPC->combatPoint);e->NPC->combatPoint=-1;}
 if(e->NPC->group)AI_DeleteSelfFromGroup(e);
}
#include "qcommon/jar_vehicle_variants.h"
bool vehicleDefinition(const Actor&a){
 COM_BeginParseSession();const char*skin=nullptr;const char*p=JAR_ResolveVehicleBlock(VehicleParms,a.name,&skin);
 const auto hash=JAR_VehicleBlockHash(p);const bool ok=hash&&(skin?JAR_StockSwoopHash(hash):hash==a.vehicleHash||hash==a.alternateHash);
 COM_EndParseSession();return ok;
}
#include "g_workshop_actor_catalog.inc"
#include "g_workshop_pickup.inc"
gentity_t*spawnActor(const Actor&a,const vec3_t pos,std::string& failure){
 NPC_PrecacheAnimationCFG(a.name);NPC_PrecacheByClassName(a.name);
 auto*spawner=G_TrySpawn();if(!spawner){failure="native entity pool is full";return nullptr;}
 spawner->classname=G_NewString(a.vehicle?"NPC_Vehicle":"jar_sandbox_spawner");spawner->NPC_type=G_NewString(a.name);
 spawner->spawnflags|=JAR_NPC_OVERLAP_SPAWN;
 spawner->count=1;spawner->wait=-1;spawner->e_UseFunc=useF_NULL;
 G_SetOrigin(spawner,pos);VectorCopy(pos,spawner->s.origin);
 vec3_t angles={0,workshopPlayer()->client->ps.viewangles[YAW],0};G_SetAngles(spawner,angles);VectorCopy(angles,spawner->s.angles);
 const auto spawnerLifetime=G_EntityLifetime(spawner);
 auto*e=NPC_Spawn_Do(spawner,qtrue);
 if(!e){
  const bool originalSpawner=G_EntityLifetime(spawner)==spawnerLifetime;
  failure=originalSpawner&&spawner->e_ThinkFunc==thinkF_NPC_Spawn_Go?
   "native entity pool cannot reserve an actor and goal":"native NPC initialization failed";
  if(originalSpawner)G_FreeEntity(spawner);
  return nullptr;
 }
 if(const char* reason=G_WorkshopActorSpawnFailure(*e,pos,a.vehicle,a.height)){
  failure=reason;
  if(e->inuse){freeActorHelpers(e);G_FreeEntity(e);}
  return nullptr;
 }
 // A requested Sandbox mount waits for its rider. Its native vehicle
 // controller continues to handle boarding, motion, weapons and dismounting.
 if(a.vehicle && e->m_pVehicle)e->NPC->behaviorState=e->NPC->defaultBehavior=BS_CINEMATIC;
 return e;
}
bool sandboxReady(){if(!enabled()){message("Enable Sandbox to use its spawn and tool actions.");return false;}if(!usable()){message("Sandbox actions require a living, freely controlled humanoid player.");return false;}return true;}
void eye(vec3_t pos,vec3_t direction){auto*p=workshopPlayer();VectorCopy(p->currentOrigin,pos);pos[2]+=p->client->ps.viewheight;AngleVectors(p->client->ps.viewangles,direction,nullptr,nullptr);}
void refreshPhysgunView(){
 vec3_t origin,direction;G_JAR_WorkshopViewRay(origin,direction);
 physgunViewOrigin={origin[0],origin[1],origin[2]};
 physgunViewDirection={direction[0],direction[1],direction[2]};
 // Use the same rendered camera orientation as the aim ray, including
 // third-person yaw offsets and when the camera is looking straight up.
 physgunViewYaw=VectorLengthSquared(cg.refdef.viewaxis[0])>0
  ? cg.refdefViewAngles[YAW] : workshopPlayer()->client->ps.viewangles[YAW];
}
void rememberEdit(gentity_t&,bool);
#include "g_workshop_children.inc"
#include "g_workshop_bounds.inc"
#include "g_workshop_prop_trace.inc"
int physicsFrozenContents(const gentity_t&);
void grant(const char*,bool select=false);
#include "g_workshop_weapons.inc"
gentity_t*spawn(uint32_t catalog,const float*requestedOrigin=nullptr,const float*requestedAngles=nullptr,
 int poseFrame=0,bool wallStartsOff=false,bool pastePortal=false,WorkshopSpawnFeedback feedback=WorkshopSpawnFeedback::Individual);
gentity_t*spawnImpl(uint32_t catalog,const float*requestedOrigin,const float*requestedAngles,int poseFrame,
 bool wallStartsOff,bool pastePortal,WorkshopSpawnFeedback feedback){if(!sandboxReady())return nullptr;
 const auto*entry=jar::catalog::Find(catalog);
 if(entry && entry->alias){return spawn(entry->alias,requestedOrigin,requestedAngles,poseFrame,wallStartsOff,pastePortal,feedback);}
 if(entry && entry->reason[0]){message(entry->reason);return nullptr;}
 const auto*entityDef=entry && entry->kind==5?entry:nullptr;
 const auto*section=entityDef?jar::bspSection::Catalog(entityDef->name):nullptr;
 unsigned needed=1;
 if(entityDef&&!pastePortal)if(const auto*p=jar::entities::Find(entityDef->name)){
  needed+=jar::entities::HelperCount(*p);
 }
 if(needed>ownedLimit-state.ownedCount||needed>jar::constraint::nativeBit-state.nextOwned){message("Sandbox object/helper capacity reached. Undo or clean spawned objects first.");return nullptr;}
 const Prop*definition=prop(catalog);const Actor*actorDef=actor(catalog);Actor loadedActor{};std::string loadedActorModel,actorFailure;gitem_t*item=nullptr;
 if(definition){if(propCount()>=propLimit()){message("The fixed 4096-prop capacity is full. Undo or clean spawned objects.");return nullptr;}if(!propValid(*definition)){message("Prop unavailable: installed model differs from the qualified stock asset.");return nullptr;}}
 else if(actorDef){loadedActor=*actorDef;if(!actorValid(loadedActor,loadedActorModel,actorFailure)){message(va("%s unavailable: %s",actorDef->name,actorFailure.c_str()));if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","actor_rejected","name=%s reason=%s",actorDef->name,actorFailure.c_str());return nullptr;}actorDef=&loadedActor;}
  else if((catalog>=2002&&catalog<=2013)||(catalog>=2101&&catalog<=2103))item=workshopWeaponItem(catalog);
  else if(const auto*s=supply(catalog))item=supplyItem(*s);
 else if(!entityDef){message("Unsupported spawn category or definition.");return nullptr;}
 vec3_t start,dir,end,mins,maxs;
 vec3_t sectionAngles={0,workshopPlayer()->client->ps.viewangles[YAW],0};
 if(requestedAngles)VectorCopy(requestedAngles,sectionAngles);
 // Match the rendered crosshair, including third-person camera offset.
 VectorCopy(cg.refdef.vieworg,start);VectorCopy(cg.refdef.viewaxis[0],dir);VectorMA(start,4096,dir,end);
 if(definition){if(!propHull(*definition,requestedAngles?requestedAngles:vec3_origin,mins,maxs,poseFrame)){message("This model has no usable visible bounds for the requested pose; nothing spawned.");return nullptr;}}
 else if(actorDef){VectorSet(mins,-actorDef->radius,-actorDef->radius,-24);VectorSet(maxs,actorDef->radius,actorDef->radius,actorDef->height-24);}
 else if(entityDef){
  if(section){std::string reason;if(!G_BSPSectionBounds(*section,sectionAngles,mins,maxs,reason)){message(va("Map section unavailable: %s",reason.c_str()));return nullptr;}}
  else if(JAR_WallClass(entityDef->name)){
   const vec3_t wallAngles={0,workshopPlayer()->client->ps.viewangles[YAW],0};
   G_WallHull(requestedAngles?requestedAngles:wallAngles,mins,maxs);
  }
  else if(G_SandboxSlideClass(entityDef->name))G_SandboxSlideHull(sectionAngles,mins,maxs);
  else if(G_LiftClass(entityDef->name))G_SandboxSlideHull(sectionAngles,mins,maxs,true);
  else if(G_ButtonClass(entityDef->name))G_ButtonHull(sectionAngles,mins,maxs);
  else if(G_BobbingClass(entityDef->name))G_BobbingHull(sectionAngles,mins,maxs);
  else if(jar::sandboxAngular::Class(entityDef->name))G_SandboxAngularHull(sectionAngles,mins,maxs);
  else {VectorSet(mins,-entityDef->radius,-entityDef->radius,entityDef->bottom);VectorSet(maxs,entityDef->radius,entityDef->radius,entityDef->height);}
 }
  else {
   if(!item || !item->world_model || item->quantity<0 || item->quantity>1000){message("Pickup definition unavailable or outside native quantity bounds.");return nullptr;}
   void*bytes=nullptr;const int length=gi.FS_ReadFile(item->world_model,&bytes);if(bytes)gi.FS_FreeFile(bytes);
   if(length<=0){message("Pickup model unavailable.");return nullptr;}
   VectorCopy(item->mins,mins);VectorCopy(item->maxs,maxs);
   if(VectorCompare(mins,vec3_origin) && VectorCompare(maxs,vec3_origin)){VectorSet(mins,-16,-16,-16);VectorSet(maxs,16,16,16);}
   for(int i=0;i<3;++i)if(!std::isfinite(mins[i])||!std::isfinite(maxs[i])||mins[i]>=maxs[i]||std::fabs(mins[i])>128||std::fabs(maxs[i])>128){message("Pickup collision bounds are not qualified.");return nullptr;}
  }
 trace_t aim{};vec3_t origin;trace_t tr{};
 if(requestedOrigin){VectorCopy(requestedOrigin,origin);VectorCopy(origin,aim.endpos);aim.plane.normal[2]=1;}else {
 gi.trace(&aim,start,nullptr,nullptr,end,0,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 if(actorDef && (aim.startsolid||aim.allsolid||aim.plane.normal[2]<.7f)){
  // Aim through existing actors and props when their sides obscure the floor.
  G_JAR_WorkshopSceneryTrace(aim,start,end,nullptr,nullptr,0);
  if(aim.startsolid||aim.allsolid||aim.fraction>=1||aim.plane.normal[2]<.7f){message("NPCs and vehicles need a level surface under the crosshair.");return nullptr;}
 }
 if(aim.startsolid || aim.allsolid || aim.fraction>=1){message("Aim at a surface within 4096 units to spawn here.");return nullptr;}
 JarSweepHull convex;
 const auto* shape=definition&&propSceneryHull(*definition,poseFrame,requestedAngles?requestedAngles:vec3_origin,convex)?&convex:nullptr;
 float offset=G_JAR_WorkshopSurfaceOffset(mins,maxs,aim.plane.normal,shape);
 if(entityDef){
  const auto angular=jar::sandboxAngular::Class(entityDef->name);
  if(angular==jar::sandboxAngular::Pendulum)offset+=8; // Reserve the default 15-degree swing.
  if(angular==jar::sandboxAngular::Spinner)offset+=10*std::hypot(aim.plane.normal[0],aim.plane.normal[1]);
 }
 VectorMA(aim.endpos,offset,aim.plane.normal,origin);
 if(section){
  vec3_t center;for(int a=0;a<3;++a)center[a]=(mins[a]+maxs[a])*.5f;
  VectorMA(origin,DotProduct(center,aim.plane.normal),aim.plane.normal,origin);VectorSubtract(origin,center,origin);
 }
 if(entityDef&&jar::weather::ZoneClass(entityDef->name)&&G_WeatherZoneCount()>=jar::weather::zoneLimit){message("Weather volume budget reached (32). Remove a weather volume before adding another.");return nullptr;}
 if((!entityDef||!jar::volumeEntity::Class(entityDef->name))&&
    !G_JAR_WorkshopPlaceHull(origin,mins,maxs,aim.plane.normal,actorDef!=nullptr,shape)){
  message("The aimed surface has insufficient space against the map geometry.");return nullptr;
 }
 }
 if(section&&!G_JAR_WorkshopSectionInWorld(origin,mins,maxs)){
  message("The map section extends outside the world limits.");return nullptr;
 }
 VectorCopy(origin,tr.endpos);
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","aim_placement","catalog=%u hit=%.2f,%.2f,%.2f normal=%.2f,%.2f,%.2f origin=%.2f,%.2f,%.2f",catalog,aim.endpos[0],aim.endpos[1],aim.endpos[2],aim.plane.normal[0],aim.plane.normal[1],aim.plane.normal[2],origin[0],origin[1],origin[2]);
 gentity_t*e=nullptr;gentity_t*helpers[jar::entities::helperLimit]{};int helperCount=0;
 if(entityDef&&jar::reflection::Class(entityDef->name)&&G_ReflectionCount()>=jar::reflection::limit){message("Reflection probe capacity reached (64). Remove a probe before adding another.");return nullptr;}
 if(entityDef){e=G_TrySpawn();if(e){e->classname=G_NewString(entityDef->name);e->model=G_NewString(entityDef->model);VectorCopy(mins,e->mins);VectorCopy(maxs,e->maxs);
  if(wallStartsOff&&JAR_WallClass(entityDef->name))e->spawnflags|=1;
  G_SetOrigin(e,tr.endpos);VectorCopy(tr.endpos,e->s.origin);vec3_t angle={0,workshopPlayer()->client->ps.viewangles[YAW],0};if(!strcmp(entityDef->name,"misc_trip_mine")){vec3_t toward;VectorScale(aim.plane.normal,-1,toward);vectoangles(toward,angle);}
  if(!requestedAngles&&(!strcmp(entityDef->name,"misc_maglock")||jar::portal::SurfaceClass(entityDef->name)))angle[YAW]=AngleNormalize180(angle[YAW]+180);
  if(!requestedAngles&&!requestedOrigin&&jar::portal::SurfaceClass(entityDef->name)&&std::fabs(aim.plane.normal[2])<.7f)
   vectoangles(aim.plane.normal,angle);
  if(!requestedAngles&&jar::skyPortal::Class(entityDef->name))VectorClear(angle); // Sky angles are an offset from the player view.
  if(requestedAngles)VectorCopy(requestedAngles,angle);G_SetAngles(e,angle);VectorCopy(angle,e->s.angles);
  if(jar::entities::Find(entityDef->name)||jar::sandboxStation::Class(entityDef->name))
   e->targetname=G_NewString(va("jar_sb_%u",state.nextOwned));
  if(!G_JAR_SpawnWorkshopEntity(e,helpers,helperCount,pastePortal)||!e->inuse){if(e->inuse)G_FreeEntity(e);e=nullptr;}
  else if(!requestedOrigin&&!section&&!jar::entities::Find(entityDef->name)){float rest=1;for(int axis=0;axis<3;++axis)rest-=aim.plane.normal[axis]*(aim.plane.normal[axis]>=0?e->mins[axis]:e->maxs[axis]);
   vec3_t placed;VectorMA(aim.endpos,rest,aim.plane.normal,placed);G_SetOrigin(e,placed);gi.linkentity(e);
   trace_t check{};G_JAR_WorkshopSceneryTrace(check,e->currentOrigin,e->currentOrigin,e->mins,e->maxs,e->s.number);
   if(check.startsolid||check.allsolid){G_FreeEntity(e);message("The native entity needs more clear space around the aimed surface.");return nullptr;}}}}
 else if(actorDef)e=spawnActor(*actorDef,tr.endpos,actorFailure);
 else if(definition){e=G_TrySpawn();if(e){e->classname=G_NewString("jar_sandbox_prop");e->model=G_NewString(definition->model);e->s.eType=ET_GENERAL;e->s.frame=poseFrame;e->s.modelindex=G_ModelIndex(e->model);
   VectorCopy(mins,e->mins);VectorCopy(maxs,e->maxs);e->contents=CONTENTS_BODY;e->clipmask=MASK_SOLID|CONTENTS_BODY;e->takedamage=qfalse;
   G_SetOrigin(e,tr.endpos);if(requestedAngles)G_SetAngles(e,requestedAngles);
   if(!Q_stricmp(COM_GetExtension(e->model),"glm")){e->playerModel=gi.G2API_InitGhoul2Model(e->ghoul2,e->model,e->s.modelindex,NULL_HANDLE,NULL_HANDLE,0,0);if(e->playerModel<0||!skeletonPose(e->ghoul2,poseFrame)){G_FreeEntity(e);e=nullptr;}}
   if(e){e->contents=physicsFrozenContents(*e);gi.linkentity(e);}}}
 else if(workshopMelee(catalog)){
  vec3_t angles={0,workshopPlayer()->client->ps.viewangles[YAW],90};if(requestedAngles)VectorCopy(requestedAngles,angles);
  e=workshopDropMelee(catalog,tr.endpos,angles);
 }else if(item){RegisterItem(item);CG_RegisterItemVisuals(item-bg_itemlist);e=LaunchItem(item,tr.endpos,vec3_origin,nullptr);if(e){
  if(!workshopPickupModel(*e)){G_FreeEntity(e);message("Pickup model could not initialize; nothing retained.");return nullptr;}
  e->count=item->quantity>0?item->quantity:item->giType==IT_WEAPON?50:0;
 }}
 if(e&&!e->inuse)e=nullptr;
 if(!e){
  message(actorDef?va("%s could not spawn: %s.",actorDef->name,actorFailure.c_str()):"Optional entity capacity is exhausted; no Sandbox object created.");
  if(actorDef&&gi.JAR_Log)gi.JAR_Log(1,"sandbox","actor_spawn_failed","name=%s reason=%s",actorDef->name,actorFailure.c_str());
  return nullptr;
 }
 if(!Own(state,e->s.number,catalog)){while(helperCount)G_FreeEntity(helpers[--helperCount]);if(actorDef)freeActorHelpers(e);G_FreeEntity(e);message("Sandbox registry is full.");return nullptr;}
 spawnCountDirty=true;
 if(!e->targetname)e->targetname=G_NewString(va("jar_sb_%u",state.owned[state.ownedCount-1].id));
 const auto parent=state.owned[state.ownedCount-1].id;
 for(int i=0;i<helperCount;++i){auto*h=helpers[i];const uint32_t helperCatalog=childCatalog(*h);
  if(!helperCatalog || !Own(state,h->s.number,helperCatalog,parent)){for(int j=i;j<helperCount;++j)G_FreeEntity(helpers[j]);remove(e->s.number);message("Setup registry exhausted; spawn rolled back.");return nullptr;}
  char*oldName=h->targetname;h->targetname=G_NewString(va("jar_sb_%u",state.owned[state.ownedCount-1].id));
  if(oldName && e->target && !std::strcmp(e->target,oldName)){if(gi.bIsFromZone(e->target,TAG_G_ALLOC))gi.Free(e->target);e->target=G_NewString(h->targetname);}
  if(oldName && e->target2 && !std::strcmp(e->target2,oldName)){if(gi.bIsFromZone(e->target2,TAG_G_ALLOC))gi.Free(e->target2);e->target2=G_NewString(h->targetname);}
  if(oldName && e->paintarget && !std::strcmp(e->paintarget,oldName)){if(gi.bIsFromZone(e->paintarget,TAG_G_ALLOC))gi.Free(e->paintarget);e->paintarget=G_NewString(h->targetname);}
  for(int j=0;j<helperCount;++j)if(oldName && helpers[j]->target && !std::strcmp(helpers[j]->target,oldName)){if(gi.bIsFromZone(helpers[j]->target,TAG_G_ALLOC))gi.Free(helpers[j]->target);helpers[j]->target=G_NewString(h->targetname);}
  if(oldName && gi.bIsFromZone(oldName,TAG_G_ALLOC))gi.Free(oldName);
 }
 if(feedback==WorkshopSpawnFeedback::Individual){
  selected=e->s.number;rememberEdit(*e,true);
  message(section?va("%s created. Activate enters a clear local start point. Physgun moves the section; Undo removes it.",section->title):entityDef?"Entity ready. Activate fires/toggles it; Link connects source to target. Undo removes its setup.":actorDef?(actorDef->vehicle?"Owned vehicle created. Use it to board; exit before Undo or cleanup.":"Owned NPC created with native AI and allegiance. Undo removes this actor and its helper."):definition?"Owned prop created frozen. Aim and select a tool; Release adds gravity.":"Owned pickup created; native quantities and 30-second expiry apply.");
  list();
 }
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","spawn","entity=%d id=%u catalog=%u",e->s.number,owned(e->s.number)->id,catalog);
 return e;
}
#include "g_workshop_spawn.inc"
bool setupMoved(gentity_t&e){
 if(G_SandboxUsable(e))return true;
 if(G_SandboxBreakable(e))return e.count!=jar::sandboxBreakable::Intact||
  jar::sandboxBreakable::Class(e.classname)!=jar::sandboxBreakable::Model;
 if(G_SandboxSlideMoved(e)||G_ButtonMoved(e)||G_BobbingMoved(e))return true;
 if(G_SandboxAngularMoved(e))return true;
 if(G_WallPortable(e)){G_WallRefresh(e);return true;}
 if(G_BSPSectionMoved(e))return true;
 if(G_RailMoved(e))return true;
 if(G_PortableShieldMoved(e))return true;
 if(G_SceneCameraMoved(e))return true;
 if(G_SkyPortalMoved(e)||G_CrystalCrateMoved(e)||G_DoorCameraMoved(e)||G_WelderMoved(e)||G_SpotlightMoved(e)||G_PortalMoved(e)||G_LaserArmMoved(e)||G_ReflectionMoved(e)||G_CloudMoved(e)||(G_WeatherPortable(e)&&!jar::weather::ZoneClass(e.classname)))return true;
 if(jar::volumeEntity::Refresh(e)){VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);return true;}
 if(const auto*o=owned(e.s.number))if(const auto*p=prop(o->catalog)){propHull(*p,e.currentAngles,e.mins,e.maxs,e.s.frame);e.contents=physicsFrozenContents(e);return true;}
 if(!owned(e.s.number) || !e.classname || !jar::entities::Find(e.classname))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);
 if(jar::siege::Item(e)&&!e.activator&&!e.delay&&!(e.spawnflags&jar::siege::delivered))
  VectorCopy(e.currentOrigin,e.pos1);
 if(!strcmp(e.classname,"misc_camera")){
  const auto parent=owned(e.s.number)->id;
  if(const auto* children=OwnedChildren(state,parent))for(const auto child:*children){
   const auto* record=OwnedId(state,child);if(!record)continue;
   auto& h=g_entities[record->entity];
   if(h.inuse&&h.model&&!strcmp(h.model,"models/map_objects/kejim/impcam_base.md3")){
    vec3_t pos;VectorCopy(e.currentOrigin,pos);pos[2]+=16;G_SetOrigin(&h,pos);G_SetAngles(&h,e.currentAngles);gi.linkentity(&h);
   }
  }
 }
 if(G_PushKind(e)==jar::push::Marker)return true;
 for(unsigned i=0;i<state.ownedCount;++i){auto&source=g_entities[state.owned[i].entity];
  if(source.classname && source.target && (e.s.number==source.s.number || (e.targetname&&!strcmp(source.target,e.targetname))) &&
    !G_PushKind(source)&&(!strcmp(source.classname,"trigger_push") || !strcmp(source.classname,"target_push"))){source.e_ThinkFunc=thinkF_AimAtTarget;source.nextthink=level.time+FRAMETIME;}
 }return true;
}
void physicsInvalidate(unsigned entity);
bool removeJoint(unsigned id);
#include "g_workshop_rack_pose.inc"
#include "g_workshop_native_control.inc"
#include "g_workshop_pickup_lifetime.inc"
bool setPropPose(gentity_t&,int);
uint32_t attachmentKey(const gentity_t&);
#include "g_workshop_give.inc"
#include "g_workshop_edits.inc"
#include "g_workshop_undo_history.inc"
#include "g_workshop_use_gate.inc"
#include "g_workshop_physics.inc"
#include "g_workshop_pose.inc"
#include "g_workshop_attachments.inc"
#include "g_workshop_assembly_teleport.inc"
int teleportVolumesFrame=-1;
unsigned teleportVolumesMask=0;
unsigned teleportObjectVolumes(){
 if(teleportVolumesFrame==level.time)return teleportVolumesMask;
 teleportVolumesFrame=level.time;teleportVolumesMask=0;
 if(ready&&enabled())for(unsigned i=0;i<state.ownedCount;++i){
  const auto& source=g_entities[state.owned[i].entity];jar::teleport::Settings settings;
  if(G_TeleportKind(source)!=jar::teleport::Volume||!G_TeleportRead(source,settings)||!settings.enabled)continue;
  if(settings.props)teleportVolumesMask|=jar::teleport::Prop;
  if(settings.projectiles)teleportVolumesMask|=jar::teleport::Projectile;
  if(settings.props&&settings.assemblies)teleportVolumesMask|=jar::teleport::Assembly;
  if(settings.props&&settings.assemblies&&settings.movers)teleportVolumesMask|=jar::teleport::Mover;
  if(teleportVolumesMask==(jar::teleport::Prop|jar::teleport::Projectile|jar::teleport::Assembly|jar::teleport::Mover))break;
 }
 return teleportVolumesMask;
}
bool teleportPropContacts(gentity_t& prop,const vec3_t previous,bool rigid,bool clipped,const float* previousAngles){
 if(!(teleportObjectVolumes()&jar::teleport::Prop)||held==unsigned(prop.s.number))return false;
 const auto* record=owned(prop.s.number);
 if(!record||record->frozen)return false;
 if(Constrained(state,record->id)){
  if(!(teleportObjectVolumes()&jar::teleport::Assembly))return false;
  auto crossings=G_TeleportCrossings(prop,previous,prop.currentOrigin,clipped,true);
  if(crossings.empty())return false;
  PendingAssemblyCrossing pending{record->id,{},rigid,clipped,std::move(crossings)};
  VectorCopy(previous,pending.previous);
  if(previousAngles)VectorCopy(previousAngles,pending.previousAngles);
  pendingAssemblyCrossings.push_back(std::move(pending));
  if(rigid)gi.linkentity(&prop);
  return true;
 }
 return G_TeleportSweepObject(prop,previous,prop.currentOrigin,clipped,rigid,
  [](gentity_t& body,const vec3_t origin,const gentity_t&,const jar::teleport::Settings& settings,bool box){
   if(!G_TeleportPropMotion(physicsWorld,body,origin,settings.keepVelocity,box))return false;
   const int contents=physicsFrozenContents(body);
   if(contents!=body.contents){body.contents=contents;gi.linkentity(&body);}
   return true;
  });
}
bool pushPropVolumes=false;
void pushPropContacts(gentity_t& e,const vec3_t previous,bool rigid){
 if(!pushPropVolumes||held==unsigned(e.s.number))return;
 const auto* record=owned(e.s.number);
 if(!record||record->frozen)return;
 G_PushPropSweep(e,previous,rigid,[](gentity_t& prop,const vec3_t velocity,bool box){
  G_PushPropMotion(physicsWorld,prop,velocity,box);
 });
}
#include "g_workshop_constraints.inc"
void select();
bool selectToolShot(const trace_t&,float* contact=nullptr);
void tool(const char*,const float* contact=nullptr);
#include "g_workshop_native.inc"
#include "g_workshop_native_motion.inc"
#include "g_workshop_assembly_crossings.inc"
#include "g_workshop_lift.inc"
#include "g_workshop_angular_cargo.inc"
bool selectWorkshopTarget(int hit){
 const int slot=toolEntityRoot(hit);
 if(slot<0 || slot>=ENTITYNUM_WORLD || !editable(g_entities[slot])){selected=noEntity;message("Aim at an entity: models, NPCs, players, pickups, vehicles and movers are selectable.");return false;}
 selected=slot;message(owned(selected)?"Spawned object selected.":"Map entity selected. Edits may break this mission; removal cannot be undone.");
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","select","entity=%u authored=%d classname=%s",selected,owned(selected)?0:1,g_entities[selected].classname);
 return true;
}
void selectAt(float x,float y,float* contact=nullptr){if(!sandboxReady())return;vec3_t start,direction;G_JAR_WorkshopViewRay(start,direction);
 VectorMA(direction,-(x/320.f-1)*std::tan(cg.refdef.fov_x*.00872664626f),cg.refdef.viewaxis[1],direction);
 VectorMA(direction,-(y/240.f-1)*std::tan(cg.refdef.fov_y*.00872664626f),cg.refdef.viewaxis[2],direction);VectorNormalize(direction);
 const auto trace=workshopTargetTrace(start,direction,contact?G2_PHYSGUN:G2_NOCOLLIDE);
 if(contact&&(trace.startsolid||trace.allsolid)){selected=noEntity;return;}
 float hitDistance=JAR_TOOL_AIM_RANGE*trace.fraction;
 const int slot=selectControlNode(start,direction,hitDistance,trace.entityNum,&hitDistance);
 if(!selectWorkshopTarget(slot)||!contact)return;
 if(slot==trace.entityNum)VectorCopy(trace.endpos,contact);
 else VectorMA(start,hitDistance,direction,contact);
}
void select(){selectAt(320,240);}
bool selectToolShot(const trace_t& shot,float* contact){
 if(shot.startsolid||shot.allsolid){selected=noEntity;return false;}
 if(!selectWorkshopTarget(shot.entityNum))return false;
 if(contact)VectorCopy(shot.endpos,contact);
 return true;
}
#include "g_workshop_native_rotation.inc"
bool rotateToolPose(gentity_t& e,const vec3_t angles,const float* contact){
 vec3_t origin;VectorCopy(e.currentOrigin,origin);
 jar::physics::PhysgunGrip grip;
 if(contact){
  if(!grip.Begin(pv(origin),pv(e.currentAngles),pv(contact),0))return false;
  nativeVector(grip.Origin(pv(contact),pv(angles)),origin);
  for(float value:origin)if(!std::isfinite(value)||std::fabs(value)>=65535)return false;
 }
 if(e.m_pVehicle)return nativeMoveVehicle(e,origin,angles,level.time,contact?contact:e.currentOrigin);
 if(e.client&&e.s.m_iVehicleNum)return false;
 RackStockPlan rack;
 if(!rackStockPose(e,origin,angles,true,rack))return false;
 std::vector<SandboxChargeMove> charges;
 const auto* model=editedProp(e);
 if(model)
  if(!propTransformClear(e,*model,e.s.frame,origin,angles,charges,true,contact,grip.localPoint))return false;
 if(G_JAR_PushableMover(e)){
  if(!G_JAR_MoverRotateAt(e,angles,contact?origin:nullptr,contact))return false;
 }else {
  if(!nativeObjectTurnClear(e,origin,angles,contact,grip,contact&&!model&&!jar::volumeEntity::Portable(e)))return false;
  if(contact)G_SetOrigin(&e,origin);
  G_SetAngles(&e,angles);
 }
 setupMoved(e);if(e.client)SetClientViewAngle(&e,e.currentAngles);if(owned(e.s.number))physicsInvalidate(e.s.number);gi.linkentity(&e);placeEditedCharges(charges);commitRackStock(rack);
 return true;
}
void tool(const char *name,const float* toolContact){
 if(enabled() && !std::strcmp(name,"release") && nativeControls.count(selected)){
  if(recordingContextEdit&&held!=selected){rememberEdit(g_entities[selected]);edits.back().freezeEdit=true;}
  if(held==selected){held=noEntity;clearHoldToken();}nativeRelease(selected,true);message("Entity released.");return;
 }
 if(!sandboxReady())return;
 if(attachmentTool(name))return;
 if(!std::strcmp(name,"select")){select();return;}
 const bool grab=!std::strcmp(name,"grab");
 vec3_t contact;
 if(grab)selectAt(320,240,contact);
 else if(selected==noEntity)select();
 if(selected==noEntity || !editable(g_entities[selected])){message("Select an eligible object first.");return;}
 auto&e=g_entities[selected];auto*o=owned(selected);
 if(!std::strcmp(name,"activate")){
  if(G_SandboxUsable(e)){
   message(G_SandboxUsableActivate(e,workshopPlayer(),false)?"Usable activated.":"Usable disabled, spent or cooling down.");
   return;
  }
  if(G_TeleportKind(e)==jar::teleport::Marker){
   message(G_TeleportUse(e,workshopPlayer())?"Teleported.":"Teleport unavailable. Check the destination, clearance and cooldown.");return;
  }
  if(o&&G_WorkshopBeamClass(e)&&!G_WorkshopFxEndpoint(e)){
   message("Link the beam to a spawned destination before activating it.");return;
  }
  if(e.e_UseFunc==useF_NULL){message("No use action. Connect this destination with Link.");return;}
  if(G_SandboxBreakable(e)&&(e.count!=jar::sandboxBreakable::Intact||(e.svFlags&SVF_INACTIVE))){
   message("Breakable is disabled, broken, or already armed.");return;
  }
  const bool gate=o&&jar::logic::UseGate(G_LogicKind(e));
  if((gate||G_SandboxSpeaker(e)||G_SandboxReplicator(e))&&!G_LogicUseEnabled(e)){message("Use is disabled for this marker.");return;}
  GEntity_UseFunc(&e,workshopPlayer(),workshopPlayer());
  if(!gate)message("Entity activated.");
  return;
 }
 if(!std::strcmp(name,"link")){linkEntity(e);return;}
 if(!std::strcmp(name,"unlink")){unlinkTool();return;}
 jointFirst=0;
 if(RigidConstrained(state,attachmentKey(e))&&(!strcmp(name,"rotate")||!strcmp(name,"rotate_back"))){message("Unlink before an exact Rotate; Physgun rotation respects the assembly.");return;}
 if(!std::strcmp(name,"remove")){
  if(e.s.number==0){G_Damage(&e,nullptr,nullptr,nullptr,nullptr,100000,DAMAGE_NO_PROTECTION,MOD_UNKNOWN);message("Player removed through native death handling.");return;}
  if(o){if(remove(selected))message("Selected spawned object removed.");}
  else {G_JAR_ForgetActor(&e);if(e.NPC)freeActorHelpers(&e);G_FreeEntity(&e);message("Map entity removed. Reload a prior save to restore it; mission scripts may break.");}
  list();return;
 }
 if(!std::strcmp(name,"duplicate")){duplicateTool(e);return;}
 if(grab){
  if(e.m_pVehicle&&!nativeVehicleHoldReady(e)){message("Wait for the vehicle and its riders to settle before grabbing it.");return;}
  if(e.client&&!e.m_pVehicle&&e.s.m_iVehicleNum){message("Aim at the vehicle to move its rider with it.");return;}
  releaseHeld();
  refreshPhysgunView();
  if(!physgunGrip.Begin(pv(e.currentOrigin),pv(e.currentAngles),pv(contact),physgunViewYaw))return;
  held=selected;
  if(!++holdSequence)++holdSequence;holdToken=holdSequence;gi.cvar_set("jar_physgunHeld",va("%u",holdToken));
  const vec3_t viewOrigin={physgunViewOrigin.x,physgunViewOrigin.y,physgunViewOrigin.z};
  holdDistance=std::max(.01f,std::min(JAR_TOOL_AIM_RANGE,Distance(viewOrigin,contact)));
  if(o && prop(o->catalog)){o->frozen=0;physicsWorld.Release(selected);}
  else nativeBegin(e,false);
  message("Holding: USE + mouse rotates; wheel adjusts distance; right click freezes; release primary to throw.");
 }else if(!std::strcmp(name,"freeze")){
  if(e.m_pVehicle&&!nativeVehicleHoldReady(e)){message("Wait for the vehicle and its riders to settle before freezing it.");return;}
  if(e.client&&!e.m_pVehicle&&e.s.m_iVehicleNum){message("Aim at the vehicle to freeze its rider with it.");return;}
  if(recordingContextEdit&&held!=selected){rememberEdit(e);edits.back().freezeEdit=true;}
  if(held==selected)releaseHeld(true);
  else if(o && prop(o->catalog)){o->frozen=1;physicsInvalidate(selected);}
  else nativeBegin(e,true);
  message("Object frozen. Gravity / alternate Freeze releases it.");
 }else if(!std::strcmp(name,"release")){
  if(recordingContextEdit&&held!=selected&&o&&prop(o->catalog)&&o->frozen){rememberEdit(e);edits.back().freezeEdit=true;}
  if(held==selected)releaseHeld();
  if(o && prop(o->catalog)){o->frozen=0;physicsWorld.Release(selected);}
  else nativeRelease(selected,true);
  message("Object released with momentum.");
 }else if(!std::strcmp(name,"rotate")||!std::strcmp(name,"rotate_back")){
  if(held==selected){message("Release the Physgun before using Rotate.");return;}
  vec3_t angles;VectorCopy(e.currentAngles,angles);angles[1]=AngleNormalize360(angles[1]+(!std::strcmp(name,"rotate")?15:-15));
  if(!rotateToolPose(e,angles,toolContact)){message("Rotation is blocked. Clear space around the object and its attachments.");return;}
 }else message("Unknown tool.");
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","tool","action=%s selected=%u held=%u",name,selected,held);
}

#include "g_workshop_context.inc"
#include "g_workshop_link_cleanup.inc"
#include "g_workshop_physgun_turn.inc"

void grant(const char *name,bool select){if(!sandboxReady())return;
 if(std::strcmp(name,"jar_shortblade")&&std::strcmp(name,"jar_pike")&&std::strcmp(name,"sith_sword")){message("Unknown melee choice.");return;}
 Weapon w;w.family=WP_SABER;w.sandbox=1;Q_strncpyz(w.saber[0],name,sizeof(w.saber[0]));
 for(auto&c:w.colors[0])c=!std::strcmp(name,"jar_shortblade")?SABER_PURPLE:SABER_YELLOW;
 if(!choiceValid(w)){message("Melee choice unavailable: required definition/model missing.");return;}
 reconcile();const auto id=Add(state,w);if(!id){message("Carried inventory is full; nothing granted.");return;}
 auto*p=workshopPlayer();p->client->ps.stats[STAT_WEAPONS]|=1<<WP_SABER;RegisterItem(FindItemForWeapon(WP_SABER));
 if(select)equip(id);else {message("Melee choice added to the weapon bar. Scroll to select it.");list();}
}
}
bool G_LiftTranslate(gentity_t& deck,float dz){return liftTranslate(deck,dz);}
bool G_AngularCargoPlan(gentity_t& deck,const vec3_t destination,const vec3_t angles,AngularCargo& cargo){
 return angularCargoPlan(deck,destination,angles,cargo);
}
bool G_AngularCargoCommit(gentity_t& deck,const AngularCargo& cargo){return angularCargoCommit(deck,cargo);}
bool G_JAR_WorkshopUseGate(gentity_t& source,gentity_t* other,gentity_t* activator){return useGateMarker(source,other,activator);}
jar::workshop::Weapon G_JAR_WorkshopCurrentWeapon(){Weapon w;auto*p=workshopPlayer();if(!p)return w;
 auto&ps=p->client->ps;if(!(ps.stats[STAT_WEAPONS]&(1<<WP_SABER)))return w;
 w.family=WP_SABER;
 for(int s=0;s<2;++s){if(s==1&&!ps.dualSabers)break;const char*n=ps.saber[s].name;if(!n||std::strlen(n)>=nameSize)return Weapon{};
  Q_strncpyz(w.saber[s],n,nameSize);for(int b=0;b<8;++b)w.colors[s][b]=uint8_t(ps.saber[s].blade[b].color);}
 return w;
}
void G_JAR_WorkshopAcquired(const Weapon&before){if(!workshopPlayer())return;if(before.family)Add(state,before);reconcile();}
void G_JAR_WorkshopDetachChild(gentity_t*source,gentity_t*child){
 detachChild(source,child);
}
bool G_JAR_WorkshopRackPickupSuspended(const gentity_t& item){return rackStockAttached(item);}
void G_JAR_WorkshopRackPickupLoose(gentity_t& item){rackStockLoose(item);}
bool G_JAR_WorkshopMoveRackStock(gentity_t& rack,const vec3_t origin,const vec3_t angles){return rackStockMove(rack,origin,angles);}
void G_JAR_WorkshopBreakableDetach(gentity_t* e){
 const auto slot=e->s.number;
 const auto key=attachmentKey(*e);
 if(held==unsigned(slot)){held=noEntity;clearHoldToken();}
 nativeRelease(slot,false);
 if(jointFirst==key)jointFirst=0;
 for(unsigned i=0;i<state.jointCount;){
  const auto& joint=state.joints[i];
  if(joint.first==key||joint.second==key)removeJoint(joint.id);
  else ++i;
 }
}
qboolean G_JAR_WorkshopTraceProp(const gentity_t* entity,const vec3_t origin,const vec3_t angles,
 const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,int mask,trace_t* trace,
 const JarSweepHull* movingHull){
 if(!entity||!entity->inuse||entity->s.eType!=ET_GENERAL||!entity->classname||
    std::strcmp(entity->classname,"jar_sandbox_prop"))return qfalse;
 *trace={};trace->fraction=1;trace->entityNum=ENTITYNUM_NONE;
 VectorCopy(end,trace->endpos);
 if(!entity->linked||!(entity->contents&mask))return qtrue;
 if(!mins)mins=vec3_origin;
 if(!maxs)maxs=vec3_origin;
 if(!propTraceBroadphase(*entity,origin,angles,start,end,mins,maxs))return qtrue;
 const auto* hull=propTraceHull(*entity,angles);
 if(!hull)return qfalse;
 vec3_t localStart,localEnd,delta;
 VectorSubtract(start,origin,localStart);VectorSubtract(end,origin,localEnd);
 const auto hit=movingHull?hull->TraceHull(pv(localStart),pv(localEnd),*movingHull)
                          :hull->Trace(pv(localStart),pv(localEnd),pv(mins),pv(maxs));
 if(!hit.startSolid&&hit.fraction==1)return qtrue;
 *trace={};
 trace->startsolid=hit.startSolid?qtrue:qfalse;
 trace->allsolid=hit.allSolid?qtrue:qfalse;
 trace->fraction=hit.fraction;
 trace->entityNum=entity->s.number;trace->contents=entity->contents;
 VectorSubtract(end,start,delta);VectorMA(start,hit.fraction,delta,trace->endpos);
 if(hit.fraction<1&&!hit.allSolid){
  nativeVector(hit.normal,trace->plane.normal);
  trace->plane.dist=hit.planeDistance+DotProduct(trace->plane.normal,origin);
  trace->plane.type=PlaneTypeForNormal(trace->plane.normal);
  SetPlaneSignbits(&trace->plane);
 }
 return qtrue;
}
bool G_JAR_WorkshopPropDepth(const gentity_t& fixed,const vec3_t origin,const vec3_t mins,
 const vec3_t maxs,const JarSweepHull* movingHull,float& depth){
 const auto* hull=propTraceHull(fixed,fixed.currentAngles);
 if(!hull)return false;
 vec3_t local;
 VectorSubtract(origin,fixed.currentOrigin,local);
 const auto point=pv(local);
 const auto hit=movingHull?hull->TraceHull(point,point,*movingHull)
                          :hull->Trace(point,point,pv(mins),pv(maxs));
 depth=hit.allSolid?hit.depth:-1.f;
 return true;
}
void G_JAR_WorkshopForget(gentity_t*e,bool freeing){if(!e)return;const uint32_t n=e->s.number;
 propTraceCache.erase(n);
 const auto* entry=owned(n);
 const auto nativeKey=jar::constraint::NativeId(n);
 const auto ownedKey=entry?entry->id:0;
 for(unsigned i=0;i<state.jointCount;){
  const auto& joint=state.joints[i];
  if(joint.first==nativeKey||joint.second==nativeKey||
     (ownedKey&&(joint.first==ownedKey||joint.second==ownedKey)))removeJoint(joint.id);
  else ++i;
 }
 physicsWorld.Forget(n);nativeControls.erase(n);
 if(const auto* o=owned(n)){
  const auto parent=*o;
  if(parent.id==jointFirst)jointFirst=0;
  if(parent.id==linkFirst)linkFirst=0;
  if(freeing&&matches(parent)&&(actor(parent.catalog)||parent.catalog==consoleCatalog))freeActorHelpers(e);
  forgetWorkshopChildren(parent,freeing);
 }
 const bool refreshContext=freeing&&forgetWorkshopReferences(*e,contextEntity);
 forgetEdits(n);
 if(held==n){held=noEntity;clearHoldToken();}if(selected==n)selected=noEntity;if(contextEntity==n)contextClear();
 if(owned(n))spawnCountDirty=true;
 Forget(state,n);if(jointFirst==jar::constraint::NativeId(n))jointFirst=0;
 if(freeing&&G_LogicKind(*e)==jar::logic::Give){
  for(int slot=0;slot<jar::logic::outputCount;++slot)givePickup::sync(G_LogicOutput(*e,slot));
 }
 if(refreshContext&&contextValid())contextPublish();
}
void G_JAR_WorkshopShutdown(){G_ContactClocksReset();G_TeleportArrivalsReset();G_SandboxChargesReset();teleportVolumesFrame=-1;G_ResetSpawnTransactions();G_SubBSPOwnershipReset();G_DoorCameraReset();G_CrystalCrateReset();G_WelderReset();G_SpotlightReset();G_PortalReset();G_ShipBoundaryReleaseAll();G_MusicReset();G_SkyPortalReset();nativeJointsReset();ropeReset();duplicateClear();contextClear();clearHoldToken();nativeReleaseAll();G_JAR_MoverMotionReset();physicsStop(false);propTraceCache.clear();physicsBounds.clear();modelFrameCounts.clear();}
void G_JAR_WorkshopInit(bool loading){G_ContactClocksReset();G_TeleportArrivalsReset();G_SandboxChargesReset();teleportVolumesFrame=-1;G_ResetSpawnTransactions();G_SubBSPOwnershipReset();G_DoorCameraReset();G_CrystalCrateReset();G_JAR_MoverMotionReset();G_WelderReset();G_SpotlightReset();G_PortalReset();G_ShipBoundaryReset();G_MusicReset();G_SkyPortalReset();nativeJointsReset();ropeReset();G_TargetMarkersReset();G_CombatPointReset();G_ReferenceReset();contextClear();clearHoldToken();npcResources.clear();npcResourcesRead=false;nativeControls.clear();duplicateClear();linkFirst=0;physicsStop(false);propTraceCache.clear();physicsBounds.clear();modelFrameCounts.clear();physicsRequested();const bool disarmed=activeTool || toolRelease;state={};edits.clear();std::fill(std::begin(entityGeneration),std::end(entityGeneration),0u);ready=false;readDone=!loading;held=selected=noEntity;lastTime=0;wasEnabled=false;prepared.clear();stopTool();toolRelease=disarmed;previousFamily=0;toolMode=0;
 spawnCountDirty=true;
 workshopFallSections.clear();workshopHostFallFloor=MIN_WORLD_COORD;
 assemblyTeleportFrame=-1;
 teleportMoverOrigins.clear();
 toolMode=std::max(0,std::min(toolModeCount-1,gi.cvar("jar_toolMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->integer));
 for (unsigned i = 0; i < state.ownedCount; ++i) {
  const auto& owner = state.owned[i];
  if (!matches(owner)) continue;
  auto& entity = g_entities[owner.entity];
  if (JAR_StartCard(entity.classname) && !G_JAR_PortableStart(entity)) {
   if (entity.model && gi.bIsFromZone(entity.model,TAG_G_ALLOC)) gi.Free(entity.model);
   entity.model = G_NewString("@jar_start");
   entity.e_UseFunc = useF_portable_start_use;
   entity.svFlags |= SVF_PLAYER_USABLE;
  }
  if (!G_AlwaysClass(entity) || G_AlwaysPortable(entity)) continue;
  // A save made before the old disposable trigger fired can still contain it.
  if (entity.model && gi.bIsFromZone(entity.model,TAG_G_ALLOC)) gi.Free(entity.model);
  entity.model = G_NewString("@jar_always");
  entity.delay = JAR_ALWAYS_DEFAULT_MS;
  entity.e_UseFunc = useF_portable_always_use;
  entity.svFlags |= SVF_PLAYER_USABLE;
 }
 gi.cvar("jar_sandbox","0",CVAR_ARCHIVE);message("Inventory and Sandbox ready after player initialization.");}
void restoreTool(){if(auto*w=Find(state,state.active))if(w->tool && enabled() && workshopPlayer() && workshopPlayer()->client->ps.weapon==int(w->family)){activeTool=w->id;toolRelease=true;toolRestorePending=true;}}
void G_JAR_WorkshopSpawned(bool hubReturn){if(!readDone)return;ready=true;
 const int chunks=gi.cvar("jar_inventoryTransferCount","0",0)->integer;
 if(chunks>0&&chunks<=120){std::string encoded;for(int i=0;i<chunks;++i)encoded+=gi.cvar(va("jar_inventoryTransfer%d",i),"",0)->string;
  std::string raw;
  if(!Unhex(encoded,raw,wireLimit)||!ImportCarriedInventory(state,reinterpret_cast<const uint8_t*>(raw.data()),raw.size(),ENTITYNUM_WORLD))
   message("Invalid carried-inventory transition data ignored; native loadout retained.");}
 gi.cvar_set("jar_inventoryTransferCount","0");restoreTool();reconcile();if(hubReturn&&!enabled())cleanup();list();
}
void G_JAR_WorkshopTransition(){G_ShipBoundaryReleaseAll();G_MusicReleaseAll();G_SkyPortalReleaseAll();duplicateClear();contextClear();releaseHeld();nativeReleaseAll();reconcile();const auto bytes=Encode(state,true);std::string encoded=Hex(std::string(bytes.begin(),bytes.end()));
 const int count=int((encoded.size()+199)/200);for(int i=0;i<count;++i)gi.cvar_set(va("jar_inventoryTransfer%d",i),encoded.substr(i*200,200).c_str());
 gi.cvar_set("jar_inventoryTransferCount",va("%d",count));held=noEntity;
}
#include "g_sandbox_vehicle_push_save.inc"
qboolean G_JAR_WorkshopPrepareSave(qboolean autosave){releaseHeld();reconcile();G_ShipBoundaryFrame();G_SandboxChargesFrame();if(!autosave)physicsSavedMotion("saved_motion");jointFirst=0;held=noEntity;if(activeTool)toolRelease=true;prepared=Encode(state,autosave!=qfalse);return (state.count||state.ownedCount||state.jointCount||!nativeControls.empty()||G_SpaceHasVolumes()||G_BidirectionalHasVolumes()||G_ShipBoundaryHasVolumes()||G_CloudHasLayers()||G_WeatherHasControllers()||G_TargetMarkersPresent()||G_JAR_MoverMotionPresent()||G_SubBSPHasOwnership()||(!autosave&&G_VehiclePushPresent()))?qtrue:qfalse;}
#include "g_workshop_save_write.inc"
void G_JAR_WorkshopRead(bool autosave,int version,bool hubReturn){G_ContactClocksReset();G_TeleportArrivalsReset();G_SandboxChargesReset();teleportVolumesFrame=-1;G_JAR_MoverMotionReset();nativeJointsReset();ropeReset();duplicateClear();contextClear();clearHoldToken();nativeControls.clear();physicsStop(false);const bool disarmed=activeTool || toolRelease;state={};edits.clear();linkFirst=0;held=selected=noEntity;stopTool();toolRelease=disarmed;
 spawnCountDirty=true;
 teleportMoverOrigins.clear();
 if(jar::IsWorkshopSaveVersion(version)){ojk::SavedGameHelper h(gi.saved_game);uint32_t size=0;h.read_chunk<uint32_t>(INT_ID('J','W','S','Z'),size);
  if(size<24||size>wireLimit)gi.Error(ERR_DROP,"Invalid JA:R inventory/Sandbox save size");std::vector<uint8_t> bytes(size);
  h.read_chunk<uint8_t>(INT_ID('J','W','S','T'),bytes.data(),int(size));State candidate;
  if(!Decode(bytes.data(),bytes.size(),candidate,ENTITYNUM_WORLD)||(autosave&&(candidate.ownedCount||candidate.jointCount)))gi.Error(ERR_DROP,"Invalid JA:R inventory/Sandbox save data");
  state=std::move(candidate);for(unsigned i=0;i<state.jointCount;++i)if(!attachmentEntity(state.joints[i].first)||!attachmentEntity(state.joints[i].second))gi.Error(ERR_DROP,"JA:R attachment endpoint does not match saved world");if(version>=jar::workshopSaveV6)nativeRead(autosave,h);
  if(version>=jar::workshopSaveV17){h.read_chunk(INT_ID('J','M','U','S'));if(!G_MusicImport(autosave,h))gi.Error(ERR_DROP,"Invalid Sandbox music controller record");h.ensure_all_data_read();}
  G_ShipBoundaryRestore();
  if(version>=jar::workshopSaveV20){h.read_chunk(INT_ID('J','S','H','P'));if(!G_ShipBoundaryImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved ship-boundary steering");h.ensure_all_data_read();}
  if(version>=jar::workshopSaveV30){h.read_chunk(INT_ID('J','M','F','R'));if(!G_JAR_MoverMotionImport(autosave,h,version>=jar::workshopSaveV31))gi.Error(ERR_DROP,"Invalid saved native force motion");h.ensure_all_data_read();}
  if(version>=jar::workshopSaveV33){h.read_chunk(INT_ID('J','S','K','Y'));if(!G_SkyPortalImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved sky ownership");h.ensure_all_data_read();}
  if(version>=jar::workshopSaveV39){
   h.read_chunk(INT_ID('J','B','S','O'));
   if(!G_SubBSPOwnershipImport(autosave,h,version>=jar::workshopSaveV40))gi.Error(ERR_DROP,"Invalid saved map-section ownership");
   h.ensure_all_data_read();
   if(!autosave)for(int n=MAX_CLIENTS;n<globals.num_entities;++n)G_BSPSectionRestoreWorkshop(g_entities[n],state);
  }
   if(auto*p=workshopPlayer()){for(unsigned i=0;i<state.count;++i)if(state.weapons[i].tool)p->client->ps.stats[STAT_WEAPONS]|=1u<<state.weapons[i].family;}
   if(version<jar::workshopSaveV3 && g_entities[0].client){auto&ps=g_entities[0].client->ps;
    for(unsigned i=0;i<state.count;++i)if(state.weapons[i].tool){const auto&w=state.weapons[i];ps.stats[STAT_WEAPONS]|=1u<<w.family;
     if(w.id==state.active && ps.weapon==(w.tool==1?WP_CONCUSSION:WP_BLASTER_PISTOL)){ps.weapon=w.family;g_entities[0].s.weapon=w.family;}}
   }
   if(gi.JAR_Log)gi.JAR_Log(1,"workshop","restored","active=%u count=%u owned=%u nativeWeapon=%d",state.active,state.count,state.ownedCount,g_entities[0].client?g_entities[0].client->ps.weapon:-1);for(unsigned i=0;i<state.ownedCount;++i)if(!matches(state.owned[i]))gi.Error(ERR_DROP,"JA:R Sandbox ownership does not match saved world");
 }
 toolMode=std::max(0,std::min(toolModeCount-1,gi.cvar("jar_toolMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->integer));
 if(!G_SkyPortalRestore())gi.Error(ERR_DROP,"Invalid saved sky viewpoint");
 if(!G_RailRestore())gi.Error(ERR_DROP,"Invalid saved rail controller");if(!G_PortableShieldRestore())gi.Error(ERR_DROP,"Invalid saved portable shield");G_SpaceRestore(version<jar::workshopSaveV13);G_BidirectionalRestore();G_ShipBoundaryRestore();G_TargetMarkersRestore();if(!G_CombatPointRestore())gi.Error(ERR_DROP,"Invalid saved combat points");if(!G_ReferenceRestore())gi.Error(ERR_DROP,"Invalid or conflicting saved reference tags");if(!G_WaypointRestore())gi.Error(ERR_DROP,"Invalid saved live waypoints");if(!G_SceneCameraRestore())gi.Error(ERR_DROP,"Invalid saved scene camera");if(!G_ReflectionRestore())gi.Error(ERR_DROP,"Invalid saved reflection probes");G_CloudRestore();G_WeatherRestore();G_LaserArmRestore();G_PortalRestore();G_SpotlightRestore();G_WelderRestore();G_DoorCameraRestore();G_CrystalCrateRestore();
 if(!autosave){
  givePickup::restore();
  for(unsigned i=0;i<state.ownedCount;++i){
   auto& e=g_entities[state.owned[i].entity];
   workshopRestoreCarriedPickup(e);
   if(G_WorkshopTrailClass(e)){
    const auto control=nativeControls.find(e.s.number);
    if(control==nativeControls.end())G_WorkshopTrailRestore(e,version<jar::workshopSaveV58);
    else G_WorkshopTrailRestore(e,version<jar::workshopSaveV58,
     &control->second.rotation,control->second.originalAngles);
   }
   if(G_WallPortable(e)){G_WallRefresh(e);gi.linkentity(&e);}
   if(G_SandboxSlidePortable(e)){
    jar::slidingDoor::State door;
    if(e.e_UseFunc!=useF_portable_sliding_door_use||e.e_ThinkFunc!=thinkF_portable_sliding_door_think||
       !G_SandboxSlideRead(e,door)||!G_SandboxSlideRestore(e,door))gi.Error(ERR_DROP,"Invalid saved Sandbox sliding door");
    gi.linkentity(&e);
   }
   if(G_ButtonPortable(e)){
    jarButtonState_t button;
    if(e.e_UseFunc!=useF_portable_button_use||e.e_ThinkFunc!=thinkF_portable_button_think||
       !G_ButtonRead(e,button)||!G_ButtonRestore(e,button))gi.Error(ERR_DROP,"Invalid saved Sandbox button");
    gi.linkentity(&e);
   }
   if(G_BobbingLegacy(e)){
    const auto controlled=nativeControls.find(e.s.number);
    const bool heldLegacy=controlled!=nativeControls.end();
    if(!G_BobbingMigrate(e,heldLegacy?controlled->second.position:e.s.pos,heldLegacy?controlled->second.started:level.time))
     gi.Error(ERR_DROP,"Invalid legacy Sandbox bobbing platform");
    gi.linkentity(&e);
   }else if(G_BobbingPortable(e)){
    jarBobState_t bob{};
    if(e.e_UseFunc!=useF_portable_bobbing_use||e.e_ThinkFunc!=thinkF_portable_bobbing_think||
       !G_BobbingRead(e,bob)||!G_BobbingRestore(e,bob))gi.Error(ERR_DROP,"Invalid saved Sandbox bobbing platform");
    gi.linkentity(&e);
   }
   if(!G_ContactRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox contact trigger");
   if(!G_PushRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox push controller");
   if(!G_TeleportRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox teleporter");
   if(!G_SandboxLaserRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox laser emitter");
   if(!G_SandboxStationRestore(e,g_spskill?g_spskill->integer:1))gi.Error(ERR_DROP,"Invalid saved Sandbox station");
   if(!G_SandboxBreakableRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox breakable");
   if(!G_SandboxUsableRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox usable");
   if(!G_SandboxSpeakerRestore(e))gi.Error(ERR_DROP,"Invalid saved Sandbox speaker");
   if(G_SandboxReplicator(e)&&!e.delay){
    G_ReplicatorModels(e);
    G_ReplicatorApplyPhase(e,{},level.time);
    e.delay=G_ReplicatorDelay(e);
   }
   if(jar::sandboxAngular::Class(e.classname)){
    const auto heldMover=nativeControls.find(e.s.number);
    const bool heldLegacy=heldMover!=nativeControls.end()&&!G_SandboxAngular(e);
    if(!G_SandboxAngularRestore(e,heldLegacy?heldMover->second.rotation:e.s.apos,
       heldLegacy?heldMover->second.started:level.time))gi.Error(ERR_DROP,"Invalid saved Sandbox angular mover");
    if(heldLegacy){heldMover->second.rotation=e.s.apos;VectorCopy(e.currentAngles,heldMover->second.originalAngles);}
   }
   G_MovementMarkerRestore(e);
   G_LogicRestore(e);
   if(G_DistanceClass(e)){
    if(!G_DistancePortable(e)){
     if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);
     e.model=G_NewString("@jar_entdist");
     if(!JAR_DistanceSettingsValid(e.count,e.spawnflags)){e.count=JAR_DISTANCE_DEFAULT;e.spawnflags=JAR_DISTANCE_PLAYER;}
    }
    if(!JAR_DistanceSettingsValid(e.count,e.spawnflags))gi.Error(ERR_DROP,"Invalid saved distance trigger");
   }
   if(!G_VisibleClass(e))continue;
   if(G_VisiblePortable(e)){
    if(!(e.radius>=JAR_VISIBLE_RANGE_MIN&&e.radius<=JAR_VISIBLE_RANGE_MAX&&e.radius==std::floor(e.radius)&&
         e.random>=JAR_VISIBLE_CONE_MIN&&e.random<=JAR_VISIBLE_CONE_MAX&&e.random==std::floor(e.random))||
       !JAR_VisibleSettingsValid(G_VisibleSettings(e)))gi.Error(ERR_DROP,"Invalid saved visibility trigger");
    continue;
   }
   // Older Sandbox markers used the authored dot threshold and disappeared after firing.
   const int range=e.radius>=JAR_VISIBLE_RANGE_MIN&&e.radius<=JAR_VISIBLE_RANGE_MAX?int(e.radius):JAR_VISIBLE_RANGE_DEFAULT;
   int cone=JAR_VISIBLE_CONE_DEFAULT;
   if(e.random>=0&&e.random<=1){
    const int degrees=int(std::acos(e.random)*(180.0f/3.14159265358979323846f)+.5f);
    cone=std::max(int(JAR_VISIBLE_CONE_MIN),std::min(int(JAR_VISIBLE_CONE_MAX),degrees));
   }
   if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);
   e.model=G_NewString("@jar_visible");
   G_VisibleSet(e,{range,cone,e.spawnflags&(JAR_VISIBLE_IGNORE_WALLS|JAR_VISIBLE_FORCE_SIGHT)});
   e.e_UseFunc=useF_portable_visible_use;e.svFlags|=SVF_PLAYER_USABLE;
  }
  restorePropPoses();physicsSavedMotion("restored_motion");
 }
 if(version>=jar::workshopSaveV45){
  ojk::SavedGameHelper h(gi.saved_game);
  h.read_chunk(INT_ID('J','H','R','T'));
  if(!G_ContactClocksImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved Sandbox contact cooldowns");
  h.ensure_all_data_read();
  if(version>=jar::workshopSaveV47){
   h.read_chunk(INT_ID('J','T','A','R'));
   if(!G_TeleportArrivalsImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved Sandbox teleporter arrivals");
   h.ensure_all_data_read();
  }
  if(version>=jar::workshopSaveV50){
   h.read_chunk(INT_ID('J','V','P','D'));
   if(!G_VehiclePushImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved Sandbox vehicle launch velocity");
  }
  if(version>=jar::workshopSaveV53){
   h.read_chunk(INT_ID('J','C','H','G'));
   if(!G_SandboxChargesImport(autosave,h))gi.Error(ERR_DROP,"Invalid saved Sandbox charge mount");
  }
  if(version>=jar::workshopSaveV55){
   h.read_chunk(INT_ID('J','U','N','D'));
   if(!readUndoHistory(autosave,h))gi.Error(ERR_DROP,"Invalid saved Sandbox creation history");
  }
 }
 if(!autosave&&version<jar::workshopSaveV55)restoreLegacySpawnHistory();
 if(hubReturn)nativeRelease(0,false);
 readDone=true;ready=!autosave&&!hubReturn;if(ready){restoreTool();reconcile();if(!enabled())cleanup();list();}
}
void G_JAR_WorkshopFrame(){if(!ready)return;const bool on=enabled();if(!on&&state.ownedCount){for(int i=int(state.ownedCount)-1;i>=0;--i)if(unsigned(i)<state.ownedCount&&removalRoot(state.owned[i])&&removalAvailable(state.owned[i]))remove(state.owned[i].entity);}
  if(activeTool && !on){stopTool();if(equipReady() && previousFamily>0)CG_ChangeWeapon(previousFamily);}
 if(!usable()||!on){releaseHeld();jointFirst=0;if(!on||(!contextMountedVehicle&&!nativeControls.count(0)))contextClear();}
 if(contextMountedVehicle){const auto* hull=contextPlayerVehicle();if(!hull||hull->s.number!=int(contextEntity))contextClear();}
 if(contextEntity!=noEntity&&!contextValid())contextClear();
 if(contextToken){
  auto& entity=g_entities[contextEntity];
  if(contextPublishedOrderAllowed&&entity.NPC)contextNPCGoalStatus(G_WorkshopReadNPCGoal(*entity.NPC),entity.NPC);
  if(G_SandboxSlidePortable(entity))contextSlideStatus(entity);
  else if(G_ButtonPortable(entity))contextButtonStatus(entity);
  else if(G_BobbingPortable(entity))contextBobbingStatus(entity);
  else if(G_SandboxAngular(entity))contextAngularStatus(entity);
  else if(G_LogicKind(entity)!=jar::logic::None&&owned(entity.s.number))contextLogicStatus(entity);
  else if(G_ContactKind(entity))contextContactStatus(entity);
  else if(G_PushKind(entity))contextPushStatus(entity);
  else if(G_TeleportKind(entity))contextTeleportStatus(entity);
  else if(G_SandboxStation(entity))contextStationStatus(entity);
  else if(G_SandboxBreakable(entity))contextBreakableStatus(entity);
  else if(G_SandboxUsable(entity))contextUsableStatus(entity);
  else if(G_PortalPortable(entity))contextPortalLinkStatus(entity,true);
  else if(G_PortableShield(entity))contextShieldStatus(entity);
  else if(entity.s.weapon==WP_DET_PACK||entity.s.weapon==WP_TRIP_MINE)contextChargeStatus(entity);
 }
 if(!on){G_MusicReleaseAll();G_SkyPortalReleaseAll();nativeReleaseAll();while(state.jointCount)removeJoint(state.joints[0].id);}
  if(on&&state.ownedCount)updateWorkshopFallSections();
  const bool rigid=physicsEnsure(on);
 pendingAssemblyCrossings.clear();
 pushPropVolumes=false;
 if(on)for(unsigned i=0;i<state.ownedCount;++i){
  const auto& source=g_entities[state.owned[i].entity];jar::push::Settings settings;
  if(G_PushKind(source)==jar::push::Volume&&G_PushRead(source,settings)&&
     settings.enabled&&settings.subject>=jar::push::Props){pushPropVolumes=true;break;}
 }
 const float dt=std::min(50,std::max(0,level.time-lastTime))*.001f;lastTime=level.time;
 if(held<ENTITYNUM_WORLD&&g_entities[held].inuse&&g_entities[held].m_pVehicle&&!nativeVehicleHoldReady(g_entities[held])){
  const auto vehicle=held;
  held=noEntity;clearHoldToken();toolRelease=true;
  nativeRelease(vehicle,false);
  message("Vehicle released because its controller or riders changed.");
 }
 uint32_t nativeGripEntity=noEntity;
 vec3_t gripStart{};
 {
 SandboxMountedChargeCollision mountedCollision;
 if(physgunGrip.active&&held<ENTITYNUM_WORLD&&g_entities[held].inuse){
  refreshPhysgunView();
  auto& target=g_entities[held];
  const auto* model=editedProp(target);
  if(dt>0&&(!rigid||!model||!physicsEligible(*model,target.s.frame))){
   nativeGripEntity=held;
   VectorCopy(target.currentOrigin,gripStart);
   physgunNativeTurn(target,dt);
  }
 }
 for(unsigned i=0;i<state.ownedCount;){const auto o=state.owned[i];if(!matches(o)){G_JAR_WorkshopForget(&g_entities[o.entity],false);continue;}
  auto&e=g_entities[o.entity];
  if(on&&(!e.client||e.e_ThinkFunc!=thinkF_NPC_Begin)){
   const float* mins=e.mins;const float* maxs=e.maxs;
   vec3_t sectionMins,sectionMaxs;
   if(e.linked&&jar::bspSection::Portable(e)){
    jar::bspSection::Hull(e.mins,e.maxs,e.currentAngles,sectionMins,sectionMaxs);
    mins=sectionMins;maxs=sectionMaxs;
   }
   if(removeOutsideMap(o,e.currentOrigin,mins,maxs))continue;
  }
  if(const auto* model=on?prop(o.catalog):nullptr){if(held!=o.entity)attachmentAdvanceProp(e,o,*model,dt);const bool rigidProp=rigid && physicsEligible(*model,e.s.frame);
   if(!rigidProp){vec3_t mins,maxs;if(propHull(*model,e.currentAngles,mins,maxs,e.s.frame)&&(!VectorCompare(mins,e.mins)||!VectorCompare(maxs,e.maxs))){VectorCopy(mins,e.mins);VectorCopy(maxs,e.maxs);e.contents=physicsFrozenContents(e);gi.linkentity(&e);}}
   if(!rigidProp && e.contents==CONTENTS_CORPSE){e.contents=physicsFrozenContents(e);gi.linkentity(&e);}vec3_t desired;VectorCopy(e.currentOrigin,desired);
   if(held==o.entity && !rigidProp){physgunOriginGoal(e.currentAngles,holdDistance,desired);if(dt>0){VectorSubtract(desired,e.currentOrigin,e.s.pos.trDelta);const float speed=VectorNormalize(e.s.pos.trDelta);VectorScale(e.s.pos.trDelta,std::min(1024.f,speed/dt),e.s.pos.trDelta);VectorMA(e.currentOrigin,dt,e.s.pos.trDelta,desired);}}
   else if(!o.frozen && !rigidProp)nativePropFall(e,o.physics,dt,desired);
   if(!rigidProp){
    advanceNativeProp(e,desired,dt);
    if(removeOutsideMap(o,e.currentOrigin,e.mins,e.maxs))continue;
   }
  }++i;
 }
 }
 G_MusicReconcile();nativeFrame(dt,rackStockPose,rackStockLoose);G_SkyPortalFrame();
 if(nativeGripEntity!=noEntity&&held==nativeGripEntity&&g_entities[held].inuse){
  auto& e=g_entities[held];
  vec3_t travel;VectorSubtract(e.currentOrigin,gripStart,travel);
  const auto control=nativeControls.find(held);
  if(control!=nativeControls.end())VectorScale(travel,1/dt,control->second.velocity);
  else VectorScale(travel,1/dt,e.s.pos.trDelta);
 }
 attachmentPrepareRopes();
 if(rigid)physicsFrame(dt);
 queueAssemblyMovers(teleportObjectVolumes()&jar::teleport::Mover);
 finishAssemblyCrossings(dt);
 for(auto& sample:teleportMoverOrigins)VectorCopy(g_entities[sample.first].currentOrigin,sample.second.origin);
 attachmentTick(dt);
 G_SandboxChargesFrame();
 for(unsigned i=0;i<state.ownedCount;++i)G_BSPSectionMoved(g_entities[state.owned[i].entity]);
  if(on!=wasEnabled){wasEnabled=on;list();}
  else publishSpawnCount();
}
qboolean G_JAR_WorkshopCommand(const char*cmd){
 if(!std::strcmp(cmd,"jar_inventory")){uint32_t id=0;const char*action=gi.argv(1);
  if(!std::strcmp(action,"equip")&&number(gi.argv(2),id))equip(id);
  else if(!std::strcmp(action,"page")&&number(gi.argv(2),id)){page=int(std::min(id,7u));list();}
  else {list();if(gi.JAR_Log&&workshopPlayer()){const auto&ps=workshopPlayer()->client->ps;
   gi.JAR_Log(0,"inventory","native","weapon=%d selected=%d bits=%d ammo=%d,%d,%d,%d,%d dual=%d first=%s second=%s colors=%d,%d length=%.1f,%.1f damage=%.2f,%.2f",ps.weapon,cg.weaponSelect,ps.stats[STAT_WEAPONS],ps.ammo[AMMO_BLASTER],ps.ammo[AMMO_METAL_BOLTS],ps.ammo[AMMO_THERMAL],ps.ammo[AMMO_TRIPMINE],ps.ammo[AMMO_DETPACK],ps.dualSabers,ps.saber[0].name?ps.saber[0].name:"",ps.saber[1].name?ps.saber[1].name:"",ps.saber[0].blade[0].color,ps.saber[1].blade[0].color,ps.saber[0].blade[0].lengthMax,ps.saber[1].blade[0].lengthMax,ps.saber[0].damageScale,ps.saber[1].damageScale);}
  if(gi.JAR_Log)for(unsigned i=0;i<state.count;++i){const auto&w=state.weapons[i];gi.JAR_Log(0,"inventory","record","id=%u family=%u sandbox=%u tool=%u first=%s second=%s active=%d",w.id,w.family,w.sandbox,w.tool,w.saber[0],w.saber[1],w.id==state.active);}}
  return qtrue;
 }
 if(std::strcmp(cmd,"jar_sb"))return qfalse;
 teleportVolumesFrame=-1;
 const char*action=gi.argv(1);uint32_t id=0;
 if(!strcmp(action,"context")){contextCommand();return qtrue;}
 if(!strcmp(action,"rotateheld")){rotateHeld();return qtrue;}
 if(!std::strcmp(action,"on")){gi.cvar_set("jar_sandbox","1");message("Sandbox enabled. Hold Q for the unpaused spawn overlay; changes can affect this mission.");}
 else if(!std::strcmp(action,"off")){gi.cvar_set("jar_sandbox","0");cleanup();G_PortableShieldCleanup();}
 else if(!std::strcmp(action,"physics")){
   const char*value=gi.argv(2);
   if(!std::strcmp(value,"off")){physicsStop(true);gi.cvar_set("jar_box3d","0");message("Box3D off. Props frozen in place; native controllers retained.");}
   else if(!std::strcmp(value,"on")&&sandboxReady()){gi.cvar_set("jar_box3d","1");if(physicsEnsure(true))message("Box3D enabled for spawned props. Release adds rigid-body motion; native actors and vehicles push props.");}
   physicsReport();
  }
  else if(!std::strcmp(action,"spawn")&&number(gi.argv(2),id))spawn(id);
  else if(!std::strcmp(action,"weapon")&&number(gi.argv(2),id))weaponCard(id);
  else if(!std::strcmp(action,"limit")){message("Prop capacity is fixed at 4096; optional entity exhaustion remains bounded.");}
 else if(!std::strcmp(action,"target")&&number(gi.argv(2),id)&&id<ENTITYNUM_WORLD){
  const auto* root=jar::bspSection::SectionLiftRoot(g_entities[id]);
  if(!root)root=G_BSPSectionBreakableRoot(g_entities[id]);
  if(root)id=root->s.number;
  if(editable(g_entities[id])){selected=id;message("Entity selected.");}
 }

 else if(!std::strcmp(action,"give")){
   const char*name=gi.argv(2);
   if(!std::strcmp(name,"physics_gun")||!std::strcmp(name,"toolgun")){
    if(sandboxReady()){Weapon w;w.tool=!std::strcmp(name,"physics_gun")?1:2;w.family=w.tool==1?WP_PHYSGUN:WP_TOOLGUN;w.sandbox=1;
     reconcile();const auto id=Add(state,w);if(!id)message("Carried inventory is full; nothing granted.");
     else {workshopPlayer()->client->ps.stats[STAT_WEAPONS]|=1u<<w.family;RegisterItem(FindItemForWeapon(weapon_t(w.family)));CG_RegisterWeapon(w.family);message("Tool added to the weapon bar. Scroll to select it.");list();}
    }
   }else grant(name);
  }
 else if(!std::strcmp(action,"equip")){
   const char*name=gi.argv(2);const uint32_t wanted=!std::strcmp(name,"physics_gun")?1:!std::strcmp(name,"toolgun")?2:0;uint32_t found=0;
   for(unsigned i=0;i<state.count;++i)if(wanted && state.weapons[i].tool==wanted)found=state.weapons[i].id;
   if(found)equip(found);else message("Add this tool from the Sandbox menu before selecting it.");
  }
  else if(!std::strcmp(action,"mode")){
    if(sandboxReady())for(int mode=0;mode<toolModeCount;++mode)if(!std::strcmp(gi.argv(2),toolModes[mode])){jointFirst=0;linkFirst=0;toolMode=mode;gi.cvar_set("jar_toolMode",va("%d",toolMode));message(va("Toolgun mode: %s",toolModes[mode]));break;}
   }
   else if(!std::strcmp(action,"tool"))tool(gi.argv(2));
 else if(!std::strcmp(action,"undo")){undoEdit();list();}
 else if(!std::strcmp(action,"cleanup")){
  if(despawnAllAvailable()){cleanup();G_PortableShieldCleanup();}
  else message("Despawn All blocked by an occupied object or a section supporting an actor. Move clear and retry.");
  list();
 }
 else if(!std::strcmp(action,"constraints")){constraintReport();}
  else if(!std::strcmp(action,"targets")){if(gi.JAR_Log)for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(editable(e))gi.JAR_Log(0,"sandbox","target","entity=%d authored=%d movable=%d classname=%s xyz=%.2f,%.2f,%.2f yaw=%.1f",i,owned(i)?0:1,movable(e),e.classname,e.currentOrigin[0],e.currentOrigin[1],e.currentOrigin[2],e.currentAngles[1]);}}
 else {list();if(gi.JAR_Log){gi.JAR_Log(0,"sandbox","status","enabled=%d owned=%u selected=%u held=%u tool=%u mode=%d disarmed=%d",enabled(),state.ownedCount,selected,held,activeTool,toolMode,toolRelease);
  for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];const auto&e=g_entities[o.entity];gi.JAR_Log(0,"sandbox","owned","entity=%u id=%u catalog=%u frozen=%u parent=%u helper=%d occupied=%d xyz=%.2f,%.2f,%.2f yaw=%.1f contents=%d",o.entity,o.id,o.catalog,o.frozen,o.parent,e.NPC&&e.NPC->tempGoal?e.NPC->tempGoal->s.number:-1,occupied(e),e.currentOrigin[0],e.currentOrigin[1],e.currentOrigin[2],e.currentAngles[1],e.contents);}}}
 return qtrue;
}

// Input remains disarmed through switches, protected actions and save/load until
// both physical triggers are released. Native weapon selection still owns Pmove.
void G_JAR_WorkshopInput(gentity_t *ent,usercmd_t *cmd){
 if(!ent||ent->s.number!=0||!cmd)return;
 // Retain the gesture's ownership of this press through drop, freeze or a
 // weapon switch. The next fresh USE press goes to the ordinary game path.
 cmd->buttons=jar::context::PhysgunUseButtons(cmd->buttons,BUTTON_USE,held!=noEntity,physgunConsumedUse);
 const int attacks=BUTTON_ATTACK|BUTTON_ALT_ATTACK,buttons=cmd->buttons&attacks;
 if(enabled() && JAR_ToolWeapon(cg.weaponSelect)){
  uint32_t wanted=0;for(unsigned i=0;i<state.count;++i)if(state.weapons[i].family==unsigned(cg.weaponSelect) && state.weapons[i].tool)wanted=state.weapons[i].id;
  if(wanted && wanted!=activeTool){if(!activeTool && !JAR_ToolWeapon(ent->client->ps.weapon))previousFamily=ent->client->ps.weapon;stopTool();activeTool=wanted;}
 }
 auto*w=Find(state,activeTool);
 if(JAR_ToolWeapon(ent->client->ps.weapon) && !enabled()){
  cmd->buttons&=~attacks;
  for(int f=1;f<MAX_PLAYER_WEAPONS;++f)if(!JAR_ToolWeapon(f) && (ent->client->ps.stats[STAT_WEAPONS]&(1u<<f))){CG_ChangeWeapon(f);cmd->weapon=f;break;}
 }
 if(toolRestorePending){
  toolRestorePending=false;
  if(w && enabled() && ent->client->ps.weapon==int(w->family)){CG_ChangeWeapon(w->family);cmd->weapon=w->family;}
  else {stopTool();w=nullptr;}
 }
 if(w && (!enabled() || cg.weaponSelect!=int(w->family))){if(!enabled() && equipReady() && previousFamily>0)CG_ChangeWeapon(previousFamily);stopTool();w=nullptr;}
 if(!w){if(JAR_ToolWeapon(ent->client->ps.weapon))cmd->buttons&=~attacks;if(toolRelease){cmd->buttons&=~attacks;if(!buttons)toolRelease=false;}return;}
 cmd->buttons&=~attacks;
 if(!usable() || ent->client->ps.weapon!=int(w->family) || !equipReady()){
  jointFirst=0;releaseHeld();toolRelease=true;toolButtons=buttons;return;
 }
 if(toolRelease){if(!buttons){toolRelease=false;toolButtons=0;}return;}
 const int pressed=buttons&~toolButtons;toolButtons=buttons;
 if(w->tool==1){
  if(pressed&BUTTON_ATTACK)tool("grab");
  if(held!=noEntity&&(cmd->buttons&BUTTON_USE))
   cmd->buttons=jar::context::PhysgunUseButtons(cmd->buttons,BUTTON_USE,true,physgunConsumedUse);
  if(pressed&BUTTON_ALT_ATTACK){if(held!=noEntity){releaseHeld(true);toolRelease=true;message("Entity frozen. Release both triggers before grabbing again.");}else {select();if(selected!=noEntity)tool("release");}}
  if(!(buttons&BUTTON_ATTACK) && held!=noEntity)releaseHeld();
 }else if(pressed&attacks){
  const auto trace=toolSurfaceTrace();VectorCopy(trace.endpos,toolShotEnd);toolShotUntil=cg.time+140;
  G_Sound(ent,G_SoundIndex(JAR_TOOLGUN_SOUND));
  const bool alternate=(pressed&BUTTON_ALT_ATTACK)!=0;
  if(attachmentTool(toolModes[toolMode],&trace,alternate))return;
  if(alternate)toolAlternate(trace);
  else if(toolMode==8 && !duplicateSnapshot.objects.empty())duplicatePaste();
  else {
   vec3_t contact;
   if(selectToolShot(trace,contact)&&toolMode)tool(toolModes[toolMode],contact);
  }
 }
}

const char *G_JAR_WorkshopToolHint(){
 auto*w=Find(state,activeTool);if(!w||!enabled()||!usable()||cg.weaponSelect!=int(w->family))return nullptr;
 if(w->tool==1)return held!=noEntity?"PHYSGUN | USE + mouse: rotate | Wheel: distance | Right: freeze | Release: throw":"PHYSGUN | Left: grab entity | Right: unfreeze";
 return va("TOOLGUN / %s | Left: apply | Right: alternate | Hold Q: properties",toolModes[toolMode]);
}
void G_JAR_WorkshopViewRay(vec3_t origin,vec3_t direction){
 VectorCopy(cg.refdef.vieworg,origin);VectorCopy(cg.refdef.viewaxis[0],direction);
 if(VectorNormalize(direction)==0)eye(origin,direction);
}

bool G_JAR_WorkshopOutsideMapPoint(const vec3_t origin){
 return outsideWorkshopMap(origin,vec3_origin,vec3_origin);
}

bool G_JAR_WorkshopOwns(const gentity_t*e){if(!e||!e->inuse)return false;auto*o=owned(e->s.number);return o&&matches(*o);}
void G_JAR_WorkshopTrackConsole(gentity_t*e){
 if(!ready||!enabled()||!e||!e->inuse||e->freeAfterEvent||e->s.number<MAX_CLIENTS||e->s.number>=ENTITYNUM_WORLD||owned(e->s.number))return;
 if(!Own(state,e->s.number,consoleCatalog))return;
 spawnCountDirty=true;
 rememberEdit(*e,true);
}
void G_JAR_WorkshopTrackConsoleChild(gentity_t*root,gentity_t*child){
 const auto* record=owned(root->s.number);
 if(!record||record->catalog!=consoleCatalog||owned(child->s.number)||child->freeAfterEvent||
    (child->classname&&!strcmp(child->classname,"NPC_goal")))return;
 const auto parent=record->id;
 Own(state,child->s.number,consoleCatalog,parent);
}
void G_JAR_WorkshopConsoleActor(gentity_t*spawner,gentity_t*e){
 const auto* record=owned(spawner->s.number);
 if(record&&record->catalog==consoleCatalog)G_JAR_WorkshopTrackConsole(e);
}
bool G_JAR_WorkshopActorSpawn(const gentity_t& e){
 const auto* record=owned(e.s.number);
 return (e.spawnflags&JAR_NPC_OVERLAP_SPAWN)||(record&&record->catalog==consoleCatalog);
}
bool G_JAR_WorkshopTrackChild(gentity_t*source,gentity_t*child){return trackChild(source,child);}

bool G_JAR_WorkshopTrackDrop(gentity_t*source,gentity_t*drop){
 if(!G_JAR_WorkshopOwns(source))return true;
 if(!drop||!drop->item||drop->item->giType!=IT_WEAPON)return false;
 return G_JAR_WorkshopTrackChild(source,drop);
}

static void workshopOutline(const gentity_t&e,vec3_t color,float width,float alpha,float padding){
 vec3_t corners[8];if(jar::bspSection::Portable(e))jar::bspSection::Corners(e,corners,padding);else jar::volumeEntity::Corners(e,corners,padding);
 for(int a=0;a<3;++a)for(int mask=0;mask<8;++mask)if(!(mask&(1<<a)))
  FX_AddLine(corners[mask],corners[mask|(1<<a)],1,width,0,alpha,alpha,color,color,1,cgi_R_RegisterShader("white"));
}
void G_JAR_WorkshopDrawBeam(){
 if(in_camera || cg_pano.integer || cg.hyperspace)return;
 if(enabled()&&gi.cvar("jar_contextOpen","0",0)->integer&&contextValid()){
  vec3_t color={1,.8f,.2f};workshopOutline(g_entities[contextEntity],color,1,.85f,1);
  vec3_t railCorners[4];if(G_RailPreview(g_entities[contextEntity],railCorners))for(int i=0;i<4;++i)FX_AddLine(railCorners[i],railCorners[(i+1)%4],1,1,0,.85f,.85f,color,color,1,cgi_R_RegisterShader("white"));
 }
 // Render the same oriented shape used for portable volume membership/picking.
 // Disabled volumes remain selectable, with a muted outline. Bound nearby work.
 if(enabled() && activeTool){unsigned drawn=0;for(unsigned i=0;i<state.ownedCount&&drawn<32;++i){
  const auto&e=g_entities[state.owned[i].entity];if(!e.inuse || (!(e.contents&CONTENTS_TRIGGER)&&!jar::volumeEntity::Portable(e)) || Distance(cg.refdef.vieworg,e.currentOrigin)>1024)continue;
  vec3_t color={.3f,.8f,1};if(!(e.contents&CONTENTS_TRIGGER)||(e.svFlags&SVF_INACTIVE))VectorSet(color,.5f,.5f,.5f);
  workshopOutline(e,color,.6f,.5f,0);
  if(e.classname&&!strcmp(e.classname,"trigger_bidirectional")){
   vec3_t axis[3],center,end,tip;AnglesToAxis(e.currentAngles,axis);VectorCopy(e.currentOrigin,center);
   for(int a=0;a<3;++a)VectorMA(center,(e.pos1[a]+e.pos2[a])*.5f,axis[a],center);
   VectorMA(center,48,axis[0],end);FX_AddLine(center,end,1,1,0,.8f,.8f,color,color,1,cgi_R_RegisterShader("white"));
   for(int side:{-1,1}){VectorMA(end,-12,axis[0],tip);VectorMA(tip,side*8.f,axis[1],tip);FX_AddLine(end,tip,1,1,0,.8f,.8f,color,color,1,cgi_R_RegisterShader("white"));}
  }++drawn;
 }}
 if(!G_JAR_WorkshopToolHint())return;
 auto*w=Find(state,activeTool);if((w->tool==1 && held==noEntity)||(w->tool==2 && cg.time>=toolShotUntil))return;
 const auto&render=workshopPlayer()->client->renderInfo;if(cg.time-render.mPCalcTime>100||!render.mPCalcTime)return;
 vec3_t start,end;VectorCopy(render.muzzlePoint,start);
 if(w->tool==1){if(held>=ENTITYNUM_WORLD||!g_entities[held].inuse)return;
  const auto& e=g_entities[held];
  const auto& rendered=cg_entities[held];
  const bool useRenderedPose=rendered.currentValid&&rendered.gent==&e;
  nativeVector(physgunGrip.Point(pv(useRenderedPose?rendered.lerpOrigin:e.currentOrigin),
                                pv(useRenderedPose?rendered.lerpAngles:e.currentAngles)),end);
  // One-frame stock laser particles follow the rendered muzzle and object.
  // Long-lived electrical arcs left several different beam positions visible.
  vec3_t color={JAR_PHYSGUN_COLOR};FX_AddLine(start,end,1,JAR_PHYSGUN_WIDTH,0,1,1,color,color,1,cgi_R_RegisterShader(JAR_PHYSGUN_SHADER));
 }else {VectorCopy(toolShotEnd,end);vec3_t color={JAR_TOOLGUN_COLOR};FX_AddLine(start,end,1,JAR_TOOLGUN_WIDTH,0,1,0,color,color,40,cgi_R_RegisterShader(JAR_TOOLGUN_SHADER),FX_ALPHA_LINEAR);}
}

void G_JAR_WorkshopDrawRopes(){
 if(cg.hyperspace)return;
 const qhandle_t shader=cgi_R_RegisterShader("white");
 for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];
  if(j.kind==jar::constraint::Thruster){
   auto*owner=attachmentEntity(j.first);vec3_t point,axis;if(!owner||G_SandboxUsableDormant(*owner)||!attachmentPoint(*owner,j.a,point,axis))continue;
   refEntity_t housing{};housing.hModel=cgi_R_RegisterModel(JAR_THRUSTER_MODEL);VectorCopy(point,housing.origin);VectorCopy(point,housing.oldorigin);
   vectoangles(axis,housing.angles);housing.angles[PITCH]+=90;AnglesToAxis(housing.angles,housing.axis);
   housing.shaderRGBA[0]=housing.shaderRGBA[1]=housing.shaderRGBA[2]=housing.shaderRGBA[3]=255;cgi_R_AddRefEntityToScene(&housing);
   if(j.enabled){vec3_t end,color={1,.6f,.15f};VectorMA(point,40,axis,end);FX_AddLine(point,end,1,2,0,.7f,.7f,color,color,1,cgi_R_RegisterShader("gfx/effects/redLine"));}
   continue;
  }
  if(j.kind!=jar::constraint::Rope)continue;
  const auto*x=attachmentEntity(j.first),*y=attachmentEntity(j.second);vec3_t start,end,axis;
  if(!x||!y||G_SandboxUsableDormant(*x)||G_SandboxUsableDormant(*y)||
     !attachmentPoint(*x,j.a,start,axis)||!attachmentPoint(*y,j.b,end,axis))continue;
  const jarRopeVertex_t* mesh=nullptr;
  const int count=ropeMesh(j,mesh);
  for(int at=0;at<count;at+=4){polyVert_t polygon[4];for(int v=0;v<4;++v){
   VectorCopy(mesh[at+v].xyz,polygon[v].xyz);polygon[v].st[0]=mesh[at+v].st[0];polygon[v].st[1]=mesh[at+v].st[1];
   memcpy(polygon[v].modulate,mesh[at+v].color,4);
  }cgi_R_AddPolyToScene(shader,4,polygon);}
 }
}

// The carried records persist distinct saber definitions, but the weapon bar is
// the primary selector. Tools have their own native IDs and possession bits.
int G_JAR_WorkshopWeaponBar(JarWeaponBarEntry* entries,int capacity,int& selectedIndex){
 selectedIndex=0;if(!workshopPlayer() || G_IsRidingVehicle(workshopPlayer()))return 0;
 reconcile();unsigned sabers=0;bool extended=false;
 for(unsigned i=0;i<state.count;++i){const auto&w=state.weapons[i];sabers+=w.family==WP_SABER;extended|=w.tool && enabled();}
 if(!extended && sabers<2)return 0;
 const auto&ps=workshopPlayer()->client->ps;const auto current=G_JAR_WorkshopCurrentWeapon();int count=0;
 const unsigned order[]={1,2,3,4,5,6,7,8,13,9,10,11,12,14,29,30};
 for(unsigned family:order)for(unsigned i=0;i<state.count && count<capacity;++i){const auto&w=state.weapons[i];
  if(w.family!=family || !(ps.stats[STAT_WEAPONS]&(1u<<family)) || (w.tool && !enabled()))continue;
  const char*name=w.tool==1?"Physgun":w.tool==2?"Toolgun":family==1?saberLabel(w.saber[0]):families[family];
  const char*icon=weaponData[family].weaponIcon;
  if(family==1){if(!std::strcmp(w.saber[0],"jar_shortblade"))icon="jar/icons/shortblade";
   else if(!std::strcmp(w.saber[0],"jar_pike"))icon="jar/icons/pike";
   else if(!std::strcmp(w.saber[0],"sith_sword"))icon="jar/icons/sword";}
  if(cg.weaponSelect==int(family) && (family!=1 || SameChoice(w,current)))selectedIndex=count;
  entries[count++]={w.id,w.family,name,icon};
 }
 return count;
}
bool G_JAR_WorkshopCycleWeapon(int direction){
 if(held!=noEntity && activeTool && enabled() && cg.weaponSelect==WP_PHYSGUN){
  holdDistance=JAR_PhysgunWheelDistance(holdDistance,JAR_PHYSGUN_MAX_DISTANCE,-direction);
  return true;
 }

 JarWeaponBarEntry entries[inventoryLimit];int selectedIndex=0;
 const int count=G_JAR_WorkshopWeaponBar(entries,inventoryLimit,selectedIndex);if(!count)return false;
 if(!equipReady() || cg.weaponSelectTime+200>cg.time)return true;
 extern qboolean CG_WeaponSelectable(int,int,qboolean);
 for(int n=1;n<count;++n){const auto&choice=entries[(selectedIndex+count+n*direction)%count];
  if(CG_WeaponSelectable(choice.family,cg.weaponSelect,qfalse)){equip(choice.id);break;}}
 return true;
}

bool G_JAR_WorkshopControls(const gentity_t*ent){return ent&&(nativeOwnedControl(ent->s.number)||nativeControlledRider(*ent));}
bool G_JAR_WorkshopTransportAllowed(const gentity_t& entity){
 return !G_JAR_WorkshopControls(&entity)&&held!=unsigned(entity.s.number)&&!Constrained(state,attachmentKey(entity));
}
bool G_JAR_WorkshopTeleportAssembly(gentity_t& source,gentity_t& actor){
 if(!ready||!enabled()||G_JAR_WorkshopControls(&actor)||held==unsigned(actor.s.number))return false;
 const auto root=attachmentKey(actor);
 return (actor.m_pVehicle||Constrained(state,root))&&teleportAssembly(root,source,assemblyTeleportBudget());
}
bool G_JAR_WorkshopTeleportProjectile(gentity_t& projectile,const vec3_t previous,const vec3_t end,bool clipped,
 const float* rollingVelocity){
 return (teleportObjectVolumes()&jar::teleport::Projectile)&&
  G_TeleportSweepProjectile(projectile,previous,end,clipped,rollingVelocity);
}
bool G_JAR_WorkshopNativeInput(gentity_t*ent,usercmd_t*cmd){
 if(!ent||!ent->client||!cmd||!G_JAR_WorkshopControls(ent))return false;
 ent->client->ps.commandTime=cmd->serverTime;VectorClear(ent->client->ps.velocity);return true;
}
void G_JAR_WorkshopTouch(gentity_t*actor,gentity_t*target){
 if(!actor||!actor->client||!target||!physicsWorld.Active())return;auto*o=owned(target->s.number);
 if(!o||!prop(o->catalog)||o->frozen||held==o->entity||actor->client->ps.groundEntityNum==target->s.number)return;
 const auto&cmd=actor->client->usercmd;vec3_t forward,right,wish,angles={0,actor->client->ps.viewangles[1],0};AngleVectors(angles,forward,right,nullptr);
 for(int a=0;a<3;++a)wish[a]=forward[a]*cmd.forwardmove+right[a]*cmd.rightmove;
 if(VectorNormalize(wish)<.01f)return;static int pushTime[MAX_GENTITIES]{};auto&previous=pushTime[actor->s.number];if(previous==level.time)return;
 const float elapsed=std::min(50,std::max(1,level.time-previous))*.001f;previous=level.time;
 const float impulse=80.f*std::min(320.f,float(actor->client->ps.speed))*elapsed;
 VectorScale(wish,impulse,wish);physicsWorld.Impulse(target->s.number,pv(wish),pv(actor->currentOrigin));
}
