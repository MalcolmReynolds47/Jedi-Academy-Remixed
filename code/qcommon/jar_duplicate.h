// Snapshot-only duplication graph. No native pointers or mission-script state.
#ifndef JAR_DUPLICATE_H
#define JAR_DUPLICATE_H
#include "jar_workshop_state.h"
#include "jar_volume.h"
#include "../../shared/qcommon/jar_always_trigger.h"
#include "../../shared/qcommon/jar_visible_trigger.h"
#include "../../shared/qcommon/jar_distance_trigger.h"
#include "../../shared/qcommon/jar_wall.h"
#include "../../shared/qcommon/jar_sliding_door.h"
#include "../../shared/qcommon/jar_lift.h"
#include "../../shared/qcommon/jar_button.h"
#include "../../shared/qcommon/jar_bobbing_motion.h"
#include "jar_cloud.h"
#include "jar_reflection_probe.h"
#include "jar_weather.h"
#include "jar_context.h"
#include "jar_movement.h"
#include "jar_npc_order.h"
#include "jar_logic_settings.h"
#include "jar_contact_trigger.h"
#include "jar_push.h"
#include "jar_teleport.h"
#include "jar_reference.h"
#include "jar_combat_point.h"
#include "jar_skyportal.h"
#include "jar_scene_camera.h"
#include "jar_waypoint.h"
#include "jar_rail.h"
#include "jar_laser_arm.h"
#include "jar_sandbox_laser.h"
#include "jar_sandbox_station.h"
#include "jar_sandbox_breakable.h"
#include "jar_sandbox_usable.h"
#include "jar_sandbox_speaker.h"
#include "jar_sandbox_angular.h"
#include "jar_portal.h"
#include "jar_spotlight.h"
#include "jar_welder.h"
#include "jar_door_camera.h"
#include "jar_crystal_crate.h"
#include "jar_music.h"
#include "jar_level_change.h"
#include "jar_script_parms.h"
#include "jar_scriptrunner.h"
#include "jar_vehicle_status.h"
#include "jar_shooter.h"
#include "jar_replicator.h"
#include "jar_pickup_quantity.h"
#include "jar_pickup_lifetime.h"
#include "jar_charge_copy.h"
#include "jar_angular_trajectory.h"
#include "jar_shield_copy.h"
#include "../game/weapons.h"
#include "../game/teams.h"
#include <array>
#include <map>
#include <set>
namespace jar { namespace duplicate {
// Handheld weapons and rebuilt sabers use the NPC's native firing paths.
constexpr uint32_t actorWeaponMask=((1u<<(WP_CONCUSSION+1))-(1u<<WP_BLASTER_PISTOL))|
 (1u<<WP_MELEE)|(1u<<WP_BRYAR_PISTOL)|(1u<<WP_SABER);
struct SaberBlade {
 int color=0;
 float radius=0,lengthMax=0;
 bool active=false;
};
struct ActorSaber {
 std::string name;
 int bladeCount=0;
 std::array<SaberBlade,MAX_BLADES> blades{};
};
struct ActorLoadout {
 int weapon=WP_NONE;
 uint32_t weapons=0;
 std::array<int,MAX_AMMO> ammo{};
 std::array<ActorSaber,MAX_SABERS> sabers{};
 int saberStyle=0,saberStyles=0;
 bool altFire=false,matchPlayer=false,manualAmmo=false;
};
struct ActorStatus {
 int team=TEAM_FREE,enemyTeam=TEAM_FREE,armor=0;
};
inline bool Valid(const ActorStatus& status){
 return status.team>=TEAM_FREE&&status.team<TEAM_NUM_TEAMS&&status.enemyTeam>=TEAM_FREE&&
        status.enemyTeam<TEAM_NUM_TEAMS&&status.armor>=0&&status.armor<=32767;
}
inline bool ValidActorMovement(const movement::ActorState& movement){
 return movement.friction>=0&&movement.friction<=movement::maxFriction&&
  (!movement.customGravity||(movement.gravity>=0&&movement.gravity<=movement::maxGravity));
}
inline bool Valid(const ActorLoadout& loadout){
 if(loadout.weapon<WP_NONE||loadout.weapon>=WP_NATIVE_WEAPON_COUNT||
    (loadout.weapons&~actorWeaponMask)||
    (loadout.weapon!=WP_NONE&&!(loadout.weapons&(1u<<loadout.weapon))))return false;
 for(int ammo:loadout.ammo)if(ammo< -1||ammo>32767)return false;
 const bool hasSaber=(loadout.weapons&(1u<<WP_SABER))!=0;
 if(hasSaber!=(loadout.sabers[0].bladeCount>0)||(!hasSaber&&loadout.sabers[1].bladeCount)||
    loadout.saberStyle<SS_NONE||loadout.saberStyle>=SS_NUM_SABER_STYLES||
    (loadout.saberStyles&~((1<<SS_NUM_SABER_STYLES)-1)))return false;
 for(const auto& saber:loadout.sabers){
  if(!saber.bladeCount){if(!saber.name.empty())return false;continue;}
  if(saber.bladeCount<1||saber.bladeCount>MAX_BLADES||
     !workshop::NameValid(saber.name.c_str(),false)||saber.name.size()!=std::strlen(saber.name.c_str()))return false;
  for(int i=0;i<saber.bladeCount;++i){
   const auto& blade=saber.blades[i];
   if(blade.color<SABER_RED||blade.color>SABER_PURPLE||!(blade.radius>0&&blade.radius<=64)||
      !(blade.lengthMax>0&&blade.lengthMax<=512))return false;
  }
 }
 return true;
}
struct Object {
 uint32_t catalog=0,source=0,parent=0,target=0; // ownership IDs, not slots
 float offset[3]{},angles[3]{},mins[3]{},maxs[3]{};
 bool frozen=true,hasHealth=false,hasCount=false,targetCaptured=false;int health=0,count=0;
 bool hasActorLoadout=false;
 ActorLoadout actorLoadout;
 bool hasActorStatus=false,hasActorMovement=false;
 ActorStatus actorStatus;
 bool hasActorOrder=false;
 npcOrder::Settings actorOrder;
 uint32_t actorGoal=0,actorPatrolGoal=0;
 movement::ActorState actorMovement;
 bool hasVehicleStatus=false;
 vehicleStatus::Values vehicleStatus;
 bool hasSky=false;skyPortal::Settings sky;
 bool hasRail=false;rail::Settings rail;
 bool hasWaypoint=false;waypoint::Settings waypoint;
 bool hasSceneCamera=false;sceneCamera::Settings sceneCamera;
 bool hasCombatPoint=false;combatPoint::Settings combat;
 bool hasReference=false;reference::Settings reference;
 int poseFrame=0; // decorative model frame, native entityState owns save/load
 workshop::PropPhysics propPhysics{};
 std::vector<chargeCopy::State> charges;
 bool hasStandaloneCharge=false;
 chargeCopy::State standaloneCharge;
 bool hasDeployedShield=false;
 shieldCopy::State deployedShield;
 bool hasLabel=false;std::string label;
 int alwaysDelay=0,alwaysRemaining=0;bool alwaysInactive=false;
 bool hasVisible=false,visibleArmed=false,visibleInactive=false;jarVisibleSettings_t visible{};
 int distanceRange=0,distanceSubjects=0;bool distanceInactive=false,distanceReferenceRequired=false;
 uint32_t distanceReference=0,distanceSubject=0;
 int wallVisibility=-1;bool wallInactive=false;
 bool hasLevelChange=false;levelChange::Settings levelChange;
 bool hasMusic=false;std::string music;
 bool hasWeather=false;weather::Settings weather;
 bool hasMoverOffset=false;float moverOffset[3]{};
 bool hasCrystal=false;crystalCrate::Settings crystal;
 bool hasDoorCamera=false;doorCamera::Settings doorCamera;float doorExtension=0;
 bool hasSlidingDoor=false;slidingDoor::State slidingDoor;
 bool hasButton=false,buttonEnabled=true;jarButtonState_t button{};
 bool hasBobbing=false;jarBobState_t bobbing{};
 movement::Marker movementMarker;
 logic::State logicMarker;
 int pendingOutputMs=0,reuseCooldownMs=0;
 bool hasShooter=false;shooter::Settings shooterSettings;int shooterPendingMs=0;
 int secretFlags=-1;
 int pickupRemainingMs=-1;
 bool rackShelfStock=false;
 int replicatorHeal=-1,replicatorDelay=0;
 replicator::State replicatorState;
 contactTrigger::Settings contact;bool contactSpent=false;
 push::Settings push;
 teleport::Settings teleport;
 bool hasUseState=false,useEnabled=true;
 std::array<uint32_t,logic::outputCount> slottedOutputs{};
 bool hasWelder=false;welder::Settings welder;int welderCooldownMs=0;
 bool hasSpot=false;spotlight::Settings spot;
 bool hasPortal=false;portal::Settings portal;
 bool hasLaser=false;laserArm::Settings laser;int laserBurstMs=0,laserSampleMs=0;
 bool hasLaserEmitter=false;sandboxLaser::Settings laserEmitter;
 sandboxStation::Settings station;
 int stationRechargeMs=0,stationUseCooldownMs=0;
 sandboxBreakable::Settings breakable;
 bool hasUsable=false;
 sandboxUsable::Settings usable;
 bool hasSpeaker=false;
 speaker::Settings speakerSettings;
 sandboxAngular::Settings angular;
 bool hasReflection=false;reflection::Settings reflection;
 bool hasCloud=false;cloud::Settings cloud;
 bool hasScript=false;scriptRunner::Settings script;
 bool hasParms=false;std::array<std::string,parms::count> parms;
 bool nativeMarker=false,markerInactive=false;
 bool shipBoundary=false,preserveEmpty=true;float turnSeconds=3;
 bool triggerVolume=false,volumeEnabled=false,volumeInactive=false;
 uint32_t backwardTarget=0;bool backwardCaptured=false,directionalVolume=false,playerOnly=false;float wait=1;int remaining=0;std::string npcFilter;
 bool orientedVolume=false;float volumeMins[3]{},volumeMaxs[3]{};
};
struct Snapshot {
 std::vector<Object> objects;
 std::vector<constraint::Joint> joints;
 float origin[3]{};
 std::map<uint32_t,std::array<float,3>> worldNormals;
};
constexpr uint32_t worldAnchor = constraint::NativeId(ENTITYNUM_WORLD);
// Physical links and owned helpers form the copied assembly. Authored target
// chains are not traversed; their scripts and unrelated mission actors stay put.
inline bool Members(const workshop::State&s,uint32_t root,std::vector<uint32_t>&out,
 const std::vector<std::pair<uint32_t,uint32_t>>&links={},const std::set<uint32_t>& nativeMembers={}){
 if(!nativeMembers.count(root)){
  if(!workshop::OwnedId(s,root))return false;
  for(unsigned depth=0;depth<workshop::ownedLimit;++depth){const auto*o=workshop::OwnedId(s,root);if(!o)return false;if(!o->parent)break;root=o->parent;if(depth+1==workshop::ownedLimit)return false;}
 }
 std::vector<uint32_t> ids{root};
 auto add=[&](uint32_t id){if(constraint::Native(id)&&!nativeMembers.count(id))return;if(std::find(ids.begin(),ids.end(),id)==ids.end())ids.push_back(id);};
 for(size_t at=0;at<ids.size();++at){
  const auto id=ids[at];const auto*member=workshop::OwnedId(s,id);
  if((!member&&!nativeMembers.count(id))||ids.size()>workshop::ownedLimit)return false;
  if(member&&member->parent)add(member->parent);
  for(unsigned i=0;i<s.ownedCount;++i)if(s.owned[i].parent==id)add(s.owned[i].id);
  for(unsigned i=0;i<s.jointCount;++i){const auto&j=s.joints[i];if(j.first==id)add(j.second);if(j.second==id)add(j.first);}
  for(const auto&link:links){if(link.first==id)add(link.second);if(link.second==id)add(link.first);}
 }
 // Keep parents before children, retaining the selected root at the front.
 std::stable_sort(ids.begin()+1,ids.end());out=std::move(ids);return true;
}
inline bool MapSection(uint32_t id){
 const auto* card=catalog::Find(id);
 return card&&card->kind==5&&card->tab==5;
}
inline bool Valid(const Snapshot&s){
 if(s.objects.empty()||s.objects.size()>workshop::ownedLimit||s.joints.size()>constraint::limit)return false;
 size_t entityCount=s.objects.size();
 std::map<uint32_t,const Object*> ids;
 for(const auto&o:s.objects){
  if(!o.source||!workshop::CatalogValid(o.catalog)||ids.count(o.source)||(o.parent&&!ids.count(o.parent)))return false;
  if(o.hasDeployedShield&&(o.catalog!=workshop::consoleCatalog||!shieldCopy::Valid(o.deployedShield)))return false;
  if(o.hasStandaloneCharge&&(s.objects.size()!=1||!s.joints.empty()||o.catalog!=workshop::consoleCatalog||
     o.hasDeployedShield||!chargeCopy::Valid(o.standaloneCharge)))return false;
  if(!o.charges.empty()&&!workshop::PropCatalog(o.catalog))return false;
  if(o.charges.size()>workshop::ownedLimit-entityCount)return false;
  for(const auto& charge:o.charges)if(!chargeCopy::Valid(charge))return false;
  entityCount+=o.charges.size();
  if(o.hasActorLoadout&&!Valid(o.actorLoadout))return false;
  if(o.hasActorStatus&&!Valid(o.actorStatus))return false;
  if(o.hasActorOrder&&(!npcOrder::Valid(o.actorOrder)||o.actorOrder.mode==npcOrder::Autonomous||o.hasVehicleStatus))return false;
  if(o.hasActorMovement&&!ValidActorMovement(o.actorMovement))return false;
  if(o.hasVehicleStatus&&(!o.hasHealth||o.hasActorLoadout||o.hasActorStatus||o.hasActorMovement||!vehicleStatus::Valid(o.vehicleStatus)))return false;
  if(o.replicatorHeal!=-1&&(o.replicatorHeal<0||o.replicatorHeal>replicator::maxHeal||
     o.replicatorDelay<replicator::minDelay||o.replicatorDelay>replicator::maxDelay||!replicator::Valid(o.replicatorState)))return false;
  if(o.hasShooter&&(!shooter::Valid(o.shooterSettings)||o.shooterPendingMs<0||
     o.shooterPendingMs>shooter::maxInterval+shooter::maxVariation||
     (o.shooterPendingMs&&(!o.shooterSettings.repeat||!o.shooterSettings.enabled))))return false;
  if((o.hasHealth&&(o.health<1||o.health>100000))||(o.hasCount&&!pickupQuantity::Valid(o.count)))return false;
  if(o.pickupRemainingMs< -1||o.pickupRemainingMs>pickupLifetime::maxMs||
     (o.pickupRemainingMs>=0&&!o.hasCount))return false;
  if(o.hasRail&&!rail::Valid(o.rail))return false;
  if(o.hasWaypoint&&!waypoint::Valid(o.waypoint))return false;
  if(o.hasSceneCamera&&!sceneCamera::Valid(o.sceneCamera))return false;
  if(o.hasCombatPoint&&!combatPoint::Valid(o.combat))return false;
  if(o.hasReference&&!reference::Valid(o.reference))return false;
  if(o.poseFrame<0||o.poseFrame>65535||(o.poseFrame&&!workshop::PropCatalog(o.catalog)))return false;
  if(!workshop::ValidPropPhysics(o.catalog,o.propPhysics))return false;
  if(o.hasLabel&&(o.label.size()!=std::strlen(o.label.c_str())||!context::Label(o.label.c_str())))return false;
  if(o.alwaysDelay?!JAR_AlwaysDelayValid(o.alwaysDelay)||o.alwaysRemaining<0||o.alwaysRemaining>JAR_ALWAYS_MAX_MS:o.alwaysRemaining||o.alwaysInactive)return false;
  if(o.hasVisible&&!JAR_VisibleSettingsValid(o.visible))return false;
  if(o.distanceRange&&!JAR_DistanceSettingsValid(o.distanceRange,o.distanceSubjects))return false;
  if(o.wallVisibility < -1 || o.wallVisibility > 1 || (o.wallVisibility >= 0 && o.catalog != JAR_WALL_CATALOG_ID))return false;
  if(o.hasLevelChange&&(!levelChange::Valid(o.levelChange)||o.targetCaptured||o.target))return false;
  if(o.hasMusic&&!music::Valid(o.music))return false;
  if(o.hasWeather&&!weather::Valid(o.weather))return false;
  if(o.hasMoverOffset)for(float v:o.moverOffset)if(!std::isfinite(v)||std::fabs(v)>131072)return false;
  if(o.hasCrystal&&!crystalCrate::Valid(o.crystal))return false;
  if(o.hasDoorCamera&&(!doorCamera::Valid(o.doorCamera)||!std::isfinite(o.doorExtension)||o.doorExtension<0||o.doorExtension>o.doorCamera.travel))return false;
  if(o.hasSlidingDoor&&((o.catalog!=JAR_SLIDING_DOOR_ID&&o.catalog!=JAR_LIFT_ID)||
      !slidingDoor::Valid(o.slidingDoor)||(o.catalog==JAR_LIFT_ID&&o.slidingDoor.settings.direction!=JAR_SLIDE_UP)))return false;
  if(o.hasButton&&(o.catalog!=JAR_BUTTON_ID||!JAR_ButtonValid(o.button)))return false;
  if(o.hasBobbing!=(o.catalog==JAR_BOB_ID)||(o.hasBobbing&&!JAR_BobValid(o.bobbing)))return false;
  if(o.movementMarker.kind!=movement::None&&(!movement::Valid(o.movementMarker)||
     o.catalog!=(o.movementMarker.kind==movement::Gravity?movement::gravityId:movement::frictionId)))return false;
  if(o.logicMarker.kind!=logic::None&&(!logic::Valid(o.logicMarker)||
     o.catalog!=logic::Catalog(o.logicMarker.kind)))return false;
  if(o.contact.kind!=contactTrigger::None&&(!o.orientedVolume||!contactTrigger::Valid(o.contact)||
     o.catalog!=contactTrigger::Catalog(o.contact.kind)||
     (o.contactSpent&&o.contact.kind!=contactTrigger::Once)))return false;
  const bool contactOutput=o.contact.kind==contactTrigger::Repeat||
   (o.contact.kind==contactTrigger::Once&&!o.contactSpent);
  if(o.pendingOutputMs<0||o.pendingOutputMs>logic::maxDelay||
     (o.pendingOutputMs&&!contactOutput&&o.logicMarker.kind!=logic::Timer&&
      o.logicMarker.kind!=logic::Delay&&o.logicMarker.kind!=logic::Relay))return false;
  if(o.reuseCooldownMs<0||o.reuseCooldownMs>logic::maxDelay||
     (o.reuseCooldownMs&&o.logicMarker.kind!=logic::Relay&&o.contact.kind!=contactTrigger::Repeat))return false;
  if(o.push.kind!=push::None&&(!push::Valid(o.push)||
     (o.push.kind==push::Volume?(o.catalog!=push::volumeId||!o.orientedVolume):o.catalog!=push::markerId)))return false;
  if(o.teleport.kind!=teleport::None&&(!teleport::Valid(o.teleport)||
     (o.teleport.kind==teleport::Volume?(!o.orientedVolume||(o.catalog!=teleport::volumeId&&o.catalog!=teleport::discId)):
      o.catalog!=teleport::markerId)))return false;
  if(o.hasWelder&&!welder::Valid(o.welder))return false;
  if(o.welderCooldownMs<0||o.welderCooldownMs>welder::maxCooldownMs||
     (o.welderCooldownMs&&!o.hasWelder))return false;
  if(o.hasSpot&&!spotlight::Valid(o.spot))return false;
  if(o.hasPortal&&!portal::Valid(o.portal))return false;
  if(o.hasLaser&&!laserArm::Valid(o.laser))return false;
  if(o.laserBurstMs<0||o.laserBurstMs>laserArm::maxBurstMs||
     (o.laserBurstMs&&(!o.hasLaser||o.laser.action!=0||o.laserSampleMs<=0))||
     (!o.laserBurstMs&&o.laserSampleMs))return false;
  if(o.hasLaserEmitter&&!sandboxLaser::Valid(o.laserEmitter))return false;
  if(o.station.capacity&&!sandboxStation::Valid(o.station))return false;
  if(o.stationRechargeMs<0||(o.stationRechargeMs&&(!o.station.capacity||!sandboxStation::Recharging(o.station)||
     o.stationRechargeMs>o.station.recharge+sandboxStation::useSoundDuration)))return false;
  if(o.stationUseCooldownMs<0||o.stationUseCooldownMs>sandboxStation::maxUseCooldown+1||
     (o.stationUseCooldownMs&&!o.station.capacity))return false;
  if(o.breakable.capacity&&!sandboxBreakable::Valid(o.breakable))return false;
  if(o.hasUsable&&!sandboxUsable::Valid(o.usable))return false;
  if(o.hasSpeaker&&!speaker::Valid(o.speakerSettings))return false;
  if(o.angular.kind&&!sandboxAngular::Valid(o.angular))return false;
  if(o.hasReflection&&!reflection::Valid(o.reflection))return false;
  if(o.hasCloud&&!cloud::Valid(o.cloud))return false;
  if(o.hasScript&&!scriptRunner::Valid(o.script))return false;
  if(o.hasParms)for(const auto&text:o.parms)if(!parms::Valid(text))return false;
  if(o.shipBoundary&&(!o.triggerVolume||!std::isfinite(o.turnSeconds)||o.turnSeconds<.1f||o.turnSeconds>60))return false;
  if(o.directionalVolume&&(!o.triggerVolume||!std::isfinite(o.wait)||o.wait< -1||o.wait>3600||o.remaining< -1||o.remaining>100000||o.npcFilter.size()>1023))return false;
  if(o.orientedVolume&&(!o.triggerVolume||!volume::Valid(o.volumeMins,o.volumeMaxs)))return false;
  // Rotating a valid BSP enclosure can project a corner farther than the
  // largest local coordinate on any single axis.
  constexpr float sectionHullLimit=65536.f*1.733f;
  const float hullLimit=MapSection(o.catalog)?sectionHullLimit:(o.triggerVolume?16384.f:4096.f);
  for(int a=0;a<3;++a)if(!std::isfinite(o.offset[a])||std::fabs(o.offset[a])>65535||!std::isfinite(o.angles[a])||std::fabs(o.angles[a])>100000||!std::isfinite(o.mins[a])||!std::isfinite(o.maxs[a])||o.mins[a]>o.maxs[a]||std::fabs(o.mins[a])>hullLimit||std::fabs(o.maxs[a])>hullLimit)return false;
  ids.emplace(o.source,&o);
 }
 for(const auto&o:s.objects)if(o.target&&(!o.targetCaptured||!ids.count(o.target)))return false;
 for(const auto&o:s.objects){
  for(auto endpoint:{o.actorGoal,o.actorPatrolGoal})if(endpoint){
   const auto goal=ids.find(endpoint);
   if(o.hasVehicleStatus||endpoint==o.source||goal==ids.end())return false;
   if(endpoint==o.actorPatrolGoal&&(!o.hasActorOrder||o.actorOrder.mode!=npcOrder::Patrol||!goal->second->hasWaypoint))return false;
   if(!goal->second->hasWaypoint){
    const auto* card=catalog::Find(goal->second->catalog);
    if(!goal->second->hasReference||!card||strcmp(card->name,"waypoint_navgoal"))return false;
   }
  }
  const bool hasSecondary=o.directionalVolume||o.hasSpot||o.hasSceneCamera||o.hasWaypoint||o.distanceRange||
   o.hasSlidingDoor||o.hasButton||o.hasUsable||o.logicMarker.kind==logic::Counter;
  if(o.backwardTarget&&(!o.backwardCaptured||!hasSecondary||!ids.count(o.backwardTarget)))return false;
  if(o.distanceReference&&(!o.distanceRange||!o.distanceReferenceRequired||o.distanceReference==o.source||!ids.count(o.distanceReference)))return false;
  if(o.distanceSubject&&(!o.distanceRange||o.distanceSubject==o.source||!ids.count(o.distanceSubject)))return false;
  for(auto endpoint:o.slottedOutputs)if(endpoint&&
      (!logic::Slotted(o.logicMarker.kind)||endpoint==o.source||!ids.count(endpoint)))return false;
 }
 for(size_t i=0;i<s.joints.size();++i){const auto&j=s.joints[i];
  const bool anchored=j.first==worldAnchor||j.second==worldAnchor;
  if(!constraint::Valid(j)||(j.first!=worldAnchor&&!ids.count(j.first))||
     (j.second!=worldAnchor&&!ids.count(j.second))||
     (j.first==worldAnchor&&j.second==worldAnchor)||anchored!=bool(s.worldNormals.count(j.id)))return false;
  if(anchored){
   float length=0;
   for(float v:s.worldNormals.at(j.id))length+=v*v;
   if(!std::isfinite(length)||std::fabs(length-1)>.01f)return false;
  }
  for(size_t k=0;k<i;++k){const auto&other=s.joints[k];if(j.id==other.id||(j.first==other.first&&j.second==other.second)||(j.first==other.second&&j.second==other.first))return false;}
 }
 return true;
}
inline bool Bounds(const Snapshot&s,float*mins,float*maxs){
 if(!Valid(s))return false;
 for(int a=0;a<3;++a){mins[a]=65536;maxs[a]=-65536;}
 for(const auto& object:s.objects){
  for(int a=0;a<3;++a){mins[a]=std::min(mins[a],object.offset[a]+object.mins[a]);maxs[a]=std::max(maxs[a],object.offset[a]+object.maxs[a]);}
  for(const auto& charge:object.charges){
   vec3_t offset;jar::angular::RebaseVector(charge.origin,vec3_origin,object.angles,offset);
   const float half=chargeCopy::HalfSize(charge.mode);
   for(int a=0;a<3;++a){
    mins[a]=std::min(mins[a],object.offset[a]+offset[a]-half);
    maxs[a]=std::max(maxs[a],object.offset[a]+offset[a]+half);
   }
  }
 }
 return true;
}
} }
#endif
