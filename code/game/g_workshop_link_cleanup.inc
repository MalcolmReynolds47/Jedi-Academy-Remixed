// Retire Workshop links when their native destination is removed. GPL-2.0-only.
#include "g_button_sandbox.h"
#include "g_distance_trigger.h"
#include "g_level_change.h"
#include "g_scene_camera.h"
#include "g_sliding_door.h"
#include "g_spotlight.h"
#include "g_waypoints.h"
#include "g_workshop_fx_aim.h"

bool forgetWorkshopReferences(gentity_t& removed,unsigned selectedSlot){
 if(G_WorkshopTrailClass(removed))for(int n=MAX_CLIENTS;n<globals.num_entities;++n){
  auto& flight=g_entities[n];
  if(flight.inuse&&flight.owner==&removed&&flight.e_ThinkFunc==thinkF_fx_explosion_trail_think)
   flight.owner=nullptr;
 }
 bool refresh=false,checkedName=false,hasOtherTarget=false;
 const auto vanished=[&](const char* name){
  if(!name||!removed.targetname||Q_stricmp(name,removed.targetname))return false;
  if(!checkedName){
   checkedName=true;
   const auto* other=G_Find(nullptr,FOFS(targetname),removed.targetname);
   if(other==&removed)other=G_Find(&removed,FOFS(targetname),removed.targetname);
   hasOtherTarget=other!=nullptr;
  }
  return !hasOtherTarget;
 };
 for(const auto& entry:state.owned){
  if(entry.entity>=ENTITYNUM_WORLD||entry.entity==unsigned(removed.s.number))continue;
  auto& source=g_entities[entry.entity];
  const bool pointer=source.enemy==&removed||source.owner==&removed||source.activator==&removed;
  const bool named=removed.targetname&&(source.target||source.target2||source.target3||source.target4||
    source.paintarget||source.ownername||source.NPC_target);
  if((!pointer&&!named)||!matches(entry))continue;
  if(source.enemy==&removed)source.enemy=nullptr;
  if(source.owner==&removed)source.owner=nullptr;
  const auto logic=G_LogicKind(source);
  if(source.activator==&removed&&(logic==jar::logic::Delay||logic==jar::logic::Timer||logic==jar::logic::Relay||
     G_ContactPending(source)||G_SandboxUsable(source)||G_WorkshopBeamClass(source)))source.activator=nullptr;

  bool changed=false;
  if(G_DistancePortable(source)){
   for(int role=0;role<JAR_DISTANCE_LINKS;++role)if(vanished(G_DistanceLink(source,role))){
    // An explicitly linked reference must not fall back to the trigger origin.
    G_DistanceSetLink(source,role,role==JAR_DISTANCE_REFERENCE?JAR_DISTANCE_MISSING_REFERENCE_NAME:nullptr);
    changed=true;
   }
  }else{
   const bool slotted=jar::logic::Slotted(logic);
   const bool secondary=G_SandboxSlidePortable(source)||G_ButtonPortable(source)||G_SpotlightPortable(source)||
     G_WaypointPortable(source)||G_SceneCameraPortable(source)||G_SandboxUsable(source)||logic==jar::logic::Counter;
   const int outputs=slotted?jar::logic::outputCount:secondary?2:1;
   for(int slot=0;slot<outputs;++slot){
    if((slot==0&&jar::levelChange::Class(source.classname))||!vanished(G_LogicOutput(source,slot)))continue;
    G_LogicSetOutput(source,slot,nullptr);
    if(slot==0){
     if(G_SandboxLaser(source))G_SandboxLaserAimChanged(source);
     if(G_WorkshopBeamClass(source))G_WorkshopBeamIdle(source);
     else if(source.e_ThinkFunc==thinkF_AimAtTarget){
      source.e_ThinkFunc=thinkF_NULL;source.nextthink=0;
     }
    }
    changed=true;
   }
   if(source.classname&&!strcmp(source.classname,"trigger_bidirectional")&&vanished(source.paintarget)){
    if(gi.bIsFromZone(source.paintarget,TAG_G_ALLOC))gi.Free(source.paintarget);
    source.paintarget=nullptr;
    changed=true;
   }
  }
  refresh|=changed&&entry.entity==selectedSlot;
 }
 return refresh;
}
