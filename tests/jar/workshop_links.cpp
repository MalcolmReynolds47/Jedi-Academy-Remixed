// Actual Link and marker picking, including rotated/containing volumes.
#include "game/g_local.h"
#include "game/g_portal.h"
#include "game/g_scene_camera.h"
#include "game/g_rail_portable.h"
#include "game/g_spotlight.h"
#include "game/g_functions.h"
#include "game/g_distance_trigger.h"
#include "game/g_volume_geometry.h"
#include "game/g_workshop_npc_order.h"
#include "qcommon/jar_workshop_state.h"
#include "qcommon/jar_entity_presets.h"
#include "test_support.h"
#include <deque>
#include <set>
game_import_t gi{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
using namespace jar::workshop;
State state;uint32_t linkFirst=0;level_locals_t level{};
static std::deque<std::string> strings;static std::string status;static cvar_t output{},randomSlot{},giveSlot{},gateSlot{};
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;}
void message(const char*s){status=s;}
Owned*owned(unsigned n){for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].entity==n)return &state.owned[i];return nullptr;}
bool matches(const Owned&o){return g_entities[o.entity].inuse&&g_entities[o.entity].targetname&&!strcmp(g_entities[o.entity].targetname,va("jar_sb_%u",o.id));}
bool G_JAR_WorkshopOwns(const gentity_t* e){const auto* entry=e?owned(e->s.number):nullptr;return entry&&matches(*entry);}
gentity_t*G_Find(gentity_t*,int,const char*name){for(unsigned i=0;i<state.ownedCount;++i){auto&e=g_entities[state.owned[i].entity];if(e.inuse&&e.targetname&&!strcmp(e.targetname,name))return &e;}return nullptr;}
static bool waypointAdmission(){return true;}
#include "game/g_waypoint_properties.inc"
void distanceLink(gentity_t&source,int role,const char*name){G_DistanceSetLink(source,role,name);}
#include "game/g_logic_settings.h"
int pickupLifetimeClock(const gentity_t&){return level.time;}
#include "game/g_workshop_give.inc"
#include "game/g_sandbox_push.h"
#include "game/g_sandbox_laser.h"
void editSlottedLink(gentity_t& source,int slot,const char* name){setSlottedLink(source,slot,name);}
void editPushAimLink(gentity_t& source,const char* name){G_LogicSetOutput(source,0,name);source.damage=jar::push::Linked;}
void editTeleportLink(gentity_t& source,const char* name){G_LogicSetOutput(source,0,name);}
void editLaserEmitterAim(gentity_t& source,const char* name){G_LogicSetOutput(source,0,name);}
void editExplosionTrailAim(gentity_t& source,const char* name){G_LogicSetOutput(source,0,name);}
void editPortalLink(gentity_t& source,gentity_t* target){
 G_LogicSetOutput(source,0,target->targetname);
 G_PortalLinkChanged(source,target);
}
bool G_ReferencePortable(const gentity_t& e){
 return e.inuse&&jar::reference::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_reference");
}
bool contextActorOrder(gentity_t& actor,jar::npcOrder::Settings& order){
 if(!actor.client||!actor.NPC||actor.health<=0)return false;
 order=G_WorkshopReadNPCOrder(*actor.NPC);
 return true;
}
void editContextActorGoal(gentity_t& actor,jar::npcOrder::Destination goal){
 G_WorkshopWriteNPCGoal(*actor.NPC,goal);
}
#include "game/g_workshop_links.inc"
static gentity_t&Make(int slot,const char*name){auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=G_NewString(name);unsigned catalog=0;for(const auto&c:jar::catalog::entries)if(!strcmp(c.name,name))catalog=c.id;CHECK(Own(state,slot,catalog));e.targetname=G_NewString(va("jar_sb_%u",owned(slot)->id));return e;}
int main(){return RunTests("forward/reverse links, cycle/lifetime rejection and volume selection priority",[]{
 gi.cvar=[](const char* name,const char*,int)->cvar_t*{return !strcmp(name,"jar_randomLinkSlot")?&randomSlot:!strcmp(name,"jar_giveLinkSlot")?&giveSlot:!strcmp(name,"jar_useGateLinkSlot")?&gateSlot:&output;};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};

 auto&rt=Make(40,"rail_track");auto&rl=Make(41,"rail_lane");auto&rm=Make(42,"rail_mover");for(auto*e:{&rt,&rl,&rm})e->model=G_NewString("@jar_rail");rm.s.pos.trType=TR_LINEAR_STOP;rm.count=1;
 linkEntity(rm);linkEntity(rl);CHECK(!strcmp(rm.target,rl.targetname)&&!rm.count&&rm.s.pos.trType==TR_STATIONARY);linkEntity(rl);linkEntity(rt);CHECK(!strcmp(rl.target,rt.targetname));linkEntity(rt);linkEntity(rm);CHECK(!rt.target);linkEntity(rl);linkEntity(rm);CHECK(!strcmp(rl.target,rt.targetname));
 auto&wa=Make(33,"waypoint");auto&wb=Make(34,"waypoint_small");auto&wc=Make(35,"target_position");CHECK(G_WaypointSpawn(wa)&&G_WaypointSpawn(wb));
 linkEntity(wa);linkEntity(wb);CHECK(wa.target&&!strcmp(wa.target,wb.targetname));output.integer=1;linkEntity(wb);linkEntity(wa);CHECK(wb.target2&&!strcmp(wb.target2,wa.targetname));
 linkEntity(wa);linkEntity(wc);CHECK(!wa.target2&&status.find("another spawned waypoint")!=std::string::npos);wb.currentOrigin[0]=4097;linkEntity(wa);linkEntity(wb);CHECK(!wa.target2&&status.find("4096")!=std::string::npos);wb.currentOrigin[0]=4096;linkEntity(wa);linkEntity(wb);CHECK(wa.target2&&!strcmp(wa.target2,wb.targetname));output.integer=0;
 auto&scene=Make(30,"misc_camera_track");scene.model=G_NewString("@jar_scene_camera");auto&focus=Make(31,"misc_camera_focus");focus.model=scene.model;auto&subject=Make(32,"target_position");
 linkEntity(scene);linkEntity(focus);CHECK(!scene.target&&!scene.target2&&status.find("Camera route")!=std::string::npos);
 output.integer=1;linkEntity(scene);linkEntity(focus);CHECK(!strcmp(scene.target2,focus.targetname));
 linkEntity(focus);linkEntity(subject);CHECK(!focus.target&&status.find("Focus Primary")!=std::string::npos);
 output.integer=0;linkEntity(focus);linkEntity(subject);CHECK(!strcmp(focus.target,subject.targetname));
 const char*subjectName=focus.target;linkEntity(focus);focus.targetname=G_NewString("reused_scene_slot");linkEntity(scene);CHECK(focus.target==subjectName&&!linkFirst);
 auto&travel=Make(20,"target_level_change");auto&trigger=Make(21,"target_print");travel.target=G_NewString(trigger.targetname); // Identical spelling is still a destination-map spawn point.
 linkEntity(travel);linkEntity(trigger);CHECK(!strcmp(travel.target,trigger.targetname)&&status.find("Context > Travel")!=std::string::npos);
 linkEntity(trigger);linkEntity(travel);CHECK(trigger.target&&!strcmp(trigger.target,travel.targetname));
 auto& replicator=Make(22,"misc_replicator_item");replicator.e_UseFunc=useF_misc_replicator_item_spawn;
 linkEntity(replicator);linkEntity(travel);CHECK(!replicator.target&&status.find("activate it")!=std::string::npos);
 linkEntity(trigger);linkEntity(replicator);CHECK(trigger.target&&!strcmp(trigger.target,replicator.targetname));
 auto&root=Make(8,"trigger_bidirectional");auto&first=Make(9,"target_print");auto&second=Make(10,"target_print");
 linkEntity(root);linkEntity(first);CHECK(root.target&&!strcmp(root.target,first.targetname)&&!root.paintarget);
 output.integer=1;linkEntity(root);linkEntity(second);CHECK(!strcmp(root.target,first.targetname)&&!strcmp(root.paintarget,second.targetname));
 linkEntity(first);linkEntity(second);CHECK(!first.target&&status.find("Secondary output is only")!=std::string::npos);
 first.classname=G_NewString("trigger_bidirectional");first.paintarget=root.targetname;output.integer=0;const char*original=root.target;linkEntity(root);linkEntity(first);CHECK(root.target==original&&status.find("activation loop")!=std::string::npos);first.paintarget=nullptr;first.classname=G_NewString("target_print");
 linkEntity(root);root.targetname=G_NewString("reused_slot");linkEntity(second);CHECK(root.target==original&&!linkFirst);root.targetname=G_NewString(va("jar_sb_%u",owned(8)->id));
 root.model=G_NewString("@jar_box");jar::volumeEntity::Initialize(root);
 vec3_t start={0,0,32},dir={1,0,0};CHECK(selectControlNode(start,dir,10,17)==17); // A nearer actor wins inside the volume.
 CHECK(selectControlNode(start,dir,100,ENTITYNUM_WORLD)==8); // Exit face remains selectable.
 vec3_t farOutside={-100,0,32};CHECK(selectControlNode(farOutside,dir,20,17)==17&&selectControlNode(farOutside,dir,100,17)==8);
 vec3_t size={32,128,64};jar::volumeEntity::Resize(root,size);VectorSet(root.currentAngles,0,45,0);jar::volumeEntity::Refresh(root);
 VectorSet(start,50,50,32);VectorSet(dir,0,0,1);CHECK(selectControlNode(start,dir,100,17)==17); // Empty corner of the broad hull.
 // Overlapping volumes choose the nearest actual boundary, not first registry order.
 second.classname=G_NewString("trigger_space");second.model=root.model;jar::volumeEntity::Initialize(second);vec3_t smaller={16,16,64};jar::volumeEntity::Resize(second,smaller);
 VectorSet(start,0,0,32);VectorSet(dir,1,0,0);CHECK(selectControlNode(start,dir,100,17)==10);
 second.contents=0;CHECK(selectControlNode(start,dir,100,17)==10);second.inuse=qfalse;CHECK(selectControlNode(start,dir,100,17)==8);
 auto&spot=Make(14,"misc_spotlight");spot.model=G_NewString("@jar_spotlight");linkEntity(spot);linkEntity(first);CHECK(spot.target&&!strcmp(spot.target,first.targetname));
 output.integer=1;linkEntity(spot);linkEntity(root);CHECK(spot.target2&&!strcmp(spot.target2,root.targetname));output.integer=0;linkEntity(root);linkEntity(spot);CHECK(status.find("activation loop")!=std::string::npos);
 auto&panel=Make(11,"misc_portal_surface");panel.model=G_NewString("@jar_portal_surface");auto&camera=Make(12,"misc_portal_camera");camera.model=G_NewString("@jar_portal_camera");
 linkEntity(panel);linkEntity(first);CHECK(!panel.target&&status.find("portal camera")!=std::string::npos);
 linkEntity(panel);linkEntity(camera);CHECK(panel.owner==&camera&&!strcmp(panel.target,camera.targetname));
 output.integer=1;linkEntity(panel);linkEntity(camera);CHECK(!panel.target2&&status.find("Primary")!=std::string::npos);
 output.integer=0;
 linkEntity(camera);linkEntity(first);CHECK(camera.owner==&first&&!strcmp(camera.target,first.targetname));
 linkEntity(first);linkEntity(camera);CHECK(first.target&&!strcmp(first.target,camera.targetname));first.target=nullptr;
 linkEntity(camera);linkEntity(panel);CHECK(camera.owner==&panel&&!strcmp(camera.target,panel.targetname));
 auto&dist=Make(43,"trigger_entdist");dist.model=G_NewString("@jar_entdist");dist.e_UseFunc=useF_trigger_entdist_use;
 first.e_UseFunc=useF_Use_Target_Print;trigger.e_UseFunc=useF_Use_Target_Print;
 linkEntity(dist);linkEntity(first);CHECK(!strcmp(dist.target,first.targetname));
 output.integer=JAR_DISTANCE_OUTSIDE;linkEntity(dist);linkEntity(trigger);CHECK(!strcmp(dist.target2,trigger.targetname));
 output.integer=JAR_DISTANCE_REFERENCE;linkEntity(dist);linkEntity(subject);CHECK(!strcmp(dist.ownername,subject.targetname));
 output.integer=JAR_DISTANCE_SUBJECT;linkEntity(dist);linkEntity(subject);CHECK(!dist.NPC_target&&status.find("living spawned NPC")!=std::string::npos);
 static gclient_t npcClient;static gNPC_t npcData;
 subject.client=&npcClient;subject.NPC=&npcData;subject.health=100;
 linkEntity(dist);linkEntity(subject);CHECK(!strcmp(dist.NPC_target,subject.targetname));
 auto& navGoal=Make(55,"waypoint_navgoal");navGoal.model=G_NewString("@jar_reference");navGoal.radius=30;
 auto* const deathTarget=G_NewString("native_death_target");subject.target=deathTarget;
 output.integer=1;linkEntity(subject);linkEntity(navGoal);CHECK(!npcData.workshopGoalOwner);
 output.integer=0;linkEntity(subject);linkEntity(first);CHECK(!npcData.workshopGoalOwner);
 linkEntity(subject);linkEntity(navGoal);
 CHECK(G_WorkshopResolveNPCGoal(G_WorkshopReadNPCGoal(npcData))==&navGoal&&subject.target==deathTarget);
 linkEntity(subject);linkEntity(wa);
 CHECK(G_WorkshopResolveNPCGoal(G_WorkshopReadNPCGoal(npcData))==&wa&&subject.target==deathTarget);
 CHECK(G_WorkshopResolveNPCGoal(G_WorkshopNextNPCPatrol(wa))==&wb);
 output.integer=JAR_DISTANCE_INSIDE;linkEntity(trigger);linkEntity(dist);
 CHECK(status.find("activation loop")!=std::string::npos);
 auto& counter=Make(70,"target_counter");
 auto& timer=Make(69,"target_delay");
 auto& notice=Make(71,"target_print");
 counter.e_UseFunc=useF_target_counter_use;timer.e_UseFunc=useF_Use_Target_Delay;
 output.integer=1;
 linkEntity(counter);linkEntity(notice);CHECK(!counter.target2);
 notice.e_UseFunc=useF_Use_Target_Print;
 linkEntity(counter);linkEntity(notice);CHECK(!strcmp(counter.target2,notice.targetname));
 output.integer=0;
 linkEntity(counter);linkEntity(timer);CHECK(!strcmp(counter.target,timer.targetname));
 linkEntity(timer);linkEntity(counter);CHECK(!timer.target&&status.find("activation loop")!=std::string::npos);
 linkEntity(notice);linkEntity(counter);CHECK(!notice.target&&status.find("activation loop")!=std::string::npos);
 auto& selector=Make(73,"target_random");selector.e_UseFunc=useF_target_random_use;
 auto& ticker=Make(72,"func_timer");ticker.e_UseFunc=useF_func_timer_use;
 randomSlot.integer=4;linkEntity(selector);linkEntity(ticker);CHECK(!selector.target);
 randomSlot.integer=3;output.integer=3;linkEntity(selector);linkEntity(ticker);
 CHECK(selector.target4&&!strcmp(selector.target4,ticker.targetname));
 randomSlot.integer=1;linkEntity(selector);linkEntity(ticker);
 CHECK(!selector.target2&&status.find("already")!=std::string::npos);
 output.integer=0;linkEntity(ticker);linkEntity(selector);
 CHECK(!ticker.target&&status.find("activation loop")!=std::string::npos);
 randomSlot.integer=1;linkEntity(selector);linkEntity(notice);CHECK(!strcmp(selector.target2,notice.targetname));
 auto& relay=Make(74,"target_relay");relay.e_UseFunc=useF_target_relay_use;
 auto& score=Make(75,"target_score");score.e_UseFunc=useF_Use_Target_Score;
 auto& kill=Make(76,"target_kill");kill.e_UseFunc=useF_target_kill_use;
 linkEntity(relay);linkEntity(subject);CHECK(!relay.target);
 linkEntity(relay);linkEntity(score);CHECK(relay.target&&!strcmp(relay.target,score.targetname));
 auto& emitter=Make(81,"target_laser");emitter.model=G_NewString(jar::sandboxLaser::model);
 CHECK(G_SandboxLaserInitialize(emitter,{}));
 output.integer=1;linkEntity(emitter);linkEntity(relay);
 CHECK(!emitter.target&&status.find("Choose Primary")!=std::string::npos);
 output.integer=0;linkEntity(emitter);linkEntity(relay);linkEntity(relay);linkEntity(emitter);
 CHECK(G_SandboxLaserAim(emitter)==&relay&&!strcmp(relay.target,emitter.targetname));
 auto& beam=Make(82,"fx_target_beam");beam.e_UseFunc=useF_fx_target_beam_use;
 output.integer=1;linkEntity(beam);linkEntity(relay);
 CHECK(!beam.target&&status.find("Choose Primary")!=std::string::npos);
 output.integer=0;linkEntity(relay);linkEntity(beam);linkEntity(beam);linkEntity(relay);
 CHECK(!strcmp(relay.target,beam.targetname)&&!strcmp(beam.target,relay.targetname));
 CHECK(beam.e_ThinkFunc==thinkF_fx_target_beam_link&&beam.nextthink==level.time+FRAMETIME);
 linkEntity(relay);linkEntity(beam);
 CHECK(status.find("activation loop")==std::string::npos&&!strcmp(relay.target,beam.targetname));
 auto& trail=Make(83,"fx_explosion_trail");trail.e_UseFunc=useF_fx_explosion_trail_use;
 linkEntity(relay);linkEntity(trail);linkEntity(trail);linkEntity(relay);
 CHECK(!strcmp(relay.target,trail.targetname)&&!strcmp(trail.target,relay.targetname)&&!trail.nextthink);
 linkEntity(relay);linkEntity(trail);CHECK(status.find("activation loop")==std::string::npos);
 linkEntity(score);linkEntity(relay);CHECK(!score.target);
 linkEntity(kill);linkEntity(relay);CHECK(!kill.target&&status.find("receive activation")!=std::string::npos);
 auto& give=Make(77,"target_give");auto& secondGive=Make(78,"target_give");
 give.e_UseFunc=secondGive.e_UseFunc=useF_Use_Target_Give;
 auto& supply=Make(79,"ammo_blaster");
 static gitem_t ammo{};ammo.giType=IT_AMMO;supply.item=&ammo;supply.s.eType=ET_ITEM;
 supply.e_TouchFunc=touchF_Touch_Item;supply.contents=CONTENTS_ITEM;supply.e_ThinkFunc=thinkF_G_FreeEntity;
 gi.linkentity=[](gentity_t* e){e->linked=qtrue;};
 level.time=100;supply.nextthink=500;giveSlot.integer=3;output.integer=2;
 linkEntity(give);linkEntity(notice);CHECK(!give.target4);
 linkEntity(give);linkEntity(supply);CHECK(give.target4&&!strcmp(give.target4,supply.targetname)&&
  supply.nextthink==givePickup::reservedDeadline&&supply.linked);
 giveSlot.integer=2;linkEntity(give);linkEntity(supply);CHECK(!give.target3);
 linkEntity(secondGive);linkEntity(supply);CHECK(secondGive.target3);
 setSlottedLink(give,3,nullptr);CHECK(supply.nextthink==givePickup::reservedDeadline);
 level.time=800;Forget(state,78);givePickup::sync(supply.targetname);CHECK(supply.nextthink==1200);
 // An older owned Give link can still reference a rejected pickup hidden by the stock callback.
 G_LogicSetOutput(give,0,supply.targetname);supply.linked=qfalse;supply.nextthink=0;
 givePickup::restore();CHECK(supply.linked&&supply.nextthink==givePickup::reservedDeadline);
 setSlottedLink(give,0,nullptr);CHECK(supply.nextthink==30800);
 // Unrelated dropped pickups and pickup-specific callbacks keep their original schedules.
 supply.e_ThinkFunc=thinkF_NULL;supply.nextthink=-1;
 setSlottedLink(give,0,supply.targetname);setSlottedLink(give,0,nullptr);
 CHECK(supply.nextthink==-1&&supply.e_ThinkFunc==thinkF_NULL);
 auto& enable=Make(84,"target_activate");enable.e_UseFunc=useF_target_activate_use;
 auto& disable=Make(85,"target_deactivate");disable.e_UseFunc=useF_target_deactivate_use;
 auto& gateTrigger=Make(86,"trigger_multiple");gateTrigger.e_UseFunc=useF_Use_Multi;
 auto& hurt=Make(87,"trigger_hurt");hurt.e_TouchFunc=touchF_hurt_touch;
 gateSlot.integer=3;output.integer=3;
 linkEntity(disable);linkEntity(supply);CHECK(!disable.target4);
 linkEntity(disable);linkEntity(gateTrigger);CHECK(disable.target4&&!strcmp(disable.target4,gateTrigger.targetname));
 output.integer=0;linkEntity(gateTrigger);linkEntity(disable);
 CHECK(gateTrigger.target&&!strcmp(gateTrigger.target,disable.targetname));
 gateSlot.integer=1;linkEntity(enable);linkEntity(hurt);
 CHECK(enable.target2&&!strcmp(enable.target2,hurt.targetname));
 linkEntity(enable);linkEntity(disable);CHECK(enable.target2&&!strcmp(enable.target2,disable.targetname));
 linkEntity(disable);linkEntity(enable);CHECK(disable.target2&&!strcmp(disable.target2,enable.targetname));
 auto& push=Make(90,"target_push");push.model=G_NewString(jar::push::markerModel);push.e_UseFunc=useF_Use_target_push;
 auto& pushRelay=Make(91,"target_relay");pushRelay.e_UseFunc=useF_target_relay_use;
 output.integer=0;linkEntity(push);linkEntity(pushRelay);CHECK(push.target&&!push.nextthink);
 linkEntity(pushRelay);linkEntity(push);CHECK(pushRelay.target&&!strcmp(pushRelay.target,push.targetname));
 auto& tele=Make(92,"target_teleporter");tele.model=G_NewString(jar::teleport::markerModel);tele.e_UseFunc=useF_target_teleporter_use;
 auto& exit=Make(93,"misc_teleporter_dest");output.integer=0;
 linkEntity(tele);linkEntity(exit);CHECK(G_TeleportDestination(tele)==&exit);
 auto& usable=Make(94,"func_usable");usable.model=G_NewString("@jar_box");usable.s.modelindex=5;
 CHECK(G_SandboxUsableInitialize(usable));
 linkEntity(usable);linkEntity(notice);CHECK(usable.target&&!strcmp(usable.target,notice.targetname));
 output.integer=1;linkEntity(usable);linkEntity(pushRelay);CHECK(usable.target2&&!strcmp(usable.target2,pushRelay.targetname));
 output.integer=0;linkEntity(pushRelay);linkEntity(usable);CHECK(status.find("activation loop")!=std::string::npos);
 });}
