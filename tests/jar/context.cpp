// Production entity-editor, undo and native controller transactions, headless.
#include "game/g_local.h"
#include "game/g_workshop.h"
#include "game/g_scene_camera.h"
#include "game/g_rail_portable.h"
#include "game/g_volume_geometry.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_reference_tags.h"
#include "game/g_combat_points.h"
#include "game/g_level_change.h"
#include "game/g_vehicles.h"
#include "game/g_workshop_vehicle_status.h"
#include "game/g_workshop_npc_loadout.h"
#include "game/g_workshop_npc_order.h"
#include "game/g_npc_ammo.h"
#include "game/g_sandbox_vehicle_push.h"
#include "game/g_sandbox_vehicle_transport.h"
#include "game/g_sandbox_secret.h"
#include "game/g_workshop_pickup_quantity.h"
#include "game/jar_physics.h"
#include "qcommon/jar_workshop_state.h"
#include "qcommon/jar_duplicate.h"
#include "qcommon/jar_context.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "test_support.h"
#include "bsp_section_motion_stub.h"
#include "memory_save.h"
#include "game/g_script_parms.h"
#include "qcommon/jar_music.h"
#include <array>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <deque>
#include <unordered_set>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
weaponData_t weaponData[WP_NUM_WEAPONS]{};
gitem_t bg_itemlist[3]{};
const int bg_numItems=3;
struct TestGhoulPool:IGhoul2InfoArray {
 std::map<int,std::vector<CGhoul2Info>> models;
 int next=0;
 int New() override {models[++next];return next;}
 void Delete(int handle) override {models.erase(handle);}
 bool IsValid(int handle) const override {return models.count(handle)!=0;}
 std::vector<CGhoul2Info>& Get(int handle) override {return models.at(handle);}
 const std::vector<CGhoul2Info>& Get(int handle) const override {return models.at(handle);}
} testGhoulPool;
IGhoul2InfoArray& _TheGhoul2InfoArray(){return testGhoulPool;}
gentity_t g_entities[MAX_GENTITIES];
bool G_JAR_WorkshopPropDepth(const gentity_t&,const vec3_t,const vec3_t,const vec3_t,const JarSweepHull*,float&){return false;}
static cvar_t mapGravity{};
cvar_t* g_gravity=&mapGravity;
static bool spaceMovement=false;
void G_SpaceGravity(gentity_t* e,float gravity){e->client->ps.gravity=spaceMovement?0:int(gravity);}
bool G_SpacePortable(const gentity_t& volume){return jar::volumeEntity::Portable(volume)&&volume.e_TouchFunc==touchF_space_touch;}
void G_SpaceSet(gentity_t& volume,bool armed,bool enabled){
 CHECK(G_SpacePortable(volume));volume.contents=armed?CONTENTS_TRIGGER:0;
 if(enabled)volume.svFlags&=~SVF_INACTIVE;else volume.svFlags|=SVF_INACTIVE;
}
using namespace jar::workshop;
bool G_BSPSectionBreakable(const gentity_t&){return false;}
State state;jar::physics::World physicsWorld;
constexpr uint32_t noEntity=ENTITYNUM_NONE;
uint32_t held=noEntity,selected=noEntity,holdToken=1;float holdDistance=144;
struct {int weaponSelect=WP_PHYSGUN;vec3_t refdefViewAngles{};struct {bool active=false;}overrides;} cg;
static bool on=true,blocked=false,busy=false;
static std::map<std::string,std::string> vars;
static std::vector<std::string> args;
static std::string status,action;
static std::deque<parms_t> parameterStorage;
void*G_Alloc(int size){CHECK(size==sizeof(parms_t));parameterStorage.emplace_back();return &parameterStorage.back();}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}

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
#include "game/g_laser_arm.h"
bool G_LaserArmMoved(gentity_t&e){return G_LaserArmPortable(e);}
#include "game/g_weather_properties.inc"
#include "game/g_scriptrunner_properties.inc"
#include "scriptrunner_files.h"
bool G_MusicSetTrack(gentity_t&e,const char*s){std::string text;if(!jar::music::Class(e.classname)||!jar::music::Canonical(s,text))return false;e.message=G_NewString(text.c_str());return true;}
bool G_MusicActive(const gentity_t&){return false;}
bool G_PortableMusic(const gentity_t&e){return jar::music::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_music");}
char*G_GetLocationForEnt(gentity_t*){return const_cast<char*>("Hangar");}
gentity_t* workshopPlayer(){return g_entities[0].inuse&&g_entities[0].client?&g_entities[0]:nullptr;}
bool enabled(){return on;}bool usable(){return !g_entities[0].s.m_iVehicleNum;}bool sandboxReady(){return enabled()&&usable();}
static int actorGroupsLeft=0;
static int actorFireReconfigurations=0;
static int attachedWeaponFailure=0,registeredWeapons=0;
static bool npcOrderScripted=false;
bool G_WorkshopNPCOrderScripted(gentity_t&){return npcOrderScripted;}
void RegisterItem(gitem_t*){++registeredWeapons;}
qboolean G_StandardHumanoid(gentity_t* actor){return actor->ghoul2.size()>0?qtrue:qfalse;}
void G_CreateG2AttachedWeaponModel(gentity_t* actor,const char* model,int bolt,int slot){
 CHECK(model[0]&&slot==0&&bolt==actor->handRBolt&&actor->weaponModel[0]==-1);
 if(attachedWeaponFailure==1)return;
 int index=1;
 while(index<actor->ghoul2.size()&&actor->ghoul2[index].mModelindex>=0)++index;
 if(index==actor->ghoul2.size())actor->ghoul2.resize(index+1);
 auto& instance=actor->ghoul2[index];
 instance.mModelindex=index;instance.mModelBoltLink=attachedWeaponFailure==2?-1:bolt;
 Q_strncpyz(instance.mFileName,model,sizeof(instance.mFileName));actor->weaponModel[0]=index;
}
#include "game/g_workshop_hand_model.inc"
void Add_Ammo(gentity_t* actor,int weapon,int amount){
 actor->client->ps.ammo[weaponData[weapon].ammoIndex]+=amount;
}
qboolean G_CanPickUpWeapons(const gentity_t* e){return e->client->NPC_class!=CLASS_RANCOR?qtrue:qfalse;}
void ChangeWeapon(gentity_t* e,int weapon){
 e->client->ps.weapon=weapon;e->NPC->currentAmmo=e->client->ps.ammo[weaponData[weapon].ammoIndex];
 ++actorFireReconfigurations;
}
void AI_DeleteSelfFromGroup(gentity_t* e){CHECK(e->NPC->group);e->NPC->group=nullptr;++actorGroupsLeft;}
void G_ClearEnemy(gentity_t* e){if(e->NPC->goalEntity==e->enemy)e->NPC->goalEntity=nullptr;e->enemy=nullptr;}
qboolean G_ValidEnemy(gentity_t* e,gentity_t* enemy){
 return e->client->playerTeam!=enemy->client->playerTeam&&e->client->enemyTeam==enemy->client->playerTeam?qtrue:qfalse;
}
bool occupied(const gentity_t&){return busy;}
bool removalOccupied(const gentity_t& e){return occupied(e);}
void message(const char*s){status=s;}
Owned*owned(unsigned entity){for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].entity==entity)return &state.owned[i];return nullptr;}
struct Prop {bool rigid=true;const char*model="crate.md3";} testProp;
uint32_t attachmentKey(const gentity_t&e){auto*o=owned(e.s.number);return o?o->id:jar::constraint::NativeId(e.s.number);}
bool physicsEligible(const Prop&p,int=0){return p.rigid;}
const Prop*prop(unsigned id){return id==1001?&testProp:nullptr;}
void forgetEdits(unsigned entity);
void G_JAR_WorkshopForget(gentity_t* entity,bool){forgetEdits(entity->s.number);Forget(state,entity->s.number);}
void G_FreeEntity(gentity_t* entity){G_JAR_WorkshopForget(entity);entity->inuse=qfalse;}
bool matches(const Owned&);
gentity_t* attachmentEntity(uint32_t key){
 const auto* record=OwnedId(state,key);
 const auto slot=jar::constraint::Native(key)?jar::constraint::NativeEntity(key):record?record->entity:unsigned(ENTITYNUM_NONE);
 return slot==ENTITYNUM_WORLD||(slot<ENTITYNUM_WORLD&&g_entities[slot].inuse)?&g_entities[slot]:nullptr;
}
static bool ropeResizeBlocked=false;
bool resizeRope(jar::constraint::Joint& joint,float length){
 if(ropeResizeBlocked)return false;
 joint.length=length;return true;
}
bool G_JAR_WorkshopOwns(const gentity_t* e){const auto* entry=e?owned(e->s.number):nullptr;return entry&&matches(*entry);}
#include "game/g_workshop_remove.inc"
bool removeJoint(unsigned){return false;}
void eye(float*origin,float*direction){VectorClear(origin);VectorClear(direction);direction[0]=1;}
void NPC_SetAnim(gentity_t*e,int,int anim,int,int){e->client->ps.torsoAnim=anim;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);if(e->client)VectorCopy(p,e->client->ps.origin);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);e->s.apos.trType=TR_STATIONARY;}
void SetClientViewAngle(gentity_t*e,vec3_t a){VectorCopy(a,e->client->ps.viewangles);}
#include "game/g_sliding_door.h"
#include "game/g_button_sandbox.h"
#include "game/g_bobbing_sandbox.h"
bool setupMoved(gentity_t&e){return G_SandboxSlideMoved(e)||G_ButtonMoved(e)||G_BobbingMoved(e)||jar::volumeEntity::Refresh(e)||G_WeatherPortable(e)||G_CloudMoved(e)||G_ReflectionMoved(e);}
void physicsInvalidate(unsigned entity){physicsWorld.Forget(entity);}
#include "game/g_mover_motion.h"
#include "game/g_angular_cargo.h"
#include "game/g_workshop_native_control.inc"
#include "game/g_workshop_pickup_lifetime.inc"
#include "sliding_door_fixture.h"
bool in_camera=false;
gentity_t* G_TrySpawn(qboolean){throw std::runtime_error("Shield property editing must not allocate");}
#include "game/g_portable_shield.inc"
jar::physics::Vec pv(const float* v){return {v[0],v[1],v[2]};}
void nativeVector(jar::physics::Vec v,float* out){VectorSet(out,v.x,v.y,v.z);}
std::shared_ptr<const jar::physics::PropGeometry> propTransformGeometry(const Prop&,int);
const jar::physics::NativePropHull* propTraceHull(const gentity_t&,const vec3_t);
#include "game/g_workshop_rack_pose.inc"
#include "game/g_workshop_lift.inc"
bool G_LiftTranslate(gentity_t& e,float dz){return liftTranslate(e,dz);}
void releaseHeld(){if(held!=noEntity){nativeRelease(held,true);physicsWorld.Release(held);}held=noEntity;physgunGrip={};}
bool matches(const Owned&o){
 const auto&e=g_entities[o.entity];
 return e.inuse&&e.classname&&(!strcmp(e.classname,"jar_sandbox_prop")||
  (e.targetname&&!strcmp(e.targetname,va("jar_sb_%u",o.id))));
}
 int propPoseFrames(const Prop&){return 8;}
 bool propHull(const Prop&,const vec3_t,vec3_t lo,vec3_t hi,int frame){VectorSet(lo,-8,-8,-8);VectorSet(hi,8+float(frame),8,8);return frame!=6;}
 #include "game/g_workshop_angular_cargo.inc"
 bool skeletonPose(CGhoul2Info_v&,int){throw std::runtime_error("Unexpected skeletal model in editor MD3 fixture");}
 int physicsFrozenContents(const gentity_t&){return CONTENTS_BODY;}
 bool setPropPose(gentity_t&,int);
