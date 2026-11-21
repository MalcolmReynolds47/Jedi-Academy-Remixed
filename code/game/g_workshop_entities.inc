#include "g_spawn_context.h"
#include "g_spawn_transaction.h"
#include "g_sandbox_push.h"
#include "g_sandbox_teleport.h"
#include "g_sandbox_laser.h"
#include "g_sandbox_station.h"
#include "g_sandbox_breakable.h"
#include "g_sandbox_usable.h"
#include "g_sandbox_speaker.h"
#include "g_sandbox_angular.h"
#include "g_workshop_fx_aim.h"
#include "qcommon/jar_replicator.h"
// Portable setups use native callbacks and native saved fields. GPL-2.0-only.
// Included by g_spawn.cpp, where the spawn-key parser is available.
#include "qcommon/jar_entity_presets.h"
#include "g_maglock.h"
#include "g_reference_tags.h"
#include "g_combat_points.h"
#include "g_waypoints.h"
#include "g_skyportal.h"
#include "g_reflection_probe.h"
#include "g_scene_camera.h"
#include "g_rail_portable.h"
#include "g_portal.h"
#include "qcommon/jar_music.h"
#include "g_weather.h"
#include "qcommon/jar_scriptrunner.h"

namespace {
struct SetupChildren {
 gentity_t *parent,**helpers;int&count;bool failed=false;
 SetupChildren*previous;
 static SetupChildren*active;
 SetupChildren(gentity_t*p,gentity_t**h,int&n):parent(p),helpers(h),count(n),previous(active){active=this;}
 ~SetupChildren(){active=previous;}
};
SetupChildren*SetupChildren::active=nullptr;
}
#include "g_start_marker.h"
#include "g_wall_sandbox.h"
#include "g_sliding_door.h"
#include "g_button_sandbox.h"
#include "g_bobbing_sandbox.h"
#include "g_movement_settings.h"
#include "g_contact_trigger.h"
// Only the currently spawning portable parent captures native allocations.
// Ordinary map spawns keep their native children and graceful pool exhaustion.
gentity_t*G_JAR_SpawnSetupChild(gentity_t*parent){
 auto*c=SetupChildren::active;
 if(c&&c->parent==parent){
  if(c->count==jar::entities::helperLimit){c->failed=true;return nullptr;}
  auto*e=G_TrySpawn();if(!e){c->failed=true;return nullptr;}
  c->helpers[c->count++]=e;return e;
 }
 return G_TrySpawn();
}
extern void spawn_rack_goods(gentity_t*);

void G_JAR_SetBrushModel(gentity_t*ent,const char*model){
 if(model&&!strcmp(model,JAR_WALL_PORTABLE_MODEL)){
  ent->bmodel=qfalse;ent->s.modelindex=G_ModelIndex(JAR_WALL_MODEL);
  G_WallHull(ent->currentAngles,ent->mins,ent->maxs);
  ent->contents=CONTENTS_SOLID;return;
 }
 if(model && !strcmp(model,"@jar_box")){
  ent->bmodel=qfalse;ent->s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");
  VectorSet(ent->mins,-21,-21,0);VectorSet(ent->maxs,21,21,22);
  ent->contents=CONTENTS_SOLID;return;
 }
 gi.SetBrushModel(ent,model);
}

