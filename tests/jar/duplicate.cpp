// Production clipboard/paste/undo against deterministic native service boundaries.
#include "game/g_local.h"
#include "game/g_workshop.h"
#include "game/g_randomizer.h"
#include "game/g_scene_camera.h"
#include "game/g_rail_portable.h"
#include "game/g_volume_geometry.h"
#include "game/g_contact_trigger.h"
#include "game/g_sandbox_push.h"
#include "game/g_push_prop_motion.h"
#include "game/g_teleport_prop_motion.h"
#include "game/g_sandbox_teleport.h"
#include "game/g_sandbox_vehicle_teleport.h"
#include "game/g_teleport_mover_motion.h"
#include "game/g_subbsp_movers.h"
#include "game/g_workshop_undo_save.h"
#include "game/g_bsp_section_copy.h"
#include "memory_save.h"
#include "../../shared/qcommon/jar_rope_path_mesh.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_reference_tags.h"
#include "game/g_combat_points.h"
#include "game/g_level_change.h"
#include "game/jar_physics.h"
#include "qcommon/jar_duplicate.h"
#include "qcommon/jar_entity_presets.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "test_support.h"
#include "bsp_section_motion_stub.h"
#include "game/g_script_parms.h"
#include "game/g_workshop_npc_loadout.h"
#include "game/g_workshop_npc_order.h"
#include "game/g_workshop_vehicle_status.h"
#include "game/g_sandbox_vehicle_push.h"
#include "game/g_sandbox_vehicle_transport.h"
#include "game/g_sandbox_secret.h"
#include "game/g_workshop_pickup_quantity.h"
#include "qcommon/jar_music.h"
#include <deque>
#include <map>
#include <set>
#include <limits>
#include <unordered_set>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
bool G_JAR_WorkshopPropDepth(const gentity_t&,const vec3_t,const vec3_t,const vec3_t,const JarSweepHull*,float&){return false;}
gentity_t g_entities[MAX_GENTITIES];
cvar_t* g_gravity=nullptr;
void G_SpaceGravity(gentity_t*,float){throw std::runtime_error("Actor movement is tested in the Context fixture");}
bool G_SpacePortable(const gentity_t&){return false;}
void G_SpaceSet(gentity_t&,bool,bool){throw std::runtime_error("Space activation is tested in the Context and Space fixtures");}
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
using namespace jar::workshop;
bool G_BSPSectionBreakable(const gentity_t&){return false;}
gentity_t* G_BSPSectionBreakableRoot(const gentity_t&){return nullptr;}
State state;jar::physics::World physicsWorld;
void AI_DeleteSelfFromGroup(gentity_t*){throw std::runtime_error("Unexpected Context NPC change in Duplicate fixture");}
void G_ClearEnemy(gentity_t*){throw std::runtime_error("Unexpected Context enemy change in Duplicate fixture");}
qboolean G_ValidEnemy(gentity_t*,gentity_t*){throw std::runtime_error("Unexpected Context enemy check in Duplicate fixture");}
constexpr uint32_t noEntity=ENTITYNUM_NONE;
uint32_t held=noEntity,selected=noEntity;float holdDistance=144;
struct {struct {vec3_t vieworg{},viewaxis[3]{{1,0,0},{0,1,0},{0,0,1}};} refdef;int time=0;} cg;
struct Prop {uint32_t id;const char*model;bool rigid;} ;std::vector<Prop> props{{1001,"crate.md3",true},{1002,"box.md3",true}};
struct Actor {uint32_t id;const char*name;};std::vector<Actor> actors{{3001,"stormtrooper"},{4001,"swoop"}};
struct Supply {uint32_t id;};std::vector<Supply> supplies{{2501}};
const Prop*prop(unsigned id){for(const auto&p:props)if(p.id==id)return &p;return nullptr;}
const Actor*actor(unsigned id){for(const auto& entry:actors)if(entry.id==id)return &entry;return nullptr;}
gitem_t pickup{},holoPickup{};gitem_t*supplyItem(const Supply&s){const auto*e=jar::catalog::Find(s.id);return e&&!strcmp(e->name,"holocron_force_heal")?&holoPickup:&pickup;}
struct PhysicsBounds {jar::physics::Vec half{10,10,10},center{};};
bool modelBounds(const Prop&,PhysicsBounds&,int=0){return true;}
bool propHull(const Prop&,const vec3_t,vec3_t mins,vec3_t maxs,int){
 VectorSet(mins,-10,-10,-10);VectorSet(maxs,10,10,10);return true;
}
void nativeVector(jar::physics::Vec v,float*p){p[0]=v.x;p[1]=v.y;p[2]=v.z;}
static std::deque<parms_t> parameterStorage;
void*G_Alloc(int size){CHECK(size==sizeof(parms_t));parameterStorage.emplace_back();return &parameterStorage.back();}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}

static std::vector<int> speakerEvents;
void G_AddEvent(gentity_t* entity,int event,int param){
 CHECK(event==EV_GENERAL_SOUND&&param==entity->noise_index);
 speakerEvents.push_back(entity->s.number);
}
#include "game/g_sandbox_speaker.inc"

void G_ShipBoundaryForget(gentity_t*){} // Lifetime cancellation is exercised in shipboundary.cpp.
#include "game/g_shipboundary_properties.inc"
#include "game/g_cloud.h"
#include "game/g_reflection_probe.h"
#include "game/g_crystal_crate.h"
bool G_CrystalCrateMoved(gentity_t&e){return G_CrystalCratePortable(e);}
#include "game/g_door_camera.h"
bool G_DoorCameraMoved(gentity_t&e){return G_DoorCameraPortable(e);}
#include "game/g_welder.h"
bool G_WelderMoved(gentity_t&e){return G_WelderPortable(e);}
#include "game/g_spotlight.h"
bool G_SpotlightMoved(gentity_t&e){return G_SpotlightPortable(e);}
#include "game/g_portal.h"
bool G_PortalMoved(gentity_t&e){return G_PortalPortable(e);}
void G_PortalFrame(){}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){
 throw std::runtime_error("Duplicate applied damage before its first think");
}
#include "game/g_laser_arm.inc"
#include "game/g_weather_properties.inc"
#include "game/g_scriptrunner_properties.inc"
#include "scriptrunner_files.h"
bool G_MusicSetTrack(gentity_t&e,const char*s){std::string text;if(!jar::music::Class(e.classname)||!jar::music::Canonical(s,text))return false;e.message=G_NewString(text.c_str());return true;}
bool G_MusicActive(const gentity_t&){return false;}
static bool on=true,blocked=false;static unsigned busy=noEntity,attempt=0,failAt=0,nextSlot=100;
static std::string status;static std::map<std::string,std::string> vars;static std::map<unsigned,gclient_t> clients;
static std::map<unsigned,gNPC_t> npcInfo;
weaponData_t weaponData[WP_NUM_WEAPONS]{};
gitem_t bg_itemlist[3]{};
const int bg_numItems=3;
static bool allowNpcWeapons=true,attachWeapon=true;
static bool allocateSaberHelper=true,attachSabers=true;
static int registeredNpcItems=0;
qboolean G_CanPickUpWeapons(const gentity_t*){return allowNpcWeapons?qtrue:qfalse;}
void RegisterItem(gitem_t*){++registeredNpcItems;}
void ChangeWeapon(gentity_t* actor,int weapon){
 actor->client->ps.weapon=weapon;
 actor->NPC->currentAmmo=actor->client->ps.ammo[weaponData[weapon].ammoIndex];
}
void G_RemoveWeaponModels(gentity_t* actor){actor->weaponModel[0]=actor->weaponModel[1]=-1;}
qboolean G_StandardHumanoid(gentity_t*){return qfalse;}
bool G_WorkshopNPCOrderScripted(gentity_t&){return false;}
bool G_ReplaceG2HandWeapon(gentity_t&,const char*){throw std::runtime_error("Hand swaps are tested in the Context fixture");}
void G_CreateG2AttachedWeaponModel(gentity_t* actor,const char* model,int,int slot){
 CHECK(!std::strcmp(model,"models/weapons2/disruptor/disruptor.md3")&&slot==0);
 if(attachWeapon)actor->weaponModel[0]=1;
}
void WP_SaberFreeStrings(saberInfo_t& saber){saber.name=nullptr;saber.model=nullptr;}
qboolean WP_SaberParseParms(const char* name,saberInfo_t* saber,qboolean){
 if(std::strcmp(name,"fixture_primary")&&std::strcmp(name,"fixture_secondary"))return qfalse;
 saber->name=G_NewString(name);
 saber->model=G_NewString("models/weapons2/saber/saber_w.glm");
 saber->numBlades=!std::strcmp(name,"fixture_primary")?2:1;
 return qtrue;
}
bool WP_SaberInitSandboxBladeData(gentity_t* actor){
 if(!allocateSaberHelper)return false;
 auto& helper=g_entities[nextSlot];
 helper.s.number=nextSlot++;
 helper.inuse=qtrue;helper.owner=actor;helper.classname=G_NewString("lightsaber");
 actor->client->ps.saberEntityNum=helper.s.number;
 actor->client->ps.saberLockEnemy=ENTITYNUM_NONE;
 return true;
}
void WP_SaberAddG2SaberModels(gentity_t* actor,int){
 if(!attachSabers)return;
 actor->weaponModel[0]=1;
 actor->weaponModel[1]=actor->client->ps.dualSabers?2:-1;
}
bool sandboxReady(){return on;}
void message(const char*s){status=s;}
static int listCalls=0;
void list(){++listCalls;}
static bool actorRecoverySuspended=false;
bool G_JAR_SuspendActorRecovery(bool value){
 const bool previous=actorRecoverySuspended;
 actorRecoverySuspended=value;
 return previous;
}
Owned*owned(unsigned entity){for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].entity==entity)return &state.owned[i];return nullptr;}
bool matches(const Owned&o){return o.entity<ENTITYNUM_WORLD&&g_entities[o.entity].inuse&&(o.catalog==consoleCatalog||(g_entities[o.entity].targetname&&!strcmp(g_entities[o.entity].targetname,va("jar_sb_%u",o.id))));}
static bool spawnCountDirty;
#include "game/g_workshop_children.inc"
bool occupied(const gentity_t&e){return unsigned(e.s.number)==busy;}
bool removalOccupied(const gentity_t& e){return occupied(e);}
void eye(float*origin,float*direction){VectorClear(origin);VectorClear(direction);direction[0]=1;}
void NPC_SetAnim(gentity_t*e,int,int anim,int,int){e->client->ps.torsoAnim=anim;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
void SetClientViewAngle(gentity_t*e,vec3_t a){VectorCopy(a,e->client->ps.viewangles);}
#include "game/g_sliding_door.h"
#include "game/g_button_sandbox.h"
#include "game/g_bobbing_sandbox.h"
#include "game/g_workshop_rack_pose.h"
bool setupMoved(gentity_t&e){return G_SandboxSlideMoved(e)||G_ButtonMoved(e)||G_BobbingMoved(e)||jar::volumeEntity::Refresh(e)||G_WeatherPortable(e)||G_CloudMoved(e)||G_ReflectionMoved(e)||workshopRack(e);}
void physicsInvalidate(unsigned entity){physicsWorld.Forget(entity);}
#include "game/g_mover_motion.h"
#include "game/g_workshop_native_control.inc"
#include "game/g_workshop_pickup_lifetime.inc"
#include "sliding_door_fixture.h"
bool G_LiftTranslate(gentity_t&,float){throw std::runtime_error("Lift motion uses the context fixture");}
void releaseHeld(){if(held!=noEntity){nativeRelease(held,true);physicsWorld.Release(held);}held=noEntity;}
void forgetJointEdit(unsigned id);
#include "game/g_workshop_remove.inc"
bool removeJoint(unsigned id){if(!ForgetJoint(state,id))return false;forgetJointEdit(id);return true;}
bool setPropPose(gentity_t&,int){throw std::runtime_error("Unexpected pose Undo in duplication fixture");}
static const gentity_t* hiddenBeamEndpoint=nullptr;
bool G_SubBSPNameVisible(const gentity_t* entity){return entity!=hiddenBeamEndpoint;}
gentity_t*G_Find(gentity_t* after,int,const char* name){
 if(!name||!*name)return nullptr;
 for(int i=after?int(after-g_entities)+1:0;i<globals.num_entities;++i){
  auto& entity=g_entities[i];
  if(entity.inuse&&entity.targetname&&!Q_stricmp(entity.targetname,name)&&G_SubBSPNameVisible(&entity))return &entity;
 }
 return nullptr;
}
#include "reference_fixture.h"
static bool waypointAdmission(){return true;}
#include "game/g_waypoint_properties.inc"
const char*G_WaypointStatus(const gentity_t&){return "Native graph tested separately";}
static int undoFeedbackCount;void undoFeedback(){++undoFeedbackCount;}
#include "game/g_logic_settings.h"
static std::vector<std::pair<gentity_t*,gentity_t*>> timerOutputs;
bool G_JAR_WorkshopOwns(const gentity_t* e){const auto* entry=e?owned(e->s.number):nullptr;return entry&&matches(*entry);}
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
void G_UseTargets(gentity_t* source,gentity_t* activator){
 if(auto* target=G_Find(nullptr,FOFS(targetname),source->target))timerOutputs.emplace_back(target,activator);
}
#include "game/g_target_secret.inc"
static gentity_t* trailSoundEvent=nullptr;
gentity_t* G_TempEntity(const vec3_t origin,int event){
 if(trailSoundEvent){
  CHECK(event==EV_BMODEL_SOUND);VectorCopy(origin,trailSoundEvent->currentOrigin);
 }
 return trailSoundEvent;
}
#include "game/g_replicator_use.inc"
qboolean G_SpawnFloat(const char*,const char*,float*){throw std::runtime_error("Unexpected map timer spawn");}
char* vtos(const vec3_t){throw std::runtime_error("Unexpected map timer warning");}
#include "game/g_logic_timer.inc"
static void PortableHurtDamage(gentity_t&,gentity_t&){throw std::runtime_error("Unexpected Hurt contact");}
#include "game/g_contact_trigger.inc"
#include "game/g_workshop_give.inc"
static uint32_t attachmentKey(const gentity_t&);
static std::shared_ptr<const jar::physics::PropGeometry> sceneryFixtureGeometry;
std::shared_ptr<const jar::physics::PropGeometry> propTransformGeometry(const Prop&,int frame){
 return frame==5?sceneryFixtureGeometry:nullptr;
}
bool propSceneryHull(const Prop& model,int frame,const vec3_t angles,JarSweepHull& shape){
 const auto geometry=propTransformGeometry(model,frame);
 return gi.JAR_TraceSceneryHull&&geometry&&jar::physics::BuildPropSweep(*geometry,{angles[0],angles[1],angles[2]},shape);
}
#include "game/g_workshop_rack_pose.inc"
#include "game/g_workshop_edits.inc"
#include "game/g_workshop_undo_history.inc"
#include "game/g_workshop_link_cleanup.inc"
static unsigned cleanupSelected=noEntity;
static bool refreshAfterCleanup=false;
void G_JAR_WorkshopForget(gentity_t* entity,bool freeing){
 if(const auto* entry=owned(entity->s.number))forgetWorkshopChildren(*entry,freeing);
 if(freeing)refreshAfterCleanup|=forgetWorkshopReferences(*entity,cleanupSelected);
 nativeControls.erase(entity->s.number);forgetEdits(entity->s.number);Forget(state,entity->s.number);
}
void G_FreeEntity(gentity_t* entity){
 G_WorkshopFreeActorSaber(*entity);
 G_SandboxChargesForget(*entity);
 G_SkyPortalForget(entity);G_CombatPointForget(entity);G_ReferenceForget(entity);
 G_JAR_WorkshopForget(entity);entity->inuse=qfalse;
}
gentity_t* G_TrySpawn(qboolean){
 const int slot=nextSlot++;
 globals.num_entities=std::max(globals.num_entities,int(nextSlot));
 auto& entity=g_entities[slot];
 entity.inuse=qtrue;entity.s.number=slot;
 return &entity;
}
int G_EffectIndex(const char* effect){
 if(!strcmp(effect,"detpack/explosion"))return 95;
 CHECK(!strcmp(effect,"tripMine/explosion"));return 96;
}
bool in_camera=false;
#include "game/g_portable_shield.inc"
unsigned Catalog(const char*name){for(const auto&e:jar::catalog::entries)if(!strcmp(e.name,name))return e.id;throw std::runtime_error(name);}
static bool regenerateRackStock=true;
gentity_t*Make(unsigned catalog,const float*pos,const float*angles=nullptr){
 const auto slot=nextSlot++;globals.num_entities=std::max(globals.num_entities,int(nextSlot));auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.client=nullptr;e.item=nullptr;e.target=e.target2=e.target3=nullptr;
 e.classname=G_NewString(prop(catalog)?"jar_sandbox_prop":actor(catalog)?"NPC":jar::catalog::Find(catalog)?jar::catalog::Find(catalog)->name:"item_medpak_instant");
 VectorSet(e.mins,-10,-10,-10);VectorSet(e.maxs,10,10,10);G_SetOrigin(&e,pos);if(angles)G_SetAngles(&e,angles);
 if(jar::waypoint::Class(e.classname))CHECK(G_WaypointSpawn(e));
 if(jar::rail::Class(e.classname)!=jar::rail::None){e.model=G_NewString("@jar_rail");e.count=0;jar::rail::Settings rs;rs.kind=jar::rail::Class(e.classname);CHECK(G_RailSet(e,rs));}
 if(jar::sceneCamera::Class(e.classname)){e.model=G_NewString("@jar_scene_camera");e.count=0;CHECK(G_SceneCameraSet(e,{}));}
 if(jar::reflection::Class(e.classname)){e.model=G_NewString("@jar_reflection");e.count=1;CHECK(G_ReflectionSet(e,{}));}
 if(jar::skyPortal::Class(e.classname))CHECK(G_SkyPortalSpawn(e));
 if(!strcmp(e.classname,"point_combat"))CHECK(G_CombatPointSpawn(e));
 if(actor(catalog)){
  e.NPC=&npcInfo[slot];e.client=&clients[slot];e.client->ps.gravity=800;
  e.client->moveType=MT_RUNJUMP;e.client->ps.friction=6;
  e.client->playerTeam=TEAM_ENEMY;e.client->enemyTeam=TEAM_PLAYER;
  e.client->ps.weapon=e.s.weapon=WP_BLASTER;e.client->ps.stats[STAT_WEAPONS]=1<<WP_BLASTER;
  e.weaponModel[0]=1;e.weaponModel[1]=-1;
  e.NPC_type=G_NewString(actor(catalog)->name);e.takedamage=qtrue;e.health=100;
  if(catalog==4001){
   static vehicleInfo_t info{};
   static std::map<unsigned,Vehicle_t> vehicleStorage;
   auto& vehicle=vehicleStorage[slot];
   e.m_pVehicle=&vehicle;
   vehicle.m_pParentEntity=&e;vehicle.m_pVehicleInfo=&info;
   vehicle.m_iArmor=e.health;vehicle.m_iShields=50;
   vehicle.weaponStatus[0].ammo=80;vehicle.turretStatus[0].ammo=90;
   vehicle.turretStatus[0].enemyEntNum=ENTITYNUM_NONE;
   e.client->ps.stats[STAT_ARMOR]=vehicle.m_iShields;
  }
 }
 if(!strcmp(e.classname,"shooter_grenade")){
  e.client=&clients[slot];e.wait=500;e.e_UseFunc=useF_misc_weapon_shooter_use;
  e.s.weapon=e.client->ps.weapon=WP_THERMAL;
 }
 if(!strcmp(e.classname,"target_secret")){e.e_UseFunc=useF_target_secret_use;e.noise_index=81;}
 if(!strcmp(e.classname,"misc_replicator_item"))G_ReplicatorApplyPhase(e,{},level.time);
 if(catalog==2501||!strcmp(e.classname,"item_force_boon")){
  e.item=&pickup;e.count=25;e.s.eType=ET_ITEM;e.e_TouchFunc=touchF_Touch_Item;e.flags=FL_DROPPED_ITEM;
  e.contents=CONTENTS_ITEM;e.e_ThinkFunc=thinkF_G_FreeEntity;e.nextthink=level.time+30000;
 }
 if(catalog==Catalog("target_level_change")){e.model=G_NewString("@jar_level_change");e.message=G_NewString("hoth2");e.e_UseFunc=useF_target_level_change_use;}
 if(catalog==Catalog("holocron_force_heal")){e.item=&holoPickup;e.s.eType=ET_ITEM;e.count=1;}
 if(jar::portal::SurfaceClass(e.classname)||jar::portal::CameraClass(e.classname)){e.model=G_NewString(jar::portal::SurfaceClass(e.classname)?"@jar_portal_surface":"@jar_portal_camera");CHECK(G_PortalSet(e,jar::portal::Settings{}));}
 if(jar::crystalCrate::Class(e.classname)){e.takedamage=qtrue;e.health=80;e.model=G_NewString("@jar_crystal_crate");CHECK(G_CrystalCrateSet(e,jar::crystalCrate::Settings{}));}
 if(jar::doorCamera::Class(e.classname)){e.model=G_NewString("@jar_door_camera");CHECK(G_DoorCameraSet(e,jar::doorCamera::Settings{}));}
 if(jar::welder::Class(e.classname)){e.model=G_NewString("@jar_welder");CHECK(G_WelderSet(e,jar::welder::Settings{}));}
 if(jar::spotlight::Class(e.classname)){e.model=G_NewString("@jar_spotlight");CHECK(G_SpotlightSet(e,jar::spotlight::Settings{}));}
 if(jar::laserArm::Class(e.classname)){e.model=G_NewString("@jar_laser_arm");CHECK(G_LaserArmSet(e,jar::laserArm::Settings{}));}
 if(jar::sandboxLaser::Class(e.classname)){e.model=G_NewString(jar::sandboxLaser::model);CHECK(G_SandboxLaserInitialize(e,{}));}
 if(jar::weather::Class(e.classname)!=jar::weather::Invalid){e.model=G_NewString(jar::weather::ZoneClass(e.classname)?"@jar_box":"@jar_weather");if(jar::weather::ZoneClass(e.classname)){jar::volumeEntity::Initialize(e);e.contents=CONTENTS_TRIGGER;}jar::weather::Settings s;s.kind=jar::weather::Class(e.classname);CHECK(G_WeatherSet(e,s));}
 CHECK(Own(state,slot,catalog));e.targetname=G_NewString(va("jar_sb_%u",owned(slot)->id));
 const auto station=jar::sandboxStation::Class(e.classname);
 if(station!=jar::sandboxStation::None){
  e.e_UseFunc=station==jar::sandboxStation::Ammo?useF_ammo_power_converter_use:useF_shield_power_converter_use;
  e.count=75;CHECK(G_SandboxStationInitialize(e,1));
 }
 if(jar::sandboxBreakable::Class(e.classname)){
  e.health=40;e.radius=1;e.s.modelindex=5;
  CHECK(G_SandboxBreakableInitialize(e));
 }
 if(jar::sandboxUsable::Class(e.classname)){
  e.model=G_NewString("@jar_box");e.health=40;e.s.modelindex=5;e.e_DieFunc=dieF_func_usable_die;
  CHECK(G_SandboxUsableInitialize(e));
 }
 return &e;
}
gentity_t* G_JAR_SpawnSetupChild(gentity_t* root){
 auto* child=Make(Catalog("info_notnull"),root->currentOrigin,root->currentAngles);
 CHECK(SetOwnershipParent(state,*owned(child->s.number),owned(root->s.number)->id));
 return child;
}
gentity_t*spawn(unsigned catalog,const float*pos,const float*angles,int poseFrame=0,
 bool wallStartsOff=false,bool pastePortal=false,WorkshopSpawnFeedback feedback=WorkshopSpawnFeedback::Individual){
 if(++attempt==failAt)return nullptr;auto*e=Make(catalog,pos,angles);e->s.frame=poseFrame;const auto parent=owned(e->s.number)->id;
 if(feedback==WorkshopSpawnFeedback::Individual){rememberEdit(*e,true);list();}
 if(const auto* card=jar::catalog::Find(catalog))if(const auto* profile=jar::bspSection::Catalog(card->name)){
  e->classname=G_NewString("misc_bsp");e->model=G_NewString(profile->model);
  e->bmodel=qtrue;e->e_UseFunc=useF_target_teleporter_use;
 }
 if(jar::reference::Class(e->classname))CHECK(G_ReferenceSpawn(*e));
 if(catalog==JAR_SLIDING_DOOR_ID){e->model=G_NewString(JAR_SLIDING_DOOR_PORTABLE);CHECK(G_SandboxSlideSpawn(*e));}
 if(catalog==JAR_LIFT_ID){e->model=G_NewString(JAR_LIFT_PORTABLE);CHECK(G_SandboxSlideSpawn(*e));}
 if(catalog==JAR_BUTTON_ID){e->model=G_NewString(JAR_BUTTON_PORTABLE_MODEL);CHECK(G_ButtonSpawn(*e));}
 if(catalog==JAR_BOB_ID){e->model=G_NewString(JAR_BOB_PORTABLE);CHECK(G_BobbingSpawn(*e));}
 if(const auto angular=jar::sandboxAngular::Class(e->classname)){
  e->model=G_NewString(jar::sandboxAngular::model);
  CHECK(G_SandboxAngularInit(*e,jar::sandboxAngular::Defaults(angular)));
 }
 if(catalog==jar::logic::delayId){e->e_UseFunc=useF_Use_Target_Delay;e->wait=1;}
 if(catalog==jar::logic::counterId){e->e_UseFunc=useF_target_counter_use;e->count=e->max_health=2;}
 if(catalog==jar::logic::timerId){e->e_UseFunc=useF_func_timer_use;e->e_ThinkFunc=thinkF_func_timer_think;e->wait=1;}
 if(catalog==jar::logic::randomId)e->e_UseFunc=useF_target_random_use;
 if(catalog==jar::logic::relayId){e->e_UseFunc=useF_target_relay_use;e->wait=1000;}
 if(catalog==jar::logic::scoreId){e->e_UseFunc=useF_Use_Target_Score;e->count=1;}
 if(catalog==jar::logic::killId)e->e_UseFunc=useF_target_kill_use;
 if(catalog==jar::logic::giveId)e->e_UseFunc=useF_Use_Target_Give;
 if(catalog==jar::logic::enableId)e->e_UseFunc=useF_target_activate_use;
 if(catalog==jar::logic::disableId)e->e_UseFunc=useF_target_deactivate_use;
 if(catalog==jar::push::volumeId||catalog==jar::push::markerId){
  const auto kind=jar::push::Class(e->classname);
  e->model=G_NewString(kind==jar::push::Volume?jar::push::volumeModel:jar::push::markerModel);
  if(kind==jar::push::Volume){
   e->contents=CONTENTS_TRIGGER;e->e_TouchFunc=touchF_trigger_push_touch;
   VectorSet(e->pos1,-21,-21,0);VectorSet(e->pos2,21,21,22);CHECK(jar::volumeEntity::Refresh(*e));
  }else e->e_UseFunc=useF_Use_target_push;
  CHECK(G_PushSet(*e,{kind,600,1000}));
 }
 if(const auto kind=jar::teleport::Class(e->classname)){
  e->model=G_NewString(kind==jar::teleport::Volume?jar::teleport::volumeModel:jar::teleport::markerModel);
  if(kind==jar::teleport::Volume){
   e->contents=CONTENTS_TRIGGER;e->e_TouchFunc=touchF_trigger_teleporter_touch;
   VectorSet(e->pos1,-21,-21,0);VectorSet(e->pos2,21,21,22);CHECK(jar::volumeEntity::Refresh(*e));
  }else e->e_UseFunc=useF_target_teleporter_use;
  CHECK(G_TeleportSet(*e,{kind,500}));
 }
 if(const auto kind=jar::contactTrigger::Class(e->classname)){
  using namespace jar::contactTrigger;
  e->model=G_NewString(model);e->contents=CONTENTS_TRIGGER;
  e->e_UseFunc=kind==Hurt?useF_hurt_use:useF_Use_Multi;
  e->e_TouchFunc=kind==Hurt?touchF_hurt_touch:touchF_Touch_Multi;
  VectorSet(e->pos1,-21,-21,0);VectorSet(e->pos2,21,21,22);
  CHECK(jar::volumeEntity::Refresh(*e)&&G_ContactSettings(*e,{kind,0,kind==Once?0:1000,0,Actors,false,true,kind==Hurt?5:0}));
 }
 if(catalog==Catalog("target_print"))e->e_UseFunc=useF_Use_Target_Print;
 if(jar::speaker::Class(e->classname)){
  e->model=G_NewString(jar::speaker::model);
  CHECK(G_SandboxSpeakerInit(*e,{}));
 }
 if(const auto movement=G_MovementMarkerClass(e->classname)){
  e->model=G_NewString(JAR_MOVEMENT_PORTABLE);
  CHECK(G_MovementMarkerSet(*e,{movement,movement==jar::movement::Gravity?400:2,true}));
 }

 if(jar::rail::Class(e->classname)!=jar::rail::None){gentity_t*parts[4]{};const char*names[]={"","rail_track","rail_lane","rail_mover"};parts[jar::rail::Class(e->classname)]=e;for(int k=1;k<=3;++k)if(!parts[k]){parts[k]=Make(Catalog(names[k]),pos,angles);CHECK(SetOwnershipParent(state,*owned(parts[k]->s.number),parent));}parts[2]->target=G_NewString(parts[1]->targetname);parts[3]->target=G_NewString(parts[2]->targetname);}
 if(catalog==Catalog("misc_camera_focus")){auto*child=Make(Catalog("misc_camera_track"),pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));e->target2=G_NewString(child->targetname);child->target2=G_NewString(e->targetname);}
 if(catalog==Catalog("misc_camera_track")){vec3_t other;VectorCopy(pos,other);other[0]+=128;auto*a=Make(Catalog("path_corner"),pos,angles);auto*b=Make(Catalog("path_corner"),other,angles);CHECK(SetOwnershipParent(state,*owned(a->s.number),parent));CHECK(SetOwnershipParent(state,*owned(b->s.number),parent));e->target=G_NewString(a->targetname);a->target=G_NewString(b->targetname);}
 if(catalog==Catalog("func_train")){vec3_t other;VectorCopy(pos,other);other[0]+=64;auto*a=Make(Catalog("path_corner"),pos,angles);auto*b=Make(Catalog("path_corner"),other,angles);
  CHECK(SetOwnershipParent(state,*owned(a->s.number),parent));CHECK(SetOwnershipParent(state,*owned(b->s.number),parent));e->target=G_NewString(a->targetname);a->target=G_NewString(b->targetname);b->target=G_NewString(a->targetname);}
 if(!pastePortal&&(catalog==Catalog("misc_portal_surface")||catalog==Catalog("misc_portal_camera"))){const bool surface=catalog==Catalog("misc_portal_surface");auto*child=Make(Catalog(surface?"misc_portal_camera":"misc_portal_surface"),pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));auto*s=surface?e:child;auto*c=surface?child:e;s->target=G_NewString(c->targetname);s->owner=c;}
 if(catalog==Catalog("misc_spotlight")){auto*a=Make(Catalog("target_position"),pos,angles);auto*b=Make(Catalog("target_print"),pos,angles);CHECK(SetOwnershipParent(state,*owned(a->s.number),parent));CHECK(SetOwnershipParent(state,*owned(b->s.number),parent));e->target=G_NewString(a->targetname);e->target2=G_NewString(b->targetname);}
 if(catalog==Catalog("misc_crystal_crate")){auto*child=Make(Catalog("info_notnull"),pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));}
 if(catalog==Catalog("misc_model_jabba_cam"))for(int i=0;i<2;++i){auto*child=Make(Catalog("info_notnull"),pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));}
 if(catalog==Catalog("misc_laser_arm"))CHECK(G_LaserArmSpawn(e));
 if(regenerateRackStock&&catalog==Catalog("misc_model_gun_rack"))for(int i=0;i<3;++i){
  auto*child=Make(2501,pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));
  child->linked=qtrue;child->flags|=FL_FORCE_PULLABLE_ONLY;
  child->s.pos.trType=TR_STATIONARY;child->e_ThinkFunc=thinkF_NULL;child->nextthink=0;
 }
 // The native setup boundary is covered by entity_setups/native_objects.
 if(catalog==Catalog("misc_maglock")){auto*door=Make(Catalog("func_door"),pos,angles);CHECK(SetOwnershipParent(state,*owned(door->s.number),parent));
  e->activator=door;e->health=10;e->takedamage=qtrue;door->lockCount=1;door->svFlags|=SVF_INACTIVE;}
 if(catalog==Catalog("trigger_bidirectional")){
  e->contents=CONTENTS_TRIGGER;e->model=G_NewString("@jar_box");jar::volumeEntity::Initialize(*e);e->wait=1;e->count=0;
  auto*a=Make(Catalog("target_print"),pos,angles);auto*b=Make(Catalog("target_print"),pos,angles);CHECK(SetOwnershipParent(state,*owned(a->s.number),parent));CHECK(SetOwnershipParent(state,*owned(b->s.number),parent));
  e->target=G_NewString(a->targetname);e->paintarget=G_NewString(b->targetname);
 }
 if(catalog==Catalog("trigger_shipboundary")){
  e->contents=CONTENTS_TRIGGER;e->model=G_NewString("@jar_box");jar::volumeEntity::Initialize(*e);e->count=3000;e->spawnflags=1;
  auto*h=Make(Catalog("target_position"),pos,angles);CHECK(SetOwnershipParent(state,*owned(h->s.number),parent));e->target=G_NewString(h->targetname);
 }
 if(catalog==Catalog("trigger_space")){e->contents=CONTENTS_TRIGGER;e->model=G_NewString("@jar_box");jar::volumeEntity::Initialize(*e);}
 if(catalog==Catalog("trigger_always")){
  e->contents=CONTENTS_ITEM;e->model=G_NewString("@jar_always");
  e->delay=JAR_ALWAYS_DEFAULT_MS;e->e_UseFunc=useF_portable_always_use;G_AlwaysSchedule(*e,e->delay);
 }
 if(catalog==Catalog("trigger_entdist")){
  e->contents=CONTENTS_ITEM;e->model=G_NewString("@jar_entdist");e->e_UseFunc=useF_trigger_entdist_use;
  e->count=JAR_DISTANCE_DEFAULT;e->spawnflags=JAR_DISTANCE_PLAYER;
  auto*inside=Make(Catalog("target_print"),pos,angles);auto*outside=Make(Catalog("target_print"),pos,angles);
  CHECK(SetOwnershipParent(state,*owned(inside->s.number),parent));CHECK(SetOwnershipParent(state,*owned(outside->s.number),parent));
  e->target=G_NewString(inside->targetname);e->target2=G_NewString(outside->targetname);
 }
 if(catalog==JAR_WALL_CATALOG_ID){
  e->model=G_NewString(JAR_WALL_PORTABLE_MODEL);e->e_UseFunc=useF_use_wall;
  CHECK(G_WallSetVisible(*e,!wallStartsOff));
 }
 if(catalog==Catalog("trigger_location")){
  e->contents=CONTENTS_ITEM;e->model=G_NewString("@jar_box");
  jar::volumeEntity::InitializeLocation(*e);CHECK(jar::volumeEntity::Refresh(*e));
 }
 if(catalog==Catalog("target_interest")){e->model=G_NewString("@jar_interest");auto*child=Make(Catalog("target_print"),pos,angles);CHECK(SetOwnershipParent(state,*owned(child->s.number),parent));e->target=G_NewString(child->targetname);}
 return e;
}
#include "game/g_mover_motion.h"
bool G_JAR_MoverTranslate(gentity_t&,const vec3_t,vec3_t actual){VectorClear(actual);return false;}
bool G_JAR_MoverRotate(gentity_t&,const vec3_t){return false;}
bool G_JAR_MoverRotateAt(gentity_t&,const vec3_t,const vec3_t,const vec3_t){return false;}
#include "game/g_mover_motion.inc"
static bool ropeAdmissionBlocked;static int ropeAdmissions;
bool ropeAdmission(jar::constraint::Joint&j,float* =nullptr){
 if(j.kind!=jar::constraint::Rope)return true;
 CHECK(!j.path.count);++ropeAdmissions;return !ropeAdmissionBlocked;
}
bool resizeRope(jar::constraint::Joint&,float){throw std::runtime_error("Unexpected rope property edit in Duplicate fixture");}
#include "game/g_workshop_placement.h"
bool G_SubBSPMembers(const gentity_t* root,std::vector<gentity_t*>& members){
 members.clear();return root&&jar::bspSection::Portable(*root);
}
const gentity_t* G_BSPSectionPickupRoot(const gentity_t&){return nullptr;}
static bool runItemPhysics=false;
static int itemThinks=0;
static bool runTrailPhysics=false;
static int trailPulses=0,trailImpacts=0,trailEffects=0;
static vec3_t trailDamagePoint;
int BMS_END=2;
sfxHandle_t CAS_GetBModelSound(const char*,int stage){CHECK(runTrailPhysics&&stage==BMS_END);return 7;}
void G_RadiusDamage(const vec3_t point,gentity_t* attacker,float damage,float,gentity_t* ignore,int method){
 CHECK(runTrailPhysics&&attacker==ignore&&method==MOD_EXPLOSIVE_SPLASH);
 VectorCopy(point,trailDamagePoint);
 if(damage==30)++trailImpacts;else {CHECK(damage==12);++trailPulses;}
}
void G_PlayEffect(int id,const vec3_t,const vec3_t direction){CHECK(runTrailPhysics&&id==4&&direction[0]==1);++trailEffects;}
void G_PlayEffect(const char* name,const vec3_t,const vec3_t normal){CHECK(runTrailPhysics&&!strcmp(name,"impact")&&normal[0]==-1);++trailEffects;}
bool G_JAR_WorkshopOutsideMapPoint(const vec3_t point){return outsideWorkshopMap(point,vec3_origin,vec3_origin);}
void EvaluateTrajectory(const trajectory_t* trajectory,int time,vec3_t result){
 if((runItemPhysics||runTrailPhysics)&&trajectory->trType==TR_GRAVITY){
  const float elapsed=(time-trajectory->trTime)*.001f;
  VectorMA(trajectory->trBase,elapsed,trajectory->trDelta,result);
  result[2]-=.5f*g_gravity->value*elapsed*elapsed;
 }else if(runTrailPhysics&&trajectory->trType==TR_LINEAR){
  VectorMA(trajectory->trBase,(time-trajectory->trTime)*.001f,trajectory->trDelta,result);
 }else{CHECK(trajectory->trType==TR_STATIONARY);VectorCopy(trajectory->trBase,result);}
}
#include "game/g_fx_trail_motion.inc"
void G_RunThink(gentity_t*){CHECK(runItemPhysics);++itemThinks;}
void G_BounceItem(gentity_t*,trace_t*){throw std::runtime_error("Unexpected blocked fall in rack item fixture");}
bool G_JAR_WorkshopRackPickupSuspended(const gentity_t& item){return rackStockAttached(item);}
bool G_JAR_WorkshopMoveRackStock(gentity_t& rack,const vec3_t origin,const vec3_t angles){return rackStockMove(rack,origin,angles);}
#include "game/g_item_motion.inc"
#include "game/g_object_pose.inc"
#include "game/g_workshop_duplicate.inc"
#include "game/g_workshop_rope_trace.inc"
static std::map<uint32_t,int> nativeJointHistory,ropeStates;
static std::set<int> assemblyArrivals;
static const gentity_t* scriptedMover;
static gentity_t* sectionMover;
static gentity_t* placedSection;
static std::vector<gentity_t*> placedSectionChildren;
qboolean Q3_TaskIDPending(gentity_t* e,taskID_t){return e==scriptedMover?qtrue:qfalse;}
gentity_t* G_SubBSPOwner(const gentity_t* e){
 if(placedSection&&(e==placedSection||std::find(placedSectionChildren.begin(),placedSectionChildren.end(),e)!=placedSectionChildren.end()))
  return placedSection;
 return e==sectionMover?&g_entities[1]:nullptr;
}
bool G_BSPSectionTeleportMembers(const gentity_t& root,std::vector<gentity_t*>& members){
 if(&root!=placedSection||!VectorCompare(root.pos1,root.currentOrigin))return false;
 members=placedSectionChildren;return true;
}
static gentity_t* usedBinaryMover;
void Use_BinaryMover(gentity_t* mover,gentity_t*,gentity_t*){usedBinaryMover=mover;}
#include "game/g_mover_touch_doortrigger.inc"
#include "game/g_mover_touch_platcentertrigger.inc"
static int assemblyClock=-1,assemblyPushes=0,assemblyActorPasses=0,assemblyImpacts=0;
static bool assemblyClockReady=true;
uint64_t G_EntityLifetime(const gentity_t* e){
 return e&&e->inuse?(uint64_t(entityGeneration[e->s.number])<<32)+e->s.number+1:0;
}
bool G_ContactClockReady(const gentity_t&,const gentity_t& actor,int){assemblyClock=actor.s.number;return assemblyClockReady;}
void G_TeleportRefreshArrival(const gentity_t&,const vec3_t){}
bool G_TeleportArrivalPending(const gentity_t& e){return assemblyArrivals.count(e.s.number)!=0;}
void G_TeleportMarkArrival(const gentity_t& e){assemblyArrivals.insert(e.s.number);}
static jar::physics::Vec pv(const vec3_t v){return {v[0],v[1],v[2]};}
static jar::physics::Pose physicsPose(const gentity_t& e){return {pv(e.currentOrigin),pv(e.currentAngles),pv(e.s.pos.trDelta),pv(e.s.apos.trDelta)};}
static bool physicsEligible(const Prop& p,int frame){return p.rigid&&frame!=7;}
static int physicsFrozenContents(const gentity_t& e){return e.contents;}
static void physicsActors(gentity_t&,const vec3_t,const vec3_t,const jar::physics::Pose&,float,bool){++assemblyActorPasses;}
static void pushPropContacts(gentity_t&,const vec3_t,bool){++assemblyPushes;}
static void impactSound(unsigned,float){++assemblyImpacts;}
static uint32_t attachmentKey(const gentity_t& e){
 const auto* o=owned(e.s.number);
 return o&&matches(*o)?o->id:jar::constraint::NativeId(e.s.number);
}
static gentity_t* attachmentEntity(uint32_t id){
 if(jar::constraint::Native(id)){
  const auto slot=jar::constraint::NativeEntity(id);
  if(slot==ENTITYNUM_WORLD)return &g_entities[slot];
  return slot<ENTITYNUM_WORLD&&editable(g_entities[slot])?&g_entities[slot]:nullptr;
 }
 const auto* o=OwnedId(state,id);
 return o&&matches(*o)?&g_entities[o->entity]:nullptr;
}
static bool attachmentMovable(const gentity_t& e){return !occupied(e)&&e.next_roff_time<=level.time;}
void G_KillBox(gentity_t*){throw std::runtime_error("Assembly transport must not telefrag");}
void PlayerStateToEntityState(playerState_t* ps,entityState_t* s){
 VectorCopy(ps->origin,s->pos.trBase);VectorCopy(ps->velocity,s->pos.trDelta);s->eFlags=ps->eFlags;
}
#include "game/g_native_teleport.inc"
#include "game/g_workshop_assembly_teleport.inc"
static gentity_t* moverCrossingSource;
std::vector<TeleportCrossing> G_TeleportCrossings(gentity_t& object,const vec3_t previous,const vec3_t end,
 bool clipped,bool assembly){
 CHECK(moverCrossingSource&&!clipped&&assembly&&G_TeleportObjectKind(object)==jar::teleport::Mover);
 CHECK(!VectorCompare(previous,end));
 return {{.5f,moverCrossingSource,{}}};
}
#include "game/g_workshop_assembly_crossings.inc"
void CheckAssemblyTeleport();
void CheckMixedAssemblyTeleport();
void CheckVehicleAssemblyTeleport();
void CheckMoverAssemblyTeleport();
void CheckBrushAssemblyTeleport();
void CheckBinaryAssemblyTeleport();
void CheckSectionAssemblyTeleport();
void CheckSectionDuplicate();
void CheckWorldAnchorAssemblyTeleport();
void CheckWorldAnchorDuplicate();
void CheckPropertiesAndPaste();
void CheckMountedChargeDuplicate();
void CheckStandaloneChargeDuplicate();
void CheckDeployedShieldDuplicate();
void CheckConvexPasteClearance();
int main(){return RunTests("assembly clipboard, property cloning, native helpers, transactional paste and grouped undo",[]{
 ScriptFiles();gi.FS_ReadFile=[](const char*path,void**out)->long{CHECK(!out);return !strcmp(path,"maps/t1_surprise.bsp")||!strcmp(path,"maps/hoth2.bsp")?100:-1;};
 gi.cvar_set=[](const char*k,const char*v){vars[k]=v;};gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t mins,const vec3_t,const vec3_t end,int,int mask,EG2_Collision mode,int){
  if(mode==G2_BRUSH_ONLY)CHECK(mask==MASK_SOLID);
  *t={};t->fraction=mins?1:.5f;t->startsolid=blocked&&mins&&mode==G2_BRUSH_ONLY?qtrue:qfalse;t->plane.normal[2]=1;
  if(mins)VectorCopy(end,t->endpos);else VectorSet(t->endpos,200,0,0);
 };
 vec3_t a={0,0,100},b={32,0,100},angles={10,20,30};auto*x=Make(1001,a,angles);auto*y=Make(1002,b,angles);const auto xId=owned(x->s.number)->id,yId=owned(y->s.number)->id;
 owned(y->s.number)->frozen=0;y->s.frame=7;
 CHECK(physicsWorld.Start({{-1024,-1024,-1000},{1024,-1024,-1000},{1024,1024,-1000},{-1024,-1024,-1000},{1024,1024,-1000},{-1024,1024,-1000}}));
 auto body=[&](gentity_t&e){const auto*o=owned(e.s.number);jar::physics::Pose p;p.position={e.currentOrigin[0],e.currentOrigin[1],e.currentOrigin[2]};p.angles={e.currentAngles[0],e.currentAngles[1],e.currentAngles[2]};CHECK(physicsWorld.Prop(e.s.number,o->id,{10,10,10},{},p,o->frozen!=0));};
 body(*x);body(*y);
 const vec3_t launch={0,120,85};jar::physics::Pose launched;
 G_PushPropMotion(physicsWorld,*y,launch,true);
 CHECK(physicsWorld.Read(y->s.number,launched)&&std::fabs(launched.linear.y-120)<.1f&&std::fabs(launched.linear.z-85)<.1f&&y->s.pos.trDelta[1]==120);
 G_PushPropMotion(physicsWorld,*x,launch,true);CHECK(!VectorLengthSquared(x->s.pos.trDelta));
 const vec3_t exit={80,10,120};
 CHECK(!G_TeleportPropMotion(physicsWorld,*x,exit,true,true)&&VectorCompare(x->currentOrigin,a));
 CHECK(G_TeleportPropMotion(physicsWorld,*y,exit,true,true));
 CHECK(physicsWorld.Read(y->s.number,launched)&&std::fabs(launched.position.x-80)<.01f&&
       std::fabs(launched.linear.y-120)<.1f&&(y->s.eFlags&EF_TELEPORT_BIT));
 CHECK(G_TeleportPropMotion(physicsWorld,*y,b,false,true));
 CHECK(physicsWorld.Read(y->s.number,launched)&&std::fabs(launched.position.x-32)<.01f&&
       std::fabs(launched.linear.y)<.01f&&!VectorLengthSquared(y->s.pos.trDelta));
 gentity_t loose{};VectorSet(loose.s.pos.trDelta,5,6,7);VectorSet(loose.s.apos.trDelta,1,2,3);
 CHECK(G_TeleportPropMotion(physicsWorld,loose,exit,true,false)&&loose.linked&&loose.s.pos.trType==TR_INTERPOLATE&&
       loose.s.pos.trDelta[2]==7&&loose.s.apos.trDelta[2]==3);
 jar::constraint::Joint weld;weld.kind=jar::constraint::Weld;weld.first=xId;weld.second=yId;CHECK(physicsWorld.JointFrames(x->s.number,y->s.number,{16,0,100},{0,1,0},weld.a,weld.b));CHECK(AddJoint(state,weld));
 jar::constraint::Joint thruster;thruster.kind=jar::constraint::Thruster;thruster.first=thruster.second=yId;thruster.force=12345;thruster.enabled=0;CHECK(AddJoint(state,thruster));
 owned(y->s.number)->physics={225,true};
 duplicateTool(*y);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==2); // newest selected root first
 CHECK(!duplicateSnapshot.objects[0].frozen&&duplicateSnapshot.objects[1].frozen);
 const auto snapshot=duplicateSnapshot;
 const auto initialListCalls=listCalls;
 duplicatePaste();
 CHECK(listCalls==initialListCalls+1&&state.ownedCount==4&&state.jointCount==4&&edits.size()==1&&edits.back().group.size()==2);
 auto*newRoot=owned(selected);CHECK(newRoot&&newRoot->id!=yId&&!newRoot->frozen);auto&copy=g_entities[selected];CHECK(copy.currentAngles[2]==30&&copy.s.frame==7&&y->s.frame==7);
 CHECK(newRoot->physics.mass==225&&newRoot->physics.noGravity);
 const auto newThruster=state.joints[3];CHECK(newThruster.first==newRoot->id&&newThruster.second==newRoot->id&&newThruster.force==12345&&!newThruster.enabled);
 CHECK(!memcmp(&state.joints[2].a,&state.joints[0].a,sizeof(weld.a))&&!memcmp(&state.joints[2].b,&state.joints[0].b,sizeof(weld.b))&&state.joints[2].first!=xId);
 for(unsigned i=2;i<state.ownedCount;++i)body(g_entities[state.owned[i].entity]);
 for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];CHECK(physicsWorld.Joint(j,OwnedId(state,j.first)->entity,OwnedId(state,j.second)->entity));}
 for(int i=0;i<120;++i)physicsWorld.Step(1.f/120,800);
 jar::physics::Pose oldA,oldB,newA,newB;const auto&copiedWeld=state.joints[2];
 CHECK(physicsWorld.Read(x->s.number,oldA)&&physicsWorld.Read(y->s.number,oldB)&&physicsWorld.Read(OwnedId(state,copiedWeld.first)->entity,newA)&&physicsWorld.Read(OwnedId(state,copiedWeld.second)->entity,newB));
 CHECK(std::fabs((oldB.position.x-oldA.position.x)-(newB.position.x-newA.position.x))<.05f&&std::fabs((oldB.position.z-oldA.position.z)-(newB.position.z-newA.position.z))<.05f&&std::fabs(oldB.angles.y-newB.angles.y)<.05f);physicsWorld.Stop();
 State decoded;const auto bytes=Encode(state);CHECK(!bytes.empty()&&Decode(bytes.data(),bytes.size(),decoded,ENTITYNUM_WORLD)&&decoded.jointCount==4&&decoded.ownedCount==4);
 const int feedback=undoFeedbackCount;
 busy=selected;undoEdit();CHECK(state.ownedCount==4&&state.jointCount==4&&undoFeedbackCount==feedback);
 busy=noEntity;undoEdit();CHECK(state.ownedCount==2&&state.jointCount==2&&edits.empty()&&undoFeedbackCount==feedback+1);
 // A copied member can overlap an actor before publishing. Rollback still
 // removes it, keeps the earlier spawn record, and refreshes the menu once.
 rememberEdit(*x,true);
 const auto previousListCalls=listCalls;
 const auto incomplete=nextSlot;
 busy=incomplete;selected=y->s.number;
 failAt=attempt+2;
 duplicatePaste();
 CHECK(!g_entities[incomplete].inuse&&state.ownedCount==2&&state.jointCount==2);
 CHECK(edits.size()==1&&edits.front().spawn&&edits.front().entity==x->s.number);
 CHECK(listCalls==previousListCalls+1&&duplicateSnapshot.objects.size()==2&&selected==unsigned(y->s.number));
 CHECK(!actorRecoverySuspended);
 busy=noEntity;failAt=0;edits.clear();
 blocked=true;const auto before=attempt;duplicatePaste();CHECK(attempt==before&&state.ownedCount==2);blocked=false;
 const auto oldCount=state.jointCount;state.jointCount=jar::constraint::limit;duplicatePaste();CHECK(attempt==before&&state.ownedCount==2);state.jointCount=oldCount;
 duplicatePaste();CHECK(state.ownedCount==4);const auto group=edits.back().group;
 remove(group[0].entity,true);CHECK(!edits.empty());
 // Reuse the removed slot for another object. Assembly undo must leave it alone.
 const auto endSlot=nextSlot;nextSlot=group[0].entity;auto*unrelated=Make(1001,a);nextSlot=endSlot;const auto unrelatedId=owned(unrelated->s.number)->id;
 undoEdit();CHECK(state.ownedCount==3&&owned(unrelated->s.number)->id==unrelatedId&&g_entities[x->s.number].inuse&&g_entities[y->s.number].inuse);
 remove(unrelated->s.number,true);
 // A copied snapshot remains usable after its original entities disappear.
 remove(x->s.number,true);remove(y->s.number,true);CHECK(!state.ownedCount&&!state.jointCount);duplicatePaste();CHECK(state.ownedCount==2&&state.jointCount==2);undoEdit();CHECK(!state.ownedCount);
 const int emptyFeedback=undoFeedbackCount;
 undoEdit();CHECK(undoFeedbackCount==emptyFeedback);
 // Snapshot validation never accepts dangling links or non-finite positions.
 auto invalid=snapshot;invalid.objects[0].offset[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!jar::duplicate::Valid(invalid));
 invalid=snapshot;invalid.joints[0].second=999;CHECK(!jar::duplicate::Valid(invalid));
 CheckPropertiesAndPaste();
 CheckAssemblyTeleport();
 CheckMixedAssemblyTeleport();
 CheckVehicleAssemblyTeleport();
 CheckMoverAssemblyTeleport();
 CheckBrushAssemblyTeleport();
 CheckBinaryAssemblyTeleport();
 CheckSectionAssemblyTeleport();
 CheckSectionDuplicate();
 CheckWorldAnchorAssemblyTeleport();
 CheckWorldAnchorDuplicate();
 CheckMountedChargeDuplicate();
 CheckStandaloneChargeDuplicate();
 CheckDeployedShieldDuplicate();
 CheckConvexPasteClearance();
 });}
