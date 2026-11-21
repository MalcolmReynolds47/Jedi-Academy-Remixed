// Exercise production spawn transactions, capacity rollback and native dispatch.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "game/g_always_trigger.h"
#include "game/g_contact_trigger.h"
#include "memory_save.h"
#include "../../shared/cgame/jar_logic_marker_sprite.h"
#include "game/g_reference_tags.h"
#include "game/g_combat_points.h"
#include "game/g_skyportal.h"
#include "game/g_level_change.h"
#include "game/g_workshop.h"
#include "game/g_spawn_transaction.h"
#include "game/g_maglock.h"
#include "game/g_laser_arm.h"
#include "game/g_portal.h"
#include "game/g_spotlight.h"
#include "game/g_welder.h"
#include "game/g_door_camera.h"
#include "game/g_crystal_crate.h"
#include "game/g_bsp_section.h"
#include "game/g_siege_item.h"
#include <stdexcept>
bool G_BSPSectionSpawn(gentity_t*){throw std::runtime_error("BSP section uses its dedicated native fixture");}
#include "game/g_target_markers.h"
#include "game/g_target_music.h"
#include "game/b_local.h"
#include "test_support.h"
#include "game/g_script_parms.h"
#include <deque>
#include <string>
#include <memory>
#include <map>
#include <cstdarg>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];gentity_t*player=&g_entities[0];
cvar_t skill{};cvar_t*g_spskill=&skill;
static qboolean spawning=qfalse;
qboolean NPCsPrecached=qtrue;
static int numSpawnVars=0,numSpawnVarChars=0;
static char*spawnVars[MAX_SPAWN_VARS][2];static char spawnVarChars[MAX_SPAWN_VARS_CHARS];
static std::deque<parms_t> parameterStorage;
void*G_Alloc(int size){CHECK(size==sizeof(parms_t));parameterStorage.emplace_back();return &parameterStorage.back();}
static std::deque<std::string> strings;
static std::vector<gentity_t*> entities;
static int capacity=10;static std::string dispatched;static bool reject=false,exhaustNative=false;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void AddSpawnField(char*k,char*v){CHECK(numSpawnVars<MAX_SPAWN_VARS);spawnVars[numSpawnVars][0]=G_NewString(k);spawnVars[numSpawnVars++][1]=G_NewString(v);}
void G_ParseField(const char*k,const char*v,gentity_t*e){
 if(!strcmp(k,"usescript"))e->behaviorSet[BSET_USE]=G_NewString(v);
 if(!strcmp(k,"parm1"))jar::entityParms::Native(*e,0,v);
 if(!strcmp(k,"health"))e->health=atoi(v);if(!strcmp(k,"wait"))e->wait=float(atof(v));if(!strcmp(k,"speed"))e->speed=float(atof(v));
 if(!strcmp(k,"fxFile"))e->fxFile=G_NewString(v);if(!strcmp(k,"message"))e->message=G_NewString(v);
}
gentity_t*G_TrySpawn(qboolean){if(capacity--<=0)return nullptr;const int slot=100+int(entities.size());CHECK(slot<MAX_GENTITIES);auto*e=&g_entities[slot];entities.push_back(e);e->inuse=qtrue;e->s.number=slot;globals.num_entities=slot+1;return e;}
void G_FreeEntity(gentity_t*e){G_SkyPortalForget(e);G_CombatPointForget(e);G_ReferenceForget(e);G_CrystalCrateForget(e);G_DoorCameraForget(e);G_WelderForget(e);G_SpotlightForget(e);G_PortalForget(e);G_LaserArmForget(e);G_MusicForget(e);G_TargetMarkersForget(e);G_MaglockForget(e);G_SiegeForget(e);e->inuse=qfalse;}
qboolean G_ClearLineOfSight(const vec3_t,const vec3_t,int,int){return qtrue;}
void CalcEntitySpot(gentity_t*e,spot_t,vec3_t p){VectorCopy(e->currentOrigin,p);}
void G_SetOrigin(gentity_t*e,const vec3_t v){VectorCopy(v,e->currentOrigin);VectorCopy(v,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t v){VectorCopy(v,e->currentAngles);}
int G_ModelIndex(const char*){return 5;}
void SP_target_print(gentity_t*e){e->e_UseFunc=useF_Use_Target_Print;}
void SP_path_corner(gentity_t*){}
void SP_target_position(gentity_t*){}
extern void SP_misc_model_gun_rack(gentity_t*);extern void SP_misc_model_ammo_rack(gentity_t*);extern void SP_misc_camera(gentity_t*);extern void SP_trigger_entdist(gentity_t*);
extern void SP_misc_maglock(gentity_t*);extern void SP_object_cargo_barrel1(gentity_t*);
extern void SP_trigger_space(gentity_t*);extern void SP_trigger_bidirectional(gentity_t*);
extern void SP_trigger_shipboundary(gentity_t*);extern void SP_fx_cloudlayer(gentity_t*);extern void SP_misc_cubemap(gentity_t*);
#include "game/g_weather.h"
extern void SP_target_scriptrunner(gentity_t*);
extern void SP_target_play_music(gentity_t*);extern void SP_target_change_parm(gentity_t*);
extern void SP_misc_ns_turret(gentity_t*);
extern void SP_target_interest(gentity_t*);extern void SP_target_location(gentity_t*);
extern void SP_trigger_always(gentity_t*);
static void G_VisibleSpawn(gentity_t*);
void SP_func_door(gentity_t*e){G_JAR_SetBrushModel(e,e->model);e->s.eType=ET_MOVER;e->e_UseFunc=useF_Use_BinaryMover;}
void SP_func_usable(gentity_t*);
void InitMover(gentity_t*e){e->s.eType=ET_MOVER;}
gentity_t* G_TestEntityPosition(gentity_t*){return nullptr;}
extern void trigger_entdist_use(gentity_t*,gentity_t*,gentity_t*);extern void camera_use(gentity_t*,gentity_t*,gentity_t*);extern void camera_aim(gentity_t*);
extern void camera_die(gentity_t*,gentity_t*,gentity_t*,int,int,int,int);
#include "game/g_target_movement.inc"
#include "game/g_sandbox_laser.h"
#include "game/g_sandbox_station.h"
#include "game/g_sandbox_usable.h"
#include "game/Q3_Interface.h"
IGameInterface* IGameInterface::GetGame(int){throw std::runtime_error("Unexpected authored debug call");}
void CQuake3GameInterface::DebugPrint(e_DebugPrintLevel,const char*,...){}
static bool restoredLogicIdentities=false;
uint64_t G_EntityLifetime(const gentity_t* entity){return entity&&entity->inuse&&!restoredLogicIdentities?1:0;}
void G_SetEnemy(gentity_t* self,gentity_t* enemy){self->enemy=enemy;}
gentity_t* G_PickTarget(char* name){return G_Find(nullptr,FOFS(targetname),name);}
void AddScore(gentity_t* actor,int points){if(actor&&actor->client)actor->client->ps.persistant[PERS_SCORE]+=points;}
static int fadeCalls=0;
void CGCam_Fade(vec4_t,vec4_t,float duration){CHECK(duration==10000);++fadeCalls;}
static int shownScore=0;
static void CaptureScore(int recipient,const char* format,...){
 CHECK(recipient==0&&!strcmp(format,"cp \"Score: %d\""));
 va_list values;va_start(values,format);shownScore=va_arg(values,int);va_end(values);
}
#include "game/g_subbsp_ownership.h"
G_SubBSPCapture::G_SubBSPCapture(const gentity_t*){}
G_SubBSPCapture::~G_SubBSPCapture(){}
static gentity_t* foreignSiegeGoal;
gentity_t* G_SubBSPOwner(const gentity_t* e){return e==foreignSiegeGoal?foreignSiegeGoal:nullptr;}
uint64_t G_SubBSPNamespace(const gentity_t*){return 0;}
void G_SubBSPAllocated(gentity_t*){}
#include "game/g_target_logic.inc"
#include "game/g_logic_timer.inc"
bool G_JAR_WorkshopUseGate(gentity_t&,gentity_t*,gentity_t*){return false;} // Workshop behavior uses the production Context fixture.
#include "game/g_use_gate_targets.inc"
static cvar_t mapGravity{};
cvar_t* g_gravity=&mapGravity;
void InitTrigger(gentity_t*);
qboolean G_CallSpawn(gentity_t*e){dispatched=e->classname;if(exhaustNative)throw G_SpawnCapacity{};if(reject)return qfalse;
 if(dispatched=="misc_model_gun_rack")SP_misc_model_gun_rack(e);
 if(dispatched=="misc_model_ammo_rack")SP_misc_model_ammo_rack(e);
 if(dispatched=="misc_camera")SP_misc_camera(e);
 if(dispatched=="misc_ns_turret")SP_misc_ns_turret(e);
 if(dispatched==jar::siege::objectiveClass)SP_info_siege_objective(e);
 if(dispatched==jar::siege::endClass)SP_target_siege_end(e);
 if(dispatched=="trigger_entdist")SP_trigger_entdist(e);
 if(dispatched=="trigger_always")SP_trigger_always(e);
 if(dispatched=="trigger_visible")G_VisibleSpawn(e);
 if(dispatched=="trigger_push"){
  InitTrigger(e);e->e_TouchFunc=touchF_trigger_push_touch;e->speed=1000;e->wait*=1000;
  e->e_ThinkFunc=thinkF_AimAtTarget;e->nextthink=500;
 }
 if(dispatched=="target_push")e->e_UseFunc=useF_Use_target_push;
 if(dispatched=="target_laser")CHECK(G_SandboxLaserInitialize(*e,{}));
 if(dispatched=="func_usable")SP_func_usable(e);
 if(dispatched=="func_breakable"||dispatched=="misc_model_breakable"){
  e->s.modelindex=5;e->s.eType=ET_MOVER;e->radius=1;
  int material=0;G_SpawnInt("material",dispatched=="misc_model_breakable"?"8":"0",&material);
  e->material=static_cast<material_t>(material);
  e->e_UseFunc=dispatched=="misc_model_breakable"?useF_misc_model_use:useF_funcBBrushUse;
  e->e_DieFunc=dispatched=="misc_model_breakable"?dieF_misc_model_breakable_die:dieF_funcBBrushDie;
  if(dispatched=="func_breakable")G_JAR_SetBrushModel(e,e->model);
  gi.linkentity(e);
 }
 const auto station=jar::sandboxStation::Class(e->classname);
 if(station!=jar::sandboxStation::None){
  e->e_UseFunc=station==jar::sandboxStation::Ammo?useF_ammo_power_converter_use:useF_shield_power_converter_use;
  e->count=75;e->contents=CONTENTS_SOLID;e->s.modelindex=5;e->svFlags|=SVF_PLAYER_USABLE;
  if(strstr(e->classname,"floor_unit"))e->s.eFlags|=EF_SHADER_ANIM;
  gi.linkentity(e);
 }
 if(dispatched=="trigger_teleport"){
  InitTrigger(e);e->e_TouchFunc=touchF_trigger_teleporter_touch;
  e->e_ThinkFunc=thinkF_trigger_teleporter_find_closest_portal;e->nextthink=500;
 }
 if(dispatched=="target_teleporter")e->e_UseFunc=useF_target_teleporter_use;
 if(jar::contactTrigger::Class(e->classname)==jar::contactTrigger::Hurt){
  InitTrigger(e);e->e_TouchFunc=touchF_hurt_touch;e->wait*=1000;gi.linkentity(e);
 }else if(jar::contactTrigger::Class(e->classname)){
  InitTrigger(e);e->e_UseFunc=useF_Use_Multi;e->e_TouchFunc=touchF_Touch_Multi;
  if(dispatched=="trigger_once")e->wait=-1;
 }
 if(dispatched=="misc_crystal_crate")G_CrystalCrateSpawn(e);
 if(dispatched=="misc_model_jabba_cam")G_DoorCameraSpawn(e);
 if(dispatched=="misc_model_welder")G_WelderSpawn(e);
 if(dispatched=="misc_spotlight")G_SpotlightSpawn(e);
 if(dispatched=="misc_laser_arm")G_LaserArmSpawn(e);
 if(dispatched=="misc_maglock")SP_misc_maglock(e);
 if(dispatched=="object_cargo_barrel1")SP_object_cargo_barrel1(e);
 if(dispatched=="trigger_bidirectional")SP_trigger_bidirectional(e);
 if(jar::weather::Class(dispatched.c_str())!=jar::weather::Invalid)CHECK(G_WeatherSpawnPortable(e));
 if(dispatched=="fx_cloudlayer")SP_fx_cloudlayer(e);
 if(dispatched=="misc_cubemap")SP_misc_cubemap(e);
 if(dispatched=="trigger_shipboundary")SP_trigger_shipboundary(e);
 if(dispatched=="trigger_space")SP_trigger_space(e);
 if(dispatched=="target_interest")SP_target_interest(e);
 if(dispatched=="target_location")SP_target_location(e);
 if(dispatched=="target_scriptrunner")SP_target_scriptrunner(e);
 if(dispatched=="target_change_parm")SP_target_change_parm(e);
 if(dispatched=="target_level_change"){if(!G_LevelChangeSpawn(*e))G_FreeEntity(e);}
 if(dispatched=="target_play_music")SP_target_play_music(e);
 if(dispatched=="target_delay")SP_target_delay(e);
 if(dispatched=="target_counter")SP_target_counter(e);
 if(dispatched=="target_random")SP_target_random(e);
 if(dispatched=="target_relay")SP_target_relay(e);
 if(dispatched=="target_score")SP_target_score(e);
 if(dispatched=="target_kill")SP_target_kill(e);
 if(dispatched=="target_give")SP_target_give(e);
 if(dispatched=="target_activate")SP_target_activate(e);
 if(dispatched=="target_deactivate")SP_target_deactivate(e);
 if(dispatched=="func_timer")SP_func_timer(e);
 if(dispatched=="target_gravity_change")SP_target_gravity_change(e);
 if(dispatched=="target_friction_change")SP_target_friction_change(e);
 return qtrue;
}
qboolean G_SpawnFloat(const char*k,const char*d,float*out){const char*v=d;for(int i=0;i<numSpawnVars;++i)if(!strcmp(k,spawnVars[i][0]))v=spawnVars[i][1];*out=float(atof(v));return v==d?qfalse:qtrue;}
qboolean G_SpawnInt(const char*k,const char*d,int*out){float v;auto found=G_SpawnFloat(k,d,&v);*out=int(v);return found;}
qboolean G_SpawnString(const char*k,const char*d,char**out){const char*v=d;for(int i=0;i<numSpawnVars;++i)if(!strcmp(k,spawnVars[i][0]))v=spawnVars[i][1];*out=const_cast<char*>(v);return v==d?qfalse:qtrue;}
qboolean G_SpawnVector(const char* key,const char* fallback,vec3_t out){
 char* text;
 const auto result=G_SpawnString(key,fallback,&text);
 CHECK(sscanf(text,"%f %f %f",&out[0],&out[1],&out[2])==3);
 return result;
}
void G_Error(const char*,...){throw std::runtime_error("Unexpected native spawn error");}
cvar_t buildScript{};cvar_t*com_buildScript=&buildScript;
static std::map<std::pair<int,int>,gitem_t> items;
gitem_t*Item(int type,int tag,const char*name){auto&i=items[{type,tag}];i.giType=itemType_t(type);i.giTag=tag;i.classname=name;i.quantity=20;return &i;}
gitem_t*FindItemForWeapon(weapon_t w){return Item(IT_WEAPON,w,"weapon");}
gitem_t*FindItemForAmmo(ammo_t a){return Item(IT_AMMO,a,"ammo");}
gitem_t*FindItem(const char*){return Item(IT_HEALTH,1,"item_medpak_instant");}
void RegisterItem(gitem_t*){}
void G_SpawnItem(gentity_t*e,gitem_t*i){e->item=i;e->s.eType=ET_ITEM;if(!e->count)e->count=i->quantity;}
void FinishSpawningItem(gentity_t*e){e->contents=CONTENTS_ITEM;}
int G_SoundIndex(const char*){return 1;}int G_EffectIndex(const char*){return 1;}
void G_Sound(gentity_t*,int){}
static std::string usedTarget;static int activations=0;static bool trackAllowed=true;static gentity_t*tracked=nullptr;
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
static gentity_t* outputActivator=nullptr;
static std::vector<int> giveAttempts;
static int rejectedPickup=-1;
static gentity_t* removeGiveOnTouch=nullptr;
void Touch_Item(gentity_t* pickup,gentity_t*,trace_t*){
 giveAttempts.push_back(pickup->s.number);
 if(pickup->s.number!=rejectedPickup)G_FreeEntity(pickup);
 if(removeGiveOnTouch)G_FreeEntity(removeGiveOnTouch);
}
static std::map<int,int> dispatchedOutputs;
void GEntity_UseFunc(gentity_t* destination,gentity_t*,gentity_t* actor){
 ++dispatchedOutputs[destination->s.number];
 outputActivator=actor;
}
void G_UseTargets2(gentity_t*,gentity_t* actor,const char*name){if(name){usedTarget=name;outputActivator=actor;++activations;}}
gentity_t*G_Find(gentity_t*after,int,const char*name){
 for(int i=after?after->s.number+1:0;i<globals.num_entities;++i){
  auto& e=g_entities[i];if(e.inuse&&e.targetname&&!strcmp(e.targetname,name))return &e;
 }
 return nullptr;
}
bool G_JAR_WorkshopOwns(const gentity_t*e){return e&&e->inuse&&e->targetname&&(!strncmp(e->targetname,"jar_sb_",7)||!strncmp(e->targetname,"jar_setup_",10));}
qboolean G_ClearViewEntity(gentity_t*e){e->client->ps.viewEntity=0;return qtrue;}
void G_SetViewEntity(gentity_t*e,gentity_t*view){e->client->ps.viewEntity=view->s.number;}
void SP_fx_runner(gentity_t*e){G_SetOrigin(e,e->s.origin);}
bool G_JAR_WorkshopTrackChild(gentity_t*,gentity_t*e){tracked=e;return trackAllowed;}
void G_JAR_WorkshopDetachChild(gentity_t*,gentity_t*){}
void WP_Explode(gentity_t*){} // Actual destruction is exercised in native_objects.
gentity_t*G_FindDoorTrigger(gentity_t*){return nullptr;}
char*vtos(const vec3_t){return const_cast<char*>("test origin");}
void InitTrigger(gentity_t*e){G_JAR_SetBrushModel(e,e->model);e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;}
qboolean G_PointInBounds(const vec3_t p,const vec3_t lo,const vec3_t hi){for(int i=0;i<3;++i)if(p[i]<lo[i]||p[i]>hi[i])return qfalse;return qtrue;}
static gentity_t* expectedKill=nullptr;
static int killCalls=0,killMethod=0;
static gentity_t* expectedEmitter=nullptr;
static int laserDamage=0,laserHits=0;
void G_Damage(gentity_t* target,gentity_t* source,gentity_t* attacker,const vec3_t,const vec3_t,int amount,int flags,int method,int){
 if(expectedEmitter){
  CHECK(target==&g_entities[0]&&source==expectedEmitter&&attacker==source&&source->alt_fire&&
        source->nextthink==level.time+jar::sandboxLaser::interval&&amount==laserDamage&&
        flags==DAMAGE_NO_KNOCKBACK&&method==MOD_ENERGY);
  ++laserHits;
  return;
 }
 if(!expectedKill)return;
 CHECK(target==expectedKill&&amount==100000&&flags==DAMAGE_NO_PROTECTION);
 ++killCalls;killMethod=method;target->health=0;
}
void G_SoundOnEnt(gentity_t*,soundChannel_t,const char*){}
void NPC_SetAnim(gentity_t*,int,int,int,int){}
struct TestPool : IGhoul2InfoArray {
 std::vector<CGhoul2Info> models;
 int New()override{return 1;}void Delete(int)override{models.clear();}bool IsValid(int h)const override{return h==1;}
 std::vector<CGhoul2Info>&Get(int)override{return models;}const std::vector<CGhoul2Info>&Get(int)const override{return models;}
};
IGhoul2InfoArray&_TheGhoul2InfoArray(){static TestPool pool;return pool;}
#include "game/g_rack_spawn.inc"
#include "game/g_security_camera.inc"
#define ENTDIST_PLAYER 1
#define ENTDIST_NPC 2
#include "game/g_trigger_distance.inc"
#include "game/g_maglock.inc"
#include "game/g_laser_arm.inc"
#include "game/g_portal.inc"
#include "game/g_spotlight.inc"
void G_PlayEffect(int,const vec3_t,const vec3_t){}
void G_RadiusDamage(const vec3_t,gentity_t*,float,float,gentity_t*,int){}
#include "game/g_welder.inc"
#include "game/g_door_camera.inc"
void G_PlayEffect(const char*,const vec3_t){}
#include "game/g_visible_trigger.h"
void G_UseTargets(gentity_t*entity,gentity_t*activator){
 if(G_AlwaysClass(*entity)){CHECK(entity==activator);G_UseTargets2(entity,activator,entity->target);}
 if(G_VisibleClass(*entity)){CHECK(activator==player);G_UseTargets2(entity,activator,entity->target);}
 if(jar::logic::Class(entity->classname)!=jar::logic::None||G_ContactKind(*entity))G_UseTargets2(entity,activator,entity->target);
 if(entity->e_UseFunc==useF_siege_objective_use||entity->e_UseFunc==useF_siege_end_use)
  G_UseTargets2(entity,activator,entity->target);
}
#include "game/g_siege_item.inc"
#include "game/g_always_trigger.inc"
bool G_ContactClockReady(const gentity_t&,const gentity_t&,int){throw std::runtime_error("Hurt contacts use the native fixture");}
static void PortableHurtDamage(gentity_t&,gentity_t&){throw std::runtime_error("Hurt damage uses the native fixture");}
#include "game/g_contact_trigger.inc"
static bool sightBlocked,sightPVS;
qboolean G_ClearTrace(const vec3_t,const vec3_t,const vec3_t,const vec3_t,int ignore,int mask){
 CHECK(ignore==0&&mask==MASK_OPAQUE);
 return sightBlocked?qfalse:qtrue;
}
#include "game/g_visible_trigger.inc"
static int startVisits;
static vec3_t startArrival,startView;
void TeleportPlayer(gentity_t*,vec3_t origin,vec3_t angles){++startVisits;VectorCopy(origin,startArrival);startArrival[2]+=1;VectorCopy(angles,startView);}
#include "game/g_start_marker.inc"
#include "game/g_crystal_crate.inc"
#include "game/g_cargo_barrel.inc"
#include "game/g_space.inc"
#include "game/g_bidirectional.inc"
void SetClientViewAngle(gentity_t*e,vec3_t angles){VectorCopy(angles,e->client->ps.viewangles);}
#include "game/g_shipboundary.inc"
#include "game/g_cloud.inc"
#include "game/g_reflection_probe.inc"
#include "game/g_weather.inc"
#include "reference_fixture.h"
static bool waypointAdmission(){return true;}
#include "game/g_waypoint_properties.inc"
const char*G_WaypointStatus(const gentity_t&){return "Native graph tested separately";}
#include "game/g_target_markers.inc"
#include "game/g_interest.inc"
void Q3_SetParm(int entity,int slot,const char*value){jar::entityParms::Native(g_entities[entity],slot,value);}
#include "game/g_target_parms.inc"
#include "game/g_scriptrunner_spawn.inc"
#include "game/g_target_music.inc"
#include "game/g_scene_camera.inc"
bool G_JAR_WorkshopControls(const gentity_t*){return false;}
void G_JAR_MoverPathOffset(const gentity_t&,vec3_t out){VectorClear(out);}
#include "game/g_rail_portable.inc"
#include "game/g_npc_classes.inc"
#define SPF_TURRETG2_TURBO 4
void TurboLaser_SetBoneAnim(gentity_t*,int,int){throw std::runtime_error("Unexpected turbo setup");}
#include "game/g_turret_spawn.inc"
#include "game/g_workshop_entities.inc"
#include "game/g_sliding_door.inc"
bool G_LiftTranslate(gentity_t&,float){throw std::runtime_error("Lift motion uses the context fixture");}
#include "game/g_button_sandbox.inc"
#include "game/g_bobbing_sandbox.inc"
#include "game/g_sandbox_laser.inc"
#define MAX_AMMO_GIVE 4
#include "game/g_sandbox_station_use.inc"
#undef MAX_AMMO_GIVE
void target_laser_think(gentity_t* source){sandboxLaserThink(*source);}
static int speakerEvents,speakerEvent,speakerReceiver,speakerSound;
void G_AddEvent(gentity_t* e,int event,int parm){
 ++speakerEvents;speakerEvent=event;speakerReceiver=e->s.number;speakerSound=parm;
}
#include "game/g_sandbox_speaker.inc"
#include "game/g_target_sp_speaker.inc"
static void CheckSpeaker(){
 using namespace jar::speaker;
 const int priorTime=level.time;
 const auto priorGet=gi.GetConfigstring;
 const auto priorSet=gi.SetConfigstring;
 static std::map<int,std::string> soundConfig;
 gi.GetConfigstring=[](int index,char* out,int size){Q_strncpyz(out,soundConfig[index].c_str(),size);};
 gi.SetConfigstring=[](int index,const char* text){soundConfig[index]=text;};
 level.time=1000;
 gentity_t source{};source.inuse=qtrue;source.s.number=53;
 source.classname=G_NewString("target_speaker");source.targetname=G_NewString("jar_sb_53");
 source.model=G_NewString(JAR_LOGIC_PLACEHOLDER_MODEL);
 gentity_t* helpers[jar::entities::helperLimit]{};int count=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&source,helpers,count)&&!count&&source.linked);
 CHECK(source.s.eType==ET_GENERAL&&source.s.modelindex==5&&!source.s.frame&&source.contents==CONTENTS_ITEM);
 CHECK(jar::entities::LogicMarker(source.classname,source.targetname,JAR_LOGIC_PLACEHOLDER_MODEL));
 speakerUse(source,nullptr);speakerUse(source,nullptr);
 CHECK(speakerEvents==1&&speakerEvent==EV_GENERAL_SOUND&&speakerReceiver==53&&speakerSound==source.noise_index);
 Settings s;CHECK(G_SandboxSpeakerRead(source,s)&&SameControl(s,{}));
 s.mode=Global;CHECK(G_SandboxSpeakerSet(source,s)&&(source.svFlags&SVF_BROADCAST));
 level.time+=oneShotCooldown;speakerUse(source,nullptr);CHECK(speakerEvents==2&&speakerEvent==EV_GLOBAL_SOUND);
 gentity_t actor{};gclient_t client{};actor.inuse=qtrue;actor.s.number=77;actor.client=&client;
 s.mode=Activator;CHECK(G_SandboxSpeakerSet(source,s)&&!(source.svFlags&SVF_BROADCAST));
 level.time+=oneShotCooldown;speakerUse(source,&actor);CHECK(speakerEvents==3&&speakerReceiver==77);
 s.mode=Loop;CHECK(!G_SandboxSpeakerSet(source,s));s.cue=defaultLoopCue;
 CHECK(G_SandboxSpeakerSet(source,s));speakerUse(source,&actor);CHECK(source.s.loopSound==source.noise_index);
 G_LogicSetUseEnabled(source,false);CHECK(!source.s.loopSound);speakerUse(source,&actor);CHECK(!source.s.loopSound);
 G_LogicSetUseEnabled(source,true);CHECK(source.s.loopSound==source.noise_index);speakerUse(source,&actor);CHECK(!source.s.loopSound);
 s.mode=Repeat;s.period=1200;s.variation=200;CHECK(G_SandboxSpeakerSet(source,s));
 speakerUse(source,&actor);const int first=source.nextthink;
 CHECK(speakerEvents==4&&first==level.time+1200);
 level.time+=500;G_LogicSetUseEnabled(source,false);CHECK(!source.nextthink&&source.fly_sound_debounce_time==700);
 level.time=first+500;G_SandboxSpeakerTick(source);CHECK(speakerEvents==4);
 MemorySave save;ojk::SavedGameHelper serializer(&save);gentity_t loaded{};
 source.sg_export(serializer);save.reset_buffer_offset();loaded.sg_import(serializer);serializer.ensure_all_data_read();
 loaded.classname=source.classname;loaded.model=source.model;
 CHECK(G_SandboxSpeakerRestore(loaded)&&!loaded.nextthink&&loaded.fly_sound_debounce_time==700);
 G_LogicSetUseEnabled(loaded,true);CHECK(loaded.nextthink==level.time+700);
 level.time=loaded.nextthink;G_SandboxSpeakerTick(loaded);
 CHECK(speakerEvents==5&&loaded.nextthink>=level.time+1000&&loaded.nextthink<=level.time+1400);
 speakerUse(loaded,&actor);CHECK(!loaded.nextthink);
 gentity_t authored{};authored.inuse=qtrue;authored.classname=source.classname;
 {
  G_SpawnContext spawn;
  AddSpawnField(G_NewString("noise"),G_NewString("sound/effects/mpalarm.wav"));
  AddSpawnField(G_NewString("wait"),G_NewString("1"));
  SP_target_speaker(&authored);
 }
 CHECK(!G_SandboxSpeaker(authored)&&authored.s.eType==ET_SPEAKER&&authored.s.frame==10);
 authored.targetname=G_NewString("jar_sb_54");
 CHECK(G_SandboxSpeakerRestore(authored)&&G_SandboxSpeakerRead(authored,s)&&s.mode==Repeat&&s.active&&s.period==1000);
 const auto registered=source.noise_index;
 gi.GetConfigstring=[](int,char* out,int size){Q_strncpyz(out,"sound/existing",size);};
 s.cue=1;CHECK(!G_SandboxSpeakerSet(source,s)&&source.noise_index==registered);
 gi.GetConfigstring=priorGet;gi.SetConfigstring=priorSet;level.time=priorTime;
}
static void CheckLaserEmitter(){
 using namespace jar::sandboxLaser;
 const auto priorTrace=gi.trace;
 const int priorTime=level.time;
 level.time=1500;
 auto& emitter=g_entities[53];
 emitter.inuse=qtrue;emitter.s.number=53;emitter.classname=G_NewString("target_laser");emitter.targetname=G_NewString("jar_sb_53");
 emitter.model=G_NewString("models/map_objects/imperial/crate1.md3");
 gentity_t* helpers[jar::entities::helperLimit]{};int count=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&emitter,helpers,count)&&!count&&emitter.linked&&emitter.s.eType==ET_GENERAL&&
       emitter.contents==CONTENTS_ITEM&&emitter.s.modelindex==5&&(emitter.svFlags&SVF_PLAYER_USABLE));
 CHECK(jar::entities::LogicMarker(emitter.classname,emitter.targetname,JAR_LOGIC_PLACEHOLDER_MODEL));
 Settings settings;
 CHECK(G_SandboxLaserRead(emitter,settings)&&Same(settings,{})&&emitter.nextthink==1600);
 expectedEmitter=&emitter;laserDamage=1;
 auto& player=g_entities[0];const auto playerInUse=player.inuse,playerDamageable=player.takedamage;
 player.inuse=player.takedamage=qtrue;
 auto& aim=g_entities[54];aim.inuse=qtrue;aim.s.number=54;aim.targetname=G_NewString("jar_sb_54");
 VectorSet(aim.currentOrigin,0,500,12);
 static int hitEntity=0,traceCalls=0;
 static bool aimY=false,blockedStart=false;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){
  CHECK(!mins&&!maxs&&pass==53&&mask==MASK_SHOT&&start[2]==12);
  CHECK(std::fabs(end[aimY?1:0]-2048)<.01f&&std::fabs(end[aimY?0:1])<.01f);
  ++traceCalls;*hit={};hit->entityNum=hitEntity;hit->fraction=.5f;
  hit->startsolid=blockedStart?qtrue:qfalse;
  VectorAdd(start,end,hit->endpos);VectorScale(hit->endpos,.5f,hit->endpos);
 };
 vec3_t start,end;
 G_SandboxLaserControlledFrame(&emitter);CHECK(!traceCalls);
 level.time=1600;G_SandboxLaserControlledFrame(&emitter);
 CHECK(traceCalls==1&&laserHits==1&&G_SandboxLaserBeam(emitter,start,end)&&end[0]==1024);
 settings.damage=0;CHECK(G_SandboxLaserSet(emitter,settings));
 emitter.target=aim.targetname;aimY=true;
 level.time=1700;target_laser_think(&emitter);
 CHECK(traceCalls==2&&laserHits==1&&G_SandboxLaserBeam(emitter,start,end)&&end[1]==1024);
 aim.inuse=qfalse;
 level.time=1800;target_laser_think(&emitter);
 CHECK(traceCalls==2&&!G_SandboxLaserBeam(emitter,start,end));
 emitter.target=nullptr;emitter.currentAngles[YAW]=90;
 settings.damage=1;CHECK(G_SandboxLaserSet(emitter,settings));
 hitEntity=ENTITYNUM_WORLD;level.time=1900;target_laser_think(&emitter);
 CHECK(traceCalls==3&&laserHits==1&&G_SandboxLaserBeam(emitter,start,end)&&end[1]==1024);
 blockedStart=true;hitEntity=0;level.time=2000;target_laser_think(&emitter);
 CHECK(traceCalls==4&&laserHits==1&&!G_SandboxLaserBeam(emitter,start,end));
 blockedStart=false;
 G_SandboxLaserUse(emitter);
 CHECK(G_SandboxLaserRead(emitter,settings)&&!settings.firing&&!emitter.nextthink&&emitter.linked);
 G_SandboxLaserUse(emitter);
 CHECK(G_SandboxLaserRead(emitter,settings)&&settings.firing&&emitter.nextthink==2100);
 settings.enabled=false;CHECK(G_SandboxLaserSet(emitter,settings));
 G_SandboxLaserUse(emitter);level.time=2100;target_laser_think(&emitter);
 CHECK(traceCalls==4&&G_SandboxLaserRead(emitter,settings)&&settings.firing&&!settings.enabled&&emitter.linked);
 G_LogicSetUseEnabled(emitter,true);level.time=2200;target_laser_think(&emitter);
 CHECK(traceCalls==5&&laserHits==2&&G_SandboxLaserBeam(emitter,start,end));
 player.inuse=playerInUse;player.takedamage=playerDamageable;
 expectedEmitter=nullptr;gi.trace=priorTrace;level.time=priorTime;
 G_FreeEntity(&emitter);
}
static void CheckStations(){
 using namespace jar::sandboxStation;
 auto& player=g_entities[0];
 static gclient_t client;
 const auto playerInUse=player.inuse;auto* priorClient=player.client;const int priorHealth=player.health;
 player.inuse=qtrue;player.client=&client;player.health=100;
 const int priorTime=level.time;
 auto native=[](gentity_t* e,gentity_t*,gentity_t* user){
  const int grant=std::min(4,e->count);
  if(Class(e->classname)==Ammo)user->client->ps.ammo[AMMO_BLASTER]+=grant;
  else user->client->ps.stats[STAT_ARMOR]+=grant;
  e->count-=grant;e->setTime=level.time+100;e->s.loopSound=grant?7:0;
  e->e_ThinkFunc=grant?thinkF_poll_converter:thinkF_NULL;
  e->nextthink=grant?level.time+500:0;
 };
 for(const char* name:{"misc_ammo_floor_unit","misc_model_ammo_power_converter",
                       "misc_shield_floor_unit","misc_model_shield_power_converter"}){
  level.time=1000;gentity_t station{};station.inuse=qtrue;station.s.number=55;
  station.classname=G_NewString(name);station.targetname=G_NewString("jar_sb_55");
  gentity_t* helpers[jar::entities::helperLimit]{};int count=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&station,helpers,count)&&!count&&station.linked&&station.contents==CONTENTS_SOLID);
  Settings settings;
  CHECK(G_SandboxStationRead(station,settings)&&settings.capacity==75&&settings.charge==75&&!settings.recharge);
  CHECK(G_SandboxStationSet(station,{10,6,100,false,true})&&station.nextthink==1100);
  int& resource=Class(name)==Ammo?client.ps.ammo[AMMO_BLASTER]:client.ps.stats[STAT_ARMOR];
  resource=0;
  sandboxStationUse(&station,nullptr,nullptr,native);
  CHECK(station.count==6&&!resource);
  sandboxStationUse(&station,&player,&player,native);
  CHECK(station.count==2&&resource==4&&station.nextthink==1500&&station.painDebounceTime==1600);
  sandboxStationUse(&station,&player,&player,native);
  CHECK(station.count==2&&resource==4);
  level.time=1500;G_SandboxStationControlledFrame(station);
  CHECK(!station.s.loopSound&&station.nextthink==1600&&station.count==2);
  level.time=1850;G_SandboxStationPoll(station);
  CHECK(station.count==5&&station.nextthink==1900);
  G_LogicSetUseEnabled(station,false);
  CHECK(!station.nextthink&&!station.s.loopSound);
  level.time=2500;G_SandboxStationPoll(station);sandboxStationUse(&station,&player,&player,native);
  CHECK(station.count==5&&resource==4);
  G_LogicSetUseEnabled(station,true);CHECK(station.nextthink==2600);
  CHECK(G_SandboxStationSet(station,{1,0,100,true,true}));
  sandboxStationUse(&station,&player,&player,native);
  CHECK(station.count==0&&resource==8&&!station.painDebounceTime&&station.nextthink==3000&&station.s.frame==0);
  CHECK(G_SandboxStationSet(station,{1,0,0,false,true}));
  if(station.s.eFlags&EF_SHADER_ANIM)CHECK(station.s.frame==1);
  G_FreeEntity(&station);
 }
 player.inuse=playerInUse;player.client=priorClient;player.health=priorHealth;level.time=priorTime;
}
static void CheckUsable(){
 using namespace jar::sandboxUsable;
 const auto oldTrace=gi.trace;
 const auto oldPortal=gi.AdjustAreaPortalState;
 static bool blocked=false;
 static int portals=0;
 gi.AdjustAreaPortalState=[](gentity_t* source,qboolean){CHECK(source->bmodel);++portals;};
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){
  CHECK(VectorCompare(start,end)&&mins[0]==-21&&maxs[0]==21&&pass==59&&mask==(MASK_PLAYERSOLID|CONTENTS_CORPSE));
  *hit={};hit->fraction=1;hit->startsolid=blocked?qtrue:qfalse;
 };
 const int oldTime=level.time,oldOutputs=activations;
 level.time=2000;
 gentity_t source{},actor{},relay{};
 gclient_t client{};
 source.inuse=actor.inuse=relay.inuse=qtrue;
 source.s.number=59;source.classname=G_NewString("func_usable");source.targetname=G_NewString("jar_sb_59");
 source.target=G_NewString("off_output");source.target2=G_NewString("on_output");
 actor.client=&client;actor.health=100;
 gentity_t* helpers[jar::entities::helperLimit]{};int count=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&source,helpers,count)&&!count&&source.contents==CONTENTS_SOLID&&source.s.modelindex==5);
 Settings s;
 CHECK(G_SandboxUsableRead(source,s)&&s.solid&&s.health==40&&s.direct==Both&&s.damage==OnHit);
 func_usable_use(&source,&actor,&actor);
 CHECK(G_SandboxUsableDormant(source)&&source.linked&&!(source.svFlags&SVF_NOCLIENT)&&!source.contents&&activations==oldOutputs+1);
 CHECK(usedTarget==source.target&&outputActivator==&actor&&!portals);
 blocked=true;func_usable_use(&source,&relay,&actor);
 CHECK(G_SandboxUsableRead(source,s)&&s.pending&&!s.solid&&source.activator==&actor&&source.nextthink==level.time+FRAMETIME);
 level.time=source.nextthink;G_SandboxUsableControlledFrame(source);
 CHECK(source.nextthink==level.time+FRAMETIME&&activations==oldOutputs+1);
 G_LogicSetUseEnabled(source,false);CHECK(!source.nextthink&&source.alt_fire);
 G_LogicSetUseEnabled(source,true);blocked=false;
 level.time=source.nextthink;G_SandboxUsableControlledFrame(source);
 CHECK(source.count==1&&!source.alt_fire&&usedTarget==source.target2&&outputActivator==&actor&&activations==oldOutputs+2&&!portals);

 CHECK(G_SandboxUsableRead(source,s));s.damage=AtZero;s.direct=NpcOnly;
 CHECK(G_SandboxUsableSet(source,s));
 func_usable_use(&source,&actor,&actor);CHECK(source.count==1&&activations==oldOutputs+2);
 source.health=20;func_usable_pain(&source,&relay,&actor,vec3_origin,20,MOD_SABER);
 CHECK(source.count==1&&source.health==20);
 source.health=-5;func_usable_die(&source,&relay,&actor,25,MOD_SABER);
 CHECK(G_SandboxUsableDormant(source)&&activations==oldOutputs+3);
 CHECK(G_SandboxUsableRead(source,s));s.mode=Pulse;s.solid=true;s.health=s.capacity;s.reuse=0;
 CHECK(G_SandboxUsableSet(source,s));
 func_usable_use(&source,&relay,&actor);func_usable_use(&source,&relay,&actor);
 CHECK(source.count==1&&source.bounceCount==1&&activations==oldOutputs+4&&usedTarget==source.target);
 CHECK(G_SandboxUsableRead(source,s));s.spent=false;s.reuse=400;
 CHECK(G_SandboxUsableSet(source,s));
 func_usable_use(&source,&relay,&actor);CHECK(source.nextthink==level.time+400);
 func_usable_use(&source,&relay,&actor);CHECK(activations==oldOutputs+5);
 level.time=source.nextthink;G_SandboxUsableControlledFrame(source);
 CHECK(!source.nextthink&&source.takedamage&&source.count==1);

 gentity_t authored{};authored.inuse=qtrue;authored.classname=G_NewString("func_usable");authored.model=G_NewString("*1");
 SP_func_usable(&authored);CHECK(authored.bmodel&&authored.count&&!G_SandboxUsable(authored));
 func_usable_use(&authored,&actor,&actor);CHECK(!authored.count&&portals==1&&(authored.svFlags&SVF_NOCLIENT));
 gi.trace=oldTrace;gi.AdjustAreaPortalState=oldPortal;level.time=oldTime;
 G_FreeEntity(&source);
}
int main(){return RunTests("portable native entity setup transactions",[]{
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=.5f;};
 static int worldHits=0;
 gi.trace=[](trace_t* trace,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
  CHECK(pass==ENTITYNUM_NONE&&mask==MASK_SOLID&&mode==G2_BRUSH_ONLY);
  *trace={};trace->fraction=1;VectorCopy(end,trace->endpos);
  if(worldHits>0){--worldHits;trace->startsolid=qtrue;}
 };
 vec3_t origin={0,0,0},low={-8,-8,-8},high={8,8,8},normal={0,0,1};
 CHECK(G_JAR_WorkshopPlaceHull(origin,low,high,normal,false));
 worldHits=2;CHECK(!G_JAR_WorkshopPlaceHull(origin,low,high,normal,false));
 {
  JarSweepHull shape{};
  for(float z:{-8.f,8.f}){
   for(float x:{-10.f,10.f}){auto& point=shape.points[shape.pointCount++];VectorSet(point,x,0,z);}
   for(float y:{-10.f,10.f}){auto& point=shape.points[shape.pointCount++];VectorSet(point,0,y,z);}
  }
  const vec3_t slope={.6f,.8f,0},boxLow={-10,-10,-8},boxHigh={10,10,8};
  CHECK(std::fabs(G_JAR_WorkshopSurfaceOffset(boxLow,boxHigh,slope,nullptr)-15)<.001f);
  CHECK(std::fabs(G_JAR_WorkshopSurfaceOffset(boxLow,boxHigh,slope,&shape)-9)<.001f);
  static int sceneryTraces=0;
  gi.JAR_TraceSceneryHull=[](trace_t* hit,const vec3_t from,const vec3_t to,const vec3_t lo,const vec3_t hi,
     int pass,int mask,const JarSweepHull& candidate){
   CHECK(pass==ENTITYNUM_NONE&&mask==MASK_SOLID&&candidate.pointCount==8);
   CHECK(lo[0]==-10&&hi[0]==10&&lo[2]==-8&&hi[2]==8);
   *hit={};hit->fraction=1;VectorCopy(to,hit->endpos);
   switch(++sceneryTraces){
   case 1: CHECK(VectorCompare(from,to)&&to[2]==0);hit->startsolid=qtrue;break;
   case 2: CHECK(from[2]==64&&to[2]==0);hit->fraction=.75f;hit->endpos[2]=16;hit->plane.normal[2]=1;break;
   case 3: CHECK(VectorCompare(from,to)&&to[2]==17);break;
   }
  };
  CHECK(G_JAR_WorkshopPlaceHull(origin,low,high,normal,false,&shape)&&sceneryTraces==3&&origin[2]==17);
  gi.JAR_TraceSceneryHull=nullptr;
 }
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=.5f;};
 const float floor=JAR_FallFloor(-1024);
 VectorSet(origin,0,0,-1284);CHECK(!JAR_OutsideMap(origin,8,floor));
 origin[2]=-1290;CHECK(JAR_OutsideMap(origin,8,floor));
 CHECK(!JAR_OutsideMap(origin,8,JAR_FallFloor(-2048)));
 origin[0]=MAX_WORLD_COORD;CHECK(JAR_OutsideMap(origin,8,floor));
 gi.JAR_WorldTrace=[](trace_t*t,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,int model,int mask){
  CHECK(model==0&&mask==MASK_SOLID);gi.trace(t,start,mins,maxs,end,ENTITYNUM_NONE,mask,G2_NOCOLLIDE,0);
 };
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 gi.SetBrushModel=[](gentity_t*e,const char*){e->bmodel=qtrue;};
 globals.num_entities=55;CheckLaserEmitter();CheckStations();CheckUsable();CheckSpeaker();
 for(const char* name:{"func_rotating","func_pendulum"}){
  gentity_t angular{};angular.inuse=qtrue;angular.classname=G_NewString(name);angular.targetname=G_NewString("jar_sb_54");
  angular.model=G_NewString(JAR_LOGIC_PLACEHOLDER_MODEL);
  gentity_t* children[jar::entities::helperLimit]{};int count=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&angular,children,count)&&!count&&angular.linked&&angular.s.eType==ET_MOVER);
  jar::sandboxAngular::Settings settings;
  CHECK(G_SandboxAngularRead(angular,settings)&&jar::sandboxAngular::SameControls(settings,jar::sandboxAngular::Defaults(settings.kind)));
  CHECK(!angular.bmodel&&angular.contents==CONTENTS_SOLID&&angular.s.modelindex==5&&angular.e_UseFunc==useF_func_rotating_use);
 }
 for(const char* type:{"func_breakable","func_glass","misc_model_breakable"}){
  gentity_t body{};body.inuse=qtrue;body.classname=G_NewString(type);
  body.targetname=G_NewString(!strcmp(type,"func_glass")?"jar_sb_56":"jar_sb_57");
  body.model=G_NewString("models/map_objects/imperial/crate1.md3");
  gentity_t* children[jar::entities::helperLimit]{};int count=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&body,children,count)&&!count&&body.linked&&body.s.eType==ET_GENERAL);
  jar::sandboxBreakable::Settings settings;
  CHECK(G_SandboxBreakableRead(body,settings)&&settings.capacity==40&&body.e_DieFunc==dieF_funcBBrushDie&&body.takedamage);
  if(!strcmp(type,"func_glass"))CHECK(settings.material==MAT_GLASS);
  G_FreeEntity(&body);
 }
 {static std::string skyConfig;gi.GetConfigstring=[](int n,char*out,int size){CHECK(n==CS_SKYBOXORG);Q_strncpyz(out,skyConfig.c_str(),size);};gi.SetConfigstring=[](int n,const char*s){CHECK(n==CS_SKYBOXORG);skyConfig=s;};
 auto&r=g_entities[46];r.inuse=qtrue;r.s.number=46;r.classname=G_NewString("misc_skyportal");r.targetname=G_NewString("spawn_sky");gentity_t*h[jar::entities::helperLimit]{};int n=0;dispatched.clear();CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n&&r.inuse&&G_SkyPortalPortable(r)&&r.s.modelindex==5&&r.contents==CONTENTS_ITEM&&r.e_ThinkFunc==thinkF_NULL&&r.e_UseFunc==useF_skyportal_use&&dispatched.empty());jar::skyPortal::Settings settings;CHECK(G_SkyPortalRead(r,settings)&&!settings.enabled);skyportal_use(&r,&r,&r);CHECK(G_SkyPortalOrder(r)==0);G_FreeEntity(&r);CHECK(skyConfig.empty()&&G_SkyPortalOrder(r)==-1);}
 {auto&r=g_entities[47];globals.num_entities=64;r.inuse=qtrue;r.s.number=47;r.classname=G_NewString("point_combat");gentity_t*h[jar::entities::helperLimit]{};int n=0;CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n&&r.inuse&&G_CombatPointPortable(r)&&G_CombatPointAvailable(r.count)&&r.mins[2]==-24&&r.maxs[2]==40);marker_use(&r,&r,&r);CHECK(!G_CombatPointAvailable(r.count));marker_use(&r,&r,&r);CHECK(G_CombatPointAvailable(r.count));const int slot=r.count;G_FreeEntity(&r);CHECK(!G_CombatPointAvailable(slot));}
 for(const char*type:{"waypoint","waypoint_small"}){auto&r=g_entities[49];r.inuse=qtrue;r.s.number=49;r.classname=G_NewString(type);r.targetname=G_NewString("live_waypoint");gentity_t*h[jar::entities::helperLimit]{};int n=0;dispatched.clear();CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n&&r.inuse&&G_WaypointPortable(r)&&dispatched.empty()&&r.e_ThinkFunc==thinkF_NULL&&r.e_UseFunc==useF_marker_use&&r.contents==CONTENTS_ITEM&&r.s.modelindex==5&&r.mins[2]==-24&&r.maxs[2]==16&&r.wait==512);marker_use(&r,&r,&r);CHECK(r.svFlags&SVF_INACTIVE);marker_use(&r,&r,&r);CHECK(!(r.svFlags&SVF_INACTIVE));G_FreeEntity(&r);CHECK(!G_WaypointPortable(r));}
 for(const char*type:{"ref_tag","waypoint_navgoal"}){auto&r=g_entities[48];r.inuse=qtrue;r.s.number=48;r.classname=G_NewString(type);r.targetname=G_NewString("jar_ref_spawn");gentity_t*h[jar::entities::helperLimit]{};int n=0;CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n&&r.inuse&&G_ReferencePortable(r)&&r.e_UseFunc==useF_marker_use&&r.contents==CONTENTS_ITEM&&r.s.modelindex==5);CHECK(G_ReferenceFind(nullptr,"jar_ref_spawn"));marker_use(&r,&r,&r);CHECK(!G_ReferenceFind(nullptr,"jar_ref_spawn"));marker_use(&r,&r,&r);CHECK(G_ReferenceFind(nullptr,"jar_ref_spawn"));G_FreeEntity(&r);CHECK(!G_ReferenceFind(nullptr,"jar_ref_spawn"));}
 for(const char*type:{"misc_camera_focus","misc_camera_track"}){
 auto&r=g_entities[45];r.inuse=qtrue;r.s.number=45;r.classname=G_NewString(type);r.targetname=G_NewString("scene_root");r.target=r.target2=nullptr;capacity=10;gentity_t*h[jar::entities::helperLimit]{};int n=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&G_SceneCameraPortable(r)&&r.e_UseFunc==useF_scene_camera_use&&r.e_ThinkFunc==thinkF_scene_camera_think&&r.speed==64&&r.radius==180);
 CHECK(n==jar::entities::HelperCount(*jar::entities::Find(type)));
 if(jar::sceneCamera::Focus(type))CHECK(n==1&&G_SceneCameraPortable(*h[0])&&!strcmp(r.target2,h[0]->targetname)&&!strcmp(h[0]->target2,r.targetname)&&h[0]->currentOrigin[0]==-128);
 else CHECK(n==2&&!strcmp(r.target,h[0]->targetname)&&!strcmp(h[0]->target,h[1]->targetname)&&!h[1]->target&&h[1]->currentOrigin[0]==128);
 for(int i=0;i<n;++i)G_FreeEntity(h[i]);G_FreeEntity(&r);
 r.inuse=qtrue;capacity=0;CHECK(!G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n);G_FreeEntity(&r);
 }capacity=10;

 for(const char*type:{"rail_track","rail_lane","rail_mover"}){
  auto&r=g_entities[44];r.inuse=qtrue;r.s.number=44;r.classname=G_NewString(type);r.targetname=G_NewString("rail_root");r.target=nullptr;VectorSet(r.currentAngles,20,45,10);capacity=10;gentity_t*h[jar::entities::helperLimit]{};int n=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&r,h,n)&&G_RailPortable(r)&&r.e_UseFunc==useF_portable_rail_use&&n==2&&jar::entities::HelperCount(*jar::entities::Find(type))==2);
  gentity_t*parts[4]{};parts[jar::rail::Class(type)]=&r;for(int i=0;i<n;++i){CHECK(G_RailPortable(*h[i]));parts[jar::rail::Class(h[i]->classname)]=h[i];}
  CHECK(!strcmp(parts[2]->target,parts[1]->targetname)&&!strcmp(parts[3]->target,parts[2]->targetname)&&parts[3]->s.eType==ET_MOVER&&parts[3]->contents==CONTENTS_SOLID);
  vec3_t axis[3],delta;AnglesToAxis(r.currentAngles,axis);VectorSubtract(parts[3]->currentOrigin,parts[1]->currentOrigin,delta);CHECK(std::fabs(DotProduct(delta,axis[0])-64)<.001f&&std::fabs(DotProduct(delta,axis[1])-64)<.001f&&std::fabs(DotProduct(delta,axis[2])-32)<.001f);
  for(int i=0;i<n;++i)G_FreeEntity(h[i]);G_FreeEntity(&r);
  for(int available:{0,1}){r.inuse=qtrue;capacity=available;CHECK(!G_JAR_SpawnWorkshopEntity(&r,h,n)&&!n);G_FreeEntity(&r);for(auto*e:entities)CHECK(!e->inuse);}
 }capacity=10;
 auto&crate=g_entities[49];crate.inuse=qtrue;crate.s.number=49;crate.classname=G_NewString("misc_crystal_crate");gentity_t*crateHelpers[jar::entities::helperLimit]{};int crateCount=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&crate,crateHelpers,crateCount));CHECK(crateCount==1&&G_CrystalCratePart(crate)&&crate.health==80&&crate.splashDamage==40&&crate.mass==40&&crate.maxs[2]==38&&crate.e_DieFunc==dieF_misc_model_breakable_die&&crate.e_UseFunc==useF_misc_use);auto*crystal=crate.lastEnemy;G_FreeEntity(&crate);CHECK(!crystal->inuse);
 capacity=0;crate.inuse=qtrue;CHECK(!G_JAR_SpawnWorkshopEntity(&crate,crateHelpers,crateCount)&&!crateCount);capacity=10;
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->allsolid=qtrue;};CHECK(!G_JAR_SpawnWorkshopEntity(&crate,crateHelpers,crateCount)&&!crateCount&&!crate.lastEnemy);G_FreeEntity(&crate);
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=.5f;};capacity=10;
 auto&parent=g_entities[50];parent.inuse=qtrue;parent.s.number=50;parent.classname=G_NewString("trigger_push");
 gentity_t*helpers[jar::entities::helperLimit]{};int count=0;
 AddSpawnField(const_cast<char*>("noise"),const_cast<char*>("mission_sound"));char*saved=spawnVars[0][1];
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==1);CHECK(dispatched=="trigger_push");
 jar::push::Settings pad;CHECK(G_PushRead(parent,pad)&&pad.aim==jar::push::Linked&&pad.speed==1000);
 CHECK(!strcmp(parent.target,helpers[0]->targetname));CHECK(parent.target!=helpers[0]->targetname);
 CHECK(helpers[0]->currentOrigin[0]==128 && helpers[0]->currentOrigin[2]==96);
 CHECK(numSpawnVars==1 && spawnVars[0][1]==saved && !spawning && NPCsPrecached);
 G_JAR_SetBrushModel(&parent,"@jar_box");CHECK(!parent.bmodel && parent.contents==CONTENTS_SOLID && parent.mins[0]==-21 && parent.maxs[2]==22);
 G_JAR_SetBrushModel(&parent,"*1");CHECK(parent.bmodel);
 parent.classname=G_NewString("func_train");VectorSet(parent.currentOrigin,512,64,96);CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==2);
 CHECK(parent.pos1[0]==512 && parent.pos2[0]==512 && parent.pos1[2]==96);
 CHECK(!strcmp(helpers[0]->target,helpers[1]->targetname) && !strcmp(helpers[1]->target,helpers[0]->targetname));
 CHECK(helpers[0]->target!=helpers[1]->targetname && helpers[1]->target!=helpers[0]->targetname);
 CHECK(!strcmp(parent.target,helpers[0]->targetname) && parent.target!=helpers[0]->targetname);
 capacity=1;const size_t before=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==0 && !entities[before]->inuse);
 capacity=10;reject=true;parent.classname=G_NewString("trigger_multiple");CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==0 && !entities.back()->inuse);
 reject=false;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};
 CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==0 && !entities.back()->inuse);
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=.5f;};
 reject=false;parent.classname=G_NewString("shooter_grenade");CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(dispatched=="misc_weapon_shooter");
 CHECK(!strcmp(parent.classname,"shooter_grenade") && !strcmp(parent.paintarget,"WP_THERMAL") && parent.wait==500);
 // Required native exhaustion unwinds both the temporary class alias and the
 // parser/helper context before the outer allocation transaction frees actors.
 char*originalClass=parent.classname;exhaustNative=true;bool caught=false;
 try{G_JAR_SpawnWorkshopEntity(&parent,helpers,count);}catch(const G_SpawnCapacity&){caught=true;}
 CHECK(caught&&parent.classname==originalClass&&numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 exhaustNative=false;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));
 parent.classname=G_NewString("misc_trip_mine");gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};
 CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(numSpawnVars==1 && spawnVars[0][1]==saved);
 // Actual native rack callbacks fill and capture every pickup, including the
 // normally delayed ammo shelf. Native quantity/difficulty rules are retained.
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};
 capacity=20;parent.classname=G_NewString("misc_model_gun_rack");VectorSet(parent.s.origin,100,200,300);VectorClear(parent.s.angles);
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==3);
 CHECK(helpers[0]->item->giTag==WP_BLASTER&&helpers[0]->count==15);CHECK(helpers[1]->item->giTag==WP_REPEATER&&helpers[1]->count==100);CHECK(helpers[2]->item->giTag==WP_ROCKET_LAUNCHER&&helpers[2]->count==4);
 CHECK(helpers[0]->currentOrigin[2]==323&&helpers[2]->currentOrigin[2]==325.5f);
 parent.classname=G_NewString("misc_model_ammo_rack");skill.integer=2;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==5&&parent.nextthink==0&&parent.e_ThinkFunc==thinkF_NULL);
 CHECK(helpers[0]->item->giTag==AMMO_BLASTER&&helpers[0]->count==30);CHECK(helpers[1]->item->giTag==AMMO_METAL_BOLTS&&helpers[1]->count==10);CHECK(helpers[2]->item->giTag==AMMO_POWERCELL);
 CHECK(helpers[3]->item->giType==IT_WEAPON&&helpers[4]->item->giType==IT_HEALTH);
 CHECK(jar::entities::HelperCount(*jar::entities::Find(parent.classname))==unsigned(count));
 capacity=2;auto begin=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(!count);
 for(size_t i=begin;i<entities.size();++i)CHECK(!entities[i]->inuse);
 capacity=20;begin=entities.size();gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};
 CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(!count);for(size_t i=begin;i<entities.size();++i)CHECK(!entities[i]->inuse);
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};capacity=20;
 parent.classname=G_NewString("misc_camera");CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==1&&parent.e_UseFunc==useF_camera_use);
 CHECK(!strcmp(helpers[0]->classname,"misc_model_static")&&helpers[0]->model&&helpers[0]->currentOrigin[2]==parent.currentOrigin[2]+16);
 gclient_t client{};player->client=&client;parent.target=nullptr;camera_use(&parent,player,player);CHECK(client.ps.viewEntity==parent.s.number);
 client.usercmd.forwardmove=1;camera_aim(&parent);CHECK(client.ps.viewEntity==0);client.usercmd.forwardmove=0;
 camera_use(&parent,player,player);CHECK(client.ps.viewEntity==parent.s.number);
 camera_die(&parent,player,player,10,0,0,0);CHECK(!client.ps.viewEntity&&!parent.takedamage&&!parent.s.modelindex);
 CHECK(tracked&&tracked->inuse&&!strcmp(tracked->classname,"fx_runner")&&!strcmp(tracked->fxFile,"sparks/spark"));
 trackAllowed=false;camera_die(&parent,player,player,10,0,0,0);CHECK(!tracked->inuse);
 capacity=0;parent.classname=G_NewString("misc_camera");CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(!count);
 // Outside a workshop scope, ordinary map rack allocation still works.
 capacity=10;SP_misc_model_gun_rack(&parent);CHECK(G_JAR_SpawnSetupChild(&parent));
 {
  capacity=10;parent.classname=G_NewString("trigger_entdist");parent.svFlags=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==2&&parent.count==192&&parent.spawnflags==1&&G_DistancePortable(parent));
  CHECK(jar::entities::HelperCount(*jar::entities::Find(parent.classname))==2);
  CHECK(!strcmp(helpers[0]->message,"Distance trigger: inside range")&&!strcmp(helpers[1]->message,"Distance trigger: outside range"));
  gclient_t playerClient{},npcClient{};gNPC_t npcData{};
  player->inuse=qtrue;player->client=&playerClient;player->health=100;
  VectorCopy(parent.currentOrigin,player->currentOrigin);activations=0;
  trigger_entdist_use(&parent,player,player);CHECK(activations==1&&usedTarget==parent.target);
  trigger_entdist_use(&parent,player,player);CHECK(activations==1);parent.svFlags&=~SVF_INACTIVE;
  player->currentOrigin[0]+=192;trigger_entdist_use(&parent,player,player);CHECK(activations==2&&usedTarget==parent.target2);
  auto* reference=G_TrySpawn();auto* npc=G_TrySpawn();CHECK(reference&&npc);
  reference->targetname=G_NewString("jar_sb_900");npc->targetname=G_NewString("jar_sb_901");npc->client=&npcClient;npc->NPC=&npcData;npc->health=100;
  VectorSet(reference->currentOrigin,1000,0,0);VectorSet(npc->currentOrigin,1050,0,0);
  G_DistanceSetLink(parent,JAR_DISTANCE_REFERENCE,reference->targetname);
  G_DistanceSetLink(parent,JAR_DISTANCE_SUBJECT,npc->targetname);
  parent.count=51;parent.spawnflags=JAR_DISTANCE_NPC;parent.svFlags&=~SVF_INACTIVE;
  trigger_entdist_use(&parent,player,player);CHECK(activations==3&&usedTarget==parent.target);
  parent.count=50;parent.svFlags&=~SVF_INACTIVE;trigger_entdist_use(&parent,player,player);CHECK(activations==4&&usedTarget==parent.target2);
  G_FreeEntity(reference);parent.svFlags&=~SVF_INACTIVE;
  trigger_entdist_use(&parent,player,player);CHECK(activations==4&&!(parent.svFlags&SVF_INACTIVE));
  G_DistanceSetLink(parent,JAR_DISTANCE_REFERENCE,JAR_DISTANCE_MISSING_REFERENCE_NAME);
  parent.spawnflags=JAR_DISTANCE_PLAYER;
  trigger_entdist_use(&parent,player,player);CHECK(activations==4&&!(parent.svFlags&SVF_INACTIVE));
  parent.spawnflags=JAR_DISTANCE_NPC;
  MemorySave save;ojk::SavedGameHelper serializer(&save);gentity_t loaded{};
  parent.sg_export(serializer);save.reset_buffer_offset();loaded.sg_import(serializer);serializer.ensure_all_data_read();
  CHECK(loaded.count==50&&loaded.spawnflags==JAR_DISTANCE_NPC&&loaded.e_UseFunc==useF_trigger_entdist_use&&!(loaded.svFlags&SVF_INACTIVE));
  G_FreeEntity(npc);G_FreeEntity(helpers[0]);G_FreeEntity(helpers[1]);player->client=nullptr;player->inuse=qfalse;
  gentity_t authored{};authored.classname=G_NewString("trigger_entdist");authored.target=G_NewString("authored_inside");authored.target2=G_NewString("authored_outside");
  SP_trigger_entdist(&authored);authored.count=192;authored.spawnflags=1;VectorCopy(player->currentOrigin,authored.currentOrigin);
  trigger_entdist_use(&authored,player,player);CHECK(activations==5&&usedTarget=="authored_inside");
 }
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 // Portable locks own a separate usable door and skip delayed map tracing.
 capacity=10;parent.classname=G_NewString("misc_maglock");parent.model=G_NewString("models/map_objects/kejim/sec_panel.md3");parent.svFlags=0;parent.target=nullptr;
 VectorSet(parent.currentOrigin,100,200,300);VectorClear(parent.currentAngles);
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count));CHECK(count==1&&parent.activator==helpers[0]);
 auto*door=helpers[0];CHECK(!strcmp(door->classname,"func_door")&&(door->spawnflags&MOVER_PLAYER_USE)&&door->e_UseFunc==useF_Use_BinaryMover);
 CHECK(door->currentOrigin[0]==79&&door->currentOrigin[2]==288&&door->lockCount==1&&(door->svFlags&SVF_INACTIVE));
 CHECK(parent.health==10&&parent.e_DieFunc==dieF_maglock_die&&parent.e_ThinkFunc==thinkF_NULL&&!parent.nextthink);
 CHECK(jar::entities::HelperCount(*jar::entities::Find(parent.classname))==1);G_FreeEntity(door);CHECK(!parent.activator);
 capacity=0;CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!parent.activator);
 // Fail the final helper overlap check after native linking, not just allocation.
 capacity=10;static int traces=0;traces=0;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;if(++traces==2)t->startsolid=qtrue;};
 CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!parent.activator&&!entities.back()->inuse);
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 parent.classname=G_NewString("object_cargo_barrel1");parent.model=G_NewString("models/map_objects/imperial/barrels.md3");VectorSet(parent.mins,-39,-39,-1);VectorSet(parent.maxs,39,39,48);
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&parent.health==40&&parent.mins[2]==-1&&parent.maxs[2]==48);
 CHECK(parent.e_DieFunc==dieF_ExplodeDeath_Wait&&parent.splashDamage==100&&parent.splashRadius==200);
 parent.classname=G_NewString("trigger_space");CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count);
 CHECK(parent.e_TouchFunc==touchF_space_touch&&parent.e_UseFunc==useF_space_use&&parent.contents==CONTENTS_TRIGGER&&!parent.bmodel);
 CHECK(parent.mins[0]==-64&&parent.maxs[0]==64&&parent.mins[2]==0&&parent.maxs[2]==128);
 CHECK(!(parent.svFlags&SVF_NOCLIENT)&&(parent.svFlags&SVF_PLAYER_USABLE));space_use(&parent,&parent,&parent);CHECK(parent.contents==0&&parent.linked);
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 // Complete portable directional setup supplies distinct owned output markers.
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};capacity=10;
 parent.classname=G_NewString("trigger_bidirectional");parent.count=0;parent.spawnflags=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==2&&parent.e_TouchFunc==touchF_bidirectional_touch);
 CHECK(jar::entities::HelperCount(*jar::entities::Find(parent.classname))==2&&parent.wait==1&&parent.pos2[2]==128);
 CHECK(!strcmp(parent.target,helpers[0]->targetname)&&!strcmp(parent.paintarget,helpers[1]->targetname)&&strcmp(parent.target,parent.paintarget));
 CHECK(!strcmp(helpers[0]->message,"Forward crossing")&&!strcmp(helpers[1]->message,"Reverse crossing"));
 capacity=1;const auto directionalStart=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count);
 for(size_t i=directionalStart;i<entities.size();++i)CHECK(!entities[i]->inuse);
 // Persistent interest bypasses the authored fixed table and owns one feedback
 // marker. Locations register immediately even after the initial map linkup.
 capacity=10;parent.classname=G_NewString("target_interest");parent.svFlags=0;parent.s.modelindex=0;parent.contents=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==1&&parent.inuse&&G_PortableInterest(parent)&&!level.numInterestPoints);
 CHECK(parent.e_UseFunc==useF_marker_use&&(parent.svFlags&SVF_PLAYER_USABLE)&&parent.s.modelindex&&parent.contents==CONTENTS_ITEM);
 CHECK(!strcmp(parent.target,helpers[0]->targetname)&&!strcmp(helpers[0]->classname,"target_print"));G_FreeEntity(&parent);CHECK(!G_TargetMarkersPresent());
 parent.inuse=qtrue;parent.classname=G_NewString("target_location");VectorSet(parent.s.origin,300,400,500);
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&level.locationHead==&parent&&G_TargetMarkersPresent());
 CHECK(!strcmp(parent.message,"Sandbox location")&&parent.currentOrigin[1]==400&&parent.e_UseFunc==useF_marker_use);G_FreeEntity(&parent);CHECK(!level.locationHead);
 {
  gentity_t& shot=g_entities[60];shot.inuse=qtrue;shot.s.number=60;shot.classname=G_NewString("trigger_always");
  shot.targetname=G_NewString("owned_one_shot");capacity=10;
  CHECK(G_JAR_SpawnWorkshopEntity(&shot,helpers,count)&&count==1&&G_AlwaysPortable(shot));
  CHECK(jar::entities::LogicMarker(shot.classname,"jar_sb_42",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(jar::entities::LogicMarker(helpers[0]->classname,"jar_sb_43",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker(shot.classname,"authored_one_shot",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker(shot.classname,"jar_sb_authored",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker("misc_crystal_crate","jar_sb_43",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker("func_door","jar_sb_43",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker("jar_sandbox_prop","jar_sb_43",JAR_LOGIC_PLACEHOLDER_MODEL));
  CHECK(!jar::entities::LogicMarker(shot.classname,"jar_sb_42","models/items/power_converter.md3"));
  const vec3_t anchor={100,200,300},forward={1,0,0};vec3_t view={0,0,0};refEntity_t sprite;
  JAR_LogicMarkerSprite(&sprite,anchor,view,forward,123);
  CHECK(sprite.reType==RT_SPRITE&&sprite.customShader==123&&sprite.radius==11&&sprite.origin[2]==318&&sprite.renderfx==0);
  CHECK(sprite.shaderRGBA[0]==153&&sprite.shaderRGBA[1]==229&&sprite.shaderRGBA[2]==255&&sprite.shaderRGBA[3]==255);
  view[0]=-692;
  JAR_LogicMarkerSprite(&sprite,anchor,view,forward,123);
  CHECK(sprite.radius==24&&sprite.origin[2]==318);
  view[0]=-1500;
  JAR_LogicMarkerSprite(&sprite,anchor,view,forward,123);
  CHECK(sprite.radius==36&&sprite.origin[2]==318);
  CHECK(shot.e_UseFunc==useF_portable_always_use&&G_AlwaysRemaining(shot)==300&&(shot.svFlags&SVF_PLAYER_USABLE));
  const int first=activations;
  trigger_always_think(&shot);CHECK(shot.inuse&&!shot.nextthink&&activations==first+1&&usedTarget==helpers[0]->targetname);
  shot.delay=800;portable_always_use(&shot,nullptr,nullptr);const int deadline=shot.nextthink;
  level.time+=25;portable_always_use(&shot,nullptr,nullptr);CHECK(shot.nextthink==deadline&&G_AlwaysRemaining(shot)==775);
  MemorySave save;ojk::SavedGameHelper serializer(&save);gentity_t loaded{};
  shot.sg_export(serializer);save.reset_buffer_offset();loaded.sg_import(serializer);serializer.ensure_all_data_read();
  CHECK(loaded.e_UseFunc==useF_portable_always_use&&loaded.e_ThinkFunc==thinkF_trigger_always_think&&loaded.delay==800&&loaded.nextthink==deadline);
  shot.svFlags|=SVF_INACTIVE;trigger_always_think(&shot);CHECK(shot.inuse&&!shot.nextthink&&activations==first+1);
  G_FreeEntity(helpers[0]);G_FreeEntity(&shot);
  shot.inuse=qtrue;shot.model=nullptr;shot.target=G_NewString("authored_output");shot.e_UseFunc=useF_NULL;
  SP_trigger_always(&shot);CHECK(shot.nextthink==level.time+300&&shot.e_UseFunc==useF_NULL);
  trigger_always_think(&shot);CHECK(!shot.inuse&&activations==first+2&&usedTarget=="authored_output");
 }
 {
  auto& marker=g_entities[62];marker.inuse=qtrue;marker.s.number=62;
  marker.classname=G_NewString("trigger_visible");marker.targetname=G_NewString("jar_sb_62");
  VectorSet(marker.s.origin,100,0,50);capacity=10;
  CHECK(G_JAR_SpawnWorkshopEntity(&marker,helpers,count)&&count==1&&G_VisiblePortable(marker));
  CHECK(marker.contents==CONTENTS_ITEM&&marker.e_UseFunc==useF_portable_visible_use);
  CHECK(marker.nextthink==level.time+JAR_VISIBLE_SAMPLE_MS&&marker.radius==512&&marker.random==45);
  const auto originalPVS=gi.inPVS;
  gi.inPVS=[](const vec3_t,const vec3_t){return sightPVS?qtrue:qfalse;};
  gclient_t view{};player->inuse=qtrue;player->health=100;player->client=&view;
  VectorSet(view.renderInfo.eyePoint,0,0,50);view.renderInfo.eyeAngles[YAW]=180;
  const int first=activations;
  sightPVS=true;G_VisibleCheck(&marker);CHECK(activations==first);
  view.renderInfo.eyeAngles[YAW]=0;sightPVS=false;G_VisibleCheck(&marker);CHECK(activations==first);
  sightPVS=true;sightBlocked=true;G_VisibleCheck(&marker);CHECK(activations==first);
  G_VisibleSet(marker,{100,30,JAR_VISIBLE_IGNORE_WALLS|JAR_VISIBLE_FORCE_SIGHT});
  G_VisibleCheck(&marker);CHECK(activations==first);
  view.ps.forcePowersActive=1<<FP_SEE;
  G_VisibleCheck(&marker);
  CHECK(marker.inuse&&!marker.nextthink&&marker.e_ThinkFunc==thinkF_NULL&&activations==first+1&&usedTarget==helpers[0]->targetname);
  marker.svFlags|=SVF_INACTIVE;portable_visible_use(&marker,nullptr,nullptr);CHECK(!marker.nextthink);
  marker.svFlags&=~SVF_INACTIVE;portable_visible_use(&marker,nullptr,nullptr);
  CHECK(marker.nextthink==level.time+JAR_VISIBLE_SAMPLE_MS);
  MemorySave save;ojk::SavedGameHelper serializer(&save);gentity_t loaded{};
  marker.sg_export(serializer);save.reset_buffer_offset();loaded.sg_import(serializer);serializer.ensure_all_data_read();
  CHECK(loaded.e_UseFunc==useF_portable_visible_use&&loaded.e_ThinkFunc==thinkF_trigger_visible_check_player_visibility&&
        loaded.radius==100&&loaded.random==30&&loaded.spawnflags==3&&loaded.nextthink==marker.nextthink);
  player->client=nullptr;player->inuse=qfalse;gi.inPVS=originalPVS;
  G_FreeEntity(helpers[0]);G_FreeEntity(&marker);
 }
 for(const char* classname:{"info_player_start","info_player_deathmatch"}){
  gentity_t& marker=g_entities[61];marker.inuse=qtrue;marker.s.number=61;marker.classname=G_NewString(classname);
  marker.targetname=G_NewString("jar_sb_61");marker.svFlags=0;marker.contents=0;marker.s.modelindex=0;
  VectorSet(marker.currentOrigin,170,200,24);VectorSet(marker.currentAngles,0,90,0);
  CHECK(G_JAR_SpawnWorkshopEntity(&marker,helpers,count)&&!count&&G_JAR_PortableStart(marker));
  CHECK(dispatched=="info_notnull"&&marker.e_UseFunc==useF_portable_start_use&&marker.contents==CONTENTS_ITEM);
  gentity_t actor{};gclient_t player{};actor.inuse=qtrue;actor.health=100;actor.client=&player;
  VectorSet(actor.mins,-15,-15,-24);VectorSet(actor.maxs,15,15,32);
  const int visits=startVisits;const auto originalTrace=gi.trace;const auto originalPrint=gi.Printf;
  gi.Printf=[](const char*,...){};
  gi.trace=[](trace_t*tr,const vec3_t point,const vec3_t mins,const vec3_t maxs,const vec3_t end,int,int,EG2_Collision,int){
   *tr={};tr->allsolid=qtrue;CHECK(VectorCompare(point,end)&&point[2]==49&&mins[2]==-24&&maxs[2]==32);
  };
  portable_start_use(&marker,&actor,&actor);CHECK(startVisits==visits);
  gi.trace=originalTrace;actor.s.m_iVehicleNum=70;portable_start_use(&marker,&actor,&actor);CHECK(startVisits==visits);
  actor.s.m_iVehicleNum=0;portable_start_use(&marker,&actor,&actor);CHECK(startVisits==visits+1&&startArrival[0]==170&&startArrival[2]==49&&startView[YAW]==90);
  marker.svFlags|=SVF_INACTIVE;portable_start_use(&marker,&actor,&actor);CHECK(startVisits==visits+1);
  gi.Printf=originalPrint;
  G_FreeEntity(&marker);marker.inuse=qtrue;marker.model=nullptr;CHECK(!G_JAR_VisitStart(&marker,&actor));G_FreeEntity(&marker);
 }
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 parent.inuse=qtrue;parent.classname=G_NewString("misc_cubemap");parent.spawnflags=0;capacity=10;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&G_ReflectionPortable(parent)&&parent.s.eType==ET_GENERAL&&parent.e_UseFunc==useF_reflection_probe_use&&parent.radius==512&&parent.count==1&&parent.maxs[2]==22&&(parent.svFlags&SVF_BROADCAST)&&parent.s.modelindex&&parent.contents==CONTENTS_ITEM);
 parent.inuse=qtrue;parent.classname=G_NewString("fx_cloudlayer");parent.spawnflags=0;capacity=10;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&G_CloudPortable(parent)&&parent.s.eType==ET_CLOUD&&parent.e_UseFunc==useF_cloudlayer_use&&parent.radius==512&&parent.random==128&&parent.wait==0&&parent.mins[0]==-21&&parent.maxs[2]==22&&(parent.svFlags&SVF_BROADCAST)&&parent.s.modelindex&&parent.contents==CONTENTS_ITEM);
 for(const char*name:{"fx_rain","fx_snow","fx_puff","fx_wind"}){parent.inuse=qtrue;parent.classname=G_NewString(name);parent.spawnflags=0;capacity=10;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&G_WeatherPortable(parent)&&parent.e_UseFunc==useF_weather_use&&parent.random==1&&parent.radius==1&&parent.wait==.5f&&parent.delay>0&&parent.s.modelindex&&parent.contents==CONTENTS_ITEM);jar::weather::Settings settings;CHECK(G_WeatherRead(parent,settings)&&settings.kind==jar::weather::Class(name));}
 for(const char*name:{"fx_wind_zone","misc_weather_zone"}){parent.inuse=qtrue;parent.classname=G_NewString(name);parent.spawnflags=0;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&G_WeatherPortable(parent)&&parent.e_UseFunc==useF_weather_use&&parent.e_TouchFunc==touchF_NULL&&parent.pos1[0]==-256&&parent.pos2[2]==256&&parent.contents==CONTENTS_TRIGGER);}
 Q_strncpyz(level.mapname,"hoth2",sizeof(level.mapname));gi.FS_ReadFile=[](const char*path,void**out)->long{CHECK(!out);return !strcmp(path,"maps/hoth2.bsp")?100:-1;};
 parent.inuse=qtrue;parent.classname=G_NewString("target_level_change");parent.target=nullptr;parent.s.modelindex=0;parent.contents=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&G_LevelChangePortable(parent)&&!strcmp(parent.message,"hoth2")&&parent.count==-1&&parent.spawnflags==0&&parent.e_UseFunc==useF_target_level_change_use&&parent.s.modelindex&&(parent.svFlags&SVF_PLAYER_USABLE));
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);G_FreeEntity(&parent);
 parent.inuse=qtrue;parent.classname=G_NewString("fx_target_beam");capacity=10;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==1&&parent.e_UseFunc==useF_fx_target_beam_use);
 CHECK((parent.svFlags&SVF_PLAYER_USABLE)&&!strcmp(helpers[0]->classname,"target_position")&&
       !strcmp(parent.target,helpers[0]->targetname));
 G_FreeEntity(helpers[0]);G_FreeEntity(&parent);
 parent.inuse=qtrue;parent.classname=G_NewString("fx_explosion_trail");capacity=10;
 VectorSet(parent.currentAngles,0,35,0);
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==1&&parent.e_UseFunc==useF_fx_explosion_trail_use);
 CHECK((parent.svFlags&SVF_PLAYER_USABLE)&&!parent.nextthink&&parent.currentAngles[YAW]==35&&
       !strcmp(parent.target,helpers[0]->targetname));
 G_FreeEntity(helpers[0]);G_FreeEntity(&parent);
 VectorClear(parent.currentAngles);VectorClear(parent.s.angles);
 // Music is a real native callback with a portable identity and default path.
 parent.inuse=qtrue;parent.classname=G_NewString("trigger_shipboundary");capacity=10;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==1&&parent.count==3000&&parent.spawnflags==1&&parent.e_TouchFunc==touchF_shipboundary_touch&&parent.e_UseFunc==useF_shipboundary_use&&parent.maxs[0]==512&&parent.maxs[2]==512&&!strcmp(parent.target,helpers[0]->targetname));CHECK(!strcmp(helpers[0]->classname,"target_position"));
 parent.inuse=qtrue;parent.classname=G_NewString("target_scriptrunner");CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&parent.count==-1&&parent.wait==1000&&parent.delay==0&&parent.spawnflags==1&&parent.e_UseFunc==useF_target_scriptrunner_use&&!strcmp(parent.behaviorSet[BSET_USE],jar::scriptRunner::defaultScript));
 parent.inuse=qtrue;parent.classname=G_NewString("target_change_parm");parent.parms=nullptr;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&parent.parms&&!strcmp(parent.parms->parm[0],"Sandbox")&&parent.e_UseFunc==useF_target_change_parm_use);
 CHECK(parent.s.modelindex&&(parent.svFlags&SVF_PLAYER_USABLE));player->inuse=qtrue;player->s.number=0;target_change_parm_use(&parent,nullptr,player);CHECK(player->parms&&!strcmp(player->parms->parm[0],"Sandbox"));
 parent.inuse=qtrue;parent.classname=G_NewString("target_play_music");parent.targetname=G_NewString("jar_music_test");parent.contents=0;parent.s.modelindex=0;parent.svFlags=0;
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&G_PortableMusic(parent));
 CHECK(!strcmp(parent.message,jar::music::defaultTrack)&&parent.e_UseFunc==useF_target_play_music_use&&(parent.svFlags&SVF_PLAYER_USABLE)&&parent.s.modelindex&&parent.contents==CONTENTS_ITEM);
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 static std::string playing="map_music";gi.GetConfigstring=[](int,char*out,int size){Q_strncpyz(out,playing.c_str(),size);};gi.SetConfigstring=[](int,const char*text){playing=text;};
 target_play_music_use(&parent,nullptr,nullptr);CHECK(playing==jar::music::defaultTrack);G_FreeEntity(&parent);CHECK(playing=="map_music");

 parent.inuse=qtrue;parent.classname=G_NewString("misc_laser_arm");parent.lastEnemy=nullptr;capacity=10;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==2&&G_LaserArmPortable(parent)&&parent.lastEnemy==helpers[0]&&helpers[0]->lastEnemy==helpers[1]&&helpers[0]->owner==&parent&&helpers[1]->owner==&parent&&parent.damage==5&&parent.wait==3&&parent.radius==4096);CHECK(G_LaserArmRoot(helpers[1])==&parent);G_FreeEntity(helpers[1]);CHECK(!parent.lastEnemy->lastEnemy&&!parent.alt_fire);G_FreeEntity(helpers[0]);CHECK(!parent.lastEnemy);
 capacity=1;const auto laserBefore=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&!entities[laserBefore]->inuse);
 capacity=10;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};const auto laserBlocked=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&!entities[laserBlocked]->inuse&&!entities[laserBlocked+1]->inuse&&!parent.lastEnemy);

 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};
 for(const char*name:{"misc_portal_surface","misc_portal_camera"}){capacity=10;parent.inuse=qtrue;parent.classname=G_NewString(name);parent.target=nullptr;parent.owner=nullptr;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==1&&G_PortalPortable(parent)&&G_PortalPortable(*helpers[0]));auto*s=G_PortalSurface(parent)?&parent:helpers[0];auto*c=s==&parent?helpers[0]:&parent;CHECK(s->owner==c&&!strcmp(s->target,c->targetname)&&s->e_UseFunc==useF_portable_portal_use&&s->s.eType==ET_PORTAL&&s->s.modelindex==0&&c->s.modelindex);jar::portal::View view;CHECK(G_PortalView(*s,view));G_FreeEntity(helpers[0]);G_FreeEntity(&parent);}
 parent.inuse=qtrue;parent.classname=G_NewString("misc_portal_surface");parent.owner=nullptr;capacity=0;CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0);G_FreeEntity(&parent);
 capacity=10;parent.inuse=qtrue;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};const auto portalBlocked=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&!entities[portalBlocked]->inuse);G_FreeEntity(&parent);
 for(const char* name:{"misc_portal_surface","misc_portal_camera"}){
  parent.inuse=qtrue;parent.classname=G_NewString(name);parent.target=nullptr;capacity=0;
  CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count,true)&&!count&&G_PortalPortable(parent)&&parent.linked);
  CHECK(parent.e_UseFunc==useF_portable_portal_use&&!parent.owner&&!parent.target);
  G_FreeEntity(&parent);
 }
 parent.inuse=qtrue;parent.classname=G_NewString("misc_spotlight");parent.target=parent.target2=nullptr;capacity=10;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=1;VectorCopy(end,t->endpos);};
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==2&&G_SpotlightPortable(parent)&&parent.e_UseFunc==useF_spotlight_use&&!strcmp(parent.target2,helpers[0]->targetname)&&!strcmp(parent.target,helpers[1]->targetname)&&parent.wait==2048&&parent.contents==CONTENTS_ITEM);for(int i=0;i<count;++i)G_FreeEntity(helpers[i]);G_FreeEntity(&parent);
 parent.inuse=qtrue;capacity=1;const auto spotBefore=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!entities[spotBefore]->inuse);G_FreeEntity(&parent);
 parent.inuse=qtrue;capacity=10;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};const auto spotBlocked=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!entities[spotBlocked]->inuse);G_FreeEntity(&parent);
 parent.inuse=qtrue;parent.classname=G_NewString("misc_model_welder");parent.svFlags=0;CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&G_WelderPortable(parent)&&parent.e_UseFunc==useF_welder_use&&parent.e_ThinkFunc==thinkF_NULL&&parent.damage==10&&parent.radius==15&&parent.random==45&&parent.wait==.2f&&parent.contents==CONTENTS_ITEM&&(parent.svFlags&SVF_PLAYER_USABLE));G_FreeEntity(&parent);CHECK(portableWelders.empty());
 parent.inuse=qtrue;parent.classname=G_NewString("misc_model_jabba_cam");parent.lastEnemy=nullptr;capacity=10;VectorClear(parent.currentAngles);gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=1;VectorCopy(end,t->endpos);};
 CHECK(G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==2&&G_DoorCameraPortable(parent)&&parent.lastEnemy==helpers[0]&&helpers[0]->lastEnemy==helpers[1]&&G_DoorCameraRoot(helpers[1])==&parent&&parent.e_UseFunc==useF_jabba_cam_use&&parent.radius==48&&parent.speed==120&&parent.maxs[0]==34);for(int i=count-1;i>=0;--i)G_FreeEntity(helpers[i]);G_FreeEntity(&parent);CHECK(portableDoorCameras.empty());
 parent.inuse=qtrue;capacity=1;const auto doorBefore=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!entities[doorBefore]->inuse);G_FreeEntity(&parent);
 parent.inuse=qtrue;capacity=10;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->startsolid=qtrue;};const auto doorBlocked=entities.size();CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&!count&&!entities[doorBlocked]->inuse&&!entities[doorBlocked+1]->inuse&&!parent.lastEnemy);G_FreeEntity(&parent);
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t={};t->fraction=1;};
 gentity_t panel{};panel.inuse=qtrue;panel.classname=G_NewString("func_door");
 CHECK(G_JAR_SpawnWorkshopEntity(&panel,helpers,count)&&count==0&&G_SandboxSlidePortable(panel));
 CHECK(panel.e_UseFunc==useF_portable_sliding_door_use&&panel.e_ThinkFunc==thinkF_portable_sliding_door_think&&panel.contents==CONTENTS_SOLID);
 CHECK(panel.mins[0]==-36&&panel.mins[1]==-96&&panel.mins[2]==0&&panel.maxs[0]==6.828125f&&panel.maxs[1]==96&&panel.maxs[2]==170);
 CHECK(panel.svFlags&SVF_PLAYER_USABLE);
 G_FreeEntity(&panel);
 gentity_t lift{};lift.inuse=qtrue;lift.classname=G_NewString("func_plat");
 CHECK(G_JAR_SpawnWorkshopEntity(&lift,helpers,count)&&count==0&&G_LiftPortable(lift));
 CHECK(lift.damage==128&&lift.wait==1000&&lift.contents==CONTENTS_SOLID&&lift.maxs[2]==4.921875f);
 G_FreeEntity(&lift);
 gentity_t bob{};bob.inuse=qtrue;bob.classname=G_NewString("func_bobbing");
 CHECK(G_JAR_SpawnWorkshopEntity(&bob,helpers,count)&&count==0&&G_BobbingPortable(bob));
 CHECK(bob.contents==CONTENTS_SOLID&&bob.count==JAR_BOB_BOTTOM&&bob.speed==12&&bob.wait==4000);
 CHECK(bob.mins[2]==.0625f&&bob.maxs[2]==21.90625f&&(bob.svFlags&SVF_PLAYER_USABLE));
 G_FreeEntity(&bob);
 gentity_t gravityMarker{},frictionMarker{},activator{};
 gclient_t movementClient{};
 activator.inuse=qtrue;activator.health=100;activator.client=&movementClient;
 movementClient.ps.gravity=800;movementClient.ps.friction=6;
 gravityMarker.inuse=frictionMarker.inuse=qtrue;
 gravityMarker.classname=G_NewString("target_gravity_change");frictionMarker.classname=G_NewString("target_friction_change");
 CHECK(G_JAR_SpawnWorkshopEntity(&gravityMarker,helpers,count)&&count==0&&G_MovementMarkerPortable(gravityMarker));
 CHECK(G_JAR_SpawnWorkshopEntity(&frictionMarker,helpers,count)&&count==0&&G_MovementMarkerPortable(frictionMarker));
 CHECK(gravityMarker.e_UseFunc==useF_target_gravity_change_use&&gravityMarker.speed==400);
 CHECK(frictionMarker.e_UseFunc==useF_target_friction_change_use&&frictionMarker.health==2);
 CHECK(gravityMarker.contents==CONTENTS_ITEM&&frictionMarker.contents==CONTENTS_ITEM);
 const auto savedCvarSetter=gi.cvar_set;
 gi.cvar_set=[](const char*,const char*){throw std::runtime_error("Portable markers cannot change map cvars");};
 gravityMarker.spawnflags=1;
 target_gravity_change_use(&gravityMarker,nullptr,nullptr);CHECK(movementClient.ps.gravity==800);
 target_gravity_change_use(&gravityMarker,nullptr,&activator);
 CHECK(movementClient.ps.gravity==400&&(activator.svFlags&SVF_CUSTOM_GRAVITY));
 frictionMarker.svFlags|=SVF_INACTIVE;
 target_friction_change_use(&frictionMarker,nullptr,&activator);CHECK(movementClient.ps.friction==6);
 frictionMarker.svFlags&=~SVF_INACTIVE;
 target_friction_change_use(&frictionMarker,nullptr,&activator);CHECK(movementClient.ps.friction==2);
 activator.s.m_iVehicleNum=3;
 CHECK(G_MovementMarkerSet(frictionMarker,{jar::movement::Friction,10,true}));
 target_friction_change_use(&frictionMarker,nullptr,&activator);CHECK(movementClient.ps.friction==2);
 gi.cvar_set=savedCvarSetter;
 G_FreeEntity(&gravityMarker);G_FreeEntity(&frictionMarker);
 gentity_t delay{},counter{};
 delay.inuse=counter.inuse=qtrue;
 delay.classname=G_NewString("target_delay");delay.targetname=G_NewString("jar_sb_111");
 counter.classname=G_NewString("target_counter");counter.targetname=G_NewString("jar_sb_112");
 CHECK(G_JAR_SpawnWorkshopEntity(&delay,helpers,count)&&count==1&&delay.e_UseFunc==useF_Use_Target_Delay);
 jar::logic::State logic;
 CHECK(G_LogicRead(delay,logic)&&logic.delayMs==1000&&!logic.jitterMs);
 const int start=level.time;
 logic.delayMs=1300;logic.jitterMs=200;
 CHECK(G_LogicSet(delay,logic));Use_Target_Delay(&delay,nullptr,player);
 CHECK(delay.nextthink>=start+1100&&delay.nextthink<=start+1500&&delay.activator==player);
 const int deadline=delay.nextthink;
 logic.delayMs=0;logic.jitterMs=0;
 CHECK(G_LogicSet(delay,logic)&&delay.nextthink==deadline);
 MemorySave logicSave;ojk::SavedGameHelper logicWriter(&logicSave);
 delay.sg_export(logicWriter);logicSave.reset_buffer_offset();
 gentity_t loadedDelay{};loadedDelay.sg_import(logicWriter);logicWriter.ensure_all_data_read();
 CHECK(loadedDelay.nextthink==deadline&&loadedDelay.e_ThinkFunc==thinkF_Think_Target_Delay&&loadedDelay.wait==0);
 const int firstActivation=activations;
 Think_Target_Delay(&delay);CHECK(activations==firstActivation+1&&outputActivator==player&&!delay.activator);
 Use_Target_Delay(&delay,nullptr,player);CHECK(delay.nextthink==start+1);
 logic.enabled=false;CHECK(G_LogicSet(delay,logic));Think_Target_Delay(&delay);
 CHECK(activations==firstActivation+1&&!delay.activator);
 gentity_t authored{};authored.inuse=qtrue;authored.classname=delay.classname;authored.wait=2;
 authored.e_UseFunc=useF_Use_Target_Delay;authored.target=G_NewString("authored_output");
 Use_Target_Delay(&authored,nullptr,player);CHECK(authored.nextthink==start+2000);
 Think_Target_Delay(&authored);CHECK(usedTarget=="authored_output"&&authored.activator==player);
 CHECK(G_JAR_SpawnWorkshopEntity(&counter,helpers,count)&&count==1&&counter.e_UseFunc==useF_target_counter_use);
 counter.target2=G_NewString("progress");
 CHECK(G_LogicRead(counter,logic)&&logic.hits==2&&logic.remaining==2&&!logic.repeat);
 target_counter_use(&counter,nullptr,player);CHECK(counter.count==1&&usedTarget=="progress"&&outputActivator==player);
 target_counter_use(&counter,nullptr,player);CHECK(counter.count==0&&usedTarget==counter.target);
 const int spent=activations;
 target_counter_use(&counter,nullptr,player);CHECK(counter.count==0&&activations==spent);
 CHECK(G_LogicSet(counter,{jar::logic::Counter,0,0,3,1,true,true}));
 target_counter_use(&counter,nullptr,nullptr);CHECK(counter.count==3&&usedTarget==counter.target&&outputActivator==&counter);
 MemorySave counterSave;ojk::SavedGameHelper counterWriter(&counterSave);
 counter.sg_export(counterWriter);counterSave.reset_buffer_offset();
 gentity_t loadedCounter{};loadedCounter.sg_import(counterWriter);counterWriter.ensure_all_data_read();
 loadedCounter.classname=counter.classname;loadedCounter.targetname=counter.targetname;
 CHECK(G_LogicRead(loadedCounter,logic)&&logic.repeat&&logic.remaining==3&&logic.hits==3);

 gentity_t ticker{};
 ticker.inuse=qtrue;ticker.classname=G_NewString("func_timer");ticker.targetname=G_NewString("jar_sb_113");
 CHECK(G_JAR_SpawnWorkshopEntity(&ticker,helpers,count)&&count==1&&!ticker.nextthink);
 CHECK(G_LogicRead(ticker,logic)&&logic.kind==jar::logic::Timer&&logic.delayMs==1000&&logic.jitterMs==0);
 CHECK(!G_LogicSet(ticker,{jar::logic::Timer,150,51}));
 CHECK(G_LogicSet(ticker,{jar::logic::Timer,500,50}));
 const int beforeTick=activations;
 func_timer_use(&ticker,nullptr,player);
 CHECK(activations==beforeTick+1&&ticker.nextthink>=start+450&&ticker.nextthink<=start+550&&outputActivator==player);
 const int tickDeadline=ticker.nextthink;
 CHECK(G_LogicSet(ticker,{jar::logic::Timer,600,0,0,0,false,false})&&ticker.nextthink==tickDeadline);
 level.time=tickDeadline; ticker.nextthink=0;func_timer_think(&ticker);
 CHECK(ticker.nextthink==tickDeadline+600&&activations==beforeTick+1);
 CHECK(G_LogicSet(ticker,{jar::logic::Timer,600,0}));
 level.time=ticker.nextthink;ticker.nextthink=0;func_timer_think(&ticker);
 CHECK(activations==beforeTick+2&&ticker.nextthink==level.time+600);
 MemorySave timerSave;ojk::SavedGameHelper timerWriter(&timerSave);
 ticker.sg_export(timerWriter);timerSave.reset_buffer_offset();
 gentity_t loadedTimer{};loadedTimer.sg_import(timerWriter);timerWriter.ensure_all_data_read();
 CHECK(loadedTimer.nextthink==ticker.nextthink&&loadedTimer.e_ThinkFunc==thinkF_func_timer_think&&loadedTimer.wait==.6f);
 func_timer_use(&ticker,nullptr,player);CHECK(!ticker.nextthink&&!ticker.activator);
 restoredLogicIdentities=true;
 func_timer_use(&ticker,nullptr,nullptr);CHECK(ticker.nextthink>level.time&&activations==beforeTick+3&&outputActivator==&ticker);
 func_timer_use(&ticker,nullptr,player);
 restoredLogicIdentities=false;
 level.time=start;

 gentity_t selector{};
 selector.inuse=qtrue;selector.classname=G_NewString("target_random");selector.targetname=G_NewString("jar_sb_114");
 CHECK(G_JAR_SpawnWorkshopEntity(&selector,helpers,count)&&count==1&&G_LogicRead(selector,logic)&&logic.repeat);
 const int oldEntityCount=globals.num_entities;
 globals.num_entities=904;
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  auto& destination=g_entities[900+slot];
  destination.inuse=qtrue;destination.s.number=900+slot;
  destination.targetname=G_NewString(va("jar_sb_%d",900+slot));
  destination.e_UseFunc=useF_Use_Target_Print;
  G_LogicSetOutput(selector,slot,destination.targetname);
 }
 for(int use=0;use<80;++use)target_random_use(&selector,nullptr,player);
 int routed=0;
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  CHECK(dispatchedOutputs[900+slot]>0);
  routed+=dispatchedOutputs[900+slot];
 }
 CHECK(routed==80&&outputActivator==player);
 CHECK(G_LogicSet(selector,{jar::logic::Random,0,0,0,1,false,true}));
 for(int slot=0;slot<jar::logic::outputCount;++slot)g_entities[900+slot].svFlags|=SVF_INACTIVE;
 target_random_use(&selector,nullptr,player);CHECK(!selector.count);
 g_entities[903].svFlags&=~SVF_INACTIVE;
 const int last=dispatchedOutputs[903];
 target_random_use(&selector,nullptr,player);
 target_random_use(&selector,nullptr,player);
 CHECK(selector.count==1&&dispatchedOutputs[903]==last+1);
 MemorySave randomSave;ojk::SavedGameHelper randomWriter(&randomSave);
 selector.sg_export(randomWriter);randomSave.reset_buffer_offset();
 gentity_t loadedRandom{};loadedRandom.sg_import(randomWriter);randomWriter.ensure_all_data_read();
 loadedRandom.classname=selector.classname;loadedRandom.targetname=selector.targetname;
 CHECK(G_LogicRead(loadedRandom,logic)&&!logic.repeat&&!logic.remaining);
 loadedRandom.e_UseFunc=useF_NULL;loadedRandom.count=0;
 G_LogicRestore(loadedRandom);
 CHECK(G_LogicRead(loadedRandom,logic)&&!logic.repeat&&!logic.remaining);
 CHECK(G_LogicSet(selector,{jar::logic::Random,0,0,0,1,false,true}));
 restoredLogicIdentities=true;
 target_random_use(&selector,nullptr,nullptr);
 restoredLogicIdentities=false;
 CHECK(selector.count==1&&dispatchedOutputs[903]==last+2&&outputActivator==&selector);
 CHECK(G_LogicSet(selector,{jar::logic::Random,0,0,0,1,false,true}));
 g_entities[903].inuse=qfalse;
 target_random_use(&selector,nullptr,player);CHECK(!selector.count&&dispatchedOutputs[903]==last+2);
 for(int slot=0;slot<jar::logic::outputCount;++slot){g_entities[900+slot].inuse=qfalse;g_entities[900+slot].svFlags=0;}
 globals.num_entities=oldEntityCount;

 gentity_t relay{};
 relay.inuse=qtrue;relay.classname=G_NewString("target_relay");relay.targetname=G_NewString("jar_sb_115");
 CHECK(G_JAR_SpawnWorkshopEntity(&relay,helpers,count)&&count==1&&relay.e_UseFunc==useF_target_relay_use);
 CHECK(G_LogicRead(relay,logic)&&logic.kind==jar::logic::Relay&&logic.delayMs==0&&logic.cooldownMs==1000);
 logic.delayMs=400;logic.cooldownMs=250;CHECK(G_LogicSet(relay,logic));
 const int beforeRelay=activations;
 level.time=start+100;
 target_relay_use(&relay,nullptr,player);
 CHECK(relay.nextthink==start+500&&relay.activator==player&&activations==beforeRelay);
 level.time=start+300;
 target_relay_use(&relay,nullptr,&activator);
 CHECK(relay.nextthink==start+500&&relay.activator==player);
 MemorySave relaySave;ojk::SavedGameHelper relayWriter(&relaySave);
 relay.sg_export(relayWriter);relaySave.reset_buffer_offset();
 gentity_t loadedRelay{};loadedRelay.sg_import(relayWriter);relayWriter.ensure_all_data_read();
 loadedRelay.classname=relay.classname;loadedRelay.targetname=relay.targetname;loadedRelay.target=relay.target;
 CHECK(loadedRelay.nextthink==start+500&&loadedRelay.e_ThinkFunc==thinkF_target_relay_use_go);
 CHECK(G_LogicRead(loadedRelay,logic)&&logic.delayMs==400&&logic.cooldownMs==250);
 level.time=loadedRelay.nextthink;loadedRelay.nextthink=0;loadedRelay.activator=player;
 restoredLogicIdentities=true;target_relay_use_go(&loadedRelay);restoredLogicIdentities=false;
 CHECK(!loadedRelay.activator&&loadedRelay.painDebounceTime==start+750&&activations==beforeRelay+1&&outputActivator==player);
 MemorySave cooldownSave;ojk::SavedGameHelper cooldownWriter(&cooldownSave);
 loadedRelay.sg_export(cooldownWriter);cooldownSave.reset_buffer_offset();
 gentity_t coolingRelay{};coolingRelay.sg_import(cooldownWriter);cooldownWriter.ensure_all_data_read();
 CHECK(coolingRelay.painDebounceTime==start+750&&!coolingRelay.nextthink);
 target_relay_use(&loadedRelay,nullptr,player);CHECK(!loadedRelay.nextthink);
 level.time=start+750;
 logic.repeat=false;CHECK(G_LogicSet(loadedRelay,logic));
 target_relay_use(&loadedRelay,nullptr,nullptr);
 CHECK(loadedRelay.nextthink==start+1150);
 level.time=loadedRelay.nextthink;loadedRelay.nextthink=0;target_relay_use_go(&loadedRelay);
 CHECK(loadedRelay.count==1&&activations==beforeRelay+2&&outputActivator==&loadedRelay);
 level.time+=1000;
 target_relay_use(&loadedRelay,nullptr,player);CHECK(!loadedRelay.nextthink);
 logic.enabled=false;CHECK(G_LogicSet(loadedRelay,logic));
 loadedRelay.activator=player;target_relay_use_go(&loadedRelay);
 CHECK(!loadedRelay.count&&activations==beforeRelay+2&&!loadedRelay.activator);
 logic.enabled=true;logic.repeat=true;logic.delayMs=0;CHECK(G_LogicSet(loadedRelay,logic));
 target_relay_use(&loadedRelay,nullptr,player);target_relay_use(&loadedRelay,nullptr,player);
 CHECK(activations==beforeRelay+3&&!loadedRelay.nextthink&&loadedRelay.painDebounceTime==level.time+250);
 gentity_t mapRelay{};mapRelay.inuse=qtrue;mapRelay.classname=relay.classname;mapRelay.delay=1;mapRelay.wait=.5f;
 mapRelay.target=relay.target;SP_target_relay(&mapRelay);
 target_relay_use(&mapRelay,&activator,player);
 CHECK(mapRelay.nextthink==level.time+1000&&mapRelay.wait==500&&mapRelay.enemy==&activator&&mapRelay.painDebounceTime==0);

 gentity_t score{},kill{};
 score.inuse=kill.inuse=qtrue;
 score.classname=G_NewString("target_score");score.targetname=G_NewString("jar_sb_116");
 kill.classname=G_NewString("target_kill");kill.targetname=G_NewString("jar_sb_117");
 CHECK(G_JAR_SpawnWorkshopEntity(&score,helpers,count)&&count==0&&G_LogicRead(score,logic)&&logic.points==1);
 CHECK(G_JAR_SpawnWorkshopEntity(&kill,helpers,count)&&count==0&&kill.e_UseFunc==useF_target_kill_use);
 const auto previousCommand=gi.SendServerCommand;
 const auto previousClient=player->client;
 const int previousHealth=player->health,previousPowerups=player->s.powerups;
 gi.SendServerCommand=CaptureScore;
 player->client=&movementClient;player->health=100;
 CHECK(G_LogicRead(score,logic));logic.points=-15;CHECK(G_LogicSet(score,logic));
 movementClient.ps.persistant[PERS_SCORE]=50;
 Use_Target_Score(&score,nullptr,player);
 CHECK(movementClient.ps.persistant[PERS_SCORE]==35&&shownScore==35);
 Use_Target_Score(&score,nullptr,&score);CHECK(shownScore==35);
 logic.points=100;CHECK(G_LogicSet(score,logic));
 movementClient.ps.persistant[PERS_SCORE]=std::numeric_limits<int>::max()-1;
 Use_Target_Score(&score,nullptr,player);CHECK(shownScore==std::numeric_limits<int16_t>::max());
 logic.points=-100;CHECK(G_LogicSet(score,logic));
 movementClient.ps.persistant[PERS_SCORE]=std::numeric_limits<int>::min();
 Use_Target_Score(&score,nullptr,player);CHECK(shownScore==std::numeric_limits<int16_t>::min());
 CHECK(G_LogicRead(kill,logic)&&logic.killMode==jar::logic::Normal);
 expectedKill=player;
 target_kill_use(&kill,nullptr,&kill);CHECK(!killCalls);
 for(const int mode:{jar::logic::Normal,jar::logic::Falling,jar::logic::Electrical}){
  player->health=100;logic.killMode=mode;CHECK(G_LogicSet(kill,logic));
  target_kill_use(&kill,nullptr,player);
  CHECK(killMethod==(mode==jar::logic::Normal?MOD_UNKNOWN:mode==jar::logic::Falling?MOD_FALLING:MOD_ELECTROCUTE));
 }
 CHECK(killCalls==3&&fadeCalls==1&&(player->s.powerups&(1<<PW_SHOCKED)));
 logic.enabled=false;CHECK(G_LogicSet(kill,logic));
 player->health=100;target_kill_use(&kill,nullptr,player);CHECK(killCalls==3&&player->health==100);
 expectedKill=nullptr;player->client=previousClient;player->health=previousHealth;
 player->s.powerups=previousPowerups;gi.SendServerCommand=previousCommand;
 level.time=start;

 gentity_t give{};
 give.inuse=qtrue;give.classname=G_NewString("target_give");give.targetname=G_NewString("jar_sb_118");
 CHECK(G_JAR_SpawnWorkshopEntity(&give,helpers,count)&&count==0&&G_LogicRead(give,logic)&&logic.kind==jar::logic::Give);
 const int beforeGiveEntities=globals.num_entities;
 globals.num_entities=904;
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  auto& supply=g_entities[900+slot];
  supply.inuse=qtrue;supply.linked=qtrue;supply.s.number=900+slot;supply.s.eType=ET_ITEM;
  supply.item=Item(IT_AMMO,AMMO_BLASTER,"ammo_blaster");supply.e_TouchFunc=touchF_Touch_Item;
  supply.targetname=G_NewString(va(slot==2?"map_%d":"jar_sb_%d",900+slot));
  supply.contents=CONTENTS_ITEM;supply.nextthink=-2;supply.e_ThinkFunc=thinkF_G_FreeEntity;
  G_LogicSetOutput(give,slot,supply.targetname);
 }
 g_entities[903].s.eFlags=EF_NODRAW;
 activator.health=100;rejectedPickup=900;
 Use_Target_Give(&give,nullptr,&give);CHECK(giveAttempts.empty());
 logic.enabled=false;CHECK(G_LogicSet(give,logic));
 Use_Target_Give(&give,nullptr,&activator);CHECK(giveAttempts.empty());
 logic.enabled=true;CHECK(G_LogicSet(give,logic));
 Use_Target_Give(&give,nullptr,&activator);
 CHECK(giveAttempts.size()==2&&giveAttempts[0]==900&&giveAttempts[1]==901);
 CHECK(g_entities[900].linked&&g_entities[900].inuse&&g_entities[900].nextthink==-2);
 CHECK(!g_entities[901].inuse&&g_entities[902].inuse&&g_entities[903].inuse);
 MemorySave giveSave;ojk::SavedGameHelper giveWriter(&giveSave);
 g_entities[900].sg_export(giveWriter);giveSave.reset_buffer_offset();
 gentity_t loadedSupply{};loadedSupply.sg_import(giveWriter);giveWriter.ensure_all_data_read();
 CHECK(loadedSupply.nextthink==-2&&loadedSupply.e_ThinkFunc==thinkF_G_FreeEntity&&loadedSupply.linked);
 g_entities[903].s.eFlags=0;
 removeGiveOnTouch=&give;
 Use_Target_Give(&give,nullptr,&activator);
 CHECK(giveAttempts.size()==3&&giveAttempts.back()==900&&g_entities[903].inuse&&!give.inuse);
 removeGiveOnTouch=nullptr;
 for(int slot=0;slot<jar::logic::outputCount;++slot)g_entities[900+slot].inuse=qfalse;
 globals.num_entities=beforeGiveEntities;

 gentity_t enable{},disable{};
 enable.inuse=disable.inuse=qtrue;
 enable.classname=G_NewString("target_activate");enable.targetname=G_NewString("jar_sb_119");
 disable.classname=G_NewString("target_deactivate");disable.targetname=G_NewString("jar_sb_120");
 CHECK(G_JAR_SpawnWorkshopEntity(&enable,helpers,count)&&count==1&&enable.e_UseFunc==useF_target_activate_use);
 CHECK(G_LogicRead(enable,logic)&&logic.kind==jar::logic::Enable&&logic.enabled);
 CHECK(G_JAR_SpawnWorkshopEntity(&disable,helpers,count)&&count==1&&disable.e_UseFunc==useF_target_deactivate_use);
 CHECK(G_LogicRead(disable,logic)&&logic.kind==jar::logic::Disable&&logic.enabled);
 const int beforeGate=globals.num_entities;globals.num_entities=902;
 enable.target=disable.target=G_NewString("map_shared_target");
 for(int slot:{900,901}){
  auto& object=g_entities[slot];object.inuse=qtrue;object.s.number=slot;
  object.targetname=enable.target;object.svFlags=SVF_PLAYER_USABLE;
 }
 // A native map target can still enable or disable all matches of an authored name.
 target_deactivate_use(&disable,nullptr,nullptr);
 CHECK((g_entities[900].svFlags&SVF_INACTIVE)&&(g_entities[901].svFlags&SVF_INACTIVE));
 MemorySave gateSave;ojk::SavedGameHelper gateWriter(&gateSave);
 g_entities[900].sg_export(gateWriter);gateSave.reset_buffer_offset();
 gentity_t loadedGateTarget{};loadedGateTarget.sg_import(gateWriter);gateWriter.ensure_all_data_read();
 CHECK(loadedGateTarget.svFlags&SVF_INACTIVE);
 target_activate_use(&enable,nullptr,nullptr);
 CHECK(g_entities[900].svFlags==SVF_PLAYER_USABLE&&g_entities[901].svFlags==SVF_PLAYER_USABLE);
 g_entities[900].inuse=g_entities[901].inuse=qfalse;globals.num_entities=beforeGate;

 {
  using namespace jar::contactTrigger;
  const int oldTime=level.time,beforeContact=activations;
  capacity=10;
  gentity_t repeat{};repeat.inuse=qtrue;repeat.classname=G_NewString("trigger_multiple");
  CHECK(G_JAR_SpawnWorkshopEntity(&repeat,helpers,count)&&count==1);
  CHECK(!(repeat.svFlags&(SVF_NOCLIENT|SVF_PLAYER_USABLE))&&repeat.pos1[0]==-21&&repeat.pos2[2]==22);
  Settings settings;
  CHECK(G_ContactRead(repeat,settings)&&settings.kind==Repeat&&settings.interval==1000);
  CHECK(G_ContactSettings(repeat,{Repeat,200,500,0,Player,true,true}));
  const vec3_t size={80,12,24},corner={24,-24,10},edge={5,11,10};
  repeat.currentAngles[YAW]=45;CHECK(jar::volumeEntity::Resize(repeat,size));
  gentity_t actor{},npc{};gclient_t actorClient{},npcClient{};gNPC_t npcData{};
  actor.inuse=npc.inuse=qtrue;actor.health=npc.health=100;
  actor.client=&actorClient;npc.client=&npcClient;npc.NPC=&npcData;npc.s.number=42;
  VectorSet(actor.mins,-3,-3,-2);VectorSet(actor.maxs,3,3,2);
  actorClient.usercmd.buttons=BUTTON_USE;npcClient.usercmd.buttons=BUTTON_USE;
  G_ContactTouchAt(repeat,&actor,corner);CHECK(!G_ContactPending(repeat));
  G_ContactTouchAt(repeat,&npc,edge);CHECK(!G_ContactPending(repeat));
  actorClient.usercmd.buttons=0;
  G_ContactTouchAt(repeat,&actor,edge);CHECK(!G_ContactPending(repeat));
  actorClient.usercmd.buttons=BUTTON_USE;
  VectorSet(actor.currentOrigin,100,100,10);
  G_ContactTouchAt(repeat,&actor,edge);CHECK(G_ContactPending(repeat)&&repeat.nextthink==oldTime+200);
  G_ContactTouchAt(repeat,&actor,edge);CHECK(repeat.nextthink==oldTime+200&&activations==beforeContact);
  MemorySave contactSave;ojk::SavedGameHelper writer(&contactSave);
  repeat.sg_export(writer);contactSave.reset_buffer_offset();
  gentity_t loaded{};loaded.sg_import(writer);writer.ensure_all_data_read();
  loaded.classname=repeat.classname;loaded.model=repeat.model;loaded.target=repeat.target;loaded.activator=&actor;
  CHECK(G_ContactRestore(loaded)&&G_ContactPending(loaded)&&loaded.pos2[0]==40);
  level.time=loaded.nextthink;loaded.nextthink=0;
  restoredLogicIdentities=true;ContactFire(loaded);restoredLogicIdentities=false;
  CHECK(activations==beforeContact+1&&outputActivator==&actor&&loaded.painDebounceTime==level.time+500);
  G_ContactTouchAt(loaded,&actor,edge);CHECK(!G_ContactPending(loaded));
  level.time+=500;G_ContactTouchAt(loaded,&actor,edge);CHECK(G_ContactPending(loaded));
  CHECK(G_ContactSettings(loaded,{Repeat,200,500,0,Player,true,false}));
  level.time=loaded.nextthink;ContactFire(loaded);
  CHECK(activations==beforeContact+1&&!G_ContactPending(loaded)&&!loaded.activator);
  G_FreeEntity(helpers[0]);G_FreeEntity(&repeat);

  gentity_t once{};once.inuse=qtrue;once.classname=G_NewString("trigger_once");
  CHECK(G_JAR_SpawnWorkshopEntity(&once,helpers,count)&&count==1);
  CHECK(G_ContactSettings(once,{Once,0,0,0,NPCs,true,true}));
  ContactUse(once,&actor);ContactUse(once,&actor);
  CHECK(activations==beforeContact+2&&G_ContactSpent(once)&&!(once.contents&CONTENTS_TRIGGER));
  contactSave.reset_buffer();once.sg_export(writer);contactSave.reset_buffer_offset();
  gentity_t spent{};spent.sg_import(writer);writer.ensure_all_data_read();
  spent.classname=once.classname;spent.model=once.model;spent.target=once.target;
  CHECK(G_ContactRestore(spent)&&G_ContactSpent(spent));
  G_ContactSetSpent(spent,false);ContactUse(spent,&npc);
  CHECK(G_ContactSpent(spent)&&outputActivator==&npc&&activations==beforeContact+3);
  once.model=G_NewString("@jar_box");once.e_TouchFunc=touchF_NULL;once.e_UseFunc=useF_NULL;
  CHECK(G_ContactRestore(once)&&G_ContactSpent(once)&&once.e_UseFunc==useF_Use_Multi);
  G_FreeEntity(helpers[0]);G_FreeEntity(&once);
  gentity_t hurt{};hurt.inuse=qtrue;hurt.classname=G_NewString("trigger_hurt");
  CHECK(G_JAR_SpawnWorkshopEntity(&hurt,helpers,count)&&!count);
  CHECK(G_ContactRead(hurt,settings)&&Same(settings,{Hurt,0,1000,0,Actors,false,true,5,Normal}));
  CHECK(hurt.linked&&!(hurt.svFlags&(SVF_NOCLIENT|SVF_PLAYER_USABLE)));
  hurt.model=G_NewString("@jar_box");hurt.linked=qfalse;
  CHECK(G_ContactRestore(hurt)&&G_ContactRead(hurt,settings)&&settings.enabled&&!settings.armed&&hurt.linked);
  G_FreeEntity(&hurt);level.time=oldTime;
  capacity=10;
  gentity_t push{};push.inuse=qtrue;push.classname=G_NewString("trigger_push");
  CHECK(G_JAR_SpawnWorkshopEntity(&push,helpers,count)&&count==1);
  jar::push::Settings pushSettings;
  CHECK(G_PushRead(push,pushSettings)&&pushSettings.aim==jar::push::Linked&&pushSettings.speed==1000);
  CHECK(push.target&&!strcmp(push.target,helpers[0]->targetname)&&!push.nextthink&&push.pos1[0]==-21);
  push.model=G_NewString("@jar_box");push.e_ThinkFunc=thinkF_AimAtTarget;push.nextthink=500;
  CHECK(G_PushRestore(push)&&G_PushRead(push,pushSettings)&&!push.nextthink);
  G_FreeEntity(helpers[0]);G_FreeEntity(&push);
  for(const char* type:{"trigger_teleport","misc_teleporter","target_teleporter"}){
   gentity_t gate{};gate.inuse=qtrue;gate.classname=G_NewString(type);
   gate.model=G_NewString("models/map_objects/imperial/crate1.md3");
   CHECK(G_JAR_SpawnWorkshopEntity(&gate,helpers,count)&&count==1);
   CHECK(!strcmp(helpers[0]->classname,"misc_teleporter_dest")&&helpers[0]->currentOrigin[2]==0);
   CHECK(gate.target&&!strcmp(gate.target,helpers[0]->targetname)&&!gate.nextthink);
   jar::teleport::Settings settings;
   CHECK(G_TeleportRead(gate,settings)&&settings.interval==jar::teleport::defaultInterval);
   if(settings.kind==jar::teleport::Volume){
    gate.model=G_NewString("@jar_box");gate.e_ThinkFunc=thinkF_trigger_teleporter_find_closest_portal;gate.nextthink=500;
    CHECK(G_TeleportRestore(gate)&&!gate.nextthink&&G_TeleportRead(gate,settings));
   }
   G_FreeEntity(helpers[0]);G_FreeEntity(&gate);
  }
 }

 {
  using namespace jar::siege;
  capacity=10;
  const auto oldTrace=gi.trace;
  const auto oldRead=gi.FS_ReadFile;
  const auto oldCommand=gi.SendServerCommand;
  gi.FS_ReadFile=[](const char*,void**)->long{return 1;};
  gi.SendServerCommand=[](int,const char*,...){};
  gi.pointcontents=[](const vec3_t,int){return 0;};
  gi.trace=[](trace_t* trace,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){
   *trace={};trace->fraction=.5f;VectorCopy(end,trace->endpos);
  };
  auto* item=G_TrySpawn();
  item->classname=G_NewString(itemClass);item->model=G_NewString(crystalModel);
  item->targetname=G_NewString("jar_sb_991");item->target2=G_NewString("take_crystal");
  item->target3=G_NewString("place_crystal");item->closetarget=G_NewString("return_crystal");
  item->opentarget=G_NewString("drop_crystal");
  item->spawnflags=forceLimit;
  CHECK(G_JAR_SpawnWorkshopEntity(item,helpers,count)&&count==1&&Item(*item));
  auto* goal=helpers[0];
  CHECK(goal->e_UseFunc==useF_siege_objective_use&&goal->spawnflags&requiresItem&&
        !strcmp(goal->targetname,item->target)&&goal->s.modelindex>0);
  auto& carrier=g_entities[0];
  gclient_t client{};carrier.inuse=qtrue;carrier.client=&client;carrier.health=100;
  const int before=activations;
  item->spawnflags|=offRadar;
  siege_item_use(item,item,&carrier);
  CHECK(!item->activator&&!(item->spawnflags&offRadar)&&activations==before);
  siege_objective_use(goal,goal,&carrier);
  CHECK(!goal->setTime&&activations==before);
  siege_item_touch(item,&carrier,nullptr);
  CHECK(item->activator==&carrier&&G_SiegeForceLimited(carrier)&&!item->contents&&usedTarget=="take_crystal");
  G_SiegeForget(item);CHECK(!G_SiegeForceLimited(carrier));
  G_SiegeRestore();CHECK(G_SiegeForceLimited(carrier));
  foreignSiegeGoal=goal;
  siege_objective_use(goal,goal,&carrier);
  CHECK(!goal->setTime&&G_SiegeForceLimited(carrier));
  foreignSiegeGoal=nullptr;
  carrier.health=0;
  siege_item_think(item);
  CHECK(!item->activator&&!G_SiegeForceLimited(carrier)&&item->nextthink==level.time+returnDelay&&usedTarget=="drop_crystal");
  const int oldTime=level.time;
  level.time=item->nextthink;siege_item_think(item);level.time=oldTime;
  CHECK(VectorCompare(item->currentOrigin,item->pos1)&&usedTarget=="return_crystal"&&!item->nextthink);
  carrier.health=100;
  siege_item_touch(item,&carrier,nullptr);
  CHECK(activations==before+3); // The pickup output is once-only.
  siege_objective_use(goal,goal,&carrier);
  CHECK(goal->setTime==1&&item->spawnflags&delivered&&!item->linked&&usedTarget=="place_crystal"&&activations==before+4);
  siege_objective_use(goal,goal,&carrier);
  CHECK(activations==before+4&&!item->activator&&!G_SiegeForceLimited(carrier));
  gentity_t marker{};
  marker.inuse=qtrue;marker.classname=G_NewString(endClass);
  marker.model=G_NewString(JAR_LOGIC_PLACEHOLDER_MODEL);marker.target=G_NewString("round_finished");
  CHECK(G_JAR_SpawnWorkshopEntity(&marker,helpers,count)&&!count&&marker.e_UseFunc==useF_siege_end_use);
  siege_end_use(&marker,&marker,&carrier);siege_end_use(&marker,&marker,&carrier);
  CHECK(marker.setTime==1&&activations==before+5&&usedTarget=="round_finished");
  G_FreeEntity(&marker);
  gentity_t armed{};
  armed.inuse=qtrue;armed.classname=G_NewString(objectiveClass);armed.spawnflags=offRadar;
  armed.target=G_NewString("objective_armed");
  CHECK(G_JAR_SpawnWorkshopEntity(&armed,helpers,count)&&!count&&armed.e_UseFunc==useF_siege_objective_use);
  siege_objective_use(&armed,&armed,&carrier);
  CHECK(!(armed.spawnflags&offRadar)&&!armed.setTime&&activations==before+5);
  siege_objective_use(&armed,&armed,&carrier);
  CHECK(armed.setTime==1&&activations==before+6&&usedTarget=="objective_armed");
  G_FreeEntity(&armed);
  carrier.client=nullptr;
  G_FreeEntity(goal);G_FreeEntity(item);
  gi.trace=oldTrace;gi.FS_ReadFile=oldRead;gi.SendServerCommand=oldCommand;
 }

 gentity_t button{};button.inuse=qtrue;button.classname=G_NewString("func_button");
 CHECK(G_JAR_SpawnWorkshopEntity(&button,helpers,count)&&count==1&&G_ButtonPortable(button));
 CHECK(button.e_UseFunc==useF_portable_button_use&&button.e_ThinkFunc==thinkF_portable_button_think&&button.contents==CONTENTS_SOLID);
 CHECK(button.mins[0]==-17.5f&&button.maxs[0]==14.71875f&&button.maxs[2]==26.390625f);
 CHECK(button.target&&!strcmp(button.target,helpers[0]->targetname));
 G_FreeEntity(helpers[0]);G_FreeEntity(&button);
 // Real NS turret setup passes through the portable transaction and restores its spawn scope.
 gi.FS_ReadFile=[](const char*,void**)->long{return -1;};
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&g,const char*p,int,qhandle_t,qhandle_t,int,int){CHECK(!strcmp(p,"models/map_objects/imp_mine/turret_canon.glm"));g.resize(1);return 0;};
 gi.G2API_SetBoneAngles=[](CGhoul2Info*,const char*,const vec3_t,int,Eorientations,Eorientations,Eorientations,qhandle_t*,int,int){return qtrue;};
 gi.G2API_AddBolt=[](CGhoul2Info*,const char*p){CHECK(!strcmp(p,"*flash03"));return 5;};
 gentity_t ns{};ns.inuse=qtrue;ns.classname=G_NewString("misc_ns_turret");
 CHECK(G_JAR_SpawnWorkshopEntity(&ns,helpers,count)&&!count&&ns.wait==200&&ns.health==40&&ns.torsoBolt==5&&ns.e_ThinkFunc==thinkF_turret_base_think&&ns.e_UseFunc==useF_turret_base_use);
 CHECK(numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning&&!strcmp(ns.classname,"misc_ns_turret"));G_FreeEntity(&ns);ns.ghoul2.clear();
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&,const char*,int,qhandle_t,qhandle_t,int,int){return -1;};ns.inuse=qtrue;
 CHECK(!G_JAR_SpawnWorkshopEntity(&ns,helpers,count)&&!count&&!ns.inuse&&numSpawnVars==1&&spawnVars[0][1]==saved&&!spawning);
 globals.num_entities=1000;for(int i=900;i<932;++i){auto&z=g_entities[i];z.inuse=qtrue;z.classname=G_NewString("fx_wind_zone");z.model=G_NewString("@jar_box");}parent.inuse=qtrue;parent.classname=G_NewString("fx_wind_zone");parent.model=G_NewString("unchanged");const auto beforeDispatch=dispatched;CHECK(!G_JAR_SpawnWorkshopEntity(&parent,helpers,count)&&count==0&&dispatched==beforeDispatch&&!strcmp(parent.model,"unchanged"));
 });}
