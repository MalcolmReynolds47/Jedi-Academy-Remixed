// Exercise the exact writer selected by the engine header, not just each codec.
#include "game/g_local.h"
#include "cgame/cg_local.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/g_vehicles.h"
#include "game/g_laser_arm.h"
#include "game/g_mover_motion.h"
#include "game/g_volume_geometry.h"
#include "game/g_sandbox_charges.h"
#include "qcommon/jar_savegame.h"
#include "game/g_mover_portal.h"
#include "qcommon/jar_workshop_state.h"
#include "chunk_save.h"
#include "bsp_section_motion_stub.h"
#include <map>
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
cg_t cg{};
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
using namespace jar::workshop;
State state;std::vector<uint8_t> prepared;
unsigned held=ENTITYNUM_NONE;float holdDistance=144;
static std::deque<std::string> strings;static std::string currentMusic,currentSky;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void eye(float*p,float*d){VectorClear(p);VectorSet(d,1,0,0);}
void NPC_SetAnim(gentity_t*,int,int,int,int){}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
#include "game/g_entity_angles.inc"
bool setupMoved(gentity_t&){return false;}
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
// These engine services are outside serialization; fail if this test reaches them.
void InitTrigger(gentity_t*){throw std::runtime_error("Unexpected trigger spawn");}
qboolean G_SpawnString(const char*,const char*,char**){throw std::runtime_error("Unexpected spawn parsing");}
qboolean G_SpawnInt(const char*,const char*,int*){throw std::runtime_error("Unexpected spawn parsing");}
gentity_t*G_Find(gentity_t*,int,const char*){throw std::runtime_error("Unexpected target lookup");}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){throw std::runtime_error("Unexpected damage");}
void SetClientViewAngle(gentity_t*,vec3_t){throw std::runtime_error("Unexpected steering");}
void EvaluateTrajectoryDelta(const trajectory_t*,int,vec3_t){throw std::runtime_error("Unexpected projectile flight");}
char*vtos(const vec3_t){throw std::runtime_error("Unexpected spawn formatting");}
void G_Error(const char*,...){throw std::runtime_error("Unexpected spawn error");}
cvar_t*com_buildScript=nullptr;
bool G_JAR_MoverTranslate(gentity_t&,const vec3_t,vec3_t){throw std::runtime_error("Unexpected movement");}
bool G_JAR_MoverRotate(gentity_t&,const vec3_t){throw std::runtime_error("Unexpected rotation");}
qboolean G_PointInBounds(const vec3_t p,const vec3_t lo,const vec3_t hi){for(int i=0;i<3;++i)if(p[i]<lo[i]||p[i]>hi[i])return qfalse;return qtrue;}
#include "game/g_workshop_native_control.inc"
#include "game/g_mover_motion.inc"
#include "game/g_target_music.inc"
#include "game/g_shipboundary.inc"
#include "game/g_skyportal.inc"
bool G_JAR_SuspendActorRecovery(bool value){return false;}
void G_FreeEntity(gentity_t*){throw std::runtime_error("Unexpected entity teardown");}
#include "game/g_subbsp_ownership.inc"
#include "game/g_contact_clocks.inc"
bool G_JAR_WorkshopControls(const gentity_t*){return false;}
bool G_JAR_WorkshopPropDepth(const gentity_t&,const vec3_t,const vec3_t,const vec3_t,const JarSweepHull*,float&){return false;}
bool G_JAR_WorkshopTransportAllowed(const gentity_t&){return true;}
bool G_JAR_WorkshopTeleportAssembly(gentity_t&,gentity_t&){throw std::runtime_error("Unexpected group teleport during serialization");}
void G_JAR_TeleportActor(gentity_t*,vec3_t,vec3_t,bool){throw std::runtime_error("Unexpected teleport during serialization");}
#include "game/g_sandbox_teleport.inc"
#include "game/g_sandbox_vehicle_push_save.inc"
#include "game/g_workshop_vehicle_thruster.h"
#include "game/g_workshop_undo_save.h"
static void writeUndoHistory(bool autosave,ojk::SavedGameHelper& save){
 std::vector<CreationRecord> records;
 if(!autosave)records={{CreationKind::Assembly,{state.owned[0].id,state.owned[1].id}},
                       {CreationKind::Attachment,{state.joints[0].id}}};
 WriteCreationHistory(save,records);
}
#include "game/g_workshop_save_write.inc"
static gentity_t&Make(int n,const char*name){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString(name);return e;}
static void Restore(ChunkSave&save,bool autosave){
 ojk::SavedGameHelper h(&save);uint32_t size=0;
 h.read_chunk<uint32_t>(INT_ID('J','W','S','Z'),size);CHECK(size>=24&&size<=wireLimit);
 std::vector<uint8_t> bytes(size);h.read_chunk<uint8_t>(INT_ID('J','W','S','T'),bytes.data(),int(size));
 State result;CHECK(Decode(bytes.data(),bytes.size(),result,ENTITYNUM_WORLD));
 CHECK(result.count==1&&result.weapons[0].family==WP_PHYSGUN&&result.weapons[0].tool==1);
 CHECK(result.ownedCount==(autosave?0u:2u)&&result.jointCount==(autosave?0u:1u));
 if(!autosave){CHECK(result.owned[0].entity==7&&result.owned[1].entity==19&&result.owned[1].frozen);CHECK(result.joints[0].length==123.5f);
  CHECK(result.owned[1].physics.mass==480&&result.owned[1].physics.noGravity);
  CHECK(result.joints[0].path.count==2&&result.joints[0].path.bend[0][1]==24&&result.joints[0].path.bend[1][1]==24);}
 nativeRead(autosave,h);
 h.read_chunk(INT_ID('J','M','U','S'));CHECK(G_MusicImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','S','H','P'));CHECK(G_ShipBoundaryImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','M','F','R'));CHECK(G_JAR_MoverMotionImport(autosave,h,true));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','S','K','Y'));CHECK(G_SkyPortalImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','B','S','O'));CHECK(G_SubBSPOwnershipImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','H','R','T'));CHECK(G_ContactClocksImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','T','A','R'));CHECK(G_TeleportArrivalsImport(autosave,h));h.ensure_all_data_read();
 h.read_chunk(INT_ID('J','V','P','D'));CHECK(G_VehiclePushImport(autosave,h));
 h.read_chunk(INT_ID('J','C','H','G'));CHECK(G_SandboxChargesImport(autosave,h));
 h.read_chunk(INT_ID('J','U','N','D'));
 std::vector<CreationRecord> records;
 CHECK(ReadCreationHistory(h,result,autosave,records));
 h.ensure_all_data_read();
 CHECK(records.size()==(autosave?0u:2u));
 if(!autosave){
  CHECK((records[0].kind==CreationKind::Assembly&&records[0].ids==std::vector<uint32_t>{1,2}));
  CHECK((records[1].kind==CreationKind::Attachment&&records[1].ids==std::vector<uint32_t>{1}));
 }
}
int main(){return RunTests("engine-selected Sandbox writer: full/autosave chunks, ownership and native restoration",[]{
 gi.Error=[](int,const char*,...){throw std::runtime_error("Save rejected");};gi.linkentity=[](gentity_t*){};
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 gi.GetConfigstring=[](int n,char*out,int size){CHECK(n==CS_MUSIC||n==CS_SKYBOXORG);Q_strncpyz(out,(n==CS_MUSIC?currentMusic:currentSky).c_str(),size);};
 gi.SetConfigstring=[](int n,const char*text){CHECK(n==CS_MUSIC||n==CS_SKYBOXORG);(n==CS_MUSIC?currentMusic:currentSky)=text;};
 globals.num_entities=64;level.time=1000;
 Weapon gun;gun.family=WP_PHYSGUN;gun.tool=1;gun.sandbox=1;CHECK(Add(state,gun));
 CHECK(Own(state,7,3001));CHECK(Own(state,19,1001));state.owned[1].physics={480,true};
 jar::constraint::Joint rope;rope.kind=jar::constraint::Rope;rope.first=1;rope.second=2;rope.length=123.5f;CHECK(AddJoint(state,rope));
 state.joints[0].path.count=2;VectorSet(state.joints[0].path.bend[0],40,24,80);VectorSet(state.joints[0].path.bend[1],60,24,80);
 auto&npc=Make(7,"NPC");gclient_t npcClient{};npc.client=&npcClient;npc.health=100;npcClient.ps.gravity=800;
 auto&mover=Make(8,"func_train");
 auto&music=Make(10,"target_play_music");music.model=G_NewString("@jar_music");music.targetname=G_NewString("fixture_music");music.message=G_NewString(jar::music::defaultTrack);
 auto&volume=Make(11,"trigger_shipboundary");volume.linked=qtrue;volume.contents=CONTENTS_TRIGGER;volume.target=G_NewString("destination");
 auto&target=Make(12,"target_position");target.targetname=G_NewString("destination");
 auto&ship=Make(13,"NPC");auto&pilot=Make(0,"player");gclient_t shipClient{},pilotClient{};Vehicle_t vehicle{};vehicleInfo_t info{};
 ship.client=&shipClient;shipClient.NPC_class=CLASS_VEHICLE;ship.health=100;ship.m_pVehicle=&vehicle;vehicle.m_pParentEntity=&ship;vehicle.m_pPilot=&pilot;vehicle.m_pVehicleInfo=&info;info.type=VH_FIGHTER;
 pilot.client=&pilotClient;pilot.health=100;pilot.s.m_iVehicleNum=13;pilot.owner=&ship;
 auto&sky=Make(14,"misc_skyportal");sky.targetname=G_NewString("fixture_sky");CHECK(G_SkyPortalSpawn(sky));
 auto&section=Make(15,"misc_bsp");section.s.modelindex=1;auto&sectionChild=Make(16,"func_door");
 auto&hurt=Make(17,"trigger_hurt");hurt.model=G_NewString(jar::contactTrigger::model);npc.takedamage=qtrue;
 auto&push=Make(18,"trigger_push");push.model=G_NewString(jar::push::volumeModel);
 auto&cargo=Make(19,"jar_sandbox_prop");
 auto&det=Make(20,"detpack");det.s.weapon=WP_DET_PACK;det.s.eType=ET_GENERAL;det.s.pos.trType=TR_STATIONARY;
 det.s.eFlags=EF_MISSILE_STICK;det.e_DieFunc=dieF_WP_ExplosiveDie;VectorSet(det.currentOrigin,10,0,10);
 auto&portal=Make(21,"trigger_teleport");
 portal.model=G_NewString(jar::teleport::volumeModel);portal.e_TouchFunc=touchF_trigger_teleporter_touch;
 CHECK(G_TeleportSet(portal,{jar::teleport::Volume,750,jar::teleport::NoActors,false,true,true,true,false,false,true,true,true,true}));
 MemorySave entityBytes;ojk::SavedGameHelper entityIO(&entityBytes);
 portal.sg_export(entityIO);entityBytes.reset_buffer_offset();
 auto& restoredPortal=g_entities[22];restoredPortal.sg_import(entityIO);entityIO.ensure_all_data_read();
 // The engine restores these string references separately from native fields.
 restoredPortal.classname=portal.classname;restoredPortal.model=portal.model;
 jar::teleport::Settings restoredTeleport;
 CHECK(G_TeleportRead(restoredPortal,restoredTeleport)&&restoredTeleport.frozenProps&&restoredTeleport.assemblies&&
       restoredTeleport.movers&&restoredTeleport.worldAnchors);
 auto& door=Make(23,"func_door");door.bmodel=qtrue;door.s.eType=ET_MOVER;
 door.flags=FL_TEAMSLAVE|jarDetachedMoverAreaPortal;
 VectorSet(door.currentOrigin,600,50,25);VectorSet(door.pos1,600,50,25);VectorSet(door.pos2,640,50,25);
 entityBytes.reset_buffer();door.sg_export(entityIO);entityBytes.reset_buffer_offset();
 auto& restoredDoor=g_entities[24];restoredDoor.sg_import(entityIO);entityIO.ensure_all_data_read();
 CHECK(restoredDoor.flags==(FL_TEAMSLAVE|jarDetachedMoverAreaPortal)&&restoredDoor.pos1[0]==600&&restoredDoor.pos2[0]==640);
 auto& emitter=Make(25,"target_laser");emitter.model=G_NewString(jar::sandboxLaser::model);
 emitter.target=G_NewString("destination");
 CHECK(G_SandboxLaserInitialize(emitter,{0,true,false}));
 entityBytes.reset_buffer();emitter.sg_export(entityIO);entityBytes.reset_buffer_offset();
 auto& restoredEmitter=g_entities[26];restoredEmitter.sg_import(entityIO);entityIO.ensure_all_data_read();
 restoredEmitter.classname=emitter.classname;restoredEmitter.model=emitter.model;restoredEmitter.target=emitter.target;
 jar::sandboxLaser::Settings laser;
 CHECK(G_SandboxLaserRestore(restoredEmitter)&&G_SandboxLaserRead(restoredEmitter,laser)&&
       jar::sandboxLaser::Same(laser,{0,true,false})&&restoredEmitter.target==emitter.target&&
       restoredEmitter.nextthink==level.time+jar::sandboxLaser::interval&&!restoredEmitter.alt_fire);
 auto& legacyEmitter=Make(27,"target_laser");legacyEmitter.model=G_NewString("models/map_objects/imperial/crate1.md3");
 legacyEmitter.e_ThinkFunc=thinkF_target_laser_think;legacyEmitter.s.eType=ET_BEAM;
 legacyEmitter.damage=7;legacyEmitter.spawnflags=1;
 CHECK(G_SandboxLaserRestore(legacyEmitter)&&G_SandboxLaserRead(legacyEmitter,laser)&&
       jar::sandboxLaser::Same(laser,{7,false,true})&&legacyEmitter.s.eType==ET_GENERAL&&!legacyEmitter.nextthink);
 auto& station=Make(28,"misc_ammo_floor_unit");
 station.targetname=G_NewString("jar_sb_28");station.e_UseFunc=useF_ammo_power_converter_use;station.count=27;
 CHECK(G_SandboxStationRestore(station,0)&&station.max_health==100&&station.count==27&&!station.delay);
 CHECK(G_SandboxStationSet(station,{110,32,500,false,true}));
 station.setTime=level.time+90;station.s.loopSound=9;station.attackDebounceTime=level.time+250;
 G_SandboxStationSchedule(station);
 const auto rechargeDeadline=station.painDebounceTime;
 entityBytes.reset_buffer();station.sg_export(entityIO);entityBytes.reset_buffer_offset();
 auto& restoredStation=g_entities[29];restoredStation.sg_import(entityIO);entityIO.ensure_all_data_read();
 restoredStation.classname=station.classname;restoredStation.targetname=station.targetname;
 jar::sandboxStation::Settings stationState;
 CHECK(G_SandboxStationRestore(restoredStation,2)&&G_SandboxStationRead(restoredStation,stationState)&&
       stationState.capacity==110&&stationState.charge==32&&stationState.recharge==500&&stationState.enabled&&
       restoredStation.setTime==station.setTime&&restoredStation.nextthink==rechargeDeadline&&!restoredStation.s.loopSound);
 auto& breakable=Make(30,"func_glass");
 breakable.targetname=G_NewString("jar_sb_30");breakable.target=G_NewString("destination");
 breakable.health=40;breakable.material=MAT_GLASS;breakable.s.modelindex=5;breakable.radius=1;
 CHECK(G_SandboxBreakableInitialize(breakable));
 for(const auto phase:{jar::sandboxBreakable::Pending,jar::sandboxBreakable::Broken}){
  jar::sandboxBreakable::Settings before;
  CHECK(G_SandboxBreakableRead(breakable,before));
  before.phase=phase;before.health=0;before.remaining=phase==jar::sandboxBreakable::Pending?275:0;
  CHECK(G_SandboxBreakableSet(breakable,before));
  entityBytes.reset_buffer();breakable.sg_export(entityIO);entityBytes.reset_buffer_offset();
  auto& restored=g_entities[31];restored.sg_import(entityIO);entityIO.ensure_all_data_read();
  restored.classname=breakable.classname;restored.targetname=breakable.targetname;restored.target=breakable.target;
  jar::sandboxBreakable::Settings after;
  CHECK(G_SandboxBreakableRestore(restored)&&G_SandboxBreakableRead(restored,after)&&
        jar::sandboxBreakable::Same(before,after)&&after.remaining==before.remaining&&restored.target==breakable.target);
 }
 auto& usable=Make(32,"func_usable");
 usable.model=G_NewString("@jar_box");usable.targetname=G_NewString("jar_sb_32");
 usable.target=G_NewString("off");usable.target2=G_NewString("on");usable.s.modelindex=5;
 usable.health=40;usable.e_DieFunc=dieF_func_usable_die;
 usable.count=1;usable.s.eFlags=EF_NODRAW;usable.e_ThinkFunc=thinkF_func_wait_return_solid;usable.activator=&npc;
 CHECK(G_SandboxUsableRestore(usable)&&G_SandboxUsableDormant(usable)&&usable.alt_fire&&usable.activator==&npc);
 const auto oldTrace=gi.trace;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
  CHECK(pass==32&&VectorCompare(start,end)&&mask==(MASK_PLAYERSOLID|CONTENTS_CORPSE));
  *hit={};hit->fraction=1;
 };
 for(bool cooling:{false,true}){
  jar::sandboxUsable::Settings before;
  CHECK(G_SandboxUsableRead(usable,before));
  before.mode=cooling?jar::sandboxUsable::Pulse:jar::sandboxUsable::Toggle;
  before.pending=!cooling;before.solid=cooling;before.remaining=cooling?250:0;
  CHECK(G_SandboxUsableSet(usable,before));
  entityBytes.reset_buffer();usable.sg_export(entityIO);entityBytes.reset_buffer_offset();
  auto& restored=g_entities[33];restored.sg_import(entityIO);entityIO.ensure_all_data_read();
  restored.classname=usable.classname;restored.model=usable.model;restored.targetname=usable.targetname;
  restored.target=usable.target;restored.target2=usable.target2;restored.activator=usable.activator;
  jar::sandboxUsable::Settings after;
  CHECK(G_SandboxUsableRestore(restored)&&G_SandboxUsableRead(restored,after)&&jar::sandboxUsable::Same(before,after)&&
        after.remaining==before.remaining&&restored.nextthink==usable.nextthink&&restored.target2==usable.target2&&
        restored.activator==(cooling?nullptr:&npc));
 }
 gi.trace=oldTrace;
 const uint32_t expected[]={INT_ID('J','W','S','Z'),INT_ID('J','W','S','T'),INT_ID('J','N','C','N'),INT_ID('J','N','A','T'),INT_ID('J','M','U','S'),INT_ID('J','S','H','P'),INT_ID('J','M','F','R'),INT_ID('J','S','K','Y'),INT_ID('J','B','S','O'),INT_ID('J','H','R','T'),INT_ID('J','T','A','R'),INT_ID('J','V','P','D'),INT_ID('J','C','H','G'),INT_ID('J','U','N','D')};
 for(bool autosave:{false,true})for(bool precomputed:{false,true}){
  G_SubBSPOwnershipReset();G_SubBSPAllocated(&section);CHECK(G_SubBSPBegin(&section,true));{G_SubBSPCapture capture(&section);G_SubBSPAllocated(&sectionChild);}
  G_SandboxChargesReset();G_SubBSPAllocated(&cargo);G_SubBSPAllocated(&det);G_SandboxChargeAttach(det,cargo);
  G_ContactClocksReset();CHECK(G_ContactClockReady(hurt,npc,400));
  CHECK(G_ContactClockReady(push,cargo,650));
  teleportArrivals[7]={G_EntityLifetime(&npc),level.time};
  teleportArrivals[19]={G_EntityLifetime(&cargo),level.time};
  nativeControls.clear();nativeBegin(npc,true);G_SetSkyPortal("100 200 300 0 0 0 0 0 300");CHECK(G_SkyPortalSet(sky,{true,false}));
  JarMoverMotion motion;motion.offset[0]=25;motion.velocity[1]=80;motion.angular[2]=2;CHECK(G_JAR_MoverMotionRestore(mover,motion));
  G_SetMusic("mission_baseline");target_play_music_use(&music,nullptr,nullptr);
  for(auto* member:{&ship,&pilot})member->client->ps.vehTurnaroundTime=0;
  VectorClear(vehicle.jarPushDrift);VectorClear(shipClient.ps.velocity);
  const vec3_t acceleration={500,-7500,0};CHECK(G_WorkshopVehicleThrust(ship,acceleration,.05f)&&G_VehiclePushPresent());
  shipTurns[13]={11,12,0,4000};for(auto*e:{&ship,&pilot}){e->client->ps.vehTurnaroundIndex=12;e->client->ps.vehTurnaroundTime=4000;}
  prepared=precomputed?Encode(state,autosave):std::vector<uint8_t>{};
  if(precomputed)state.weapons[0].family=WP_BLASTER; // Writer must retain the prepared snapshot.
  ChunkSave save;gi.saved_game=&save;
  // This is the selector used by the server before WriteLevel dispatches here.
  const int version=jar::WriteSaveVersion(1,false,true);CHECK(version==jar::workshopSaveCurrent);
  G_JAR_WorkshopWrite(autosave,version);
  state.weapons[0].family=WP_PHYSGUN;
  CHECK(save.chunks.size()==sizeof(expected)/sizeof(*expected));
  for(size_t i=0;i<save.chunks.size();++i)CHECK(save.chunks[i].first==expected[i]);
  CHECK(prepared.empty());nativeControls.clear();G_MusicReset();G_ShipBoundaryReset();G_JAR_MoverMotionReset();G_SkyPortalReset();
  G_ContactClocksReset();
  G_TeleportArrivalsReset();
  G_SandboxChargesReset();
  VectorClear(vehicle.jarPushDrift);
  if(!autosave)G_SubBSPOwnershipReset();
  Restore(save,autosave);CHECK(G_SubBSPOwner(&sectionChild)==&section&&G_SubBSPNamespace(&sectionChild)!=0);
  CHECK(G_ContactClockReady(hurt,npc,400)==autosave);
  CHECK(G_ContactClockReady(push,cargo,650)==autosave);
  CHECK(G_TeleportArrivalPending(npc)==!autosave);
  CHECK(G_TeleportArrivalPending(cargo)==!autosave);
  std::vector<SandboxChargeMove> chargeMoves;CHECK(G_SandboxChargesPlan(cargo,cargo.currentOrigin,chargeMoves));
  CHECK(chargeMoves.size()==(autosave?0u:1u));
  CHECK(G_VehiclePushPresent()==!autosave);
  CHECK(vehicle.jarPushDrift[0]==(autosave?0:25)&&vehicle.jarPushDrift[1]==(autosave?0:-375));
  jar::skyPortal::Settings restoredSky;CHECK(G_SkyPortalRead(sky,restoredSky)&&restoredSky.enabled==!autosave&&!restoredSky.entities);
  CHECK(autosave?currentSky=="100 200 300 0 0 0 0 0 300":currentSky==jar::skyPortal::Encode(skyView(sky)));
  CHECK(nativeControls.size()==(autosave?0u:1u)&&shipTurns.size()==(autosave?0u:1u));
  CHECK(G_MusicActive(music)==!autosave);CHECK(currentMusic==(autosave?"mission_baseline":jar::music::defaultTrack));
  CHECK(G_JAR_MoverMotionRead(mover,motion)==!autosave);
  if(!autosave){CHECK(motion.offset[0]==25&&motion.velocity[1]==80&&motion.angular[2]==2);CHECK(nativeControls[7].frozen&&nativeControls[7].gravity==800);}
  // Recreate the unfrozen baseline for the next run, rather than taking a
  // second snapshot of the tool's temporary gravity override.
  nativeReleaseAll();npcClient.ps.gravity=800;npcClient.ps.eFlags=0;
 }
 // A malformed later row must leave a valid live vehicle unchanged.
 ChunkSave bad;ojk::SavedGameHelper badIO(&bad);
 badIO.write<uint32_t>(2);
 badIO.write<uint16_t>(13);badIO.write<float>(50);badIO.write<float>(25);
 badIO.write<uint16_t>(13);badIO.write<float>(10);badIO.write<float>(10);
 bad.reset_buffer_offset();VectorSet(vehicle.jarPushDrift,9,8,0);
 CHECK(!G_VehiclePushImport(false,badIO)&&vehicle.jarPushDrift[0]==9&&vehicle.jarPushDrift[1]==8);
 for(const auto& records:std::vector<std::vector<CreationRecord>>{
     {{CreationKind::Spawn,{1}},{CreationKind::Assembly,{1,2}}},
     {{CreationKind::Assembly,{2,2}}},
     {{CreationKind::Spawn,{100}}},
     {{CreationKind::Attachment,{100}}},
     {{CreationKind::Attachment,{1,2}}}}){
  MemorySave data;ojk::SavedGameHelper io(&data);
  WriteCreationHistory(io,records);data.reset_buffer_offset();
  std::vector<CreationRecord> output={{CreationKind::Spawn,{2}}};
  CHECK(!ReadCreationHistory(io,state,false,output)&&output.size()==1&&output[0].ids[0]==2);
 }
 // Native and recovery-only headers must still omit all Sandbox chunks.
 for(int version:{1,jar::actorRecoverySaveV1}){ChunkSave save;gi.saved_game=&save;prepared.clear();G_JAR_WorkshopWrite(false,version);CHECK(save.chunks.empty());}
 gi.saved_game=nullptr;
 });}
