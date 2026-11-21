// Production fighter-boundary callbacks, steering and persistence; headless.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_vehicles.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static int damageCount=0;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void SetClientViewAngle(gentity_t*e,vec3_t angles){VectorCopy(angles,e->client->ps.viewangles);}
qboolean G_PointInBounds(const vec3_t p,const vec3_t lo,const vec3_t hi){for(int i=0;i<3;++i)if(p[i]<lo[i]||p[i]>hi[i])return qfalse;return qtrue;}
void InitTrigger(gentity_t*e){e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;if(e->spawnflags&128)e->svFlags|=SVF_INACTIVE;}
qboolean G_SpawnInt(const char*,const char*d,int*out){*out=atoi(d);return qfalse;}
gentity_t*G_Find(gentity_t*,int,const char*name){for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(e.inuse&&e.targetname&&!Q_stricmp(e.targetname,name))return &e;}return nullptr;}
void G_Damage(gentity_t*e,gentity_t*inflictor,gentity_t*attacker,const vec3_t,const vec3_t,int damage,int flags,int mod,int){CHECK(e==inflictor&&e==attacker&&damage==99999&&flags==DAMAGE_NO_PROTECTION&&mod==MOD_SUICIDE);++damageCount;}
#include "game/g_shipboundary.inc"
#define UNUSED_TOUCH(name) void name(gentity_t*,gentity_t*,trace_t*){throw std::runtime_error("Wrong touch callback: " #name);}
UNUSED_TOUCH(Touch_Item) UNUSED_TOUCH(teleporter_touch) UNUSED_TOUCH(charge_stick) UNUSED_TOUCH(Touch_DoorTrigger)
UNUSED_TOUCH(Touch_PlatCenterTrigger) UNUSED_TOUCH(Touch_Plat) UNUSED_TOUCH(Touch_Button) UNUSED_TOUCH(Touch_Multi)
UNUSED_TOUCH(trigger_push_touch) UNUSED_TOUCH(trigger_teleporter_touch) UNUSED_TOUCH(hurt_touch) UNUSED_TOUCH(NPC_Touch)
UNUSED_TOUCH(touch_ammo_crystal_tigger) UNUSED_TOUCH(funcBBrushTouch) UNUSED_TOUCH(touchLaserTrap) UNUSED_TOUCH(prox_mine_stick)
UNUSED_TOUCH(func_rotating_touch) UNUSED_TOUCH(TouchTieBomb) UNUSED_TOUCH(space_touch) UNUSED_TOUCH(bidirectional_touch)
void portable_shield_touch(gentity_t*,gentity_t*,trace_t*){throw std::runtime_error("Unexpected shield callback");}
UNUSED_TOUCH(factory_flag_touch)
UNUSED_TOUCH(siege_item_touch)
#include "game/g_entity_touch.inc"
static void Link(gentity_t*e){e->linked=qtrue;VectorAdd(e->currentOrigin,e->mins,e->absmin);VectorAdd(e->currentOrigin,e->maxs,e->absmax);}
static gentity_t&Make(int index,const char*name){auto&e=g_entities[index];e.inuse=qtrue;e.s.number=index;e.classname=G_NewString(name);return e;}
static void Touch(gentity_t&v,gentity_t&ship){GEntity_TouchFunc(&v,&ship,nullptr);}
int main(){return RunTests("flight boundary geometry, native callbacks, steering, ownership and save/load",[]{
 static_assert(touchF_space_touch==19&&touchF_bidirectional_touch==20&&touchF_shipboundary_touch==21,"Saved callbacks append only");
 static_assert(useF_space_use==79&&useF_bidirectional_use==80&&useF_marker_use==81&&useF_shipboundary_use==82,"Saved use callback IDs must remain stable");
 gi.linkentity=Link;globals.num_entities=64;level.time=1000;
 auto&v=Make(10,"trigger_shipboundary");v.model=G_NewString("@jar_box");v.spawnflags=1;SP_trigger_shipboundary(&v);
 auto&target=Make(11,"target_position");target.targetname=G_NewString("destination");VectorSet(target.currentOrigin,0,1024,256);v.target=G_NewString(target.targetname);
 CHECK(v.e_TouchFunc==touchF_shipboundary_touch&&v.e_UseFunc==useF_shipboundary_use&&v.count==3000&&v.maxs[0]==512&&v.maxs[2]==512&&G_ShipBoundaryHasVolumes());
 auto&ship=Make(20,"NPC");auto&pilot=Make(0,"player");gclient_t shipClient{},pilotClient{};Vehicle_t vehicle{};vehicleInfo_t info{};
 ship.client=&shipClient;ship.health=100;ship.m_pVehicle=&vehicle;vehicle.m_pParentEntity=&ship;vehicle.m_pPilot=&pilot;vehicle.m_pVehicleInfo=&info;vehicle.m_fTimeModifier=6;
 info.type=VH_FIGHTER;pilot.client=&pilotClient;pilot.health=100;pilot.s.m_iVehicleNum=20;VectorSet(ship.client->ps.origin,0,0,256);VectorCopy(ship.client->ps.origin,ship.currentOrigin);VectorCopy(ship.currentOrigin,ship.lastOrigin);
 Touch(v,ship);CHECK(shipTurns.size()==1&&shipClient.ps.vehTurnaroundIndex==11&&shipClient.ps.vehTurnaroundTime==4000&&pilotClient.ps.vehTurnaroundIndex==11);
 level.time=1200;Touch(v,ship);CHECK(shipClient.ps.vehTurnaroundTime==4000);G_ShipBoundaryGuide(&vehicle);CHECK(std::fabs(vehicle.m_vOrientation[YAW]-9)<.001f&&vehicle.m_ucmd.forwardmove==127&&!vehicle.m_ucmd.upmove&&!vehicle.m_ucmd.rightmove&&pilotClient.ps.viewangles[YAW]==9);
 usercmd_t cmd{};cmd.forwardmove=-127;cmd.rightmove=127;G_ShipBoundaryPilot(&pilot,&cmd);CHECK(cmd.forwardmove==127&&!cmd.rightmove&&!cmd.upmove&&cmd.angles[YAW]==ANGLE2SHORT(9));
 shipClient.ps.eFlags|=EF_FORCE_GRIPPED;VectorCopy(vehicle.m_vOrientation,vehicle.m_vPrevOrientation);G_ShipBoundaryGuide(&vehicle);CHECK(vehicle.m_vOrientation[YAW]==9);cmd.forwardmove=-127;G_ShipBoundaryPilot(&pilot,&cmd);CHECK(cmd.forwardmove==-127);shipClient.ps.eFlags&=~EF_FORCE_GRIPPED;
 // Native destination motion, not its stale spawn origin, drives steering.
 VectorSet(target.currentOrigin,1024,-1024,256);VectorClear(vehicle.m_vPrevOrientation);G_ShipBoundaryGuide(&vehicle);CHECK(std::fabs(vehicle.m_vOrientation[YAW]+9)<.001f);
 // Rotation rate is independent of the native update cadence.
 VectorSet(target.currentOrigin,0,1024,256);
 for(int rate:{20,60,120}){VectorClear(vehicle.m_vOrientation);vehicle.m_fTimeModifier=60.f/rate;for(int i=0;i<rate;++i){VectorCopy(vehicle.m_vOrientation,vehicle.m_vPrevOrientation);G_ShipBoundaryGuide(&vehicle);}CHECK(std::fabs(vehicle.m_vOrientation[YAW]-90)<.002f);}
 vehicle.m_fTimeModifier=6;
 // Expiry releases input and cannot rearm on repeated inside contacts.
 level.time=4000;G_ShipBoundaryFrame();CHECK(!shipClient.ps.vehTurnaroundTime&&!pilotClient.ps.vehTurnaroundIndex&&shipTurns.size()==1);Touch(v,ship);CHECK(!shipClient.ps.vehTurnaroundTime);
 VectorSet(shipClient.ps.origin,700,0,256);G_ShipBoundaryFrame();CHECK(shipTurns.empty());VectorSet(shipClient.ps.origin,0,0,256);Touch(v,ship);CHECK(shipClient.ps.vehTurnaroundTime==7000);
 // Disabled/inactive volumes and deleted targets, pilots or controllers cancel
 // both native mirrors before any referenced entity slot can be reused.
 shipboundary_use(&v,nullptr,nullptr);CHECK(!v.contents&&shipTurns.empty()&&!pilotClient.ps.vehTurnaroundTime);Touch(v,ship);CHECK(shipTurns.empty());shipboundary_use(&v,nullptr,nullptr);Touch(v,ship);
 v.svFlags|=SVF_INACTIVE;G_ShipBoundaryFrame();CHECK(shipTurns.empty());v.svFlags&=~SVF_INACTIVE;Touch(v,ship);G_ShipBoundaryForget(&target);CHECK(shipTurns.empty()&&!shipClient.ps.vehTurnaroundIndex);target.inuse=qfalse;target.inuse=qtrue;
 Touch(v,ship);pilot.s.m_iVehicleNum=0;G_ShipBoundaryPilot(&pilot,&cmd);G_ShipBoundaryFrame();CHECK(shipTurns.empty()&&!pilotClient.ps.vehTurnaroundTime);pilot.s.m_iVehicleNum=20;
 Touch(v,ship);G_ShipBoundaryForget(&pilot);CHECK(shipTurns.empty());Touch(v,ship);G_ShipBoundaryForget(&v);CHECK(shipTurns.empty());
 // An external override ends our lease without overwriting its newer state.
 Touch(v,ship);shipClient.ps.vehTurnaroundIndex=99;shipClient.ps.vehTurnaroundTime=9000;G_ShipBoundaryFrame();CHECK(shipTurns.empty()&&shipClient.ps.vehTurnaroundIndex==99&&shipClient.ps.vehTurnaroundTime==9000&&!pilotClient.ps.vehTurnaroundTime);shipClient.ps.vehTurnaroundIndex=shipClient.ps.vehTurnaroundTime=0;
 // The portable default ignores empty/damaged fighters; native destruction is
 // available explicitly. Ordinary actors/ground vehicles are unaffected.
 vehicle.m_pPilot=nullptr;Touch(v,ship);CHECK(!damageCount&&shipTurns.empty());v.spawnflags=0;Touch(v,ship);CHECK(damageCount==1);vehicle.m_pPilot=&pilot;vehicle.m_iRemovedSurfaces=1;Touch(v,ship);CHECK(damageCount==2);v.spawnflags=1;Touch(v,ship);CHECK(damageCount==2);vehicle.m_iRemovedSurfaces=0;
 info.type=VH_SPEEDER;Touch(v,ship);CHECK(shipTurns.empty());info.type=VH_FIGHTER;
 // Rotated volumes use exact local geometry, including swept fly-throughs.
 vec3_t dimensions={32,128,64};CHECK(jar::volumeEntity::Resize(v,dimensions));VectorSet(v.currentAngles,0,45,0);jar::volumeEntity::Refresh(v);Link(&v);
 VectorSet(shipClient.ps.origin,50,50,32);VectorCopy(shipClient.ps.origin,ship.lastOrigin);Touch(v,ship);CHECK(shipTurns.empty());
 VectorSet(ship.lastOrigin,-100,0,32);VectorSet(shipClient.ps.origin,100,0,32);Touch(v,ship);CHECK(shipTurns.size()==1);G_ShipBoundaryReleaseAll();CHECK(shipTurns.empty()&&!pilotClient.ps.vehTurnaroundTime);
 VectorSet(shipClient.ps.origin,0,0,32);VectorCopy(shipClient.ps.origin,ship.lastOrigin);Touch(v,ship);
 const int end=shipClient.ps.vehTurnaroundTime;for(float bad:{0.f,.09f,60.1f,std::numeric_limits<float>::quiet_NaN()})CHECK(!G_ShipBoundarySet(v,bad,true)&&shipClient.ps.vehTurnaroundTime==end);
 CHECK(G_ShipBoundarySet(v,60,false)&&v.count==60000&&!(v.spawnflags&1)&&shipTurns.empty());CHECK(G_ShipBoundarySet(v,.1f,true)&&v.count==100);CHECK(G_ShipBoundarySet(v,3,true));Touch(v,ship);
 MemorySave save;ojk::SavedGameHelper h(&save);G_ShipBoundaryExport(false,h);G_ShipBoundaryReset();save.reset_buffer_offset();CHECK(G_ShipBoundaryImport(false,h));h.ensure_all_data_read();CHECK(shipTurns.size()==1&&shipActive(20));
 G_ShipBoundaryForget(&target);save.reset_buffer_offset();CHECK(!G_ShipBoundaryImport(false,h)&&shipTurns.empty()); // native mirror no longer matches
 Touch(v,ship);save.reset_buffer();G_ShipBoundaryExport(false,h);G_ShipBoundaryReset();target.targetname=G_NewString("reused");save.reset_buffer_offset();CHECK(!G_ShipBoundaryImport(false,h));target.targetname=G_NewString("destination");save.reset_buffer_offset();CHECK(G_ShipBoundaryImport(false,h));
 // Native volume fields and callback IDs retain dimensions and policy.
 MemorySave native;ojk::SavedGameHelper fields(&native);gentity_t stored{},loaded{};stored.count=v.count;stored.spawnflags=v.spawnflags;stored.contents=v.contents;stored.e_TouchFunc=v.e_TouchFunc;stored.e_UseFunc=v.e_UseFunc;VectorCopy(v.pos1,stored.pos1);VectorCopy(v.pos2,stored.pos2);stored.sg_export(fields);native.reset_buffer_offset();loaded.sg_import(fields);fields.ensure_all_data_read();CHECK(loaded.count==3000&&loaded.spawnflags==1&&loaded.e_TouchFunc==touchF_shipboundary_touch&&loaded.e_UseFunc==useF_shipboundary_use&&VectorCompare(loaded.pos2,v.pos2));
 G_ShipBoundaryRestore();CHECK(v.contents==CONTENTS_TRIGGER&&v.maxs[2]==64);
 // Autosaves omit temporary vehicle/controller ownership and stale pilot aim.
 MemorySave automatic;ojk::SavedGameHelper ah(&automatic);G_ShipBoundaryExport(true,ah);G_ShipBoundaryReset();automatic.reset_buffer_offset();CHECK(G_ShipBoundaryImport(true,ah));ah.ensure_all_data_read();CHECK(shipTurns.empty()&&!pilotClient.ps.vehTurnaroundIndex&&!shipClient.ps.vehTurnaroundTime);
 MemorySave invalid;ojk::SavedGameHelper bad(&invalid);bad.write<uint32_t>(ENTITYNUM_WORLD);invalid.reset_buffer_offset();CHECK(!G_ShipBoundaryImport(false,bad));
 });}