gentity_t*G_Find(gentity_t* after,int field,const char* name){
 if(field!=FOFS(targetname)||!name)return nullptr;
 for(int slot=after?after->s.number+1:84;slot<=96;++slot){
  auto& entity=g_entities[slot];
  if(entity.inuse&&entity.targetname&&!strcmp(entity.targetname,name))return &entity;
 }
 return nullptr;
}
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
bool G_SubBSPNameVisible(const gentity_t*){return true;}
static uint64_t portalCameraLifetime=1;
uint64_t G_EntityLifetime(const gentity_t* entity){
 return entity&&entity->inuse?(entity->s.number==96?portalCameraLifetime:1):0;
}
#include "reference_fixture.h"
static bool waypointAdmission(){return true;}
#include "game/g_waypoint_properties.inc"
const char*G_WaypointStatus(const gentity_t&){return "Native graph tested separately";}
static int undoFeedbackCount;void undoFeedback(){++undoFeedbackCount;}
#include "game/g_logic_settings.h"
#include "game/g_workshop_give.inc"
static std::shared_ptr<const jar::physics::PropGeometry> clearanceGeometry;
static std::map<int,std::shared_ptr<const jar::physics::PropGeometry>> frameClearanceGeometry;
std::shared_ptr<const jar::physics::PropGeometry> propTransformGeometry(const Prop&,int frame){
 const auto found=frameClearanceGeometry.find(frame);
 return found==frameClearanceGeometry.end()?clearanceGeometry:found->second;
}
const jar::physics::NativePropHull* propTraceHull(const gentity_t& entity,const vec3_t angles){
 static jar::physics::NativePropHull hull;
 const auto geometry=propTransformGeometry(testProp,entity.s.frame);
 return geometry&&hull.Build(*geometry,pv(angles))?&hull:nullptr;
}
#include "game/g_workshop_edits.inc"
#include "game/g_workshop_pose.inc"
#include "game/g_workshop_use_gate.inc"
bool undoSpawnGroup(const std::vector<SpawnedEdit>&){throw std::runtime_error("Unexpected assembly undo in property fixture");}
void selectAt(float,float){selected=7;}
void duplicatePaste(){action="paste";}
void tool(const char*name){action=name;if(action=="remove")g_entities[selected].inuse=qfalse;}
void attachmentRope(gentity_t&e,const vec3_t p,const vec3_t){CHECK(e.s.number==int(selected)&&VectorCompare(p,e.currentOrigin));action="rope";}
void attachmentThruster(gentity_t&e,bool,const vec3_t p,const vec3_t axis){CHECK(e.s.number==int(selected)&&VectorCompare(p,e.currentOrigin)&&axis[2]==1);action="thruster";}
void unlinkTool(){action="unlink";}
void constraintLink(gentity_t&e,const vec3_t p,const vec3_t axis,unsigned kind){CHECK(e.s.number==int(selected)&&VectorCompare(p,e.currentOrigin)&&axis[2]==1);action=kind==jar::constraint::Weld?"weld":"hinge";}
#include "game/g_workshop_context.inc"
static bool nativePortal=false;
static std::vector<bool> nativePropSegments;
static unsigned nativePushes=0;
static float nativeImpact=0;
bool teleportPropContacts(gentity_t&,const vec3_t,bool rigid,bool clipped){
 CHECK(!rigid);nativePropSegments.push_back(clipped);return nativePortal;
}
void pushPropContacts(gentity_t&,const vec3_t,bool rigid){CHECK(!rigid);++nativePushes;}
void impactSound(unsigned,float speed){nativeImpact=speed;}
const JarSweepHull* propMovingHull(const gentity_t&){return nullptr;}
#include "game/g_workshop_native_motion.inc"
bool G_JAR_MoverTranslate(gentity_t&,const vec3_t,vec3_t actual){VectorClear(actual);return false;}
bool G_JAR_MoverRotate(gentity_t&,const vec3_t){return false;}
bool G_JAR_MoverRotateAt(gentity_t& e,const vec3_t angles,const vec3_t origin,const vec3_t){
 CHECK(e.bmodel&&origin);
 if(blocked)return false;
 vec3_t offset;
 VectorSubtract(origin,e.currentOrigin,offset);
 VectorAdd(e.pos1,offset,e.pos1);VectorAdd(e.pos2,offset,e.pos2);
 VectorAdd(e.s.pos.trBase,offset,e.s.pos.trBase);
 VectorCopy(origin,e.currentOrigin);VectorCopy(angles,e.currentAngles);
 return true;
}
#include "game/g_workshop_native_rotation.inc"
#include "game/g_workshop_physgun_turn.inc"
#include "game/g_mover_motion.inc"
void Run(const char*operation,std::initializer_list<const char*> values={}){
 args={"jar_sb","context",operation,std::to_string(contextToken)};for(auto*s:values)args.emplace_back(s);contextCommand();
}
void Revert(){revertProperty();}
int main(){return RunTests("context selections, bounded properties, native trajectories, undo and rotation gestures",[]{
 ScriptFiles();gi.FS_ReadFile=[](const char*path,void**out)->long{CHECK(!out);return !strcmp(path,"maps/t1_surprise.bsp")||!strcmp(path,"maps/hoth2.bsp")?100:-1;};
 gi.cvar_set=[](const char*k,const char*v){vars[k]=v;};
 gi.argv=[](int i)->char*{return const_cast<char*>(i<int(args.size())?args[i].c_str():"");};
 gi.linkentity=[](gentity_t*){};
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->startsolid=blocked?qtrue:qfalse;t->fraction=1;VectorCopy(end,t->endpos);};
 {
  auto& trail=g_entities[84];auto& endpoint=g_entities[85];
  trail.inuse=endpoint.inuse=qtrue;trail.s.number=84;endpoint.s.number=85;
  trail.classname=G_NewString("fx_explosion_trail");
  CHECK(Own(state,84,100474240));
  trail.targetname=G_NewString(va("jar_sb_%u",owned(84)->id));
  endpoint.targetname=G_NewString("trail_context_endpoint");trail.target=G_NewString(endpoint.targetname);
  VectorSet(trail.currentAngles,0,90,0);VectorSet(endpoint.currentOrigin,40,0,30);
  selected=84;contextSelect();CHECK(vars["jar_contextTrailAllowed"]=="1"&&vars["jar_contextTrailLinked"]=="1");
  CHECK(vars["jar_contextTrailAim"]=="Aim: entity 85");
  vec3_t direction;
  Run("trail_unlink");CHECK(!trail.target&&vars["jar_contextTrailAim"]=="Aim: emitter facing");
  G_WorkshopTrailDirection(trail,direction);CHECK(std::fabs(direction[0])<.001f&&direction[1]>.999f);
  Run("revert");
  CHECK(trail.target&&!strcmp(trail.target,endpoint.targetname)&&vars["jar_contextTrailLinked"]=="1");
  Run("trail_unlink");endpoint.inuse=qfalse;Run("revert");
  CHECK(!trail.target&&edits.size()==1);
  Forget(state,84);trail={};endpoint={};edits.clear();contextClear();selected=noEntity;
 }
 {
  using namespace jar::constraint;
  auto& root=g_entities[47];root.inuse=qtrue;root.s.number=47;
  auto& peer=g_entities[46];peer.inuse=qtrue;peer.s.number=46;peer.classname="test_anchor";
  Joint rope;rope.kind=Rope;rope.first=NativeId(47);rope.second=NativeId(46);rope.length=120;
  const auto first=AddJoint(state,rope);CHECK(first);
  rope.second=NativeId(ENTITYNUM_WORLD);rope.length=80;
  const auto second=AddJoint(state,rope);CHECK(second);
  const auto firstText=std::to_string(first),secondText=std::to_string(second);
  g_entities[ENTITYNUM_WORLD].s.number=ENTITYNUM_WORLD;
  selected=47;contextSelect();const auto token=contextToken;
  CHECK(vars["jar_contextRopeCount"]=="2"&&vars["jar_contextRopeId"]==firstText&&
   vars["jar_contextRopeEnd"]=="Other end: 46 test_anchor");
  Run("rope_next");CHECK(contextToken!=token&&vars["jar_contextRopeId"]==secondText&&vars["jar_contextRopeEnd"]=="Other end: World");
  args={"jar_sb","context","rope_length",std::to_string(token),secondText,"99"};contextCommand();
  Run("rope_length",{firstText.c_str(),"99"});
  for(const auto* invalid:{"nan","0","8193","10;quit"})Run("rope_length",{secondText.c_str(),invalid});
  ropeResizeBlocked=true;Run("rope_length",{secondText.c_str(),"99"});ropeResizeBlocked=false;
  CHECK(state.joints[0].length==120&&state.joints[1].length==80&&edits.empty());
  Run("rope_length",{secondText.c_str(),"99.5"});
  CHECK(state.joints[1].length==99.5f&&edits.size()==1&&edits.back().context&&!isCreation(edits.back()));
  CHECK(!undoCreation()&&state.jointCount==2);
  const auto bytes=Encode(state);State decoded;
  CHECK(Decode(bytes.data(),bytes.size(),decoded,ENTITYNUM_WORLD)&&decoded.joints[1].length==99.5f);
  state.joints[1].length=100;Run("revert");CHECK(edits.size()==1&&state.joints[1].length==100);
  state.joints[1].length=99.5f;Run("revert");CHECK(edits.empty()&&state.joints[1].length==80);
  Run("rope_length",{secondText.c_str(),"100"});CHECK(edits.size()==1);
  CHECK(ForgetJoint(state,second));forgetJointEdit(second);Run("refresh");
  CHECK(edits.empty()&&vars["jar_contextRopeId"]==firstText&&vars["jar_contextRopeCount"]=="1");
  CHECK(ForgetJoint(state,first));root={};peer={};contextClear();
 }
 {
  using namespace jar::constraint;
  auto& mount=g_entities[47];mount.inuse=qtrue;mount.s.number=47;
  Joint jet;jet.kind=Thruster;jet.first=jet.second=NativeId(47);jet.force=5000;
  jet.a.position[0]=jet.b.position[0]=8;
  const auto id=AddJoint(state,jet);CHECK(id);const auto idText=std::to_string(id);
  selected=47;contextSelect();const auto token=contextToken;
  CHECK(vars["jar_contextThrusterId"]==idText&&vars["jar_contextThrusterEnabled"]=="1");
  Run("thruster_properties",{"999","4000","1","0"});
  args={"jar_sb","context","thruster_properties",std::to_string(token),idText,"6000","1","1"};contextCommand();
  for(const auto* invalid:{"0","100001","inf","10;quit"})Run("thruster_properties",{idText.c_str(),invalid,"0","1"});
  CHECK(state.joints[0].force==5000&&state.joints[0].enabled&&edits.empty());
  Run("thruster_properties",{idText.c_str(),"6500.125","0","1"});
  auto& live=state.joints[0];
  CHECK(live.force==6500.125f&&!live.enabled&&live.a.position[0]==8&&live.b.position[0]==8&&live.a.rotation[0]==1);
  CHECK(edits.size()==1&&edits.back().context&&!isCreation(edits.back())&&!undoCreation());
  const auto bytes=Encode(state);State loaded;
  CHECK(Decode(bytes.data(),bytes.size(),loaded,ENTITYNUM_WORLD)&&ThrusterProperties(live).Matches(loaded.joints[0]));
  live.enabled=1;Run("revert");CHECK(edits.size()==1&&live.force==6500.125f);
  live.enabled=0;Run("revert");CHECK(edits.empty()&&ThrusterProperties(jet).Matches(live));
  Run("thruster_properties",{idText.c_str(),"5000","0","1"});CHECK(edits.size()==1);
  CHECK(ForgetJoint(state,id));forgetJointEdit(id);Run("refresh");
  CHECK(edits.empty()&&vars["jar_contextThrusterId"]=="0");mount={};contextClear();
 }
 {static std::string skyConfig;gi.GetConfigstring=[](int n,char*out,int size){CHECK(n==CS_SKYBOXORG);Q_strncpyz(out,skyConfig.c_str(),size);};gi.SetConfigstring=[](int n,const char*text){CHECK(n==CS_SKYBOXORG);skyConfig=text;};
 auto&sky=g_entities[45];sky.inuse=qtrue;sky.s.number=45;sky.classname=G_NewString("misc_skyportal");sky.targetname=G_NewString("context_sky");CHECK(G_SkyPortalSpawn(sky));selected=45;contextSelect();const auto initial=contextToken;CHECK(vars["jar_contextSkyAllowed"]=="1"&&vars["jar_contextSkyEnabled"]=="0");
 Run("sky",{"1","0"});CHECK(!sky.count&&G_SkyPortalOrder(sky)==0&&edits.size()==1);Revert();CHECK(sky.count==1&&G_SkyPortalOrder(sky)==-1&&skyConfig.empty());
 for(const char*bad:{"2","-1","0.5","nan","1;quit"}){Run("sky",{bad,"1"});CHECK(edits.empty());}
 args={"jar_sb","context","sky",std::to_string(initial),"1","1"};contextCommand();CHECK(G_SkyPortalOrder(sky)==-1);G_SkyPortalForget(&sky);sky.inuse=qfalse;}
 {auto&point=g_entities[44];point.inuse=qtrue;point.s.number=44;globals.num_entities=64;point.classname=G_NewString("point_combat");CHECK(G_CombatPointSpawn(point));selected=44;contextSelect();CHECK(vars["jar_contextCombatAllowed"]=="1");const auto initial=contextToken;
 Run("combat",{"34","0"});CHECK(point.spawnflags==34&&(point.svFlags&SVF_INACTIVE)&&edits.size()==1);Revert();CHECK(!point.spawnflags&&!(point.svFlags&SVF_INACTIVE)&&edits.empty());
 for(const char*bad:{"64","-1","1.5","nan","1;quit"}){Run("combat",{bad,"1"});CHECK(edits.empty());}
 args={"jar_sb","context","combat",std::to_string(initial),"2","1"};contextCommand();CHECK(!point.spawnflags);G_CombatPointForget(&point);point.inuse=qfalse;}
 auto&reference=g_entities[45];reference.inuse=qtrue;reference.s.number=45;reference.classname=G_NewString("ref_tag");reference.targetname=G_NewString("jar_ref_context");CHECK(G_ReferenceSpawn(reference));
 selected=45;contextSelect();CHECK(vars["jar_contextReferenceAllowed"]=="1");const auto referenceToken=contextToken;
 Run("reference",{Hex("PointAlpha").c_str(),Hex("squadA").c_str(),"42","0"});CHECK(!strcmp(reference.message,"pointalpha")&&!strcmp(reference.ownername,"squadA")&&reference.radius==42&&(reference.svFlags&SVF_INACTIVE)&&edits.size()==1);
 auto&collision=g_entities[46];collision.inuse=qtrue;collision.s.number=46;collision.classname=G_NewString("ref_tag");collision.targetname=G_NewString("collision");CHECK(G_ReferenceSpawn(collision));CHECK(G_ReferenceSet(collision,jar::reference::Settings{"jar_ref_context","",16,true}));
 Revert();CHECK(edits.size()==1&&!strcmp(reference.message,"pointalpha"));G_ReferenceForget(&collision);collision.inuse=qfalse;Revert();CHECK(edits.empty()&&!strcmp(reference.message,"jar_ref_context")&&!reference.ownername&&reference.radius==16&&!(reference.svFlags&SVF_INACTIVE));
 for(const char*bad:{"bad;quit","","two words","\\bad"}){Run("reference",{Hex(bad).c_str(),"","16","1"});CHECK(edits.empty());}
 for(const char*bad:{"-1","1025","1.5","nan"}){Run("reference",{Hex("valid").c_str(),"",bad,"1"});CHECK(edits.empty());}
 args={"jar_sb","context","reference",std::to_string(referenceToken),Hex("stale"),"","16","1"};contextCommand();CHECK(!strcmp(reference.message,"jar_ref_context"));
 G_ReferenceForget(&reference);reference.inuse=qfalse;
 auto&decor=g_entities[40];decor.inuse=qtrue;decor.s.number=40;decor.classname=G_NewString("jar_sandbox_prop");CHECK(Own(state,40,1001));
 selected=40;contextSelect();CHECK(vars["jar_contextPoseAllowed"]=="1"&&vars["jar_contextPoseLast"]=="7");
 Run("pose",{"4"});CHECK(decor.s.frame==4&&decor.maxs[0]==12&&edits.size()==1);
 MemorySave poseSave;ojk::SavedGameHelper poseWriter(&poseSave);decor.s.sg_export(poseWriter);poseSave.reset_buffer_offset();decor.s.frame=0;decor.s.sg_import(poseWriter);poseWriter.ensure_all_data_read();decor.maxs[0]=0;restorePropPoses();CHECK(decor.s.frame==4&&decor.maxs[0]==12);
 blocked=true;Run("pose",{"2"});CHECK(decor.s.frame==4&&edits.size()==1);Revert();CHECK(decor.s.frame==4&&edits.size()==1);blocked=false;
 Revert();CHECK(decor.s.frame==0&&decor.maxs[0]==8&&edits.empty());
 {
  using namespace jar::physics;
  auto square=std::make_shared<PropGeometry>(),diamond=std::make_shared<PropGeometry>();
  for(float z:{-8.f,8.f}){
   for(float x:{-8.f,8.f})for(float y:{-8.f,8.f})square->Add({x,y,z});
   for(float x:{-8.f,12.f})diamond->Add({x,0,z});
   for(float y:{-8.f,8.f})diamond->Add({2,y,z});
  }
  frameClearanceGeometry={{0,square},{4,diamond}};
  static NativePropHull cornerObstacle;
  PropGeometry wall;
  for(float x:{6.5f,7.5f})for(float y:{6.5f,7.5f})for(float z:{-4.f,4.f})wall.Add({x,y,z});
  CHECK(cornerObstacle.Build(wall,{}));
  static bool hasCorner=true;
  static int poseTraceCount=0;
  gi.JAR_TraceMovingHull=[](trace_t* trace,const vec3_t from,const vec3_t to,const vec3_t lo,const vec3_t hi,
   int pass,int mask,const JarSweepHull& shape){
   CHECK(pass==40&&VectorCompare(from,to)&&lo[0]<-7.9f&&hi[0]>7.9f);
   CHECK(mask==(MASK_SOLID|CONTENTS_BODY|SANDBOX_MOUNTED_CHARGE_CONTENTS));
   const auto hit=cornerObstacle.TraceHull(pv(from),pv(to),shape);
   *trace={};trace->fraction=1;
   if(hasCorner&&hit.startSolid){trace->startsolid=qtrue;trace->allsolid=qtrue;trace->fraction=0;}
   ++poseTraceCount;
  };
  CHECK(physicsWorld.Start({{-100,-100,-20},{100,-100,-20},{100,100,-20}}));
  owned(40)->frozen=0;
  Pose live;
  live.linear={36,12,7};live.angular={0,0,1.5f};
  CHECK(physicsWorld.Prop(40,owned(40)->id,{8,8,8},{},live,false));
  // The obstacle lies inside both rectangular bounds and outside frame 4's hull.
  Run("pose",{"4"});
  CHECK(poseTraceCount==1&&decor.s.frame==4&&decor.maxs[0]==12&&edits.size()==1);
  CHECK(std::fabs(decor.s.pos.trDelta[0]-36)<.001f&&std::fabs(decor.s.pos.trDelta[1]-12)<.001f&&
        std::fabs(decor.s.apos.trDelta[2]-1.5f)<.001f&&!physicsWorld.Read(40,live));
  Revert();CHECK(poseTraceCount==2&&decor.s.frame==4&&decor.maxs[0]==12&&edits.size()==1);
  hasCorner=false;
  Revert();CHECK(poseTraceCount==3&&decor.s.frame==0&&decor.maxs[0]==8&&edits.empty());
  CHECK(std::fabs(decor.s.pos.trDelta[2]-7)<.001f&&std::fabs(decor.s.apos.trDelta[2]-1.5f)<.001f);
  gi.JAR_TraceMovingHull=nullptr;frameClearanceGeometry.clear();physicsWorld.Stop();
 }
 for(const char*bad:{"-1","8","6","1.5","nan","1;quit"}){Run("pose",{bad});CHECK(decor.s.frame==0&&edits.empty());}
 const auto poseToken=contextToken;contextPublish();args={"jar_sb","context","pose",std::to_string(poseToken),"3"};contextCommand();CHECK(!decor.s.frame);
 Run("pose",{"3"});forgetEdits(40);decor.classname=G_NewString("NPC");CHECK(edits.empty()&&decor.s.frame==3); // slot reuse discards the old pose Undo
 state={};decor.inuse=qfalse;edits.clear();
 auto&e=g_entities[7];e.inuse=qtrue;e.s.number=7;e.classname=const_cast<char*>("NPC");e.takedamage=qtrue;e.health=100;gclient_t client{};e.client=&client;client.ps.gravity=800;
 selected=7;contextSelect();CHECK(contextToken&&contextValid());const auto oldToken=contextToken;
 Run("health",{"250"});CHECK(e.health==250&&client.ps.stats[STAT_HEALTH]==250&&contextToken!=oldToken);
 auto& spawned=g_entities[47];spawned.inuse=qtrue;spawned.s.number=47;spawned.classname=G_NewString("jar_sandbox_prop");CHECK(Own(state,47,1001));
 rememberEdit(spawned,true);Run("health",{"300"});undoEdit();CHECK(e.health==300&&!owned(47)&&edits.size()==2);
 Run("revert");CHECK(e.health==250);
 args={"jar_sb","context","health",std::to_string(oldToken),"500"};contextCommand();CHECK(e.health==250);
 Run("revert");CHECK(e.health==100&&client.ps.stats[STAT_HEALTH]==100);
 for(const char*invalid:{"0","-10","1.1","100001","nan","1;quit","1e5","+","..","9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999"}){Run("health",{invalid});CHECK(e.health==100);}
 e.health=client.ps.stats[STAT_HEALTH]=83; // Damage while the open form still shows 100.
 const auto healthToken=contextToken;
 Run("health",{"250"});
 CHECK(e.health==83&&client.ps.stats[STAT_HEALTH]==83&&edits.empty()&&
       contextToken!=healthToken&&vars["jar_contextHealth"]=="83");
 Run("health",{"83"});CHECK(edits.empty());
 Run("health",{"100000"});
 CHECK(edits.size()==1&&e.health==100000&&client.ps.stats[STAT_HEALTH]==100000);
 {
  MemorySave healthSave;ojk::SavedGameHelper writer(&healthSave);
  e.sg_export(writer);client.sg_export(writer);
  healthSave.reset_buffer_offset();
  gentity_t restored{};gclient_t restoredClient{};
  restored.sg_import(writer);restoredClient.sg_import(writer);writer.ensure_all_data_read();
  CHECK(restored.health==100000&&restoredClient.ps.stats[STAT_HEALTH]==100000);
 }
 e.health=client.ps.stats[STAT_HEALTH]=92;
 Run("revert");
 CHECK(edits.empty()&&e.health==92&&client.ps.stats[STAT_HEALTH]==92&&vars["jar_contextHealth"]=="92");
 Run("health",{"150"});CHECK(edits.size()==1);
 e.health=client.ps.stats[STAT_HEALTH]=0;
 Run("revert");CHECK(edits.empty()&&e.health==0&&client.ps.stats[STAT_HEALTH]==0);
 e.health=client.ps.stats[STAT_HEALTH]=100;contextPublish();
 Run("position",{"50","60","70"});CHECK(e.currentOrigin[0]==50&&client.ps.origin[1]==60&&client.ps.gravity==800&&nativeControls.empty());
 Revert();CHECK(e.currentOrigin[0]==0&&client.ps.origin[1]==0&&client.ps.gravity==800);
 blocked=true;Run("position",{"30","40","50"});CHECK(e.currentOrigin[0]==0);blocked=false;
 Run("angles",{"10","45","-20"});CHECK(e.currentAngles[1]==45&&client.ps.viewangles[2]==-20);Revert();CHECK(client.ps.viewangles[1]==0);
 busy=true;Run("position",{"50","60","70"});CHECK(e.currentOrigin[0]==0);busy=false;
 // Frozen native ownership survives an exact pose edit and its undo.
 nativeBegin(e,true);Run("position",{"10","20","30"});CHECK(nativeControls.count(7)&&client.ps.gravity==0);Revert();CHECK(nativeControls.count(7)&&client.ps.gravity==0&&e.currentOrigin[0]==0);
 nativeRelease(7,false);CHECK(client.ps.gravity==800);
 {
  auto& rack=g_entities[160];auto& stock=g_entities[161];
  rack.inuse=stock.inuse=qtrue;rack.s.number=160;stock.s.number=161;
  rack.classname=G_NewString("misc_model_gun_rack");stock.classname=G_NewString("weapon_blaster");
  gitem_t blaster{};stock.item=&blaster;stock.s.eType=ET_ITEM;
  stock.flags=FL_FORCE_PULLABLE_ONLY;stock.s.pos.trType=TR_STATIONARY;
  CHECK(Own(state,160,96929416)&&Own(state,161,2000+WP_BLASTER));
  rack.targetname=G_NewString(va("jar_sb_%u",owned(160)->id));
  stock.targetname=G_NewString(va("jar_sb_%u",owned(161)->id));
  CHECK(SetOwnershipParent(state,*owned(161),owned(160)->id));
  const vec3_t rackStart={0,0,100},stockStart={8,3,120};
  G_SetOrigin(&rack,rackStart);G_SetOrigin(&stock,stockStart);
  const auto link=gi.linkentity,unlink=gi.unlinkentity;
  const auto trace=gi.trace;
  static bool stockBlocked=false,sweptStock=false;
  gi.linkentity=[](gentity_t* member){member->linked=qtrue;};
  gi.unlinkentity=[](gentity_t* member){member->linked=qfalse;};
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,
              int slot,int,EG2_Collision,int){
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(slot==161){
    CHECK(!g_entities[160].linked);
    sweptStock=!VectorCompare(start,end);
    if(stockBlocked)hit->fraction=.5f;
   }
  };
  gi.linkentity(&rack);gi.linkentity(&stock);
  const int previousEntityCount=globals.num_entities;globals.num_entities=162;
  selected=160;contextSelect();
  Run("position",{"50","0","100"});CHECK(rack.currentOrigin[0]==50&&stock.currentOrigin[0]==58&&!sweptStock);
  Run("angles",{"0","90","0"});
  const vec3_t turned={47,8,120};CHECK(Distance(stock.currentOrigin,turned)<.001f&&rack.linked);
  const auto editCount=edits.size();
  stockBlocked=true;Run("position",{"80","0","100"});Revert();
  CHECK(rack.currentOrigin[0]==50&&rack.currentAngles[YAW]==90&&Distance(stock.currentOrigin,turned)<.001f&&
        edits.size()==editCount&&rack.linked);
  stockBlocked=false;Revert();CHECK(rack.currentAngles[YAW]==0&&stock.currentOrigin[0]==58);
  Revert();CHECK(VectorCompare(rack.currentOrigin,rackStart)&&Distance(stock.currentOrigin,stockStart)<.001f);
  nativeBegin(rack,false);held=160;
  stockBlocked=true;nativeFrame(.05f,rackStockPose,rackStockLoose);
  CHECK(sweptStock&&rack.linked&&VectorCompare(rack.currentOrigin,rackStart)&&Distance(stock.currentOrigin,stockStart)<.001f);
  stockBlocked=false;nativeFrame(.05f,rackStockPose,rackStockLoose);
  vec3_t shelfOffset;VectorSubtract(stock.currentOrigin,rack.currentOrigin,shelfOffset);
  const vec3_t initialOffset={8,3,20};CHECK(rack.currentOrigin[0]>0&&Distance(shelfOffset,initialOffset)<.001f);
  CHECK(physgunGrip.Begin(pv(rack.currentOrigin),pv(rack.currentAngles),pv(rack.currentOrigin),0));
  vec3_t dragged;VectorCopy(stock.currentOrigin,dragged);
  physgunViewYaw=90;stockBlocked=true;physgunNativeTurn(rack,.3f);
  CHECK(rack.currentAngles[YAW]==0&&VectorCompare(stock.currentOrigin,dragged));
  stockBlocked=false;physgunNativeTurn(rack,.3f);
  VectorSubtract(stock.currentOrigin,rack.currentOrigin,shelfOffset);
  const vec3_t rotatedOffset={-3,8,20};
  CHECK(std::fabs(rack.currentAngles[YAW]-90)<.001f&&Distance(shelfOffset,rotatedOffset)<.001f);
  nativeRelease(160,false);releaseHeld();physgunViewYaw=0;
  rackStockLoose(stock);CHECK(!(stock.flags&FL_FORCE_PULLABLE_ONLY)&&owned(161)->parent==owned(160)->id);
  VectorCopy(stock.currentOrigin,dragged);Run("position",{"80","0","100"});
  CHECK(rack.currentOrigin[0]==80&&VectorCompare(stock.currentOrigin,dragged));
  forgetEdits(160);Forget(state,161);Forget(state,160);rack={};stock={};
  globals.num_entities=previousEntityCount;gi.linkentity=link;gi.unlinkentity=unlink;gi.trace=trace;
  selected=7;contextSelect();
 }
 {
  auto& hull=g_entities[48];auto& rider=g_entities[49];
  gclient_t hullClient{},riderClient{};vehicleInfo_t info{};Vehicle_t vehicle{};
  hull={};rider={};hull.inuse=rider.inuse=qtrue;hull.s.number=48;rider.s.number=49;
  hull.classname=G_NewString("NPC");rider.classname=hull.classname;
  hull.health=rider.health=100;hull.client=&hullClient;rider.client=&riderClient;
  hullClient.NPC_class=CLASS_VEHICLE;hullClient.ps.gravity=800;hull.m_pVehicle=&vehicle;
  vehicle.m_pParentEntity=&hull;vehicle.m_pVehicleInfo=&info;vehicle.m_pPilot=&rider;
  rider.owner=&hull;rider.s.m_iVehicleNum=48;hull.playerModel=0;hull.crotchBolt=4;
  const vec3_t initialRider={12,0,0};G_SetOrigin(&rider,initialRider);
  const auto link=gi.linkentity,unlink=gi.unlinkentity;
  const auto trace=gi.trace;
  const auto getBolt=gi.G2API_GetBoltMatrix;
  const auto getVector=gi.G2API_GiveMeVectorFromMatrix;
  gi.linkentity=[](gentity_t* member){member->linked=qtrue;};
  gi.unlinkentity=[](gentity_t* member){member->linked=qfalse;};
  gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int,int,mdxaBone_t* matrix,const vec3_t angles,
                           const vec3_t origin,int,qhandle_t*,const vec3_t){
   vec3_t forward,point;AngleVectors(angles,forward,nullptr,nullptr);VectorMA(origin,12,forward,point);
   for(int axis=0;axis<3;++axis)matrix->matrix[axis][3]=point[axis];
   return qtrue;
  };
  gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t& matrix,Eorientations mode,vec3_t& out){
   CHECK(mode==ORIGIN);for(int axis=0;axis<3;++axis)out[axis]=matrix.matrix[axis][3];
  };
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
   CHECK(VectorCompare(start,end));
   if(pass==49)CHECK(!g_entities[48].linked&&!g_entities[49].linked);
   *hit={};hit->fraction=1;hit->startsolid=blocked&&pass==49?qtrue:qfalse;
  };
  hull.linked=qtrue;selected=48;contextSelect();
  Run("position",{"40","0","0"});CHECK(edits.empty()&&hull.currentOrigin[0]==0);
  nativeBegin(hull,true);blocked=true;
  Run("position",{"40","0","0"});CHECK(edits.empty()&&hull.currentOrigin[0]==0&&rider.currentOrigin[0]==12);
  blocked=false;Run("position",{"40","0","0"});Run("angles",{"0","90","0"});
  CHECK(edits.size()==2&&hull.currentOrigin[0]==40&&vehicle.m_vOrientation[YAW]==90);
  CHECK(std::fabs(rider.currentOrigin[0]-40)<.001f&&rider.currentOrigin[1]==12&&rider.client->ps.origin[1]==12);
  blocked=true;Run("revert");CHECK(edits.size()==2&&vehicle.m_vOrientation[YAW]==90);
  blocked=false;Run("revert");CHECK(edits.size()==1&&rider.currentOrigin[0]==52&&vehicle.m_vOrientation[YAW]==0);
  Run("revert");CHECK(edits.empty()&&VectorCompare(rider.currentOrigin,initialRider)&&hull.currentOrigin[0]==0);
  CHECK(hull.linked&&!rider.linked&&nativeControls[48].frozen&&rider.owner==&hull&&rider.health==100);
  selected=49;contextSelect();Run("position",{"40","0","0"});CHECK(edits.empty()&&VectorCompare(rider.currentOrigin,initialRider));
  nativeRelease(48,false);hull={};rider={};gi.linkentity=link;gi.unlinkentity=unlink;gi.trace=trace;
  gi.G2API_GetBoltMatrix=getBolt;gi.G2API_GiveMeVectorFromMatrix=getVector;
  selected=7;contextSelect();
 }
 {
  auto& hull=g_entities[48];
  gclient_t hullClient{};
  vehicleInfo_t info{};
  Vehicle_t vehicle{};
  hull.inuse=hull.takedamage=qtrue;hull.s.number=48;hull.classname=G_NewString("NPC");
  hull.health=100;hull.client=&hullClient;hull.m_pVehicle=&vehicle;
  hullClient.NPC_class=CLASS_VEHICLE;
  vehicle.m_pParentEntity=&hull;vehicle.m_pVehicleInfo=&info;
  info.type=VH_SPEEDER;info.armor=100;info.shields=60;
  info.weapon[0].ammoMax=25;info.turret[0].iAmmoMax=40;
  vehicle.m_iArmor=95;vehicle.m_iShields=hullClient.ps.stats[STAT_ARMOR]=45;
  vehicle.weaponStatus[0].ammo=hullClient.ps.ammo[0]=15;
  vehicle.turretStatus[0].ammo=hullClient.ps.ammo[MAX_VEHICLE_WEAPONS]=12;
  vehicle.weaponStatus[0].lastAmmoInc=355;
  selected=48;contextSelect();const auto vehicleToken=contextToken;
  CHECK(vars["jar_contextVehicleAllowed"]=="1"&&vars["jar_contextHealthAllowed"]=="0");
  CHECK(vars["jar_contextVehicleValue0"]=="95"&&vars["jar_contextVehicleMax2"]=="25"&&vars["jar_contextVehicleMax3"]=="0");
  Run("health",{"25"});
  Run("vehicle_resources",{"0","0","7","0","18","0"});
  Run("vehicle_resources",{"76","0","26","0","18","0"});
  Run("vehicle_resources",{"76","0","7","1","18","0"});
  busy=true;Run("vehicle_resources",{"76","0","7","0","18","0"});busy=false;
  CHECK(edits.empty()&&hull.health==100&&vehicle.m_iArmor==95&&vehicle.weaponStatus[0].ammo==15);
  Run("vehicle_resources",{"76","0","7","0","18","0"});
  CHECK(edits.size()==1&&vehicle.m_iArmor==76&&hull.health==100&&vehicle.m_iShields==0&&hullClient.ps.stats[STAT_ARMOR]==0);
  CHECK(vehicle.weaponStatus[0].lastAmmoInc==355&&hullClient.ps.ammo[0]==7&&vehicle.turretStatus[0].ammo==18);
  args={"jar_sb","context","vehicle_resources",std::to_string(vehicleToken),"20","0","7","0","18","0"};contextCommand();
  CHECK(vehicle.m_iArmor==76&&edits.size()==1);
  undoEdit();CHECK(vehicle.m_iArmor==76&&edits.size()==1);
  vehicle.m_iArmor=75;Revert();CHECK(vehicle.m_iArmor==75&&edits.size()==1);
  vehicle.m_iArmor=76;Revert();CHECK(vehicle.m_iArmor==95&&hullClient.ps.ammo[0]==15&&vehicle.m_iShields==45&&edits.empty());
  {
   auto& npc=g_entities[49];gclient_t npcClient{};
   npc.inuse=qtrue;npc.health=100;npc.s.number=49;npc.client=&npcClient;
   npc.owner=&hull;npc.s.m_iVehicleNum=48;vehicle.m_pPilot=&npc;
   busy=true;Run("refresh");CHECK(vars["jar_contextVehicleAllowed"]=="1");
   Run("vehicle_resources",{"76","0","7","0","18","0"});
   CHECK(edits.size()==1&&vehicle.m_iArmor==76&&vehicle.m_pPilot==&npc&&npc.s.m_iVehicleNum==48);
   vehicle.m_iBoarding=1;Run("revert");CHECK(edits.size()==1&&vehicle.m_iArmor==76&&vars["jar_contextVehicleAllowed"]=="0");
   vehicle.m_iBoarding=0;Run("revert");CHECK(edits.empty()&&vehicle.m_iArmor==95);
   npc={};busy=false;vehicle.m_pPilot=nullptr;
  }
  {
   auto& pilot=g_entities[0];gclient_t pilotClient{};
   CHECK(!pilot.inuse);
   pilot.inuse=qtrue;pilot.health=100;pilot.s.number=0;pilot.client=&pilotClient;
   pilot.owner=&hull;pilot.s.m_iVehicleNum=48;vehicle.m_pPilot=&pilot;
   busy=true;
   in_camera=true;Run("open");CHECK(!contextToken);in_camera=false;
   cg.overrides.active=true;Run("open");CHECK(!contextToken);cg.overrides.active=false;
   on=false;Run("open");CHECK(!contextToken);on=true;
   pilot.owner=nullptr;Run("open");CHECK(!contextToken);pilot.owner=&hull;
   Run("open");const auto mountedToken=contextToken;
   CHECK(mountedToken&&contextEntity==48&&vars["jar_contextMountedVehicle"]=="1"&&vars["jar_contextVehicleAllowed"]=="1");
   Run("pick",{"320","240"});Run("player");Run("remove");Run("position",{"12","0","0"});
   CHECK(contextToken==mountedToken&&hull.inuse&&hull.currentOrigin[0]==0&&edits.empty());
   Run("vehicle_resources",{"76","12","7","0","18","0"});
   CHECK(edits.size()==1&&vehicle.m_iArmor==76&&hullClient.ps.stats[STAT_ARMOR]==12&&hullClient.ps.ammo[0]==7);
   CHECK(pilotClient.ps.ammo[0]==0&&pilot.owner==&hull&&vehicle.m_pPilot==&pilot);
   args={"jar_sb","context","vehicle_resources",std::to_string(mountedToken),"20","0","7","0","18","0"};contextCommand();
   CHECK(vehicle.m_iArmor==76&&edits.size()==1);
   Edit unrelated{};unrelated.context=true;unrelated.entity=48;unrelated.generation=entityGeneration[48];unrelated.healthEdit=true;unrelated.health=17;
   edits.push_back(unrelated);
   Run("revert");CHECK(edits.size()==1&&edits.back().healthEdit&&hull.health==100&&vehicle.m_iArmor==95);
   Run("revert");CHECK(edits.size()==1&&hull.health==100);
   edits.pop_back();
   pilot.s.m_iVehicleNum=0;vehicle.m_pPilot=nullptr;pilot.owner=nullptr;busy=false;
   Run("vehicle_resources",{"76","12","7","0","18","0"});
   CHECK(!contextToken&&vars["jar_contextMountedVehicle"]=="0"&&edits.empty()&&vehicle.m_iArmor==95);
   pilot={};selected=48;contextSelect();
  }
  info.type=VH_ANIMAL;
  Run("vehicle_resources",{"76","0","7","0","18","0"});
  CHECK(hull.health==76&&hullClient.ps.stats[STAT_HEALTH]==76&&vehicle.m_iArmor==76);
  MemorySave nativeVehicleSave;ojk::SavedGameHelper vehicleWriter(&nativeVehicleSave);
  vehicle.sg_export(vehicleWriter);nativeVehicleSave.reset_buffer_offset();
  Vehicle_t loadedVehicle{};loadedVehicle.sg_import(vehicleWriter);vehicleWriter.ensure_all_data_read();
  CHECK(loadedVehicle.m_iArmor==76&&loadedVehicle.m_iShields==0&&loadedVehicle.weaponStatus[0].ammo==7&&loadedVehicle.turretStatus[0].ammo==18);
  Revert();CHECK(hull.health==100&&hullClient.ps.stats[STAT_HEALTH]==100&&vehicle.m_iArmor==95&&edits.empty());
  hull={};selected=7;contextSelect();
 }
 {
  auto& subject=g_entities[51];auto& watcher=g_entities[52];auto& scripted=g_entities[53];
  gclient_t subjectClient{},watcherClient{},scriptedClient{};
  gNPC_t subjectInfo{},watcherInfo{},scriptedInfo{};
  subject.client=&subjectClient;subject.NPC=&subjectInfo;
  watcher.client=&watcherClient;watcher.NPC=&watcherInfo;
  scripted.client=&scriptedClient;scripted.NPC=&scriptedInfo;
  for(auto* npc:{&subject,&watcher,&scripted}){
   npc->s.number=int(npc-g_entities);npc->inuse=qtrue;npc->health=100;npc->classname=G_NewString("NPC");
  }
  CHECK(globals.num_entities>53&&Own(state,51,3001));
  subject.targetname=G_NewString(va("jar_sb_%u",owned(51)->id));
  subjectClient.playerTeam=TEAM_ENEMY;subjectClient.enemyTeam=TEAM_PLAYER;subjectClient.ps.stats[STAT_ARMOR]=40;
  watcherClient.playerTeam=scriptedClient.playerTeam=TEAM_PLAYER;
  watcherClient.enemyTeam=scriptedClient.enemyTeam=TEAM_ENEMY;
  scripted.svFlags|=SVF_LOCKEDENEMY;
  subject.enemy=subject.lastEnemy=subjectInfo.goalEntity=&watcher;subjectInfo.enemyLastSeenTime=100;
  watcher.enemy=scripted.enemy=&subject;
  AIGroupInfo_t group{};subjectInfo.group=&group;
  selected=52;contextSelect();CHECK(vars["jar_contextNPCAllowed"]=="0");
  Run("npc_status",{"1","2","50"});CHECK(edits.empty());
  selected=51;contextSelect();
  CHECK(vars["jar_contextNPCAllowed"]=="1"&&vars["jar_contextNPCTeam"]=="2"&&vars["jar_contextNPCArmor"]=="40");
  Run("npc_status",{"4","2","80"});Run("npc_status",{"1","2","32768"});Run("npc_status",{"1","2","1.5"});
  CHECK(edits.empty()&&subjectClient.playerTeam==TEAM_ENEMY&&subject.enemy==&watcher);
  subjectClient.ps.stats[STAT_ARMOR]=33;
  Run("npc_status",{"1","2","80"});CHECK(edits.empty()&&subjectClient.ps.stats[STAT_ARMOR]==33);
  Run("npc_status",{"1","2","40"});
  CHECK(edits.size()==1&&subjectClient.playerTeam==TEAM_PLAYER&&subjectClient.enemyTeam==TEAM_ENEMY&&
        subjectClient.ps.persistant[PERS_TEAM]==TEAM_PLAYER&&subjectClient.ps.stats[STAT_ARMOR]==33);
  CHECK(actorGroupsLeft==1&&!subjectInfo.group&&!subject.enemy&&!subject.lastEnemy&&!subjectInfo.goalEntity&&
        !subjectInfo.enemyLastSeenTime&&!watcher.enemy&&scripted.enemy==&subject);
  CHECK(!undoCreation()&&edits.size()==1);
  subject.svFlags|=SVF_LOCKEDENEMY;Run("refresh");CHECK(vars["jar_contextNPCTeamsAllowed"]=="0");
  Run("npc_status",{"2","2","90"});CHECK(edits.size()==1&&subjectClient.playerTeam==TEAM_PLAYER&&subjectClient.ps.stats[STAT_ARMOR]==33);
  Run("npc_status",{"1","2","90"});CHECK(edits.size()==2&&subjectClient.ps.stats[STAT_ARMOR]==90);
  MemorySave npcSave;ojk::SavedGameHelper npcWriter(&npcSave);
  subjectClient.sg_export(npcWriter);npcSave.reset_buffer_offset();
  gclient_t loadedNPC{};loadedNPC.sg_import(npcWriter);npcWriter.ensure_all_data_read();
  CHECK(loadedNPC.playerTeam==TEAM_PLAYER&&loadedNPC.enemyTeam==TEAM_ENEMY&&loadedNPC.ps.stats[STAT_ARMOR]==90);
  subjectClient.enemyTeam=TEAM_NEUTRAL;
  Run("revert");CHECK(edits.size()==1&&subjectClient.ps.stats[STAT_ARMOR]==33&&subjectClient.enemyTeam==TEAM_NEUTRAL);
  Run("revert");CHECK(edits.size()==1&&subjectClient.playerTeam==TEAM_PLAYER);
  subjectClient.enemyTeam=TEAM_ENEMY;subjectClient.ps.stats[STAT_ARMOR]=24;
  Run("revert");CHECK(edits.size()==1&&subjectClient.playerTeam==TEAM_PLAYER);
  subject.svFlags&=~SVF_LOCKEDENEMY;
  Run("revert");CHECK(edits.empty()&&subjectClient.playerTeam==TEAM_ENEMY&&subjectClient.enemyTeam==TEAM_PLAYER&&subjectClient.ps.stats[STAT_ARMOR]==24);
  CHECK(vars["jar_contextNPCFireAllowed"]=="0");
  weaponData[WP_BLASTER].ammoIndex=AMMO_BLASTER;weaponData[WP_BLASTER].altFireTime=300;
  weaponData[WP_BLASTER].energyPerShot=1;weaponData[WP_BLASTER].altEnergyPerShot=2;
  weaponData[WP_REPEATER].ammoIndex=AMMO_METAL_BOLTS;
  subject.s.weapon=subjectClient.ps.weapon=WP_BLASTER;subjectClient.ps.ammo[AMMO_BLASTER]=30;subjectClient.ps.ammo[AMMO_METAL_BOLTS]=40;
  subjectInfo.currentAmmo=30;
  Run("refresh");CHECK(vars["jar_contextNPCFireAllowed"]=="1"&&vars["jar_contextNPCAltAllowed"]=="1"&&vars["jar_contextNPCAmmo"]=="30");
  CHECK(vars["jar_contextNPCManualAmmo"]=="0");
  Run("npc_fire",{"-2","0","0"});Run("npc_fire",{"32768","0","0"});Run("npc_fire",{"1.5","0","0"});
  Run("npc_fire",{"0","2","0"});Run("npc_fire",{"0","1","2"});
  CHECK(edits.empty()&&actorFireReconfigurations==0&&subjectClient.ps.ammo[AMMO_BLASTER]==30);
  subjectClient.ps.ammo[AMMO_BLASTER]=24;
  Run("npc_fire",{"-1","1","1"});CHECK(edits.empty()&&!(subjectInfo.scriptFlags&SCF_ALT_FIRE));
  Run("npc_fire",{"30","1","0"});
  CHECK(edits.size()==1&&(subjectInfo.scriptFlags&SCF_ALT_FIRE)&&subjectInfo.currentAmmo==24&&actorFireReconfigurations==1);
  Run("npc_fire",{"-1","1","1"});CHECK(edits.size()==2&&subjectClient.ps.ammo[AMMO_BLASTER]==-1&&actorFireReconfigurations==1);
  CHECK(vars["jar_contextNPCManualAmmo"]=="1"&&G_NPCConsumeScriptShot(subject,true));
  G_NPCRefillWeaponAmmo(subject);CHECK(subjectClient.ps.ammo[AMMO_BLASTER]==AMMO_UNLIMITED);
  MemorySave fireSave;ojk::SavedGameHelper fireWriter(&fireSave);
  subjectClient.sg_export(fireWriter);subjectInfo.sg_export(fireWriter);fireSave.reset_buffer_offset();
  gclient_t restoredClient{};gNPC_t restoredInfo{};
  restoredClient.sg_import(fireWriter);restoredInfo.sg_import(fireWriter);fireWriter.ensure_all_data_read();
  CHECK(restoredClient.ps.ammo[AMMO_BLASTER]==-1&&restoredInfo.currentAmmo==-1&&(restoredInfo.scriptFlags&SCF_ALT_FIRE));
  CHECK(restoredInfo.aiFlags&NPCAI_MANUAL_AMMO);
  subjectClient.ps.weapon=WP_REPEATER;
  Run("npc_fire",{"50","1","1"});CHECK(edits.size()==2&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==40);
  Run("revert");CHECK(edits.size()==2&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==40);
  subjectClient.ps.weapon=WP_BLASTER;subjectInfo.scriptFlags&=~SCF_ALT_FIRE;
  Run("revert");CHECK(edits.size()==1&&subjectClient.ps.ammo[AMMO_BLASTER]==24&&!(subjectInfo.scriptFlags&SCF_ALT_FIRE));
  CHECK(!(subjectInfo.aiFlags&NPCAI_MANUAL_AMMO));
  Run("revert");CHECK(edits.size()==1&&actorFireReconfigurations==1);
  subjectInfo.scriptFlags|=SCF_ALT_FIRE;subjectClient.ps.ammo[AMMO_BLASTER]=21;
  Run("revert");CHECK(edits.empty()&&!(subjectInfo.scriptFlags&SCF_ALT_FIRE)&&subjectInfo.currentAmmo==21&&actorFireReconfigurations==2);
  weaponData[WP_BLASTER].altFireTime=0;
  Run("refresh");CHECK(vars["jar_contextNPCAltAllowed"]=="0");
  Run("npc_fire",{"0","1","0"});CHECK(edits.empty()&&subjectClient.ps.ammo[AMMO_BLASTER]==21);
  Run("npc_fire",{"0","0","1"});CHECK(edits.size()==1&&subjectInfo.currentAmmo==0);
  G_NPCRefillWeaponAmmo(subject);
  CHECK(!G_NPCConsumeScriptShot(subject,false)&&subjectClient.ps.ammo[AMMO_BLASTER]==0);
  Run("revert");CHECK(edits.empty()&&subjectInfo.currentAmmo==21);
  // Changing only the refill mode must preserve ammo spent by normal combat.
  Run("npc_fire",{"21","0","1"});CHECK(edits.size()==1);
  CHECK(G_NPCConsumeScriptShot(subject,true)&&subjectInfo.currentAmmo==19);
  Run("revert");CHECK(edits.empty()&&subjectClient.ps.ammo[AMMO_BLASTER]==19&&!(subjectInfo.aiFlags&NPCAI_MANUAL_AMMO));
  subjectClient.ps.ammo[AMMO_BLASTER]=0;
  G_NPCRefillWeaponAmmo(subject);CHECK(subjectClient.ps.ammo[AMMO_BLASTER]==10);
  subjectClient.ps.ammo[AMMO_BLASTER]=AMMO_UNLIMITED;
  G_NPCRefillWeaponAmmo(subject);CHECK(subjectClient.ps.ammo[AMMO_BLASTER]==AMMO_UNLIMITED);
  CHECK(BG_WeaponHasAmmo(AMMO_UNLIMITED,5)&&!BG_WeaponHasAmmo(0,1)&&!BG_WeaponHasAmmo(1,5)&&BG_WeaponHasAmmo(5,5));
  // Equip rebuilds the active hand atomically, with a finite starting grant for a new pool.
  bg_itemlist[1].giType=bg_itemlist[2].giType=IT_WEAPON;
  bg_itemlist[1].giTag=WP_BLASTER;bg_itemlist[2].giTag=WP_REPEATER;bg_itemlist[2].quantity=40;
  Q_strncpyz(weaponData[WP_BLASTER].weaponMdl,"models/weapons2/blaster_r/blaster_w.glm",MAX_QPATH);
  Q_strncpyz(weaponData[WP_REPEATER].weaponMdl,"models/weapons2/repeater/repeater_w.glm",MAX_QPATH);
  subject.ghoul2.resize(2);subject.ghoul2[0].mModelindex=0;subject.ghoul2[1].mModelindex=1;
  subject.playerModel=0;subject.weaponModel[0]=1;subject.weaponModel[1]=-1;subject.handRBolt=2;
  subjectClient.ps.stats[STAT_WEAPONS]=1<<WP_BLASTER;subjectClient.ps.ammo[AMMO_METAL_BOLTS]=0;
  subjectInfo.aiFlags|=NPCAI_MATCHPLAYERWEAPON|NPCAI_MANUAL_AMMO;
  subjectInfo.scriptFlags|=SCF_ALT_FIRE;
  subjectClient.fireDelay=200;subjectClient.ps.weaponChargeTime=100;subjectClient.ps.weaponTime=20;
  gi.G2API_RemoveGhoul2Model=[](CGhoul2Info_v& model,int index){
   CHECK(index>0&&model[index].mModelindex==index);
   model[index]=CGhoul2Info{};return qtrue;
  };
  {
   gentity_t restoredPlayer{};
   gclient_t restoredState{};
   restoredPlayer.client=&restoredState;restoredState.ps.weapon=WP_ATST_SIDE;
   restoredPlayer.playerModel=0;restoredPlayer.handRBolt=2;
   restoredPlayer.weaponModel[0]=1;restoredPlayer.weaponModel[1]=-1;
   restoredPlayer.ghoul2.resize(2);
   restoredPlayer.ghoul2[0].mModelindex=0;
   restoredPlayer.ghoul2[1].mModelindex=1;
   Q_strncpyz(restoredPlayer.ghoul2[1].mFileName,jar::carriedWeapon::placeholderPickup,MAX_QPATH);
   Q_strncpyz(weaponData[WP_ATST_SIDE].weaponMdl,jar::carriedWeapon::placeholderView,MAX_QPATH);
   const auto oldRead=gi.FS_ReadFile;
   gi.FS_ReadFile=[](const char* path,void**)->long{
    return strcmp(path,jar::carriedWeapon::Find(WP_ATST_SIDE)->art.pickup)?-1:100;
   };
   restoredState.ps.eFlags=EF_IN_ATST;
   G_RestoreCarriedWeaponHand(restoredPlayer);CHECK(restoredPlayer.weaponModel[0]==1);
   restoredState.ps.eFlags=0;
   attachedWeaponFailure=2;
   G_RestoreCarriedWeaponHand(restoredPlayer);CHECK(restoredPlayer.weaponModel[0]==1);
   attachedWeaponFailure=0;
   G_RestoreCarriedWeaponHand(restoredPlayer);CHECK(restoredPlayer.weaponModel[0]==2);
   CHECK(!strcmp(restoredPlayer.ghoul2[2].mFileName,jar::carriedWeapon::Find(WP_ATST_SIDE)->art.pickup));
   G_RestoreCarriedWeaponHand(restoredPlayer);CHECK(restoredPlayer.weaponModel[0]==2);
   gi.FS_ReadFile=oldRead;
  }
  Run("refresh");CHECK(std::stoul(vars["jar_contextNPCWeaponChoices"])&(1u<<WP_REPEATER));
  Run("npc_weapon",{"1"});Run("npc_weapon",{"29"});Run("npc_weapon",{"6.5"});
  CHECK(edits.empty()&&subject.weaponModel[0]==1&&registeredWeapons==0);
  for(int failure:{1,2}){
   attachedWeaponFailure=failure;
   Run("npc_weapon",{"6"});
   CHECK(edits.empty()&&subject.weaponModel[0]==1&&subject.ghoul2[1].mModelindex==1&&registeredWeapons==0);
   CHECK(subjectClient.ps.weapon==WP_BLASTER&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==0&&subjectClient.fireDelay==200);
  }
  attachedWeaponFailure=0;
  Run("npc_weapon",{"6"});
  CHECK(edits.size()==1&&subject.s.weapon==WP_REPEATER&&subjectClient.ps.weapon==WP_REPEATER&&subject.weaponModel[0]==2);
  CHECK(subject.ghoul2[1].mModelindex<0&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==40&&registeredWeapons==1);
  CHECK(subjectInfo.last_ucmd.weapon==WP_REPEATER&&subjectClient.ps.stats[STAT_WEAPONS]==((1<<WP_BLASTER)|(1<<WP_REPEATER)));
  CHECK((subjectInfo.aiFlags&NPCAI_MANUAL_AMMO)&&!(subjectInfo.aiFlags&NPCAI_MATCHPLAYERWEAPON)&&!(subjectInfo.scriptFlags&SCF_ALT_FIRE));
  CHECK(!subjectClient.fireDelay&&!subjectClient.ps.weaponChargeTime&&!subjectClient.ps.weaponTime);
  MemorySave weaponSave;ojk::SavedGameHelper weaponWriter(&weaponSave);
  subjectClient.sg_export(weaponWriter);subjectInfo.sg_export(weaponWriter);weaponSave.reset_buffer_offset();
  restoredClient.sg_import(weaponWriter);restoredInfo.sg_import(weaponWriter);weaponWriter.ensure_all_data_read();
  CHECK(restoredClient.ps.weapon==WP_REPEATER&&restoredInfo.currentAmmo==40&&restoredInfo.last_ucmd.weapon==WP_REPEATER);
  attachedWeaponFailure=2;
  Run("revert");CHECK(edits.size()==1&&subject.weaponModel[0]==2&&subject.ghoul2[2].mModelindex==2);
  attachedWeaponFailure=0;
  Run("revert");CHECK(edits.empty()&&subjectClient.ps.weapon==WP_BLASTER&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==0);
  CHECK((subjectInfo.aiFlags&NPCAI_MATCHPLAYERWEAPON)&&(subjectInfo.scriptFlags&SCF_ALT_FIRE));
  Run("npc_weapon",{"6"});subjectClient.ps.ammo[AMMO_METAL_BOLTS]=31;
  Run("revert");CHECK(edits.empty()&&subjectClient.ps.weapon==WP_BLASTER&&subjectClient.ps.ammo[AMMO_METAL_BOLTS]==31);
  CHECK(subjectClient.ps.stats[STAT_WEAPONS]==1<<WP_BLASTER);
  subject.s.weapon=subjectClient.ps.weapon=WP_NONE;
  Run("npc_weapon",{"6"});CHECK(edits.empty());
  subject.s.weapon=subjectClient.ps.weapon=WP_BLASTER;
  Run("npc_weapon",{"0"});CHECK(edits.size()==1&&subject.s.weapon==WP_NONE&&subject.weaponModel[0]==-1);
  Run("revert");CHECK(edits.empty()&&subject.s.weapon==WP_BLASTER&&subject.weaponModel[0]>0);
  subject.weaponModel[1]=3;Run("refresh");CHECK(vars["jar_contextNPCWeaponChoices"]=="0");
  subject.weaponModel[1]=-1;
  auto& player=g_entities[0];gclient_t playerClient{};
  player.inuse=qtrue;player.client=&playerClient;player.health=100;playerClient.playerTeam=TEAM_PLAYER;
  subjectClient.leader=&watcher;
  subjectInfo.eFlags|=4;
  subjectInfo.last_ucmd.forwardmove=80;subjectInfo.last_ucmd.buttons=BUTTON_ATTACK;
  subjectInfo.attackHoldTime=900;subjectClient.fireDelay=350;subjectClient.ps.weaponstate=WEAPON_CHARGING;
  Run("refresh");CHECK(vars["jar_contextNPCOrderAllowed"]=="1"&&vars["jar_contextNPCOrderMode"]=="0");
  Run("npc_order",{"3","110"});Run("npc_order",{"2","110"});CHECK(edits.empty());
  Run("npc_order",{"1","110"});
  CHECK(edits.size()==1&&vars["jar_contextNPCOrderMode"]=="1"&&(subjectInfo.eFlags&4));
  CHECK(!subjectInfo.last_ucmd.forwardmove&&!subjectInfo.last_ucmd.buttons&&!subjectInfo.attackHoldTime);
  CHECK(!subjectClient.fireDelay&&subjectClient.ps.weaponstate==WEAPON_READY);
  auto& goal=g_entities[54];
  int waits=0,follows=0,goes=0;
  const auto wait=[&]{++waits;CHECK(subjectClient.leader==&watcher);};
  const auto follow=[&]{++follows;CHECK(subjectClient.leader==&player);};
  const auto go=[&](gentity_t& marker){++goes;CHECK(&marker==&goal&&marker.radius==42);return true;};
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&waits==1&&!follows);
  npcOrderScripted=true;
  CHECK(!G_WorkshopRunNPCOrder(subject,player,wait,follow,go));
  Run("revert");CHECK(edits.size()==1);
  npcOrderScripted=false;
  Run("revert");CHECK(edits.empty()&&vars["jar_contextNPCOrderMode"]=="0");
  CHECK(!G_WorkshopRunNPCOrder(subject,player,wait,follow,go));
  subjectClient.playerTeam=TEAM_PLAYER;
  Run("refresh");Run("npc_order",{"2","71"});Run("npc_order",{"2","1025"});CHECK(edits.empty());
  Run("npc_order",{"2","190.5"});
  CHECK(edits.size()==1&&vars["jar_contextNPCOrderMode"]=="2"&&subjectInfo.followDist==190.5f);
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&waits==1&&follows==1&&subjectClient.leader==&watcher);
  player.health=0;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&waits==2&&follows==1);
  player.health=100;subject.s.m_iVehicleNum=40;
  CHECK(!G_WorkshopRunNPCOrder(subject,player,wait,follow,go));
  subject.s.m_iVehicleNum=0;
  subjectInfo.tempBehavior=BS_CINEMATIC;
  CHECK(!G_WorkshopRunNPCOrder(subject,player,wait,follow,go));
  Run("refresh");CHECK(vars["jar_contextNPCOrderAllowed"]=="0");
  subjectInfo.tempBehavior=BS_DEFAULT;
  Run("refresh");CHECK(vars["jar_contextNPCOrderAllowed"]=="1");
  MemorySave orderSave;ojk::SavedGameHelper orderWriter(&orderSave);
  subjectInfo.sg_export(orderWriter);orderSave.reset_buffer_offset();restoredInfo.sg_import(orderWriter);
  orderWriter.ensure_all_data_read();
  const auto restoredOrder=G_WorkshopReadNPCOrder(restoredInfo);
  CHECK(restoredOrder.mode==jar::npcOrder::Follow&&restoredOrder.distance==190.5f&&(restoredInfo.eFlags&4));
  subjectInfo.followDist=215;
  Run("revert");CHECK(edits.size()==1&&subjectInfo.followDist==215);
  subjectInfo.followDist=190.5f;
  Run("revert");CHECK(edits.empty()&&vars["jar_contextNPCOrderMode"]=="0"&&subjectInfo.followDist==0);
  Run("npc_order",{"3","110"});CHECK(edits.empty());
  goal.inuse=qtrue;goal.s.number=54;goal.classname=G_NewString("waypoint_navgoal");
  CHECK(Own(state,54,90750275u));
  const jar::npcOrder::Destination goalLink{owned(54)->id,54};
  goal.targetname=G_NewString(va("jar_sb_%u",goalLink.owner));CHECK(G_ReferenceSpawn(goal));goal.radius=42;
  editContextActorGoal(subject,goalLink);
  CHECK(edits.size()==1&&edits.back().context&&jar::npcOrder::Same(G_WorkshopReadNPCGoal(subjectInfo),goalLink));
  Run("npc_order",{"3","110"});CHECK(edits.size()==1); // The old Context token saw no destination.
  Run("refresh");Run("npc_order",{"3","110"});
  CHECK(edits.size()==2&&G_WorkshopReadNPCOrder(subjectInfo).mode==jar::npcOrder::Go);
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&goes==1);
  goal.svFlags|=SVF_INACTIVE;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&goes==1&&waits==3);
  goal.svFlags&=~SVF_INACTIVE;auto* originalName=goal.targetname;
  goal.targetname=G_NewString("jar_sb_999999");
  CHECK(!G_WorkshopResolveNPCGoal(goalLink)&&G_WorkshopRunNPCOrder(subject,player,wait,follow,go)&&waits==4);
  goal.targetname=originalName;
  MemorySave goalSave;ojk::SavedGameHelper goalWriter(&goalSave);
  goalLink.sg_export(goalWriter);goalSave.reset_buffer_offset();
  jar::npcOrder::Destination restoredGoal;restoredGoal.sg_import(goalWriter);goalWriter.ensure_all_data_read();
  CHECK(jar::npcOrder::Same(restoredGoal,goalLink)&&jar::npcOrder::ValidDestination(restoredGoal,ENTITYNUM_WORLD));
  CHECK(!jar::npcOrder::ValidDestination({0,54},ENTITYNUM_WORLD));
  CHECK(!jar::npcOrder::ValidDestination({goalLink.owner,ENTITYNUM_WORLD},ENTITYNUM_WORLD));
  Run("npc_goal_clear");
  CHECK(edits.size()==3&&vars["jar_contextNPCGoalLinked"]=="0"&&!G_WorkshopReadNPCGoal(subjectInfo).owner);
  Run("revert");CHECK(edits.size()==2&&jar::npcOrder::Same(G_WorkshopReadNPCGoal(subjectInfo),goalLink));
  Run("revert");Run("revert");CHECK(edits.empty()&&!G_WorkshopReadNPCGoal(subjectInfo).owner);
  G_ReferenceForget(&goal);Forget(state,54);goal={};
  auto& firstPoint=g_entities[92];auto& lastPoint=g_entities[93];
  const auto waypoint=[&](gentity_t& point,int slot){
   point.inuse=qtrue;point.s.number=slot;point.classname=G_NewString("waypoint");
   const auto* card=std::find_if(std::begin(jar::catalog::entries),std::end(jar::catalog::entries),
    [](const jar::catalog::Entry& entry){return !strcmp(entry.name,"waypoint");});
   CHECK(card!=std::end(jar::catalog::entries)&&Own(state,slot,card->id));
   const auto id=owned(slot)->id;
   point.targetname=G_NewString(va("jar_sb_%u",id));CHECK(G_WaypointSpawn(point));
   return jar::npcOrder::Destination{id,unsigned(slot)};
  };
  const auto firstWaypoint=waypoint(firstPoint,92),lastWaypoint=waypoint(lastPoint,93);
  firstPoint.target=lastPoint.targetname;
  editContextActorGoal(subject,firstWaypoint);
  Run("refresh");Run("npc_order",{"4","110"});
  CHECK(edits.size()==2&&G_WorkshopReadNPCOrder(subjectInfo).mode==jar::npcOrder::Patrol);
  bool reached=false;
  gentity_t* movingTo=nullptr;
  const auto patrol=[&](gentity_t& point){movingTo=&point;return reached;};
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,patrol)&&movingTo==&firstPoint);
  CHECK(!G_WorkshopReadNPCPatrol(subjectInfo).owner);
  reached=true;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,patrol)&&movingTo==&firstPoint);
  CHECK(jar::npcOrder::Same(G_WorkshopReadNPCPatrol(subjectInfo),lastWaypoint));
  lastPoint.svFlags|=SVF_INACTIVE;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,patrol)&&waits==5&&movingTo==&firstPoint);
  lastPoint.svFlags&=~SVF_INACTIVE;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,patrol)&&movingTo==&lastPoint);
  CHECK(jar::npcOrder::Same(G_WorkshopReadNPCPatrol(subjectInfo),lastWaypoint));
  MemorySave patrolSave;ojk::SavedGameHelper patrolWriter(&patrolSave);
  G_WorkshopReadNPCPatrol(subjectInfo).sg_export(patrolWriter);patrolSave.reset_buffer_offset();
  jar::npcOrder::Destination restoredPatrol;restoredPatrol.sg_import(patrolWriter);patrolWriter.ensure_all_data_read();
  CHECK(jar::npcOrder::Same(restoredPatrol,lastWaypoint));
  Run("npc_order",{"1","110"});CHECK(!G_WorkshopReadNPCPatrol(subjectInfo).owner);
  Run("revert");CHECK(jar::npcOrder::Same(G_WorkshopReadNPCPatrol(subjectInfo),lastWaypoint));
  Run("npc_goal_clear");CHECK(!G_WorkshopReadNPCPatrol(subjectInfo).owner);
  Run("revert");CHECK(jar::npcOrder::Same(G_WorkshopReadNPCPatrol(subjectInfo),lastWaypoint));
  lastPoint.target=firstPoint.targetname;
  CHECK(G_WorkshopRunNPCOrder(subject,player,wait,follow,patrol)&&movingTo==&lastPoint);
  CHECK(jar::npcOrder::Same(G_WorkshopReadNPCPatrol(subjectInfo),firstWaypoint));
  Run("revert");Run("revert");CHECK(edits.empty()&&!G_WorkshopReadNPCPatrol(subjectInfo).owner);
  Forget(state,92);Forget(state,93);firstPoint={};lastPoint={};
  player={};
  subjectClient.NPC_class=CLASS_RANCOR;Run("refresh");CHECK(vars["jar_contextNPCFireAllowed"]=="0");
  CHECK(vars["jar_contextNPCWeaponChoices"]=="0");
  CHECK(vars["jar_contextNPCOrderAllowed"]=="0");
  subject.ghoul2.clear();
  Forget(state,51);subject={};watcher={};scripted={};selected=7;contextSelect();
 }
 {
  auto& shooter=g_entities[48];gclient_t shooterClient{};
  shooter.inuse=qtrue;shooter.s.number=48;shooter.classname=G_NewString("shooter_grenade");
  shooter.targetname=G_NewString("jar_sb_48");shooter.client=&shooterClient;
  shooter.e_UseFunc=useF_misc_weapon_shooter_use;shooter.wait=500;
  selected=48;contextSelect();CHECK(vars["jar_contextShooterAllowed"]=="1");
  Run("shooter_interval",{"99","0"});CHECK(edits.empty()&&shooter.wait==500);
  Run("shooter_interval",{"250","80"});Run("shooter_repeat");Run("shooter_alternate");
  CHECK(shooter.wait==250&&shooter.random==80&&(shooter.spawnflags&3)==3);
  G_SandboxShooterSchedule(shooter,level.time,200);
  Run("shooter_enabled");CHECK((shooter.svFlags&SVF_INACTIVE)&&!shooter.nextthink);
  Revert();CHECK(!(shooter.svFlags&SVF_INACTIVE)&&G_SandboxShooterPending(shooter,level.time)==200);
  G_LogicSetUseEnabled(shooter,false);CHECK(!shooter.nextthink);
  G_LogicSetUseEnabled(shooter,true);CHECK(!shooter.nextthink);
  Revert();Revert();Revert();CHECK(edits.empty()&&shooter.wait==500&&!(shooter.spawnflags&3));
  shooter={};selected=7;contextSelect();
 }
 {
  auto& secret=g_entities[48];secret.inuse=qtrue;secret.s.number=48;
  secret.classname=G_NewString("target_secret");secret.e_UseFunc=useF_target_secret_use;
  CHECK(Own(state,48,87785505));secret.targetname=G_NewString(va("jar_sb_%u",owned(48)->id));
  secret.spawnflags=JAR_SECRET_FOUND;selected=48;contextSelect();
  CHECK(vars["jar_contextSecretAllowed"]=="1"&&vars["jar_contextSecretFound"]=="1");
  Run("secret_rearm");CHECK(!secret.spawnflags&&vars["jar_contextSecretFound"]=="0");
  secret.spawnflags|=JAR_SECRET_FOUND;Revert();CHECK(edits.size()==1);
  secret.spawnflags=0;Revert();CHECK((secret.spawnflags&JAR_SECRET_FOUND)&&edits.empty());
  Run("secret_repeat");CHECK(secret.spawnflags==JAR_SECRET_REPEAT);
  Run("secret_enabled");CHECK(secret.svFlags&SVF_INACTIVE);
  MemorySave memory;ojk::SavedGameHelper save(&memory);secret.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  CHECK((restored.svFlags&SVF_INACTIVE)&&restored.spawnflags==JAR_SECRET_REPEAT&&restored.e_UseFunc==useF_target_secret_use);
  Revert();Revert();CHECK(!(secret.svFlags&SVF_INACTIVE)&&secret.spawnflags==JAR_SECRET_FOUND);
  Forget(state,48);contextSelect();CHECK(vars["jar_contextSecretAllowed"]=="0");
  Run("secret_rearm");CHECK(secret.spawnflags==JAR_SECRET_FOUND);
  secret={};selected=7;contextSelect();
 }
 {
  auto& replicator=g_entities[48];replicator.inuse=qtrue;replicator.s.number=48;
  replicator.classname=G_NewString("misc_replicator_item");replicator.e_UseFunc=useF_misc_replicator_item_spawn;
  CHECK(Own(state,48,96626319));replicator.targetname=G_NewString(va("jar_sb_%u",owned(48)->id));
  selected=48;contextSelect();CHECK(vars["jar_contextReplicatorAllowed"]=="1"&&vars["jar_contextReplicatorDelay"]=="4000");
  Run("replicator_apply",{"101","800"});CHECK(edits.empty());
  Run("replicator_apply",{"0","800"});CHECK(G_ReplicatorHeal(replicator)==0&&replicator.wait==800);
  Run("replicator_enabled");CHECK(replicator.svFlags&SVF_INACTIVE);
  Revert();Revert();CHECK(edits.empty()&&G_ReplicatorHeal(replicator)==5&&replicator.wait==4000);
  Forget(state,48);replicator={};selected=7;contextSelect();
 }
 {
  auto& pickup=g_entities[48];gitem_t item{},replacement{};
  pickup.inuse=qtrue;pickup.s.number=48;pickup.s.eType=ET_ITEM;
  pickup.classname=G_NewString("item_shield_sm_instant");pickup.item=&item;
  pickup.flags=FL_DROPPED_ITEM;pickup.count=25;item.giType=IT_ARMOR;item.quantity=25;
  pickup.e_ThinkFunc=thinkF_G_FreeEntity;pickup.nextthink=level.time+30000;
  selected=48;contextSelect();CHECK(vars["jar_contextPickupQuantityAllowed"]=="0");
  CHECK(Own(state,48,2501));pickup.targetname=G_NewString(va("jar_sb_%u",owned(48)->id));
  contextSelect();CHECK(vars["jar_contextPickupQuantityAllowed"]=="1");
  CHECK(vars["jar_contextPickupLifetimeAllowed"]=="1");
  Run("pickup_lifetime",{"600001"});CHECK(edits.empty());
  Run("pickup_lifetime",{"0"});CHECK(pickup.nextthink==jar::pickupLifetime::keepDeadline);
  Run("pickup_lifetime",{"1500"});CHECK(pickup.nextthink==level.time+1500);
  level.time+=100;Revert();CHECK(pickup.nextthink==jar::pickupLifetime::keepDeadline);
  level.time-=100;Revert();CHECK(pickup.nextthink==level.time+30000&&edits.empty());
  for(const char* bad:{"1001","-1","1.5","nan","1;quit"})Run("pickup_quantity",{bad});
  CHECK(edits.empty()&&pickup.count==25);
  Run("pickup_quantity",{"48"});CHECK(pickup.count==48&&G_WorkshopArmorPickupQuantity(pickup)==48);
  pickup.flags=0;CHECK(G_WorkshopArmorPickupQuantity(pickup)==25);pickup.flags=FL_DROPPED_ITEM;
  Run("pickup_quantity",{"0"});CHECK(!pickup.count&&G_WorkshopArmorPickupQuantity(pickup)==25);
  replacement=item;pickup.item=&replacement;Revert();CHECK(pickup.count==0&&edits.size()==2);
  pickup.item=&item;Revert();Revert();CHECK(pickup.count==25&&edits.empty());
  item.giType=IT_WEAPON;item.giTag=WP_BLASTER;weaponData[WP_BLASTER].ammoIndex=AMMO_BLASTER;
  contextSelect();CHECK(vars["jar_contextPickupWeapon"]=="1");
  Run("pickup_quantity",{"0"});CHECK(!pickup.count);Revert();CHECK(pickup.count==25);
  item.giTag=WP_SABER;contextSelect();CHECK(vars["jar_contextPickupQuantityAllowed"]=="0");
  Forget(state,48);CHECK(Own(state,48,78298821));
  pickup.classname=G_NewString("item_force_boon");pickup.targetname=G_NewString(va("jar_sb_%u",owned(48)->id));
  item.giType=IT_POWERUP;item.giTag=PW_FORCE_BOON;pickup.count=0;
  contextSelect();CHECK(vars["jar_contextPickupDuration"]=="1"&&vars["jar_contextPickupWeapon"]=="0");
  Run("pickup_quantity",{"1001"});CHECK(!pickup.count&&edits.empty());
  Run("pickup_quantity",{"60"});CHECK(pickup.count==60&&edits.size()==1);
  Revert();CHECK(!pickup.count&&edits.empty());
  item.giTag=PW_QUAD;contextSelect();CHECK(vars["jar_contextPickupDuration"]=="0"&&vars["jar_contextPickupQuantityAllowed"]=="0");
  Forget(state,48);pickup={};selected=7;contextSelect();
 }
 {
  auto& shield=g_entities[48];const int previousTime=level.time;
  shield.inuse=qtrue;shield.s.number=48;shield.classname=G_NewString("jar_portable_shield");
  shield.model=G_NewString("models/map_objects/mp/shield.md3");shield.e_ThinkFunc=thinkF_portable_shield_think;
  shield.e_TouchFunc=touchF_portable_shield_touch;shield.health=140;shield.max_health=250;
  shield.delay=level.time+2000;shield.attackDebounceTime=level.time+120;shield.contents=CONTENTS_ITEM|CONTENTS_TRIGGER;
  VectorSet(shield.pos1,-4,-30,0);VectorSet(shield.pos2,4,50,144);
  VectorSet(shield.currentAngles,0,90,0);CHECK(G_PortableShieldMoved(shield));
  selected=48;contextSelect();CHECK(vars["jar_contextDeployedShieldAllowed"]=="1");
  CHECK(vars["jar_contextDeployedShieldStatus"]=="Lowered | 140 health | 2.0 s left");
  for(const char* bad:{"0","1.5","100001","nan","1;quit"})Run("deployed_shield",{bad,"0","30","50","144"});
  CHECK(edits.empty()&&shield.health==140);
  const int previousExpiry=shield.delay;
  blocked=true;Run("deployed_shield",{"190","9000","40.5","60","156"});
  CHECK(edits.empty()&&shield.health==140&&shield.delay==previousExpiry&&shield.pos1[1]==-30);
  blocked=false;Run("deployed_shield",{"190","9000","40.5","60","156"});
  const int appliedExpiry=shield.delay;
  CHECK(edits.size()==1&&shield.health==190&&appliedExpiry==level.time+9000);
  CHECK(shield.pos1[1]==-40.5f&&shield.pos2[1]==60&&shield.maxs[2]==156&&!shield.count&&!shield.takedamage);
  CHECK(shield.attackDebounceTime==previousTime+120);
  MemorySave memory;ojk::SavedGameHelper save(&memory);shield.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  CHECK(restored.pos1[1]==-40.5f&&restored.pos2[2]==156&&restored.delay==appliedExpiry&&restored.health==190);
  level.time+=100;shield.health=180;Revert();CHECK(edits.size()==1&&shield.health==180);
  shield.health=190;blocked=true;Revert();CHECK(edits.size()==1&&shield.pos2[2]==156);
  blocked=false;Revert();CHECK(edits.empty()&&shield.delay==previousExpiry&&shield.health==140&&shield.pos2[2]==144);
  Run("deployed_shield",{"160","0","30","50","144"});CHECK(shield.delay==previousExpiry&&shield.health==160);
  Revert();CHECK(edits.empty());
  level.time=previousExpiry;contextSelect();CHECK(vars["jar_contextDeployedShieldAllowed"]=="0");
  Run("deployed_shield",{"180","9000","30","50","144"});CHECK(edits.empty()&&shield.health==140);
  shield={};level.time=previousTime;selected=7;contextSelect();
 }
 {
  const int oldTime=level.time;
  auto& mount=g_entities[46];auto& charge=g_entities[48];
  mount.inuse=qtrue;mount.s.number=46;mount.s.eType=ET_GENERAL;mount.classname=G_NewString("jar_sandbox_prop");
  CHECK(Own(state,46,1001));
  charge.inuse=qtrue;charge.s.number=48;charge.s.eType=ET_GENERAL;charge.s.weapon=WP_TRIP_MINE;
  charge.classname=G_NewString("tripmine");charge.s.eFlags=EF_MISSILE_STICK|EF_PROX_TRIP;
  charge.e_DieFunc=dieF_WP_ExplosiveDie;charge.e_ThinkFunc=thinkF_WP_prox_mine_think;
  charge.activator=&g_entities[0];charge.takedamage=qtrue;charge.health=15;
  charge.splashDamage=75;charge.splashRadius=128;charge.delay=level.time+40000;charge.nextthink=level.time+500;
  G_SandboxChargeAttach(charge,mount);CHECK(G_SandboxChargeSupport(charge)==&mount);
  selected=48;contextSelect();
  CHECK(vars["jar_contextMountedChargeEditable"]=="1"&&vars["jar_contextMountedChargeProximity"]=="1");
  CHECK(vars["jar_contextMountedChargeStatus"]=="Proximity armed | 40.0 s left");
  for(const char* bad:{"-1","4097","1.5","nan","1;quit"})Run("mounted_charge",{"25","100",bad,"0"});
  Run("health",{"50"});CHECK(edits.empty()&&charge.health==15);
  Run("mounted_charge",{"25","100","320","60000"});
  CHECK(edits.size()==1&&charge.health==25&&charge.splashDamage==100&&charge.splashRadius==320);
  CHECK(charge.nextthink==oldTime+500&&charge.delay==oldTime+60000&&(charge.s.eFlags&EF_PROX_TRIP));
  std::vector<jar::chargeCopy::State> copied;
  CHECK(G_SandboxChargesCapture(mount,copied)&&copied.size()==1&&copied[0].timeout==60000&&copied[0].splash==100);
  MemorySave memory;ojk::SavedGameHelper save(&memory);charge.sg_export(save);memory.reset_buffer_offset();
  gentity_t restored{};restored.sg_import(save);save.ensure_all_data_read();
  CHECK(restored.splashDamage==100&&restored.splashRadius==320&&restored.delay==charge.delay&&restored.nextthink==charge.nextthink);
  ++level.time;charge.health=24;Revert();CHECK(edits.size()==1&&charge.health==24);
  charge.health=25;++entityGeneration[46];Revert();CHECK(edits.size()==1&&charge.health==25);
  --entityGeneration[46];Revert();CHECK(edits.empty()&&charge.health==15&&charge.delay==oldTime+40000);
  Run("mounted_charge",{"20","0","0","0"});CHECK(charge.delay==oldTime+40000);
  charge.e_ThinkFunc=thinkF_WP_Explode;contextChargeStatus(charge);
  CHECK(vars["jar_contextMountedChargeAllowed"]=="1"&&vars["jar_contextMountedChargeEditable"]=="0");
  Run("mounted_charge",{"30","120","256","0"});Revert();CHECK(edits.size()==1&&charge.health==20);
  charge.activator=&g_entities[7];contextSelect();CHECK(vars["jar_contextMountedChargeAllowed"]=="0");
  forgetEdits(48);charge.activator=&g_entities[0];
  charge.e_ThinkFunc=thinkF_laserTrapThink;charge.s.eFlags=EF_MISSILE_STICK|EF_FIRING;charge.s.loopSound=10;
  contextSelect();CHECK(vars["jar_contextMountedChargeStatus"]=="Tripwire armed");
  Run("mounted_charge",{"50","95","128","1000"});CHECK(edits.empty()&&charge.health==20);
  Run("mounted_charge",{"50","95","128","0"});CHECK(charge.health==50&&charge.s.loopSound==10&&(charge.s.eFlags&EF_FIRING));
  Revert();CHECK(edits.empty()&&charge.health==20);
  charge.s.weapon=WP_DET_PACK;charge.classname=G_NewString("detpack");charge.e_ThinkFunc=thinkF_NULL;charge.nextthink=-1;
  contextSelect();CHECK(vars["jar_contextMountedChargeStatus"]=="Remote det pack ready");
  Run("mounted_charge",{"30","150","200","0"});CHECK(charge.nextthink==-1&&charge.splashDamage==150);
  Revert();CHECK(edits.empty()&&charge.splashDamage==0);
  G_SandboxChargesForget(mount);
  CHECK(!G_SandboxChargeSupport(charge));
  // Both current and older native world charges can use the same tab.
  for(int supportSlot:{ENTITYNUM_WORLD,0,ENTITYNUM_NONE}){
   charge.s.groundEntityNum=supportSlot;
   contextSelect();CHECK(vars["jar_contextMountedChargeEditable"]=="1"&&vars["jar_contextHealthAllowed"]=="0");
   CHECK(vars["jar_contextInfo"]=="Player-planted | Released");
   Run("health",{"50"});CHECK(edits.empty()&&charge.health==20);
   Run("mounted_charge",{"32","151","210","0"});
   CHECK(edits.size()==1&&charge.s.groundEntityNum==supportSlot&&charge.health==32&&charge.splashDamage==151);
   Revert();CHECK(edits.empty()&&charge.health==20&&charge.splashDamage==0);
  }
  charge.s.groundEntityNum=ENTITYNUM_WORLD;
  contextSelect();Run("mounted_charge",{"32","151","210","0"});
  G_SandboxChargeAttach(charge,mount);Revert();CHECK(edits.size()==1&&charge.health==32);
  G_SandboxChargesForget(mount);Revert();CHECK(edits.empty()&&charge.health==20);
  charge.s.groundEntityNum=46;contextSelect();CHECK(vars["jar_contextMountedChargeAllowed"]=="0");
  Run("mounted_charge",{"32","151","210","0"});CHECK(edits.empty()&&charge.health==20);
  G_SandboxChargesForget(charge);Forget(state,46);charge={};mount={};
  level.time=oldTime;selected=7;contextSelect();
 }
 mapGravity.value=800;client.ps.friction=6;
 contextSelect();CHECK(vars["jar_contextMovementKind"]=="1"&&vars["jar_contextMovementGravity"]=="800");
 const auto movementToken=contextToken;
 for(const char* bad:{"-1","3201","0.5","nan","1;quit"}){Run("movement_gravity",{bad});CHECK(edits.empty());}
 Run("movement_gravity",{"350"});CHECK(client.ps.gravity==350&&(e.svFlags&SVF_CUSTOM_GRAVITY));
 args={"jar_sb","context","movement_gravity",std::to_string(movementToken),"100"};contextCommand();
 CHECK(client.ps.gravity==350&&edits.size()==1);
 Run("movement_friction",{"11"});CHECK(client.ps.friction==6);
 Run("movement_friction",{"3"});CHECK(client.ps.friction==3&&edits.size()==2);
 client.ps.friction=4;Revert();CHECK(client.ps.friction==4&&edits.size()==2);
 client.ps.friction=3;Revert();CHECK(client.ps.friction==6&&edits.size()==1);
 mapGravity.value=720;spaceMovement=true;
 Run("movement_map");CHECK(!(e.svFlags&SVF_CUSTOM_GRAVITY)&&client.ps.gravity==0);
 Revert();CHECK((e.svFlags&SVF_CUSTOM_GRAVITY)&&client.ps.gravity==350);
 spaceMovement=false;Revert();CHECK(!(e.svFlags&SVF_CUSTOM_GRAVITY)&&client.ps.gravity==720&&edits.empty());
 Vehicle_t boarded{};e.m_pVehicle=&boarded;
 contextSelect();CHECK(vars["jar_contextMovementKind"]=="0");
 Run("movement_gravity",{"200"});CHECK(client.ps.gravity==720&&edits.empty());
 e.m_pVehicle=nullptr;
 client.ps.gravity=800;mapGravity.value=800;
 auto& gravityMarker=g_entities[68];gravityMarker.inuse=qtrue;gravityMarker.s.number=68;
 gravityMarker.classname=G_NewString("target_gravity_change");gravityMarker.model=G_NewString(JAR_MOVEMENT_PORTABLE);
 CHECK(Own(state,68,jar::movement::gravityId));
 CHECK(G_MovementMarkerSet(gravityMarker,{jar::movement::Gravity,400,true}));
 selected=68;contextSelect();CHECK(vars["jar_contextMovementKind"]=="2"&&vars["jar_contextMovementGravity"]=="400");
 Run("movement_friction",{"1"});CHECK(edits.empty()&&gravityMarker.speed==400);
 Run("movement_gravity",{"150"});CHECK(gravityMarker.speed==150&&client.ps.gravity==800);
 Run("movement_enabled");CHECK(gravityMarker.svFlags&SVF_INACTIVE);
 CHECK(!G_MovementMarkerActivate(gravityMarker,&e)&&client.ps.gravity==800);
 Revert();CHECK(!(gravityMarker.svFlags&SVF_INACTIVE));
 CHECK(G_MovementMarkerActivate(gravityMarker,&e)&&client.ps.gravity==150);
 CHECK(!G_MovementMarkerActivate(gravityMarker,nullptr));
 MemorySave actorSave;ojk::SavedGameHelper actorWriter(&actorSave);
 client.ps.sg_export(actorWriter);actorSave.reset_buffer_offset();
 playerState_t loadedMovement{};loadedMovement.sg_import(actorWriter);actorWriter.ensure_all_data_read();
 CHECK(loadedMovement.gravity==150&&loadedMovement.friction==6);
 MemorySave markerSave;ojk::SavedGameHelper markerWriter(&markerSave);
 gravityMarker.sg_export(markerWriter);markerSave.reset_buffer_offset();
 gentity_t loadedMarker{};loadedMarker.sg_import(markerWriter);markerWriter.ensure_all_data_read();
 loadedMarker.classname=gravityMarker.classname;loadedMarker.model=gravityMarker.model;
 jar::movement::Marker markerState;
 CHECK(G_MovementMarkerRead(loadedMarker,markerState)&&markerState.value==150&&markerState.enabled);
 loadedMarker.model=G_NewString("models/map_objects/imperial/crate1.md3");
 loadedMarker.svFlags|=SVF_INACTIVE;G_MovementMarkerRestore(loadedMarker);
 CHECK(G_MovementMarkerRead(loadedMarker,markerState)&&markerState.value==150&&!markerState.enabled);
 Revert();CHECK(gravityMarker.speed==400&&edits.empty());
 Forget(state,68);gravityMarker.inuse=qfalse;
 e.svFlags&=~SVF_CUSTOM_GRAVITY;client.ps.gravity=800;
 auto& timer=g_entities[69];timer.inuse=qtrue;timer.s.number=69;timer.classname=G_NewString("target_delay");
 timer.targetname=G_NewString("jar_sb_400");timer.e_UseFunc=useF_Use_Target_Delay;
 timer.wait=1;timer.nextthink=2000;timer.e_ThinkFunc=thinkF_Think_Target_Delay;
 selected=69;contextSelect();CHECK(vars["jar_contextLogicKind"]=="0");
 Run("logic_apply",{"1200","200"});CHECK(edits.empty()&&timer.wait==1);
 CHECK(Own(state,69,jar::logic::delayId));contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="1"&&vars["jar_contextLogicPending"]=="1");
 const auto delayToken=contextToken;
 Run("logic_apply",{"500","501"});Run("logic_apply",{"59000","1001"});
 CHECK(edits.empty()&&timer.wait==1);
 Run("logic_apply",{"1250","250"});CHECK(timer.wait==1.25f&&timer.random==.25f&&timer.nextthink==2000);
 args={"jar_sb","context","logic_apply",std::to_string(delayToken),"0","0"};contextCommand();
 CHECK(timer.wait==1.25f&&edits.size()==1);
 Revert();CHECK(timer.wait==1&&!timer.random&&timer.nextthink==2000&&edits.empty());

 auto& counter=g_entities[70];counter.inuse=qtrue;counter.s.number=70;counter.classname=G_NewString("target_counter");
 counter.targetname=G_NewString("jar_sb_401");counter.e_UseFunc=useF_target_counter_use;
 counter.max_health=counter.count=2;CHECK(Own(state,70,jar::logic::counterId));
 selected=70;contextSelect();CHECK(vars["jar_contextLogicKind"]=="2");
 Run("logic_apply",{"4"});CHECK(counter.max_health==4&&counter.count==4);
 Run("logic_repeat");CHECK(counter.bounceCount==-1);
 counter.count=3;Revert();CHECK(!counter.bounceCount&&counter.count==3&&edits.size()==1);
 Revert();CHECK(counter.max_health==4&&counter.count==3&&edits.size()==1);
 counter.count=4;Revert();CHECK(counter.max_health==2&&counter.count==2&&edits.empty());
 counter.count=0;Run("logic_reset");CHECK(counter.count==2);Revert();CHECK(!counter.count);
 Run("logic_enabled");CHECK(counter.svFlags&SVF_INACTIVE);Revert();CHECK(!(counter.svFlags&SVF_INACTIVE)&&edits.empty());
 Forget(state,69);Forget(state,70);timer.inuse=counter.inuse=qfalse;
 auto& ticker=g_entities[72];ticker.inuse=qtrue;ticker.s.number=72;ticker.classname=G_NewString("func_timer");
 ticker.targetname=G_NewString("jar_sb_402");ticker.e_UseFunc=useF_func_timer_use;
 ticker.wait=1;ticker.nextthink=2500;ticker.e_ThinkFunc=thinkF_func_timer_think;
 CHECK(Own(state,72,jar::logic::timerId));selected=72;contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="3"&&vars["jar_contextLogicPending"]=="1");
 Run("logic_apply",{"100","1"});CHECK(ticker.wait==1&&edits.empty());
 Run("logic_apply",{"450","300"});CHECK(ticker.wait==.45f&&ticker.random==.3f&&ticker.nextthink==2500);
 Revert();CHECK(ticker.wait==1&&!ticker.random&&ticker.nextthink==2500);
 ticker.svFlags|=SVF_INACTIVE;
 Run("logic_running");CHECK(!ticker.nextthink&&edits.empty());
 ticker.svFlags&=~SVF_INACTIVE;
 Run("logic_running");CHECK(action=="activate"&&edits.empty());
 auto& selector=g_entities[73];selector.inuse=qtrue;selector.s.number=73;selector.classname=G_NewString("target_random");
 selector.targetname=G_NewString("jar_sb_403");selector.e_UseFunc=useF_target_random_use;
 CHECK(Own(state,73,jar::logic::randomId));selected=73;contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="4"&&vars["jar_contextLogicRemaining"]=="1");
 Run("logic_repeat");CHECK(selector.spawnflags&1);
 selector.count=1;contextSelect();
 Run("logic_reset");CHECK(!selector.count);Revert();CHECK(selector.count==1);
 Revert();CHECK(!selector.count&&!(selector.spawnflags&1)&&edits.empty());
 selector.target4=G_NewString("jar_sb_404");
 Run("logic_clear_output",{"0"});Run("logic_clear_output",{"5"});CHECK(selector.target4&&edits.empty());
 Run("logic_clear_output",{"4"});CHECK(!selector.target4&&edits.size()==1);
 Revert();CHECK(!selector.target4&&edits.size()==1); // Deleted destinations stay disconnected.
 edits.clear();Forget(state,72);Forget(state,73);ticker.inuse=selector.inuse=qfalse;
 auto& relay=g_entities[74];relay.inuse=qtrue;relay.s.number=74;relay.classname=G_NewString("target_relay");
 relay.targetname=G_NewString("jar_sb_405");relay.e_UseFunc=useF_target_relay_use;
 relay.wait=1000;relay.nextthink=1500;relay.e_ThinkFunc=thinkF_target_relay_use_go;
 CHECK(Own(state,74,jar::logic::relayId));selected=74;contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="5"&&vars["jar_contextLogicPending"]=="1");
 Run("logic_apply",{"400","60000.5"});CHECK(edits.empty());
 Run("logic_apply",{"400","750"});CHECK(relay.delay==400&&relay.wait==750&&relay.nextthink==1500);
 Run("logic_repeat");CHECK(relay.spawnflags&8);Revert();CHECK(!(relay.spawnflags&8));
 Revert();CHECK(!relay.delay&&relay.wait==1000&&relay.nextthink==1500);
 auto& score=g_entities[75];score.inuse=qtrue;score.s.number=75;score.classname=G_NewString("target_score");
 score.targetname=G_NewString("jar_sb_406");score.e_UseFunc=useF_Use_Target_Score;score.count=1;
 CHECK(Own(state,75,jar::logic::scoreId));selected=75;contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="6"&&vars["jar_contextLogicPoints"]=="1");
 Run("logic_apply",{"-10001"});Run("logic_apply",{"-5.5"});CHECK(edits.empty()&&score.count==1);
 Run("logic_apply",{"-25"});CHECK(score.count==-25);Revert();CHECK(score.count==1);
 auto& kill=g_entities[76];kill.inuse=qtrue;kill.s.number=76;kill.classname=G_NewString("target_kill");
 kill.targetname=G_NewString("jar_sb_407");kill.e_UseFunc=useF_target_kill_use;kill.spawnflags=64;
 CHECK(Own(state,76,jar::logic::killId));selected=76;contextSelect();
 CHECK(vars["jar_contextLogicKind"]=="7"&&vars["jar_contextLogicKillMode"]=="0");
 Run("logic_kill_mode");CHECK(kill.spawnflags==65);
 Run("logic_kill_mode");CHECK(kill.spawnflags==66);Revert();CHECK(kill.spawnflags==65);
 Revert();CHECK(kill.spawnflags==64&&edits.empty());
 Forget(state,74);Forget(state,75);Forget(state,76);relay.inuse=score.inuse=kill.inuse=qfalse;
 const auto ownedGateEntity=[](int slot,const char* name,unsigned catalog)->gentity_t&{
  auto& object=g_entities[slot];object.s.number=slot;object.inuse=qtrue;object.classname=G_NewString(name);
  CHECK(Own(state,slot,catalog));object.targetname=G_NewString(va("jar_sb_%u",owned(slot)->id));return object;
 };
 auto& off=ownedGateEntity(84,"target_deactivate",jar::logic::disableId);
 auto& onMarker=ownedGateEntity(85,"target_activate",jar::logic::enableId);
 auto& notice=ownedGateEntity(86,"target_print",87615608u);
 auto& hurt=ownedGateEntity(87,"trigger_hurt",jar::logic::timerId);
 auto& authored=g_entities[88];authored.s.number=88;authored.inuse=qtrue;
 authored.targetname=G_NewString("map_print");authored.e_UseFunc=useF_Use_Target_Print;
 off.e_UseFunc=useF_target_deactivate_use;onMarker.e_UseFunc=useF_target_activate_use;
 notice.e_UseFunc=useF_Use_Target_Print;hurt.e_TouchFunc=touchF_hurt_touch;
 off.target=notice.targetname;off.target2=hurt.targetname;off.target3=authored.targetname;
 onMarker.target=notice.targetname;onMarker.target2=hurt.targetname;
 selected=84;contextSelect();CHECK(vars["jar_contextLogicKind"]=="10"&&vars["jar_contextLogicOutput0"]=="86: target_print (use on)");
 CHECK(vars["jar_contextLogicOutput2"]=="Unavailable");
 CHECK(useGateMarker(off,&g_entities[0],&g_entities[0]));
 CHECK(!G_LogicUseEnabled(notice)&&!G_LogicUseEnabled(hurt)&&G_LogicUseEnabled(authored)&&edits.empty());
 contextLogicStatus(off);CHECK(vars["jar_contextLogicOutput0"]=="86: target_print (use off)");
 CHECK(useGateMarker(onMarker,&off,&g_entities[0]));CHECK(G_LogicUseEnabled(hurt)&&edits.empty());
 Forget(state,84);forgetEdits(84);off.inuse=qfalse;
 selected=85;contextSelect();Run("logic_enabled");CHECK(!G_LogicUseEnabled(onMarker));
 G_LogicSetUseEnabled(hurt,false);CHECK(useGateMarker(onMarker,&g_entities[0],&g_entities[0]));
 CHECK(!G_LogicUseEnabled(hurt)&&edits.size()==1);
 Revert();CHECK(G_LogicUseEnabled(onMarker)&&edits.empty());
 for(int slot:{85,86,87}){Forget(state,slot);g_entities[slot].inuse=qfalse;}
 authored.inuse=qfalse;
 {
  using namespace jar::contactTrigger;
  auto& repeat=ownedGateEntity(89,"trigger_multiple",repeatId);
  auto& once=ownedGateEntity(90,"trigger_once",onceId);
  for(auto* trigger:{&repeat,&once}){
   trigger->model=G_NewString(model);trigger->contents=CONTENTS_TRIGGER;
   trigger->e_UseFunc=useF_Use_Multi;trigger->e_TouchFunc=touchF_Touch_Multi;
   VectorSet(trigger->pos1,-21,-21,0);VectorSet(trigger->pos2,21,21,22);
  }
  CHECK(G_ContactSettings(repeat,{Repeat,0,1000,0,Actors,false,true}));
  selected=89;contextSelect();
  CHECK(vars["jar_contextContactKind"]=="1"&&vars["jar_contextContactInterval"]=="1000");
  Run("contact_apply",{"125","500","450"});CHECK(edits.empty());
  Run("contact_apply",{"125","500","40"});Run("contact_subject");Run("contact_use");
  Settings settings;
  CHECK(G_ContactRead(repeat,settings)&&Same(settings,{Repeat,125,500,40,Player,true,true}));
  Revert();Revert();Revert();
  CHECK(G_ContactRead(repeat,settings)&&Same(settings,{Repeat,0,1000,0,Actors,false,true})&&edits.empty());
  CHECK(G_ContactSettings(once,{Once,0,0,0,Actors,false,true}));
  G_ContactSetSpent(once,true);selected=90;contextSelect();
  CHECK(vars["jar_contextContactKind"]=="2"&&vars["jar_contextContactSpent"]=="1");
  Run("contact_rearm");CHECK(!G_ContactSpent(once)&&(once.contents&CONTENTS_TRIGGER));
  Revert();CHECK(G_ContactSpent(once)&&!(once.contents&CONTENTS_TRIGGER)&&edits.empty());
  auto& portableHurt=ownedGateEntity(91,"trigger_hurt",hurtId);
  portableHurt.model=G_NewString(model);portableHurt.e_UseFunc=useF_hurt_use;
  portableHurt.e_TouchFunc=touchF_hurt_touch;portableHurt.contents=CONTENTS_TRIGGER;
  CHECK(G_ContactSettings(portableHurt,{Hurt,0,1000,0,Actors,false,true,5,Normal}));
  selected=91;contextSelect();CHECK(vars["jar_contextContactDamage"]=="5"&&vars["jar_contextContactKind"]=="3");
  Run("contact_hurt_apply",{"45","99"});CHECK(edits.empty());
  Run("contact_hurt_apply",{"45","900"});Run("contact_hurt_mode");
  CHECK(G_ContactRead(portableHurt,settings)&&Same(settings,{Hurt,0,900,0,Actors,false,true,45,Falling}));
  Run("contact_hurt_arm");CHECK(vars["jar_contextContactStatus"]=="Disarmed"&&!portableHurt.contents);
  Revert();CHECK(portableHurt.contents&CONTENTS_TRIGGER);
  Revert();Revert();CHECK(G_ContactRead(portableHurt,settings)&&Same(settings,{Hurt,0,1000,0,Actors,false,true,5,Normal}));
  Forget(state,91);portableHurt.inuse=qfalse;
  auto& push=ownedGateEntity(92,"target_push",jar::push::markerId);
  auto& aim=ownedGateEntity(93,"target_position",95740442u);
  push.model=G_NewString(jar::push::markerModel);push.e_UseFunc=useF_Use_target_push;
  CHECK(G_PushSet(push,{jar::push::Marker,600,100}));
  selected=92;contextSelect();CHECK(vars["jar_contextPushKind"]=="2");
  Run("push_apply",{"2001","200"});CHECK(edits.empty());
  Run("push_apply",{"850","500"});Run("push_add");
  Run("push_vehicles");CHECK(vars["jar_contextPushVehicles"]=="1");
  Revert();jar::push::Settings pushSettings;
  CHECK(G_PushRead(push,pushSettings)&&!pushSettings.vehicles);
  Revert();Revert();CHECK(push.speed==600&&push.wait==100&&edits.empty());
  editPushAimLink(push,aim.targetname);CHECK(G_PushTarget(push)==&aim&&push.damage==jar::push::Linked);
  CHECK(edits.empty());Run("push_unlink");CHECK(!push.target&&edits.size()==1);
  Revert();CHECK(G_PushTarget(push)==&aim&&edits.empty());
  for(int slot:{92,93}){Forget(state,slot);g_entities[slot].inuse=qfalse;}
  auto& tele=ownedGateEntity(94,"target_teleporter",jar::teleport::markerId);
  auto& exit=ownedGateEntity(95,"misc_teleporter_dest",jar::teleport::destinationId);
  tele.model=G_NewString(jar::teleport::markerModel);tele.e_UseFunc=useF_target_teleporter_use;
  CHECK(G_TeleportSet(tele,{jar::teleport::Marker,500}));
  exit.message=G_NewString("West exit");editTeleportLink(tele,exit.targetname);
  selected=94;contextSelect();CHECK(vars["jar_contextTeleportTarget"]=="Destination: 95 West exit");
  Run("teleport_apply",{"99"});CHECK(edits.empty());
  Run("teleport_apply",{"800"});Run("teleport_view");
  Run("teleport_vehicles");CHECK(vars["jar_contextTeleportVehicles"]=="1");
  Run("teleport_unlink");CHECK(!tele.target);
  Revert();CHECK(G_TeleportDestination(tele)==&exit);
  Revert();jar::teleport::Settings markerSettings;
  CHECK(G_TeleportRead(tele,markerSettings)&&!markerSettings.vehicles);
  Revert();Revert();CHECK(G_TeleportDestination(tele)==&exit&&tele.delay==500&&edits.empty());
  for(int slot:{94,95}){Forget(state,slot);g_entities[slot].inuse=qfalse;}
  auto& portal=ownedGateEntity(96,"trigger_teleport",jar::teleport::volumeId);
  portal.model=G_NewString(jar::teleport::volumeModel);portal.e_TouchFunc=touchF_trigger_teleporter_touch;
  CHECK(G_TeleportSet(portal,{jar::teleport::Volume,500,jar::teleport::NPCs}));
  selected=96;contextSelect();Run("teleport_props");Run("teleport_projectiles");Run("teleport_subject");Run("teleport_assemblies");
  jar::teleport::Settings portalSettings;
  CHECK(G_TeleportRead(portal,portalSettings)&&portalSettings.props&&portalSettings.projectiles&&portalSettings.assemblies&&
        !portalSettings.frozenProps&&portalSettings.subject==jar::teleport::NoActors&&vars["jar_contextTeleportAssemblies"]=="1");
  Run("teleport_frozen_props");
  CHECK(G_TeleportRead(portal,portalSettings)&&portalSettings.frozenProps&&vars["jar_contextTeleportFrozenProps"]=="1");
  Run("teleport_movers");
  CHECK(G_TeleportRead(portal,portalSettings)&&portalSettings.movers&&vars["jar_contextTeleportMovers"]=="1");
  Run("teleport_world_anchors");
  CHECK(G_TeleportRead(portal,portalSettings)&&portalSettings.worldAnchors&&vars["jar_contextTeleportWorldAnchors"]=="1");
  Revert();CHECK(G_TeleportRead(portal,portalSettings)&&!portalSettings.worldAnchors&&portalSettings.movers);
  Revert();CHECK(G_TeleportRead(portal,portalSettings)&&!portalSettings.movers&&portalSettings.frozenProps);
  Revert();CHECK(G_TeleportRead(portal,portalSettings)&&!portalSettings.frozenProps&&portalSettings.assemblies);
  Revert();CHECK(G_TeleportRead(portal,portalSettings)&&!portalSettings.assemblies);
  Revert();Revert();Revert();CHECK(G_TeleportRead(portal,portalSettings)&&!portalSettings.props&&!portalSettings.projectiles&&edits.empty());
  Forget(state,96);portal.inuse=qfalse;
  for(int slot:{89,90}){Forget(state,slot);g_entities[slot].inuse=qfalse;}
 }
 // Moving map controllers keep their trajectory and endpoint offsets.
 auto&m=g_entities[8];m.inuse=qtrue;m.s.number=8;m.s.eType=ET_MOVER;m.bmodel=qtrue;m.classname=const_cast<char*>("func_door");m.s.pos.trType=TR_LINEAR;m.s.pos.trDelta[0]=10;m.pos2[0]=100;m.nextthink=200;
 selected=8;contextSelect();level.time=100;Run("position",{"50","0","0"});CHECK(m.s.pos.trType==TR_LINEAR&&m.s.pos.trBase[0]==50&&m.pos2[0]==150);
 const auto moverEdits=edits.size();
 blocked=true;Run("position",{"60","0","0"});Revert();
 CHECK(edits.size()==moverEdits&&m.currentOrigin[0]==50&&m.pos2[0]==150);
 blocked=false;
 level.time=150;Revert();CHECK(m.s.pos.trBase[0]==0&&m.pos2[0]==100&&m.nextthink==250&&m.s.pos.trDelta[0]==10);
 Run("angles",{"0","90","0"});CHECK(m.currentAngles[YAW]==90&&m.pos2[0]==100);
 Revert();CHECK(m.currentAngles[YAW]==0&&m.pos2[0]==100);
 nativeBegin(m,true);Run("position",{"25","0","0"});
 CHECK(m.currentOrigin[0]==25&&m.pos2[0]==100&&nativeOwnedControl(8));
 nativeRelease(8,false);CHECK(m.pos2[0]==125&&m.s.pos.trBase[0]==25);
 Revert();CHECK(m.currentOrigin[0]==0&&m.pos2[0]==100&&nativeOwnedControl(8));
 nativeRelease(8,false);CHECK(m.pos2[0]==100&&m.s.pos.trBase[0]==0);
 // Train pose edits and Undo restore both path offset and external momentum.
 m.classname=const_cast<char*>("func_train");contextSelect();Run("position",{"25","0","0"});
 JarMoverMotion motion;CHECK(G_JAR_MoverMotionRead(m,motion)&&motion.offset[0]==25);Revert();
 CHECK(G_JAR_MoverMotionRead(m,motion)&&motion.offset[0]==0&&m.currentOrigin[0]==0);
 motion.offset[0]=12;motion.velocity[0]=80;motion.angular[2]=2;CHECK(G_JAR_MoverMotionRestore(m,motion));
 contextSelect();Run("position",{"10","0","0"});CHECK(G_JAR_MoverMotionRead(m,motion)&&motion.offset[0]==22&&motion.velocity[0]==0&&motion.angular[2]==0);
 Revert();CHECK(G_JAR_MoverMotionRead(m,motion)&&motion.offset[0]==12&&motion.velocity[0]==80&&motion.angular[2]==2&&m.currentOrigin[0]==0);
 // Removed/reused slots cannot receive a stale editor transaction.
 selected=7;contextSelect();const auto reused=contextToken;forgetEdits(7);args={"jar_sb","context","health",std::to_string(reused),"900"};contextCommand();CHECK(e.health==100);
 contextSelect();on=false;Run("health",{"700"});CHECK(e.health==100);on=true;
 for(const char*name:{"rope","thruster","unlink","weld","hinge"}){Run(name);CHECK(action==name);}
 Run("remove");CHECK(contextToken==0&&!contextValid());
 // A held native NPC rotates without damage or changing its grip controller.
 e.inuse=qtrue;nativeBegin(e,false);held=7;
 CHECK(physgunGrip.Begin(pv(e.currentOrigin),pv(e.currentAngles),pv(e.currentOrigin),0));
 args={"jar_sb","rotateheld","1","30","10"};rotateHeld();physgunNativeTurn(e,.3f);
 CHECK(std::fabs(e.currentAngles[1]-30)<.01f&&std::fabs(e.currentAngles[0]-10)<.01f&&e.health==100&&nativeControls.count(7));
 args[2]="2";rotateHeld();physgunNativeTurn(e,.3f);CHECK(std::fabs(e.currentAngles[1]-30)<.01f);releaseHeld();
 {
  vec3_t before,oldFacing;
  VectorCopy(e.currentOrigin,before);VectorCopy(e.currentAngles,oldFacing);
  nativeBegin(e,false);held=7;
  const jar::physics::Vec click={before[0]+10,before[1],before[2]};
  CHECK(physgunGrip.Begin(pv(before),pv(oldFacing),click,0));
  physgunViewYaw=90;
  blocked=true;physgunNativeTurn(e,.3f);blocked=false;
  CHECK(VectorCompare(e.currentOrigin,before)&&VectorCompare(e.currentAngles,oldFacing));
  physgunNativeTurn(e,.3f);
  const auto contact=physgunGrip.Point(pv(e.currentOrigin),pv(e.currentAngles));
  CHECK(std::fabs(contact.x-click.x)<.001f&&std::fabs(contact.y-click.y)<.001f&&
        std::fabs(contact.z-click.z)<.001f&&std::fabs(AngleSubtract(e.currentAngles[YAW],oldFacing[YAW])-90)<.001f);
  CHECK(e.health==100&&Distance(e.currentOrigin,before)>5);
  releaseHeld();physgunViewYaw=0;
 }
 {
  // These points lie inside a middle pose but outside both endpoint hulls.
  static vec3_t obstacle;
  static int expectedMask;
  static bool clip;
  const auto trace=gi.trace;
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t low,const vec3_t high,
   const vec3_t end,int,int mask,EG2_Collision,int){
   CHECK(mask==expectedMask);
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   bool overlaps=clip;
   for(int a=0;a<3;++a)
    overlaps=overlaps&&obstacle[a]>=start[a]+low[a]&&obstacle[a]<=start[a]+high[a];
   if(overlaps)hit->startsolid=qtrue;
  };
  gentity_t wall{};
  wall.inuse=qtrue;wall.count=1;
  wall.classname=const_cast<char*>("func_wall");wall.model=const_cast<char*>(JAR_WALL_PORTABLE_MODEL);
  G_WallRefresh(wall);
  const jar::physics::Vec click={32,0,0};
  CHECK(physgunGrip.Begin({}, {}, click, 0));
  expectedMask=MASK_SOLID|CONTENTS_BODY;
  VectorSet(obstacle,-57,0,0);
  physgunViewYaw=90;clip=true;
  physgunNativeTurn(wall,.3f);
  CHECK(VectorCompare(wall.currentOrigin,vec3_origin)&&VectorCompare(wall.currentAngles,vec3_origin));
  clip=false;
  physgunNativeTurn(wall,.3f);
  CHECK(std::fabs(wall.currentAngles[YAW]-90)<.01f);
  const auto point=physgunGrip.Point(pv(wall.currentOrigin),pv(wall.currentAngles));
  CHECK(std::fabs(point.x-click.x)<.001f&&std::fabs(point.y-click.y)<.001f&&std::fabs(point.z-click.z)<.001f);
  physgunViewYaw=0;

  gentity_t panel{};panel.inuse=qtrue;
  const vec3_t turned={0,90,0};
  expectedMask=MASK_PLAYERSOLID|CONTENTS_CORPSE;
  panel.classname=const_cast<char*>("func_door");panel.model=const_cast<char*>(JAR_SLIDING_DOOR_PORTABLE);
  VectorSet(obstacle,-100,-9,40);clip=true;
  CHECK(!nativeObjectTurnClear(panel,vec3_origin,turned,nullptr,{},false));
  clip=false;
  CHECK(nativeObjectTurnClear(panel,vec3_origin,turned,nullptr,{},false));
  panel.classname=const_cast<char*>("func_button");panel.model=const_cast<char*>(JAR_BUTTON_PORTABLE_MODEL);
  VectorSet(obstacle,-21.5f,0,10);clip=true;
  CHECK(!nativeObjectTurnClear(panel,vec3_origin,turned,nullptr,{},false));
  clip=false;
  CHECK(nativeObjectTurnClear(panel,vec3_origin,turned,nullptr,{},false));
  gi.trace=trace;
 }
 // Portable volumes expose bounded local dimensions and remain non-solid
 // during exact placement, Physgun motion and Undo, even in occupied scenery.
 auto&v=g_entities[9];v.inuse=qtrue;v.s.number=9;v.classname=const_cast<char*>("trigger_space");v.model=const_cast<char*>("@jar_box");jar::volumeEntity::Initialize(v);
 v.contents=CONTENTS_TRIGGER;v.e_TouchFunc=touchF_space_touch;
 selected=9;contextSelect();CHECK(vars["jar_contextVolumeAllowed"]=="1"&&vars["jar_contextVolume0"]=="128.000"&&vars["jar_contextHealthAllowed"]=="0");
 CHECK(vars["jar_contextSpaceAllowed"]=="1"&&vars["jar_contextSpaceArmed"]=="1"&&vars["jar_contextSpaceEnabled"]=="1");
 Run("space_armed");CHECK(!v.contents&&vars["jar_contextSpaceArmed"]=="0");
 Run("space_enabled");CHECK(!v.contents&&(v.svFlags&SVF_INACTIVE)&&vars["jar_contextSpaceEnabled"]=="0");
 Revert();CHECK(!v.contents&&!(v.svFlags&SVF_INACTIVE));
 Revert();CHECK(v.contents==CONTENTS_TRIGGER&&!(v.svFlags&SVF_INACTIVE)&&edits.empty());
 edits.clear();blocked=true;Run("volume",{"32","128","64"});CHECK(v.pos1[0]==-16&&v.pos2[2]==64&&edits.size()==1);
 for(const char*bad:{"nan","1;quit","7","8193","-32"}){Run("volume",{bad,"128","64"});CHECK(v.pos2[0]==16&&edits.size()==1);}
 Run("angles",{"0","90","0"});CHECK(std::fabs(v.maxs[0]-64)<.001f&&std::fabs(v.maxs[1]-16)<.001f);Revert();CHECK(v.currentAngles[1]==0&&v.maxs[0]==16);
 Revert();CHECK(v.pos1[0]==-64&&v.pos2[2]==128&&edits.empty());
 {
  // A native edit can retire an older record while relinking the selected entity.
  Edit earlier{};earlier.entity=400;earlier.spawn=true;rememberAction(std::move(earlier));
  Run("volume",{"32","128","64"});
  Edit later{};later.entity=401;later.spawn=true;rememberAction(std::move(later));
  const auto link=gi.linkentity;
  gi.linkentity=[](gentity_t* entity){if(entity->s.number==9)forgetEdits(400);};
  Revert();
  gi.linkentity=link;
  CHECK(v.pos1[0]==-64&&v.pos2[2]==128&&edits.size()==1&&edits.front().entity==401);
  edits.clear();
 }
 Run("position",{"100","0","0"});CHECK(v.currentOrigin[0]==100);Revert();CHECK(v.currentOrigin[0]==0);
 nativeBegin(v,false);held=9;nativeFrame(.1f);CHECK(std::fabs(v.currentOrigin[0]-102.4f)<.001f);releaseHeld();CHECK(!nativeControls.count(9));
 vec3_t dimensions;CHECK(jar::volumeEntity::Dimensions(v,dimensions)&&dimensions[0]==128);blocked=false;
 selected=7;contextSelect();CHECK(vars["jar_contextVolumeAllowed"]=="0");const auto count=edits.size();Run("volume",{"32","32","32"});CHECK(edits.size()==count);
 Run("space_armed");CHECK(vars["jar_contextSpaceAllowed"]=="0"&&edits.size()==count);
 // Labels use a bounded hex payload and exact selection tokens. The server
 // rejects command punctuation, embedded NULs and edits to unrelated classes.
 auto&marker=g_entities[10];marker.inuse=qtrue;marker.s.number=10;marker.classname=G_NewString("target_location");marker.message=G_NewString("Old room");
 selected=10;contextSelect();CHECK(vars["jar_contextLabelAllowed"]=="1"&&vars["jar_contextText"]=="Old room"&&vars["jar_contextLocation"]=="Hangar");
 edits.clear();Run("label",{"48616e6761722031"});CHECK(!strcmp(marker.message,"Hangar 1")&&edits.size()==1);Revert();CHECK(!strcmp(marker.message,"Old room")&&edits.empty());
 for(const char*bad:{"","zz","4","00","410042","2020","412242","413b42","415c42","410a42","c3a9"}){Run("label",{bad});CHECK(!strcmp(marker.message,"Old room")&&edits.empty());}
 const auto longLabel=Hex(std::string(64,'a'));Run("label",{longLabel.c_str()});CHECK(edits.empty());
 const auto maxLabel=Hex(std::string(63,'a'));Run("label",{maxLabel.c_str()});CHECK(strlen(marker.message)==63);Revert();
 selected=7;contextSelect();CHECK(vars["jar_contextLabelAllowed"]=="0");Run("label",{"48616e676172"});CHECK(edits.empty());
 marker.message=nullptr;selected=10;contextSelect();Run("label",{"48616e676172"});Revert();CHECK(marker.message==nullptr);
 marker.classname=G_NewString("item_datapad");contextSelect();CHECK(vars["jar_contextLabelAllowed"]=="1");
 Run("label",{"4163636573732037333432"});CHECK(!strcmp(marker.message,"Access 7342"));Revert();CHECK(!marker.message);
 marker.classname=G_NewString("target_location");
 marker.classname=G_NewString("trigger_location");marker.model=G_NewString("@jar_box");
 jar::volumeEntity::InitializeLocation(marker);CHECK(jar::volumeEntity::Refresh(marker));
 contextSelect();CHECK(vars["jar_contextVolumeAllowed"]=="1"&&vars["jar_contextLabelAllowed"]=="1");
 Run("label",{"48616e676172"});Run("volume",{"96","48","32"});
 CHECK(!strcmp(marker.message,"Hangar")&&marker.pos2[0]==48&&marker.pos2[2]==32);
 Revert();CHECK(marker.pos2[0]==21&&marker.pos2[2]==22&&!strcmp(marker.message,"Hangar"));
 Revert();CHECK(marker.message==nullptr);
 marker.classname=G_NewString("target_location");marker.model=nullptr;
 marker.classname=G_NewString("trigger_always");marker.model=G_NewString("@jar_always");marker.delay=300;G_AlwaysSchedule(marker,250);
 contextSelect();CHECK(vars["jar_contextAlwaysAllowed"]=="1"&&vars["jar_contextAlwaysDelay"]=="300");
 for(const char*bad:{"49","60001","100.5","nan"}){Run("always_delay",{bad});CHECK(edits.empty()&&marker.delay==300);}
 Run("always_delay",{"1200"});CHECK(marker.delay==1200&&G_AlwaysRemaining(marker)==250);
 Revert();CHECK(marker.delay==300&&G_AlwaysRemaining(marker)==250&&edits.empty());
 marker.classname=G_NewString("target_location");marker.model=nullptr;G_AlwaysSchedule(marker,0);
 marker.classname=G_NewString("trigger_entdist");marker.model=G_NewString("@jar_entdist");
 marker.count=JAR_DISTANCE_DEFAULT;marker.spawnflags=JAR_DISTANCE_PLAYER;
 marker.ownername=G_NewString("jar_sb_missing");contextSelect();
 CHECK(vars["jar_contextDistanceAllowed"]=="1"&&vars["jar_contextDistanceReference"]=="2");
 for(const char*bad:{"0","4097","12.5","nan"}){Run("distance",{bad});CHECK(edits.empty()&&marker.count==192);}
 Run("distance",{"4096"});CHECK(marker.count==4096);
 Run("distance_subjects",{});CHECK(marker.spawnflags==JAR_DISTANCE_NPC);
 Run("distance_arm",{});CHECK(marker.svFlags&SVF_INACTIVE);
 Run("distance_clear_reference",{});CHECK(!marker.ownername);
 Revert();CHECK(marker.ownername&&!strcmp(marker.ownername,JAR_DISTANCE_MISSING_REFERENCE_NAME));
 Revert();CHECK(!(marker.svFlags&SVF_INACTIVE));Revert();CHECK(marker.spawnflags==JAR_DISTANCE_PLAYER);
 Revert();CHECK(marker.count==192&&edits.empty());marker.ownername=nullptr;
 marker.classname=G_NewString("target_location");marker.model=nullptr;
 marker.classname=G_NewString("func_wall");marker.model=G_NewString(JAR_WALL_PORTABLE_MODEL);
 VectorClear(marker.currentAngles);
 CHECK(G_WallSetVisible(marker,true));contextSelect();
 CHECK(vars["jar_contextWallAllowed"]=="1"&&vars["jar_contextWallVisible"]=="1");
 CHECK(marker.mins[0]==-32&&marker.maxs[1]==64&&marker.maxs[2]==64);
 Run("wall_visible",{});CHECK(!marker.count&&marker.contents==CONTENTS_ITEM);
 blocked=true;Revert();CHECK(!marker.count&&!edits.empty());
 blocked=false;
 Revert();CHECK(marker.count==1&&marker.contents==CONTENTS_SOLID);
 Run("wall_enabled",{});CHECK(marker.svFlags&SVF_INACTIVE);Revert();CHECK(!(marker.svFlags&SVF_INACTIVE));
 marker.classname=G_NewString("target_location");marker.model=nullptr;
 marker.classname=G_NewString("info_player_start");contextSelect();CHECK(vars["jar_contextLabelAllowed"]=="0");
 marker.model=G_NewString("@jar_start");contextSelect();CHECK(vars["jar_contextLabelAllowed"]=="1");
 Run("label",{"48616e676172"});CHECK(!strcmp(marker.message,"Hangar"));Revert();CHECK(!marker.message);
 marker.classname=G_NewString("target_location");marker.model=nullptr;
 // Parameter values are literal, safely transported, bounded, and undoable.
 selected=7;contextSelect();edits.clear();CHECK(vars["jar_contextParm0"].empty()&&vars["jar_contextParmAllowed0"]=="1");
 Run("parm",{"1","2b35"});CHECK(e.parms&&!strcmp(e.parms->parm[0],"+5")&&edits.size()==1);Revert();CHECK(!e.parms->parm[0][0]&&edits.empty());
 Run("parm",{"16","413b2242225c"});CHECK(!strcmp(e.parms->parm[15],"A;\"B\"\\"));Run("parm",{"16",""});CHECK(!e.parms->parm[15][0]);Revert();CHECK(!strcmp(e.parms->parm[15],"A;\"B\"\\"));Revert();CHECK(edits.empty());
 for(const char*slot:{"0","17","-1","1.0"}){Run("parm",{slot,"41"});CHECK(edits.empty());}
 for(const std::string invalid:std::vector<std::string>{"zz","41ff","410a","410042",Hex(std::string(64,'a'))}){Run("parm",{"1",invalid.c_str()});CHECK(edits.empty()&&!e.parms->parm[0][0]);}
 const auto staleParmToken=contextToken;contextPublish();args={"jar_sb","context","parm",std::to_string(staleParmToken),"1","41"};contextCommand();CHECK(edits.empty());
 // Travel applies a whole transaction, retains a cross-map spawn point, and undoes.
 auto&travel=g_entities[27];travel.inuse=qtrue;travel.s.number=27;travel.classname=G_NewString("target_level_change");travel.model=G_NewString("@jar_level_change");travel.message=G_NewString("hoth2");selected=27;contextSelect();edits.clear();
 CHECK(vars["jar_contextTravelAllowed"]=="1"&&vars["jar_contextTravelMap"]=="hoth2"&&vars["jar_contextHealthAllowed"]=="0");
 const auto travelToken=contextToken;Run("travel",{Hex("t1_surprise").c_str(),Hex("arrival").c_str(),"1"});CHECK(!strcmp(travel.message,"t1_surprise")&&!strcmp(travel.target,"arrival")&&(travel.spawnflags&1)&&edits.size()==1);
 Revert();CHECK(!strcmp(travel.message,"hoth2")&&!travel.target&&!(travel.spawnflags&1)&&edits.empty());
 args={"jar_sb","context","travel",std::to_string(travelToken),Hex("t1_surprise").c_str(),"","0"};contextCommand();CHECK(!strcmp(travel.message,"hoth2")&&edits.empty());
 for(const char*bad:{"missing","../hoth2","hoth2;quit","+menu","hoth2\nquit"}){Run("travel",{Hex(bad).c_str(),"","0"});CHECK(!strcmp(travel.message,"hoth2")&&edits.empty());}
 Run("travel",{Hex("t1_surprise").c_str(),Hex("bad spawn").c_str(),"0"});CHECK(edits.empty());Run("travel",{Hex("t1_surprise").c_str(),"",".5"});CHECK(edits.empty());
 Run("travel",{Hex("t1_surprise").c_str(),"","0"});CHECK(!strcmp(travel.message,"t1_surprise")&&!travel.target);forgetEdits(27);CHECK(edits.empty());
 // Music has independent visibility, bounded hex transport, cycling and Undo.
 marker.classname=G_NewString("target_play_music");marker.message=G_NewString(jar::music::defaultTrack);selected=10;contextSelect();
 CHECK(vars["jar_contextMusicAllowed"]=="1"&&vars["jar_contextLabelAllowed"]=="0"&&vars["jar_contextHealthAllowed"]=="0");
 const std::string pair=std::string(jar::music::tracks[0])+" "+jar::music::tracks[1];const auto payload=Hex(pair);Run("music",{payload.c_str()});CHECK(marker.message==pair&&edits.size()==1);Revert();CHECK(!strcmp(marker.message,jar::music::defaultTrack)&&edits.empty());
 Run("music_next");CHECK(strcmp(marker.message,jar::music::defaultTrack));Revert();CHECK(!strcmp(marker.message,jar::music::defaultTrack));
 for(const std::string bad:std::vector<std::string>{"","00","nothex",Hex("music/missing.mp3"),Hex(std::string(jar::music::defaultTrack)+";quit"),Hex(std::string(jar::music::defaultTrack)+std::string("\0bad",4))}){Run("music",{bad.c_str()});CHECK(edits.empty()&&!strcmp(marker.message,jar::music::defaultTrack));}
 {
 auto&probe=g_entities[46];probe.inuse=qtrue;probe.s.number=46;probe.classname=G_NewString("misc_cubemap");probe.model=G_NewString("@jar_reflection");probe.targetname=G_NewString("probe_context");probe.count=1;probe.radius=512;probe.spawnflags=128;
 selected=46;contextSelect();edits.clear();const auto initial=contextToken;CHECK(vars["jar_contextReflectionAllowed"]=="1");
 Run("reflection",{"1024","0"});CHECK(probe.radius==1024&&probe.spawnflags==129&&probe.count==1&&edits.size()==1);Revert();CHECK(probe.radius==512&&probe.spawnflags==128&&edits.empty());
 for(const char*bad:{"15","32769","nan","1;quit"}){Run("reflection",{bad,"1"});CHECK(edits.empty()&&probe.radius==512);}Run("reflection",{"512","0.5"});CHECK(edits.empty());
 Run("reflection_refresh");CHECK(probe.count==2&&edits.empty());Run("angles",{"90","45","0"});CHECK(probe.s.angles[0]==90&&probe.currentAngles[0]==90);Revert();CHECK(probe.s.angles[0]==0);
 selected=7;contextSelect();args={"jar_sb","context","reflection",std::to_string(initial),"2048","0"};contextCommand();CHECK(probe.radius==512&&edits.empty());CHECK(vars["jar_contextReflectionAllowed"]=="0");probe.inuse=qfalse;
 }
 // Cloud edits are atomic, token-checked and preserve unrelated flags.
 auto&cloud=g_entities[13];cloud.inuse=qtrue;cloud.s.number=13;cloud.classname=G_NewString("fx_cloudlayer");cloud.model=G_NewString("@jar_cloud");cloud.s.eType=ET_CLOUD;cloud.radius=512;cloud.random=128;cloud.wait=0;cloud.spawnflags=128;
 selected=13;contextSelect();edits.clear();CHECK(vars["jar_contextCloudAllowed"]=="1"&&vars["jar_contextCloudRadius"]=="512.000");
 Run("cloud",{"1024","256","2","1","1","0"});CHECK(cloud.radius==1024&&cloud.random==256&&cloud.wait==2&&cloud.spawnflags==135&&edits.size()==1);Revert();CHECK(cloud.radius==512&&cloud.spawnflags==128&&edits.empty());
 for(const char*bad:{"15","32769","nan","1;quit"}){Run("cloud",{bad,"0","0","0","0","1"});CHECK(cloud.radius==512&&edits.empty());}
 Run("cloud",{"512","505","0","0","0","1"});CHECK(edits.empty());Run("cloud",{"512","128","0",".5","0","1"});CHECK(edits.empty());
 nativeBegin(cloud,false);nativeControls[13].velocity[2]=100;nativeRelease(13,true);CHECK(cloud.s.pos.trType!=TR_GRAVITY&&cloud.e_ThinkFunc!=thinkF_G_RunObject);
 Run("angles",{"90","0","0"});CHECK(cloud.currentAngles[0]==90&&cloud.s.angles[0]==90);Revert();CHECK(cloud.currentAngles[0]==0&&cloud.s.angles[0]==0);
 selected=7;contextSelect();CHECK(vars["jar_contextCloudAllowed"]=="0");Run("cloud",{"512","128","0","0","0","1"});CHECK(edits.empty());
 // Flight boundaries expose time/policy beside their existing geometry page.
 auto&boundary=g_entities[12];boundary.inuse=qtrue;boundary.s.number=12;boundary.classname=G_NewString("trigger_shipboundary");boundary.model=G_NewString("@jar_box");boundary.count=3000;boundary.spawnflags=128|1;jar::volumeEntity::Initialize(boundary);
 selected=12;contextSelect();edits.clear();CHECK(vars["jar_contextBoundaryAllowed"]=="1"&&vars["jar_contextBoundaryTime"]=="3.000"&&vars["jar_contextVolume0"]=="1024.000");
 Run("boundary",{"60","0"});CHECK(boundary.count==60000&&boundary.spawnflags==128&&edits.size()==1);Revert();CHECK(boundary.count==3000&&boundary.spawnflags==129&&edits.empty());
 for(const char*bad:{"0",".09","60.001","nan","1;quit"}){Run("boundary",{bad,"1"});CHECK(boundary.count==3000&&edits.empty());}
 Run("boundary",{"3",".5"});CHECK(edits.empty());Run("boundary",{".1","1"});CHECK(boundary.count==100);Revert();
 selected=7;contextSelect();CHECK(vars["jar_contextBoundaryAllowed"]=="0");Run("boundary",{"3","1"});CHECK(edits.empty());
 // Runner transactions preserve queued work on rejection; valid edits/Undo
 // cancel it, retain other spawnflags, and use independent string ownership.
 auto&runner=g_entities[11];runner.inuse=qtrue;runner.s.number=11;runner.classname=G_NewString("target_scriptrunner");runner.behaviorSet[BSET_USE]=G_NewString(jar::scriptRunner::defaultScript);runner.spawnflags=128|1;runner.count=-1;runner.wait=1000;
 selected=11;contextSelect();edits.clear();CHECK(vars["jar_contextScriptAllowed"]=="1"&&vars["jar_contextScriptWait"]=="1.000");
 const auto scriptHex=Hex(jar::scriptRunner::scripts[0]);Run("script",{scriptHex.c_str(),"0.125","2","3","0"});CHECK(runner.delay==125&&runner.wait==2000&&runner.count==3&&runner.spawnflags==128&&edits.size()==1);
 Revert();CHECK(runner.delay==0&&runner.wait==1000&&runner.count==-1&&runner.spawnflags==129&&edits.empty());
 runner.e_ThinkFunc=thinkF_scriptrunner_run;runner.nextthink=9999;runner.activator=&e;
 scriptMissing=true;Run("script",{scriptHex.c_str(),"0","1","-1","1"});scriptMissing=false;CHECK(edits.empty()&&runner.nextthink==9999&&runner.activator==&e);
 for(const char*bad:{"-2","1.5","100001","nan","1;quit"}){Run("script",{scriptHex.c_str(),"0","1",bad,"1"});CHECK(edits.empty()&&runner.nextthink==9999);}
 for(const char*bad:{"-1","3601","nan"}){Run("script",{scriptHex.c_str(),bad,"1","1","1"});CHECK(edits.empty()&&runner.nextthink==9999);}
 Run("script",{Hex("../escape").c_str(),"0","1","1","1"});CHECK(edits.empty());
 Run("script",{scriptHex.c_str(),"0","1","0","1"});CHECK(!runner.nextthink&&!runner.activator&&runner.e_UseFunc==useF_NULL&&runner.e_ThinkFunc==thinkF_NULL);Revert();CHECK(runner.e_UseFunc==useF_target_scriptrunner_use&&!runner.nextthink);
 selected=7;contextSelect();CHECK(vars["jar_contextScriptAllowed"]=="0");Run("script",{scriptHex.c_str(),"0","1","-1","1"});CHECK(edits.empty());
 // Weather transactions retain the earlier controller priority on Undo.
 globals.num_entities=32;auto&weather=g_entities[15];weather.inuse=qtrue;weather.s.number=15;weather.classname=G_NewString("fx_rain");weather.model=G_NewString("@jar_weather");weather.spawnflags=256;
 jar::weather::Settings weatherSettings;CHECK(G_WeatherSet(weather,weatherSettings));const int weatherOrder=weather.delay;selected=15;contextSelect();edits.clear();CHECK(vars["jar_contextWeatherAllowed"]=="1");
 Run("weather",{"3","2","4","0.75","1500","1"});CHECK(weather.spawnflags==259&&weather.random==2&&weather.radius==4&&weather.wait==.75f&&weather.speed==1500&&weather.delay>weatherOrder&&edits.size()==1);Revert();CHECK(weather.spawnflags==257&&weather.delay==weatherOrder&&weather.random==1&&edits.empty());
 for(const char*bad:{"0","4.01","nan","1;quit"}){Run("weather",{"1",bad,"1","0.5","500","1"});CHECK(weather.random==1&&edits.empty());}
 Run("weather",{"1.5","1","1",".5","500","1"});CHECK(edits.empty());Run("weather",{"1","1","1",".5","500",".5"});CHECK(edits.empty());
 Run("weather",{"0",".1",".1","0","0","0"});CHECK(weather.spawnflags==384&&weather.delay==weatherOrder);Revert();CHECK(weather.delay==weatherOrder&&weather.spawnflags==257);
 nativeBegin(weather,false);nativeControls[15].velocity[2]=100;nativeRelease(15,true);CHECK(weather.s.pos.trType!=TR_GRAVITY&&weather.e_ThinkFunc!=thinkF_G_RunObject);
 selected=7;contextSelect();CHECK(vars["jar_contextWeatherAllowed"]=="0");Run("weather",{"0","1","1",".5","500","1"});CHECK(edits.empty());
 auto&laser=g_entities[18];laser.inuse=qtrue;laser.s.number=18;laser.classname=G_NewString("misc_laser_arm");laser.model=G_NewString("@jar_laser_arm");CHECK(G_LaserArmSet(laser,jar::laserArm::Settings{}));selected=18;contextSelect();edits.clear();CHECK(vars["jar_contextLaserAllowed"]=="1"&&vars["jar_contextLaserDamage"]=="5");
 Run("laser",{"2","75","-30","90","0",".5","512"});CHECK(laser.count==2&&laser.pos2[1]==75&&laser.pos2[0]==-30&&laser.speed==90&&laser.damage==0&&laser.wait==.5f&&laser.radius==512);Revert();CHECK(laser.count==0&&laser.damage==5&&laser.radius==4096&&laser.pos2[0]==0&&laser.pos2[1]==0);
 for(const char*bad:{"-1","1001","1.5","nan","1;quit"}){Run("laser",{"0","0","0","30",bad,"3","4096"});CHECK(edits.empty()&&laser.damage==5);}selected=7;contextSelect();CHECK(vars["jar_contextLaserAllowed"]=="0");Run("laser",{"0","0","0","30","5","3","4096"});CHECK(edits.empty());
 {
  auto& emitter=g_entities[94];emitter={};emitter.inuse=qtrue;emitter.s.number=94;emitter.classname=G_NewString("target_laser");
  emitter.model=G_NewString(jar::sandboxLaser::model);
  CHECK(Own(state,94,91473820));emitter.targetname=G_NewString(va("jar_sb_%u",owned(94)->id));
  CHECK(G_SandboxLaserInitialize(emitter,{}));
  auto& aim=g_entities[95];aim.inuse=qtrue;aim.s.number=95;aim.classname=nullptr;aim.targetname=G_NewString("laser_aim");
  selected=94;contextSelect();
  CHECK(vars["jar_contextLaserEmitterAllowed"]=="1"&&vars["jar_contextLaserEmitterDamage"]=="1"&&vars["jar_contextLaserEmitterAim"]=="Aim: forward");
  for(const char* bad:{"-1","1001","1.5","nan","1;quit"}){Run("laser_emitter_damage",{bad});CHECK(edits.empty()&&emitter.damage==1);}
  Run("laser_emitter_damage",{"0"});CHECK(emitter.damage==0&&edits.size()==1);
  Run("laser_emitter_firing");CHECK(!emitter.nextthink&&vars["jar_contextLaserEmitterFiring"]=="0");
  Run("laser_emitter_enabled");CHECK((emitter.svFlags&SVF_INACTIVE)&&vars["jar_contextLaserEmitterEnabled"]=="0");
  Revert();Revert();Revert();CHECK(emitter.damage==1&&(emitter.spawnflags&1)&&!(emitter.svFlags&SVF_INACTIVE)&&edits.empty());
  editLaserEmitterAim(emitter,aim.targetname);contextPublish();CHECK(G_SandboxLaserAim(emitter)==&aim&&vars["jar_contextLaserEmitterAim"]=="Aim: 95 (entity)");
  Run("laser_emitter_unlink");CHECK(!emitter.target);
  Revert();CHECK(G_SandboxLaserAim(emitter)==&aim);
  CHECK(edits.empty());
  Run("laser_emitter_damage",{"42"});emitter.damage=43;
  Revert();CHECK(edits.size()==1&&emitter.damage==43);
  forgetEdits(94);Forget(state,94);emitter.inuse=aim.inuse=qfalse;
 }
 {
  using namespace jar::speaker;
  gentity_t& source=g_entities[94];source={};source.inuse=qtrue;source.s.number=94;
  source.classname=G_NewString("target_speaker");source.model=G_NewString(model);
  source.e_UseFunc=useF_Use_Target_Speaker;source.noise_index=5;
  CHECK(Own(state,94,85447538)&&G_SandboxSpeakerSet(source,{}));
  selected=94;contextSelect();CHECK(vars["jar_contextSpeakerAllowed"]=="1"&&vars["jar_contextSpeakerCue"]=="Bryar shot");
  Run("speaker_apply",{"499","0"});Run("speaker_apply",{"600","101"});CHECK(edits.empty());
  Run("speaker_apply",{"1500","300"});Run("speaker_mode");Run("speaker_enabled");
  CHECK(source.wait==1500&&source.random==300&&(source.svFlags&SVF_BROADCAST)&&!G_LogicUseEnabled(source));
  Revert();Revert();Revert();CHECK(edits.empty()&&source.wait==1000&&G_LogicUseEnabled(source));
  Forget(state,94);contextPublish();CHECK(vars["jar_contextSpeakerAllowed"]=="0");source.inuse=qfalse;
 }
 {
  using namespace jar::sandboxAngular;
  auto& mover=g_entities[94];mover={};mover.inuse=qtrue;mover.s.number=94;
  mover.classname=G_NewString("func_pendulum");mover.model=G_NewString(model);
  CHECK(Own(state,94,97415810)&&G_SandboxAngularInit(mover,{Pendulum,Forward,15,2000,500,true,true}));
  selected=94;contextSelect();CHECK(vars["jar_contextAngularAllowed"]=="1"&&vars["jar_contextAngularStatus"]=="Running");
  Run("angular_apply",{"76","1000"});Run("angular_apply",{"45","749"});CHECK(edits.empty());
  Run("angular_apply",{"45","1000"});CHECK(mover.count==250&&mover.speed==45);
  Run("angular_axis");Run("angular_running");Run("angular_enabled");CHECK(vars["jar_contextAngularStatus"]=="Disabled");
  Revert();Revert();Revert();mover.count=350;Revert();
  Settings s;CHECK(edits.empty()&&G_SandboxAngularRead(mover,s)&&s.axis==Forward&&s.running&&s.enabled&&s.phase==700&&s.rate==15);
  Forget(state,94);mover.inuse=qfalse;
 }
 {
  auto& station=g_entities[94];station.inuse=qtrue;station.s.number=94;station.classname=G_NewString("misc_ammo_floor_unit");
  station.e_UseFunc=useF_ammo_power_converter_use;station.count=50;
  CHECK(Own(state,94,89078252));station.targetname=G_NewString(va("jar_sb_%u",owned(94)->id));
  CHECK(G_SandboxStationInitialize(station,1));
  selected=94;contextSelect();CHECK(vars["jar_contextStationAllowed"]=="1"&&vars["jar_contextStationCharge"]=="50");
  Run("station_apply",{"0","100"});Run("station_apply",{"20","99"});Run("station_charge",{"51"});CHECK(edits.empty());
  Run("station_apply",{"40","200"});CHECK(station.max_health==40&&station.count==40&&station.delay==200);
  Run("station_charge",{"25"});CHECK(station.count==25&&station.nextthink>level.time);
  ++station.count;Revert();CHECK(station.count==40);
  Revert();CHECK(station.max_health==50&&station.count==50&&!station.delay);
  Run("station_charge",{"0"});Run("station_unlimited");Run("station_refill");Run("station_enabled");
  CHECK(station.count==50&&(station.spawnflags&jarStationUnlimited)&&(station.svFlags&SVF_INACTIVE));
  Revert();Revert();Revert();Revert();CHECK(station.count==50&&!(station.spawnflags&jarStationUnlimited)&&!(station.svFlags&SVF_INACTIVE));
  Forget(state,94);station.inuse=qfalse;
 }
 {
  using namespace jar::sandboxBreakable;
  auto& body=g_entities[94];body.inuse=qtrue;body.s.number=94;body.classname=G_NewString("func_breakable");
  body.spawnflags=body.flags=body.svFlags=0;body.e_ThinkFunc=thinkF_NULL;body.s.modelindex=5;
  body.max_health=body.health=40;body.radius=1;
  CHECK(Own(state,94,98415299));body.targetname=G_NewString(va("jar_sb_%u",owned(94)->id));
  CHECK(G_SandboxBreakableInitialize(body));
  selected=94;contextSelect();CHECK(vars["jar_contextBreakableAllowed"]=="1"&&vars["jar_contextHealthAllowed"]=="0");
  Run("breakable_apply",{"90","60001","150","20","100"});Run("health",{"80"});CHECK(edits.empty()&&body.health==40);
  Run("breakable_apply",{"90","250","150","20","100"});
  CHECK(body.max_health==90&&body.health==40&&body.delay==250&&body.splashDamage==20&&body.radius==1.5f);
  Run("breakable_mode");CHECK(body.damage==Saber&&(body.flags&FL_DMG_BY_SABER_ONLY));
  Run("breakable_use");CHECK(body.svFlags&SVF_PLAYER_USABLE);
  Run("breakable_enabled");CHECK(!body.takedamage&&!(body.svFlags&SVF_PLAYER_USABLE));
  Revert();Revert();Revert();Revert();CHECK(edits.empty()&&body.damage==Normal&&body.takedamage&&body.max_health==40);
  Settings stateBefore;
  CHECK(G_SandboxBreakableRead(body,stateBefore));
  stateBefore.phase=Broken;stateBefore.health=0;CHECK(G_SandboxBreakableSet(body,stateBefore));
  contextPublish();CHECK(vars["jar_contextBreakableStatus"]=="Broken");
  blocked=true;Run("breakable_repair");CHECK(edits.empty()&&G_SandboxBreakableBroken(body));
  blocked=false;Run("breakable_repair");CHECK(body.health==40&&body.takedamage&&body.contents&&edits.size()==1);
  Revert();CHECK(edits.empty()&&G_SandboxBreakableBroken(body)&&!body.contents);
  Forget(state,94);body.inuse=qfalse;
 }
 {
  using namespace jar::sandboxUsable;
  auto& source=g_entities[94];source.inuse=qtrue;source.s.number=94;source.classname=G_NewString("func_usable");
  source.model=G_NewString("@jar_box");source.spawnflags=source.svFlags=source.s.eFlags=0;
  source.e_DieFunc=dieF_func_usable_die;source.e_ThinkFunc=thinkF_NULL;source.s.modelindex=5;
  source.health=40;source.max_health=0;source.wait=1;
  CHECK(Own(state,94,87460708));source.targetname=G_NewString(va("jar_sb_%u",owned(94)->id));
  CHECK(G_SandboxUsableInitialize(source));
  selected=94;contextSelect();CHECK(vars["jar_contextUsableAllowed"]=="1"&&vars["jar_contextUsableCapacity"]=="40");
  Run("usable_apply",{"10001","500"});Run("usable_apply",{"60","60001"});CHECK(edits.empty());
  Run("usable_apply",{"60","500"});Run("usable_mode");Run("usable_damage");Run("usable_direct");
  CHECK(source.max_health==60&&source.delay==500&&(source.spawnflags&jarUsablePulse)&&source.damage==AtZero);
  CHECK(!(source.svFlags&SVF_PLAYER_USABLE));
  Revert();Revert();Revert();Revert();CHECK(edits.empty()&&source.max_health==40&&!(source.spawnflags&jarUsablePulse));
  Run("usable_solid");CHECK(G_SandboxUsableDormant(source)&&edits.size()==1);
  blocked=true;Revert();CHECK(edits.size()==1&&G_SandboxUsableDormant(source));
  blocked=false;Revert();CHECK(edits.empty()&&source.count==1&&source.contents);
  auto& target=g_entities[95];target.inuse=qtrue;target.s.number=95;target.classname=G_NewString("target_print");
  target.e_UseFunc=useF_Use_Target_Print;CHECK(Own(state,95,87615608));
  target.targetname=G_NewString(va("jar_sb_%u",owned(95)->id));
  editSlottedLink(source,1,target.targetname);CHECK(source.target2&&edits.empty());
  {const WorkshopActionScope context(recordingContextEdit);editSlottedLink(source,1,nullptr);}
  CHECK(!source.target2);Revert();CHECK(source.target2&&!strcmp(source.target2,target.targetname)&&edits.empty());
  Forget(state,94);Forget(state,95);source.inuse=target.inuse=qfalse;
 }
 auto&crate=g_entities[24];crate.inuse=qtrue;crate.s.number=24;crate.classname=G_NewString("misc_crystal_crate");crate.model=G_NewString("@jar_crystal_crate");CHECK(G_CrystalCrateSet(crate,jar::crystalCrate::Settings{}));selected=24;contextSelect();edits.clear();CHECK(vars["jar_contextCrystalAllowed"]=="1");Run("crystal",{"9","48.5","120","192","0"});CHECK(crate.damage==9&&crate.radius==48.5&&crate.splashDamage==120&&crate.splashRadius==192&&crate.contents==CONTENTS_SHOTCLIP);Revert();CHECK(crate.damage==16&&crate.radius==32&&crate.splashDamage==40&&!(crate.spawnflags&1));
 for(const char*bad:{"-1","1001","nan","9;quit"}){contextSelect();Run("crystal",{bad,"32","40","80","1"});CHECK(crate.damage==16&&edits.empty());}
 contextSelect();Run("crystal",{"16","32","40","80.5","1"});CHECK(crate.splashRadius==80&&edits.empty());
 contextSelect();Run("crystal",{"16","32","40","80","0"});CHECK(edits.size()==1);blocked=true;Revert();CHECK(edits.size()==1&&(crate.spawnflags&1));contextSelect();Run("crystal",{"16","32","40","80","1"});CHECK(edits.size()==1&&(crate.spawnflags&1));blocked=false;Revert();CHECK(edits.empty()&&!(crate.spawnflags&1));
 auto&door=g_entities[22];door.inuse=qtrue;door.s.number=22;door.classname=G_NewString("misc_model_jabba_cam");door.model=G_NewString("@jar_door_camera");CHECK(G_DoorCameraSet(door,jar::doorCamera::Settings{}));selected=22;contextSelect();edits.clear();CHECK(vars["jar_contextDoorAllowed"]=="1");Run("door",{"128","250","1"});CHECK(door.radius==128&&door.speed==250&&(door.spawnflags&1));Revert();CHECK(door.radius==48&&door.speed==120&&!(door.spawnflags&1));
 for(const char*bad:{"7","257","nan","1;quit"}){Run("door",{bad,"120","0"});CHECK(edits.empty()&&door.radius==48);}Run("door",{"48","120",".5"});CHECK(edits.empty());selected=7;contextSelect();Run("door",{"48","120","1"});CHECK(vars["jar_contextDoorAllowed"]=="0"&&edits.empty());
 auto&welder=g_entities[21];welder.inuse=qtrue;welder.s.number=21;welder.classname=G_NewString("misc_model_welder");welder.model=G_NewString("@jar_welder");CHECK(G_WelderSet(welder,jar::welder::Settings{}));selected=21;contextSelect();edits.clear();CHECK(vars["jar_contextWelderAllowed"]=="1"&&vars["jar_contextWelderRadius"]=="45.000");
 Run("welder",{"64","32.5",".5","0","0"});CHECK(welder.radius==64&&welder.random==32.5f&&welder.wait==.5f&&!welder.damage&&(welder.spawnflags&1));Revert();CHECK(welder.radius==15&&welder.random==45&&welder.wait==.2f&&welder.damage==10&&!(welder.spawnflags&1));
 for(const char*bad:{"0","257","nan","1;quit"}){Run("welder",{bad,"45",".2","10","1"});CHECK(edits.empty()&&welder.radius==15);}Run("welder",{"15","45",".2","1.5","1"});CHECK(edits.empty());selected=7;contextSelect();Run("welder",{"15","45",".2","10","1"});CHECK(vars["jar_contextWelderAllowed"]=="0"&&edits.empty());
 auto&spot=g_entities[20];spot.inuse=qtrue;spot.s.number=20;spot.classname=G_NewString("misc_spotlight");spot.model=G_NewString("@jar_spotlight");CHECK(G_SpotlightSet(spot,jar::spotlight::Settings{}));selected=20;contextSelect();edits.clear();CHECK(vars["jar_contextSpotAllowed"]=="1");
 Run("spot",{"1024","15","128",".2",".4",".6","0","0"});CHECK(spot.wait==1024&&spot.speed==15&&spot.random==128&&spot.pos1[1]==.4f&&spot.count==1&&(spot.spawnflags&1));Revert();CHECK(spot.wait==2048&&spot.speed==8&&spot.pos1[0]==1&&!spot.count&&!(spot.spawnflags&1));
 for(const char*bad:{"63","8193","nan","1;quit"}){Run("spot",{bad,"8","225","1","1","1","1","1"});CHECK(edits.empty()&&spot.wait==2048);}Run("spot",{"2048","8","225","1.1","1","1","1","1"});CHECK(edits.empty());selected=7;contextSelect();Run("spot",{"2048","8","225","1","1","1","1","1"});CHECK(vars["jar_contextSpotAllowed"]=="0"&&edits.empty());
 auto&portal=g_entities[86];portal.inuse=qtrue;portal.s.number=86;portal.classname=G_NewString("misc_portal_surface");portal.model=G_NewString("@jar_portal_surface");CHECK(G_PortalSet(portal,jar::portal::Settings{}));selected=86;contextSelect();edits.clear();CHECK(vars["jar_contextPortalAllowed"]=="1"&&vars["jar_contextPortalWidth"]=="128.000");Run("portal",{"256","512","1","0"});CHECK(portal.radius==256&&portal.random==512&&portal.count==1&&(portal.spawnflags&128));Revert();CHECK(portal.radius==128&&portal.random==192&&!portal.count&&!(portal.spawnflags&128));
 Run("portal",{"256","512","1","0"});portal.spawnflags&=~128;
 Revert();CHECK(edits.size()==1&&portal.radius==256);
 portal.spawnflags|=128;Revert();CHECK(edits.empty()&&portal.radius==128);
 Run("portal",{"128","192","0","1"});CHECK(edits.empty());
 {
  CHECK(Own(state,86,88142876)&&Own(state,96,97890286));
  portal.targetname=G_NewString(va("jar_sb_%u",owned(86)->id));
  auto& camera=g_entities[96];camera={};camera.inuse=qtrue;camera.s.number=96;
  camera.classname=G_NewString("misc_portal_camera");camera.model=G_NewString("@jar_portal_camera");
  camera.targetname=G_NewString(va("jar_sb_%u",owned(96)->id));
  CHECK(G_PortalSet(camera,jar::portal::Settings{}));
  contextSelect();CHECK(vars["jar_contextPortalLink"]=="Camera link: None"&&vars["jar_contextPortalCanUnlink"]=="0");
  editPortalLink(portal,&camera);contextSelect();CHECK(edits.size()==1&&edits.back().context&&portal.owner==&camera);
  CHECK(vars["jar_contextPortalLink"]=="Camera link: #96 (On)"&&vars["jar_contextPortalCanUnlink"]=="1");
  camera.spawnflags|=128;contextPortalLinkStatus(portal,true);CHECK(vars["jar_contextPortalLink"]=="Camera link: #96 (Off)");
  camera.freeAfterEvent=qtrue;contextSelect();CHECK(vars["jar_contextPortalLink"]=="Linked camera missing.");
  camera.freeAfterEvent=qfalse;contextSelect();
  Run("portal_unlink",{});CHECK(!portal.owner&&!portal.target&&vars["jar_contextPortalCanUnlink"]=="0");
  ++portalCameraLifetime;Revert();CHECK(!portal.target&&edits.size()==2);
  --portalCameraLifetime;Revert();CHECK(portal.owner==&camera&&edits.size()==1);
  Revert();CHECK(!portal.owner&&!portal.target&&edits.empty());

  selected=96;contextSelect();editPortalLink(camera,&portal);contextSelect();
  CHECK(vars["jar_contextPortalLink"]=="Aim link: #86");
  Run("portal_unlink",{});CHECK(!camera.target&&vars["jar_contextPortalLink"]=="Aim link: None");
  Revert();CHECK(camera.owner==&portal&&camera.target);
  Revert();CHECK(!camera.owner&&!camera.target&&edits.empty());
  Forget(state,96);Forget(state,86);camera={};portal.targetname=nullptr;
  selected=86;contextSelect();
 }
 for(const char*bad:{"15","1025","nan","1;quit"}){Run("portal",{bad,"192","0","1"});CHECK(edits.empty()&&portal.radius==128);}selected=7;contextSelect();CHECK(vars["jar_contextPortalAllowed"]=="0");Run("portal",{"128","192","0","1"});CHECK(edits.empty());
 // Laser firing is timed independently of the suspended native motion controller.
 selected=18;contextSelect();laser.nextthink=level.time+100;nativeBegin(laser,true);level.time+=1000;laser.nextthink=level.time+100;const int firingTick=laser.nextthink;nativeRelease(18,false);CHECK(laser.nextthink==firingTick);
 Run("position",{"10","20","30"});level.time+=500;laser.nextthink=level.time+100;const int undoTick=laser.nextthink;Revert();CHECK(laser.nextthink==undoTick);
 auto&zone=g_entities[16];zone.inuse=qtrue;zone.s.number=16;zone.classname=G_NewString("fx_wind_zone");zone.model=G_NewString("@jar_box");jar::volumeEntity::Initialize(zone);jar::weather::Settings zs;zs.kind=jar::weather::WindZone;zs.mode=0;G_WeatherSet(zone,zs);selected=16;contextSelect();edits.clear();CHECK(vars["jar_contextWeatherKind"]=="4"&&vars["jar_contextVolumeAllowed"]=="1");
 Run("volume",{"32","128","64"});Run("angles",{"0","90","0"});CHECK(std::fabs(zone.maxs[0]-64)<.001f&&std::fabs(zone.maxs[1]-16)<.001f);Run("weather",{"2","1","1",".5","1500","0"});CHECK(zone.speed==1500&&(zone.spawnflags&128));Revert();CHECK(zone.speed==500&&!(zone.spawnflags&128));Revert();Revert();CHECK(zone.currentAngles[YAW]==0&&zone.pos2[0]==256&&zone.pos2[2]==256&&edits.empty());
 zone.classname=G_NewString("misc_weather_zone");zs.kind=jar::weather::Exposure;zs.mode=0;G_WeatherSet(zone,zs);contextSelect();Run("weather",{"2","1","1",".5","500","1"});CHECK(edits.empty());Run("weather",{"1","1","1",".5","500","1"});CHECK((zone.spawnflags&7)==1);Revert();CHECK((zone.spawnflags&7)==0);
 // Holocron level editing changes only the saved item count; token reuse,
 // unsupported entities, out-of-range/fractional levels and Undo are checked.
 auto&holocron=g_entities[26];gitem_t holoItem{};holoItem.giType=IT_HOLOCRON;holoItem.giTag=FP_HEAL;
 holocron.inuse=qtrue;holocron.s.number=26;holocron.s.eType=ET_ITEM;holocron.item=&holoItem;holocron.count=1;selected=26;contextSelect();edits.clear();CHECK(vars["jar_contextHolocronAllowed"]=="1"&&vars["jar_contextHolocronLevel"]=="1");
 const auto holoToken=contextToken;Run("holocron",{"3"});CHECK(holocron.count==3&&edits.size()==1);Revert();CHECK(holocron.count==1&&edits.empty());
 args={"jar_sb","context","holocron",std::to_string(holoToken),"2"};contextCommand();CHECK(holocron.count==1&&edits.empty());
 for(const char*bad:{"0","4","-1","1.5","nan","1;quit"}){Run("holocron",{bad});CHECK(holocron.count==1&&edits.empty());}
 holocron.count=0;contextSelect();Run("holocron",{"2"});CHECK(holocron.count==2);Revert();CHECK(holocron.count==0);
 holocron.count=1;contextSelect();Run("holocron",{"3"});holoItem.giTag=FP_PUSH;Revert();CHECK(holocron.count==3&&edits.size()==1);forgetEdits(26);CHECK(edits.empty());
 contextSelect();const auto deletedToken=contextToken;forgetEdits(26);args={"jar_sb","context","holocron",std::to_string(deletedToken),"2"};contextCommand();CHECK(holocron.count==3);
 selected=7;contextSelect();CHECK(vars["jar_contextHolocronAllowed"]=="0");Run("holocron",{"2"});CHECK(edits.empty());
 auto&scene=g_entities[36];scene.inuse=qtrue;scene.s.number=36;scene.classname=G_NewString("misc_camera_track");scene.model=G_NewString("@jar_scene_camera");G_SceneCameraSet(scene,{});selected=36;contextSelect();edits.clear();
 CHECK(vars["jar_contextSceneAllowed"]=="1"&&vars["jar_contextSceneTrack"]=="1");const auto sceneToken=contextToken;
 Run("scene",{"128","90","2","0"});CHECK(scene.speed==128&&scene.radius==90&&scene.wait==2&&!(scene.spawnflags&1)&&edits.size()==1);Revert();CHECK(scene.speed==64&&scene.radius==180&&scene.wait==1&&(scene.spawnflags&1)&&edits.empty());
 args={"jar_sb","context","scene",std::to_string(sceneToken),"32","60","0","1"};contextCommand();CHECK(scene.speed==64&&edits.empty());
 for(const char*bad:{"-1","1025","nan","1;quit"}){Run("scene",{bad,"90","2","1"});CHECK(scene.speed==64&&edits.empty());}Run("scene",{"64","90","2","0.5"});CHECK(edits.empty());
 scene.classname=G_NewString("misc_camera_focus");contextSelect();CHECK(vars["jar_contextSceneTrack"]=="0");Run("scene",{"0","45","0","0"});CHECK(scene.radius==45);Revert();CHECK(scene.radius==180);
 auto&wp=g_entities[37];wp.inuse=qtrue;wp.s.number=37;wp.classname=G_NewString("waypoint");CHECK(G_WaypointSpawn(wp));selected=37;contextSelect();edits.clear();
 CHECK(vars["jar_contextWaypointAllowed"]=="1"&&vars["jar_contextWaypointRange"]=="512.000");const auto wpToken=contextToken;
 Run("waypoint",{"0","0"});CHECK(wp.wait==0&&(wp.svFlags&SVF_INACTIVE)&&edits.size()==1);Revert();CHECK(wp.wait==512&&!(wp.svFlags&SVF_INACTIVE)&&edits.empty());
 args={"jar_sb","context","waypoint",std::to_string(wpToken),"64","1"};contextCommand();CHECK(wp.wait==512&&edits.empty());
 for(const char*bad:{"-1","1025","nan","1;quit"}){Run("waypoint",{bad,"1"});CHECK(wp.wait==512&&edits.empty());}Run("waypoint",{"64",".5"});CHECK(edits.empty());
 Run("waypoint",{"1024","1"});CHECK(wp.wait==1024);Revert();CHECK(wp.wait==512);selected=7;contextSelect();CHECK(vars["jar_contextWaypointAllowed"]=="0");Run("waypoint",{"64","1"});CHECK(wp.wait==512&&edits.empty());

 auto&rail=g_entities[38];rail.inuse=qtrue;rail.s.number=38;rail.classname=G_NewString("rail_track");rail.model=G_NewString("@jar_rail");jar::rail::Settings rs;rs.kind=jar::rail::Track;CHECK(G_RailSet(rail,rs));selected=38;contextSelect();edits.clear();
 CHECK(vars["jar_contextRailAllowed"]=="1"&&vars["jar_contextRailKind"]=="1");const auto railToken=contextToken;
 Run("rail",{"1024","512","128","256","2","3","0","1"});CHECK(rail.radius==1024&&rail.random==512&&rail.mass==128&&rail.speed==256&&rail.wait==2&&rail.health==3&&(rail.spawnflags&2)&&(rail.spawnflags&1)&&edits.size()==1);Revert();CHECK(rail.radius==512&&rail.mass==64&&rail.health==1&&!(rail.spawnflags&2)&&edits.empty());
 args={"jar_sb","context","rail",std::to_string(railToken),"128","64","64","16","0","1","1","0"};contextCommand();CHECK(rail.radius==512&&edits.empty());
 for(const char*bad:{"127","8193","nan","1;quit"}){Run("rail",{bad,"256","64","128","0","1","1","0"});CHECK(rail.radius==512&&edits.empty());}
 Run("rail",{"512","64","128","128","0","1","1","0"});CHECK(edits.empty());Run("rail",{"512","256","64","128","0","1.5","1","0"});CHECK(edits.empty());Run("rail",{"512","256","64","128","0","1",".5","0"});CHECK(edits.empty());
 selected=7;contextSelect();CHECK(vars["jar_contextRailAllowed"]=="0");Run("rail",{"512","256","64","128","0","1","1","0"});CHECK(edits.empty());

 auto& panel=g_entities[39];panel.inuse=qtrue;panel.s.number=39;
 panel.classname=G_NewString("func_door");panel.model=G_NewString(JAR_SLIDING_DOOR_PORTABLE);
 CHECK(G_SandboxSlideSpawn(panel));selected=39;contextSelect();edits.clear();
 CHECK(vars["jar_contextSlideAllowed"]=="1"&&vars["jar_contextSlideTravel"]=="176");
 for(const char* bad:{"15","385","16.5","nan","1;quit"}){
  Run("slide",{bad,"100","0","0"});CHECK(edits.empty()&&panel.damage==176);
 }
 Run("slide",{"60","100","0","250"});CHECK(panel.damage==60&&panel.wait==250&&edits.size()==1);
 panel.target=G_NewString("opened");panel.target2=G_NewString("closed");
 portable_sliding_door_use(&panel,nullptr,&g_entities[0]);
 auto tick=[&](gentity_t& e,int ms){level.time+=ms;portable_sliding_door_think(&e);};
 tick(panel,100);CHECK(panel.count==10000&&panel.currentOrigin[2]==10&&panel.s.loopSound==32&&doorSounds==1);
 blocked=true;tick(panel,100);CHECK(panel.count==10000&&!panel.s.loopSound&&panel.fly_sound_debounce_time==JAR_SLIDE_BLOCKED);
 blocked=false;
 Run("slide",{"60","100","2","250"});CHECK(panel.bounceCount==0&&edits.size()==1);
 nativeBegin(panel,true);portable_sliding_door_use(&panel,nullptr,&g_entities[0]);CHECK(panel.alt_fire&&!panel.s.loopSound);
 level.time+=1000;nativeRelease(39,false);tick(panel,100);CHECK(panel.count==20000);
 for(int i=0;i<4;++i)tick(panel,100);
 CHECK(panel.count==60000&&panel.delay==250&&!panel.s.loopSound&&!strcmp(doorOutput,"opened"));
 tick(panel,100);CHECK(panel.delay==150);
 MemorySave doorSave;ojk::SavedGameHelper doorWriter(&doorSave);
 panel.sg_export(doorWriter);doorSave.reset_buffer_offset();
 gentity_t loadedPanel{};loadedPanel.sg_import(doorWriter);doorWriter.ensure_all_data_read();
 loadedPanel.classname=panel.classname;loadedPanel.model=panel.model;
 loadedPanel.target2=panel.target2;loadedPanel.activator=nullptr;
 jar::slidingDoor::State savedDoor;
 CHECK(loadedPanel.e_UseFunc==useF_portable_sliding_door_use&&loadedPanel.e_ThinkFunc==thinkF_portable_sliding_door_think);
 CHECK(G_SandboxSlideRead(loadedPanel,savedDoor)&&savedDoor.remainingWait==150&&savedDoor.extension==60000);
 CHECK(G_SandboxSlideRestore(loadedPanel,savedDoor)&&loadedPanel.currentOrigin[2]==60);
 tick(loadedPanel,100);CHECK(loadedPanel.delay==50&&loadedPanel.count==60000);
 tick(loadedPanel,50);CHECK(!loadedPanel.alt_fire&&loadedPanel.count==55000);
 for(int i=0;i<6;++i)tick(loadedPanel,100);
 CHECK(loadedPanel.count==0&&loadedPanel.currentOrigin[2]==0&&!strcmp(doorOutput,"closed")&&!loadedPanel.s.loopSound);
 Revert();CHECK(edits.empty()&&panel.damage==176&&panel.count==60000&&panel.currentOrigin[2]==60);
 panel.classname=G_NewString("func_door");panel.model=G_NewString("@jar_box");contextSelect();
 CHECK(vars["jar_contextSlideAllowed"]=="0");

 auto& button=g_entities[40];button.inuse=qtrue;button.s.number=40;
 button.classname=G_NewString("func_button");button.model=G_NewString(JAR_BUTTON_PORTABLE_MODEL);
 CHECK(G_ButtonSpawn(button));selected=40;contextSelect();
 CHECK(vars["jar_contextButtonAllowed"]=="1"&&vars["jar_contextButtonWait"]=="1000");
 Run("button",{"30001"});CHECK(edits.empty());
 Run("button",{"125"});CHECK(button.wait==125&&edits.size()==1);
 Run("button_enabled");CHECK(button.svFlags&SVF_INACTIVE);
 portable_button_use(&button,nullptr,&g_entities[0]);CHECK(button.count==JAR_BUTTON_IDLE);
 Revert();CHECK(!(button.svFlags&SVF_INACTIVE));
 button.target=G_NewString("pressed");button.target2=G_NewString("released");
 doorOutput=nullptr;portable_button_use(&button,nullptr,&g_entities[0]);
 level.time+=100;portable_button_think(&button);CHECK(button.s.frame==2&&!doorOutput);
 level.time+=100;portable_button_think(&button);CHECK(button.s.frame==4&&!strcmp(doorOutput,"pressed"));
 level.time+=80;portable_button_think(&button);CHECK(button.delay==45);
 MemorySave buttonSave;ojk::SavedGameHelper buttonWriter(&buttonSave);
 button.sg_export(buttonWriter);buttonSave.reset_buffer_offset();
 gentity_t loadedButton{};loadedButton.sg_import(buttonWriter);buttonWriter.ensure_all_data_read();
 loadedButton.classname=button.classname;loadedButton.model=button.model;
 loadedButton.target2=button.target2;loadedButton.activator=nullptr;
 jarButtonState_t buttonState;
 CHECK(G_ButtonRead(loadedButton,buttonState)&&buttonState.remaining==45&&buttonState.phase==JAR_BUTTON_HELD);
 CHECK(G_ButtonRestore(loadedButton,buttonState));
 level.time+=95;portable_button_think(&loadedButton);CHECK(loadedButton.s.frame==3&&loadedButton.count==JAR_BUTTON_RETURNING);
 for(int i=0;i<3;++i){level.time+=50;portable_button_think(&loadedButton);}
 CHECK(loadedButton.count==JAR_BUTTON_IDLE&&!strcmp(doorOutput,"released"));
 Revert();CHECK(button.wait==1000&&edits.empty());

 auto& deck=g_entities[60];deck.inuse=qtrue;deck.s.number=60;
 deck.classname=G_NewString("func_plat");deck.model=G_NewString(JAR_LIFT_PORTABLE);
 CHECK(G_SandboxSlideSpawn(deck));selected=60;contextSelect();
 CHECK(vars["jar_contextSlideLift"]=="1"&&vars["jar_contextSlideTravel"]=="128"&&vars["jar_contextSlideWait"]=="1000");
 Run("slide",{"30","100","1","0"});CHECK(edits.empty()&&deck.damage==128);
 Run("slide",{"30","100","0","0"});CHECK(deck.damage==30&&edits.size()==1);
 Run("slide_enabled");CHECK(deck.svFlags&SVF_INACTIVE);Revert();CHECK(!(deck.svFlags&SVF_INACTIVE));
 deck.target=G_NewString("raised");deck.target2=G_NewString("lowered");
 auto& rider=g_entities[61];gclient_t riderClient{};rider.inuse=qtrue;rider.s.number=61;rider.client=&riderClient;rider.contents=CONTENTS_BODY;
 rider.client->ps.groundEntityNum=60;VectorSet(rider.mins,-4,-4,-8);VectorSet(rider.maxs,4,4,8);
 const float top=deck.maxs[2];vec3_t riderOrigin={-12,0,top+8};G_SetOrigin(&rider,riderOrigin);
 VectorCopy(riderOrigin,riderClient.ps.origin);
 auto& cargo=g_entities[62];cargo.inuse=qtrue;cargo.s.number=62;cargo.classname=G_NewString("jar_sandbox_prop");cargo.contents=CONTENTS_SOLID;
 CHECK(Own(state,62,1001));owned(62)->frozen=0;
 vec3_t cargoOrigin={12,0,top+2};G_SetOrigin(&cargo,cargoOrigin);
 VectorSet(cargo.mins,-2,-2,-2);VectorSet(cargo.maxs,2,2,2);cargo.s.pos.trDelta[0]=8;
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int limit){
  CHECK(limit>=3);list[0]=&g_entities[60];list[1]=&g_entities[61];list[2]=&g_entities[62];return 3;
 };
 gi.linkentity=[](gentity_t* e){e->linked=qtrue;};gi.unlinkentity=[](gentity_t* e){e->linked=qfalse;};
 for(auto* e:{&deck,&rider,&cargo})gi.linkentity(e);
 gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
  CHECK(!g_entities[60].linked&&!g_entities[61].linked&&!g_entities[62].linked);
  *tr={};tr->fraction=blocked&&pass==61?.5f:1;VectorCopy(end,tr->endpos);
 };
 portable_sliding_door_use(&deck,nullptr,&rider);tick(deck,100);
 CHECK(deck.currentOrigin[2]==10&&rider.currentOrigin[2]==riderOrigin[2]+10&&cargo.currentOrigin[2]==cargoOrigin[2]+10);
 CHECK(riderClient.ps.origin[2]==rider.currentOrigin[2]&&cargo.s.pos.trDelta[0]==8);
 blocked=true;tick(deck,100);CHECK(deck.count==10000&&deck.fly_sound_debounce_time==JAR_SLIDE_BLOCKED);
 CHECK(deck.linked&&rider.linked&&cargo.linked&&rider.currentOrigin[2]==riderOrigin[2]+10&&cargo.currentOrigin[2]==cargoOrigin[2]+10);
 blocked=false;tick(deck,100);tick(deck,100);CHECK(deck.count==30000&&!strcmp(doorOutput,"raised"));
 portable_sliding_door_use(&deck,nullptr,&rider);
 for(int i=0;i<3;++i)tick(deck,100);
 CHECK(deck.count==0&&rider.currentOrigin[2]==riderOrigin[2]&&cargo.currentOrigin[2]==cargoOrigin[2]&&!strcmp(doorOutput,"lowered"));
 Revert();CHECK(deck.damage==128&&edits.empty());
 vehicleInfo_t liftInfo{};liftInfo.type=VH_WALKER;
 Vehicle_t liftVehicle{};
 auto& hull=g_entities[63];auto& droid=g_entities[64];gclient_t hullClient{},droidClient{};
 hull.client=&hullClient;droid.client=&droidClient;
 hull.s.number=63;droid.s.number=64;
 hull.inuse=droid.inuse=qtrue;hull.health=rider.health=droid.health=100;
 hull.contents=droid.contents=CONTENTS_BODY;
 hull.m_pVehicle=&liftVehicle;liftVehicle.m_pParentEntity=&hull;liftVehicle.m_pVehicleInfo=&liftInfo;
 liftVehicle.m_pOldPilot=liftVehicle.m_pPilot=&rider;liftVehicle.m_pDroidUnit=&droid;
 rider.s.m_iVehicleNum=droid.s.m_iVehicleNum=hull.s.number;
 hullClient.ps.groundEntityNum=deck.s.number;
 liftVehicle.m_LandTrace.entityNum=deck.s.number;liftVehicle.m_LandTrace.fraction=.1f;
 liftVehicle.m_LandTrace.endpos[2]=top;
 VectorSet(hull.mins,-4,-4,-8);VectorSet(hull.maxs,4,4,8);
 for(auto* e:{&hull,&droid}){
  vec3_t origin={0,0,top+(e==&hull?8.f:24.f)};G_SetOrigin(e,origin);VectorCopy(origin,e->client->ps.origin);
  e->client->ps.velocity[0]=7;e->client->ps.eventSequence=13;
 }
 gi.linkentity(&hull);
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int limit){
  CHECK(limit>=2);list[0]=&g_entities[61];list[1]=&g_entities[63];return 2;
 };
 gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
  for(int n:{60,61,63,64})CHECK(!g_entities[n].linked);
  *tr={};tr->fraction=blocked&&pass==64?.5f:1;VectorCopy(end,tr->endpos);
 };
 blocked=true;CHECK(!liftTranslate(deck,10));blocked=false;
 CHECK(deck.currentOrigin[2]==0&&hull.currentOrigin[2]==top+8&&droid.currentOrigin[2]==top+24);
 CHECK(deck.linked&&hull.linked&&rider.linked&&!droid.linked);
 liftVehicle.m_iBoarding=1;CHECK(!liftTranslate(deck,10));liftVehicle.m_iBoarding=0;
 jar::constraint::Joint deckLink;deckLink.kind=jar::constraint::Weld;
 deckLink.first=attachmentKey(hull);deckLink.second=jar::constraint::NativeId(ENTITYNUM_WORLD);
 const auto anchoredHull=AddJoint(state,deckLink);
 CHECK(anchoredHull&&!liftTranslate(deck,10)&&ForgetJoint(state,anchoredHull));
 deckLink.second=attachmentKey(deck);
 const auto linkedHull=AddJoint(state,deckLink);CHECK(linkedHull);
 CHECK(liftTranslate(deck,10)&&liftVehicle.m_LandTrace.endpos[2]==top+10);
 CHECK(deck.currentOrigin[2]==10&&hullClient.ps.origin[2]==top+18&&riderClient.ps.origin[2]==riderOrigin[2]+10&&droidClient.ps.origin[2]==top+34);
 CHECK(hullClient.ps.velocity[0]==7&&droidClient.ps.eventSequence==13&&!droid.linked);
 CHECK(liftTranslate(deck,-10)&&deck.currentOrigin[2]==0&&hullClient.ps.origin[2]==top+8&&droidClient.ps.origin[2]==top+24);
 CHECK(ForgetJoint(state,linkedHull));
 auto& bob=g_entities[65];bob.inuse=qtrue;bob.s.number=65;
 bob.classname=G_NewString("func_bobbing");bob.model=G_NewString(JAR_BOB_PORTABLE);
 gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){
  *tr={};tr->fraction=1;VectorCopy(end,tr->endpos);
 };
 CHECK(G_BobbingSpawn(bob)&&Own(state,65,JAR_BOB_ID));gi.linkentity(&bob);
 selected=65;contextSelect();CHECK(vars["jar_contextBobbingAllowed"]=="1"&&vars["jar_contextBobbingAmplitude"]=="12"&&vars["jar_contextBobbingPeriod"]=="4000");
 for(const char* bad:{"0","129","1.25","nan","1;quit"}){Run("bobbing",{bad,"1000"});CHECK(edits.empty());}
 Run("bobbing",{"24","499"});CHECK(edits.empty());
 Run("bobbing",{"24","1000"});CHECK(bob.speed==24&&bob.wait==1000&&edits.size()==1);
 auto& walker=g_entities[66];gclient_t walkerClient{};
 walker.inuse=qtrue;walker.s.number=66;walker.client=&walkerClient;walker.contents=CONTENTS_BODY;
 walkerClient.ps.groundEntityNum=65;
 VectorSet(walker.mins,-4,-4,-8);VectorSet(walker.maxs,4,4,8);
 vec3_t walkerStart={-9,0,bob.maxs[2]+8};G_SetOrigin(&walker,walkerStart);VectorCopy(walkerStart,walkerClient.ps.origin);gi.linkentity(&walker);
 vec3_t cargoStart={10,0,bob.maxs[2]+2};G_SetOrigin(&cargo,cargoStart);
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int limit){
  CHECK(limit>=3);list[0]=&g_entities[65];list[1]=&g_entities[66];list[2]=&g_entities[62];return 3;
 };
 auto bobTick=[](gentity_t& e,int ms){level.time+=ms;portable_bobbing_think(&e);};
 bobTick(bob,100);bobTick(bob,100);
 const float ascent=bob.currentOrigin[2];const int phase=bob.count;
 CHECK(ascent>16&&std::fabs(walkerClient.ps.origin[2]-walkerStart[2]-ascent)<.001f&&
       std::fabs(cargo.currentOrigin[2]-cargoStart[2]-ascent)<.001f);
 gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
  *tr={};tr->fraction=pass==66?.5f:1;VectorCopy(end,tr->endpos);
 };
 bobTick(bob,100);CHECK(bob.count==phase&&bob.currentOrigin[2]==ascent&&bob.fly_sound_debounce_time==JAR_BOB_BLOCKED);
 CHECK(bob.linked&&walker.linked&&cargo.linked);
 Revert();CHECK(bob.speed==12&&bob.wait==4000&&bob.count==phase&&bob.currentOrigin[2]==ascent);
 Run("bobbing_running");CHECK(!bob.alt_fire&&vars["jar_contextBobbingStatus"]=="Paused");
 bobTick(bob,100);CHECK(bob.count==phase&&bob.currentOrigin[2]==ascent);
 Run("bobbing_enabled");CHECK(bob.svFlags&SVF_INACTIVE);
 Revert();CHECK(!(bob.svFlags&SVF_INACTIVE)&&!bob.alt_fire);
 MemorySave bobSave;ojk::SavedGameHelper bobWriter(&bobSave);bob.sg_export(bobWriter);bobSave.reset_buffer_offset();
 auto& loadedBob=g_entities[67];loadedBob.sg_import(bobWriter);bobWriter.ensure_all_data_read();
 loadedBob.s.number=67;
 loadedBob.classname=bob.classname;loadedBob.model=bob.model;
 jarBobState_t savedBob{};
 CHECK(loadedBob.e_UseFunc==useF_portable_bobbing_use&&loadedBob.e_ThinkFunc==thinkF_portable_bobbing_think);
 CHECK(G_BobbingRead(loadedBob,savedBob)&&!savedBob.running&&savedBob.phase==phase&&loadedBob.currentOrigin[2]==ascent);
 CHECK(G_BobbingRestore(loadedBob,savedBob));
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t**,int){return 0;};
 gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){
  *tr={};tr->fraction=end[2]<start[2]?.5f:1;tr->entityNum=ENTITYNUM_WORLD;tr->plane.normal[2]=1;VectorCopy(end,tr->endpos);
 };
 savedBob.phase=JAR_BOB_CYCLE/2;savedBob.running=1;CHECK(G_BobbingRestore(loadedBob,savedBob));
 bobTick(loadedBob,100);CHECK(loadedBob.count==JAR_BOB_BOTTOM&&loadedBob.currentOrigin[2]==ascent);
 bobTick(loadedBob,100);CHECK(loadedBob.currentOrigin[2]>ascent&&loadedBob.fly_sound_debounce_time==JAR_BOB_MOVING);
 const int heldPhase=loadedBob.count;
 nativeBegin(loadedBob,true);
 bobTick(loadedBob,100);
 portable_bobbing_use(&loadedBob,nullptr,&walker);
 CHECK(loadedBob.count==heldPhase&&loadedBob.alt_fire);
 vec3_t newPlacement={20,0,80};G_SetOrigin(&loadedBob,newPlacement);setupMoved(loadedBob);
 nativeRelease(67,false);bobTick(loadedBob,100);
 CHECK(loadedBob.count!=heldPhase&&loadedBob.currentOrigin[2]>80&&loadedBob.s.pos.trType==TR_INTERPOLATE);
 portable_bobbing_use(&loadedBob,nullptr,&walker);
 CHECK(!loadedBob.alt_fire);
 gentity_t oldBob{};oldBob.inuse=qtrue;oldBob.classname=bob.classname;oldBob.model=G_NewString("@jar_box");
 oldBob.s.pos.trType=TR_SINE;oldBob.s.pos.trDuration=4000;oldBob.s.pos.trTime=900;
 oldBob.s.pos.trDelta[2]=12;oldBob.currentOrigin[2]=72;
 CHECK(G_BobbingMigrate(oldBob,oldBob.s.pos,1900)&&G_BobbingRead(oldBob,savedBob));
 CHECK(savedBob.phase==JAR_BOB_TOP&&savedBob.running&&oldBob.currentOrigin[2]==72&&oldBob.s.pos.trType==TR_INTERPOLATE);
 oldBob.model=G_NewString("*3");oldBob.bmodel=qtrue;
 CHECK(!G_BobbingMigrate(oldBob,oldBob.s.pos,1900)&&!G_BobbingPortable(oldBob));
 Revert();CHECK(bob.alt_fire&&edits.empty());
 {
  using namespace jar::sandboxAngular;
  auto& platform=g_entities[150];auto& propBody=g_entities[151];auto& carrier=g_entities[152];
  auto& pilot=g_entities[153];auto& unit=g_entities[154];
  const vec3_t center={0,0,60};
  for(auto* e:{&platform,&propBody,&carrier,&pilot,&unit}){
   e->inuse=qtrue;e->s.number=int(e-g_entities);e->health=100;e->contents=CONTENTS_BODY;
   VectorSet(e->mins,-4,-4,-4);VectorSet(e->maxs,4,4,4);gi.linkentity(e);
  }
  platform.classname=G_NewString("func_rotating");platform.model=G_NewString(model);
  G_SetOrigin(&platform,center);CHECK(G_SandboxAngularInit(platform,Defaults(Spinner)));
  CHECK(Own(state,150,88599552));platform.targetname=G_NewString(va("jar_sb_%u",owned(150)->id));
  const float height=center[2]+platform.maxs[2];
  const vec3_t propStart={-10,0,height+8.25f};
  G_SetOrigin(&propBody,propStart);propBody.classname=G_NewString("jar_sandbox_prop");
  VectorSet(propBody.mins,-8,-8,-8);VectorSet(propBody.maxs,8,8,8);
  CHECK(Own(state,151,1001));owned(151)->frozen=0;
  selected=151;contextSelect();
  CHECK(vars["jar_contextPropPhysicsAllowed"]=="1"&&vars["jar_contextPropMass"]=="0");
  edits.clear();Run("prop_physics",{"235.5"});CHECK(!owned(151)->physics.mass&&edits.empty());
  Run("prop_physics",{"235"});CHECK(owned(151)->physics.mass==235&&vars["jar_contextPropMass"]=="235");
  Run("prop_gravity");CHECK(owned(151)->physics.noGravity&&vars["jar_contextPropGravity"]=="0"&&!owned(151)->frozen);
  vec3_t fallTarget;VectorSet(propBody.s.pos.trDelta,12,0,0);
  nativePropFall(propBody,owned(151)->physics,.05f,fallTarget);
  CHECK(fallTarget[2]==propStart[2]&&fallTarget[0]>propStart[0]);
  Revert();CHECK(!owned(151)->physics.noGravity&&owned(151)->physics.mass==235);
  nativePropFall(propBody,owned(151)->physics,.05f,fallTarget);CHECK(fallTarget[2]<propStart[2]);
  Revert();CHECK(!owned(151)->physics.mass&&edits.empty());VectorClear(propBody.s.pos.trDelta);
  gclient_t mountedClients[3]{};vehicleInfo_t info{};info.type=VH_WALKER;Vehicle_t vehicle{};
  carrier.m_pVehicle=&vehicle;vehicle.m_pParentEntity=&carrier;vehicle.m_pVehicleInfo=&info;
  vehicle.m_pPilot=vehicle.m_pOldPilot=&pilot;vehicle.m_pDroidUnit=&unit;
  vehicle.m_LandTrace.entityNum=150;vehicle.m_LandTrace.fraction=.1f;vehicle.m_LandTrace.endpos[2]=height;
  pilot.owner=unit.owner=&carrier;pilot.s.m_iVehicleNum=unit.s.m_iVehicleNum=152;
  int i=0;for(auto* e:{&carrier,&pilot,&unit}){
   e->client=&mountedClients[i];e->client->ps.eventSequence=11;
   const vec3_t p={10.f+i,0,height+4.25f+10*i};G_SetOrigin(e,p);VectorCopy(p,e->client->ps.origin);++i;
  }
  carrier.client->ps.groundEntityNum=150;carrier.client->ps.velocity[0]=7;unit.linked=qfalse;
  CHECK(physicsWorld.Start({{-100,-100,-20},{100,-100,-20},{100,100,-20},{-100,-100,-20},{100,100,-20},{-100,100,-20}}));
  jar::physics::Pose pose;pose.position=pv(propStart);pose.linear={8,0,0};
  CHECK(physicsWorld.Prop(151,owned(151)->id,{8,8,8},{},pose,false));
  auto& det=g_entities[155];auto& mine=g_entities[156];
  const int previousEntityCount=globals.num_entities;globals.num_entities=157;
  for(auto* charge:{&det,&mine}){
   charge->inuse=qtrue;charge->s.number=int(charge-g_entities);charge->s.eType=ET_GENERAL;
   charge->s.eFlags=EF_MISSILE_STICK;charge->e_DieFunc=dieF_WP_ExplosiveDie;charge->contents=CONTENTS_SHOTCLIP;
   VectorSet(charge->mins,-1,-1,-1);VectorSet(charge->maxs,1,1,1);
   const vec3_t origin={-19,charge==&det?0.f:4.f,propStart[2]};G_SetOrigin(charge,origin);
   charge->classname=G_NewString(charge==&det?"detpack":"tripmine");
   charge->s.weapon=charge==&det?WP_DET_PACK:WP_TRIP_MINE;
   charge->e_ThinkFunc=charge==&det?thinkF_WP_Explode:thinkF_laserTrapThink;
   charge->nextthink=level.time+1234;VectorSet(charge->s.origin2,-21,origin[1],origin[2]);
   gi.linkentity(charge);G_SandboxChargeAttach(*charge,propBody);
  }
  mine.s.eFlags|=EF_FIRING;
  const int deadline=det.nextthink;
  static int cargoObstacle=-1,beamChecks=0;
  gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int limit){
   CHECK(limit>=4);for(int n=0;n<4;++n)list[n]=&g_entities[150+n];return 4;
  };
  gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision mode,int){
   if(mode==G2_RETURNONHIT){CHECK(pass==156&&g_entities[152].linked&&g_entities[155].linked&&!g_entities[151].linked);++beamChecks;}
   else for(int n:{151,152,153,154,155,156})CHECK(!g_entities[n].linked);
   *tr={};tr->fraction=pass==cargoObstacle?.5f:1;VectorCopy(end,tr->endpos);
  };
  const vec3_t destination={3,0,60},turn={0,90,0};
  AngularCargo cargo;
  const vec3_t straight={3,0,65};
  for(const bool lift:{true,false}){
   platform.classname=G_NewString(lift?"func_plat":"func_bobbing");
   platform.model=G_NewString(lift?JAR_LIFT_PORTABLE:JAR_BOB_PORTABLE);
   G_JAR_ProjectedDeckHull(platform,vec3_origin,platform.mins,platform.maxs);
   const float offset=platform.maxs[2]+center[2]-height;
   for(auto* body:{&propBody,&carrier,&pilot,&unit,&det,&mine})body->currentOrigin[2]+=offset;
   CHECK(angularCargoPlan(platform,straight,vec3_origin,cargo)&&cargo.members.size()==4&&cargo.charges.size()==2);
   for(const auto& member:cargo.members)CHECK(std::fabs(member.destination[2]-member.entity->currentOrigin[2]-5)<.001f);
   cargo={};
   for(auto* body:{&propBody,&carrier,&pilot,&unit,&det,&mine})body->currentOrigin[2]-=offset;
  }
  platform.classname=G_NewString("func_rotating");platform.model=G_NewString(model);G_SandboxAngularMoved(platform);
  cargoObstacle=154;CHECK(!angularCargoPlan(platform,destination,turn,cargo));cargoObstacle=-1;
  CHECK(platform.linked&&propBody.linked&&carrier.linked&&pilot.linked&&!unit.linked&&VectorCompare(propBody.currentOrigin,propStart));
  cargo={};cargoObstacle=155;CHECK(!angularCargoPlan(platform,destination,turn,cargo));cargoObstacle=-1;
  CHECK(det.linked&&mine.linked&&det.currentOrigin[0]==-19&&!beamChecks);
  jar::constraint::Joint anchor;anchor.kind=jar::constraint::Weld;
  anchor.first=attachmentKey(propBody);anchor.second=jar::constraint::NativeId(ENTITYNUM_WORLD);
  const auto joint=AddJoint(state,anchor);cargo={};CHECK(joint&&!angularCargoPlan(platform,destination,turn,cargo)&&ForgetJoint(state,joint));
  cargo={};CHECK(angularCargoPlan(platform,destination,turn,cargo)&&cargo.members.size()==4&&cargo.rigid.size()==1&&cargo.charges.size()==2);
  {
   AngularCargoUnlinked unlinked(cargo);
   G_SetOrigin(&platform,destination);G_SetAngles(&platform,turn);G_SandboxAngularMoved(platform);
   cargoObstacle=156;CHECK(!angularCargoCommit(platform,cargo));cargoObstacle=-1;
   CHECK(VectorCompare(propBody.currentOrigin,propStart)&&physicsWorld.Read(151,pose)&&pose.position.x==-10&&carrier.currentOrigin[0]==10&&det.currentOrigin[0]==-19);
   CHECK(angularCargoCommit(platform,cargo));
  }
  G_SandboxChargesRefresh(cargo.charges);
  CHECK(physicsWorld.Read(151,pose)&&std::fabs(pose.position.x-3)<.001f&&std::fabs(pose.position.y+10)<.001f&&
        std::fabs(pose.angles.y-90)<.001f&&pose.linear.x==8);
  CHECK(std::fabs(carrier.currentOrigin[0]-3)<.001f&&std::fabs(carrier.currentOrigin[1]-10)<.001f&&
        std::fabs(pilot.currentOrigin[1]-11)<.001f&&std::fabs(unit.currentOrigin[1]-12)<.001f);
  CHECK(vehicle.m_vOrientation[YAW]==90&&carrier.client->ps.velocity[0]==7&&unit.client->ps.eventSequence==11&&
        propBody.linked&&carrier.linked&&pilot.linked&&!unit.linked&&pilot.owner==&carrier);
  CHECK(beamChecks==1&&det.s.groundEntityNum==151&&det.nextthink==deadline&&!(mine.s.eFlags&EF_TELEPORT_BIT)&&
        std::fabs(det.currentOrigin[0]-3)<.001f&&std::fabs(det.currentOrigin[1]+19)<.001f&&mine.movedir[1]>.999f);
  const vec3_t tilt={0,90,20};cargo={};
  CHECK(angularCargoPlan(platform,destination,tilt,cargo)&&cargo.members.size()==4);
  {
   AngularCargoUnlinked unlinked(cargo);
   G_SetAngles(&platform,tilt);G_SandboxAngularMoved(platform);
   CHECK(angularCargoCommit(platform,cargo));
  }
  G_SandboxChargesRefresh(cargo.charges);
  CHECK(std::fabs(vehicle.m_vOrientation[ROLL])<.001f&&std::fabs(propBody.currentAngles[ROLL]-20)<.001f&&
        std::fabs(carrier.currentOrigin[2]+carrier.mins[2]-platform.currentOrigin[2]-platform.maxs[2]-.25f)<.001f);
  const float priorZ=mine.currentOrigin[2];
  cargoObstacle=155;CHECK(!liftTranslate(platform,5)&&mine.currentOrigin[2]==priorZ&&platform.currentOrigin[2]==60);cargoObstacle=-1;
  CHECK(liftTranslate(platform,5)&&std::fabs(mine.currentOrigin[2]-priorZ-5)<.001f&&platform.currentOrigin[2]==65&&
        physicsWorld.Read(151,pose)&&std::fabs(pose.position.z-propBody.currentOrigin[2])<.001f&&beamChecks==3&&det.nextthink==deadline);

  // Exact edits keep mounted charges and support in the same transaction.
  static bool blockChargeSweep=false,slideChargeContact=false;
  gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t maxs,const vec3_t end,int pass,int,EG2_Collision mode,int){
   if(mode==G2_RETURNONHIT){
    CHECK(pass==156&&!g_entities[151].linked&&g_entities[155].linked&&g_entities[156].linked);++beamChecks;
   }else if(pass==155||pass==156){
    CHECK(!g_entities[151].linked&&!g_entities[155].linked&&!g_entities[156].linked);
   }
   *tr={};
   tr->fraction=(pass==cargoObstacle||(blockChargeSweep&&pass==155&&!VectorCompare(start,end)))?.5f:1;
   if(slideChargeContact&&pass==155&&end[2]<start[2]-.001f){tr->fraction=.5f;tr->plane.normal[2]=1;}
   if(blockChargeSweep&&pass==155&&!VectorCompare(start,end)&&g_entities[151].currentAngles[YAW]==90)
    CHECK(maxs[0]>1&&maxs[1]>1);
   VectorCopy(end,tr->endpos);
  };
  selected=151;contextSelect();CHECK(contextEntity==151&&edits.empty());
  vec3_t priorProp,priorDet,priorAngles;
  VectorCopy(propBody.currentOrigin,priorProp);VectorCopy(det.currentOrigin,priorDet);VectorCopy(propBody.currentAngles,priorAngles);
  cargoObstacle=155;Run("position",{"100","200","300"});
  CHECK(edits.empty()&&VectorCompare(propBody.currentOrigin,priorProp)&&VectorCompare(det.currentOrigin,priorDet)&&
        propBody.linked&&det.linked&&mine.linked&&beamChecks==3);
  cargoObstacle=-1;Run("position",{"100","200","300"});
  CHECK(edits.size()==1&&propBody.currentOrigin[0]==100&&std::fabs(det.currentOrigin[1]-191)<.001f&&
        beamChecks==4&&det.nextthink==deadline&&!physicsWorld.Read(151,pose));
  cargoObstacle=156;Revert();CHECK(edits.size()==1&&propBody.currentOrigin[0]==100&&beamChecks==4);
  cargoObstacle=-1;Revert();
  CHECK(edits.empty()&&VectorCompare(propBody.currentOrigin,priorProp)&&Distance(det.currentOrigin,priorDet)<.001f&&beamChecks==5);
  pose.position=pv(priorProp);pose.angles=pv(priorAngles);
  CHECK(physicsWorld.Prop(151,owned(151)->id,{8,8,8},{},pose,false));
  cargoObstacle=151;Run("angles",{"0","180","0"});
  CHECK(edits.empty()&&VectorCompare(propBody.currentAngles,priorAngles)&&beamChecks==5&&
        physicsWorld.Read(151,pose)&&std::fabs(pose.angles.y-90)<.001f);
  cargoObstacle=-1;Run("angles",{"0","180","0"});
  CHECK(edits.size()==1&&propBody.currentAngles[YAW]==180&&std::fabs(det.currentOrigin[0]-priorProp[0]-9)<.001f&&beamChecks==6);
  Revert();CHECK(edits.empty()&&VectorCompare(propBody.currentAngles,priorAngles)&&beamChecks==7);

  physicsWorld.Stop();
  held=151;args={"jar_sb","rotateheld",std::to_string(holdToken),"90","0"};
  CHECK(physgunGrip.Begin(pv(priorProp),pv(priorAngles),pv(priorProp),0));
  rotateHeld();
  auto bodyGeometry=std::make_shared<jar::physics::PropGeometry>();
  for(float x:{-8.f,8.f})for(float y:{-3.f,3.f})for(float z:{-2.f,2.f})bodyGeometry->Add({x,y,z});
  clearanceGeometry=bodyGeometry;
  static int rotationChecks=0,stopRotationAt=4;
  gi.JAR_TraceMovingHull=[](trace_t* tr,const vec3_t start,const vec3_t end,const vec3_t low,const vec3_t high,
     int pass,int mask,const JarSweepHull& shape){
   CHECK(pass==151&&VectorCompare(start,end)&&g_entities[151].currentAngles[YAW]==90);
   CHECK(mask==(MASK_SOLID|CONTENTS_BODY|SANDBOX_MOUNTED_CHARGE_CONTENTS));
   CHECK(shape.pointCount==16&&shape.arcRadius>0&&std::fabs(shape.arcAxis[2])>.999f);
   CHECK(!g_entities[155].linked&&!g_entities[156].linked&&low[2]< -1.99f&&high[2]>1.99f);
   *tr={};tr->fraction=1;
   if(++rotationChecks==stopRotationAt)tr->startsolid=tr->allsolid=qtrue;
  };
  physgunNativeTurn(propBody,1);
  CHECK(rotationChecks==4&&VectorCompare(propBody.currentAngles,priorAngles)&&
        VectorCompare(det.currentOrigin,priorDet)&&det.linked&&mine.linked&&beamChecks==7);
  stopRotationAt=0;rotationChecks=0;
  blockChargeSweep=true;physgunNativeTurn(propBody,1);
  CHECK(rotationChecks>=18&&VectorCompare(propBody.currentAngles,priorAngles)&&
        Distance(det.currentOrigin,priorDet)<.001f&&beamChecks==7);
  blockChargeSweep=false;physgunNativeTurn(propBody,1);
  CHECK(std::fabs(AngleSubtract(propBody.currentAngles[YAW],180))<.001f&&std::fabs(det.currentOrigin[0]-priorProp[0]-9)<.001f&&beamChecks==8);
  gi.JAR_TraceMovingHull=nullptr;clearanceGeometry.reset();
  vec3_t drag;VectorCopy(propBody.currentOrigin,drag);drag[0]+=20;
  std::vector<SandboxChargeMove> draggedCharges;
  trace_t dragHit{};bool chargeContact=false;
  blockChargeSweep=true;CHECK(G_SandboxPropTrace(propBody,drag,dragHit,draggedCharges,chargeContact));
  CHECK(chargeContact&&dragHit.fraction==.5f&&std::fabs(dragHit.endpos[0]-priorProp[0]-10)<.001f);
  CHECK(VectorCompare(propBody.currentOrigin,priorProp)&&beamChecks==8&&det.linked&&mine.linked);
  draggedCharges.clear();blockChargeSweep=false;
  CHECK(G_SandboxPropTrace(propBody,drag,dragHit,draggedCharges,chargeContact)&&!chargeContact&&dragHit.fraction==1);
  CHECK(draggedCharges.size()==2);
  G_SetOrigin(&propBody,drag);gi.linkentity(&propBody);placeEditedCharges(draggedCharges);
  CHECK(std::fabs(det.currentOrigin[0]-drag[0]-9)<.001f&&beamChecks==9&&
        det.nextthink==deadline&&!(mine.s.eFlags&EF_TELEPORT_BIT));
  releaseHeld();
  slideChargeContact=true;
  VectorSet(propBody.s.pos.trDelta,400,0,-400);
  vec3_t slideTarget;VectorMA(propBody.currentOrigin,.05f,propBody.s.pos.trDelta,slideTarget);
  const float preSlideZ=propBody.currentOrigin[2];
  advanceNativeProp(propBody,slideTarget,.05f);
  CHECK(std::fabs(propBody.currentOrigin[0]-slideTarget[0])<.001f&&
        std::fabs(propBody.currentOrigin[2]-preSlideZ+10)<.001f&&propBody.s.pos.trDelta[2]==0&&
        propBody.s.pos.trDelta[0]==400&&nativeImpact==400&&nativePushes==2&&beamChecks==11);
  CHECK(nativePropSegments.size()==2&&nativePropSegments[0]&&!nativePropSegments[1]);
  VectorSet(propBody.s.pos.trDelta,400,0,-400);
  const float beforePortalX=propBody.currentOrigin[0];
  VectorMA(propBody.currentOrigin,.05f,propBody.s.pos.trDelta,slideTarget);
  nativePropSegments.clear();nativePortal=true;nativeImpact=0;
  advanceNativeProp(propBody,slideTarget,.05f);
  CHECK(nativePropSegments.size()==1&&nativePropSegments[0]&&nativePushes==2&&nativeImpact==0&&
        std::fabs(propBody.currentOrigin[0]-beforePortalX-10)<.001f&&beamChecks==12&&det.nextthink==deadline);
  const auto contactTrace=gi.trace;
  static bool poseChargeBlocked=true;
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision mode,int){
   CHECK(VectorCompare(start,end)||mode==G2_RETURNONHIT);
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(mode==G2_RETURNONHIT){CHECK(pass==156&&!g_entities[151].linked);++beamChecks;return;}
   if(pass==151||pass==155||pass==156){
    CHECK(!g_entities[155].linked&&!g_entities[156].linked);
    if(pass==155&&poseChargeBlocked){CHECK(!g_entities[151].linked);hit->startsolid=qtrue;hit->fraction=0;}
   }
  };
  vec3_t beforePoseDet;VectorCopy(det.currentOrigin,beforePoseDet);
  selected=151;contextSelect();Run("pose",{"4"});
  CHECK(propBody.s.frame==0&&edits.empty()&&det.linked&&mine.linked&&beamChecks==12);
  poseChargeBlocked=false;Run("pose",{"4"});
  CHECK(propBody.s.frame==4&&edits.size()==1&&det.linked&&mine.linked&&beamChecks==13&&
        VectorCompare(det.currentOrigin,beforePoseDet)&&det.nextthink==deadline);
  Revert();CHECK(!propBody.s.frame&&edits.empty()&&beamChecks==14&&VectorCompare(det.currentOrigin,beforePoseDet));
  gi.trace=contactTrace;
  slideChargeContact=false;nativePortal=false;contextClear();
  G_SandboxChargesReset();globals.num_entities=previousEntityCount;
  Forget(state,150);Forget(state,151);
 }
 {
  auto& deck=g_entities[164];auto& rack=g_entities[165];
  auto& shelf=g_entities[166];auto& loose=g_entities[167];
  const int previousCount=globals.num_entities;globals.num_entities=168;
  const auto link=gi.linkentity,unlink=gi.unlinkentity;
  const auto trace=gi.trace;
  const auto entitiesInBox=gi.EntitiesInBox;
  gi.linkentity=[](gentity_t* e){e->linked=qtrue;};
  gi.unlinkentity=[](gentity_t* e){e->linked=qfalse;};
  for(auto* e:{&deck,&rack,&shelf,&loose}){
   *e={};e->inuse=qtrue;e->s.number=int(e-g_entities);gi.linkentity(e);
   e->s.groundEntityNum=ENTITYNUM_NONE;
  }
  deck.classname=G_NewString("func_plat");deck.model=G_NewString(JAR_LIFT_PORTABLE);deck.contents=CONTENTS_SOLID;
  const vec3_t deckStart={0,0,80};G_SetOrigin(&deck,deckStart);
  G_JAR_ProjectedDeckHull(deck,vec3_origin,deck.mins,deck.maxs);
  const float top=deck.currentOrigin[2]+deck.maxs[2];
  rack.classname=G_NewString("misc_model_ammo_rack");
  rack.contents=CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;
  const vec3_t rackStart={4,0,top+.25f};G_SetOrigin(&rack,rackStart);
  VectorSet(rack.mins,-6,-6,0);VectorSet(rack.maxs,6,6,24);
  CHECK(Own(state,165,98056028));rack.targetname=G_NewString(va("jar_sb_%u",owned(165)->id));
  gitem_t ammo{};
  for(auto* item:{&shelf,&loose}){
   item->item=&ammo;item->classname=G_NewString("ammo_blaster");item->s.eType=ET_ITEM;
   item->flags=FL_FORCE_PULLABLE_ONLY;item->contents=CONTENTS_ITEM;
   VectorSet(item->mins,-2,-2,-2);VectorSet(item->maxs,2,2,2);
   const vec3_t start={6,item==&shelf?1.f:-1.f,rackStart[2]+12};G_SetOrigin(item,start);
   CHECK(Own(state,item->s.number,2501));
   auto* itemRecord=owned(item->s.number);CHECK(SetOwnershipParent(state,*itemRecord,owned(165)->id));
   item->targetname=G_NewString(va("jar_sb_%u",itemRecord->id));
  }
  rackStockLoose(loose);CHECK(rackStockAttached(shelf)&&!rackStockAttached(loose));
  vec3_t looseStart;VectorCopy(loose.currentOrigin,looseStart);
  // Stock may appear in any order among the engine's trigger candidates.
  gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** list,int limit){
   CHECK(limit>=4);
   list[0]=&g_entities[166];list[1]=&g_entities[165];list[2]=&g_entities[164];list[3]=&g_entities[167];
   return 4;
  };
  static bool obstructShelf=false;
  static unsigned shelfChecks=0;
  gi.trace=[](trace_t* hit,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(pass==166){
    CHECK(!g_entities[165].linked&&!g_entities[166].linked&&g_entities[167].linked);
    ++shelfChecks;if(obstructShelf)hit->fraction=.5f;
   }
  };
  const float stockStartZ=shelf.currentOrigin[2];
  obstructShelf=true;CHECK(!liftTranslate(deck,6));
  CHECK(deck.currentOrigin[2]==80&&VectorCompare(rack.currentOrigin,rackStart)&&shelf.currentOrigin[2]==stockStartZ&&
        deck.linked&&rack.linked&&shelf.linked&&shelfChecks==1);
  obstructShelf=false;CHECK(liftTranslate(deck,6));
  CHECK(deck.currentOrigin[2]==86&&rack.currentOrigin[2]==rackStart[2]+6&&shelf.currentOrigin[2]==stockStartZ+6&&
        VectorCompare(loose.currentOrigin,looseStart)&&shelfChecks==2);

  const vec3_t destination={10,0,86},turn={0,90,0};
  AngularCargo cargo;
  nativeBegin(rack,true);
  CHECK(!liftTranslate(deck,4)&&!angularCargoPlan(deck,destination,turn,cargo));
  nativeRelease(165,false);
  obstructShelf=true;CHECK(!angularCargoPlan(deck,destination,turn,cargo));
  CHECK(rack.currentOrigin[0]==4&&shelf.currentOrigin[0]==6&&rack.linked&&shelf.linked);
  cargo={};obstructShelf=false;
  CHECK(angularCargoPlan(deck,destination,turn,cargo)&&cargo.members.size()==2);
  CHECK(angularCargoPairClear(cargo.members[0],cargo.members[1]));
  auto other=angularCargoMember(loose,ENTITYNUM_NONE);loose.contents=CONTENTS_BODY;
  VectorCopy(cargo.members[1].destination,other.destination);
  CHECK(!angularCargoPairClear(other,cargo.members[1])&&!angularCargoPairClear(other,cargo.members[0]));
  loose.contents=CONTENTS_ITEM;
  {
   AngularCargoUnlinked unlinked(cargo);
   G_SetOrigin(&deck,destination);G_SetAngles(&deck,turn);
   G_JAR_ProjectedDeckHull(deck,turn,deck.mins,deck.maxs);gi.linkentity(&deck);
   obstructShelf=true;CHECK(!angularCargoCommit(deck,cargo));
   CHECK(rack.currentOrigin[0]==4&&shelf.currentOrigin[0]==6);
   obstructShelf=false;CHECK(angularCargoCommit(deck,cargo));
  }
  const vec3_t expectedRack={10,4,rackStart[2]+6},expectedShelf={9,6,stockStartZ+6};
  CHECK(Distance(rack.currentOrigin,expectedRack)<.001f&&Distance(shelf.currentOrigin,expectedShelf)<.001f&&
        std::fabs(shelf.currentAngles[YAW]-90)<.001f&&rackStockAttached(shelf)&&VectorCompare(loose.currentOrigin,looseStart));
  for(auto* e:{&deck,&rack,&shelf,&loose}){Forget(state,e->s.number);*e={};}
  globals.num_entities=previousCount;gi.trace=trace;gi.linkentity=link;gi.unlinkentity=unlink;gi.EntitiesInBox=entitiesInBox;
 }
 {
  const auto savedImports=gi;
  const int savedCount=globals.num_entities;
  globals.num_entities=170;
  auto& deck=g_entities[168];
  auto& propBody=g_entities[169];
  gi.linkentity=[](gentity_t* e){e->linked=qtrue;};
  gi.unlinkentity=[](gentity_t* e){e->linked=qfalse;};
  gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t** entities,int limit){
   CHECK(limit>0);
   entities[0]=&g_entities[169];
   return 1;
  };
  for(auto* e:{&deck,&propBody}){
   *e={};e->inuse=qtrue;e->s.number=int(e-g_entities);gi.linkentity(e);
  }
  deck.classname=G_NewString("func_plat");deck.model=G_NewString(JAR_LIFT_PORTABLE);
  deck.contents=CONTENTS_SOLID;
  const vec3_t deckStart={0,0,80};
  G_SetOrigin(&deck,deckStart);
  G_JAR_ProjectedDeckHull(deck,vec3_origin,deck.mins,deck.maxs);
  const vec3_t propStart={-10,0,80+deck.maxs[2]+8.25f};
  propBody.classname=G_NewString("jar_sandbox_prop");propBody.contents=CONTENTS_BODY;
  propBody.s.groundEntityNum=168;
  VectorSet(propBody.mins,-8,-8,-8);VectorSet(propBody.maxs,8,8,8);
  G_SetOrigin(&propBody,propStart);
  CHECK(Own(state,169,1001));
  owned(169)->frozen=0;
  auto thin=std::make_shared<jar::physics::PropGeometry>();
  for(float x:{-7.f,7.f})for(float y:{-.5f,.5f})for(float z:{-8.f,8.f})
   thin->Add({(x-y)*.7071068f,(x+y)*.7071068f,z});
  clearanceGeometry=thin;
  static jar::physics::NativePropHull obstacleHull;
  jar::physics::PropGeometry obstacle;
  for(float x:{-.25f,.25f})for(float y:{-.25f,.25f})for(float z:{-.25f,.25f})obstacle.Add({x,y,z});
  CHECK(obstacleHull.Build(obstacle,{}));
  static vec3_t obstacleOrigin;
  static int arcsChecked;
  static bool boxBlocks;
  gi.trace=[](trace_t* tr,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
   *tr={};tr->fraction=boxBlocks&&pass==169?0:1;tr->startsolid=tr->fraction==0?qtrue:qfalse;VectorCopy(end,tr->endpos);
  };
  const auto convexTrace=[](trace_t* tr,const vec3_t start,const vec3_t end,const vec3_t low,const vec3_t high,
      int pass,int mask,const JarSweepHull& shape){
   CHECK(pass==169&&(mask&CONTENTS_SOLID)&&!g_entities[169].linked);
   CHECK(low[0]<high[0]&&low[1]<high[1]&&low[2]<high[2]);
   arcsChecked+=shape.arcRadius>0;
   vec3_t from,to;VectorSubtract(start,obstacleOrigin,from);VectorSubtract(end,obstacleOrigin,to);
   const auto hit=obstacleHull.TraceHull(pv(from),pv(to),shape);
   *tr={};tr->fraction=hit.fraction;tr->startsolid=hit.startSolid?qtrue:qfalse;tr->allsolid=hit.allSolid?qtrue:qfalse;
   VectorCopy(end,tr->endpos);
  };
  // A diagonal prop at the deck corner can overlap in both broad axes
  // while its complete footprint misses the deck.
  const vec3_t emptyCorner={deck.maxs[0]+4,deck.mins[1]-4,propStart[2]};
  G_SetOrigin(&propBody,emptyCorner);
  const auto platformBody=liftBody(deck);
  auto contactBody=liftBody(propBody);
  CHECK(JAR_LiftSupported(&platformBody,&contactBody,true));
  CHECK(!platformPropSupported(platformBody,contactBody,propBody,3));
  const vec3_t supportTurn={0,90,0};
  AngularCargo empty;
  CHECK(angularCargoPlan(deck,deck.currentOrigin,supportTurn,empty)&&empty.members.empty());
  CHECK(liftTranslate(deck,4)&&deck.currentOrigin[2]==84&&VectorCompare(propBody.currentOrigin,emptyCorner));
  CHECK(liftTranslate(deck,-4)&&deck.currentOrigin[2]==80&&VectorCompare(propBody.currentOrigin,emptyCorner));
  const vec3_t partial={deck.maxs[0]+4,0,propStart[2]};
  G_SetOrigin(&propBody,partial);
  contactBody=liftBody(propBody);
  CHECK(platformPropSupported(platformBody,contactBody,propBody,1));
  G_SetOrigin(&propBody,propStart);
  // The first obstacle lies inside the old AABB and outside the model.
  VectorSet(obstacleOrigin,propStart[0]+4,propStart[1]-4,propStart[2]+10);
  boxBlocks=true;
  CHECK(!liftTranslate(deck,4)&&VectorCompare(propStart,propBody.currentOrigin));
  gi.JAR_TraceMovingHull=convexTrace;
  CHECK(liftTranslate(deck,4)&&propBody.currentOrigin[2]==propStart[2]+4);
  VectorSet(obstacleOrigin,propStart[0],propStart[1],propStart[2]+14);
  CHECK(!liftTranslate(deck,4)&&propBody.currentOrigin[2]==propStart[2]+4&&deck.currentOrigin[2]==84);
  CHECK(deck.linked&&propBody.linked);
  boxBlocks=false;
  vec3_t destination;VectorCopy(deck.currentOrigin,destination);
  const vec3_t turn={0,90,0};
  AngularCargo clear;
  VectorSet(obstacleOrigin,100,100,100);
  CHECK(angularCargoPlan(deck,destination,turn,clear)&&clear.members.size()==1);
  const auto& planned=clear.members[0];
  CHECK(planned.geometry==thin);
  auto fixed=planned,moving=planned;
  moving.destination[0]+=3;
  CHECK(!angularCargoBoxesDisjoint(fixed,moving)&&angularCargoHullsDisjoint(fixed,moving));
  moving.destination[0]-=3;
  CHECK(!angularCargoHullsDisjoint(fixed,moving));
  // Both endpoints fit. The prop itself hits the obstacle halfway around the deck.
  VectorSet(obstacleOrigin,-7.071f,-7.071f,propBody.currentOrigin[2]);
  {
   AngularCargoUnlinked unlinked(clear);
   auto start=planned;
   VectorCopy(propBody.currentOrigin,start.destination);VectorCopy(propBody.currentAngles,start.angles);
   CHECK(angularCargoClear(start)&&angularCargoClear(planned));
  }
  AngularCargo blockedTurn;
  arcsChecked=0;
  CHECK(!angularCargoPlan(deck,destination,turn,blockedTurn)&&arcsChecked>1);
  CHECK(deck.linked&&propBody.linked&&propBody.currentOrigin[0]==propStart[0]&&propBody.currentAngles[YAW]==0);
  VectorSet(obstacleOrigin,100,100,100);
  {
   AngularCargoUnlinked unlinked(clear);
   G_SetAngles(&deck,turn);
   CHECK(angularCargoCommit(deck,clear));
  }
  CHECK(Distance(propBody.currentOrigin,planned.destination)<.001f&&propBody.currentAngles[YAW]==90);
  // The lowest part of this overhang is beside the deck. A turn should
  // keep the gap at the actual supported underside instead of lifting by
  // the depth of the unsupported tip.
  G_SetAngles(&deck,vec3_origin);
  G_JAR_ProjectedDeckHull(deck,vec3_origin,deck.mins,deck.maxs);
  auto slope=std::make_shared<jar::physics::PropGeometry>();
  for(float x:{-7.f,7.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})
   slope->Add({x,y,-.5f*x+z});
  clearanceGeometry=slope;
  const vec3_t overhang={deck.maxs[0]+4,0,deck.currentOrigin[2]+deck.maxs[2]-.3125f};
  G_SetOrigin(&propBody,overhang);G_SetAngles(&propBody,vec3_origin);
  VectorSet(propBody.mins,-7,-1,-4.5f);VectorSet(propBody.maxs,7,1,4.5f);
  const auto deckBody=liftBody(deck),overhangBody=liftBody(propBody);
  float gap;
  CHECK(JAR_LiftGap(&deckBody,&overhangBody)< -4);
  CHECK(platformPropSupported(deckBody,overhangBody,propBody,1,&gap)&&std::fabs(gap-.75f)<.001f);
  const vec3_t supportedTurns[]={{0,90,0},{15,90,0}};
  for(const auto& angle:supportedTurns){
   AngularCargo settled;
   CHECK(angularCargoPlan(deck,destination,angle,settled)&&settled.members.size()==1);
   const auto& member=settled.members.front();
   auto next=deckBody;
   VectorCopy(destination,next.origin);
   G_JAR_ProjectedDeckHull(deck,angle,next.mins,next.maxs);
   jar::physics::NativePropHull rotatedHull;
   CHECK(rotatedHull.Build(*slope,pv(member.angles)));
   float afterGap;
   CHECK(platformPropGap(next,member.destination,rotatedHull,afterGap)&&std::fabs(afterGap-gap)<.001f);
   CHECK(VectorCompare(propBody.currentOrigin,overhang));
  }
  clearanceGeometry.reset();
  Forget(state,169);deck={};propBody={};
  gi=savedImports;globals.num_entities=savedCount;
 }
 using jar::context::RotateGesture;
 CHECK(RotateGesture(false,true,1,BUTTON_ATTACK|BUTTON_USE,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 CHECK(!RotateGesture(false,true,0,BUTTON_ATTACK|BUTTON_USE,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 CHECK(!RotateGesture(true,true,1,BUTTON_ATTACK|BUTTON_USE,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 CHECK(!RotateGesture(false,false,1,BUTTON_ATTACK|BUTTON_USE,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 CHECK(!RotateGesture(false,true,1,BUTTON_USE,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 CHECK(!RotateGesture(false,true,1,BUTTON_ATTACK|BUTTON_USE|BUTTON_ALT_ATTACK,BUTTON_ATTACK,BUTTON_USE,BUTTON_ALT_ATTACK));
 using jar::context::PhysgunUseButtons;
 bool consumed=false;
 CHECK(PhysgunUseButtons(BUTTON_USE,BUTTON_USE,false,consumed)==BUTTON_USE&&!consumed);
 CHECK(PhysgunUseButtons(BUTTON_ATTACK|BUTTON_USE,BUTTON_USE,true,consumed)==BUTTON_ATTACK&&consumed);
 CHECK(PhysgunUseButtons(BUTTON_ALT_ATTACK|BUTTON_USE,BUTTON_USE,true,consumed)==BUTTON_ALT_ATTACK&&consumed);
 CHECK(PhysgunUseButtons(BUTTON_USE,BUTTON_USE,false,consumed)==0&&consumed);
 CHECK(PhysgunUseButtons(0,BUTTON_USE,false,consumed)==0&&!consumed);
 CHECK(PhysgunUseButtons(BUTTON_USE,BUTTON_USE,false,consumed)==BUTTON_USE&&!consumed);
 });}

