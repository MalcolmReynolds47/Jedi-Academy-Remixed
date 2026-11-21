#include "g_sliding_door.h"
#include "g_button_sandbox.h"
#include "g_logic_settings.h"
#include "g_contact_trigger.h"
#include "g_sandbox_push.h"
#include "g_sandbox_teleport.h"
#include "g_sandbox_laser.h"
#include "g_sandbox_breakable.h"
#include "g_sandbox_usable.h"
#include "g_workshop_fx_aim.h"
void linkEntity(gentity_t&e){
 auto*target=owned(e.s.number);
 if(!target||!matches(*target)){message("Link uses spawned entities so map mission wiring stays intact.");return;}
 if(!linkFirst){linkFirst=target->id;message("Source selected. Left click a spawned destination; right cancels.");return;}
 const auto* source=OwnedId(state,linkFirst);
 linkFirst=0;if(!source||!matches(*source)||source->entity==unsigned(e.s.number)){message("Choose two different spawned entities.");return;}
 if(!e.targetname)e.targetname=G_NewString(va("jar_sb_%u",target->id));
 auto&from=g_entities[source->entity];
 const int role=gi.cvar("jar_linkOutput","0",CVAR_ARCHIVE|CVAR_NORESTART)->integer;
 if(from.NPC){
  jar::npcOrder::Settings order;
  const jar::npcOrder::Destination goal{target->id,unsigned(e.s.number)};
  if(role!=0||!contextActorOrder(from,order)||!G_WorkshopResolveNPCGoal(goal)){
   message("Use Primary from a compatible Sandbox NPC to a spawned Nav goal or waypoint.");return;
  }
  editContextActorGoal(from,goal);
  message("NPC destination linked. Choose Go to goal or Patrol route in NPC orders.");return;
 }
 if(G_SandboxReplicator(from)){message("Link another controller to the Replicator to activate it.");return;}
 if(G_PushKind(from)){
  if(role!=0){message("Choose Primary to connect a Push aim target.");return;}
  editPushAimLink(from,e.targetname);message("Push aim linked. Move the target to redirect it.");return;
 }
 if(G_TeleportKind(from)){
  if(role!=0){message("Choose Primary to connect a teleport destination.");return;}
  editTeleportLink(from,e.targetname);message("Teleport destination linked.");return;
 }
 if(G_SandboxLaser(from)){
  if(role!=0){message("Choose Primary to connect a laser aim target.");return;}
  editLaserEmitterAim(from,e.targetname);message("Laser aim linked. Move the target to redirect the beam.");return;
 }
 if(G_WorkshopBeamClass(from)){
  if(role!=0){message("Choose Primary to connect a beam destination.");return;}
  G_LogicSetOutput(from,0,e.targetname);from.enemy=nullptr;
  from.e_ThinkFunc=thinkF_fx_target_beam_link;from.nextthink=level.time+FRAMETIME;
  message("Beam destination linked. It follows the target's current position.");return;
 }
 if(G_WorkshopTrailClass(from)){
  if(role!=0){message("Choose Primary to connect an Explosion Trail aim target.");return;}
  editExplosionTrailAim(from,e.targetname);
  message("Explosion Trail aims at the target's current position on Use.");return;
 }
 if(G_PortalPortable(from)){
  if(role!=0){message("Choose Primary to connect a portal camera or aim target.");return;}
  if(!G_PortalLinkMatches(from,&e,e.targetname)){
   message(G_PortalSurface(from)?"Portal surfaces link to a spawned portal camera.":"Choose a live aim target.");return;
  }
  editPortalLink(from,&e);
  message(G_PortalSurface(from)?"Portal camera linked.":"Portal camera aim linked.");return;
 }
 const bool distance=G_DistancePortable(from);
 const auto logic=G_LogicKind(from);
 const bool slotted=jar::logic::Slotted(logic);
 const bool gate=jar::logic::UseGate(logic);
 if(G_ContactKind(from)==jar::contactTrigger::Hurt){
  message("Hurt triggers receive activation to toggle their damage. Choose another source.");return;
 }
 if(logic==jar::logic::Score||logic==jar::logic::Kill){
  message("Score and Kill receive activation. Select a trigger or relay as the source.");return;
 }
 if(!slotted&&role>=JAR_DISTANCE_REFERENCE){
  if(!distance||role>=JAR_DISTANCE_LINKS){message("Reference and NPC inputs are for distance triggers. Choose Primary for other entities.");return;}
  if(role==JAR_DISTANCE_SUBJECT&&(!e.client||!e.NPC||e.health<=0)){message("Choose a living spawned NPC or vehicle for the NPC input.");return;}
  distanceLink(from,role,e.targetname);
  message(role==JAR_DISTANCE_REFERENCE?"Distance reference linked. Its current position is used.":"NPC linked. Choose NPC or Either in Context > Distance.");
  return;
 }
 if(distance&&e.e_UseFunc==useF_NULL){message("Choose an entity with an activation action for the distance output.");return;}
 if(from.classname&&!strcmp(from.classname,"target_level_change")){message("Set the destination map and spawn point in Context > Travel. Link can activate this controller from another source.");return;}
 if((from.classname&&!strcmp(from.classname,"func_train")) || (from.classname&&!strcmp(from.classname,"path_corner"))){message("Train route: move its two supplied path corners with the Physgun. Link cannot replace a train route.");return;}
 if(G_RailPortable(from)){
  if(!G_RailPortable(e)||!jar::rail::Link(jar::rail::Class(from.classname),jar::rail::Class(e.classname))){message("Rail lanes link to tracks; movers link to a track or lane.");return;}
  if(from.target&&gi.bIsFromZone(from.target,TAG_G_ALLOC))gi.Free(from.target);from.target=G_NewString(e.targetname);
  if(jar::rail::Class(from.classname)==jar::rail::Mover){G_SetOrigin(&from,from.currentOrigin);from.count=0;}
  message("Rail route linked. Activate toggles traffic; Context > Rail edits its properties.");return;
 }
 if(G_WaypointPortable(from)){
  if(!G_WaypointPortable(e)){message("Waypoint routes link to another spawned waypoint.");return;}
  if(DistanceSquared(from.currentOrigin,e.currentOrigin)>4096.f*4096.f){message("Waypoint links reach up to 4096 units. Add intermediate points.");return;}
  const bool secondary=gi.cvar("jar_linkOutput","0",CVAR_ARCHIVE|CVAR_NORESTART)->integer==1;
  char*&output=secondary?from.target2:from.target;if(output&&gi.bIsFromZone(output,TAG_G_ALLOC))gi.Free(output);output=G_NewString(e.targetname);
  message("Navigation link created. Traversal checks walls, clearance and floor support.");return;
 }
 if(G_SceneCameraPortable(from)){
  const bool secondary=gi.cvar("jar_linkOutput","0",CVAR_ARCHIVE|CVAR_NORESTART)->integer==1;
  if(jar::sceneCamera::Track(from.classname)&&!secondary){message("Camera route: move its supplied path corners. Choose Secondary to link its focus.");return;}
  if(jar::sceneCamera::Focus(from.classname)&&secondary){message("Focus Primary links its subject. Activate uses its owned camera.");return;}
  char*&output=secondary?from.target2:from.target;if(output&&gi.bIsFromZone(output,TAG_G_ALLOC))gi.Free(output);output=G_NewString(e.targetname);
  message(secondary?"Camera focus linked. Activate the camera to view.":"Focus subject linked; the camera follows its current position.");return;
 }
 // A use chain must not recurse through itself. Native train paths are the
 // one cyclic graph: they advance on later simulation frames, not recursively.
 std::vector<const gentity_t*> pending{&e};std::vector<int> visited;
 for(size_t at=0;!gate&&at<pending.size();++at){const auto*cursor=pending[at];
  if(cursor==&from||visited.size()>=ownedLimit){message("That link would create an activation loop.");return;}
  if(std::find(visited.begin(),visited.end(),cursor->s.number)!=visited.end())continue;visited.push_back(cursor->s.number);
  const auto cursorKind=G_LogicKind(*cursor);
  if(G_PushKind(*cursor)||G_TeleportKind(*cursor)||G_SandboxLaser(*cursor)||
     G_WorkshopBeamClass(*cursor)||G_WorkshopTrailClass(*cursor)||G_PortalPortable(*cursor))continue; // Position links do not invoke Use.
  if(jar::logic::UseGate(cursorKind))continue; // State changes do not invoke the destination.
  const bool slottedCursor=jar::logic::Slotted(cursorKind);
  const char*outputs[]={cursor->classname&&!strcmp(cursor->classname,"target_level_change")?nullptr:cursor->target,(G_SpotlightPortable(*cursor)||G_DistancePortable(*cursor)||G_SandboxSlidePortable(*cursor)||G_ButtonPortable(*cursor)||G_SandboxUsable(*cursor)||G_LogicKind(*cursor)==jar::logic::Counter||slottedCursor)?cursor->target2:cursor->classname&&!strcmp(cursor->classname,"trigger_bidirectional")?cursor->paintarget:nullptr,
   slottedCursor?cursor->target3:nullptr,slottedCursor?cursor->target4:nullptr};
  for(const char*output:outputs)if(output)if(auto*next=G_Find(nullptr,FOFS(targetname),output))pending.push_back(next);
 }
 if(!from.classname || !jar::entities::Find(from.classname)){message("Source needs a map-entity preset with a native target input.");return;}
 if(slotted){
  const bool give=logic==jar::logic::Give;
  const int slot=gi.cvar(jar::logic::SlotCvar(logic),"0",CVAR_NORESTART)->integer;
  if(!G_LogicOutputField(slot)){message("Choose a slot from 1 to 4 in Context.");return;}
  if(give&&!G_LogicPickup(e)){message("Give needs a spawned pickup as its destination.");return;}
  if(gate&&!G_LogicCanSetUse(e)){message("Choose an entity with a Use action or an eligible trigger.");return;}
  if(!give&&!gate&&e.e_UseFunc==useF_NULL){message("Choose a destination with an activation action.");return;}
  for(int i=0;i<jar::logic::outputCount;++i){
   const char* name=G_LogicOutput(from,i);
   if(i!=slot&&name&&!strcmp(name,e.targetname)){message("That destination is already linked to this marker.");return;}
  }
  editSlottedLink(from,slot,e.targetname);
  message(va(give?"Give pickup %d linked.":gate?"Use destination %d linked.":"Random output %d linked.",slot+1));
  return;
 }
 const bool reverse=gi.cvar("jar_linkOutput","0",CVAR_ARCHIVE|CVAR_NORESTART)->integer==1;
 if(G_SandboxUsable(from)){
  if(role>1||role<0||e.e_UseFunc==useF_NULL){message("Choose Primary or Secondary and an activatable destination.");return;}
  editSlottedLink(from,role,e.targetname);
  message(reverse?"On output linked.":"Off or pulse output linked.");
  return;
 }
 if(distance){
  distanceLink(from,reverse?JAR_DISTANCE_OUTSIDE:JAR_DISTANCE_INSIDE,e.targetname);
  message(reverse?"Outside-range output linked.":"Inside-range output linked.");
  return;
 }
 const bool door=G_SandboxSlidePortable(from);
 const bool button=G_ButtonPortable(from);
 const bool counter=logic==jar::logic::Counter;
 if((counter||logic==jar::logic::Relay)&&e.e_UseFunc==useF_NULL){
  message("Choose a destination with an activation action.");return;
 }
 if(reverse&&!door&&!button&&!counter&&!G_SpotlightPortable(from)&&strcmp(from.classname,"trigger_bidirectional")){message("Secondary output is only available on certain controllers. Choose Primary in Q for this source.");return;}
 char*&output=reverse?(door||button||counter||G_SpotlightPortable(from)?from.target2:from.paintarget):from.target;if(output&&gi.bIsFromZone(output,TAG_G_ALLOC))gi.Free(output);
 output=G_NewString(e.targetname);from.enemy=nullptr;
 if(G_SpotlightPortable(from)){from.painDebounceTime=0;from.attackDebounceTime=0;}
 const auto*p=jar::entities::Find(from.classname);
 if(!strcmp(p->spawn,"trigger_push")||!strcmp(p->spawn,"target_push")){from.e_ThinkFunc=thinkF_AimAtTarget;from.nextthink=level.time+FRAMETIME;}
 if(!strcmp(p->spawn,"target_laser")){from.e_ThinkFunc=thinkF_target_laser_start;from.nextthink=level.time+FRAMETIME;}
 if(G_LiftPortable(from))message(reverse?"Lift Lowered output linked.":"Lift Raised output linked.");
 else if(door)message(reverse?"Door Closed output linked.":"Door Opened output linked.");
 else if(button)message(reverse?"Button Released output linked.":"Button Pressed output linked.");
 else if(counter)message(reverse?"Counter Progress output linked.":"Counter Complete output linked.");
 else message(G_SpotlightPortable(from)?(reverse?"Spotlight detection linked to destination.":"Spotlight aim linked to destination."):reverse?"Reverse output linked to destination.":"Primary output linked to destination.");
}