void CheckConvexPasteClearance(){
 using namespace jar::physics;
 auto shape=std::make_shared<PropGeometry>();
 for(float x:{-12.f,12.f})for(float y:{-2.f,2.f})for(float z:{-6.f,6.f})shape->Add({x,y,z});
 sceneryFixtureGeometry=shape;
 static NativePropHull corner,center;
 PropGeometry cornerBrush,centerBrush;
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){
  cornerBrush.Add({8+x,8+y,z});
  centerBrush.Add({x,8+y,z});
 }
 CHECK(corner.Build(cornerBrush,{})&&center.Build(centerBrush,{}));
 static const NativePropHull* obstacle=&center;
 static int convexQueries=0;
 static bool slopeAim=false,stepScene=false,ceiling=false;
 const auto previousTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t,const vec3_t mins,const vec3_t,const vec3_t end,int,int,EG2_Collision mode,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  if(!mins){
   hit->fraction=.5f;VectorSet(hit->endpos,200,0,0);
   if(slopeAim)VectorSet(hit->plane.normal,.6f,0,.8f);else VectorSet(hit->plane.normal,0,0,1);
  }
  else if(mode==G2_BRUSH_ONLY)hit->startsolid=qtrue;
 };
 gi.JAR_TraceSceneryHull=[](trace_t* hit,const vec3_t from,const vec3_t to,const vec3_t lo,const vec3_t hi,
    int pass,int mask,const JarSweepHull& shape){
  CHECK(mask==MASK_SOLID);
  if(pass!=ENTITYNUM_NONE)CHECK(g_entities[pass].s.frame==5&&!g_entities[pass].linked);
  CHECK(std::fabs(lo[0]+2)<.01f&&std::fabs(hi[0]-2)<.01f&&std::fabs(hi[1]-12)<.01f);
  CHECK(obstacle);
  *hit={};hit->fraction=1;VectorCopy(to,hit->endpos);
  if(stepScene){
   const float floor=to[0]>215?26.f:15.f;
   CHECK(from[0]==to[0]&&from[1]==to[1]);
   if(from[2]<floor||(ceiling&&from[2]>64)){hit->startsolid=hit->allsolid=qtrue;hit->fraction=0;}
   else if(to[2]<floor){hit->fraction=(from[2]-floor)/(from[2]-to[2]);hit->endpos[2]=floor;hit->plane.normal[2]=1;}
  }else if(obstacle->TraceHull({},{},shape).startSolid){hit->startsolid=hit->allsolid=qtrue;hit->fraction=0;}
  ++convexQueries;
 };
 const vec3_t origin={17,18,32},angles={0,90,0};
 auto* original=Make(1001,origin,angles);original->s.frame=5;
 edits.clear();duplicateTool(*original);
 const vec3_t slope={.6f,0,.8f},up={0,0,1};
 auto mixed=duplicateSnapshot;
 auto peer=mixed.objects.front();peer.catalog=3001;peer.poseFrame=0;VectorSet(peer.offset,-10,0,16);
 mixed.objects.push_back(peer);
 auto volume=peer;volume.triggerVolume=true;VectorSet(volume.offset,-250,0,-250);
 mixed.objects.push_back(volume);
 CHECK(std::fabs(duplicateSurfaceOffset(mixed,slope,100)-8.2f)<.001f);
 jar::chargeCopy::State mounted;
 VectorSet(mounted.origin,0,20,0);
 mixed.objects.front().charges.push_back(mounted);
 CHECK(std::fabs(duplicateSurfaceOffset(mixed,slope,100)-27)<.001f);
 mixed.objects={volume};CHECK(duplicateSurfaceOffset(mixed,slope,42)==42);
 auto lift=peer;lift.catalog=JAR_LIFT_ID;lift.hasSlidingDoor=true;lift.slidingDoor.extension=24000;
 VectorSet(lift.offset,0,0,8);mixed.objects={lift};CHECK(duplicateSurfaceOffset(mixed,up,42)==27);
 auto bobber=peer;bobber.hasBobbing=true;bobber.bobbing.amplitude=24;bobber.bobbing.phase=0;
 VectorSet(bobber.offset,0,0,1);mixed.objects={bobber};CHECK(duplicateSurfaceOffset(mixed,up,42)==34);
 const auto count=state.ownedCount;
 const auto before=attempt;
 duplicatePaste();
 CHECK(convexQueries==2&&attempt==before&&state.ownedCount==count&&edits.empty());
 obstacle=&corner;
 duplicatePaste();
 CHECK(convexQueries==4&&attempt==before+1&&state.ownedCount==count+1&&edits.size()==1);
 CHECK(g_entities[selected].s.frame==5&&g_entities[selected].currentAngles[YAW]==90);
 undoEdit();CHECK(state.ownedCount==count&&original->inuse&&original->s.frame==5);
 slopeAim=true;
 duplicatePaste();
 CHECK(convexQueries==6&&state.ownedCount==count+1&&edits.size()==1);
 const auto& sloped=g_entities[selected];
 CHECK(std::fabs(sloped.currentOrigin[0]-204.2f)<.001f&&std::fabs(sloped.currentOrigin[2]-5.6f)<.001f);
 undoEdit();CHECK(state.ownedCount==count&&original->inuse);
 slopeAim=false;stepScene=true;
 const vec3_t secondOrigin={49,18,34};
 auto* second=Make(1002,secondOrigin,angles);second->s.frame=5;
 jar::constraint::Joint weld;weld.kind=jar::constraint::Weld;
 weld.first=owned(original->s.number)->id;weld.second=owned(second->s.number)->id;
 CHECK(AddJoint(state,weld));
 duplicateTool(*original);CHECK(duplicateSnapshot.objects.size()==2);
 const auto beforeStep=attempt;
 ceiling=true;duplicatePaste();
 CHECK(attempt==beforeStep&&state.ownedCount==count+1&&edits.empty());
 ceiling=false;duplicatePaste();
 CHECK(attempt==beforeStep+2&&state.ownedCount==count+3&&edits.size()==1&&edits.back().group.size()==2);
 const auto& group=edits.back().group;
 const auto& firstCopy=g_entities[group[0].entity];const auto& secondCopy=g_entities[group[1].entity];
 CHECK(std::fabs(firstCopy.currentOrigin[2]-25)<.001f&&std::fabs(secondCopy.currentOrigin[2]-27)<.001f);
 CHECK(secondCopy.currentOrigin[0]-firstCopy.currentOrigin[0]==32&&firstCopy.s.frame==5&&secondCopy.s.frame==5);
 undoEdit();CHECK(state.ownedCount==count+1&&original->inuse&&second->inuse);
 CHECK(remove(second->s.number,true));
 CHECK(remove(original->s.number,true));duplicateClear();
 gi.JAR_TraceSceneryHull=nullptr;gi.trace=previousTrace;sceneryFixtureGeometry.reset();
}
void CheckDeployedShieldDuplicate(){
 const auto previousTrace=gi.trace;
 static bool shieldBlocked;
 gi.trace=[](trace_t* hit,const vec3_t,const vec3_t mins,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_NONE;hit->plane.normal[2]=1;
  VectorCopy(end,hit->endpos);
  if(!mins){hit->fraction=.5f;VectorSet(hit->endpos,200,0,0);}
  else if(shieldBlocked&&pass>0&&mask==(MASK_SOLID|CONTENTS_BODY)&&mode==G2_NOCOLLIDE&&G_PortableShield(g_entities[pass]))hit->startsolid=qtrue;
 };
 const vec3_t origin={0,0,120},angles={0,25,0};
 auto* prop=Make(1001,origin);
 const auto propId=owned(prop->s.number)->id;
 jar::shieldCopy::State live;
 VectorSet(live.mins,-4,-20,0);VectorSet(live.maxs,4,90,80);
 live.health=170;live.capacity=250;live.remaining=5000;live.nextCheck=70;live.damageFlash=120;live.raised=true;
 level.time=5000;
 const vec3_t shieldOrigin={36,0,120};
 auto* shield=G_PortableShieldSpawnCopy(live,shieldOrigin,angles);
 CHECK(shield&&shield->s.loopSound==40&&!shield->linked);gi.linkentity(shield);
 const unsigned native=jar::constraint::NativeId(shield->s.number);
 jar::constraint::Joint rope;rope.kind=jar::constraint::Rope;rope.first=propId;rope.second=native;rope.length=62;
 CHECK(AddJoint(state,rope));
 auto& inventory=g_entities[0].client->ps.inventory[INV_PORTABLE_SHIELD];inventory=2;
 const auto oldOwned=state.ownedCount,oldJoints=state.jointCount;
 duplicateTool(*prop);
 CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==1);
 CHECK(duplicateSnapshot.objects[1].source==native&&duplicateSnapshot.objects[1].hasDeployedShield);
 level.time=6000;
 shieldBlocked=true;duplicatePaste();
 CHECK(state.ownedCount==oldOwned&&state.jointCount==oldJoints&&inventory==2);
 CHECK(shield->linked&&shield->count==1&&shield->health==170&&shield->delay==10000);
 shieldBlocked=false;duplicatePaste();
 CHECK(state.ownedCount==oldOwned+2&&state.jointCount==oldJoints+1&&inventory==2);
 const auto group=edits.back().group;
 CHECK(group.size()==2);
 auto& pasted=g_entities[group.back().entity];
 CHECK(G_PortableShield(pasted)&&pasted.linked&&pasted.count&&pasted.takedamage&&pasted.s.loopSound==40);
 CHECK(pasted.health==170&&pasted.max_health==250&&pasted.delay==11000&&pasted.painDebounceTime==6120);
 CHECK(pasted.nextthink==6070&&pasted.currentOrigin[0]==236&&pasted.pos1[1]==-20&&pasted.pos2[1]==90);
 CHECK(state.joints[oldJoints].first==owned(group[0].entity)->id&&state.joints[oldJoints].second==owned(group[1].entity)->id);
 duplicateTool(pasted);
 CHECK(duplicateSnapshot.objects[0].hasDeployedShield&&duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==1);
 undoEdit();
 CHECK(state.ownedCount==oldOwned&&state.jointCount==oldJoints&&!pasted.inuse&&shield->inuse);
 g_entities[0].client->playerTeam=TEAM_PLAYER;g_entities[0].health=100;
 portable_shield_touch(shield,&g_entities[0],nullptr);
 CHECK(!shield->count);duplicateTool(*shield);
 CHECK(duplicateSnapshot.objects[0].hasDeployedShield&&duplicateSnapshot.objects[0].deployedShield.rearm==200);
 CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==1);
 level.time=6100;duplicatePaste();
 auto& lowered=g_entities[edits.back().group.front().entity];
 CHECK(G_PortableShield(lowered)&&!lowered.count&&!lowered.takedamage&&lowered.attackDebounceTime==6300);
 level.time=6300;portable_shield_think(&lowered);CHECK(lowered.count&&lowered.takedamage);
 undoEdit();CHECK(!lowered.inuse&&shield->inuse&&!shield->count);
 level.time=10000;portable_shield_think(shield);CHECK(shield->s.eFlags&EF_NODRAW);
 const auto previousRemaining=duplicateSnapshot.objects[0].deployedShield.remaining;
 duplicateTool(*shield);CHECK(duplicateSnapshot.objects[0].deployedShield.remaining==previousRemaining);
 G_FreeEntity(shield);remove(prop->s.number,true);duplicateClear();gi.trace=previousTrace;
}
void CheckMountedChargeDuplicate(){
 const auto previousTrace=gi.trace;
 static int placements,blockPlacement,beamQueries;
 static bool chargeShelf=false;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_NONE;hit->plane.normal[2]=1;
  VectorCopy(end,hit->endpos);
  if(!mins){hit->fraction=.5f;VectorSet(hit->endpos,200,0,0);}
  else if(chargeShelf&&mode==G2_BRUSH_ONLY&&pass==ENTITYNUM_NONE&&end[2]<15&&mins[0]==-10){
   if(start[2]<15){hit->startsolid=hit->allsolid=qtrue;hit->fraction=0;}
   else {hit->fraction=(start[2]-15)/(start[2]-end[2]);hit->endpos[2]=15;hit->plane.normal[2]=1;}
  }else if(mask==MASK_SHOT&&mode==G2_NOCOLLIDE){
   CHECK(VectorCompare(start,end));
   if(++placements==blockPlacement)hit->startsolid=qtrue;
  }else if(mask==MASK_SHOT&&mode==G2_RETURNONHIT){
   ++beamQueries;
   const auto* support=G_SandboxChargeSupport(g_entities[pass]);
   CHECK(support&&!support->linked&&mins[0]==-4&&maxs[0]==4);
  }
 };
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&,const char* model,int index,qhandle_t,qhandle_t,int,int){
  CHECK(!strcmp(model,"charge_fixture.glm")&&index==24);return 0;
 };
 Q_strncpyz(weaponData[WP_DET_PACK].missileMdl,"charge_fixture.glm",sizeof(weaponData[WP_DET_PACK].missileMdl));
 Q_strncpyz(weaponData[WP_TRIP_MINE].missileMdl,"charge_fixture.glm",sizeof(weaponData[WP_TRIP_MINE].missileMdl));
 const vec3_t origin={0,0,110},angles={0,90,0};
 auto* prop=Make(1001,origin,angles);
 auto& player=g_entities[0];player.inuse=qtrue;player.client=&clients[0];player.client->ps.eFlags=0;
 level.time=3000;
 const auto plant=[&](int weapon,int think,const vec3_t position)->gentity_t&{
  auto& charge=*G_TrySpawn();
  charge.s.eType=ET_GENERAL;charge.s.weapon=weapon;charge.s.eFlags=EF_MISSILE_STICK;
  charge.s.pos.trType=TR_STATIONARY;charge.e_DieFunc=dieF_WP_ExplosiveDie;
  charge.classname=weapon==WP_DET_PACK?"detpack":"tripmine";
  charge.e_ThinkFunc=static_cast<thinkFunc_t>(think);charge.nextthink=think==thinkF_NULL?-1:level.time+250;
  charge.health=weapon==WP_DET_PACK?25:15;charge.takedamage=qtrue;charge.splashDamage=140;charge.splashRadius=96;
  charge.activator=prop;
  G_SetOrigin(&charge,position);G_SetAngles(&charge,angles);
  VectorCopy(position,charge.s.origin2);charge.s.origin2[1]+=2;
  G_SandboxChargeAttach(charge,*prop);CHECK(G_SandboxChargeSupport(charge)==prop);
  return charge;
 };
 const vec3_t packOrigin={0,0,94},wireOrigin={0,16,110},proxOrigin={-16,0,110},blastOrigin={16,0,110};
 auto& pack=plant(WP_DET_PACK,thinkF_NULL,packOrigin);
 auto& wire=plant(WP_TRIP_MINE,thinkF_laserTrapThink,wireOrigin);wire.s.eFlags|=EF_FIRING;
 auto& prox=plant(WP_TRIP_MINE,thinkF_WP_prox_mine_think,proxOrigin);prox.count=1;prox.delay=level.time+12345;prox.methodOfDeath=MOD_LASERTRIP_ALT;
 auto& blast=plant(WP_DET_PACK,thinkF_WP_Explode,blastOrigin);blast.takedamage=qfalse;blast.health=-8;
 const auto priorOwned=state.ownedCount;
 const auto priorEdits=edits.size();
 duplicateTool(wire);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].charges.size()==4);
 CHECK(vars["jar_duplicateStatus"]=="Copied: 5 objects / 0 links");
 level.time=4000;
 blockPlacement=2;placements=0;
 duplicatePaste();
 CHECK(state.ownedCount==priorOwned&&edits.size()==priorEdits&&!player.client->ps.eFlags&&placements==2);
 CHECK(G_SandboxChargeSupport(pack)==prop&&pack.nextthink==-1&&G_SandboxChargeSupport(wire)==prop);
 blockPlacement=0;placements=0;beamQueries=0;
 duplicatePaste();
 CHECK(state.ownedCount==priorOwned+5&&edits.back().group.size()==5&&beamQueries==1);
 CHECK(player.client->ps.eFlags&EF_PLANTED_CHARGE);
 const auto group=edits.back().group;
 auto& copiedProp=g_entities[group[0].entity];CHECK(std::fabs(copiedProp.currentOrigin[2]-27)<.001f);
 auto& copiedPack=g_entities[group[1].entity];auto& copiedWire=g_entities[group[2].entity];
 auto& copiedProx=g_entities[group[3].entity];auto& copiedBlast=g_entities[group[4].entity];
 for(size_t i=1;i<group.size();++i){
  auto& charge=g_entities[group[i].entity];
  CHECK(charge.linked&&G_SandboxChargeSupport(charge)==&copiedProp&&charge.activator==&player);
  CHECK(!charge.owner&&!charge.target&&owned(charge.s.number)->parent==owned(copiedProp.s.number)->id);
 }
 CHECK(copiedPack.e_ThinkFunc==thinkF_NULL&&copiedPack.nextthink==-1&&copiedPack.splashDamage==140&&copiedPack.fxID==95);
 CHECK(copiedWire.e_ThinkFunc==thinkF_laserTrapThink&&copiedWire.nextthink==4250&&copiedWire.s.loopSound==39&&copiedWire.fxID==96);
 CHECK(std::fabs(copiedWire.currentOrigin[1]-16)<.001f&&std::fabs(copiedWire.s.origin2[1]-18)<.001f);
 CHECK(copiedProx.e_ThinkFunc==thinkF_WP_prox_mine_think&&copiedProx.delay==16345&&copiedProx.count==1&&copiedProx.methodOfDeath==MOD_LASERTRIP_ALT);
 CHECK(copiedBlast.e_ThinkFunc==thinkF_WP_Explode&&copiedBlast.nextthink==4250&&!copiedBlast.takedamage&&!copiedBlast.health);
 MemorySave mounts;ojk::SavedGameHelper writer(&mounts);
 G_SandboxChargesExport(false,writer);G_SandboxChargesReset();
 CHECK(!G_SandboxChargeSupport(copiedWire));
 mounts.reset_buffer_offset();CHECK(G_SandboxChargesImport(false,writer));
 CHECK(G_SandboxChargeSupport(copiedWire)==&copiedProp&&G_SandboxChargeSupport(wire)==prop);
 duplicateTool(copiedWire);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects.front().charges.size()==4);
 undoEdit();
 CHECK(state.ownedCount==priorOwned&&std::all_of(group.begin(),group.end(),[](const auto& member){return !g_entities[member.entity].inuse;}));
 CHECK(G_SandboxChargeSupport(pack)==prop&&G_SandboxChargeSupport(wire)==prop&&prox.delay==15345&&blast.nextthink==3250);
 chargeShelf=true;duplicatePaste();chargeShelf=false;
 CHECK(state.ownedCount==priorOwned+5&&edits.back().group.size()==5);
 const auto& raised=edits.back().group;
 CHECK(std::fabs(g_entities[raised[0].entity].currentOrigin[2]-32)<.001f&&
       std::fabs(g_entities[raised[1].entity].currentOrigin[2]-16)<.001f);
 undoEdit();CHECK(state.ownedCount==priorOwned&&G_SandboxChargeSupport(pack)==prop);
 G_FreeEntity(&pack);G_FreeEntity(&wire);G_FreeEntity(&prox);G_FreeEntity(&blast);remove(prop->s.number,true);
 duplicateClear();gi.trace=previousTrace;
}
void CheckStandaloneChargeDuplicate(){
 const auto previousTrace=gi.trace;
 static bool wall,blockedCharge,actorSurface,sky;
 static int beams;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_WORLD;VectorCopy(end,hit->endpos);
  if(!mins){
   hit->fraction=.5f;VectorSet(hit->endpos,200,0,0);
   VectorSet(hit->plane.normal,wall?-1:0,0,wall?0:1);
   if(actorSurface)hit->entityNum=7;
   if(sky)hit->surfaceFlags=SURF_NOIMPACT;
  }else if(mask==MASK_SHOT&&mode==G2_NOCOLLIDE){
   CHECK(VectorCompare(start,end));
   hit->startsolid=blockedCharge?qtrue:qfalse;
  }else if(mask==MASK_SHOT&&mode==G2_RETURNONHIT){
   auto& charge=g_entities[pass];++beams;
   CHECK(wall&&charge.linked&&!G_SandboxChargeSupport(charge)&&start[0]==195&&end[0]==195-2048);
  }
 };
 auto& player=g_entities[0];
 player.client->ps.eFlags&=~EF_PLANTED_CHARGE;
 auto& source=*G_TrySpawn();
 source.s.eType=ET_GENERAL;source.s.weapon=WP_TRIP_MINE;source.s.eFlags=EF_MISSILE_STICK|EF_FIRING;
 source.s.pos.trType=TR_STATIONARY;source.e_DieFunc=dieF_WP_ExplosiveDie;
 source.classname="tripmine";source.e_ThinkFunc=thinkF_laserTrapThink;source.activator=&player;
 source.health=15;source.takedamage=qtrue;source.splashDamage=88;source.splashRadius=160;source.nextthink=level.time+250;
 // Old saves can have the zero-initialized ground field even on the world.
 source.s.groundEntityNum=0;
 const auto priorOwned=state.ownedCount;
 duplicateTool(source);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasStandaloneCharge&&
       duplicateSnapshot.objects[0].standaloneCharge.armed);
 auto invalid=duplicateSnapshot;invalid.objects[0].standaloneCharge.pending=0;
 CHECK(!jar::duplicate::Valid(invalid));
 actorSurface=true;duplicatePaste();actorSurface=false;
 sky=true;duplicatePaste();sky=false;
 blockedCharge=true;duplicatePaste();blockedCharge=false;
 CHECK(state.ownedCount==priorOwned&&beams==0&&source.inuse);
 wall=true;level.time+=1000;duplicatePaste();
 auto& copy=g_entities[selected];
 CHECK(&copy!=&source&&copy.linked&&beams==1&&copy.currentOrigin[0]==195&&copy.movedir[0]==-1);
 CHECK(copy.nextthink==level.time+250&&copy.s.loopSound==39&&copy.splashDamage==88&&copy.s.groundEntityNum==ENTITYNUM_WORLD);
 CHECK(owned(copy.s.number)&&!owned(copy.s.number)->parent);
 MemorySave native;
 ojk::SavedGameHelper save(&native);
 copy.sg_export(save);native.reset_buffer_offset();
 gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
 CHECK(restored.s.groundEntityNum==ENTITYNUM_WORLD&&restored.e_ThinkFunc==thinkF_laserTrapThink&&
       restored.s.origin2[0]==195&&restored.movedir[0]==-1&&restored.nextthink==copy.nextthink);
 duplicateTool(copy);CHECK(duplicateSnapshot.objects[0].hasStandaloneCharge);
 undoEdit();CHECK(!copy.inuse&&source.inuse&&state.ownedCount==priorOwned);
 source.e_ThinkFunc=thinkF_WP_prox_mine_think;source.s.eFlags=EF_MISSILE_STICK|EF_PROX_TRIP;
 source.methodOfDeath=MOD_LASERTRIP_ALT;source.delay=level.time+20000;source.nextthink=level.time+300;
 duplicateTool(source);level.time+=500;wall=false;duplicatePaste();
 auto& proximity=g_entities[selected];
 CHECK(proximity.e_ThinkFunc==thinkF_WP_prox_mine_think&&proximity.delay==level.time+20000&&
       proximity.nextthink==level.time+300&&(proximity.s.eFlags&EF_PROX_TRIP)&&proximity.count==0);
 undoEdit();CHECK(!proximity.inuse&&source.inuse);
 source.s.weapon=WP_DET_PACK;source.classname="detpack";source.e_ThinkFunc=thinkF_NULL;source.nextthink=-1;
 duplicateTool(source);wall=false;duplicatePaste();
 auto& remote=g_entities[selected];
 CHECK(remote.s.weapon==WP_DET_PACK&&remote.nextthink==-1&&remote.currentOrigin[2]==11&&
       (player.client->ps.eFlags&EF_PLANTED_CHARGE));
 undoEdit();CHECK(!remote.inuse&&source.inuse);
 source.s.groundEntityNum=7;source.splashDamage=200;duplicateTool(source);
 CHECK(duplicateSnapshot.objects[0].standaloneCharge.splash==88);
 G_FreeEntity(&source);duplicateClear();gi.trace=previousTrace;
}
void CheckPropertiesAndPaste(){
 vec3_t p={0,0,100},a={0,75,0};
 {
  using namespace jar::sandboxAngular;
  auto* source=spawn(Catalog("func_pendulum"),p,a);
  CHECK(G_SandboxAngularSet(*source,{Pendulum,Side,31,1600,625,true,true}));
  duplicateTool(*source);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].angular.kind==Pendulum);
  auto invalid=duplicateSnapshot;invalid.objects[0].angular.phase=1600;CHECK(!jar::duplicate::Valid(invalid));
  duplicatePaste();auto& copy=g_entities[selected];Settings settings;
  CHECK(&copy!=source&&G_SandboxAngularRead(copy,settings)&&settings.axis==Side&&settings.rate==31&&
        settings.period==1600&&settings.phase==625&&settings.running&&VectorCompare(copy.currentAngles,a));
  undoEdit();CHECK(!copy.inuse&&source->inuse);remove(source->s.number,true);
 }
 {
  using namespace jar::speaker;
  const int priorTime=level.time;
  const auto priorGet=gi.GetConfigstring;
  const auto priorSet=gi.SetConfigstring;
  static std::map<int,std::string> soundConfig;
  gi.GetConfigstring=[](int index,char* out,int size){Q_strncpyz(out,soundConfig[index].c_str(),size);};
  gi.SetConfigstring=[](int index,const char* text){soundConfig[index]=text;};
  level.time=1000;
  auto* source=spawn(Catalog("target_speaker"),p,a);
  CHECK(G_SandboxSpeakerSet(*source,{2,Repeat,1500,200,true,true,950}));
  const int deadline=source->nextthink;
  duplicateTool(*source);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasSpeaker);
  auto invalid=duplicateSnapshot;
  invalid.objects[0].speakerSettings.remaining=1701;
  CHECK(!jar::duplicate::Valid(invalid));
  level.time+=200;
  duplicatePaste();auto& copy=g_entities[selected];Settings settings;
  CHECK(&copy!=source&&G_SandboxSpeakerRead(copy,settings)&&settings.cue==2&&settings.mode==Repeat&&
        settings.period==1500&&settings.variation==200&&settings.active&&settings.remaining==950&&
        copy.nextthink==level.time+950&&source->nextthink==deadline&&speakerEvents.empty());
  level.time=copy.nextthink-1;
  G_SandboxSpeakerTick(copy);
  CHECK(speakerEvents.empty());
  ++level.time;
  G_SandboxSpeakerTick(copy);
  CHECK(speakerEvents==std::vector<int>{copy.s.number}&&source->nextthink==deadline);
  undoEdit();CHECK(!copy.inuse&&source->inuse&&source->nextthink==deadline);

  CHECK(G_SandboxSpeakerSet(*source,{2,Repeat,1500,200,false,true,700}));
  duplicateTool(*source);
  duplicatePaste();auto& pausedCopy=g_entities[selected];
  CHECK(G_SandboxSpeakerRead(pausedCopy,settings)&&!settings.enabled&&settings.active&&settings.remaining==700);
  CHECK(!pausedCopy.nextthink);
  G_SandboxSpeakerEnable(pausedCopy,true);
  CHECK(pausedCopy.nextthink==level.time+700&&!source->nextthink&&source->fly_sound_debounce_time==700);
  CHECK(speakerEvents.size()==1);
  undoEdit();CHECK(!pausedCopy.inuse&&source->inuse);

  CHECK(G_SandboxSpeakerSet(*source,{defaultLoopCue,Loop,1000,0,true,true,0}));
  auto* relay=spawn(Catalog("target_relay"),p,a);
  relay->target=G_NewString(source->targetname);
  duplicateTool(*relay);
  CHECK(duplicateSnapshot.objects.size()==2);
  duplicatePaste();auto& relayCopy=g_entities[selected];
  auto* loopCopy=G_Find(nullptr,FOFS(targetname),relayCopy.target);
  CHECK(loopCopy&&loopCopy!=source&&loopCopy->s.loopSound==source->s.loopSound&&loopCopy->s.loopSound!=0);
  CHECK(G_SandboxSpeakerRead(*loopCopy,settings)&&settings.mode==Loop&&settings.enabled&&settings.active);
  G_SandboxSpeakerEnable(*loopCopy,false);
  CHECK(!loopCopy->s.loopSound&&source->s.loopSound&&speakerEvents.size()==1);
  undoEdit();CHECK(!relayCopy.inuse&&!loopCopy->inuse&&relay->inuse&&source->inuse);
  remove(relay->s.number,true);
  remove(source->s.number,true);
  speakerEvents.clear();
  level.time=priorTime;
  gi.GetConfigstring=priorGet;gi.SetConfigstring=priorSet;
 }
 {auto*w=spawn(Catalog("waypoint"),p,a);auto*v=spawn(Catalog("waypoint_small"),p,a);w->target2=G_NewString(v->targetname);v->target=G_NewString(w->targetname);CHECK(G_WaypointSet(*w,{64,false})&&G_WaypointSet(*v,{0,true}));edits.clear();duplicateTool(*w);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].hasWaypoint);auto bad=duplicateSnapshot;bad.objects[0].waypoint.range=1025;CHECK(!jar::duplicate::Valid(bad));const auto count=state.ownedCount;duplicatePaste();CHECK(state.ownedCount==count+2&&edits.back().group.size()==2);auto&copy=g_entities[selected];CHECK(copy.inuse&&copy.wait==64&&(copy.svFlags&SVF_INACTIVE)&&copy.target2&&strcmp(copy.target2,v->targetname));gentity_t*other=nullptr;for(unsigned i=0;i<state.ownedCount;++i){auto&e=g_entities[state.owned[i].entity];if(e.targetname&&!strcmp(e.targetname,copy.target2))other=&e;}CHECK(other&&other!=v&&!strcmp(other->target,copy.targetname)&&!other->wait&&!(other->svFlags&SVF_INACTIVE));undoEdit();CHECK(!copy.inuse&&!other->inuse&&w->inuse&&v->inuse);remove(w->s.number,true);remove(v->s.number,true);}
 {auto*camera=spawn(Catalog("misc_camera_track"),p,a);auto*focus=spawn(Catalog("misc_camera_focus"),p,a);camera->target2=G_NewString(focus->targetname);focus->target2=G_NewString(camera->targetname);camera->count=1;camera->target3=G_NewString("live_route_cursor");CHECK(G_SceneCameraSet(*camera,{96,90,3,false}));
 edits.clear();duplicateTool(*camera);CHECK(duplicateSnapshot.objects.size()==5&&duplicateSnapshot.objects[0].hasSceneCamera);const auto sceneOwned=state.ownedCount;auto invalid=duplicateSnapshot;invalid.objects[0].sceneCamera.speed=-1;CHECK(!jar::duplicate::Valid(invalid));
 duplicatePaste();CHECK(state.ownedCount==sceneOwned+5&&edits.back().group.size()==5);auto&copy=g_entities[selected];CHECK(copy.inuse&&copy.speed==96&&copy.radius==90&&copy.wait==3&&!(copy.spawnflags&1)&&!copy.count&&!copy.target3&&strcmp(copy.target2,focus->targetname));
 gentity_t*copiedFocus=nullptr;for(unsigned i=0;i<state.ownedCount;++i){auto&e=g_entities[state.owned[i].entity];if(e.targetname&&!strcmp(e.targetname,copy.target2))copiedFocus=&e;}CHECK(copiedFocus&&copiedFocus!=focus&&!strcmp(copiedFocus->target2,copy.targetname));
 undoEdit();CHECK(!copy.inuse&&!copiedFocus->inuse&&camera->inuse&&focus->inuse);for(int i=int(state.ownedCount)-1;i>=0;--i)remove(state.owned[i].entity,true);}
 {static std::string skyConfig;gi.GetConfigstring=[](int n,char*out,int size){CHECK(n==CS_SKYBOXORG);Q_strncpyz(out,skyConfig.c_str(),size);};gi.SetConfigstring=[](int n,const char*text){CHECK(n==CS_SKYBOXORG);skyConfig=text;};
 auto*sky=spawn(Catalog("misc_skyportal"),p,a);CHECK(G_SkyPortalSet(*sky,{true,false}));const auto originalSky=skyConfig;edits.clear();duplicateTool(*sky);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasSky&&!duplicateSnapshot.objects[0].sky.enabled);duplicatePaste();auto&copy=g_entities[selected];jar::skyPortal::Settings settings;CHECK(copy.inuse&&G_SkyPortalRead(copy,settings)&&!settings.enabled&&!settings.entities&&skyConfig==originalSky);undoEdit();CHECK(!copy.inuse&&sky->inuse&&skyConfig==originalSky);remove(sky->s.number,true);CHECK(skyConfig.empty());}
 {auto*point=spawn(Catalog("point_combat"),p,a);CHECK(G_CombatPointSet(*point,{34,false}));const int originalSlot=point->count;edits.clear();duplicateTool(*point);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasCombatPoint);auto bad=duplicateSnapshot;bad.objects[0].combat.flags=64;CHECK(!jar::duplicate::Valid(bad));duplicatePaste();auto&copy=g_entities[selected];CHECK(copy.inuse&&copy.count!=originalSlot&&copy.spawnflags==34&&(copy.svFlags&SVF_INACTIVE));const int copiedSlot=copy.count;undoEdit();CHECK(!copy.inuse&&point->inuse&&combatOwners[copiedSlot]==-1);remove(point->s.number,true);}

 for(const char*name:{"target_location","target_print","trigger_location","item_datapad"}){
  auto*marker=spawn(Catalog(name),p,a);marker->message=G_NewString("Hangar 2");marker->svFlags|=SVF_INACTIVE;
  const bool isVolume=jar::volumeEntity::LocationClass(name);
  const vec3_t extent={96,48,32};
  if(isVolume)CHECK(jar::volumeEntity::Resize(*marker,extent));
  edits.clear();duplicateTool(*marker);
  CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasLabel);auto bad=duplicateSnapshot;bad.objects[0].label=std::string("A\0B",3);CHECK(!jar::duplicate::Valid(bad));
  duplicatePaste();auto& copy=g_entities[selected];CHECK(&copy!=marker&&!strcmp(copy.message,"Hangar 2"));
  if(isVolume){
   vec3_t copiedExtent;
   CHECK(jar::volumeEntity::Dimensions(copy,copiedExtent)&&VectorCompare(copiedExtent,extent));
   CHECK(copy.contents==CONTENTS_ITEM&&(copy.svFlags&SVF_INACTIVE)&&VectorCompare(copy.currentAngles,a));
  }else if(!strcmp(name,"target_location"))CHECK(copy.svFlags&SVF_INACTIVE);
  undoEdit();CHECK(state.ownedCount==1);remove(marker->s.number,true);
 }
 auto*interest=spawn(Catalog("target_interest"),p,a);interest->svFlags|=SVF_INACTIVE;edits.clear();duplicateTool(*interest);CHECK(duplicateSnapshot.objects.size()==2);
 duplicatePaste();CHECK(state.ownedCount==4&&(g_entities[selected].svFlags&SVF_INACTIVE)&&strcmp(g_entities[selected].target,interest->target));
 undoEdit();CHECK(state.ownedCount==2);while(state.ownedCount)remove(state.owned[state.ownedCount-1].entity,true);
 auto*shot=spawn(Catalog("trigger_always"),p,a);shot->delay=900;G_AlwaysSchedule(*shot,210);
 duplicateTool(*shot);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].alwaysRemaining==210);
 level.time+=75;edits.clear();duplicatePaste();auto&pendingCopy=g_entities[selected];
 CHECK(&pendingCopy!=shot&&pendingCopy.delay==900&&G_AlwaysRemaining(pendingCopy)==210&&G_AlwaysRemaining(*shot)==135);
 undoEdit();CHECK(!pendingCopy.inuse&&shot->inuse);
 G_AlwaysSchedule(*shot,0);duplicateTool(*shot);duplicatePaste();auto&spentCopy=g_entities[selected];
 CHECK(&spentCopy!=shot&&spentCopy.delay==900&&!G_AlwaysRemaining(spentCopy));undoEdit();remove(shot->s.number,true);
 auto*dist=spawn(Catalog("trigger_entdist"),p,a);
 auto*reference=Make(Catalog("target_position"),p,a);auto*subject=Make(3001,p,a);
 G_DistanceSetLink(*dist,JAR_DISTANCE_REFERENCE,reference->targetname);
 G_DistanceSetLink(*dist,JAR_DISTANCE_SUBJECT,subject->targetname);
 dist->count=321;dist->spawnflags=3;dist->svFlags|=SVF_INACTIVE;
 duplicateTool(*dist);CHECK(duplicateSnapshot.objects.size()==5&&duplicateSnapshot.objects[0].distanceRange==321);
 duplicatePaste();auto&distCopy=g_entities[selected];
 CHECK(&distCopy!=dist&&distCopy.count==321&&distCopy.spawnflags==3&&(distCopy.svFlags&SVF_INACTIVE));
 CHECK(distCopy.target&&distCopy.target2&&distCopy.ownername&&distCopy.NPC_target);
 CHECK(strcmp(distCopy.target,dist->target)&&strcmp(distCopy.target2,dist->target2)&&
       strcmp(distCopy.ownername,reference->targetname)&&strcmp(distCopy.NPC_target,subject->targetname));
 undoEdit();CHECK(!distCopy.inuse&&dist->inuse&&reference->inuse&&subject->inuse);
 remove(reference->s.number,true);duplicateTool(*dist);CHECK(duplicateSnapshot.objects[0].distanceReferenceRequired&&!duplicateSnapshot.objects[0].distanceReference);
 duplicatePaste();CHECK(g_entities[selected].ownername&&!strcmp(g_entities[selected].ownername,"@jar_missing_reference"));
 undoEdit();while(state.ownedCount)remove(state.owned[state.ownedCount-1].entity,true);
 auto*wall=spawn(JAR_WALL_CATALOG_ID,p,a,0,true);wall->svFlags|=SVF_INACTIVE;
 duplicateTool(*wall);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].wallVisibility==0);
 duplicatePaste();auto&wallCopy=g_entities[selected];
 CHECK(&wallCopy!=wall&&G_WallPortable(wallCopy)&&!wallCopy.count&&wallCopy.contents==CONTENTS_ITEM&&(wallCopy.svFlags&SVF_INACTIVE));
 undoEdit();CHECK(!wallCopy.inuse&&wall->inuse);remove(wall->s.number,true);
 auto*lock=spawn(Catalog("misc_maglock"),p,a);auto*door=lock->activator;lock->health=5;edits.clear();duplicateTool(*door);
 CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].catalog==Catalog("misc_maglock"));duplicatePaste();
 auto*copyLock=&g_entities[selected];auto*copyDoor=copyLock->activator;
 CHECK(state.ownedCount==4&&copyLock!=lock&&copyDoor!=door&&copyDoor->inuse&&copyDoor->lockCount==1&&(copyDoor->svFlags&SVF_INACTIVE)&&copyLock->health==5);
 CHECK(owned(copyDoor->s.number)->parent==owned(copyLock->s.number)->id&&edits.back().group.size()==2);
 // The actual ownership detach cannot affect another parent's child or a
 // reused slot, and persists the now independent door in the saved registry.
 detachChild(lock,copyDoor);CHECK(owned(copyDoor->s.number)->parent==owned(copyLock->s.number)->id);
 auto*name=copyLock->targetname;copyLock->targetname=G_NewString("reused_slot");detachChild(copyLock,copyDoor);CHECK(owned(copyDoor->s.number)->parent!=0);copyLock->targetname=name;
 detachChild(copyLock,copyDoor);CHECK(!owned(copyDoor->s.number)->parent);State lockSaved;auto lockBytes=Encode(state);
 CHECK(Decode(lockBytes.data(),lockBytes.size(),lockSaved,ENTITYNUM_WORLD)&&OwnedId(lockSaved,owned(copyDoor->s.number)->id)->parent==0);
 undoEdit();CHECK(state.ownedCount==2&&lock->inuse&&door->inuse);remove(door->s.number,true);remove(lock->s.number,true);CHECK(!state.ownedCount);
 // Native space volumes copy their bounds and active state, including an
 // authored volume larger than the portable default, without restarting it.
 vec3_t volumeAngles={20,35,17};
 auto*volume=spawn(Catalog("trigger_space"),p,a);volume->contents=0;volume->svFlags|=SVF_INACTIVE;volume->pos2[2]=256;G_SetAngles(volume,volumeAngles);jar::volumeEntity::Refresh(*volume);edits.clear();duplicateTool(*volume);blocked=true;duplicatePaste();blocked=false;
 CHECK(state.ownedCount==2&&g_entities[selected].contents==0&&(g_entities[selected].svFlags&SVF_INACTIVE)&&g_entities[selected].pos2[2]==256&&VectorCompare(g_entities[selected].currentAngles,volumeAngles)&&VectorCompare(g_entities[selected].maxs,volume->maxs));
 undoEdit();CHECK(state.ownedCount==1);volume->contents=CONTENTS_TRIGGER;volume->svFlags&=~SVF_INACTIVE;duplicateTool(*volume);duplicatePaste();CHECK(g_entities[selected].contents==CONTENTS_TRIGGER&&!(g_entities[selected].svFlags&SVF_INACTIVE));undoEdit();remove(volume->s.number,true);CHECK(!state.ownedCount);
 auto*directional=spawn(Catalog("trigger_bidirectional"),p,a);directional->wait=2.5f;directional->count=4;directional->spawnflags=1;
 vec3_t shape={32,256,64};CHECK(jar::volumeEntity::Resize(*directional,shape));edits.clear();duplicateTool(*directional);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].backwardTarget);
 duplicatePaste();auto&newDirectional=g_entities[selected];CHECK(state.ownedCount==6&&newDirectional.wait==2.5f&&newDirectional.count==4&&(newDirectional.spawnflags&1)&&newDirectional.pos2[2]==64);
 CHECK(strcmp(newDirectional.target,directional->target)&&strcmp(newDirectional.paintarget,directional->paintarget)&&strcmp(newDirectional.target,newDirectional.paintarget));
 unsigned linked=0;for(unsigned i=0;i<state.ownedCount;++i){auto&child=g_entities[state.owned[i].entity];if(!strcmp(child.targetname,newDirectional.target)||!strcmp(child.targetname,newDirectional.paintarget)){CHECK(state.owned[i].parent==owned(selected)->id);++linked;}}CHECK(linked==2);
 undoEdit();CHECK(state.ownedCount==3);while(state.ownedCount)remove(state.owned[state.ownedCount-1].entity,true);CHECK(!state.ownedCount);
 Q_strncpyz(weaponData[WP_DISRUPTOR].weaponMdl,"models/weapons2/disruptor/disruptor.md3",sizeof(weaponData[WP_DISRUPTOR].weaponMdl));
 weaponData[WP_DISRUPTOR].ammoIndex=AMMO_POWERCELL;
 bg_itemlist[1].giType=IT_WEAPON;bg_itemlist[1].giTag=WP_DISRUPTOR;
 auto*npc=Make(3001,p,a);npc->health=777;
 npc->client->ps.weapon=npc->s.weapon=WP_DISRUPTOR;
 npc->client->ps.stats[STAT_WEAPONS]|=1<<WP_DISRUPTOR;
 npc->client->ps.ammo[AMMO_POWERCELL]=37;npc->client->ps.ammo[AMMO_BLASTER]=19;
 npc->client->playerTeam=TEAM_PLAYER;npc->client->enemyTeam=TEAM_ENEMY;
 npc->client->ps.stats[STAT_ARMOR]=42;
 CHECK(G_MovementActorSet(*npc,{425,3,true}));
 npc->NPC->scriptFlags|=SCF_ALT_FIRE;
 npc->NPC->aiFlags|=NPCAI_MANUAL_AMMO;
 G_WorkshopWriteNPCOrder(*npc->NPC,{jar::npcOrder::Follow,180});
 nativeBegin(*npc,true);duplicateTool(*npc);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasHealth&&
       duplicateSnapshot.objects[0].frozen&&duplicateSnapshot.objects[0].hasActorLoadout);
 CHECK(duplicateSnapshot.objects[0].hasActorStatus&&duplicateSnapshot.objects[0].hasActorMovement&&
       duplicateSnapshot.objects[0].actorMovement.gravity==425&&npc->client->ps.gravity==0);
 CHECK(duplicateSnapshot.objects[0].hasActorOrder&&duplicateSnapshot.objects[0].actorOrder.distance==180);
 auto invalidLoadout=duplicateSnapshot;
 invalidLoadout.objects[0].actorLoadout.weapons|=1u<<WP_PHYSGUN;
 CHECK(!jar::duplicate::Valid(invalidLoadout));
 npc->client->ps.ammo[AMMO_POWERCELL]=3;
 attachWeapon=false;duplicatePaste();attachWeapon=true;
 CHECK(state.ownedCount==1&&npc->inuse&&!registeredNpcItems);
 duplicatePaste();auto&copy=g_entities[selected];
 CHECK(copy.health==777&&copy.client->ps.stats[STAT_HEALTH]==777&&copy.client->ps.viewangles[1]==75&&nativeControls.count(selected));
 CHECK(copy.client->ps.weapon==WP_DISRUPTOR&&copy.s.weapon==WP_DISRUPTOR&&copy.weaponModel[0]==1&&registeredNpcItems==1);
 CHECK(copy.client->ps.ammo[AMMO_POWERCELL]==37&&copy.client->ps.ammo[AMMO_BLASTER]==19&&copy.NPC->currentAmmo==37);
 CHECK(copy.NPC->scriptFlags&SCF_ALT_FIRE);
 CHECK(copy.NPC->aiFlags&NPCAI_MANUAL_AMMO);
 const auto npcCopiedOrder=G_WorkshopReadNPCOrder(*copy.NPC);
 CHECK(npcCopiedOrder.mode==jar::npcOrder::Follow&&npcCopiedOrder.distance==180);
 CHECK(copy.client->playerTeam==TEAM_PLAYER&&copy.client->enemyTeam==TEAM_ENEMY&&copy.client->ps.persistant[PERS_TEAM]==TEAM_PLAYER);
 CHECK(copy.client->ps.stats[STAT_ARMOR]==42&&copy.client->ps.friction==3&&copy.client->ps.gravity==0);
 nativeRelease(copy.s.number,false);
 CHECK((copy.svFlags&SVF_CUSTOM_GRAVITY)&&copy.client->ps.gravity==425&&npc->client->ps.gravity==0);
 undoEdit();CHECK(!copy.inuse&&state.ownedCount==1&&npc->client->ps.ammo[AMMO_POWERCELL]==3);
 npc->client->ps.weapon=npc->s.weapon=WP_NONE;
 duplicateTool(*npc);duplicatePaste();auto&disarmed=g_entities[selected];
 CHECK(disarmed.client->ps.weapon==WP_NONE&&disarmed.s.weapon==WP_NONE&&disarmed.weaponModel[0]==-1);
 undoEdit();CHECK(!disarmed.inuse&&npc->inuse);
 auto& saberState=npc->client->ps;
 saberState.weapon=npc->s.weapon=WP_SABER;
 saberState.stats[STAT_WEAPONS]|=1<<WP_SABER;
 saberState.dualSabers=qtrue;
 saberState.saberAnimLevel=SS_DUAL;saberState.saberStylesKnown=1<<SS_DUAL;
 CHECK(WP_SaberParseParms("fixture_primary",&saberState.saber[0],qfalse));
 CHECK(WP_SaberParseParms("fixture_secondary",&saberState.saber[1],qfalse));
 for(auto& saber:saberState.saber)for(int blade=0;blade<saber.numBlades;++blade){
  saber.blade[blade].color=SABER_PURPLE;
  saber.blade[blade].radius=2.5f;
  saber.blade[blade].lengthMax=43;
  saber.blade[blade].active=blade==0?qtrue:qfalse;
 }
 CHECK(WP_SaberInitSandboxBladeData(npc));
 const int sourceSaber=saberState.saberEntityNum;
 saberState.saberInFlight=qtrue;
 duplicateTool(*npc);CHECK(duplicateSnapshot.objects[0].hasActorLoadout);
 bg_itemlist[2].giType=IT_WEAPON;bg_itemlist[2].giTag=WP_SABER;
 allocateSaberHelper=false;duplicatePaste();allocateSaberHelper=true;
 CHECK(state.ownedCount==1&&g_entities[sourceSaber].inuse);
 attachSabers=false;duplicatePaste();attachSabers=true;
 CHECK(state.ownedCount==1&&!g_entities[nextSlot-1].inuse&&g_entities[sourceSaber].inuse);
 duplicatePaste();auto& saberCopy=g_entities[selected];
 const auto& copiedSaber=saberCopy.client->ps;
 const int copiedHelper=copiedSaber.saberEntityNum;
 CHECK(copiedSaber.weapon==WP_SABER&&copiedSaber.dualSabers&&copiedSaber.saberAnimLevel==SS_DUAL);
 CHECK(!copiedSaber.saberInFlight&&saberState.saberInFlight);
 CHECK(copiedHelper!=sourceSaber&&g_entities[copiedHelper].inuse&&g_entities[copiedHelper].owner==&saberCopy);
 CHECK(!std::strcmp(copiedSaber.saber[1].name,"fixture_secondary")&&copiedSaber.saber[1].name!=saberState.saber[1].name);
 CHECK(copiedSaber.saber[0].blade[0].active&&!copiedSaber.saber[0].blade[1].active);
 CHECK(copiedSaber.saber[0].blade[1].color==SABER_PURPLE&&copiedSaber.saber[0].blade[1].lengthMax==43&&copiedSaber.saber[0].blade[1].radius==2.5f);
 undoEdit();CHECK(!saberCopy.inuse&&!g_entities[copiedHelper].inuse&&g_entities[sourceSaber].inuse);
 nativeRelease(npc->s.number,false);remove(npc->s.number,true);
 {
  auto* walker=Make(3001,p,a);
  vec3_t goalPosition;VectorCopy(p,goalPosition);goalPosition[1]+=80;
  auto* goal=spawn(Catalog("waypoint_navgoal"),goalPosition,a);
  CHECK(walker&&goal&&owned(goal->s.number));
  const jar::npcOrder::Destination original{owned(goal->s.number)->id,unsigned(goal->s.number)};
  G_WorkshopWriteNPCGoal(*walker->NPC,original);
  G_WorkshopWriteNPCOrder(*walker->NPC,{jar::npcOrder::Go,0});
  duplicateTool(*walker);
  CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects.front().actorGoal==original.owner);
  auto badGoal=duplicateSnapshot;badGoal.objects.front().actorGoal=badGoal.objects.front().source;
  CHECK(!jar::duplicate::Valid(badGoal));
  duplicatePaste();
  auto& copiedWalker=g_entities[selected];
  CHECK(copiedWalker.NPC&&G_WorkshopReadNPCOrder(*copiedWalker.NPC).mode==jar::npcOrder::Go);
  const auto copiedLink=G_WorkshopReadNPCGoal(*copiedWalker.NPC);
  auto* copiedGoal=G_WorkshopResolveNPCGoal(copiedLink);
  CHECK(copiedGoal&&copiedGoal!=goal&&copiedLink.owner!=original.owner&&G_WorkshopResolveNPCGoal(original)==goal);
  undoEdit();CHECK(!copiedWalker.inuse&&!copiedGoal->inuse&&goal->inuse&&walker->inuse);
  remove(goal->s.number);remove(walker->s.number);CHECK(!state.ownedCount);
 }
 {
  auto* walker=Make(3001,p,a);
  vec3_t firstPos,lastPos;VectorCopy(p,firstPos);VectorCopy(p,lastPos);firstPos[1]+=70;lastPos[1]+=140;
  auto* first=spawn(Catalog("waypoint"),firstPos,a);
  auto* last=spawn(Catalog("waypoint_small"),lastPos,a);
  CHECK(walker&&first&&last);
  first->target=G_NewString(last->targetname);
  last->target=G_NewString(first->targetname);
  G_WorkshopWriteNPCGoal(*walker->NPC,{owned(first->s.number)->id,unsigned(first->s.number)});
  G_WorkshopWriteNPCPatrol(*walker->NPC,{owned(last->s.number)->id,unsigned(last->s.number)});
  G_WorkshopWriteNPCOrder(*walker->NPC,{jar::npcOrder::Patrol,0});
  duplicateTool(*walker);
  CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects.front().actorPatrolGoal==owned(last->s.number)->id);
  duplicatePaste();auto& copy=g_entities[selected];
  CHECK(copy.NPC&&G_WorkshopReadNPCOrder(*copy.NPC).mode==jar::npcOrder::Patrol);
  auto* copyStart=G_WorkshopResolveNPCGoal(G_WorkshopReadNPCGoal(*copy.NPC));
  auto* copyCursor=G_WorkshopResolveNPCGoal(G_WorkshopReadNPCPatrol(*copy.NPC));
  CHECK(copyStart&&copyCursor&&copyStart!=first&&copyCursor!=last);
  CHECK(!strcmp(copyStart->target,copyCursor->targetname)&&!strcmp(copyCursor->target,copyStart->targetname));
  undoEdit();CHECK(!copy.inuse&&!copyStart->inuse&&!copyCursor->inuse);
  remove(first->s.number);remove(last->s.number);remove(walker->s.number);CHECK(!state.ownedCount);
 }
 auto* normalGravityNpc=Make(3001,p,a);
 normalGravityNpc->client->ps.gravity=0; // Temporary space exposure.
 normalGravityNpc->client->ps.friction=2;
 duplicateTool(*normalGravityNpc);duplicatePaste();auto& normalGravityCopy=g_entities[selected];
 CHECK(&normalGravityCopy!=normalGravityNpc&&!(normalGravityCopy.svFlags&SVF_CUSTOM_GRAVITY)&&
       normalGravityCopy.client->ps.gravity==800&&normalGravityCopy.client->ps.friction==2);
 undoEdit();CHECK(!normalGravityCopy.inuse&&normalGravityNpc->inuse);
 remove(normalGravityNpc->s.number,true);
 {
  auto* source=Make(4001,p,a);
  auto& vehicle=*source->m_pVehicle;
  source->health=67;vehicle.m_iArmor=53;
  source->client->ps.stats[STAT_ARMOR]=26;
  vehicle.weaponStatus[0].ammo=17;vehicle.turretStatus[0].ammo=30;
  vehicle.turretStatus[0].enemyEntNum=0;
  VectorSet(vehicle.jarPushDrift,200,0,0);
  duplicateTool(*source);
  CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasVehicleStatus);
  auto invalid=duplicateSnapshot;
  invalid.objects[0].vehicleStatus.value[2]=-1;
  CHECK(!jar::duplicate::Valid(invalid));
  // A second copy of a dying or stripped vehicle keeps the usable clipboard.
  vehicle.m_iDieTime=level.time+1;vehicle.m_iArmor=40;
  duplicateTool(*source);vehicle.m_iDieTime=0;
  vehicle.m_iRemovedSurfaces=1;duplicateTool(*source);vehicle.m_iRemovedSurfaces=0;
  CHECK(duplicateSnapshot.objects[0].vehicleStatus.value[0]==53);
  vehicle.weaponStatus[0].ammo=1;source->health=36;
  const int oldTime=level.time;
  level.time+=2500;
  duplicatePaste();auto& pasted=g_entities[selected];
  CHECK(&pasted!=source&&pasted.m_pVehicle&&pasted.m_pVehicle!=&vehicle);
  const auto& fresh=*pasted.m_pVehicle;
  const auto& ps=pasted.client->ps;
  CHECK(pasted.health==67&&ps.stats[STAT_HEALTH]==67&&fresh.m_iArmor==53);
  CHECK(fresh.m_iShields==26&&ps.stats[STAT_ARMOR]==26);
  CHECK(fresh.weaponStatus[0].ammo==17&&ps.ammo[0]==17&&fresh.weaponStatus[0].lastAmmoInc==level.time);
  CHECK(fresh.turretStatus[0].ammo==30&&ps.ammo[MAX_VEHICLE_WEAPONS]==30&&fresh.lastShieldInc==level.time);
  CHECK(fresh.m_pParentEntity==&pasted&&!fresh.m_pPilot&&!fresh.m_pDroidUnit&&
        fresh.turretStatus[0].enemyEntNum==ENTITYNUM_NONE&&!VectorLengthSquared(fresh.jarPushDrift));
  CHECK(VectorCompare(fresh.m_vOrientation,a)&&VectorCompare(fresh.m_vPrevOrientation,a));
  undoEdit();CHECK(!pasted.inuse&&source->inuse&&source->health==36&&vehicle.weaponStatus[0].ammo==1);
  remove(source->s.number,true);
  level.time=oldTime;
 }
 {
  auto& player=g_entities[0];const auto originalPlayer=player;auto* originalClients=level.clients;
  gclient_t playerClient{};player.client=level.clients=&playerClient;player.inuse=qtrue;player.health=100;
  playerClient.sess.missionStats.secretsFound=2;playerClient.sess.missionStats.totalSecrets=2;
  const auto originalNotice=gi.SendServerCommand;
  gi.SendServerCommand=[](int slot,const char* fmt,...){CHECK(slot==0&&!strcmp(fmt,"cp @SP_INGAME_SECRET_AREA"));};
  gentity_t authored{};authored.inuse=qtrue;authored.s.number=30;authored.classname=G_NewString("target_secret");
  authored.e_UseFunc=useF_target_secret_use;authored.noise_index=81;
  target_secret_use(&authored,nullptr,&player);
  CHECK(playerClient.sess.missionStats.secretsFound==3&&playerClient.sess.missionStats.totalSecrets==3);
  auto* secret=spawn(Catalog("target_secret"),p,a);
  auto* output=spawn(Catalog("target_position"),p,a);secret->target=G_NewString(output->targetname);
  const auto prior=timerOutputs.size();const int sounds=secretSounds;
  target_secret_use(secret,nullptr,nullptr);target_secret_use(secret,nullptr,nullptr);
  CHECK(secretSounds==sounds+1&&timerOutputs.size()==prior+1&&timerOutputs.back().first==output&&timerOutputs.back().second==&player);
  CHECK(secret->spawnflags&JAR_SECRET_FOUND);
  CHECK(playerClient.sess.missionStats.secretsFound==3&&playerClient.sess.missionStats.totalSecrets==3);
  duplicateTool(*secret);duplicatePaste();auto& pasted=g_entities[selected];
  CHECK(&pasted!=secret&&G_SandboxSecret(pasted)&&(pasted.spawnflags&JAR_SECRET_FOUND));
  CHECK(pasted.target&&strcmp(pasted.target,secret->target));
  target_secret_use(&pasted,nullptr,&player);CHECK(secretSounds==sounds+1);
  pasted.spawnflags=JAR_SECRET_REPEAT;pasted.svFlags|=SVF_INACTIVE;
  target_secret_use(&pasted,nullptr,&player);CHECK(secretSounds==sounds+1);
  pasted.svFlags&=~SVF_INACTIVE;target_secret_use(&pasted,nullptr,&player);
  target_secret_use(&pasted,nullptr,&player);CHECK(secretSounds==sounds+2&&timerOutputs.back().first!=output);
  ++level.time;target_secret_use(&pasted,nullptr,&player);--level.time;CHECK(secretSounds==sounds+3);
  undoEdit();CHECK(!pasted.inuse&&secret->inuse&&output->inuse);
  remove(secret->s.number,true);remove(output->s.number,true);
  timerOutputs.resize(prior);player=originalPlayer;level.clients=originalClients;gi.SendServerCommand=originalNotice;
 }
 {
  auto* replicator=spawn(Catalog("misc_replicator_item"),p,a);
  const int oldTime=level.time;replicator->wait=800;replicator->damage=12;
  misc_replicator_item_spawn(replicator,nullptr,nullptr);
  level.time+=125;
  CHECK(G_LogicCanSetUse(*replicator)&&G_ReplicatorPhase(*replicator,level.time).remaining==675);
  duplicateTool(*replicator);duplicatePaste();auto& copy=g_entities[selected];
  CHECK(&copy!=replicator&&copy.s.modelindex==replicator->s.modelindex&&copy.nextthink==level.time+675);
  CHECK(copy.wait==800&&copy.damage==12);
  MemorySave memory;ojk::SavedGameHelper save(&memory);copy.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  const auto savedPhase=G_ReplicatorPhase(restored,level.time);
  CHECK(savedPhase.phase==jar::replicator::Forming&&savedPhase.remaining==675&&
        savedPhase.variant==G_ReplicatorPhase(copy,level.time).variant&&restored.wait==800&&restored.damage==12);
  const auto formedModel=copy.s.modelindex;copy.nextthink=0; // The engine clears nextthink before dispatch.
  misc_replicator_item_finish_spawn(&copy);CHECK(copy.e_UseFunc==useF_misc_replicator_item_remove&&copy.s.modelindex==formedModel);
  misc_replicator_item_remove(&copy,nullptr,nullptr);CHECK(copy.e_UseFunc==useF_misc_replicator_item_remove);
  gentity_t recipient{};gclient_t recipientClient{};recipient.inuse=qtrue;recipient.client=&recipientClient;
  recipient.health=93;recipientClient.ps.stats[STAT_MAX_HEALTH]=100;
  misc_replicator_item_remove(&copy,nullptr,&recipient);
  CHECK(recipient.health==100&&recipientClient.ps.stats[STAT_HEALTH]==100&&copy.e_UseFunc==useF_misc_replicator_item_spawn);
  CHECK(!(copy.s.eFlags&EF_NODRAW)&&copy.s.modelindex==21);
  undoEdit();CHECK(!copy.inuse&&replicator->inuse&&replicator->e_ThinkFunc==thinkF_misc_replicator_item_finish_spawn);
  remove(replicator->s.number,true);level.time=oldTime;
 }
 auto*item=Make(2501,p,a);item->count=89;duplicateTool(*item);duplicatePaste();CHECK(g_entities[selected].count==89);undoEdit();remove(item->s.number,true);
 {
  auto* armor=Make(2501,p,a);pickup.giType=IT_ARMOR;pickup.quantity=25;armor->count=48;
  const int oldTime=level.time;
  armor->nextthink=level.time+1234;
  nativeBegin(*armor,true);level.time+=400;
  duplicateTool(*armor);duplicatePaste();auto& copy=g_entities[selected];
  CHECK(&copy!=armor&&G_WorkshopArmorPickupQuantity(copy)==48);
  CHECK(pickupLifetimeRemaining(copy)==1234&&nativeControls[selected].frozen);
  level.time+=100;nativeRelease(copy.s.number,false);CHECK(copy.nextthink==level.time+1234);
  MemorySave memory;ojk::SavedGameHelper save(&memory);copy.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  CHECK(restored.count==48&&(restored.flags&FL_DROPPED_ITEM)&&restored.nextthink==copy.nextthink);
  undoEdit();nativeRelease(armor->s.number,false);
  pickupLifetimeSet(*armor,0);
  duplicateTool(*armor);duplicatePaste();CHECK(g_entities[selected].nextthink==jar::pickupLifetime::keepDeadline);
  undoEdit();remove(armor->s.number,true);pickup={};level.time=oldTime;
 }
 {
  const unsigned boonId=Catalog("item_force_boon");
  supplies.push_back({boonId});
  pickup.giType=IT_POWERUP;pickup.giTag=PW_FORCE_BOON;pickup.quantity=25;
  auto* boon=Make(boonId,p,a);boon->count=60;
  duplicateTool(*boon);duplicatePaste();auto& copy=g_entities[selected];
  CHECK(&copy!=boon&&copy.item==&pickup&&G_WorkshopPickupQuantityEditable(copy)&&copy.count==60);
  MemorySave memory;ojk::SavedGameHelper save(&memory);copy.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  CHECK(restored.count==60&&restored.s.eType==ET_ITEM);
  undoEdit();remove(boon->s.number,true);supplies.pop_back();pickup={};
 }
 {
  auto* shooter=spawn(Catalog("shooter_grenade"),p,a);
  auto* aim=spawn(Catalog("target_position"),p,a);shooter->target=G_NewString(aim->targetname);
  CHECK(G_SandboxShooterSet(*shooter,{250,50,true,true,true}));
  G_SandboxShooterSchedule(*shooter,level.time,110);nativeBegin(*shooter,true);
  const int oldTime=level.time;level.time+=220;
  duplicateTool(*shooter);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects.front().shooterPendingMs==110);
  duplicatePaste();auto& pasted=g_entities[selected];
  CHECK(&pasted!=shooter&&pasted.client->ps.weapon==WP_THERMAL&&pasted.wait==250&&pasted.random==50&&(pasted.spawnflags&3)==3);
  CHECK(pasted.target&&strcmp(pasted.target,aim->targetname));
  CHECK(nativeControls[selected].frozen&&G_SandboxShooterPending(pasted,level.time)==110);
  level.time+=400;nativeRelease(pasted.s.number,false);CHECK(G_SandboxShooterPending(pasted,level.time)==110);
  undoEdit();CHECK(!pasted.inuse&&shooter->inuse&&aim->inuse);
  nativeRelease(shooter->s.number,false);remove(shooter->s.number,true);remove(aim->s.number,true);level.time=oldTime;
 }
 supplies.push_back({Catalog("holocron_force_heal")});holoPickup.classname="holocron_force_heal";holoPickup.giType=IT_HOLOCRON;holoPickup.giTag=FP_HEAL;
 auto*holo=Make(Catalog("holocron_force_heal"),p,a);holo->count=3;edits.clear();duplicateTool(*holo);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasCount&&duplicateSnapshot.objects[0].count==3);duplicatePaste();auto&holoCopy=g_entities[selected];CHECK(&holoCopy!=holo&&holoCopy.item==&holoPickup&&holoCopy.count==3&&holo->count==3);undoEdit();CHECK(!holoCopy.inuse&&holo->inuse);remove(holo->s.number,true);
 // Portable helper poses and cycles use fresh names/ownership. No double helpers.
 auto*train=spawn(Catalog("func_train"),p,a);edits.clear();const auto parent=owned(train->s.number)->id;std::vector<unsigned> children;
 for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].parent==parent)children.push_back(state.owned[i].entity);
 CHECK(children.size()==2);g_entities[children[1]].currentOrigin[0]=177;duplicateTool(g_entities[children[1]]);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].catalog==Catalog("func_train"));
 duplicatePaste();CHECK(state.ownedCount==6&&edits.size()==1&&edits.back().group.size()==3);const auto newParent=owned(selected)->id;
 std::vector<unsigned> copies;for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].parent==newParent)copies.push_back(state.owned[i].entity);
 CHECK(copies.size()==2&&g_entities[copies[1]].currentOrigin[0]-g_entities[copies[0]].currentOrigin[0]==177);
 CHECK(!strcmp(g_entities[selected].target,g_entities[copies[0]].targetname)&&!strcmp(g_entities[copies[0]].target,g_entities[copies[1]].targetname)&&!strcmp(g_entities[copies[1]].target,g_entities[copies[0]].targetname));
 State decoded;auto bytes=Encode(state);CHECK(Decode(bytes.data(),bytes.size(),decoded,ENTITYNUM_WORLD)&&decoded.ownedCount==6);undoEdit();CHECK(state.ownedCount==3);
 // Copying an owned target network includes its connected members once.
 auto*destination=Make(Catalog("target_position"),p,a);g_entities[children[0]].target=G_NewString(destination->targetname);duplicateTool(*destination);CHECK(duplicateSnapshot.objects.size()==4);
 duplicateClear();CHECK(duplicateSnapshot.objects.empty());
 for(int i=int(state.ownedCount)-1;i>=0;--i)remove(state.owned[i].entity,true);edits.clear();
 {
  // Many parents regenerate the same two helper types. Each copy must
  // inherit only its own helpers and publish a single assembly result.
  constexpr unsigned trains=24;
  const unsigned catalog=Catalog("func_train");
  auto* first=spawn(catalog,p,a);
  const auto rootId=owned(first->s.number)->id;
  for(unsigned i=1;i<trains;++i){
   vec3_t position;VectorCopy(p,position);position[1]+=i*24;
   auto* train=spawn(catalog,position,a);
   CHECK(SetOwnershipParent(state,*owned(train->s.number),rootId));
  }
  CHECK(state.ownedCount==3*trains);
  duplicateTool(*first);
  CHECK(duplicateSnapshot.objects.size()==3*trains);
  const auto before=attempt;
  const auto menusBefore=listCalls;
  const auto firstCopyId=state.nextOwned;
  duplicatePaste();
  CHECK(attempt==before+trains&&listCalls==menusBefore+1);
  CHECK(state.ownedCount==6*trains&&edits.size()==trains+1&&edits.back().group.size()==3*trains);
  for(const auto& entry:state.owned)if(entry.id>=firstCopyId&&entry.catalog==catalog){
   const auto& copy=g_entities[entry.entity];
   const auto* corner=G_Find(nullptr,FOFS(targetname),copy.target);
   CHECK(corner&&owned(corner->s.number)->parent==entry.id);
   const auto* next=G_Find(nullptr,FOFS(targetname),corner->target);
   CHECK(next&&next!=corner&&owned(next->s.number)->parent==entry.id);
  }
  undoEdit();
  CHECK(state.ownedCount==3*trains&&edits.size()==trains);
  CHECK(remove(first->s.number)&&state.ownedCount==0&&edits.empty());
 }
 // Copying a partly depleted native rack must not refill regenerated children.
 auto*rack=spawn(Catalog("misc_model_gun_rack"),p,a);CHECK(state.ownedCount==4);
 remove(state.owned[state.ownedCount-1].entity,true);remove(state.owned[state.ownedCount-1].entity,true);
 CHECK(state.ownedCount==2);duplicateTool(*rack);
 CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[1].rackShelfStock);
 edits.clear();duplicatePaste();CHECK(state.ownedCount==4&&edits.size()==1&&edits.back().group.size()==2);
 {
  auto& copiedRack=g_entities[selected];auto& item=g_entities[state.owned[3].entity];
  CHECK(rackStockAttached(item)&&item.s.groundEntityNum==ENTITYNUM_NONE);
  const int previousTime=level.time;auto* previousGravity=g_gravity;
  cvar_t localGravity{};g_gravity=&localGravity;runItemPhysics=true;itemThinks=0;
  for(float gravity:{800.f,0.f}){
   localGravity.value=gravity;
   vec3_t initial;VectorCopy(item.currentOrigin,initial);
   level.time+=100;G_RunItem(&item);
   CHECK(rackStockAttached(item)&&VectorCompare(item.currentOrigin,initial)&&!VectorLengthSquared(item.s.pos.trDelta));
  }
  CHECK(itemThinks==2);

  vec3_t destination;VectorCopy(copiedRack.currentOrigin,destination);destination[0]+=20;
  nativeBegin(copiedRack,false);nativeRelease(copiedRack.s.number,true);
  CHECK(copiedRack.s.pos.trType==TR_STATIONARY&&copiedRack.e_ThinkFunc==thinkF_NULL);
  nativeBegin(copiedRack,false);nativeControls[copiedRack.s.number].velocity[0]=250;
  nativeRelease(copiedRack.s.number,true);
  CHECK(copiedRack.s.pos.trType==TR_GRAVITY&&copiedRack.e_ThinkFunc==thinkF_G_RunObject&&copiedRack.nextthink==level.time+FRAMETIME);
  const float initialItemX=item.currentOrigin[0];
  CHECK(G_AdvanceObjectPose(copiedRack,destination,copiedRack.currentAngles));
  CHECK(item.currentOrigin[0]==initialItemX+20&&copiedRack.s.pos.trDelta[0]==250);
  const auto previousTrace=gi.trace;
  gi.trace=[](trace_t* hit,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int slot,int,EG2_Collision,int){
   CHECK(slot>0&&rackStockAttached(g_entities[slot]));
   *hit={};hit->fraction=.25f;VectorCopy(end,hit->endpos);
  };
  destination[0]+=20;const vec3_t newAngles={10,30,0};
  CHECK(!G_AdvanceObjectPose(copiedRack,destination,newAngles));
  CHECK(copiedRack.currentOrigin[0]==destination[0]-20&&item.currentOrigin[0]==initialItemX+20&&
        copiedRack.s.pos.trType==TR_STATIONARY&&copiedRack.e_ThinkFunc==thinkF_NULL&&!copiedRack.nextthink&&copiedRack.linked);
  gi.trace=previousTrace;

  localGravity.value=800;const float attachedZ=item.currentOrigin[2];
  rackStockLoose(item);G_RunItem(&item);
  level.time+=100;G_RunItem(&item);
  CHECK(!rackStockAttached(item)&&item.s.pos.trType==TR_GRAVITY&&item.currentOrigin[2]<attachedZ);
  g_gravity=previousGravity;level.time=previousTime;runItemPhysics=false;
 }
 undoEdit();CHECK(state.ownedCount==2);
 // Asset/setup changes can require the ordinary pickup spawn during paste.
 regenerateRackStock=false;duplicateTool(*rack);edits.clear();duplicatePaste();regenerateRackStock=true;
 CHECK(state.ownedCount==4&&rackStockAttached(g_entities[state.owned[3].entity]));
 const auto& fallback=g_entities[state.owned[3].entity];CHECK(!fallback.nextthink&&fallback.e_ThinkFunc==thinkF_NULL);
 undoEdit();CHECK(state.ownedCount==2);
 rackStockLoose(g_entities[state.owned[1].entity]);duplicateTool(*rack);
 CHECK(duplicateSnapshot.objects.size()==2&&!duplicateSnapshot.objects[1].rackShelfStock);
 edits.clear();duplicatePaste();CHECK(state.ownedCount==4&&!(g_entities[state.owned[3].entity].flags&FL_FORCE_PULLABLE_ONLY));
 undoEdit();CHECK(state.ownedCount==2);remove(state.owned[1].entity,true);duplicateTool(*rack);
 edits.clear();duplicatePaste();CHECK(state.ownedCount==2&&edits.back().group.size()==1);undoEdit();CHECK(state.ownedCount==1);
 // Late native children (camera death sparks) use the same saved ownership
 // registry. Unknown classes and exhaustion cannot create untracked objects.
 auto&effect=g_entities[nextSlot++];effect.inuse=qtrue;effect.s.number=int(nextSlot-1);effect.classname=G_NewString("fx_runner");
 CHECK(trackChild(rack,&effect));CHECK(owned(effect.s.number)->parent==owned(rack->s.number)->id&&matches(*owned(effect.s.number)));
 const auto saved=Encode(state);CHECK(Decode(saved.data(),saved.size(),decoded,ENTITYNUM_WORLD)&&decoded.ownedCount==2&&decoded.owned[1].parent==decoded.owned[0].id);
 auto&unknown=g_entities[nextSlot++];unknown.inuse=qtrue;unknown.s.number=int(nextSlot-1);unknown.classname=G_NewString("not_a_catalog_entity");CHECK(!trackChild(rack,&unknown));
 unknown.classname=G_NewString("fx_runner");const auto count=state.ownedCount;state.ownedCount=ownedLimit;CHECK(!trackChild(rack,&unknown));state.ownedCount=count;CHECK(!owned(unknown.s.number));
 gitem_t gun{};gun.giType=IT_WEAPON;gun.giTag=WP_BLASTER;unknown.item=&gun;CHECK(childCatalog(unknown)==2003);
 unknown.item=&pickup;CHECK(childCatalog(unknown)==2501);
 gentity_t authored{};authored.s.number=5;authored.inuse=qtrue;CHECK(trackChild(&authored,nullptr));
 // Console roots are counted through the same registry and keep their native
 // target names. More than 64 consecutive spawn records remain undoable.
 edits.clear();const auto beforeConsole=state.ownedCount;
 for(int i=0;i<140;++i){auto* e=Make(Catalog("target_print"),p,a);auto* record=owned(e->s.number);record->catalog=consoleCatalog;e->targetname=nullptr;rememberEdit(*e,true);}
 CHECK(edits.size()==140&&state.ownedCount==beforeConsole+140);
 duplicateTool(g_entities[state.owned.back().entity]);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].catalog==Catalog("target_print"));
 for(int i=0;i<140;++i)undoEdit();CHECK(edits.empty()&&state.ownedCount==beforeConsole);
 {
  WorkshopEditHistory capacityHistory;
  const unsigned last=ENTITYNUM_WORLD-1;
  for(unsigned slot=1;slot<last-1;++slot){
   Edit spawn{};spawn.entity=slot;spawn.spawn=true;
   capacityHistory.push_back(std::move(spawn));
  }
  Edit assembly{};assembly.group={{last-1,1,last-1},{last,1,last}};
  capacityHistory.push_back(std::move(assembly));
  for(unsigned slot=1;slot<=65;++slot){
   Edit property{};property.entity=slot;property.context=true;property.healthEdit=true;
   capacityHistory.push_back(std::move(property));
  }
  const auto property=[](const Edit& edit){return edit.context;};
  const auto creation=[](const Edit& edit){return isCreation(edit);};
  CHECK(capacityHistory.size()==last-1+64);
  CHECK(capacityHistory.lastFor(1,property)==capacityHistory.all().end());
  CHECK(capacityHistory.lastFor(2,property)!=capacityHistory.all().end());
  capacityHistory.forgetEntity(2);
  CHECK(capacityHistory.lastFor(2,creation)==capacityHistory.all().end());
  const auto groupKey=capacityHistory.lastFor(last,creation)->first;
  capacityHistory.forgetEntity(last);
  CHECK(capacityHistory.all().at(groupKey).group.size()==1);
  CHECK(capacityHistory.all().at(groupKey).group.front().entity==last-1);
  Edit reused{};reused.entity=last;reused.generation=2;reused.spawn=true;
  capacityHistory.push_back(std::move(reused));
  CHECK(capacityHistory.lastFor(last,creation)->first!=groupKey);
  capacityHistory.forgetEntity(last-1);
  CHECK(!capacityHistory.all().count(groupKey)&&capacityHistory.lastFor(last,creation)->second.generation==2);

  // Retire every remaining slot, including ones with both kinds of history.
  for(unsigned slot=1;slot<=last;++slot)capacityHistory.forgetEntity(slot);
  CHECK(capacityHistory.empty());
 }
 {
  auto* older=Make(1001,p,a);rememberEdit(*older,true);
  auto* copyA=Make(1001,p,a);auto* copyB=Make(1001,p,a);
  Edit pasted{};
  for(auto* member:{copyA,copyB})pasted.group.push_back({
   unsigned(member->s.number),entityGeneration[member->s.number],owned(member->s.number)->id});
  rememberAction(std::move(pasted));
  jar::constraint::Joint mount;mount.kind=jar::constraint::Thruster;
  mount.first=mount.second=owned(older->s.number)->id;mount.force=6000;
  mount.a.rotation[0]=1;mount.a.rotation[3]=0;mount.b=mount.a;
  const auto joint=AddJoint(state,mount);CHECK(joint);
  Edit attachment{};attachment.joint=joint;rememberAction(std::move(attachment));
  {WorkshopActionScope context(recordingContextEdit);rememberEdit(*older);}
  MemorySave bytes;ojk::SavedGameHelper save(&bytes);writeUndoHistory(false,save);
  edits.clear();++entityGeneration[older->s.number];++entityGeneration[copyB->s.number];
  bytes.reset_buffer_offset();CHECK(readUndoHistory(false,save)&&edits.size()==3);
  undoEdit();CHECK(older->inuse&&copyA->inuse&&copyB->inuse&&edits.size()==2);
  busy=copyB->s.number;undoEdit();CHECK(copyA->inuse&&copyB->inuse&&edits.size()==2);
  busy=noEntity;undoEdit();CHECK(!copyA->inuse&&!copyB->inuse&&older->inuse&&edits.size()==1);
  undoEdit();CHECK(!older->inuse&&edits.empty()&&state.ownedCount==beforeConsole);
 }
 {
  auto* root=Make(Catalog("target_print"),p,a);rememberEdit(*root,true);
  auto* helper=Make(Catalog("target_print"),p,a);
  auto* actor=Make(3001,p,a);
  CHECK(SetOwnershipParent(state,*owned(helper->s.number),owned(root->s.number)->id));
  CHECK(SetOwnershipParent(state,*owned(actor->s.number),owned(helper->s.number)->id));
  const int feedback=undoFeedbackCount;
  busy=actor->s.number;
  CHECK(!despawnAllAvailable());
  undoEdit();
  CHECK(root->inuse&&helper->inuse&&actor->inuse&&edits.size()==1&&undoFeedbackCount==feedback);
  CHECK(!remove(helper->s.number)&&root->inuse&&helper->inuse&&actor->inuse);
  busy=noEntity;
  CHECK(despawnAllAvailable());
  undoEdit();
  CHECK(!root->inuse&&!helper->inuse&&!actor->inuse&&edits.empty()&&undoFeedbackCount==feedback+1);
 }
 {
  auto* endpoint=Make(Catalog("target_print"),p,a);rememberEdit(*endpoint,true);
  auto* sameName=G_TrySpawn();sameName->targetname=G_NewString(endpoint->targetname);
  auto* door=Make(Catalog("func_door"),p,a);door->model=G_NewString(JAR_SLIDING_DOOR_PORTABLE);
  auto* button=Make(Catalog("func_button"),p,a);button->model=G_NewString(JAR_BUTTON_PORTABLE_MODEL);
  auto* spotlight=Make(Catalog("misc_spotlight"),p,a);spotlight->model=G_NewString("@jar_spotlight");
  auto* bidirectional=Make(Catalog("trigger_bidirectional"),p,a);
  auto* distance=Make(Catalog("trigger_entdist"),p,a);distance->model=G_NewString("@jar_entdist");
  auto* relay=Make(Catalog("target_relay"),p,a);relay->activator=endpoint;
  auto* travel=Make(Catalog("target_level_change"),p,a);
  for(auto* controller:{door,button,spotlight})controller->target2=G_NewString(endpoint->targetname);
  for(int role=0;role<JAR_DISTANCE_LINKS;++role)G_DistanceSetLink(*distance,role,endpoint->targetname);
  bidirectional->paintarget=G_NewString(endpoint->targetname);
  relay->target=G_NewString(endpoint->targetname);
  travel->target=G_NewString(endpoint->targetname);
  cleanupSelected=distance->s.number;refreshAfterCleanup=false;
  const vec3_t fallen={0,0,-512};workshopHostFallFloor=-256;
  CHECK(removeOutsideMap(*owned(endpoint->s.number),fallen,endpoint->mins,endpoint->maxs));
  CHECK(!endpoint->inuse&&sameName->inuse&&door->target2&&distance->ownername&&relay->target&&!relay->activator&&!refreshAfterCleanup);
  G_FreeEntity(sameName);
  CHECK(!door->target2&&!button->target2&&!spotlight->target2&&!bidirectional->paintarget&&!relay->target&&travel->target);
  CHECK(!distance->target&&!distance->target2&&!distance->NPC_target&&
        !strcmp(distance->ownername,JAR_DISTANCE_MISSING_REFERENCE_NAME)&&refreshAfterCleanup);
  CHECK(edits.empty());
  cleanupSelected=noEntity;workshopHostFallFloor=MIN_WORLD_COORD;
  for(auto* controller:{door,button,spotlight,bidirectional,distance,relay,travel})CHECK(remove(controller->s.number));
 }
 {
  auto* beam=Make(Catalog("fx_target_beam"),p,a);
  auto* first=Make(Catalog("target_position"),p,a);
  beam->target=G_NewString(first->targetname);
  beam->activator=first;
  beam->e_ThinkFunc=thinkF_fx_target_beam_link;beam->nextthink=level.time+100;
  VectorSet(beam->currentOrigin,16,24,32);VectorSet(beam->s.origin2,900,700,400);
  CHECK(remove(first->s.number));
  CHECK(beam->inuse&&!beam->target&&!beam->activator&&!beam->nextthink&&beam->e_ThinkFunc==thinkF_NULL);
  CHECK(beam->e_UseFunc==useF_fx_target_beam_use&&(beam->svFlags&SVF_PLAYER_USABLE));
  CHECK(VectorCompare(beam->s.origin2,beam->currentOrigin));
  vec3_t aim;
  CHECK(!G_WorkshopBeamAim(*beam,aim));

  auto* linked=Make(Catalog("target_position"),p,a);
  auto* duplicate=G_TrySpawn();duplicate->targetname=G_NewString(linked->targetname);
  VectorSet(linked->currentOrigin,120,200,300);VectorSet(duplicate->currentOrigin,-60,50,40);
  beam->target=G_NewString(linked->targetname);beam->enemy=first;
  CHECK(G_WorkshopBeamAim(*beam,aim)&&beam->enemy==linked&&VectorCompare(aim,linked->currentOrigin));
  linked->currentOrigin[1]=321;
  CHECK(G_WorkshopBeamAim(*beam,aim)&&aim[1]==321);
  hiddenBeamEndpoint=linked;
  CHECK(G_WorkshopBeamAim(*beam,aim)&&beam->enemy==duplicate);
  hiddenBeamEndpoint=nullptr;
  beam->enemy=nullptr;
  CHECK(G_WorkshopBeamAim(*beam,aim)&&beam->enemy==linked);
  CHECK(remove(linked->s.number)&&beam->target&&!beam->enemy);
  CHECK(G_WorkshopBeamAim(*beam,aim)&&beam->enemy==duplicate&&VectorCompare(aim,duplicate->currentOrigin));
  G_FreeEntity(duplicate);
  CHECK(!G_WorkshopBeamAim(*beam,aim)&&!beam->target&&beam->inuse);
  CHECK(remove(beam->s.number));
 }
 {
  auto* trail=Make(Catalog("fx_explosion_trail"),p,a);
  auto* target=Make(Catalog("target_position"),p,a);
  G_WorkshopTrailEnableUse(*trail);
  trail->target=G_NewString(target->targetname);
  VectorSet(trail->currentOrigin,20,10,0);VectorSet(trail->currentAngles,-30,90,0);
  VectorSet(target->currentOrigin,20,40,40);
  vec3_t direction,originalAngles;
  VectorCopy(trail->currentAngles,originalAngles);
  G_WorkshopTrailDirection(*trail,direction);
  CHECK(std::fabs(direction[1]-.6f)<.001f&&std::fabs(direction[2]-.8f)<.001f);
  target->currentOrigin[1]=-20;
  G_WorkshopTrailDirection(*trail,direction);CHECK(direction[1]<-.59f&&VectorCompare(originalAngles,trail->currentAngles));
  const auto previousSelection=selected;
  selected=trail->s.number;
  {WorkshopActionScope context(recordingContextEdit);editExplosionTrailAim(*trail,nullptr);}
  ++entityGeneration[target->s.number];
  const auto count=edits.size();
  revertProperty();CHECK(!trail->target&&edits.size()==count);
  edits.pop_back();selected=previousSelection;
  editExplosionTrailAim(*trail,target->targetname);
  CHECK(remove(target->s.number)&&trail->inuse&&!trail->target);
  G_WorkshopTrailDirection(*trail,direction);
  CHECK(std::fabs(VectorLength(direction)-1)<.001f&&std::fabs(direction[2]-.5f)<.001f&&direction[1]>.86f);

  auto* flight=G_TrySpawn();flight->owner=trail;flight->e_ThinkFunc=thinkF_fx_explosion_trail_think;
  G_WorkshopTrailOrientFlight(*flight,direction);
  vec3_t forward;AngleVectors(flight->currentAngles,forward,nullptr,nullptr);
  CHECK(Distance(forward,direction)<.001f&&VectorCompare(flight->movedir,direction));
  CHECK(remove(trail->s.number)&&flight->inuse&&!flight->owner);G_FreeEntity(flight);

  gentity_t legacy{};legacy.e_UseFunc=useF_fx_explosion_trail_use;
  VectorSet(legacy.currentAngles,0,1,0);vec3_t oldFacing={0,-1,0};
  trajectory_t heldRotation{};heldRotation.trType=TR_STATIONARY;VectorCopy(oldFacing,heldRotation.trBase);
  G_WorkshopTrailRestore(legacy,true,&heldRotation,oldFacing);
  CHECK(legacy.currentAngles[YAW]==90&&oldFacing[YAW]==270&&VectorCompare(heldRotation.trBase,oldFacing));
  VectorSet(legacy.currentAngles,0,1,0);
  G_WorkshopTrailRestore(legacy,false);
  CHECK(legacy.currentAngles[YAW]==1);
 }
 {
  const auto trace=gi.trace;
  const int previousTime=level.time;
  const float previousFloor=workshopHostFallFloor;
  auto* previousGravity=g_gravity;
  static cvar_t trailGravity{};trailGravity.value=800;g_gravity=&trailGravity;
  static float wall=55;static int traces=0;static bool noImpact=false;
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision mode,int){
   CHECK(pass==ENTITYNUM_NONE&&mask==MASK_SHOT&&mode==G2_RETURNONHIT);
   ++traces;*hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(start[0]<wall&&end[0]>wall){
    hit->fraction=(wall-start[0])/(end[0]-start[0]);
    hit->endpos[0]=wall;hit->plane.normal[0]=-1;
    if(noImpact)hit->surfaceFlags=SURF_NOIMPACT;
   }
  };
  auto launch=[](){
   auto* entity=G_TrySpawn();entity->classname=G_NewString("fx_exp_trail");
   entity->e_ThinkFunc=thinkF_fx_explosion_trail_think;entity->s.eType=ET_MOVER;
   entity->s.pos.trType=TR_LINEAR;entity->s.pos.trTime=1000;entity->nextthink=1050;
   VectorSet(entity->movedir,1,0,0);VectorSet(entity->s.pos.trDelta,1000,0,0);
   entity->clipmask=MASK_SHOT;entity->damage=12;entity->radius=50;
   entity->splashDamage=30;entity->splashRadius=100;entity->fxID=4;
   entity->cameraGroup=G_NewString("impact");entity->soundSet=G_NewString("trail_sound");
   return entity;
  };
  runTrailPhysics=true;workshopHostFallFloor=-256;
  auto* moving=launch();
  level.time=1020;CHECK(G_WorkshopTrailFrame(*moving));
  CHECK(std::fabs(moving->currentOrigin[0]-20)<.001f&&traces==1&&trailPulses==0);
  level.time=1050;CHECK(G_WorkshopTrailFrame(*moving));
  CHECK(std::fabs(moving->currentOrigin[0]-50)<.001f&&traces==2&&trailPulses==1&&trailEffects==1);
  gentity_t sound{};trailSoundEvent=&sound;
  level.time=1060;CHECK(G_WorkshopTrailFrame(*moving)&&!moving->inuse);
  CHECK(traces==3&&trailImpacts==1&&trailPulses==1&&trailEffects==2&&trailDamagePoint[0]==55);
  CHECK(sound.s.eventParm==7&&sound.currentOrigin[0]==55);
  trailSoundEvent=nullptr;
  noImpact=true;moving=launch();
  CHECK(G_WorkshopTrailFrame(*moving)&&!moving->inuse&&trailImpacts==1&&trailEffects==2);

  moving=launch();moving->s.pos.trType=TR_GRAVITY;
  VectorSet(moving->currentOrigin,0,0,-254);VectorSet(moving->s.pos.trBase,0,0,-254);
  VectorClear(moving->s.pos.trDelta);level.time=1100;
  CHECK(G_WorkshopTrailFrame(*moving)&&!moving->inuse&&trailPulses==1&&trailEffects==2);
  moving=launch();level.time=31000;
  const int beforeExpiry=traces;
  CHECK(G_WorkshopTrailFrame(*moving)&&!moving->inuse&&traces==beforeExpiry);
  auto* authored=launch();VectorClear(authored->movedir);
  CHECK(!G_WorkshopTrailFrame(*authored)&&authored->inuse);G_FreeEntity(authored);
  runTrailPhysics=false;gi.trace=trace;g_gravity=previousGravity;
  workshopHostFallFloor=previousFloor;level.time=previousTime;
 }
 {
  auto* replaced=Make(Catalog("target_print"),p,a);rememberEdit(*replaced,true);
  auto* reusedChild=Make(Catalog("target_print"),p,a);
  auto* occupiedChild=Make(3001,p,a);
  auto* watcher=Make(Catalog("target_relay"),p,a);
  CHECK(SetOwnershipParent(state,*owned(reusedChild->s.number),owned(replaced->s.number)->id));CHECK(SetOwnershipParent(state,*owned(occupiedChild->s.number),owned(replaced->s.number)->id));
  replaced->targetname=G_NewString("authored_reused_root");reusedChild->targetname=G_NewString("authored_reused_child");
  watcher->target=G_NewString(reusedChild->targetname);
  busy=occupiedChild->s.number;
  G_JAR_WorkshopForget(replaced,false);
  CHECK(replaced->inuse&&reusedChild->inuse&&occupiedChild->inuse&&watcher->target&&
        !owned(replaced->s.number)&&!owned(reusedChild->s.number)&&!owned(occupiedChild->s.number)->parent);
  MemorySave bytes;ojk::SavedGameHelper save(&bytes);writeUndoHistory(false,save);
  edits.clear();bytes.reset_buffer_offset();CHECK(readUndoHistory(false,save)&&edits.size()==1);
  undoEdit();CHECK(occupiedChild->inuse&&edits.size()==1);
  busy=noEntity;undoEdit();CHECK(!occupiedChild->inuse&&reusedChild->inuse&&watcher->target&&edits.empty());
  G_FreeEntity(reusedChild);G_FreeEntity(replaced);
  CHECK(!watcher->target&&remove(watcher->s.number));
 }
 {
  auto* distance=Make(Catalog("trigger_entdist"),p,a);distance->model=G_NewString("@jar_entdist");
  auto* before=Make(Catalog("target_position"),p,a);
  auto* after=Make(Catalog("target_position"),p,a);
  G_DistanceSetLink(*distance,JAR_DISTANCE_REFERENCE,before->targetname);
  {WorkshopActionScope context(recordingContextEdit);distanceLink(*distance,JAR_DISTANCE_REFERENCE,after->targetname);}
  selected=distance->s.number;revertProperty();CHECK(!strcmp(distance->ownername,before->targetname)&&edits.empty());
  {WorkshopActionScope context(recordingContextEdit);distanceLink(*distance,JAR_DISTANCE_REFERENCE,after->targetname);}
  ++entityGeneration[before->s.number];
  revertProperty();CHECK(!strcmp(distance->ownername,after->targetname)&&edits.size()==1);
  CHECK(remove(after->s.number));revertProperty();
  CHECK(!strcmp(distance->ownername,JAR_DISTANCE_MISSING_REFERENCE_NAME)&&edits.size()==1);
  CHECK(remove(distance->s.number)&&remove(before->s.number)&&edits.empty());
 }
 {
  const auto worldBounds=gi.JAR_WorldBounds;
  gi.JAR_WorldBounds=[](vec3_t mins,vec3_t maxs){VectorSet(mins,-128,-128,-1024);VectorSet(maxs,128,128,1024);};
  auto* room=spawn(Catalog("Vjun Sentinel"),p,a);
  CHECK(room&&jar::bspSection::Portable(*room));
  room->linked=qtrue;
  VectorSet(room->currentOrigin,1000,0,-4750);
  VectorClear(room->currentAngles);
  VectorSet(room->mins,-100,-100,-250);VectorSet(room->maxs,100,100,250);
  updateWorkshopFallSections();
  const vec3_t mins={-8,-8,-8},maxs={8,8,8};
  const vec3_t outside={0,0,-2000},approaching={1000,0,-2000},below={1000,0,-5300};
  const vec3_t edge={893,0,-2000},clear={891,0,-2000},offWorld={MAX_WORLD_COORD,0,-2000};
  CHECK(outsideWorkshopMap(outside,mins,maxs)&&outsideWorkshopMap(below,mins,maxs));
  CHECK(!outsideWorkshopMap(approaching,mins,maxs)&&!outsideWorkshopMap(edge,mins,maxs));
  CHECK(outsideWorkshopMap(clear,mins,maxs)&&outsideWorkshopMap(offWorld,mins,maxs));
  // The same linked brush's rotated broad-phase cube exceeds its footprint.
  VectorSet(room->mins,-40,-100,-250);VectorSet(room->maxs,40,100,250);
  room->currentAngles[YAW]=90;
  VectorSet(room->absmin,725,-275,-5025);VectorSet(room->absmax,1275,275,-4475);
  const vec3_t outsideRotated={1000,80,-2000},insideRotated={1000,47,-2000};
  CHECK(outsideWorkshopMap(outsideRotated,mins,maxs)&&!outsideWorkshopMap(insideRotated,mins,maxs));
  room->currentAngles[YAW]=45;
  const vec3_t emptyCorner={1075,75,-2000},rotatedInterior={1038,-38,-2000};
  CHECK(outsideWorkshopMap(emptyCorner,mins,maxs)&&!outsideWorkshopMap(rotatedInterior,mins,maxs));
  // A tilted room has different lower boundaries beneath the high and low ends.
  VectorSet(room->currentAngles,45,0,0);
  VectorSet(room->mins,-128,-50,-10);VectorSet(room->maxs,128,50,10);
  const vec3_t aboveHighEnd={930,0,-5020},aboveLowEnd={1070,0,-5020};
  CHECK(outsideWorkshopMap(aboveHighEnd,mins,maxs)&&!outsideWorkshopMap(aboveLowEnd,mins,maxs));
  const vec3_t columnLow={922,-8,0},columnHigh={938,8,0};
  float clippedBottom=0;
  CHECK(workshopSectionColumnBottom(*room,columnLow,columnHigh,clippedBottom));
  CHECK(std::fabs(clippedBottom-(-4750+62-std::sqrt(200.f)))<.002f);
  auto* falling=Make(1001,outside,a);
  busy=falling->s.number;
  CHECK(!removeOutsideMap(*owned(falling->s.number),outside,mins,maxs)&&falling->inuse);
  busy=noEntity;
  CHECK(removeOutsideMap(*owned(falling->s.number),outside,mins,maxs)&&!falling->inuse);
  falling=Make(1001,approaching,a);
  CHECK(!removeOutsideMap(*owned(falling->s.number),approaching,mins,maxs)&&falling->inuse);
  room->linked=qfalse;
  CHECK(outsideWorkshopMap(approaching,mins,maxs));
  room->linked=qtrue;
  ++entityGeneration[room->s.number];
  CHECK(outsideWorkshopMap(approaching,mins,maxs));
  updateWorkshopFallSections();
  CHECK(!outsideWorkshopMap(approaching,mins,maxs));
  CHECK(remove(falling->s.number)&&remove(room->s.number));
  CHECK(outsideWorkshopMap(approaching,mins,maxs));
  gi.JAR_WorldBounds=worldBounds;workshopFallSections.clear();workshopHostFallFloor=MIN_WORLD_COORD;
 }
 {
  auto* root=Make(Catalog("target_print"),p,a);rememberEdit(*root,true);
  auto* drop=Make(2501,p,a);
  CHECK(SetOwnershipParent(state,*owned(drop->s.number),owned(root->s.number)->id));
  const vec3_t fallen={0,0,-512};
  workshopHostFallFloor=-256;
  drop->flags|=FL_DROPPED_ITEM;
  CHECK(!removeOutsideMap(*owned(drop->s.number),fallen,drop->mins,drop->maxs));
  drop->s.pos.trType=TR_GRAVITY;
  CHECK(removeOutsideMap(*owned(drop->s.number),fallen,drop->mins,drop->maxs));
  workshopHostFallFloor=MIN_WORLD_COORD;
  CHECK(!drop->inuse&&root->inuse&&edits.size()==1);
  undoEdit();CHECK(!root->inuse&&edits.empty()&&state.ownedCount==beforeConsole);
 }
 {
  // Live Undo cannot select an owned object with no creation record.
  auto* unrecorded=Make(1001,p,a);
  const int feedback=undoFeedbackCount;
  {WorkshopActionScope context(recordingContextEdit);rememberEdit(*unrecorded);}
  undoEdit();CHECK(unrecorded->inuse&&edits.size()==1&&undoFeedbackCount==feedback);
  // Older saves infer their spawn order from the ownership registry once.
  edits.clear();restoreLegacySpawnHistory();
  undoEdit();CHECK(!unrecorded->inuse&&undoFeedbackCount==feedback+1);
  edits.clear();
 }
 {
  auto* lock=spawn(Catalog("misc_maglock"),p,a);auto* door=lock->activator;
  auto* newer=spawn(1001,p,a);
  detachChild(lock,door);G_FreeEntity(lock);
  CHECK(door->inuse&&edits.size()==2);
  undoEdit();CHECK(!newer->inuse&&door->inuse);
  undoEdit();CHECK(!door->inuse&&edits.empty());
 }
 {
  auto* body=Make(1001,p,a);rememberEdit(*body,true);
  jar::constraint::Joint mount;mount.kind=jar::constraint::Thruster;
  mount.first=mount.second=owned(body->s.number)->id;mount.force=6000;mount.a.rotation[0]=1;mount.a.rotation[3]=0;mount.b=mount.a;
  mount.id=AddJoint(state,mount);CHECK(mount.id);
  Edit creation{};creation.joint=mount.id;rememberAction(std::move(creation));
  state.joints[state.jointCount-1].enabled=0;
  rememberEdit(*body);CHECK(edits.size()==2);
  const auto jointCount=state.jointCount;
  undoEdit();CHECK(body->inuse&&state.jointCount+1==jointCount);
  undoEdit();CHECK(!body->inuse&&edits.empty());
 }
 // Copied music retains track properties, not activation history or identity.

 for(const char*type:{"rail_track","rail_lane","rail_mover"}){auto*r=spawn(Catalog(type),p,a);jar::rail::Settings rs;CHECK(G_RailRead(*r,rs));rs.speed=400;rs.width=320;rs.delay=2;rs.enabled=false;rs.center=true;CHECK(G_RailSet(*r,rs));r->count=1;duplicateTool(*r);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].hasRail);const auto beforeRail=state.ownedCount;edits.clear();duplicatePaste();auto&rc=g_entities[selected];jar::rail::Settings copied;CHECK(&rc!=r&&state.ownedCount==beforeRail+3&&G_RailRead(rc,copied)&&copied.kind==rs.kind&&copied.delay==2&&!copied.enabled&&copied.center&&!rc.count);if(rs.kind==jar::rail::Track)CHECK(copied.speed==400&&copied.width==320);gentity_t*parts[4]{};parts[copied.kind]=&rc;for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].parent==owned(rc.s.number)->id){auto&child=g_entities[state.owned[i].entity];parts[jar::rail::Class(child.classname)]=&child;}CHECK(parts[1]&&parts[2]&&parts[3]&&!strcmp(parts[2]->target,parts[1]->targetname)&&!strcmp(parts[3]->target,parts[2]->targetname));undoEdit();CHECK(state.ownedCount==beforeRail&&!rc.inuse&&r->inuse);}
 auto*travel=Make(Catalog("target_level_change"),p,a);auto*unrelated=Make(Catalog("target_print"),p,a);travel->target=G_NewString(unrelated->targetname);travel->spawnflags=1;
 duplicateTool(*travel);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasLevelChange&&!duplicateSnapshot.objects[0].targetCaptured);
 edits.clear();duplicatePaste();auto&travelCopy=g_entities[selected];CHECK(&travelCopy!=travel&&!strcmp(travelCopy.message,"hoth2")&&!strcmp(travelCopy.target,travel->target)&&(travelCopy.spawnflags&1));undoEdit();CHECK(!travelCopy.inuse&&travel->inuse&&unrelated->inuse);
 const auto beforeBadTravel=state.ownedCount;duplicateSnapshot.objects[0].levelChange.map="missing";duplicatePaste();CHECK(state.ownedCount==beforeBadTravel&&travel->inuse);remove(travel->s.number,true);remove(unrelated->s.number,true);
 auto*music=Make(Catalog("target_play_music"),p,a);music->message=G_NewString("music/death_music.mp3 music/endcredits.mp3");duplicateTool(*music);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasMusic);
 edits.clear();duplicatePaste();CHECK(selected!=unsigned(music->s.number)&&!strcmp(g_entities[selected].message,music->message)&&strcmp(g_entities[selected].targetname,music->targetname));undoEdit();CHECK(music->inuse);
 auto*probe=Make(Catalog("misc_cubemap"),p,a);jar::reflection::Settings probeSettings;probeSettings.radius=1024;probeSettings.enabled=false;CHECK(G_ReflectionSet(*probe,probeSettings));probe->count=17;
 duplicateTool(*probe);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasReflection);edits.clear();duplicatePaste();auto&probeCopy=g_entities[selected];CHECK(&probeCopy!=probe&&probeCopy.radius==1024&&(probeCopy.spawnflags&1)&&probeCopy.count==1&&strcmp(probeCopy.targetname,probe->targetname));undoEdit();CHECK(!probeCopy.inuse&&probe->inuse&&probe->count==17);
 duplicateSnapshot.objects[0].reflection.radius=15;const auto probeOwned=state.ownedCount;duplicatePaste();CHECK(state.ownedCount==probeOwned&&probe->inuse);
 auto*cloud=Make(Catalog("fx_cloudlayer"),p,a);jar::cloud::Settings cloudSettings;cloudSettings.radius=1024;cloudSettings.inner=256;cloudSettings.curve=-2;cloudSettings.tube=cloudSettings.alternate=true;cloudSettings.enabled=false;CHECK(G_CloudSet(*cloud,cloudSettings));
 duplicateTool(*cloud);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasCloud);edits.clear();duplicatePaste();auto&cloudCopy=g_entities[selected];CHECK(&cloudCopy!=cloud&&cloudCopy.radius==1024&&cloudCopy.random==256&&cloudCopy.wait==-2&&cloudCopy.spawnflags==7);undoEdit();CHECK(!cloudCopy.inuse&&cloud->inuse);
 for(const char*type:{"misc_portal_surface","misc_portal_camera"}){auto*portal=spawn(Catalog(type),p,a);jar::portal::Settings ps;ps.width=256;ps.height=384;ps.mirror=true;ps.enabled=false;CHECK(G_PortalSet(*portal,ps));duplicateTool(*portal);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].hasPortal&&duplicateSnapshot.objects[1].hasPortal);const auto beforePortal=state.ownedCount;edits.clear();duplicatePaste();auto&pc=g_entities[selected];CHECK(&pc!=portal&&state.ownedCount==beforePortal+2&&pc.radius==256&&pc.random==384&&pc.count==1&&(pc.spawnflags&128));auto*root=owned(selected);gentity_t*child=nullptr;for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].parent==root->id)child=&g_entities[state.owned[i].entity];CHECK(child);auto*s=G_PortalSurface(pc)?&pc:child;auto*c=s==&pc?child:&pc;CHECK(!strcmp(s->target,c->targetname));undoEdit();CHECK(state.ownedCount==beforePortal&&!pc.inuse&&portal->inuse);}
 for(const char*type:{"ref_tag","waypoint_navgoal"}){auto*r=spawn(Catalog(type),p,a);jar::reference::Settings settings;CHECK(G_ReferenceRead(*r,settings));settings.name="original_ref";settings.radius=73;CHECK(G_ReferenceSet(*r,settings));duplicateTool(*r);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasReference);edits.clear();const auto beforeReference=state.ownedCount;duplicatePaste();auto&rc=g_entities[selected];CHECK(&rc!=r&&state.ownedCount==beforeReference+1&&strcmp(rc.message,r->message)&&rc.radius==73);const std::string copiedName=rc.message;CHECK(G_ReferenceFind(nullptr,copiedName.c_str()));undoEdit();CHECK(!rc.inuse&&r->inuse&&state.ownedCount==beforeReference&&!G_ReferenceFind(nullptr,copiedName.c_str())&&G_ReferenceFind(nullptr,"original_ref"));remove(r->s.number);}
 auto*forceTrain=spawn(Catalog("func_train"),p,a);vec3_t forceOffset={45,12,-4},copiedOffset;CHECK(G_JAR_MoverSetOffset(*forceTrain,forceOffset));vec3_t sourceSpin={0,0,2};G_JAR_MoverSpin(*forceTrain,sourceSpin);duplicateTool(*forceTrain);CHECK(duplicateSnapshot.objects[0].hasMoverOffset);const auto beforeForceTrain=state.ownedCount;edits.clear();duplicatePaste();auto&forceTrainCopy=g_entities[selected];G_JAR_MoverPathOffset(forceTrainCopy,copiedOffset);CHECK(&forceTrainCopy!=forceTrain&&VectorCompare(forceOffset,copiedOffset)&&state.ownedCount==beforeForceTrain+3);JarMoverMotion copiedMotion;CHECK(G_JAR_MoverMotionRead(forceTrainCopy,copiedMotion)&&VectorLength(copiedMotion.angular)==0);CHECK(G_JAR_MoverMotionRead(*forceTrain,copiedMotion)&&copiedMotion.angular[2]==2);undoEdit();CHECK(!forceTrainCopy.inuse&&forceTrain->inuse&&state.ownedCount==beforeForceTrain);
 auto*crateAssembly=spawn(Catalog("misc_crystal_crate"),p,a);jar::crystalCrate::Settings cs;cs.hitDamage=7;cs.hitRadius=52;cs.blastDamage=120;cs.blastRadius=256;cs.solid=false;CHECK(G_CrystalCrateSet(*crateAssembly,cs));crateAssembly->health=37;duplicateTool(*crateAssembly);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].hasCrystal);const auto beforeCrate=state.ownedCount;edits.clear();duplicatePaste();auto&crateCopy=g_entities[selected];CHECK(&crateCopy!=crateAssembly&&state.ownedCount==beforeCrate+2&&crateCopy.health==37&&crateCopy.damage==7&&crateCopy.radius==52&&crateCopy.splashDamage==120&&crateCopy.splashRadius==256&&(crateCopy.spawnflags&1));undoEdit();CHECK(!crateCopy.inuse&&crateAssembly->inuse&&state.ownedCount==beforeCrate);
 auto*eyeAssembly=spawn(Catalog("misc_model_jabba_cam"),p,a);jar::doorCamera::Settings ds;ds.travel=96;ds.speed=240;ds.extended=true;CHECK(G_DoorCameraSet(*eyeAssembly,ds));eyeAssembly->random=32;duplicateTool(*eyeAssembly);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].hasDoorCamera&&duplicateSnapshot.objects[0].doorExtension==32);const auto beforeDoor=state.ownedCount;edits.clear();duplicatePaste();auto&doorCopy=g_entities[selected];CHECK(&doorCopy!=eyeAssembly&&state.ownedCount==beforeDoor+3&&doorCopy.radius==96&&doorCopy.speed==240&&doorCopy.random==32&&(doorCopy.spawnflags&1));undoEdit();CHECK(!doorCopy.inuse&&eyeAssembly->inuse&&state.ownedCount==beforeDoor);
 auto*welder=spawn(Catalog("misc_model_welder"),p,a);jar::welder::Settings ws;ws.reach=64;ws.radius=25.5f;ws.interval=.7f;ws.damage=0;ws.enabled=false;CHECK(G_WelderSet(*welder,ws));duplicateTool(*welder);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasWelder);const auto beforeWelder=state.ownedCount;edits.clear();duplicatePaste();auto&welderCopy=g_entities[selected];CHECK(&welderCopy!=welder&&state.ownedCount==beforeWelder+1&&welderCopy.radius==64&&welderCopy.random==25.5f&&welderCopy.wait==.7f&&!welderCopy.damage&&(welderCopy.spawnflags&1));undoEdit();CHECK(!welderCopy.inuse&&welder->inuse&&state.ownedCount==beforeWelder);
 auto*spot=spawn(Catalog("misc_spotlight"),p,a);jar::spotlight::Settings ss;ss.range=1024;ss.cone=20;ss.rgb[0]=.3f;ss.enabled=false;CHECK(G_SpotlightSet(*spot,ss));duplicateTool(*spot);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].hasSpot&&duplicateSnapshot.objects[0].backwardTarget);const auto beforeSpot=state.ownedCount;edits.clear();duplicatePaste();auto&spotCopy=g_entities[selected];CHECK(&spotCopy!=spot&&state.ownedCount==beforeSpot+3&&spotCopy.wait==1024&&spotCopy.speed==20&&spotCopy.pos1[0]==.3f&&(spotCopy.spawnflags&1));CHECK(strcmp(spotCopy.target,spot->target)&&strcmp(spotCopy.target2,spot->target2));undoEdit();CHECK(!spotCopy.inuse&&spot->inuse&&state.ownedCount==beforeSpot);
 auto*laser=spawn(Catalog("misc_laser_arm"),p,a);jar::laserArm::Settings ls;ls.action=3;ls.yaw=25;ls.pitch=-15;ls.damage=17;ls.duration=8;CHECK(G_LaserArmSet(*laser,ls));duplicateTool(*laser);CHECK(duplicateSnapshot.objects.size()==3&&duplicateSnapshot.objects[0].hasLaser);edits.clear();const auto beforeLaser=state.ownedCount;duplicatePaste();auto&laserCopy=g_entities[selected];CHECK(&laserCopy!=laser&&state.ownedCount==beforeLaser+3&&laserCopy.count==3&&laserCopy.pos2[1]==25&&laserCopy.damage==17&&laserCopy.wait==8&&!laserCopy.alt_fire);undoEdit();CHECK(!laserCopy.inuse&&laser->inuse&&state.ownedCount==beforeLaser);
 const int beforeTiming=level.time;
 level.time=1000;
 CHECK(G_WelderSet(*welder,ws));
 welder->attackDebounceTime=1750;
 duplicateTool(*welder);CHECK(duplicateSnapshot.objects[0].welderCooldownMs==750);
 level.time=2000;duplicatePaste();
 auto& coolingCopy=g_entities[selected];
 CHECK(&coolingCopy!=welder&&coolingCopy.attackDebounceTime==2750&&(coolingCopy.spawnflags&1));
 undoEdit();CHECK(!coolingCopy.inuse&&welder->attackDebounceTime==1750);
 ls.action=0;CHECK(G_LaserArmSet(*laser,ls));
 CHECK(G_LaserArmResumeBurst(*laser,675,45));
 duplicateTool(*laser);CHECK(duplicateSnapshot.objects[0].laserBurstMs==675&&duplicateSnapshot.objects[0].laserSampleMs==45);
 level.time=3000;duplicatePaste();
 auto& firingCopy=g_entities[selected];vec3_t beamStart,beamEnd;
 CHECK(&firingCopy!=laser&&state.ownedCount==beforeLaser+3&&firingCopy.alt_fire&&firingCopy.s.loopSound);
 CHECK(G_LaserArmRoot(firingCopy.lastEnemy->lastEnemy)==&firingCopy);
 CHECK(firingCopy.nextthink==3045&&firingCopy.attackDebounceTime==3675&&!G_LaserArmBeam(firingCopy,beamStart,beamEnd));
 undoEdit();CHECK(!firingCopy.inuse&&laser->inuse&&laser->attackDebounceTime==2675);
 duplicateTool(*laser);CHECK(!duplicateSnapshot.objects[0].laserBurstMs);
 level.time=beforeTiming;
 auto*weather=Make(Catalog("fx_wind"),p,a);jar::weather::Settings weatherSettings;CHECK(G_WeatherRead(*weather,weatherSettings));weatherSettings.mode=2;weatherSettings.speed=2345;weatherSettings.enabled=false;CHECK(G_WeatherSet(*weather,weatherSettings));
 duplicateTool(*weather);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasWeather);edits.clear();duplicatePaste();auto&weatherCopy=g_entities[selected];CHECK(&weatherCopy!=weather&&weatherCopy.speed==2345&&(weatherCopy.spawnflags&128)&&weatherCopy.currentAngles[1]==a[1]);undoEdit();CHECK(!weatherCopy.inuse&&weather->inuse);
 weatherSettings.enabled=true;CHECK(G_WeatherSet(*weather,weatherSettings));duplicateTool(*weather);edits.clear();duplicatePaste();auto&activeCopy=g_entities[selected];CHECK(&activeCopy!=weather&&activeCopy.delay>weather->delay&&!(activeCopy.spawnflags&128));undoEdit();CHECK(!activeCopy.inuse&&weather->inuse);
 for(const char*zoneName:{"fx_wind_zone","misc_weather_zone"}){auto*zone=Make(Catalog(zoneName),p,a);vec3_t extent={32,128,64};CHECK(jar::volumeEntity::Resize(*zone,extent));jar::weather::Settings zs;CHECK(G_WeatherRead(*zone,zs));zs.mode=1;zs.speed=999;zs.enabled=false;CHECK(G_WeatherSet(*zone,zs));duplicateTool(*zone);CHECK(duplicateSnapshot.objects[0].hasWeather&&duplicateSnapshot.objects[0].orientedVolume);edits.clear();duplicatePaste();auto&zoneCopy=g_entities[selected];CHECK(&zoneCopy!=zone&&zoneCopy.pos1[0]==-16&&zoneCopy.pos2[1]==64&&zoneCopy.pos2[2]==64&&zoneCopy.speed==999&&(zoneCopy.spawnflags&128)&&zoneCopy.contents==CONTENTS_TRIGGER&&zoneCopy.currentAngles[YAW]==a[YAW]);undoEdit();CHECK(!zoneCopy.inuse&&zone->inuse);}
 // Copy all 16 literal script parameters without executing counter syntax.
 auto*boundary=spawn(Catalog("trigger_shipboundary"),p,a);boundary->count=1234;boundary->spawnflags=0;vec3_t boundarySize={128,256,512};CHECK(jar::volumeEntity::Resize(*boundary,boundarySize));duplicateTool(*boundary);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].shipBoundary);
 edits.clear();duplicatePaste();auto&boundaryCopy=g_entities[selected];CHECK(&boundaryCopy!=boundary&&boundaryCopy.count==1234&&!(boundaryCopy.spawnflags&1)&&boundaryCopy.target&&strcmp(boundaryCopy.target,boundary->target)&&boundaryCopy.pos2[1]==128&&boundaryCopy.pos2[2]==512);undoEdit();CHECK(!boundaryCopy.inuse&&boundary->inuse);
 auto*runner=Make(Catalog("target_scriptrunner"),p,a);jar::scriptRunner::Settings runnerSettings;runnerSettings.script=jar::scriptRunner::defaultScript;runnerSettings.count=3;runnerSettings.delay=.25f;runnerSettings.wait=2;
 CHECK(G_ScriptRunnerSet(*runner,runnerSettings));runner->nextthink=9999;runner->e_ThinkFunc=thinkF_scriptrunner_run;runner->activator=&g_entities[0];duplicateTool(*runner);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasScript);
 edits.clear();duplicatePaste();auto&runnerCopy=g_entities[selected];CHECK(&runnerCopy!=runner&&runnerCopy.count==3&&runnerCopy.delay==250&&runnerCopy.wait==2000&&runnerCopy.e_UseFunc==useF_target_scriptrunner_use&&runnerCopy.e_ThinkFunc==thinkF_NULL&&!runnerCopy.nextthink&&!runnerCopy.activator&&runnerCopy.behaviorSet[BSET_USE]!=runner->behaviorSet[BSET_USE]);undoEdit();CHECK(!runnerCopy.inuse&&runner->inuse&&runner->nextthink==9999);
 auto ownedBefore=state.ownedCount;scriptMissing=true;duplicatePaste();scriptMissing=false;CHECK(state.ownedCount==ownedBefore&&runner->inuse);
 auto*slide=Make(JAR_SLIDING_DOOR_ID,p,a);
 slide->model=G_NewString(JAR_SLIDING_DOOR_PORTABLE);CHECK(G_SandboxSlideSpawn(*slide));
 jar::slidingDoor::State slideState{{60,140,JAR_SLIDE_UP,1000},60000,375,true,false};
 CHECK(G_SandboxSlideRestore(*slide,slideState));
 auto*slideOutput=Make(Catalog("target_print"),p,a);
 slide->target2=G_NewString(slideOutput->targetname);
 duplicateTool(*slide);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].hasSlidingDoor);
 edits.clear();duplicatePaste();auto&slideCopy=g_entities[selected];jar::slidingDoor::State copiedSlide;
 CHECK(G_SandboxSlideRead(slideCopy,copiedSlide)&&copiedSlide.settings.speed==140&&copiedSlide.extension==60000&&copiedSlide.remainingWait==375);
 CHECK(slideCopy.target2&&strcmp(slideCopy.target2,slide->target2));undoEdit();CHECK(!slideCopy.inuse&&slide->inuse);
 auto*deck=spawn(JAR_LIFT_ID,p,a);
 vec3_t elevated;VectorCopy(deck->currentOrigin,elevated);elevated[2]+=60;
 G_SetOrigin(deck,elevated);
 CHECK(G_SandboxSlideRestore(*deck,{{60,100,JAR_SLIDE_UP,1200},60000,675,true,false}));
 duplicateTool(*deck);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasSlidingDoor);
 edits.clear();duplicatePaste();auto&deckCopy=g_entities[selected];jar::slidingDoor::State copiedDeck;
 CHECK(G_SandboxSlideRead(deckCopy,copiedDeck)&&copiedDeck.extension==60000&&copiedDeck.remainingWait==675);
 CHECK(deckCopy.currentOrigin[2]+deckCopy.mins[2]==61);undoEdit();CHECK(!deckCopy.inuse&&deck->inuse);
 auto* bob=spawn(JAR_BOB_ID,p,vec3_origin);
 CHECK(G_BobbingRestore(*bob,{24,2500,JAR_BOB_TOP,0,1}));
 duplicateTool(*bob);CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects[0].hasBobbing);
 edits.clear();duplicatePaste();auto& bobCopy=g_entities[selected];jarBobState_t copiedBob{};
 CHECK(G_BobbingRead(bobCopy,copiedBob)&&copiedBob.amplitude==24&&copiedBob.periodMs==2500&&
       copiedBob.phase==JAR_BOB_TOP&&!copiedBob.running&&copiedBob.inactive);
 CHECK(bobCopy.currentOrigin[2]+bobCopy.mins[2]==49);undoEdit();CHECK(!bobCopy.inuse&&bob->inuse);
 for(const auto value:{jar::movement::Marker{jar::movement::Gravity,205,false},jar::movement::Marker{jar::movement::Friction,0,true}}){
  auto* marker=spawn(value.kind==jar::movement::Gravity?jar::movement::gravityId:jar::movement::frictionId,p,a);
  CHECK(G_MovementMarkerSet(*marker,value));
  duplicateTool(*marker);CHECK(duplicateSnapshot.objects.size()==1);
  CHECK(jar::movement::Same(duplicateSnapshot.objects[0].movementMarker,value));
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];jar::movement::Marker saved;
  CHECK(G_MovementMarkerRead(copy,saved)&&jar::movement::Same(saved,value));
  undoEdit();CHECK(marker->inuse&&!copy.inuse);
 }
 auto* counter=spawn(jar::logic::counterId,p,a);
 auto* delay=spawn(jar::logic::delayId,p,a);
 CHECK(G_LogicSet(*counter,{jar::logic::Counter,0,0,3,1,true,false}));
 CHECK(G_LogicSet(*delay,{jar::logic::Delay,1300,200}));
 const int delayCopyTime=level.time;
 delay->nextthink=level.time+700;delay->e_ThinkFunc=thinkF_Think_Target_Delay;
 delay->activator=&g_entities[0];
 counter->target2=G_NewString(delay->targetname);
 delay->target=G_NewString(counter->targetname);
 duplicateTool(*counter);
 CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].backwardTarget==owned(delay->s.number)->id);
 level.time+=200;
 edits.clear();duplicatePaste();auto& counterCopy=g_entities[selected];jar::logic::State logic;
 CHECK(G_LogicRead(counterCopy,logic)&&logic.hits==3&&logic.remaining==1&&logic.repeat&&!logic.enabled);
 CHECK(counterCopy.target2&&strcmp(counterCopy.target2,counter->target2));
 const auto* delayCopy=std::find_if(g_entities,g_entities+globals.num_entities,[&](const gentity_t& entity){
  return entity.inuse&&entity.targetname&&!strcmp(entity.targetname,counterCopy.target2);
 });
 CHECK(delayCopy!=g_entities+globals.num_entities&&G_LogicRead(*delayCopy,logic)&&logic.delayMs==1300&&logic.jitterMs==200);
 CHECK(delayCopy->nextthink==level.time+700&&delayCopy->e_ThinkFunc==thinkF_Think_Target_Delay&&delayCopy->activator==delayCopy);
 CHECK(delayCopy->target&&!strcmp(delayCopy->target,counterCopy.targetname)&&timerOutputs.empty());
 CHECK(delay->nextthink==delayCopyTime+700&&delay->activator==&g_entities[0]);
 undoEdit();CHECK(counter->inuse&&!counterCopy.inuse&&delay->inuse&&!delayCopy->inuse);
 level.time=delayCopyTime;
 auto* selector=spawn(jar::logic::randomId,p,a);
 auto* ticker=spawn(jar::logic::timerId,p,a);
 auto* result=Make(Catalog("target_print"),p,a);
 CHECK(G_LogicSet(*selector,{jar::logic::Random,0,0,0,0,false,true}));
 CHECK(G_LogicSet(*ticker,{jar::logic::Timer,700,200}));
 const int priorTimerTime=level.time;
 ticker->nextthink=level.time+400;
 ticker->activator=&g_entities[0];
 G_LogicSetOutput(*ticker,0,result->targetname);
 G_LogicSetOutput(*selector,0,result->targetname);
 G_LogicSetOutput(*selector,3,ticker->targetname);
 duplicateTool(*selector);CHECK(duplicateSnapshot.objects.size()==3);
 CHECK(duplicateSnapshot.objects[0].slottedOutputs[3]==owned(ticker->s.number)->id);
 auto invalidTimer=duplicateSnapshot;
 invalidTimer.objects[0].pendingOutputMs=1;
 CHECK(!jar::duplicate::Valid(invalidTimer));
 level.time+=125;
 edits.clear();duplicatePaste();auto& selectorCopy=g_entities[selected];
 CHECK(G_LogicRead(selectorCopy,logic)&&!logic.remaining&&!logic.repeat);
 CHECK(selectorCopy.target&&strcmp(selectorCopy.target,selector->target)&&!selectorCopy.target2&&!selectorCopy.target3);
 CHECK(selectorCopy.target4&&strcmp(selectorCopy.target4,selector->target4));
 auto* tickerCopy=G_Find(nullptr,FOFS(targetname),selectorCopy.target4);
 CHECK(tickerCopy&&G_LogicRead(*tickerCopy,logic)&&logic.delayMs==700&&logic.jitterMs==200);
 CHECK(tickerCopy->nextthink==level.time+400&&tickerCopy->activator==tickerCopy&&timerOutputs.empty());
 auto* copiedResult=G_Find(nullptr,FOFS(targetname),tickerCopy->target);
 CHECK(copiedResult&&copiedResult!=result&&!strcmp(tickerCopy->target,selectorCopy.target));
 level.time=tickerCopy->nextthink;
 tickerCopy->nextthink=0; // G_RunThink clears this before dispatching the native callback.
 func_timer_think(tickerCopy);
 CHECK(timerOutputs.size()==1&&timerOutputs[0].first==copiedResult&&timerOutputs[0].second==tickerCopy);
 CHECK(tickerCopy->nextthink>=level.time+500&&tickerCopy->nextthink<=level.time+900);
 CHECK(ticker->nextthink==priorTimerTime+400&&ticker->activator==&g_entities[0]);
 undoEdit();CHECK(selector->inuse&&ticker->inuse&&!selectorCopy.inuse&&!tickerCopy->inuse);

 G_LogicSetUseEnabled(*ticker,false);
 ticker->nextthink=level.time+325;
 nativeBegin(*ticker,true);
 level.time+=700;
 duplicateTool(*ticker);
 duplicatePaste();auto& frozenTimer=g_entities[selected];
 CHECK(&frozenTimer!=ticker&&nativeOwnedControl(frozenTimer.s.number)&&frozenTimer.nextthink==level.time+325);
 level.time+=600;
 nativeRelease(frozenTimer.s.number,false);
 CHECK(frozenTimer.nextthink==level.time+325);
 level.time=frozenTimer.nextthink;
 func_timer_think(&frozenTimer);
 CHECK(timerOutputs.size()==1&&frozenTimer.nextthink>level.time);
 undoEdit();CHECK(!frozenTimer.inuse);
 nativeRelease(ticker->s.number,false);
 G_LogicTimerStop(*ticker);
 duplicateTool(*ticker);
 duplicatePaste();auto& stoppedTimer=g_entities[selected];
 CHECK(&stoppedTimer!=ticker&&!stoppedTimer.nextthink&&!stoppedTimer.activator);
 undoEdit();CHECK(!stoppedTimer.inuse&&ticker->inuse);
 timerOutputs.clear();
 level.time=priorTimerTime;
 auto* relay=spawn(jar::logic::relayId,p,a);
 auto* score=spawn(jar::logic::scoreId,p,a);
 auto* kill=spawn(jar::logic::killId,p,a);
 CHECK(G_LogicRead(*relay,logic));logic.delayMs=225;logic.cooldownMs=750;
 CHECK(G_LogicSet(*relay,logic));relay->nextthink=level.time+50;relay->painDebounceTime=level.time+250;
 relay->e_ThinkFunc=thinkF_target_relay_use_go;relay->activator=&g_entities[0];
 CHECK(G_LogicRead(*score,logic));logic.points=-300;CHECK(G_LogicSet(*score,logic));
 CHECK(G_LogicRead(*kill,logic));logic.killMode=jar::logic::Electrical;logic.enabled=false;CHECK(G_LogicSet(*kill,logic));
 relay->target=G_NewString(score->targetname);
 duplicateTool(*relay);CHECK(duplicateSnapshot.objects.size()==2);
 edits.clear();duplicatePaste();auto& relayCopy=g_entities[selected];
 CHECK(G_LogicRead(relayCopy,logic)&&logic.delayMs==225&&logic.cooldownMs==750);
 CHECK(relayCopy.nextthink==level.time+50&&relayCopy.painDebounceTime==level.time+250&&
       relayCopy.e_ThinkFunc==thinkF_target_relay_use_go&&relayCopy.activator==&relayCopy);
 CHECK(relayCopy.target&&strcmp(relayCopy.target,relay->target));
 bool copiedScore=false;
 for(int i=0;i<globals.num_entities;++i){
  const auto& entity=g_entities[i];
  if(!entity.inuse||!entity.targetname||!G_LogicRead(entity,logic))continue;
  if(!strcmp(entity.targetname,relayCopy.target))copiedScore=logic.kind==jar::logic::Score&&logic.points==-300;
 }
 CHECK(copiedScore);undoEdit();CHECK(relay->inuse&&score->inuse&&!relayCopy.inuse);
 relay->nextthink=0;
 relay->target=G_NewString("authored_output");
 duplicateTool(*relay);
 duplicatePaste();auto& coolingRelay=g_entities[selected];
 CHECK(coolingRelay.painDebounceTime==level.time+250&&!coolingRelay.nextthink&&!coolingRelay.activator&&!coolingRelay.target);
 undoEdit();CHECK(!coolingRelay.inuse&&relay->inuse);
 duplicateTool(*kill);CHECK(duplicateSnapshot.objects.size()==1);
 duplicatePaste();auto& killCopy=g_entities[selected];
 CHECK(G_LogicRead(killCopy,logic)&&logic.killMode==jar::logic::Electrical&&!logic.enabled);
 undoEdit();CHECK(kill->inuse&&!killCopy.inuse);
 auto* give=spawn(jar::logic::giveId,p,a);
 auto* supply=spawn(2501,p,a);
 supply->nextthink=level.time+12000;
 editSlottedLink(*give,3,supply->targetname);
 CHECK(supply->nextthink==givePickup::reservedDeadline);
 {const WorkshopActionScope context(recordingContextEdit);editSlottedLink(*give,3,nullptr);}
 CHECK(supply->nextthink==level.time+12000);
 selected=give->s.number;revertProperty();CHECK(give->target4&&supply->nextthink==givePickup::reservedDeadline);
 duplicateTool(*give);CHECK(duplicateSnapshot.objects.size()==2);
 CHECK(duplicateSnapshot.objects[0].slottedOutputs[3]==owned(supply->s.number)->id);
 edits.clear();duplicatePaste();auto& giveCopy=g_entities[selected];
 CHECK(G_LogicRead(giveCopy,logic)&&logic.kind==jar::logic::Give);
 CHECK(giveCopy.target4&&strcmp(giveCopy.target4,give->target4));
 auto* supplyCopy=G_Find(nullptr,FOFS(targetname),giveCopy.target4);
 CHECK(supplyCopy&&supplyCopy->item==supply->item&&supplyCopy->count==supply->count&&
       supplyCopy->nextthink==givePickup::reservedDeadline&&supplyCopy->setTime==12000);
 undoEdit();CHECK(give->inuse&&supply->inuse&&!giveCopy.inuse&&!supplyCopy->inuse);
 auto* disable=spawn(jar::logic::disableId,p,a);
 auto* switched=spawn(Catalog("target_print"),p,a);
 G_LogicSetUseEnabled(*switched,false);
 editSlottedLink(*disable,2,switched->targetname);
 duplicateTool(*disable);CHECK(duplicateSnapshot.objects.size()==2);
 CHECK(duplicateSnapshot.objects[0].slottedOutputs[2]==owned(switched->s.number)->id);
 edits.clear();duplicatePaste();auto& disableCopy=g_entities[selected];
 CHECK(G_LogicRead(disableCopy,logic)&&logic.kind==jar::logic::Disable&&logic.enabled);
 auto* switchedCopy=G_Find(nullptr,FOFS(targetname),disableCopy.target3);
 CHECK(switchedCopy&&switchedCopy!=switched&&!G_LogicUseEnabled(*switchedCopy));
 undoEdit();CHECK(disable->inuse&&switched->inuse&&!disableCopy.inuse&&!switchedCopy->inuse);
 {
  using namespace jar::contactTrigger;
  const int previousTime=level.time;
  auto* repeat=spawn(repeatId,p,a);
  auto* once=spawn(onceId,p,a);
  repeat->target=G_NewString(once->targetname);
  CHECK(G_ContactSettings(*repeat,{Repeat,120,500,40,Player,true,true}));
  const vec3_t size={70,30,26};CHECK(jar::volumeEntity::Resize(*repeat,size));
  repeat->e_ThinkFunc=thinkF_multi_trigger_run;repeat->nextthink=level.time+120;
  repeat->activator=&g_entities[0];
  G_ContactSetSpent(*once,true);
  duplicateTool(*repeat);CHECK(duplicateSnapshot.objects.size()==2);
  auto invalid=duplicateSnapshot;
  invalid.objects[1].pendingOutputMs=1;
  CHECK(invalid.objects[1].contactSpent&&!jar::duplicate::Valid(invalid));
  level.time+=175;
  edits.clear();duplicatePaste();auto& repeatCopy=g_entities[selected];
  Settings settings;
  CHECK(G_ContactRead(repeatCopy,settings)&&Same(settings,{Repeat,120,500,40,Player,true,true}));
  CHECK(repeatCopy.pos2[0]==35&&G_ContactPending(repeatCopy)&&!repeatCopy.painDebounceTime);
  CHECK(repeatCopy.nextthink==level.time+120&&repeatCopy.activator==&repeatCopy&&timerOutputs.empty());
  auto* onceCopy=G_Find(nullptr,FOFS(targetname),repeatCopy.target);
  CHECK(onceCopy&&onceCopy!=once&&G_ContactSpent(*onceCopy)&&!(onceCopy->contents&CONTENTS_TRIGGER));
  level.time=repeatCopy.nextthink;
  ContactFire(repeatCopy);
  CHECK(timerOutputs.size()==1&&timerOutputs.back().first==onceCopy&&timerOutputs.back().second==&repeatCopy);
  CHECK(repeatCopy.painDebounceTime>=level.time+460&&repeatCopy.painDebounceTime<=level.time+540);
  duplicateTool(repeatCopy);
  duplicatePaste();auto& coolingCopy=g_entities[selected];
  CHECK(coolingCopy.painDebounceTime==repeatCopy.painDebounceTime&&!G_ContactPending(coolingCopy));
  ContactUse(coolingCopy,&g_entities[0]);
  CHECK(!G_ContactPending(coolingCopy)&&timerOutputs.size()==1);
  level.time=coolingCopy.painDebounceTime;
  ContactUse(coolingCopy,&g_entities[0]);
  CHECK(coolingCopy.nextthink==level.time+120);
  undoEdit();CHECK(!coolingCopy.inuse&&repeatCopy.inuse);
  undoEdit();CHECK(!repeatCopy.inuse&&!onceCopy->inuse&&G_ContactPending(*repeat)&&G_ContactSpent(*once));
  CHECK(repeat->activator==&g_entities[0]&&repeat->nextthink==previousTime+120);

  repeat->target=nullptr;
  G_ContactSetSpent(*once,false);
  CHECK(G_ContactSettings(*once,{Once,250,0,0,Actors,false,true}));
  once->target=G_NewString("authored_trigger_target");
  ContactUse(*once,&g_entities[0]);
  nativeBegin(*once,true);
  level.time+=325;
  duplicateTool(*once);
  duplicatePaste();auto& frozenOnce=g_entities[selected];
  CHECK(nativeOwnedControl(frozenOnce.s.number)&&frozenOnce.nextthink==level.time+250&&!frozenOnce.target);
  level.time+=100;
  nativeRelease(frozenOnce.s.number,false);
  CHECK(frozenOnce.nextthink==level.time+250);
  level.time=frozenOnce.nextthink;
  ContactFire(frozenOnce);
  CHECK(G_ContactSpent(frozenOnce)&&!G_ContactPending(frozenOnce)&&!(frozenOnce.contents&CONTENTS_TRIGGER));
  CHECK(!G_ContactSpent(*once)&&G_ContactPending(*once)&&timerOutputs.size()==1);
  undoEdit();CHECK(!frozenOnce.inuse&&once->inuse);
  nativeRelease(once->s.number,false);
  timerOutputs.clear();
  level.time=previousTime;
  auto* hurt=spawn(hurtId,p,a);
  CHECK(G_ContactSettings(*hurt,{Hurt,0,250,0,NPCs,true,false,65,Electrical,false}));
  CHECK(jar::volumeEntity::Resize(*hurt,size));
  duplicateTool(*hurt);edits.clear();duplicatePaste();auto& hurtCopy=g_entities[selected];
  CHECK(hurtCopy.pos2[0]==35&&G_ContactRead(hurtCopy,settings)&&Same(settings,{Hurt,0,250,0,NPCs,true,false,65,Electrical,false}));
  undoEdit();CHECK(!hurtCopy.inuse&&hurt->inuse);
 }
 {
  using namespace jar::push;
  auto* launch=spawn(volumeId,p,a);auto* aim=spawn(Catalog("target_position"),p,a);
  launch->target=G_NewString(aim->targetname);
  CHECK(G_PushSet(*launch,{Volume,750,300,All,Toward,true,true,true}));
  duplicateTool(*launch);CHECK(duplicateSnapshot.objects.size()==2);
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];Settings settings;
  CHECK(G_PushRead(copy,settings)&&Same(settings,{Volume,750,300,All,Toward,true,true,true}));
  CHECK(G_PushTarget(copy)&&G_PushTarget(copy)!=aim&&!copy.nextthink);
  undoEdit();CHECK(!copy.inuse&&launch->inuse);
 }
 {
  using namespace jar::teleport;
  auto* gate=spawn(discId,p,a);auto* dest=spawn(destinationId,p,a);
  gate->target=G_NewString(dest->targetname);dest->message=G_NewString("Courtyard exit");
  CHECK(G_TeleportSet(*gate,{Volume,800,NoActors,true,false,true,true,true,true,true,true,true,true}));
  duplicateTool(*gate);CHECK(duplicateSnapshot.objects.size()==2);
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];Settings settings;
  CHECK(G_TeleportRead(copy,settings)&&Same(settings,{Volume,800,NoActors,true,false,true,true,true,true,true,true,true,true}));
  const auto* newDest=G_TeleportDestination(copy);CHECK(newDest&&newDest!=dest&&!strcmp(newDest->message,"Courtyard exit"));
  undoEdit();CHECK(!copy.inuse&&gate->inuse);
 }
 {
  auto* emitter=spawn(Catalog("target_laser"),p,a);
  auto* aim=spawn(Catalog("target_position"),p,a);
  CHECK(G_SandboxLaserSet(*emitter,{0,true,false}));
  editLaserEmitterAim(*emitter,aim->targetname);
  duplicateTool(*emitter);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects.front().hasLaserEmitter);
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];jar::sandboxLaser::Settings settings;
  CHECK(G_SandboxLaserRead(copy,settings)&&jar::sandboxLaser::Same(settings,{0,true,false}));
  const auto* newAim=G_SandboxLaserAim(copy);CHECK(newAim&&newAim!=aim&&!copy.alt_fire&&copy.nextthink>level.time);
  undoEdit();CHECK(!copy.inuse&&!newAim->inuse&&emitter->inuse&&aim->inuse);
 }
 {
  auto* source=spawn(Catalog("misc_shield_floor_unit"),p,a);
  const int previousTime=level.time;
  level.time=1000;
  CHECK(G_SandboxStationSet(*source,{120,31,200,false,true}));
  source->setTime=level.time+700;
  source->s.loopSound=42;
  source->attackDebounceTime=level.time+500;
  source->painDebounceTime=source->attackDebounceTime+source->delay;
  G_SandboxStationSchedule(*source);
  level.time+=125;
  duplicateTool(*source);CHECK(duplicateSnapshot.objects.size()==1);
  auto invalid=duplicateSnapshot;
  invalid.objects[0].stationRechargeMs=701;
  CHECK(!jar::duplicate::Valid(invalid));
  level.time+=300;
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];
  jar::sandboxStation::Settings copied;
  CHECK(G_SandboxStationRead(copy,copied)&&copied.capacity==120&&copied.charge==31&&copied.recharge==200&&
        copied.enabled&&!copied.unlimited&&!copy.s.loopSound&&!copy.attackDebounceTime);
  CHECK(copy.nextthink==level.time+575&&copy.setTime==copy.nextthink);
  level.time=copy.nextthink-1;
  G_SandboxStationControlledFrame(copy);
  CHECK(copy.count==31);
  ++level.time;
  G_SandboxStationControlledFrame(copy);
  CHECK(copy.count==32&&copy.nextthink==level.time+200);
  undoEdit();CHECK(!copy.inuse&&source->inuse&&source->count==31&&source->setTime==1700&&source->s.loopSound==42);

  CHECK(G_SandboxStationSet(*source,{120,31,200,false,false}));
  source->setTime=level.time;
  duplicateTool(*source);
  duplicatePaste();auto& disabledCopy=g_entities[selected];
  CHECK(G_SandboxStationRead(disabledCopy,copied)&&!copied.enabled&&!disabledCopy.nextthink&&disabledCopy.setTime==level.time);
  G_LogicSetUseEnabled(disabledCopy,true);
  CHECK(disabledCopy.nextthink==level.time+200&&!source->nextthink);
  undoEdit();CHECK(!disabledCopy.inuse&&source->inuse);
  level.time=previousTime;
 }
 {
  using namespace jar::sandboxUsable;
  auto* source=spawn(Catalog("func_usable"),p,a);
  auto* off=Make(Catalog("target_print"),p,a);auto* on=Make(Catalog("target_print"),p,a);
  source->target=G_NewString(off->targetname);source->target2=G_NewString(on->targetname);
  Settings settings;
  CHECK(G_SandboxUsableRead(*source,settings));
  settings.solid=false;settings.pending=true;settings.damage=AtZero;settings.direct=NpcOnly;
  CHECK(G_SandboxUsableSet(*source,settings));source->activator=&g_entities[0];
  duplicateTool(*source);CHECK(duplicateSnapshot.objects.size()==3);
  edits.clear();duplicatePaste();auto& copy=g_entities[selected];Settings copied;
  CHECK(G_SandboxUsableRead(copy,copied)&&Same(copied,settings)&&!copy.contents&&copy.nextthink==level.time+FRAMETIME);
  CHECK(copy.target&&copy.target2&&strcmp(copy.target,source->target)&&strcmp(copy.target2,source->target2)&&!copy.activator);
  undoEdit();CHECK(!copy.inuse&&source->inuse&&off->inuse&&on->inuse);
 }
 {
  using namespace jar::sandboxBreakable;
  auto* source=spawn(Catalog("func_glass"),p,a);
  auto* output=Make(Catalog("target_print"),p,a);source->target=G_NewString(output->targetname);
  for(const auto phase:{Pending,Broken}){
   Settings settings;
   CHECK(G_SandboxBreakableRead(*source,settings));
   settings.phase=phase;settings.health=0;settings.remaining=phase==Pending?125:0;settings.material=MAT_GLASS;
   CHECK(G_SandboxBreakableSet(*source,settings));
   duplicateTool(*source);CHECK(duplicateSnapshot.objects.size()==2);
   edits.clear();duplicatePaste();auto& copy=g_entities[selected];
   Settings copied;
   CHECK(G_SandboxBreakableRead(copy,copied)&&Same(settings,copied)&&settings.remaining==copied.remaining);
   CHECK(copy.target&&strcmp(copy.target,source->target)&&copy.enemy==nullptr);
   CHECK((copy.contents!=0)==(phase==Pending));
   undoEdit();CHECK(!copy.inuse&&source->inuse);
  }
 }
 auto*button=spawn(JAR_BUTTON_ID,p,a);
 CHECK(G_ButtonRestore(*button,{1200,JAR_BUTTON_HELD,4,675}));
 auto*buttonOutput=Make(Catalog("target_print"),p,a);button->target2=G_NewString(buttonOutput->targetname);
 duplicateTool(*button);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.objects[0].hasButton);
 edits.clear();duplicatePaste();auto&buttonCopy=g_entities[selected];jarButtonState_t copiedButton;
 CHECK(G_ButtonRead(buttonCopy,copiedButton)&&copiedButton.frame==4&&copiedButton.remaining==675&&copiedButton.waitMs==1200);
 CHECK(buttonCopy.target2&&strcmp(buttonCopy.target2,button->target2));undoEdit();CHECK(!buttonCopy.inuse&&button->inuse);
 auto*parameters=Make(Catalog("target_change_parm"),p,a);CHECK(jar::entityParms::Literal(*parameters,0,"+5")&&jar::entityParms::Literal(*parameters,15,"last"));duplicateTool(*parameters);edits.clear();duplicatePaste();auto&parmCopy=g_entities[selected];
 CHECK(parmCopy.parms&&parmCopy.parms!=parameters->parms&&!strcmp(parmCopy.parms->parm[0],"+5")&&!strcmp(parmCopy.parms->parm[15],"last")&&!parmCopy.parms->parm[1][0]);undoEdit();CHECK(parameters->inuse);
 const auto previous=duplicateSnapshot;parameters->parms->parm[3][0]='\n';duplicateTool(*parameters);CHECK(duplicateSnapshot.objects[0].source==previous.objects[0].source);parameters->parms->parm[3][0]=0;
 // Native owned NPCs and props keep their internal rope/thruster attachments
 // through the actual paste/remapping path. Unqualified world links are omitted.
 auto*actor=Make(3001,p,a);auto*box=Make(1001,p,a);jar::constraint::Joint rope;rope.kind=jar::constraint::Rope;rope.first=owned(actor->s.number)->id;rope.second=owned(box->s.number)->id;rope.length=100;CHECK(AddJoint(state,rope));
 auto external=rope;external.first=owned(box->s.number)->id;external.second=jar::constraint::NativeId(ENTITYNUM_WORLD);CHECK(AddJoint(state,external));
 duplicateTool(*actor);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==1&&status.find("excluded")!=std::string::npos);
 // World-space scenery bends must be recalculated at the paste destination.
 duplicateSnapshot.joints[0].path.count=2;duplicateSnapshot.joints[0].path.bend[0][1]=20;duplicateSnapshot.joints[0].path.bend[1][0]=40;
 const auto oldOwned=state.ownedCount,oldJoints=state.jointCount;const int admissionsBefore=ropeAdmissions;
 edits.clear();duplicatePaste();CHECK(state.ownedCount==oldOwned+2&&state.jointCount==oldJoints+1&&ropeAdmissions==admissionsBefore+1);
 const auto&copied=state.joints[state.jointCount-1];CHECK(copied.first!=rope.first&&copied.second!=rope.second&&copied.length==100&&!copied.path.count);undoEdit();CHECK(state.ownedCount==oldOwned&&state.jointCount==oldJoints);
 // A destination with no valid route rolls back the complete pasted assembly.
 ropeAdmissionBlocked=true;duplicatePaste();ropeAdmissionBlocked=false;
 CHECK(ropeAdmissions==admissionsBefore+2&&state.ownedCount==oldOwned&&state.jointCount==oldJoints&&actor->inuse&&box->inuse&&edits.empty());
 CHECK(state.joints[0].first==rope.first&&state.joints[0].second==rope.second&&!state.joints[0].path.count);
 for(unsigned kind:{jar::constraint::Weld,jar::constraint::Hinge}){
  state.joints[0].kind=kind;duplicateTool(*actor);CHECK(duplicateSnapshot.objects.size()==2&&duplicateSnapshot.joints.size()==1&&duplicateSnapshot.joints[0].kind==kind);
  duplicatePaste();CHECK(state.ownedCount==oldOwned+2&&state.joints[state.jointCount-1].kind==kind);
  auto save=Encode(state);CHECK(Decode(save.data(),save.size(),decoded,ENTITYNUM_WORLD));auto legacy=save;legacy[0]=6;CHECK(!Decode(legacy.data(),legacy.size(),decoded,ENTITYNUM_WORLD));
  undoEdit();CHECK(state.ownedCount==oldOwned&&state.jointCount==oldJoints);
 }
}