#include "g_workshop_placement.h"
#include "g_siege_item.h"
bool G_JAR_SpawnWorkshopEntity(gentity_t*ent,gentity_t**helpers,int&helperCount,bool pastePortal=false){
 using namespace jar::entities;
 helperCount=0;
 if(jar::bspSection::Class(ent->classname))return G_BSPSectionSpawn(ent);
 SetupChildren children(ent,helpers,helperCount);
 const Preset*p=Find(ent->classname);
 if(jar::weather::ZoneClass(ent->classname)&&G_WeatherZoneCount(ent)>=jar::weather::zoneLimit)return false;
 gentity_t*maglockDoor=nullptr;
 G_SpawnContext scope;
 auto key=[&](const char*k,const char*v){AddSpawnField(const_cast<char*>(k),const_cast<char*>(v));};
 auto field=[&](const char*k,const char*v){key(k,v);G_ParseField(k,v,ent);};
 auto helper=[&](const char*classname,float distance,float up,float side=0,const char*feedback="Sandbox target activated")->gentity_t*{
  auto*h=G_JAR_SpawnSetupChild(ent);if(!h)return nullptr;
  h->classname=G_NewString(classname);h->targetname=G_NewString(va("jar_setup_%d_%d_%d",ent->s.number,level.time,helperCount));
  vec3_t forward,pos;AngleVectors(ent->currentAngles,forward,nullptr,nullptr);VectorMA(ent->currentOrigin,distance,forward,pos);vec3_t axis[3];AnglesToAxis(ent->currentAngles,axis);if(p&&(p->flags&Rail))VectorMA(pos,up,axis[2],pos);else pos[2]+=up;VectorMA(pos,side,axis[1],pos);
  vec3_t lo={-21,-21,0},hi={21,21,22};
  if(!strcmp(classname,"target_position")){VectorSet(lo,-16,-16,-24);VectorSet(hi,16,16,40);}
  trace_t space{};G_JAR_WorkshopSceneryTrace(space,pos,pos,lo,hi);
  if(space.startsolid || space.allsolid)return nullptr;
  G_SetOrigin(h,pos);VectorCopy(pos,h->s.origin);G_SetAngles(h,ent->currentAngles);VectorCopy(ent->currentAngles,h->s.angles);
  h->s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");h->model=G_NewString(!strcmp(classname,"func_door")?"@jar_box":"models/map_objects/imperial/crate1.md3");
  VectorSet(h->mins,-21,-21,0);VectorSet(h->maxs,21,21,22);h->contents=CONTENTS_ITEM;
  if(!strcmp(classname,"item_security_key")){h->message=G_NewString("jar_workshop_key");if(!G_CallSpawn(h))return nullptr;}
  else if(!strcmp(classname,"target_print")){h->message=G_NewString(feedback);SP_target_print(h);}
  else if(!strcmp(classname,"path_corner")){h->wait=1;SP_path_corner(h);}
  else if(jar::rail::Class(classname)!=jar::rail::None){if(h->model&&gi.bIsFromZone(h->model,TAG_G_ALLOC))gi.Free(h->model);h->model=G_NewString("@jar_rail");if(!G_RailSpawn(*h))return nullptr;}
  else if(jar::sceneCamera::Class(classname)){if(h->model&&gi.bIsFromZone(h->model,TAG_G_ALLOC))gi.Free(h->model);h->model=G_NewString("@jar_scene_camera");if(!G_SceneCameraSpawn(*h))return nullptr;h->s.modelindex=G_ModelIndex("models/map_objects/kejim/impcam.md3");}
  else if(!strcmp(classname,"func_door")){
   h->spawnflags=MOVER_PLAYER_USE;h->speed=100;h->wait=1;SP_func_door(h);
  }else SP_target_position(h);
  gi.linkentity(h);return h;
 };
 auto fail=[&](){while(helperCount){auto*h=helpers[--helperCount];if(h->inuse)G_FreeEntity(h);}return false;};
 if(!strcmp(ent->classname,jar::siege::itemClass)){
  auto* goal=helper(jar::siege::objectiveClass,96,0);
  if(!goal)return fail();
  if(goal->model&&gi.bIsFromZone(goal->model,TAG_G_ALLOC))gi.Free(goal->model);
  goal->model=G_NewString("models/map_objects/mp/flag_base.md3");
  goal->spawnflags|=jar::siege::requiresItem;
  SP_info_siege_objective(goal);
  ent->target=G_NewString(goal->targetname);
  ent->spawnflags|=jar::siege::immediate;
  key("respawnfx",jar::siege::crystalReturn);
  if(ent->mins[0]<ent->maxs[0]&&ent->mins[1]<ent->maxs[1]&&ent->mins[2]<ent->maxs[2]){
   key("mins",va("%g %g %g",ent->mins[0],ent->mins[1],ent->mins[2]));
   key("maxs",va("%g %g %g",ent->maxs[0],ent->maxs[1],ent->maxs[2]));
  }
  SP_misc_siege_item(ent);
  return ent->inuse?true:fail();
 }
 if(p){
  const char*name=p->name;
  if(p->flags&Rail){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_rail");
   const auto kind=jar::rail::Class(name);gentity_t*parts[4]{};parts[kind]=ent;
   const float along[]={0,0,0,64},side[]={0,0,64,64},up[]={0,0,0,32};
   const char*names[]={"","rail_track","rail_lane","rail_mover"};
   for(int k=1;k<=3;++k)if(k!=kind){parts[k]=helper(names[k],along[k]-along[kind],up[k]-up[kind],side[k]-side[kind]);if(!parts[k])return fail();}
   parts[jar::rail::Lane]->target=G_NewString(parts[jar::rail::Track]->targetname);
   parts[jar::rail::Mover]->target=G_NewString(parts[jar::rail::Lane]->targetname);
  }
  if(jar::sceneCamera::Class(name)){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_scene_camera");}
  if(jar::sceneCamera::Focus(name)){auto*h=helper("misc_camera_track",-128,48);if(!h)return fail();ent->target2=G_NewString(h->targetname);h->target2=G_NewString(ent->targetname);}
  if(jar::portal::SurfaceClass(name)||jar::portal::CameraClass(name)){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString(jar::portal::SurfaceClass(name)?"@jar_portal_surface":"@jar_portal_camera");}
  if(!strcmp(name,"misc_spotlight")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_spotlight");auto*h=helper("target_print",-64,24);if(!h)return fail();ent->target2=G_NewString(h->targetname);}
  if(!strcmp(name,"misc_crystal_crate")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_crystal_crate");}
  if(!strcmp(name,"misc_model_jabba_cam")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_door_camera");}
  if(!strcmp(name,"misc_model_welder")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_welder");}
  if(!strcmp(name,"misc_laser_arm")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_laser_arm");}
  if(!strcmp(name,"misc_maglock")){maglockDoor=helper("func_door",-21,-12);if(!maglockDoor)return fail();}
  if(p->flags&Directional){
   auto*a=helper("target_print",128,0);auto*b=helper("target_print",-128,0);if(!a||!b)return fail();
   a->message=G_NewString("Forward crossing");b->message=G_NewString("Reverse crossing");ent->target=G_NewString(a->targetname);ent->paintarget=G_NewString(b->targetname);
  }else if(!strcmp(name,"trigger_entdist")){
   auto* inside=helper("target_print",72,16,48,"Distance trigger: inside range");if(!inside)return fail();
   auto* outside=helper("target_print",72,16,-48,"Distance trigger: outside range");if(!outside)return fail();
   ent->target=G_NewString(inside->targetname);ent->target2=G_NewString(outside->targetname);
  }else if(!strcmp(name,"trigger_shipboundary")){
   auto*h=helper("target_position",1024,256);if(!h)return fail();ent->target=G_NewString(h->targetname);
  }else if(p->flags&Train){
   VectorCopy(ent->currentOrigin,ent->pos1);VectorCopy(ent->currentOrigin,ent->pos2);
   auto*a=helper("path_corner",0,0);auto*b=helper("path_corner",128,0);if(!a||!b)return fail();
   a->target=G_NewString(b->targetname);if(!jar::sceneCamera::Track(name))b->target=G_NewString(a->targetname);ent->target=G_NewString(a->targetname);
  }else if(jar::teleport::Class(name)){
   auto*h=helper("misc_teleporter_dest",128,0);if(!h)return fail();
   h->message=G_NewString("Teleport destination");ent->target=G_NewString(h->targetname);
  }else if(p->flags&(Position|Feedback)){
   auto*h=helper(p->flags&Position?"target_position":"target_print",128,p->flags&Position?96:0);if(!h)return fail();ent->target=G_NewString(h->targetname);
  }
  if(p->flags&Brush){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);
   ent->model=G_NewString(JAR_WallClass(name)?JAR_WALL_PORTABLE_MODEL:G_SandboxSlideClass(name)?JAR_SLIDING_DOOR_PORTABLE:G_LiftClass(name)?JAR_LIFT_PORTABLE:G_ButtonClass(name)?JAR_BUTTON_PORTABLE_MODEL:G_BobbingClass(name)?JAR_BOB_PORTABLE:jar::sandboxAngular::Class(name)?jar::sandboxAngular::model:"@jar_box");
  }
  field("message",JAR_StartCard(name)?(!strcmp(name,"info_player_start")?"Player start":"Deathmatch start"):
   !strcmp(name,"target_location")?"Sandbox location":!strcmp(name,"misc_teleporter_dest")?"Teleport destination":"Sandbox target activated");
  if(!strcmp(name,"target_interest")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_interest");}
  if(jar::speaker::Class(name)){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);
   ent->model=G_NewString(jar::speaker::model);
  }
  if(!strcmp(name,"trigger_always")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_always");}
  if(!strcmp(name,"trigger_visible")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_visible");}
  if(JAR_StartCard(name)){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);
   ent->model=G_NewString("@jar_start");ent->e_UseFunc=useF_portable_start_use;
  }
  field("health","40");field("speed","100");field("wait","1");
  key("gravity","400");key("noise","sound/weapons/bryar/fire.mp3");key("dmg","0");
  if(!strcmp(name,"target_change_parm"))field("parm1","Sandbox");
  if(!strcmp(name,"target_scriptrunner")){field("usescript",jar::scriptRunner::defaultScript);ent->count=-1;ent->spawnflags=1;key("delay","0");}
  if(!strcmp(name,"target_level_change")){field("message",level.mapname);ent->count=-1;ent->spawnflags=0;if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_level_change");}
  if(!strcmp(name,"target_play_music")){key("music",jar::music::defaultTrack);if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_music");}
  if(const auto movement=G_MovementMarkerClass(name)){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);
   ent->model=G_NewString(JAR_MOVEMENT_PORTABLE);
   if(movement==jar::movement::Friction)ent->health=jar::movement::defaultFriction;
  }
  if(!strcmp(name,"target_counter"))ent->count=2;
  if(!strcmp(name,"func_timer"))key("random","0");
  if(!strcmp(name,"trigger_shipboundary")){ent->spawnflags=1;key("traveltime","3000");}
  if(!strcmp(name,"trigger_entdist")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_entdist");}
  if(!strcmp(name,"misc_model_gun_rack"))ent->spawnflags=1|2|4;
  if(!strcmp(name,"misc_model_ammo_rack"))ent->spawnflags=1|2|8|16|32;
  if(!strcmp(name,"misc_camera"))ent->spawnflags=1;
  if(!strcmp(name,"target_score"))ent->count=1;
  if(!strcmp(name,"target_push")){ent->speed=600;ent->spawnflags=2;}
  if(!strcmp(name,"func_door")||!strcmp(name,"func_button"))ent->spawnflags|=MOVER_PLAYER_USE;
  if(!strcmp(name,"trigger_push"))ent->spawnflags=4;
  if(!strcmp(name,"trigger_hurt")){ent->damage=5;ent->spawnflags=4;}
  if(!strcmp(name,"func_glass"))key("material","1");
  if(!strcmp(name,"light")||!strcmp(name,"misc_dlight"))field("startRGBA","0.5 0.7 1 160");
  if(jar::sandboxLaser::Class(name)){
   if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);
   ent->model=G_NewString(jar::sandboxLaser::model);
  }
  if(jar::weather::Class(name)!=jar::weather::Invalid&&!jar::weather::ZoneClass(name)){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_weather");}
  if(jar::reflection::Class(name)){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_reflection");key("radius","512");}
  if(!strcmp(name,"fx_cloudlayer")){if(ent->model&&gi.bIsFromZone(ent->model,TAG_G_ALLOC))gi.Free(ent->model);ent->model=G_NewString("@jar_cloud");key("radius","512");key("random","128");key("wait","0");}
  if(!strcmp(name,"fx_runner")){field("fxFile","sparks/spark");key("delay","500");}
  if(!strcmp(name,"fx_explosion_trail"))key("fxFile","sparks/spark");
  if(!strcmp(name,"fx_target_beam"))ent->speed=.2f;
  if(!strcmp(name,"shooter_rocket"))ent->paintarget=G_NewString("WP_ROCKET_LAUNCHER");
  if(!strcmp(name,"shooter_grenade"))ent->paintarget=G_NewString("WP_THERMAL");
  if(!strcmp(name,"shooter_plasma"))ent->paintarget=G_NewString("WP_BRYAR_PISTOL");
  if(!strcmp(p->spawn,"misc_weapon_shooter"))ent->wait=500;
  if(!strcmp(name,"misc_model_cargo_small"))ent->spawnflags=1;
  if(!strcmp(name,"misc_turret")||!strcmp(name,"misc_ns_turret"))ent->wait=200;
  if(!strcmp(name,"misc_ion_cannon"))ent->wait=1500;
  if(!strcmp(name,"misc_security_panel")){
   auto*h=helper("item_security_key",64,24);if(!h)return fail();if(ent->message && gi.bIsFromZone(ent->message,TAG_G_ALLOC))gi.Free(ent->message);ent->message=G_NewString("jar_workshop_key");
  }
  if(!strcmp(name,"misc_trip_mine")){
   vec3_t forward,end;AngleVectors(ent->s.angles,forward,nullptr,nullptr);VectorMA(ent->currentOrigin,128,forward,end);
   trace_t hit;gi.trace(&hit,ent->currentOrigin,nullptr,nullptr,end,ent->s.number,MASK_SHOT,(EG2_Collision)0,0);
   if(hit.startsolid || hit.allsolid || hit.fraction>=1)return fail();
   ent->spawnflags=1;
  }
 }
 char*original=ent->classname;
 if(p && p->spawn[0])ent->classname=const_cast<char*>(p->spawn);
 bool result=false;
 try{
 if(p&&jar::speaker::Class(p->name))result=G_SandboxSpeakerInit(*ent,{});
 else if(G_SandboxAngular(*ent))result=G_SandboxAngularClear(*ent,ent->currentOrigin,ent->currentAngles)&&
  G_SandboxAngularInit(*ent,jar::sandboxAngular::Defaults(jar::sandboxAngular::Class(ent->classname)));
 else if(G_PortalPortable(*ent))result=pastePortal?G_PortalSpawnMember(*ent):G_PortalSpawnPair(ent);
 else result=G_BobbingPortable(*ent)?G_BobbingSpawn(*ent):G_ButtonPortable(*ent)?G_ButtonSpawn(*ent):G_SandboxSlidePortable(*ent)?G_SandboxSlideSpawn(*ent):G_RailPortable(*ent)?G_RailSpawn(*ent):jar::waypoint::Class(ent->classname)?G_WaypointSpawn(*ent):G_SceneCameraPortable(*ent)?G_SceneCameraSpawn(*ent):jar::skyPortal::Class(ent->classname)?G_SkyPortalSpawn(*ent):!strcmp(ent->classname,"point_combat")?G_CombatPointSpawn(*ent):jar::reference::Class(ent->classname)?G_ReferenceSpawn(*ent):(p && !p->spawn[0]) || G_CallSpawn(ent)!=qfalse;
 if(result && ent->inuse && p && !strcmp(p->name,"misc_model_ammo_rack")){
  // Complete the native delayed fill inside this ownership transaction.
  ent->nextthink=0;ent->e_ThinkFunc=thinkF_NULL;spawn_rack_goods(ent);
 }
 }catch(const G_SpawnCapacity&){
  if(ent->inuse)ent->classname=original;
  else if(original && gi.bIsFromZone(original,TAG_G_ALLOC))gi.Free(original);
  throw;
 }
 if(!result || !ent->inuse || children.failed){
  if(ent->inuse)ent->classname=original;
  else if(original && gi.bIsFromZone(original,TAG_G_ALLOC))gi.Free(original);
  return fail();
 }
 ent->classname=original;
 if(jar::contactTrigger::Class(ent->classname)&&!G_ContactInitialize(*ent))return fail();
 if(jar::push::Class(ent->classname)&&!G_PushInitialize(*ent))return fail();
 if(jar::teleport::Class(ent->classname)&&!G_TeleportInitialize(*ent))return fail();
 if(jar::sandboxStation::Class(ent->classname)&&!G_SandboxStationInitialize(*ent,1))return fail();
 if(jar::sandboxBreakable::Class(ent->classname)){
  ent->svFlags|=SVF_PLAYER_USABLE;
  if(!G_SandboxBreakableInitialize(*ent))return fail();
 }
 if(jar::sandboxUsable::Class(ent->classname)&&!G_SandboxUsableInitialize(*ent))return fail();
 if(maglockDoor&&!G_MaglockAttach(ent,maglockDoor))return fail();
 // Native assemblies can protrude beyond their root. Check their final
 // bounds against the BSP while allowing objects to share space.
 bool blocked=false;
 if(p&&!strcmp(p->name,"misc_crystal_crate")){trace_t space{};G_JAR_WorkshopSceneryTrace(space,ent->currentOrigin,ent->currentOrigin,ent->mins,ent->maxs,ent->s.number);blocked=space.startsolid||space.allsolid;}
 for(int i=0;i<helperCount;++i){auto*h=helpers[i];trace_t space{};
  if(!h->inuse){blocked=true;break;}
  G_JAR_WorkshopSceneryTrace(space,h->currentOrigin,h->currentOrigin,h->mins,h->maxs,h->s.number);
  if(space.startsolid||space.allsolid){blocked=true;break;}
 }
 if(blocked)return fail();
 if(p && ent->fxFile){char*old=ent->fxFile;ent->fxFile=G_NewString(old);if(gi.bIsFromZone(old,TAG_G_ALLOC))gi.Free(old);}
 if(p){
  if(G_WorkshopBeamClass(*ent))G_WorkshopBeamEnableUse(*ent);
  if(G_WorkshopTrailClass(*ent))G_WorkshopTrailEnableUse(*ent);
  // Compile-time-only model entities become ordinary removable native models.
  if(!p->spawn[0]){ent->s.eType=ET_GENERAL;ent->s.modelindex=G_ModelIndex(ent->model);ent->contents=CONTENTS_SOLID;}
  if(p->flags&Marker){
   ent->svFlags&=~SVF_NOCLIENT;
   if(!ent->s.modelindex)ent->s.modelindex=G_ModelIndex(ModelPath(*p));
   if(!ent->contents)ent->contents=CONTENTS_ITEM;
   if(!strcmp(p->name,"misc_replicator_item")){
    ent->s.eFlags&=~EF_NODRAW;
    ent->damage=jar::replicator::defaultHeal;
    ent->delay=jar::replicator::defaultDelay;
    ent->wait=jar::replicator::defaultDelay;
   }
  }
  if(ent->e_UseFunc!=useF_NULL&&!G_ContactKind(*ent)&&!G_SandboxBreakable(*ent)&&!G_SandboxUsable(*ent))
   ent->svFlags|=SVF_PLAYER_USABLE;
  gi.linkentity(ent);
 }
 return true;
}