int selectControlNode(const vec3_t start,const vec3_t direction,float limit,int fallback,float* hitDistance=nullptr){
 for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];auto&e=g_entities[o.entity];
  if(!e.inuse || !e.classname)continue;const auto*p=jar::entities::Find(e.classname);
  if((!p||!(p->flags&jar::entities::Marker))&&!G_SandboxBreakableBroken(e)&&!G_SandboxUsableDormant(e))continue;
  if(jar::volumeEntity::Portable(e)){float hit;if(jar::volume::Ray(e.pos1,e.pos2,e.currentOrigin,e.currentAngles,start,direction,limit,hit)&&hit<limit){limit=hit;fallback=e.s.number;}continue;}
  float near=0,far=limit;
  for(int a=0;a<3 && near<=far;++a){const float lo=e.currentOrigin[a]+e.mins[a],hi=e.currentOrigin[a]+e.maxs[a];
   if(std::fabs(direction[a])<.00001f){if(start[a]<lo||start[a]>hi)far=-1;continue;}
   float first=(lo-start[a])/direction[a],last=(hi-start[a])/direction[a];if(first>last)std::swap(first,last);
   near=std::max(near,first);far=std::min(far,last);
  }
  if(near<=far && near<limit){limit=near;fallback=e.s.number;}
 }
 if(hitDistance)*hitDistance=limit;
 return fallback;
}