void CheckSectionDuplicate(){
 const auto previousTrace=gi.trace;
 static float aimX;
 gi.trace=[](trace_t* hit,const vec3_t,const vec3_t mins,const vec3_t,const vec3_t end,int,int mask,EG2_Collision mode,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  if(!mins){hit->fraction=.5f;VectorSet(hit->endpos,aimX,0,0);hit->plane.normal[2]=1;}
  else if(mode==G2_BRUSH_ONLY)CHECK(mask==MASK_SOLID);
 };
 const auto id=Catalog("Vjun Sentinel");
 const vec3_t origin={0,0,400},angles={0,90,0};
 auto* section=Make(id,origin,angles);
 section->classname=G_NewString("misc_bsp");section->model=G_NewString("#mp/ffa1");
 section->bmodel=qtrue;section->e_UseFunc=useF_target_teleporter_use;
 VectorSet(section->mins,1856,-1376,-352);VectorSet(section->maxs,5664,2976,3392);
 duplicateTool(*section);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.objects.front().maxs[1]>4096);
 auto ordinary=duplicateSnapshot;
 ordinary.objects.front().catalog=1001;
 CHECK(!jar::duplicate::Valid(ordinary));
 const auto attempts=attempt;
 const auto count=state.ownedCount;
 aimX=64000;duplicatePaste();
 CHECK(attempt==attempts&&state.ownedCount==count&&status.find("outside the world limits")!=std::string::npos);
 aimX=3000;duplicatePaste();
 CHECK(attempt==attempts+1&&state.ownedCount==count+1);
 const auto& pasted=g_entities[selected];
 CHECK(pasted.currentAngles[YAW]==90&&std::fabs(pasted.currentOrigin[0]-3800)<.001f);
 undoEdit();CHECK(state.ownedCount==count&&section->inuse&&!pasted.inuse);
 remove(section->s.number,true);
 gi.trace=previousTrace;
}

void CheckWorldAnchorDuplicate(){
 const auto previousTrace=gi.trace;
 static bool destinationSolid;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  if(mins)return;
  const float dz=start[2]-end[2];
  if(std::fabs(dz-4)<.01f){
   if(start[2]<0||end[2]>0||(start[0]>100&&!destinationSolid))return;
   hit->fraction=start[2]/dz;hit->entityNum=ENTITYNUM_WORLD;hit->endpos[2]=0;
  }else{
   if(VectorCompare(start,end))return;
   hit->fraction=.5f;VectorSet(hit->endpos,200,0,0);
  }
  hit->plane.normal[2]=1;
 };
 for(const auto kind:{jar::constraint::Rope,jar::constraint::Weld,jar::constraint::Hinge}){
  const vec3_t origin={0,0,40};
  auto* source=Make(1001,origin);
  jar::constraint::Joint joint;
  joint.kind=kind; joint.length=kind==jar::constraint::Rope?50:0;
  joint.first=kind==jar::constraint::Hinge?jar::duplicate::worldAnchor:owned(source->s.number)->id;
  joint.second=kind==jar::constraint::Hinge?owned(source->s.number)->id:jar::duplicate::worldAnchor;
  if(kind==jar::constraint::Hinge){
   joint.a.rotation[1]=joint.a.rotation[3]=std::sqrt(.5f);
   joint.b=joint.a;joint.b.position[2]=-40;
  }else if(kind==jar::constraint::Weld)joint.a.position[2]=-40;
  const auto id=AddJoint(state,joint);CHECK(id);
  duplicateTool(*source);CHECK(duplicateSnapshot.joints.size()==1&&duplicateSnapshot.worldNormals.count(id));
  remove(source->s.number);
  const auto objectsBefore=state.ownedCount,linksBefore=state.jointCount;
  destinationSolid=false;duplicatePaste();
  CHECK(state.ownedCount==objectsBefore&&state.jointCount==linksBefore&&status.find("matching scenery")!=std::string::npos);
  destinationSolid=true;duplicatePaste();
  CHECK(state.ownedCount==objectsBefore+1&&state.jointCount==linksBefore+1&&g_entities[selected].currentOrigin[2]==40);
  const auto& pasted=state.joints[state.jointCount-1];
  const auto& anchor=pasted.first==jar::duplicate::worldAnchor?pasted.a:pasted.b;
  CHECK(pasted.kind==kind&&anchor.position[0]==200&&anchor.position[2]==0);
  undoEdit();CHECK(state.ownedCount==objectsBefore&&state.jointCount==linksBefore);
 }
 // A copyable map model can keep its own world anchor and mounted thruster.
 // Its link to an uncopied entity stays on the original.
 const vec3_t mapOrigin={18,0,40};
 auto* mapModel=Make(1001,mapOrigin);Forget(state,mapModel->s.number);
 mapModel->classname="misc_model_static";mapModel->model="crate.md3";
 auto* peer=Make(1002,mapOrigin);
 jar::constraint::Joint weld;weld.kind=jar::constraint::Weld;
 weld.first=jar::constraint::NativeId(mapModel->s.number);weld.second=jar::duplicate::worldAnchor;
 weld.a.position[2]=-40;weld.b.position[0]=18;
 CHECK(AddJoint(state,weld));
 auto external=weld;external.second=owned(peer->s.number)->id;CHECK(AddJoint(state,external));
 auto jet=weld;jet.kind=jar::constraint::Thruster;jet.second=jet.first;jet.force=7500;jet.enabled=0;
 CHECK(AddJoint(state,jet));
 duplicateTool(*mapModel);
 CHECK(duplicateSnapshot.objects.size()==1&&duplicateSnapshot.joints.size()==2&&
       duplicateSnapshot.origin[0]==18&&status.find("1 attachment(s)")!=std::string::npos);
 const auto count=state.ownedCount,links=state.jointCount;
 destinationSolid=false;duplicatePaste();CHECK(state.ownedCount==count&&state.jointCount==links);
 destinationSolid=true;
 duplicatePaste();CHECK(state.ownedCount==count+1&&state.jointCount==links+2);
 CHECK(g_entities[selected].currentOrigin[0]==200&&g_entities[selected].currentOrigin[2]==40);
 CHECK(state.joints[links].b.position[0]==200&&state.joints[links].first==owned(selected)->id);
 CHECK(state.joints[links+1].force==7500&&!state.joints[links+1].enabled&&state.joints[links+1].first==owned(selected)->id);
 const auto bytes=Encode(state);State decoded;
 CHECK(Decode(bytes.data(),bytes.size(),decoded,ENTITYNUM_WORLD)&&decoded.joints[links].b.position[0]==200&&
       decoded.joints[links+1].force==7500&&!decoded.joints[links+1].enabled);
 undoEdit();CHECK(mapModel->inuse&&peer->inuse&&state.ownedCount==count&&state.jointCount==links);
 G_FreeEntity(mapModel);remove(peer->s.number,true);
 gi.trace=previousTrace;
}

void CheckAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t p={0,0,100},q={32,0,108},r={64,0,80},angles={0,37,0};
 const vec3_t exit1={500,20,5},exit2={700,40,5};
 auto& lead=*Make(1001,p,angles);auto& peer=*Make(1002,q,angles);auto& tail=*Make(1001,r,angles);
 auto& gate=*spawn(volumeId,p,vec3_origin);auto& secondGate=*spawn(volumeId,p,vec3_origin);
 auto& dest=*Make(destinationId,exit1);auto& secondDest=*Make(destinationId,exit2);
 gate.target=G_NewString(dest.targetname);secondGate.target=G_NewString(secondDest.targetname);
 dest.contents=CONTENTS_SOLID;secondDest.contents=CONTENTS_SOLID;
 for(auto* member:{&lead,&peer,&tail}){owned(member->s.number)->frozen=0;gi.linkentity(member);}
 // Keep one peer on native motion while the other two have real Box3D bodies.
 peer.s.frame=7;
 const auto leadId=owned(lead.s.number)->id,peerId=owned(peer.s.number)->id,tailId=owned(tail.s.number)->id;
 const unsigned chargeSlot=nextSlot++;globals.num_entities=std::max(globals.num_entities,int(nextSlot));
 auto& charge=g_entities[chargeSlot];charge.s.number=int(chargeSlot);charge.inuse=charge.linked=qtrue;
 charge.classname="detpack";charge.s.weapon=WP_DET_PACK;charge.s.eType=ET_GENERAL;charge.s.pos.trType=TR_STATIONARY;
 charge.s.eFlags=EF_MISSILE_STICK;charge.e_DieFunc=dieF_WP_ExplosiveDie;charge.e_ThinkFunc=thinkF_WP_Explode;charge.nextthink=5000;
 VectorSet(charge.currentOrigin,64,12,80);VectorSet(charge.mins,-3,-3,-3);VectorSet(charge.maxs,3,3,3);
 G_SandboxChargeAttach(charge,tail);CHECK(charge.s.groundEntityNum==tail.s.number);
 CHECK(physicsWorld.Start({{-1024,-1024,-1000},{1024,-1024,-1000},{1024,1024,-1000},
                           {-1024,-1024,-1000},{1024,1024,-1000},{-1024,1024,-1000}}));
 for(auto* member:{&lead,&peer,&tail}){
  VectorSet(member->s.pos.trDelta,10,20,30);VectorSet(member->s.apos.trDelta,0,0,2);
  if(member!=&peer)CHECK(physicsWorld.Prop(member->s.number,owned(member->s.number)->id,{10,10,10},{},physicsPose(*member),false));
 }
 jar::constraint::Joint joint;joint.kind=jar::constraint::Hinge;joint.first=leadId;joint.second=peerId;
 const auto hinge=AddJoint(state,joint);CHECK(hinge);
 joint.kind=jar::constraint::Rope;joint.first=peerId;joint.second=tailId;joint.length=120;
 joint.path.count=2;VectorSet(joint.path.bend[0],40,20,98);VectorSet(joint.path.bend[1],50,20,90);
 const auto rope=AddJoint(state,joint);CHECK(rope);
 jar::constraint::Joint thruster;thruster.kind=jar::constraint::Thruster;thruster.first=thruster.second=tailId;
 thruster.force=500;const auto thrust=AddJoint(state,thruster);CHECK(thrust);
 ropeStates[rope]=1;nativeJointHistory[leadId]=nativeJointHistory[peerId]=1;
 auto getLink=[](uint32_t id)->jar::constraint::Joint&{
  for(unsigned i=0;i<state.jointCount;++i)if(state.joints[i].id==id)return state.joints[i];
  throw std::runtime_error("Expected assembly link");
 };

 static int occupiedExit=-1,collisionPasses=0;
 static bool blockedRope=false,ropeObstacle=false;
 static gentity_t *groupA,*groupB,*groupC,*mountedCharge;
 groupA=&lead;groupB=&peer;groupC=&tail;mountedCharge=&charge;
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_WORLD;VectorCopy(end,hit->endpos);
  if(pass==ENTITYNUM_NONE){
   CHECK(mask==JAR_ROPE_PATH_CONTENTS&&!groupB->linked&&!groupC->linked&&groupA->linked&&groupA->currentOrigin[0]>=468);
   CHECK(start[0]>=468&&end[0]>=468);
   if(blockedRope){hit->startsolid=qtrue;hit->fraction=0;VectorCopy(start,hit->endpos);}
   else if(ropeObstacle){
    const vec3_t center={548,20,40},half={3,7,4};
    float enter=0,leave=1;bool inside=true;vec3_t normal{};
    for(int axis=0;axis<3;++axis){
     const float lo=center[axis]-half[axis]+mins[axis],hi=center[axis]+half[axis]-mins[axis];
     const float delta=end[axis]-start[axis];
     inside=inside&&start[axis]>=lo&&start[axis]<=hi;
     if(std::fabs(delta)<.0001f){if(start[axis]<lo||start[axis]>hi)return;continue;}
     const float near=((delta>0?lo:hi)-start[axis])/delta;
     const float far=((delta>0?hi:lo)-start[axis])/delta;
     if(near>enter){enter=near;VectorClear(normal);normal[axis]=delta>0?-1.f:1.f;}
     leave=std::min(leave,far);
     if(enter>leave)return;
    }
    hit->fraction=enter;hit->startsolid=inside?qtrue:qfalse;
    VectorCopy(normal,hit->plane.normal);vec3_t delta;VectorSubtract(end,start,delta);VectorMA(start,enter,delta,hit->endpos);
   }
  }else{
   CHECK(!groupA->linked&&!groupB->linked&&!groupC->linked);
   if(pass==mountedCharge->s.number)CHECK(mask==MASK_SHOT&&!mountedCharge->linked);
   else CHECK(mask==(MASK_SOLID|CONTENTS_BODY));
   CHECK(VectorCompare(start,end));++collisionPasses;
   if(pass==occupiedExit)hit->startsolid=qtrue;
  }
 };
 int budget=4096;
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true}));
 CHECK(!teleportAssembly(leadId,gate,budget)&&!collisionPasses);
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true}));
 CHECK(G_TeleportSet(secondGate,{Volume,500,NoActors,false,true,true,true,false,false,true}));
 auto anchor=joint;anchor.first=leadId;anchor.second=jar::constraint::NativeId(ENTITYNUM_WORLD);anchor.path={};
 const auto anchored=AddJoint(state,anchor);CHECK(anchored);
 CHECK(!teleportAssembly(leadId,gate,budget)&&!collisionPasses&&ForgetJoint(state,anchored));
 held=tail.s.number;CHECK(!teleportAssembly(leadId,gate,budget)&&!collisionPasses);held=noEntity;
 owned(peer.s.number)->frozen=1;CHECK(!teleportAssembly(leadId,gate,budget)&&!collisionPasses);owned(peer.s.number)->frozen=0;
 occupiedExit=tail.s.number;CHECK(!teleportAssembly(leadId,gate,budget));occupiedExit=-1;
 occupiedExit=charge.s.number;CHECK(!teleportAssembly(leadId,gate,budget)&&charge.currentOrigin[0]==64&&charge.linked);occupiedExit=-1;
 blockedRope=true;CHECK(!teleportAssembly(leadId,gate,budget));blockedRope=false;
 assemblyClockReady=false;CHECK(!teleportAssembly(leadId,gate,budget)&&assemblyClock==lead.s.number);assemblyClockReady=true;
 CHECK(VectorCompare(lead.currentOrigin,p)&&VectorCompare(peer.currentOrigin,q)&&VectorCompare(tail.currentOrigin,r)&&
       lead.linked&&peer.linked&&tail.linked&&assemblyArrivals.empty()&&getLink(rope).path.count==2);
 jar::physics::Pose original;CHECK(physicsWorld.Read(lead.s.number,original)&&original.position.x==0);
 auto failedUpdate=original;failedUpdate.position.x=200;
 CHECK(!physicsWorld.CorrectPoses({{uint32_t(lead.s.number),failedUpdate},{uint32_t(peer.s.number),failedUpdate}}));
 CHECK(physicsWorld.Read(lead.s.number,original)&&original.position.x==0);

 // The earliest member crossing wins even when queued after another portal.
 pendingAssemblyCrossings.push_back({leadId,{},true,false,{{.8f,&gate,{}}}});
 pendingAssemblyCrossings.push_back({peerId,{},false,false,{{.2f,&secondGate,{}}}});
 finishAssemblyCrossings(.05f);
 CHECK(lead.currentOrigin[0]==668&&peer.currentOrigin[0]==700&&tail.currentOrigin[0]==732);
 CHECK(lead.currentOrigin[1]==40&&lead.currentOrigin[2]==46&&peer.currentOrigin[2]==54&&tail.currentOrigin[2]==26);
 CHECK(std::fabs(charge.currentOrigin[0]-732)<.001f&&std::fabs(charge.currentOrigin[1]-52)<.001f&&
       charge.currentOrigin[2]==26&&(charge.s.eFlags&EF_TELEPORT_BIT)&&charge.nextthink==5000);
 CHECK(assemblyArrivals.size()==3&&pendingAssemblyCrossings.empty()&&!assemblyPushes&&!assemblyActorPasses);
 CHECK(assemblyClock==lead.s.number&&getLink(rope).length==120&&!getLink(rope).path.count&&
       getLink(thrust).force==500&&!ropeStates.count(rope)&&nativeJointHistory.empty());
 for(auto* member:{&lead,&peer,&tail}){
  CHECK(member->currentAngles[YAW]==37&&(member->s.eFlags&EF_TELEPORT_BIT)&&member->s.pos.trDelta[1]==20&&member->s.apos.trDelta[2]==2);
 }
 CHECK(physicsWorld.Read(tail.s.number,original)&&original.position.x==732&&original.linear.y==20);
 CHECK(!teleportAssembly(leadId,gate,budget));

 assemblyArrivals.clear();
 getLink(hinge).kind=jar::constraint::Weld;
 owned(peer.s.number)->frozen=1;owned(tail.s.number)->frozen=1;
 CHECK(physicsWorld.Prop(tail.s.number,tailId,{10,10,10},{},physicsPose(tail),true));
 CHECK(!teleportAssembly(leadId,gate,budget)); // Frozen members require explicit opt-in.
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true,true}));
 CHECK(!teleportAssembly(tailId,gate,budget)); // A frozen body cannot initiate a crossing.
 occupiedExit=tail.s.number;
 CHECK(!teleportAssembly(leadId,gate,budget)&&physicsWorld.Read(tail.s.number,original)&&original.position.x==732&&
       lead.currentOrigin[0]==668&&tail.currentOrigin[0]==732&&peer.currentOrigin[0]==700&&assemblyArrivals.empty());
 occupiedExit=-1;
 CHECK(!physicsWorld.CorrectPoses({{uint32_t(lead.s.number),failedUpdate},{uint32_t(tail.s.number),failedUpdate}}));
 CHECK(physicsWorld.Read(lead.s.number,original)&&original.position.x==668);
 ropeObstacle=true;
 CHECK(teleportAssembly(leadId,gate,budget)&&lead.currentOrigin[0]==500&&tail.currentOrigin[0]==564);
 CHECK(getLink(hinge).kind==jar::constraint::Weld&&getLink(rope).path.count>=2&&getLink(rope).path.bend[0][0]>500);
 ropeObstacle=false;
 CHECK(lead.s.pos.trDelta[1]==20);
 for(auto* member:{&peer,&tail})CHECK(owned(member->s.number)->frozen&&
    !VectorLengthSquared(member->s.pos.trDelta)&&!VectorLengthSquared(member->s.apos.trDelta));
 physicsWorld.Step(.05f,800);
 CHECK(physicsWorld.Read(tail.s.number,original)&&original.position.x==564&&original.angles.y==37&&
       !original.linear.y&&!original.angular.z&&!physicsWorld.CorrectPose(tail.s.number,failedUpdate));
 CHECK(std::fabs(charge.currentOrigin[0]-564)<.001f&&charge.s.groundEntityNum==tail.s.number);
 owned(peer.s.number)->frozen=0;owned(tail.s.number)->frozen=0;
 CHECK(physicsWorld.Prop(tail.s.number,tailId,{10,10,10},{},physicsPose(tail),false));
 assemblyArrivals.clear();occupiedExit=peer.s.number;
 pendingAssemblyCrossings.push_back({leadId,{},true,false,{{.2f,&gate,{}}}});
 pendingAssemblyCrossings.push_back({peerId,{},false,true,{{.4f,&gate,{}}}});
 finishAssemblyCrossings(.05f);
 CHECK(assemblyActorPasses==1&&assemblyImpacts==1&&assemblyPushes==2&&lead.currentOrigin[0]==500&&tail.currentOrigin[0]==564);
 CHECK(ForgetJoint(state,rope));G_SandboxChargesForget(tail);
 vec3_t underside;VectorCopy(tail.currentOrigin,underside);underside[2]-=12;
 G_SetOrigin(&charge,underside);G_SandboxChargeAttach(charge,tail);
 const vec3_t singleExit={700,0,20};
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  CHECK(!groupC->linked&&start[0]==700&&VectorCompare(start,end));
  if(pass==mountedCharge->s.number){
   CHECK(mask==MASK_SHOT&&!mountedCharge->linked&&start[2]==13);
   hit->startsolid=occupiedExit==pass?qtrue:qfalse;
  }else CHECK(pass==groupC->s.number&&mask==(MASK_SOLID|CONTENTS_BODY)&&start[2]==25);
 };
 occupiedExit=charge.s.number;
 CHECK(!G_TeleportPropMotion(physicsWorld,tail,singleExit,true,true)&&tail.currentOrigin[0]==564&&charge.currentOrigin[0]==564);
 occupiedExit=-1;
 CHECK(G_TeleportPropMotion(physicsWorld,tail,singleExit,true,true)&&tail.currentOrigin[2]==25&&charge.currentOrigin[2]==13);
 CHECK(physicsWorld.Read(tail.s.number,original)&&original.position.x==700&&original.position.z==25);
 G_SandboxChargesReset();gi.trace=priorTrace;physicsWorld.Stop();
}

void CheckMixedAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t origin={0,0,60},playerOrigin={-48,0,68},npcOrigin={48,0,72},exit={900,10,0};
 const vec3_t facing={0,65,0};
 auto& box=*Make(1001,origin);
 auto& npc=*Make(3001,npcOrigin,facing);
 auto& gate=*spawn(volumeId,origin,vec3_origin);
 auto& destination=*Make(destinationId,exit);
 gate.target=destination.targetname;
 const auto boxKey=owned(box.s.number)->id,npcKey=owned(npc.s.number)->id;
 owned(box.s.number)->frozen=0;
 auto& player=g_entities[0];
 player.inuse=qtrue;player.health=100;player.client=&clients[0];
 VectorSet(player.mins,-8,-8,-24);VectorSet(player.maxs,8,8,32);
 G_SetOrigin(&player,playerOrigin);G_SetAngles(&player,facing);
 for(auto* actor:{&player,&npc}){
  auto& ps=actor->client->ps;
  VectorCopy(actor->currentOrigin,ps.origin);
  VectorCopy(facing,ps.viewangles);
  VectorSet(ps.velocity,4,7,10);
  ps.groundEntityNum=ENTITYNUM_WORLD;
  gi.linkentity(actor);
 }
 gi.linkentity(&box);
 VectorSet(box.s.pos.trDelta,10,20,30);
 npc.clipmask=CONTENTS_MONSTERCLIP;

 const auto playerKey=jar::constraint::NativeId(0);
 jar::constraint::Joint joint;
 joint.kind=jar::constraint::Rope;joint.first=playerKey;joint.second=boxKey;joint.length=72;
 joint.a.position[1]=4;
 const auto rope=AddJoint(state,joint);CHECK(rope);
 joint={};joint.kind=jar::constraint::Weld;joint.first=boxKey;joint.second=npcKey;
 const auto weld=AddJoint(state,joint);CHECK(weld);
 const auto priorTrace=gi.trace;
 static int blockedSlot=-1,actorTraces=0,ropeTraces=0;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_WORLD;VectorCopy(end,hit->endpos);
  if(pass==ENTITYNUM_NONE){
   CHECK(mask==JAR_ROPE_PATH_CONTENTS&&start[0]>800&&end[0]>800);
   ++ropeTraces;return;
  }
  CHECK(VectorCompare(start,end)&&!g_entities[pass].linked);
  if(g_entities[pass].client){CHECK(mask==(MASK_PLAYERSOLID|g_entities[pass].clipmask));++actorTraces;}
  hit->startsolid=pass==blockedSlot?qtrue:qfalse;
 };
 assemblyArrivals.clear();
 int budget=4096;
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true}));
 CHECK(!teleportAssembly(boxKey,gate,budget)&&!actorTraces);
 CHECK(G_TeleportSet(gate,{Volume,500,Player,false,true,true,true,false,false,true}));
 CHECK(!teleportAssembly(playerKey,gate,budget)&&!actorTraces); // The NPC is part of the same group.
 CHECK(G_TeleportSet(gate,{Volume,500,Actors,false,true,true,true,false,false,true}));
 npc.next_roff_time=level.time+1;
 CHECK(!teleportAssembly(playerKey,gate,budget)&&!actorTraces);
 npc.next_roff_time=level.time;
 blockedSlot=npc.s.number;
 CHECK(!teleportAssembly(playerKey,gate,budget)&&actorTraces&&assemblyArrivals.empty()&&
       VectorCompare(box.currentOrigin,origin)&&VectorCompare(player.currentOrigin,playerOrigin)&&
       VectorCompare(npc.currentOrigin,npcOrigin)&&player.linked&&npc.linked&&box.linked);
 blockedSlot=-1;
 CHECK(teleportAssembly(playerKey,gate,budget)&&ropeTraces&&assemblyArrivals.size()==3);
 CHECK(player.currentOrigin[0]==900&&box.currentOrigin[0]==948&&npc.currentOrigin[0]==996&&
       player.currentOrigin[2]==25&&box.currentOrigin[2]==17&&npc.currentOrigin[2]==29);
 for(auto* actor:{&player,&npc}){
  CHECK(VectorCompare(actor->client->ps.origin,actor->currentOrigin)&&VectorCompare(actor->lastOrigin,actor->currentOrigin)&&
        VectorCompare(actor->client->ps.viewangles,facing)&&actor->client->ps.velocity[1]==7&&
        actor->client->ps.groundEntityNum==ENTITYNUM_NONE&&(actor->s.eFlags&EF_TELEPORT_BIT));
 }
 CHECK(box.s.pos.trDelta[1]==20&&!teleportAssembly(boxKey,gate,budget));
 assemblyArrivals.clear();
 CHECK(G_TeleportSet(gate,{Volume,500,Actors,false,false,true,true,false,false,true}));
 CHECK(teleportAssembly(boxKey,gate,budget)&&box.currentOrigin[0]==900&&player.currentOrigin[0]==852&&
       npc.currentOrigin[0]==948&&!VectorLengthSquared(player.client->ps.velocity)&&
       !VectorLengthSquared(npc.client->ps.velocity)&&!VectorLengthSquared(box.s.pos.trDelta));
 CHECK(ForgetJoint(state,rope)&&ForgetJoint(state,weld));gi.trace=priorTrace;
}

void CheckVehicleAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t hullStart={0,0,100},cargoStart={60,0,110},exit={1200,25,5};
 auto& hull=*Make(3001,hullStart);
 auto& cargo=*Make(1001,cargoStart);
 auto& droid=*Make(3001,hullStart);
 auto& pilot=g_entities[0];
 auto& gate=*spawn(volumeId,hullStart,vec3_origin);
 auto& destination=*Make(destinationId,exit);
 gate.target=destination.targetname;
 const auto hullKey=attachmentKey(hull),cargoKey=attachmentKey(cargo),pilotKey=attachmentKey(pilot);
 owned(cargo.s.number)->frozen=0;
 CHECK(physicsWorld.Start({{-2048,-2048,-1000},{2048,-2048,-1000},{2048,2048,-1000},
                           {-2048,-2048,-1000},{2048,2048,-1000},{-2048,2048,-1000}}));
 CHECK(physicsWorld.Prop(cargo.s.number,cargoKey,{10,10,10},{},physicsPose(cargo),false));

 vehicleInfo_t definition{};definition.type=VH_FIGHTER;
 Vehicle_t vehicle{};vehicle.m_pVehicleInfo=&definition;vehicle.m_pParentEntity=&hull;
 vehicle.m_pPilot=&pilot;vehicle.m_pDroidUnit=&droid;vehicle.m_iDroidUnitTag=5;
 VectorSet(vehicle.m_vOrientation,15,63,-10);
 VectorSet(vehicle.jarPushDrift,80,40,0);
 hull.m_pVehicle=&vehicle;hull.client->NPC_class=CLASS_VEHICLE;hull.playerModel=0;hull.crotchBolt=4;
 hull.owner=&pilot;hull.s.m_iVehicleNum=pilot.s.number+1;
 VectorSet(hull.mins,-24,-30,-20);VectorSet(hull.maxs,24,30,20);
 VectorCopy(vehicle.m_vOrientation,hull.currentAngles);
 VectorCopy(vehicle.m_vOrientation,hull.client->ps.viewangles);
 VectorSet(hull.client->ps.velocity,300,10,4);hull.client->ps.speed=300;
 const vec3_t pilotStart={-4,0,120},droidStart={8,0,112};
 G_SetOrigin(&pilot,pilotStart);G_SetOrigin(&droid,droidStart);
 for(auto* rider:{&pilot,&droid}){
  rider->owner=&hull;rider->s.m_iVehicleNum=hull.s.number;
  VectorCopy(rider->currentOrigin,rider->client->ps.origin);
  VectorSet(rider->client->ps.viewangles,0,75,0);VectorSet(rider->client->ps.velocity,90,15,5);
 }
 gi.linkentity(&hull);gi.linkentity(&pilot);gi.linkentity(&cargo);
 gi.unlinkentity(&droid);
 jar::constraint::Joint hitch;
 hitch.kind=jar::constraint::Rope;hitch.first=pilotKey;hitch.second=cargoKey;hitch.length=120;
 const auto rope=AddJoint(state,hitch);CHECK(rope); // No explicit link touches the hull.

 static bool missingBolt=false;
 static int blockedSlot=-1,blockedStageSlot=-1,stagedChecks=0;
 static gentity_t* cargoEntity;
 cargoEntity=&cargo;
 gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int model,int bolt,mdxaBone_t* matrix,const vec3_t angles,
                            const vec3_t origin,int,qhandle_t*,const vec3_t){
  CHECK(model==0&&(bolt==4||bolt==5)&&angles[YAW]==63);
  if(missingBolt&&bolt==5)return qfalse;
  matrix->matrix[0][3]=origin[0]+(bolt==5?8.f:0.f);
  matrix->matrix[1][3]=origin[1];
  matrix->matrix[2][3]=origin[2]+(bolt==5?12.f:20.f);
  return qtrue;
 };
 gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t& matrix,Eorientations mode,vec3_t& out){
  CHECK(mode==ORIGIN);VectorSet(out,matrix.matrix[0][3],matrix.matrix[1][3],matrix.matrix[2][3]);
 };
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_WORLD;VectorCopy(end,hit->endpos);
  if(pass==ENTITYNUM_NONE){CHECK(mask==JAR_ROPE_PATH_CONTENTS);return;}
  CHECK(start[0]>=1140&&VectorCompare(start,end)&&!g_entities[pass].linked);
  const bool staged=cargoEntity->linked;
  if(staged)++stagedChecks;
  hit->startsolid=pass==(staged?blockedStageSlot:blockedSlot)?qtrue:qfalse;
 };
 assemblyArrivals.clear();int budget=4096;
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true}));
 CHECK(!teleportAssembly(hullKey,gate,budget));
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,true,true}));
 vehicle.m_iBoarding=1;
 CHECK(!teleportAssembly(cargoKey,gate,budget));vehicle.m_iBoarding=0;
 missingBolt=true;CHECK(!teleportAssembly(cargoKey,gate,budget));missingBolt=false;
 blockedSlot=droid.s.number;CHECK(!teleportAssembly(hullKey,gate,budget));blockedSlot=-1;
 blockedStageSlot=pilot.s.number;CHECK(!teleportAssembly(hullKey,gate,budget)&&stagedChecks);blockedStageSlot=-1;
 CHECK(VectorCompare(hull.currentOrigin,hullStart)&&VectorCompare(cargo.currentOrigin,cargoStart)&&
       VectorCompare(pilot.currentOrigin,pilotStart)&&VectorCompare(droid.currentOrigin,droidStart)&&
       hull.linked&&pilot.linked&&cargo.linked&&!droid.linked&&assemblyArrivals.empty());
 jar::physics::Pose cargoPose;
 CHECK(physicsWorld.Read(cargo.s.number,cargoPose)&&cargoPose.position.x==60);

 CHECK(teleportAssembly(hullKey,gate,budget)&&assemblyArrivals.size()==4&&
       hull.currentOrigin[0]==1200&&hull.currentOrigin[2]==26&&cargo.currentOrigin[0]==1260&&cargo.currentOrigin[2]==36&&
       pilot.currentOrigin[0]==1200&&pilot.currentOrigin[2]==46&&droid.currentOrigin[0]==1208&&droid.currentOrigin[2]==38);
 CHECK(vehicle.m_pPilot==&pilot&&vehicle.m_pDroidUnit==&droid&&pilot.owner==&hull&&droid.owner==&hull&&
       hull.s.m_iVehicleNum==1&&pilot.s.m_iVehicleNum==hull.s.number&&droid.s.m_iVehicleNum==hull.s.number&&
       hull.linked&&pilot.linked&&cargo.linked&&!droid.linked&&pilot.client->ps.viewangles[YAW]==75);
 CHECK(vehicle.m_vOrientation[YAW]==63&&vehicle.m_vPrevOrientation[PITCH]==15&&vehicle.m_LandTrace.fraction==1&&
       hull.client->ps.speed==300&&hull.client->ps.velocity[0]==300&&vehicle.jarPushDrift[0]==80&&
       droid.client->ps.velocity[1]==15&&physicsWorld.Read(cargo.s.number,cargoPose)&&cargoPose.position.x==1260);
 CHECK(!teleportAssembly(cargoKey,gate,budget));

 assemblyArrivals.clear();
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,false,true,true,false,true,true}));
 CHECK(teleportAssembly(cargoKey,gate,budget)&&hull.currentOrigin[0]==1140&&cargo.currentOrigin[0]==1200&&
       pilot.currentOrigin[0]==1140&&droid.currentOrigin[0]==1148&&assemblyArrivals.size()==4);
 CHECK(!hull.client->ps.speed&&!VectorLengthSquared(vehicle.jarPushDrift)&&!VectorLengthSquared(hull.client->ps.velocity)&&
       !VectorLengthSquared(pilot.client->ps.velocity)&&!VectorLengthSquared(droid.client->ps.velocity));
 assemblyArrivals.clear();
 CHECK(ForgetJoint(state,rope)&&!teleportAssembly(hullKey,gate,budget));
 physicsWorld.Stop();gi.trace=priorTrace;
}

void CheckMoverAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t start={0,0,100},cargoStart={70,0,110},exit={500,25,20};
 auto& deck=*spawn(Catalog("func_bobbing"),start,vec3_origin);
 auto& box=*Make(1001,cargoStart);
 auto& gate=*spawn(volumeId,start,vec3_origin);
 auto& target=*Make(destinationId,exit);
 auto& rider=*Make(3001,start);
 const auto deckKey=attachmentKey(deck),boxKey=attachmentKey(box);
 owned(box.s.number)->frozen=0;gi.linkentity(&box);
 gate.target=target.targetname;
 CHECK(G_BobbingRestore(deck,{20,1000,JAR_BOB_TOP,1,0}));
 deck.pos1[2]=90;deck.bounceCount=JAR_BOB_BOTTOM;
 const int deadline=deck.nextthink,phase=deck.count;
 jar::constraint::Joint joint;
 joint.kind=jar::constraint::Weld;joint.first=deckKey;joint.second=boxKey;
 const auto weld=AddJoint(state,joint);CHECK(weld);
 static gentity_t* grounded;
 grounded=&rider;rider.client->ps.groundEntityNum=deck.s.number;
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** nearby,int){
  if(!grounded)return 0;
  nearby[0]=grounded;return 1;
 };
 static int blockedSlot=-1;
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  CHECK(!g_entities[pass].linked&&VectorCompare(start,end));
  const auto& member=g_entities[pass];
  CHECK(mask==(member.client?MASK_PLAYERSOLID:
    G_TeleportObjectKind(member)==jar::teleport::Mover?(MASK_PLAYERSOLID|CONTENTS_CORPSE):(MASK_SOLID|CONTENTS_BODY)));
  *hit={};hit->fraction=1;hit->startsolid=blockedSlot==pass?qtrue:qfalse;
 };
 int budget=4096;assemblyArrivals.clear();
 CHECK(G_TeleportSet(gate,{Volume,500,Actors,false,true,true,true,false,false,true}));
 CHECK(!teleportAssembly(boxKey,gate,budget));
 CHECK(G_TeleportSet(gate,{Volume,500,Actors,false,true,true,true,false,false,true,false,true}));
 CHECK(!teleportAssembly(boxKey,gate,budget)); // A rider outside the component blocks departure.
 grounded=nullptr;
 blockedSlot=deck.s.number;
 CHECK(!teleportAssembly(boxKey,gate,budget)&&VectorCompare(deck.currentOrigin,start)&&deck.count==phase);
 blockedSlot=-1;
 joint.first=deckKey;joint.second=attachmentKey(rider);
 const auto riderLink=AddJoint(state,joint);CHECK(riderLink);
 grounded=&rider;gi.linkentity(&rider);
 const vec3_t aboveDeck={0,0,136};G_SetOrigin(&rider,aboveDeck);VectorCopy(aboveDeck,rider.client->ps.origin);
 moverCrossingSource=&gate;
 teleportMoverOrigins.clear();queueAssemblyMovers(true);
 CHECK(pendingAssemblyCrossings.empty());
 vec3_t underway={0,0,104};G_SetOrigin(&deck,underway);deck.s.pos.trType=TR_INTERPOLATE;deck.pos2[2]=104;
 queueAssemblyMovers(true);CHECK(pendingAssemblyCrossings.size()==1&&pendingAssemblyCrossings[0].mover);
 finishAssemblyCrossings(.05f);
 CHECK(deck.currentOrigin[0]==500&&box.currentOrigin[0]==570&&deck.currentOrigin[2]==25&&box.currentOrigin[2]==31);
 CHECK(deck.pos1[2]==11&&deck.pos2[2]==25&&deck.count==phase&&deck.nextthink==deadline&&deck.alt_fire&&
       (deck.s.eFlags&EF_TELEPORT_BIT)&&assemblyArrivals.size()==3&&pendingAssemblyCrossings.empty());
 CHECK(rider.currentOrigin[2]==57&&rider.client->ps.groundEntityNum==deck.s.number);
 CHECK(ForgetJoint(state,weld)&&ForgetJoint(state,riderLink));
 teleportMoverOrigins.clear();moverCrossingSource=nullptr;gi.trace=priorTrace;grounded=nullptr;

 auto& rail=*Make(Catalog("rail_mover"),start);
 rail.s.pos.trType=TR_LINEAR_STOP;rail.s.pos.trTime=200;rail.s.pos.trDuration=700;
 VectorSet(rail.s.pos.trDelta,100,0,0);VectorSet(rail.pos1,70,0,100);VectorCopy(start,rail.pos2);
 TeleportMoverMotion motion;
 CHECK(G_PlanTeleportMover(rail,TeleportMoverType::Rail,exit,false,motion));
 G_CommitTeleportMover(rail,exit,motion);
 JarMoverMotion external;
 CHECK(rail.s.pos.trType==TR_LINEAR_STOP&&rail.s.pos.trTime==200&&rail.s.pos.trDuration==700&&
       rail.s.pos.trDelta[0]==100&&rail.pos1[0]==570&&rail.pos2[0]==500&&
       G_JAR_MoverMotionRead(rail,external)&&external.offset[0]==500&&!VectorLengthSquared(external.velocity));
 auto& lift=*spawn(Catalog("func_plat"),start,vec3_origin);
 lift.count=12000;lift.alt_fire=qtrue;
 CHECK(G_PlanTeleportMover(lift,TeleportMoverType::Slide,exit,false,motion));
 G_CommitTeleportMover(lift,exit,motion);
 CHECK(lift.count==12000&&lift.alt_fire&&!lift.delay&&VectorCompare(lift.currentOrigin,exit));
}

void CheckBrushAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t start={20,0,100},cargoStart={20,65,120},exit={600,15,10};
 const unsigned slot=nextSlot++;
 globals.num_entities=std::max(globals.num_entities,int(nextSlot));
 auto& brush=g_entities[slot];brush.inuse=brush.bmodel=qtrue;brush.s.number=slot;
 brush.s.eType=ET_MOVER;brush.s.modelindex=7;brush.model=G_NewString("*7");
 brush.classname=G_NewString("func_bobbing");brush.contents=CONTENTS_SOLID;
 brush.e_UseFunc=useF_func_bobbing_use;
 VectorSet(brush.mins,-24,-4,-8);VectorSet(brush.maxs,24,4,8);
 VectorSet(brush.currentAngles,0,90,0);G_SetOrigin(&brush,start);
 VectorSet(brush.s.origin,0,0,100);VectorCopy(brush.s.origin,brush.s.pos.trBase);
 brush.s.pos.trType=TR_SINE;brush.s.pos.trTime=300;brush.s.pos.trDuration=2000;
 brush.s.pos.trDelta[0]=20;gi.linkentity(&brush);
 CHECK(G_TeleportMoverKind(brush)==TeleportMoverType::Brush);
 auto& cargo=*Make(1001,cargoStart);owned(cargo.s.number)->frozen=0;gi.linkentity(&cargo);
 auto& gate=*spawn(volumeId,start,vec3_origin);
 auto& target=*Make(destinationId,exit);gate.target=target.targetname;
 const auto root=attachmentKey(brush),cargoId=attachmentKey(cargo);
 CHECK(jar::constraint::Native(root));
 jar::constraint::Joint joint;joint.kind=jar::constraint::Weld;joint.first=root;joint.second=cargoId;
 const auto weld=AddJoint(state,joint);CHECK(weld);
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true,false,true}));
 assemblyArrivals.clear();int budget=4096;
 scriptedMover=&brush;CHECK(!teleportAssembly(root,gate,budget));scriptedMover=nullptr;
 sectionMover=&brush;CHECK(!teleportAssembly(root,gate,budget));sectionMover=nullptr;
 brush.next_roff_time=1;CHECK(!teleportAssembly(root,gate,budget));brush.next_roff_time=0;
 brush.flags|=FL_TEAMSLAVE;CHECK(!teleportAssembly(root,gate,budget));brush.flags&=~FL_TEAMSLAVE;
 brush.teamchain=&cargo;CHECK(!teleportAssembly(root,gate,budget));brush.teamchain=nullptr;

 const auto priorTrace=gi.trace;
 static bool blocked=true;static int brushSlot;
 brushSlot=brush.s.number;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,
             int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  CHECK(!g_entities[pass].linked&&VectorCompare(start,end));
  if(pass==brushSlot){
   CHECK(std::fabs(mins[0]+4)<.001f&&std::fabs(maxs[1]-24)<.001f&&mask==(MASK_PLAYERSOLID|CONTENTS_CORPSE));
   hit->startsolid=blocked?qtrue:qfalse;
  }
 };
 CHECK(!teleportAssembly(root,gate,budget)&&VectorCompare(brush.currentOrigin,start)&&brush.s.origin[0]==0&&
       brush.s.pos.trBase[0]==0&&brush.linked&&assemblyArrivals.empty());
 blocked=false;
 moverCrossingSource=&gate;teleportMoverOrigins.clear();queueAssemblyMovers(true);
 CHECK(pendingAssemblyCrossings.empty());
 brush.currentOrigin[0]=22;
 queueAssemblyMovers(true);CHECK(pendingAssemblyCrossings.size()==1&&pendingAssemblyCrossings[0].owner==root);
 finishAssemblyCrossings(.05f);
 CHECK(brush.currentOrigin[0]==600&&brush.s.origin[0]==578&&brush.s.pos.trBase[0]==578&&
       brush.currentOrigin[2]==19&&cargo.currentOrigin[0]==598&&
       brush.s.pos.trType==TR_SINE&&brush.s.pos.trTime==300&&brush.s.pos.trDuration==2000&&
       brush.s.pos.trDelta[0]==20&&brush.e_UseFunc==useF_func_bobbing_use&&brush.linked&&assemblyArrivals.size()==2);
 CHECK(ForgetJoint(state,weld));
 teleportMoverOrigins.clear();moverCrossingSource=nullptr;gi.trace=priorTrace;
}

void CheckBinaryAssemblyTeleport(){
 using namespace jar::teleport;
 const auto native=[](const char* classname){
  const auto slot=nextSlot++;
  globals.num_entities=std::max(globals.num_entities,int(nextSlot));
  auto& entity=g_entities[slot];entity.inuse=qtrue;entity.s.number=slot;entity.classname=G_NewString(classname);
  return &entity;
 };
 auto& leader=*native("func_door");auto& leaf=*native("func_door");
 auto& sensor=*native("trigger_door");auto& disabled=*native("trigger_door");
 const vec3_t origin={10,0,100},leafOrigin={50,0,100},cargoOrigin={20,70,100},exit={600,15,20};
 for(auto* part:{&leader,&leaf}){
  part->bmodel=qtrue;part->s.modelindex=2;part->model=G_NewString("*2");part->s.eType=ET_MOVER;
  part->contents=CONTENTS_SOLID|CONTENTS_OPAQUE;
  part->e_ReachedFunc=reachedF_Reached_BinaryMover;part->moverState=MOVER_POS1;
  part->teammaster=&leader;
  VectorSet(part->mins,-12,-6,-8);VectorSet(part->maxs,12,6,8);
  G_SetOrigin(part,part==&leader?origin:leafOrigin);
  VectorCopy(part->currentOrigin,part->s.origin);VectorCopy(part->currentOrigin,part->pos1);
  VectorCopy(part->currentOrigin,part->pos2);part->pos2[1]+=40;
  gi.linkentity(part);
 }
 leader.teamchain=&leaf;leaf.flags|=FL_TEAMSLAVE;
 for(auto* trigger:{&sensor,&disabled}){
  trigger->owner=&leader;trigger->e_TouchFunc=touchF_Touch_DoorTrigger;trigger->contents=CONTENTS_TRIGGER;
  VectorSet(trigger->mins,-2,-126,92);VectorSet(trigger->maxs,62,126,108);
 }
 disabled.svFlags|=SVF_INACTIVE;gi.linkentity(&sensor);
 auto& cargo=*Make(1001,cargoOrigin);owned(cargo.s.number)->frozen=0;gi.linkentity(&cargo);
 auto& gate=*spawn(volumeId,origin,vec3_origin);auto& target=*Make(destinationId,exit);gate.target=target.targetname;
 jar::constraint::Joint joint;joint.kind=jar::constraint::Weld;joint.first=attachmentKey(leaf);joint.second=attachmentKey(cargo);
 const auto weld=AddJoint(state,joint);CHECK(weld);
 CHECK(G_TeleportSet(gate,{Volume,500,NoActors,false,true,true,true,false,false,true,false,true}));
 static int blockedSlot=-1,portalOpens=0;static gentity_t* oldDoor;
 oldDoor=&leader;
 gi.AdjustAreaPortalState=[](gentity_t* ent,qboolean open){
  CHECK(ent==oldDoor&&open&&ent->currentOrigin[0]==10&&ent->linked&&!(ent->flags&jarDetachedMoverAreaPortal));
  ++portalOpens;
 };
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
  *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
  CHECK(!g_entities[pass].linked&&VectorCompare(start,end)&&g_entities[pass].e_TouchFunc!=touchF_Touch_DoorTrigger);
  hit->startsolid=pass==blockedSlot?qtrue:qfalse;
 };
 assemblyArrivals.clear();int budget=4096;
 leader.e_ThinkFunc=thinkF_Think_SpawnNewDoorTrigger;leader.nextthink=100;
 CHECK(!teleportAssembly(joint.second,gate,budget));leader.nextthink=0;
 leaf.teamchain=&leader;
 CHECK(!teleportAssembly(joint.second,gate,budget));leaf.teamchain=nullptr;
 blockedSlot=leaf.s.number;
 CHECK(!teleportAssembly(joint.second,gate,budget)&&!portalOpens&&sensor.currentOrigin[0]==0&&
       disabled.currentOrigin[0]==0&&leaf.currentOrigin[0]==50&&leader.currentOrigin[0]==10);
 blockedSlot=-1;
 moverCrossingSource=&gate;teleportMoverOrigins.clear();queueAssemblyMovers(true);
 CHECK(teleportMoverOrigins.size()==2&&pendingAssemblyCrossings.empty());
 leader.currentOrigin[1]=1;
 queueAssemblyMovers(true);CHECK(pendingAssemblyCrossings.size()==1&&pendingAssemblyCrossings[0].owner==attachmentKey(leader));
 finishAssemblyCrossings(.05f);
 CHECK(portalOpens==1&&leader.currentOrigin[0]==600&&leaf.currentOrigin[0]==640&&cargo.currentOrigin[0]==610&&
       leader.pos2[1]==54&&leaf.pos2[1]==54&&leader.teamchain==&leaf&&leaf.teammaster==&leader&&
       (leader.flags&jarDetachedMoverAreaPortal)&&(leaf.flags&jarDetachedMoverAreaPortal)&&assemblyArrivals.size()==3);
 for(auto* trigger:{&sensor,&disabled}){
  CHECK(trigger->currentOrigin[0]==590&&trigger->s.pos.trBase[0]==590&&trigger->mins[0]+trigger->currentOrigin[0]==588&&
        trigger->maxs[0]+trigger->currentOrigin[0]==652&&trigger->owner==&leader&&
        trigger->e_TouchFunc==touchF_Touch_DoorTrigger);
 }
 CHECK(sensor.linked&&!disabled.linked&&(disabled.svFlags&SVF_INACTIVE));
 G_AdjustMoverAreaPortalState(&leader,qfalse);
 G_AdjustMoverAreaPortalState(&leaf,qtrue);
 CHECK(portalOpens==1);
 usedBinaryMover=nullptr;
 Touch_DoorTrigger(&disabled,&cargo,nullptr);CHECK(!usedBinaryMover);
 Touch_DoorTrigger(&sensor,&cargo,nullptr);CHECK(usedBinaryMover==&leader);
 assemblyArrivals.clear();target.currentOrigin[0]=900;
 CHECK(teleportAssembly(attachmentKey(leader),gate,budget)&&portalOpens==1&&sensor.currentOrigin[0]==890);
 CHECK(ForgetJoint(state,weld));teleportMoverOrigins.clear();moverCrossingSource=nullptr;gi.trace=priorTrace;

 auto& deck=*native("func_plat");auto& bottom=*native("");
 deck.bmodel=qtrue;deck.model=G_NewString("*3");deck.s.modelindex=3;deck.s.eType=ET_MOVER;
 deck.e_ReachedFunc=reachedF_Reached_BinaryMover;deck.moverState=MOVER_POS2;
 VectorSet(deck.mins,-8,-8,-4);VectorSet(deck.maxs,8,8,4);G_SetOrigin(&deck,origin);
 VectorCopy(origin,deck.pos2);VectorSet(deck.pos1,10,0,70);gi.linkentity(&deck);
 bottom.owner=&deck;bottom.contents=CONTENTS_TRIGGER;bottom.e_TouchFunc=touchF_Touch_PlatCenterTrigger;
 VectorSet(bottom.mins,8,-2,66);VectorSet(bottom.maxs,12,2,78);gi.linkentity(&bottom);
 joint.first=attachmentKey(deck);
 const auto deckLink=AddJoint(state,joint);CHECK(deckLink);
 assemblyArrivals.clear();
 CHECK(teleportAssembly(joint.first,gate,budget)&&bottom.currentOrigin[0]==890&&portalOpens==1&&
       bottom.owner==&deck&&bottom.e_TouchFunc==touchF_Touch_PlatCenterTrigger&&bottom.linked);
 CHECK(ForgetJoint(state,deckLink));
}

void CheckSectionAssemblyTeleport(){
 using namespace jar::teleport;
 const vec3_t roomStart={20,0,100},cargoStart={20,150,110},exit={600,15,30};
 auto& room=*Make(Catalog("Bespin Courtyard"),roomStart);
 room.classname=G_NewString("misc_bsp");room.model=G_NewString("#mp/duel1");
 room.e_UseFunc=useF_target_teleporter_use;room.bmodel=qtrue;room.s.eType=ET_MOVER;room.s.modelindex=11;
 room.contents=CONTENTS_SOLID;VectorCopy(roomStart,room.pos1);gi.linkentity(&room);
 VectorSet(room.mins,-100,-100,-20);VectorSet(room.maxs,100,100,60);
 const auto native=[](const char* classname){
  const auto slot=nextSlot++;globals.num_entities=std::max(globals.num_entities,int(nextSlot));
  auto& entity=g_entities[slot];entity.inuse=qtrue;entity.s.number=slot;entity.classname=G_NewString(classname);
  return &entity;
 };
 auto& platform=*native("func_bobbing");auto& hazard=*native("trigger_hurt");
 platform.bmodel=qtrue;platform.model=G_NewString("*1");platform.s.eType=ET_MOVER;
 platform.s.modelindex=12;platform.contents=CONTENTS_SOLID;platform.s.pos.trType=TR_SINE;
 platform.s.pos.trTime=750;platform.s.pos.trDuration=3000;platform.s.pos.trDelta[2]=20;
 VectorSet(platform.currentOrigin,35,40,110);VectorSet(platform.s.origin,35,40,100);
 VectorCopy(platform.s.origin,platform.s.pos.trBase);gi.linkentity(&platform);
 hazard.contents=CONTENTS_TRIGGER;hazard.e_TouchFunc=touchF_hurt_touch;hazard.damage=50;
 VectorSet(hazard.pos1,-8,-8,-8);VectorSet(hazard.pos2,8,8,8);
 const vec3_t hazardStart={21,30,120};G_SetOrigin(&hazard,hazardStart);VectorCopy(hazardStart,hazard.s.origin);gi.linkentity(&hazard);
 placedSection=&room;placedSectionChildren={&platform,&hazard};
 auto& cargo=*Make(1001,cargoStart);owned(cargo.s.number)->frozen=0;gi.linkentity(&cargo);
 auto& gate=*spawn(volumeId,roomStart,vec3_origin);auto& target=*Make(destinationId,exit);gate.target=target.targetname;
 auto& actor=*Make(3001,roomStart);actor.client->ps.groundEntityNum=platform.s.number;gi.linkentity(&actor);
 jar::constraint::Joint joint;joint.kind=jar::constraint::Weld;joint.first=attachmentKey(platform);joint.second=attachmentKey(cargo);
 const auto weld=AddJoint(state,joint);CHECK(weld);
 CHECK(G_TeleportSet(gate,{Volume,500,Actors,false,true,true,true,false,false,true,false,true}));
 static gentity_t* outsider;static int roomSlot,platformSlot,hazardSlot,blockedSlot=-1;
 outsider=&actor;roomSlot=room.s.number;platformSlot=platform.s.number;hazardSlot=hazard.s.number;
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** out,int){if(!outsider)return 0;out[0]=outsider;return 1;};
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int,EG2_Collision,int){
  CHECK(VectorCompare(start,end)&&!g_entities[platformSlot].linked&&!g_entities[hazardSlot].linked&&
        pass!=platformSlot&&pass!=hazardSlot);
  if(pass==roomSlot)CHECK(mins[0]==-100&&maxs[0]==100);
  *hit={};hit->fraction=1;hit->startsolid=pass==blockedSlot?qtrue:qfalse;
 };
 assemblyArrivals.clear();int budget=4096;
 CHECK(!teleportAssembly(joint.second,gate,budget));
 outsider=nullptr;blockedSlot=room.s.number;
 CHECK(!teleportAssembly(joint.second,gate,budget)&&VectorCompare(room.currentOrigin,roomStart)&&
       VectorCompare(hazard.currentOrigin,hazardStart)&&platform.linked&&hazard.linked&&assemblyArrivals.empty());
 blockedSlot=-1;
 joint.second=attachmentKey(actor);
 const auto actorLink=AddJoint(state,joint);CHECK(actorLink);
 outsider=&actor;
 teleportMoverOrigins.clear();queueAssemblyMovers(true);
 CHECK(teleportMoverOrigins.size()==1&&teleportMoverOrigins.count(room.s.number));
 CHECK(teleportAssembly(attachmentKey(room),gate,budget)&&room.currentOrigin[0]==600&&room.currentOrigin[2]==51&&
       VectorCompare(room.currentOrigin,room.pos1)&&cargo.currentOrigin[0]==600&&cargo.currentOrigin[2]==61);
 CHECK(platform.currentOrigin[0]==615&&platform.currentOrigin[2]==61&&platform.s.pos.trBase[2]==51&&
       platform.s.pos.trType==TR_SINE&&platform.s.pos.trTime==750&&platform.s.pos.trDuration==3000);
 CHECK(hazard.currentOrigin[0]==601&&hazard.currentOrigin[2]==71&&hazard.damage==50&&hazard.pos1[2]==-8&&
       hazard.e_TouchFunc==touchF_hurt_touch&&hazard.linked&&platform.linked&&assemblyArrivals.size()==3);
 CHECK(actor.currentOrigin[0]==600&&actor.client->ps.groundEntityNum==platform.s.number);
 CHECK(G_SubBSPOwner(&platform)==&room&&ForgetJoint(state,weld)&&ForgetJoint(state,actorLink));
 gi.trace=priorTrace;outsider=nullptr;placedSection=nullptr;placedSectionChildren.clear();teleportMoverOrigins.clear();
}

void CheckWorldAnchorAssemblyTeleport(){
 using namespace jar::teleport;
 using namespace jar::constraint;
 const vec3_t start={0,0,30},peerStart={40,0,30},otherStart={120,100,30},exit={500,0,19};
 auto& cargo=*Make(1001,start);auto& peer=*Make(1002,peerStart);auto& other=*Make(1001,otherStart);
 for(auto* member:{&cargo,&peer,&other}){owned(member->s.number)->frozen=0;gi.linkentity(member);}
 auto& gate=*spawn(volumeId,start,vec3_origin);auto& target=*Make(destinationId,exit);gate.target=target.targetname;
 auto& world=g_entities[ENTITYNUM_WORLD];world.s.number=ENTITYNUM_WORLD;
 const auto worldId=NativeId(ENTITYNUM_WORLD),cargoId=attachmentKey(cargo),peerId=attachmentKey(peer);
 Joint joint;
 joint.kind=Rope;joint.first=worldId;joint.second=cargoId;joint.length=80;
 VectorSet(joint.a.position,0,0,100);joint.a.rotation[0]=1;joint.a.rotation[3]=0;
 const auto rope=AddJoint(state,joint);CHECK(rope);
 joint.second=attachmentKey(other);VectorSet(joint.a.position,120,100,100);
 const auto unrelated=AddJoint(state,joint);CHECK(unrelated);
 joint={};joint.kind=Hinge;joint.first=peerId;joint.second=worldId;
 VectorSet(joint.b.position,40,0,100);
 const auto hinge=AddJoint(state,joint);CHECK(hinge);
 joint={};joint.kind=Weld;joint.first=cargoId;joint.second=peerId;
 const auto weld=AddJoint(state,joint);CHECK(weld);
 const auto link=[](uint32_t id)->const Joint&{
  for(unsigned i=0;i<state.jointCount;++i)if(state.joints[i].id==id)return state.joints[i];
  throw std::runtime_error("Expected world anchor link");
 };
 static gentity_t* lead;static gentity_t* neighbor;static gentity_t* outsider;
 static bool sourceMissing,destinationSloped,routeBlocked;
 static unsigned surfaceTraces,routeTraces;
 lead=&cargo;neighbor=&peer;outsider=&other;
 const auto priorTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_NONE;VectorCopy(end,hit->endpos);
  CHECK(outsider->linked&&outsider->currentOrigin[0]==120&&!lead->linked);
  if(pass!=ENTITYNUM_NONE){CHECK(!neighbor->linked&&VectorCompare(start,end));return;}
  if(!mins){
   ++surfaceTraces;
   CHECK(mask==MASK_SOLID&&!neighbor->linked&&(start[0]==0||start[0]==40||start[0]==500||start[0]==540));
   if(start[2]>100||(sourceMissing&&start[0]<500))return;
   hit->fraction=.46875f;hit->entityNum=ENTITYNUM_WORLD;
   VectorSet(hit->endpos,start[0],start[1],99.875f);
   VectorSet(hit->plane.normal,0,0,-1);
   if(destinationSloped&&start[0]>=500)VectorSet(hit->plane.normal,.6f,0,-.8f);
   return;
  }
  ++routeTraces;
  CHECK(mask==JAR_ROPE_PATH_CONTENTS&&neighbor->linked&&neighbor->currentOrigin[0]==540);
  CHECK(start[0]==500&&end[0]==500&&start[2]<99&&start[2]>90&&end[2]==30);
  if(routeBlocked){hit->startsolid=qtrue;hit->fraction=0;VectorCopy(start,hit->endpos);}
 };
 assemblyArrivals.clear();ropeStates[rope]=1;ropeStates[unrelated]=1;
 Settings settings{Volume,500,NoActors,false,true,true,true,false,false,true};
 CHECK(G_TeleportSet(gate,settings));
 int budget=4096;
 CHECK(!teleportAssembly(cargoId,gate,budget)&&!surfaceTraces);
 settings.worldAnchors=true;CHECK(G_TeleportSet(gate,settings));
 sourceMissing=true;CHECK(!teleportAssembly(cargoId,gate,budget)&&!routeTraces);sourceMissing=false;
 destinationSloped=true;CHECK(!teleportAssembly(cargoId,gate,budget)&&!routeTraces);destinationSloped=false;
 routeBlocked=true;CHECK(!teleportAssembly(cargoId,gate,budget)&&routeTraces);routeBlocked=false;
 CHECK(VectorCompare(cargo.currentOrigin,start)&&VectorCompare(peer.currentOrigin,peerStart)&&cargo.linked&&peer.linked&&
       link(rope).a.position[0]==0&&link(hinge).b.position[0]==40&&ropeStates.count(rope)&&assemblyArrivals.empty());
 CHECK(teleportAssembly(cargoId,gate,budget)&&cargo.currentOrigin[0]==500&&peer.currentOrigin[0]==540&&
       cargo.currentOrigin[2]==30&&peer.currentOrigin[2]==30&&cargo.linked&&peer.linked&&assemblyArrivals.size()==2);
 CHECK(link(rope).a.position[0]==500&&link(rope).a.position[2]==100&&link(rope).a.rotation[0]==1&&link(rope).length==80&&
       link(hinge).b.position[0]==540&&link(hinge).b.position[2]==100&&link(hinge).b.rotation[3]==1&&!ropeStates.count(rope));
 CHECK(VectorCompare(other.currentOrigin,otherStart)&&link(unrelated).a.position[0]==120&&ropeStates.count(unrelated));
 for(const auto id:{weld,hinge,unrelated,rope})CHECK(ForgetJoint(state,id));
 ropeStates.erase(unrelated);assemblyArrivals.clear();gi.trace=priorTrace;
}
