// Map-object tools and session-local undo. Included inside the workshop namespace.
// Native entity layouts and script names are never repurposed. GPL-2.0-only.
#include "g_holocron.h"
#include "g_always_trigger.h"
#include "g_distance_trigger.h"
#include "g_wall_sandbox.h"
#include "g_start_marker.h"
#include "g_visible_trigger.h"
#include "g_bobbing_sandbox.h"
#include "g_movement_settings.h"
#include "g_logic_settings.h"
#include "g_contact_trigger.h"
#include "g_sandbox_push.h"
#include "g_sandbox_teleport.h"
#include "g_sandbox_charges.h"
#include "g_space.h"
#include "g_portable_shield.h"
#include "g_workshop_fx_aim.h"
#include "qcommon/jar_shield_copy.h"
#include "g_workshop_mounted_motion.inc"
#include "g_workshop_prop_clearance.inc"

bool contextChargeProperties(const gentity_t& charge,jar::chargeCopy::Properties& properties,
 const gentity_t* support){
 return charge.activator==&g_entities[0]&&(!support||G_JAR_WorkshopOwns(support))&&
  G_SandboxChargeProperties(charge,properties);
}

bool contextHealthEditable(const gentity_t& e){
 if(e.m_pVehicle||!e.takedamage||e.health<=0||G_PortableShield(e)||
    G_SandboxBreakable(e)||G_SandboxUsable(e)||G_SandboxChargeSupport(e))return false;
 jar::chargeCopy::Properties charge;
 return !G_SandboxChargeProperties(e,charge);
}

const Prop* editedProp(const gentity_t& entity){
 const auto* record=owned(entity.s.number);
 return record&&matches(*record)?prop(record->catalog):nullptr;
}

bool propTransformClear(gentity_t& entity,const Prop& model,int frame,const vec3_t origin,const vec3_t angles,
 std::vector<SandboxChargeMove>& charges,bool sweep,const float* pivot=nullptr,jar::physics::Vec localPivot={}){
 vec3_t mins,maxs;
 if(!propHull(model,angles,mins,maxs,frame))return false;
 if(!G_SandboxChargesPlanPose(entity,origin,angles,charges,ENTITYNUM_WORLD))return false;
 {
  SandboxMountedChargeCollision mountedCollision;
  SandboxChargesUnlinked own(charges);
  if(!propBodyTransformClear(entity,model,frame,origin,angles,sweep,mins,maxs,pivot,localPivot))return false;
 }
 return mountedChargeSweepClear(entity,angles,charges,sweep,pivot);
}

bool BobbingSameSettings(const jarBobState_t& a,const jarBobState_t& b){
 return a.amplitude==b.amplitude&&a.periodMs==b.periodMs&&a.running==b.running&&a.inactive==b.inactive;
}
struct SpawnedEdit {uint32_t entity,generation,owner;};
bool undoSpawnGroup(const std::vector<SpawnedEdit>&);
struct ThrusterProperties {
 float force=0;
 unsigned enabled=0;
 float a[4]{},b[4]{};
 ThrusterProperties()=default;
 explicit ThrusterProperties(const jar::constraint::Joint& joint):force(joint.force),enabled(joint.enabled){
  std::copy_n(joint.a.rotation,4,a);std::copy_n(joint.b.rotation,4,b);
 }
 bool Matches(const jar::constraint::Joint& joint)const{
  return force==joint.force&&enabled==joint.enabled&&
   std::equal(a,a+4,joint.a.rotation)&&std::equal(b,b+4,joint.b.rotation);
 }
 void Apply(jar::constraint::Joint& joint)const{
  joint.force=force;joint.enabled=enabled;
  std::copy_n(a,4,joint.a.rotation);std::copy_n(b,4,joint.b.rotation);
 }
};
struct Edit {
 uint32_t entity,generation,joint=0; bool spawn,embedded,linked=false; unsigned frozen;
 uint32_t ropeLengthId=0;
 float ropeLengthBefore=0,ropeLengthAfter=0;
 uint32_t thrusterId=0;
 ThrusterProperties thrusterBefore,thrusterAfter;
 bool context=false,freezeEdit=false,vehiclePoseEdit=false;
 vec3_t origin,angles; trajectory_t position,rotation;
 vec3_t appliedVehicleOrigin,appliedVehicleAngles;
 bool railEdit=false;jar::rail::Settings rail;
 bool waypointEdit=false;jar::waypoint::Settings waypoint;
 bool sceneCameraEdit=false;jar::sceneCamera::Settings sceneCamera;
 bool skyEdit=false;jar::skyPortal::Settings sky;int skyOrder=-1;
 bool combatPointEdit=false;jar::combatPoint::Settings combatPoint;
 bool referenceEdit=false;jar::reference::Settings reference;
 bool poseEdit=false;int poseFrame=0;
 bool propPhysicsEdit=false;PropPhysics propPhysics{};
 bool labelEdit=false;std::string label;
 int alwaysDelay=0;
 bool visibleEdit=false,visibleInactive=false;jarVisibleSettings_t visible{};
 int distanceRange=0,distanceSubjects=0,distanceLink=-1;
 bool distanceArmEdit=false,distanceWasInactive=false;
 std::string distancePreviousLink,distanceAppliedLink;
 uint64_t distancePreviousLifetime=0;
 int wallVisibility=-1;bool wallEnabledEdit=false,wallWasInactive=false;
 bool holocronEdit=false;int holocronLevel=0,holocronPower=0;
 const gitem_t* pickupQuantityItem=nullptr;int pickupQuantityBefore=0,pickupQuantityAfter=0;
 const gitem_t* pickupLifetimeItem=nullptr;
 int pickupLifetimeBefore=0,pickupLifetimeAfter=0,pickupLifetimeEditClock=0;
 bool portableShieldEdit=false;
 jar::shieldCopy::Properties portableShieldBefore,portableShieldAfter;
 bool mountedChargeEdit=false;
 jar::chargeCopy::Properties mountedChargeBefore,mountedChargeAfter;
 unsigned mountedChargeSupport=ENTITYNUM_NONE,mountedChargeSupportGeneration=0;
 bool levelChangeEdit=false;jar::levelChange::Settings levelChange;
 bool musicEdit=false;std::string music;
 bool weatherEdit=false;jar::weather::Settings weather;int weatherOrder=0;
 bool crystalEdit=false;jar::crystalCrate::Settings crystal;
 bool doorCameraEdit=false;jar::doorCamera::Settings doorCamera;
 bool slidingDoorEdit=false,slidingDoorInactive=false;jar::slidingDoor::Settings slidingDoor;
 int buttonWait=-1;bool buttonEnabled=true;
 bool bobbingEdit=false;jarBobState_t bobbingBefore{},bobbingAfter{};
 jar::movement::Marker movementMarkerBefore,movementMarkerAfter;
 jar::logic::State logicBefore,logicAfter;
 jar::contactTrigger::Settings contactBefore,contactAfter;
 jar::push::Settings pushBefore,pushAfter;
 jar::teleport::Settings teleportBefore,teleportAfter;
 bool positionLinkEdit=false;
 std::string positionLinkBefore,positionLinkAfter;
 uint64_t positionLinkBeforeLifetime=0,positionLinkAfterLifetime=0;
 bool contactRearmed=false;
 int slottedLinkSlot=-1;std::string slottedLinkBefore,slottedLinkAfter;
 bool actorMovementEdit=false;jar::movement::ActorState movementBefore,movementAfter;
 bool welderEdit=false;jar::welder::Settings welder;
 bool spotEdit=false;jar::spotlight::Settings spot;
 bool portalEdit=false;jar::portal::Settings portalBefore,portalAfter;
 bool laserEdit=false;jar::laserArm::Settings laser;
 bool laserEmitterEdit=false;
 jar::sandboxLaser::Settings laserEmitterBefore,laserEmitterAfter;
 jar::sandboxStation::Settings stationBefore,stationAfter;
 jar::sandboxBreakable::Settings breakableBefore,breakableAfter;
 bool usableEdit=false;
 jar::sandboxUsable::Settings usableBefore,usableAfter;
 bool speakerEdit=false;
 jar::speaker::Settings speakerBefore,speakerAfter;
 jar::sandboxAngular::Settings angularBefore,angularAfter;
 bool reflectionEdit=false;jar::reflection::Settings reflection;
 bool cloudEdit=false;jar::cloud::Settings cloud;
 bool boundaryEdit=false,boundaryPreserve=true;float boundaryTime=3;
 bool scriptEdit=false;jar::scriptRunner::Settings script;
 int parmSlot=-1;std::string parm;
 bool healthEdit=false,volumeEdit=false;int health=0,healthAfter=0;std::vector<SpawnedEdit> group;
 bool vehicleResourcesEdit=false;
 bool actorStatusEdit=false;
 jar::duplicate::ActorStatus actorStatusBefore,actorStatusAfter;
 bool actorFireEdit=false;
 WorkshopActorFire actorFireBefore,actorFireAfter;
 bool actorWeaponEdit=false;
 bool actorOrderEdit=false;
 jar::npcOrder::Settings actorOrderBefore,actorOrderAfter;
 bool actorGoalEdit=false;
 jar::npcOrder::Destination actorGoalBefore,actorGoalAfter;
 jar::npcOrder::Destination actorPatrolBefore;
 WorkshopActorWeapon actorWeaponBefore;
 int actorWeaponAfter=WP_NONE,actorWeaponPool=AMMO_NONE,actorWeaponGrantedAmmo=0;
 uint32_t actorWeaponGrant=0;
 jar::vehicleStatus::Values vehicleResourcesBefore,vehicleResourcesAfter;
 bool shooterEdit=false;
 jar::shooter::Settings shooterBefore,shooterAfter;
 int shooterPendingMs=0;
 int secretFlagsBefore=-1,secretFlagsAfter=-1;
 bool secretEnabledBefore=true,secretEnabledAfter=true;
 int replicatorHealBefore=-1,replicatorHealAfter=0,replicatorDelayBefore=0,replicatorDelayAfter=0;
 bool replicatorEnabledBefore=true,replicatorEnabledAfter=true;
 bool spaceEdit=false,spaceArmed=false,spaceEnabled=false;
 bool hasMoverMotion=false;JarMoverMotion moverMotion;
 bool controlled=false;NativeControl native;vec3_t pos1,pos2;int time=0,nextthink=0,nextRoff=0;
};
uint32_t entityGeneration[MAX_GENTITIES]{};
const Owned* spawnGroupMember(const SpawnedEdit& member){
 const auto* entry=owned(member.entity);
 return entry&&entry->id==member.owner&&entityGeneration[member.entity]==member.generation&&matches(*entry)?entry:nullptr;
}
#include "g_workshop_edit_history.inc"
WorkshopEditHistory edits;
bool isCreation(const Edit& edit){return edit.spawn||!edit.group.empty()||edit.joint;}
bool recordingContextEdit=false;
struct WorkshopActionScope {
 bool& active;
 const bool previous;
 explicit WorkshopActionScope(bool& flag,bool value=true):active(flag),previous(flag){active=value;}
 ~WorkshopActionScope(){active=previous;}
};
void rememberAction(Edit edit){
 edit.context=recordingContextEdit&&!isCreation(edit);
 // Global Undo records only live spawns and created attachments.
 if(!edit.context&&!isCreation(edit))return;
 edits.push_back(std::move(edit));
}
bool mapModel(const gentity_t&e){
 return !e.client && !e.bmodel && e.s.eType==ET_GENERAL && e.model && e.classname &&
  (!Q_stricmp(e.classname,"misc_model_breakable") || !Q_stricmp(e.classname,"misc_model_static") || !Q_stricmp(e.classname,"misc_model"));
}
bool editable(const gentity_t&e){return e.inuse && e.s.number>=0 && e.s.number<ENTITYNUM_WORLD && !e.freeAfterEvent && !G_BSPSectionBreakable(e);}
bool contextVehicleMember(const gentity_t& member){
 return !nativeControls.count(member.s.number)&&held!=unsigned(member.s.number)&&
  !(member.client->ps.eFlags&JAR_VEHICLE_IMMOBILE_FLAGS);
}
bool contextVehicleResources(const gentity_t& hull,jar::vehicleStatus::Values& values){
 if(!editable(hull)||held==unsigned(hull.s.number)||!G_WorkshopCaptureVehicleStatus(hull,values))return false;
 const auto& vehicle=*hull.m_pVehicle;
 if(vehicle.m_pPilot||vehicle.m_pDroidUnit)return G_TransportVehicleReady(hull,contextVehicleMember);
 return !occupied(hull);
}
bool readWorkshopActorStatus(const gentity_t& actor,jar::duplicate::ActorStatus& status){
 if(!actor.NPC||!actor.client||actor.m_pVehicle)return false;
 status={actor.client->playerTeam,actor.client->enemyTeam,actor.client->ps.stats[STAT_ARMOR]};
 return jar::duplicate::Valid(status);
}
void writeWorkshopActorStatus(gentity_t& actor,const jar::duplicate::ActorStatus& status){
 auto& client=*actor.client;
 client.playerTeam=static_cast<team_t>(status.team);client.enemyTeam=static_cast<team_t>(status.enemyTeam);
 client.ps.persistant[PERS_TEAM]=client.playerTeam;client.ps.stats[STAT_ARMOR]=status.armor;
}
bool contextEditableActor(const gentity_t& actor){
 const auto* record=editable(actor)&&actor.s.number>=MAX_CLIENTS?owned(actor.s.number):nullptr;
 return record&&matches(*record)&&actor.NPC&&!actor.m_pVehicle&&G_LogicLiveActor(&actor)&&
  actor.e_ThinkFunc!=thinkF_NPC_Begin&&!occupied(actor)&&held!=unsigned(actor.s.number);
}
bool contextActorStatus(const gentity_t& actor,jar::duplicate::ActorStatus& status){
 return contextEditableActor(actor)&&readWorkshopActorStatus(actor,status);
}
bool contextActorFire(const gentity_t& actor,WorkshopActorFire& fire){
 return contextEditableActor(actor)&&G_WorkshopReadActorFire(actor,fire);
}
uint32_t contextActorWeaponChoices(gentity_t& actor){
 return contextEditableActor(actor)?G_WorkshopActorWeaponChoices(actor):0;
}
bool contextActorOrder(gentity_t& actor,jar::npcOrder::Settings& order){
 if(!contextEditableActor(actor)||!G_CanPickUpWeapons(&actor)||!G_StandardHumanoid(&actor)||
    G_WorkshopNPCOrderHasNativeControl(actor)||G_WorkshopNPCOrderScripted(actor))return false;
 order=G_WorkshopReadNPCOrder(*actor.NPC);
 return jar::npcOrder::Valid(order);
}
void applyContextActorOrder(gentity_t& actor,const jar::npcOrder::Settings& before,const jar::npcOrder::Settings& next){
 auto& info=*actor.NPC;
 G_WorkshopWriteNPCOrder(info,next);
 info.nextBStateThink=level.time;
 if(before.mode==next.mode)return;
 G_WorkshopWriteNPCPatrol(info,{});
 auto& cmd=info.last_ucmd;
 cmd.forwardmove=cmd.rightmove=cmd.upmove=0;
 cmd.buttons=0;
 G_WorkshopStopNPCOrderAttack(actor);
 if(next.mode!=jar::npcOrder::Autonomous&&info.group)AI_DeleteSelfFromGroup(&actor);
 if(next.mode==jar::npcOrder::Wait||jar::npcOrder::UsesDestination(next.mode)||actor.enemy==&g_entities[0]){
  G_ClearEnemy(&actor);
  actor.lastEnemy=nullptr;
  info.enemyLastSeenTime=0;
 }
}
void setContextActorGoal(gentity_t& actor,jar::npcOrder::Destination goal,jar::npcOrder::Destination patrol={}){
 G_WorkshopWriteNPCGoal(*actor.NPC,goal);
 G_WorkshopWriteNPCPatrol(*actor.NPC,patrol);
 if(!jar::npcOrder::UsesDestination(G_WorkshopReadNPCOrder(*actor.NPC).mode))return;
 auto& info=*actor.NPC;
 info.nextBStateThink=level.time;
 info.last_ucmd.forwardmove=info.last_ucmd.rightmove=info.last_ucmd.upmove=0;
}
void editContextActorGoal(gentity_t& actor,jar::npcOrder::Destination goal){
 const auto before=G_WorkshopReadNPCGoal(*actor.NPC);
 if(jar::npcOrder::Same(before,goal))return;
 const WorkshopActionScope propertyEdit(recordingContextEdit);
 Edit edit{};edit.entity=actor.s.number;edit.generation=entityGeneration[actor.s.number];
 edit.actorGoalEdit=true;edit.actorGoalBefore=before;edit.actorGoalAfter=goal;
 edit.actorPatrolBefore=G_WorkshopReadNPCPatrol(*actor.NPC);
 setContextActorGoal(actor,goal);
 rememberAction(std::move(edit));
}
bool mergeWorkshopActorStatus(const jar::duplicate::ActorStatus& current,const jar::duplicate::ActorStatus& expected,
 const jar::duplicate::ActorStatus& requested,jar::duplicate::ActorStatus& merged){
 merged=current;
 using Status=jar::duplicate::ActorStatus;
 for(auto field:{&Status::team,&Status::enemyTeam,&Status::armor}){
  if(expected.*field==requested.*field)continue;
  if(current.*field!=expected.*field)return false;
  merged.*field=requested.*field;
 }
 return true;
}
bool applyContextActorStatus(gentity_t& actor,const jar::duplicate::ActorStatus& previous,const jar::duplicate::ActorStatus& next){
 const bool allegiance=previous.team!=next.team||previous.enemyTeam!=next.enemyTeam;
 if(allegiance&&(actor.svFlags&SVF_LOCKEDENEMY))return false;
 writeWorkshopActorStatus(actor,next);
 if(!allegiance)return true;
 if(actor.NPC->group)AI_DeleteSelfFromGroup(&actor);
 G_ClearEnemy(&actor);actor.lastEnemy=nullptr;actor.NPC->enemyLastSeenTime=0;
 for(int i=MAX_CLIENTS;i<globals.num_entities;++i){
  auto& other=g_entities[i];
  if(other.inuse&&other.NPC&&other.client&&other.enemy==&actor&&!(other.svFlags&SVF_LOCKEDENEMY)&&!G_ValidEnemy(&other,&actor))
   G_ClearEnemy(&other);
 }
 return true;
}
bool editableLabel(const gentity_t&e){return jar::context::LabelClass(e.classname)&&(!JAR_StartCard(e.classname)||G_JAR_PortableStart(e));}
bool movable(const gentity_t&e){return editable(e);}
bool contextMoveMover(gentity_t& e,const vec3_t origin,const vec3_t angles){
 vec3_t start,first,last;
 VectorCopy(e.currentOrigin,start);VectorCopy(e.pos1,first);VectorCopy(e.pos2,last);
 if(!G_JAR_MoverRotateAt(e,angles,origin,nullptr))return false;
 if(nativeOwnedControl(e.s.number)){
  // A frozen mover will rebase its original path when it is released.
  VectorCopy(first,e.pos1);VectorCopy(last,e.pos2);
 }else{
  vec3_t move;VectorSubtract(e.currentOrigin,start,move);
  G_JAR_MoverMotionStop(e);
  if(VectorLengthSquared(move))G_JAR_MoverMotionRebase(e,move);
 }
 setupMoved(e);
 return true;
}
Edit capturePropertyEdit(gentity_t& e){
 Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];
 edit.hasMoverMotion=G_JAR_MoverMotionRead(e,edit.moverMotion);
 VectorCopy(e.currentOrigin,edit.origin);VectorCopy(e.currentAngles,edit.angles);
 edit.position=e.s.pos;edit.rotation=e.s.apos;if(auto*o=owned(e.s.number)){edit.frozen=o->frozen;edit.linked=Constrained(state,o->id);}
 const auto control=nativeControls.find(e.s.number);if(control!=nativeControls.end()){edit.controlled=true;edit.native=control->second;}
 VectorCopy(e.pos1,edit.pos1);VectorCopy(e.pos2,edit.pos2);edit.time=level.time;edit.nextthink=e.nextthink;edit.nextRoff=e.next_roff_time;
 vec3_t lo,hi;VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);if(jar::bspSection::Portable(e))jar::bspSection::Hull(e.mins,e.maxs,e.currentAngles,lo,hi);
 trace_t tr{};{BSPSectionMotionScope sectionMotion(e);if(sectionMotion)gi.trace(&tr,e.currentOrigin,lo,hi,e.currentOrigin,e.s.number,MASK_SOLID,(EG2_Collision)0,0);}
 edit.embedded=tr.startsolid || tr.allsolid;
 return edit;
}
void rememberEdit(gentity_t&e,bool spawn=false){
 if(spawn){
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];edit.spawn=true;
  rememberAction(std::move(edit));
 }else if(recordingContextEdit)rememberAction(capturePropertyEdit(e));
}
void keepDetachedChildCreation(const Owned& parent,const Owned& child){
 // Duplicate groups already record their children. A normal spawn's
 // record must keep the detached member even if the original root dies.
 const auto found=edits.lastFor(parent.entity,[&](const Edit& edit){
  return (edit.spawn&&edit.entity==parent.entity&&edit.generation==entityGeneration[parent.entity])||
   std::any_of(edit.group.begin(),edit.group.end(),[&](const SpawnedEdit& member){return member.owner==parent.id;});
 });
 if(found==edits.all().end())return;
 edits.detachChild(found->first,{parent.entity,entityGeneration[parent.entity],parent.id},
  {child.entity,entityGeneration[child.entity],child.id});
}
void forgetJointEdit(unsigned id){
 edits.forgetAttachment(id);
}
jar::constraint::Joint* selectedRope(const gentity_t& e,uint32_t id){
 if(!id)return nullptr;
 const auto key=attachmentKey(e);
 for(unsigned i=0;i<state.jointCount;++i){auto& joint=state.joints[i];
  if(joint.id==id&&joint.kind==jar::constraint::Rope&&(joint.first==key||joint.second==key))return &joint;
 }
 return nullptr;
}
bool resizeRope(jar::constraint::Joint&,float);
Edit* rememberPositionLink(gentity_t& source,const char* name){
 if(!recordingContextEdit)return nullptr;
 rememberEdit(source);auto& edit=edits.back();edit.positionLinkEdit=true;
 if(source.target)edit.positionLinkBefore=source.target;
 if(name)edit.positionLinkAfter=name;
 return &edit;
}
bool positionLinkMatches(const gentity_t& source,const Edit& edit){
 if(!edit.positionLinkEdit)return true;
 return edit.positionLinkAfter==(source.target?source.target:"")&&
  (edit.positionLinkBefore.empty()||G_Find(nullptr,FOFS(targetname),edit.positionLinkBefore.c_str()));
}
void restorePositionLink(gentity_t& source,const Edit& edit){
 if(edit.positionLinkEdit)G_LogicSetOutput(source,0,edit.positionLinkBefore.empty()?nullptr:edit.positionLinkBefore.c_str());
}
void editPushAimLink(gentity_t& source,const char* name){
 jar::push::Settings before;
 if(!G_PushRead(source,before))return;
 auto after=before;
 if(name&&after.aim==jar::push::Forward)after.aim=jar::push::Linked;
 if(jar::push::Same(before,after)&&((!source.target&&!name)||
    (source.target&&name&&!strcmp(source.target,name))))return;
 if(auto* edit=rememberPositionLink(source,name)){edit->pushBefore=before;edit->pushAfter=after;}
 G_LogicSetOutput(source,0,name);G_PushSet(source,after);
}
void editTeleportLink(gentity_t& source,const char* name){
 jar::teleport::Settings settings;
 if(!G_TeleportRead(source,settings)||(!source.target&&!name)||
    (source.target&&name&&!strcmp(source.target,name)))return;
 if(auto* edit=rememberPositionLink(source,name))edit->teleportBefore=edit->teleportAfter=settings;
 G_LogicSetOutput(source,0,name);
}
void editLaserEmitterAim(gentity_t& source,const char* name){
 if((!source.target&&!name)||(source.target&&name&&!strcmp(source.target,name)))return;
 rememberPositionLink(source,name);
 G_LogicSetOutput(source,0,name);
 G_SandboxLaserAimChanged(source);
}
void editExplosionTrailAim(gentity_t& source,const char* name){
 if((!source.target&&!name)||(source.target&&name&&!strcmp(source.target,name)))return;
 auto* edit=rememberPositionLink(source,name);
 if(edit)edit->positionLinkBeforeLifetime=G_EntityLifetime(G_WorkshopFxEndpoint(source));
 G_LogicSetOutput(source,0,name);
 source.enemy=nullptr;
 if(edit)edit->positionLinkAfterLifetime=G_EntityLifetime(G_WorkshopFxEndpoint(source));
}
void editPortalLink(gentity_t& source,gentity_t* target){
 const auto* name=target?target->targetname:nullptr;
 if((!source.target&&!name)||(source.target&&name&&!strcmp(source.target,name)))return;
 const WorkshopActionScope propertyEdit(recordingContextEdit);
 auto* edit=rememberPositionLink(source,name);
 edit->positionLinkBeforeLifetime=G_EntityLifetime(G_PortalLinkTarget(source,source.target));
 edit->positionLinkAfterLifetime=G_EntityLifetime(target);
 G_LogicSetOutput(source,0,name);
 G_PortalLinkChanged(source,target);
 G_PortalFrame();
}
void distanceLink(gentity_t& source,int role,const char* name){
 const char* before=G_DistanceLink(source,role);
 if((!before&&!name)||(before&&name&&!strcmp(before,name)))return;
 if(recordingContextEdit){
  rememberEdit(source);
  auto& edit=edits.back();
  edit.distanceLink=role;
  edit.distancePreviousLink=before?before:"";
  edit.distanceAppliedLink=name?name:"";
  if(before){
   const auto* previous=G_Find(nullptr,FOFS(targetname),before);
   if(previous)edit.distancePreviousLifetime=G_EntityLifetime(previous);
   else if(role==JAR_DISTANCE_REFERENCE)edit.distancePreviousLink=JAR_DISTANCE_MISSING_REFERENCE_NAME;
  }
 }
 G_DistanceSetLink(source,role,name);
}
void editSlottedLink(gentity_t& source,int slot,const char* name){
 const char* before=G_LogicOutput(source,slot);
 if((!before&&!name)||(before&&name&&!strcmp(before,name)))return;
 if(recordingContextEdit){
  rememberEdit(source);
  auto& edit=edits.back();
  edit.slottedLinkSlot=slot;
  edit.slottedLinkBefore=before?before:"";
  edit.slottedLinkAfter=name?name:"";
 }
 setSlottedLink(source,slot,name);
}
void forgetEdits(unsigned entity){
 ++entityGeneration[entity];
 edits.forgetEntity(entity);
}
bool undoCreation(){
 while(true){
  const auto& history=edits.all();
  const auto found=std::find_if(history.rbegin(),history.rend(),[](const auto& entry){return isCreation(entry.second);});
  if(found==history.rend())break;
  const auto key=found->first;
  const Edit& edit=found->second;
  if(!edit.group.empty()){
   // Removing members also prunes this record from the live history.
   auto group=edit.group;
   group.erase(std::remove_if(group.begin(),group.end(),[](const SpawnedEdit& member){return !spawnGroupMember(member);}),group.end());
   if(!group.empty())return undoSpawnGroup(group);
  }
  if(edit.joint){
   if(removeJoint(edit.joint)){message("Last attachment removed; prop poses retained.");return true;}
  }else if(edit.spawn){
   const auto* entry=owned(edit.entity);
   if(entry&&entityGeneration[edit.entity]==edit.generation&&matches(*entry)){
    if(!remove(edit.entity))return false;
    message("Last spawn removed.");
    return true;
   }
  }
  edits.erase(key);
 }
 message("No spawned objects or Toolgun creations to undo.");
 return false;
}
void undoFeedback();
void undoEdit(){
 if(sandboxReady()&&undoCreation())undoFeedback();
}
void revertProperty(bool vehicleResourcesOnly=false){
 while(true){
  const auto found=edits.lastFor(selected,[vehicleResourcesOnly](const Edit& edit){
   return edit.context&&edit.entity==selected&&(!vehicleResourcesOnly||edit.vehicleResourcesEdit);
  });
  if(found==edits.all().end())break;
  const Edit edit=found->second;
  const auto retire=[key=found->first]{edits.erase(key);};
  auto&e=g_entities[edit.entity];
  if(!e.inuse || edit.generation!=entityGeneration[edit.entity]){retire();continue;}
  if(edit.thrusterId){
   auto* thruster=ThrusterFor(state,attachmentKey(e));
   if(!thruster||thruster->id!=edit.thrusterId){retire();continue;}
   if(!edit.thrusterAfter.Matches(*thruster)){
    message("Thruster properties changed after this edit.");return;
   }
   edit.thrusterBefore.Apply(*thruster);
   retire();message("Previous thruster properties restored.");return;
  }
  if(edit.ropeLengthId){
   auto* rope=selectedRope(e,edit.ropeLengthId);
   if(!rope){retire();continue;}
   if(rope->length!=edit.ropeLengthAfter){message("Rope length changed after this edit.");return;}
   if(!resizeRope(*rope,edit.ropeLengthBefore)){message("The previous rope length could not be restored around scenery.");return;}
   retire();message(va("Previous length restored for rope %u.",edit.ropeLengthId));return;
  }
  if(edit.propPhysicsEdit){
   auto* o=owned(edit.entity);
   if(o&&matches(*o)&&prop(o->catalog)){
    o->physics=edit.propPhysics;
    retire();message("Previous prop mass and gravity restored.");return;
   }
   retire();continue;
  }
  if(edit.skyEdit){if(!G_SkyPortalSet(e,edit.sky,edit.skyOrder)){message("Sky viewpoint no longer available; You can retry Revert property.");return;}retire();message("Previous sky properties and priority restored.");return;}
  if(edit.combatPointEdit){if(!G_CombatPointSet(e,edit.combatPoint)){message("Combat point no longer registered; You can retry Revert property.");return;}retire();message("Previous combat point properties restored.");return;}
  if(edit.referenceEdit){if(!G_ReferenceSet(e,edit.reference)){message("Previous tag name is occupied or unavailable; You can retry Revert property.");return;}retire();message("Previous reference tag properties restored.");return;}
  if(edit.poseEdit){if(!setPropPose(e,edit.poseFrame)){message("Previous model pose is obstructed or unavailable; You can retry Revert property.");return;}retire();message("Previous model pose and collision restored.");return;}
  if(edit.pickupLifetimeItem){
   const int after=edit.pickupLifetimeAfter?
    std::max(1,edit.pickupLifetimeAfter-(pickupLifetimeClock(e)-edit.pickupLifetimeEditClock)):0;
   if(!pickupLifetimeEditable(e)||e.item!=edit.pickupLifetimeItem||pickupLifetimeRemaining(e)!=after){
    message("This pickup's lifetime changed after the edit. Revert requires the applied lifetime.");return;
   }
   pickupLifetimeSet(e,edit.pickupLifetimeBefore);retire();message("Previous pickup lifetime restored.");return;
  }
  if(edit.portableShieldEdit){
   jar::shieldCopy::Properties current;
   if(!G_PortableShieldProperties(e,current)||!jar::shieldCopy::Same(current,edit.portableShieldAfter)){
    message("Shield properties changed after this edit. Revert requires the applied values.");return;
   }
   if(!G_PortableShieldSetProperties(e,edit.portableShieldBefore)){
    message("The previous shield hull is obstructed or its expiry time passed.");return;
   }
   retire();message("Previous shield properties restored.");return;
  }
  if(edit.mountedChargeEdit){
   jar::chargeCopy::Properties current;
   const auto* support=G_SandboxChargeSupport(e);
   const unsigned slot=support?support->s.number:ENTITYNUM_NONE;
   if(slot!=edit.mountedChargeSupport||(support&&entityGeneration[slot]!=edit.mountedChargeSupportGeneration)||
      !contextChargeProperties(e,current,support)||!jar::chargeCopy::Same(current,edit.mountedChargeAfter)){
    message("The charge or its mount changed after this edit.");return;
   }
   if(!G_SandboxChargeSetProperties(e,edit.mountedChargeBefore)){
    message("The charge is busy or its previous expiry passed.");return;
   }
   retire();message("Previous charge properties restored.");return;
  }
  if(edit.pickupQuantityItem){
   if(!G_WorkshopPickupQuantityEditable(e)||e.item!=edit.pickupQuantityItem||e.count!=edit.pickupQuantityAfter){
    message("This pickup changed after the edit. Revert requires the applied quantity.");return;
   }
   e.count=edit.pickupQuantityBefore;retire();
   message(e.item->giType==IT_POWERUP?"Previous Force pickup duration restored.":"Previous pickup quantity restored.");return;
  }
  if(edit.holocronEdit){if(!G_HolocronEditable(e)||e.item->giTag!=edit.holocronPower){message("This entity no longer has the same holocron power.");return;}e.count=edit.holocronLevel;retire();message("Previous holocron level restored.");return;}
  if(edit.weatherEdit){if(!G_WeatherSet(e,edit.weather,false)){message("This entity no longer supports weather properties.");return;}e.delay=edit.weatherOrder;retire();message("Previous weather properties and priority restored.");return;}
  if(edit.crystalEdit){if(!G_CrystalCrateSet(e,edit.crystal)){message("Crate properties cannot be restored: solidity is obstructed or the entity changed.");return;}retire();message("Previous crystal-crate properties restored.");return;}
  if(edit.railEdit){if(!G_RailSet(e,edit.rail)){message("Rail controller no longer available.");return;}retire();message("Previous rail settings restored.");return;}
  if(edit.waypointEdit){if(!G_WaypointSet(e,edit.waypoint)){message("Waypoint no longer available.");return;}retire();message("Previous navigation settings restored.");return;}
  if(edit.sceneCameraEdit){if(!G_SceneCameraSet(e,edit.sceneCamera)){message("Scene camera no longer available.");return;}retire();message("Previous camera properties restored.");return;}
  if(edit.doorCameraEdit){if(!G_DoorCameraSet(e,edit.doorCamera)){message("Door-camera properties no longer available.");return;}retire();message("Previous door-camera properties restored.");return;}
  if(edit.slidingDoorEdit){
   if(!G_SandboxSlideSettings(e,edit.slidingDoor)){message("Return the mover to rest before reverting its settings. You can retry Revert property.");return;}
   if(edit.slidingDoorInactive)e.svFlags|=SVF_INACTIVE;else e.svFlags&=~SVF_INACTIVE;
   retire();message("Previous mover settings restored.");return;
  }
  if(edit.bobbingEdit){
   jarBobState_t current{};
   if(!G_BobbingRead(e,current)||!BobbingSameSettings(current,edit.bobbingAfter)){
    message("Platform settings changed after this edit. You can retry Revert property.");return;
   }
   if(!G_BobbingSettings(e,edit.bobbingBefore))return;
   retire();message("Previous platform settings restored.");return;
  }
  if(edit.pushBefore.kind!=jar::push::None){
   jar::push::Settings current;
   if(!owned(e.s.number)||!G_PushRead(e,current)||!jar::push::Same(current,edit.pushAfter)||
      !positionLinkMatches(e,edit)){
    message("Push settings or aim changed. You can retry Revert property.");return;
   }
   if(!G_PushSet(e,edit.pushBefore))return;
   restorePositionLink(e,edit);
   retire();message("Previous push settings restored.");return;
  }
  if(edit.teleportBefore.kind!=jar::teleport::None){
   jar::teleport::Settings current;
   if(!owned(e.s.number)||!G_TeleportRead(e,current)||!jar::teleport::Same(current,edit.teleportAfter)||
      !positionLinkMatches(e,edit)){
    message("Teleport settings or destination changed. You can retry Revert property.");return;
   }
   if(!G_TeleportSet(e,edit.teleportBefore))return;
   restorePositionLink(e,edit);
   retire();message("Previous teleport settings restored.");return;
  }
  if(edit.laserEmitterEdit||(edit.positionLinkEdit&&G_SandboxLaser(e))){
   jar::sandboxLaser::Settings current;
   if(!owned(e.s.number)||!G_SandboxLaserRead(e,current)||!positionLinkMatches(e,edit)||
      (edit.laserEmitterEdit&&!jar::sandboxLaser::Same(current,edit.laserEmitterAfter))){
    message("Laser emitter settings or aim changed. You can retry Revert property.");return;
   }
   if(edit.laserEmitterEdit&&!G_SandboxLaserSet(e,edit.laserEmitterBefore))return;
   restorePositionLink(e,edit);
   if(edit.positionLinkEdit)G_SandboxLaserAimChanged(e);
   retire();message("Previous laser emitter settings restored.");return;
  }
  if(edit.positionLinkEdit&&G_WorkshopTrailClass(e)){
   const auto* previous=edit.positionLinkBefore.empty()?nullptr:G_Find(nullptr,FOFS(targetname),edit.positionLinkBefore.c_str());
   if(!G_JAR_WorkshopOwns(&e)||!positionLinkMatches(e,edit)||
      (!edit.positionLinkBefore.empty()&&(!previous||previous==&e||!edit.positionLinkBeforeLifetime||
       G_EntityLifetime(previous)!=edit.positionLinkBeforeLifetime))||
      G_EntityLifetime(G_WorkshopFxEndpoint(e))!=edit.positionLinkAfterLifetime){
    message("Explosion Trail aim changed or its previous target is gone.");return;
   }
   restorePositionLink(e,edit);e.enemy=nullptr;
   retire();message("Previous Explosion Trail aim restored.");return;
  }
  if(edit.stationBefore.capacity){
   jar::sandboxStation::Settings current;
   if(!owned(e.s.number)||!G_SandboxStationRead(e,current)||!jar::sandboxStation::SameProperties(current,edit.stationAfter)){
    message("Station settings changed. You can retry Revert property.");return;
   }
   auto restored=edit.stationBefore;
   // Account for supply dispensed or regenerated after this edit.
   const int charge=current.charge+edit.stationBefore.charge-edit.stationAfter.charge;
   restored.charge=std::max(0,std::min(restored.capacity,charge));
   if(!G_SandboxStationSet(e,restored))return;
   retire();message("Previous station settings restored.");return;
  }
  if(edit.breakableBefore.capacity){
   jar::sandboxBreakable::Settings current;
   const auto* owner=owned(e.s.number);
   if(!owner||!G_SandboxBreakableRead(e,current)){
    message("Breakable unavailable. You can retry Revert property.");return;
   }
   auto expected=edit.breakableAfter;
   const bool samePhase=edit.breakableBefore.phase==expected.phase;
   if(samePhase)expected.health=current.health;
   if(!jar::sandboxBreakable::Same(current,expected)||
      (edit.breakableBefore.phase==jar::sandboxBreakable::Broken&&Constrained(state,owner->id))){
    message("Breakable state changed. You can retry Revert property.");return;
   }
   auto restored=edit.breakableBefore;
   if(samePhase&&restored.phase==jar::sandboxBreakable::Intact){
    const int health=current.health+restored.health-edit.breakableAfter.health;
    restored.health=std::max(1,std::min(restored.capacity,health));
   }
   if(!G_SandboxBreakableSet(e,restored)){message("Breakable is obstructed. You can retry Revert property.");return;}
   retire();message("Previous breakable settings restored.");return;
  }
  if(edit.usableEdit){
   jar::sandboxUsable::Settings current;
   if(!owned(e.s.number)||!G_SandboxUsableRead(e,current)){
    message("Usable unavailable. You can retry Revert property.");return;
   }
   auto expected=edit.usableAfter;
   auto restored=edit.usableBefore;
   const bool settingsOnly=expected.solid==restored.solid&&expected.pending==restored.pending&&
    expected.mode==restored.mode&&expected.spent==restored.spent&&expected.remaining==restored.remaining;
   if(settingsOnly){
    expected.remaining=restored.remaining=current.remaining;
    expected.health=current.health;
    restored.health=std::max(0,std::min(restored.capacity,current.health+edit.usableBefore.health-edit.usableAfter.health));
   }
   if(!jar::sandboxUsable::Same(current,expected)){
    message("Usable state changed. You can retry Revert property.");return;
   }
   if(!G_SandboxUsableSet(e,restored)){message("Usable is obstructed. You can retry Revert property.");return;}
   retire();message("Previous usable settings restored.");return;
  }
  if(edit.contactBefore.kind!=jar::contactTrigger::None){
   jar::contactTrigger::Settings current;
   if(!owned(e.s.number)||!G_ContactRead(e,current)||!jar::contactTrigger::Same(current,edit.contactAfter)||
      !G_ContactSettings(e,edit.contactBefore)){
    message("Contact trigger settings changed. You can retry Revert property.");return;
   }
   retire();message("Previous contact trigger settings restored.");return;
  }
  if(edit.contactRearmed){
   if(!owned(e.s.number)||G_ContactKind(e)!=jar::contactTrigger::Once||G_ContactPending(e)){
    message("Contact trigger is pending or unavailable. You can retry Revert property.");return;
   }
   G_ContactSetSpent(e,true);
   retire();message("Once trigger returned to its spent state.");return;
  }
  if(edit.slottedLinkSlot>=0){
   const char* current=G_LogicOutput(e,edit.slottedLinkSlot);
   auto* previous=edit.slottedLinkBefore.empty()?nullptr:G_Find(nullptr,FOFS(targetname),edit.slottedLinkBefore.c_str());
   const auto kind=G_LogicKind(e);
   const bool usableLink=G_SandboxUsable(e)&&edit.slottedLinkSlot<2;
   if(!owned(e.s.number)||(!jar::logic::Slotted(kind)&&!usableLink)||edit.slottedLinkAfter!=(current?current:"")||
      (!edit.slottedLinkBefore.empty()&&(!previous||!owned(previous->s.number)||
       (kind==jar::logic::Give&&!G_LogicPickup(*previous))||
       (jar::logic::UseGate(kind)&&!G_LogicCanSetUse(*previous))||(usableLink&&previous->e_UseFunc==useF_NULL)))){
    message("Link or destination changed. You can retry Revert property.");return;
   }
   setSlottedLink(e,edit.slottedLinkSlot,previous?previous->targetname:nullptr);
   retire();message("Previous link restored.");return;
  }
  if(edit.logicBefore.kind!=jar::logic::None){
   jar::logic::State current;
   if(!owned(e.s.number)||!G_LogicRead(e,current)||!jar::logic::SameSettings(current,edit.logicAfter)){
    message("Logic marker settings changed. You can retry Revert property.");return;
   }
   auto previous=edit.logicBefore;
   if(previous.kind==jar::logic::Counter&&current.remaining!=edit.logicAfter.remaining){
    if(previous.hits!=current.hits||edit.logicBefore.remaining!=edit.logicAfter.remaining){
     message("Counter progress changed. You can retry Revert property.");return;
    }
    previous.remaining=current.remaining;
   }
   if(jar::logic::OneShotMode(previous.kind)&&current.remaining!=edit.logicAfter.remaining){
    if(edit.logicBefore.remaining!=edit.logicAfter.remaining){
     message("Marker has fired since the edit. You can retry Revert property.");return;
    }
    previous.remaining=previous.repeat?1:current.remaining;
   }
   if(!G_LogicSet(e,previous))return;
   retire();message("Previous logic marker settings restored.");return;
  }
  if(edit.movementMarkerBefore.kind!=jar::movement::None){
   jar::movement::Marker current;
   if(!owned(e.s.number)||!G_MovementMarkerRead(e,current)||!jar::movement::Same(current,edit.movementMarkerAfter)||
      !G_MovementMarkerSet(e,edit.movementMarkerBefore)){
    message("Movement marker changed. You can retry Revert property.");return;
   }
   retire();message("Previous movement marker settings restored.");return;
  }
  if(edit.actorMovementEdit){
   jar::movement::ActorState current;
   if(!G_MovementActorRead(e,current)||!jar::movement::Same(current,edit.movementAfter)||
      !G_MovementActorSet(e,edit.movementBefore)){
    message("Actor movement changed or is temporarily unavailable. You can retry Revert property.");return;
   }
   retire();message("Previous actor movement settings restored.");return;
  }
  if(edit.buttonWait>=0){
   if(!G_ButtonSettings(e,edit.buttonWait,edit.buttonEnabled)){message("Button properties no longer available.");return;}
   retire();message("Previous button settings restored.");return;
  }
  if(edit.welderEdit){if(!G_WelderSet(e,edit.welder)){message("Welder properties no longer available.");return;}retire();message("Previous welder properties restored.");return;}
  if(edit.spotEdit){if(!G_SpotlightSet(e,edit.spot)){message("Spotlight properties no longer available.");return;}retire();message("Previous spotlight properties restored.");return;}
  if(edit.portalEdit||(edit.positionLinkEdit&&G_PortalPortable(e))){
   jar::portal::Settings current;
   auto* previous=G_PortalLinkTarget(e,edit.positionLinkBefore.c_str());
   if(!G_PortalRead(e,current)||(edit.portalEdit&&!jar::portal::Same(current,edit.portalAfter))||
      (edit.positionLinkEdit&&(!G_JAR_WorkshopOwns(&e)||
       edit.positionLinkAfter!=(e.target?e.target:"")||
       (!edit.positionLinkBefore.empty()&&(!edit.positionLinkBeforeLifetime||
        G_EntityLifetime(previous)!=edit.positionLinkBeforeLifetime))||
       G_EntityLifetime(G_PortalLinkTarget(e,e.target))!=edit.positionLinkAfterLifetime))){
    message("Portal properties or its link changed, or the previous target is gone.");return;
   }
   if(edit.portalEdit&&!G_PortalSet(e,edit.portalBefore))return;
   if(edit.positionLinkEdit){
    restorePositionLink(e,edit);
    G_PortalLinkChanged(e,previous);
   }
   const bool linkEdit=edit.positionLinkEdit;
   retire();G_PortalFrame();
   message(linkEdit?"Previous portal link restored.":"Previous portal properties restored.");return;
  }
  if(edit.laserEdit){if(!G_LaserArmSet(e,edit.laser)){message("Laser properties are no longer available.");return;}retire();message("Previous laser properties restored; firing stopped.");return;}
  if(edit.reflectionEdit){if(!G_ReflectionSet(e,edit.reflection)){message("Reflection probe no longer available.");return;}retire();message("Previous reflection properties restored.");return;}
  if(edit.cloudEdit){if(!G_CloudSet(e,edit.cloud)){message("This entity no longer supports cloud properties.");return;}retire();message("Previous cloud properties restored.");return;}
  if(edit.boundaryEdit){if(!G_ShipBoundarySet(e,edit.boundaryTime,edit.boundaryPreserve)){message("This entity no longer supports flight-boundary properties.");return;}retire();message("Previous boundary properties restored; current turns cancelled.");return;}
  if(edit.scriptEdit){if(!G_ScriptRunnerSet(e,edit.script)){message("Previous script is no longer available.");return;}retire();message("Previous runner properties restored. Pending activation cancelled.");return;}
  if(edit.parmSlot>=0){jar::entityParms::Literal(e,edit.parmSlot,edit.parm);retire();message("Previous script parameter restored.");return;}
  if(edit.levelChangeEdit){if(!G_LevelChangeSet(e,edit.levelChange)){message("Previous destination map is no longer available.");return;}retire();message("Previous travel settings restored.");return;}
  if(edit.angularBefore.kind){
   jar::sandboxAngular::Settings current;
   if(!owned(e.s.number)||!G_SandboxAngularRead(e,current)||!jar::sandboxAngular::SameControls(current,edit.angularAfter)){
    message("Mover settings changed. You can retry Revert property.");return;
   }
   auto previous=edit.angularBefore;
   previous.phase=int(int64_t(current.phase)*previous.period/current.period);
   if(!G_SandboxAngularSet(e,previous))return;
   retire();message("Previous motion settings restored at the current pose.");return;
  }
  if(edit.speakerEdit){
   jar::speaker::Settings current;
   if(!owned(e.s.number)||!G_SandboxSpeakerRead(e,current)||!jar::speaker::SameControl(current,edit.speakerAfter)){
    message("Speaker state changed. You can retry Revert property.");return;
   }
   if(!G_SandboxSpeakerSet(e,edit.speakerBefore)){message("Previous speaker sound is unavailable. You can retry Revert property.");return;}
   retire();message("Previous speaker settings restored.");return;
  }
  if(edit.musicEdit){if(!G_MusicSetTrack(e,edit.music.c_str())){message("Previous music track is no longer available.");return;}retire();message("Previous music track restored.");return;}
  if(edit.labelEdit){
   if(!editableLabel(e)){message("This entity no longer supports labels.");return;}
   if(e.message&&gi.bIsFromZone(e.message,TAG_G_ALLOC))gi.Free(e.message);e.message=edit.label.empty()?nullptr:G_NewString(edit.label.c_str());
   retire();message("Previous label restored.");return;
  }
  if(edit.alwaysDelay){
   if(!G_AlwaysPortable(e)){message("This entity no longer supports a one-shot delay.");return;}
   e.delay=edit.alwaysDelay;retire();message("Previous one-shot delay restored.");return;
  }
  if(edit.visibleEdit){
   if(!G_VisiblePortable(e)){message("This entity no longer supports sight settings.");return;}
   G_VisibleSet(e,edit.visible);
   if(edit.visibleInactive)e.svFlags|=SVF_INACTIVE;else e.svFlags&=~SVF_INACTIVE;
   retire();message("Previous sight settings restored.");return;
  }
  if(edit.distanceRange||edit.distanceArmEdit||edit.distanceLink>=0){
   if(!G_DistancePortable(e)){message("This entity no longer supports distance settings.");return;}
   if(edit.distanceLink>=0){
    const char* current=G_DistanceLink(e,edit.distanceLink);
    const auto& name=edit.distancePreviousLink;
    if(edit.distanceAppliedLink!=(current?current:"")){
     message("Distance link changed after this edit.");return;
    }
    if(!name.empty()&&!(edit.distanceLink==JAR_DISTANCE_REFERENCE&&name==JAR_DISTANCE_MISSING_REFERENCE_NAME)){
     const auto* previous=G_Find(nullptr,FOFS(targetname),name.c_str());
     if(!edit.distancePreviousLifetime||!previous||G_EntityLifetime(previous)!=edit.distancePreviousLifetime){
      message("Previous distance link destination is unavailable.");return;
     }
    }
    G_DistanceSetLink(e,edit.distanceLink,name.c_str());
   }
   if(edit.distanceRange){e.count=edit.distanceRange;e.spawnflags=edit.distanceSubjects;}
   if(edit.distanceArmEdit){if(edit.distanceWasInactive)e.svFlags|=SVF_INACTIVE;else e.svFlags&=~SVF_INACTIVE;}
   retire();message("Previous distance trigger setting restored.");return;
  }
  if(edit.wallVisibility>=0||edit.wallEnabledEdit){
   if(!G_WallPortable(e)){message("This entity no longer supports wall properties.");return;}
   if(edit.wallVisibility>=0&&!G_WallSetVisible(e,edit.wallVisibility!=0)){message("Previous wall state is obstructed. You can retry Revert property.");return;}
   if(edit.wallEnabledEdit){if(edit.wallWasInactive)e.svFlags|=SVF_INACTIVE;else e.svFlags&=~SVF_INACTIVE;}
   retire();message("Previous wall property restored.");return;
  }
  if(edit.spaceEdit){
   if(!G_SpacePortable(e)){retire();continue;}
   G_SpaceSet(e,edit.spaceArmed,edit.spaceEnabled);
   retire();message("Previous space volume activation restored.");return;
  }
  if(edit.volumeEdit){
   if(!jar::volumeEntity::Portable(e)||!jar::volume::Valid(edit.pos1,edit.pos2)){message("The selected entity no longer has editable volume dimensions.");return;}
   VectorCopy(edit.pos1,e.pos1);VectorCopy(edit.pos2,e.pos2);
   jar::volumeEntity::Refresh(e);gi.linkentity(&e);retire();message("Previous volume dimensions restored.");return;
  }
  if(edit.replicatorHealBefore!=-1){
   if(!G_SandboxReplicator(e)||G_ReplicatorHeal(e)!=edit.replicatorHealAfter||
      G_ReplicatorDelay(e)!=edit.replicatorDelayAfter||G_LogicUseEnabled(e)!=edit.replicatorEnabledAfter){
    message("Replicator settings changed after this edit. Revert requires the applied settings.");return;
   }
   e.damage=edit.replicatorHealBefore;e.wait=edit.replicatorDelayBefore;
   G_LogicSetUseEnabled(e,edit.replicatorEnabledBefore);
   retire();message("Previous Replicator settings restored.");return;
  }
  if(edit.secretFlagsBefore!=-1){
   if(!G_SandboxSecret(e)||(e.spawnflags&JAR_SECRET_FLAGS)!=edit.secretFlagsAfter||G_LogicUseEnabled(e)!=edit.secretEnabledAfter){
    message("This Secret marker changed after the edit. Revert requires the applied state.");return;
   }
   e.spawnflags=(e.spawnflags&~JAR_SECRET_FLAGS)|edit.secretFlagsBefore;
   G_LogicSetUseEnabled(e,edit.secretEnabledBefore);
   retire();message("Previous Secret marker settings restored.");return;
  }
  if(edit.shooterEdit){
   jar::shooter::Settings current;
   if(!G_SandboxShooterRead(e,current)||!jar::shooter::Same(current,edit.shooterAfter)){
    message("Shooter settings changed after this edit. Revert requires the applied settings.");return;
   }
   const bool restoresPending=(!current.repeat||!current.enabled)&&edit.shooterPendingMs;
   if(!G_SandboxShooterSet(e,edit.shooterBefore))return;
   if(restoresPending){
    const auto control=nativeControls.find(e.s.number);
    const int clock=control==nativeControls.end()?level.time:control->second.started;
    G_SandboxShooterSchedule(e,clock,edit.shooterPendingMs);
   }
   retire();message("Previous shooter settings restored.");return;
  }
  if(edit.actorGoalEdit){
   jar::npcOrder::Settings order;
   if(!contextActorOrder(e,order)||!jar::npcOrder::Same(G_WorkshopReadNPCGoal(*e.NPC),edit.actorGoalAfter)||
      (edit.actorGoalBefore.owner&&!G_WorkshopResolveNPCGoal(edit.actorGoalBefore))){
    message("The NPC goal changed or its previous destination is gone.");return;
   }
   setContextActorGoal(e,edit.actorGoalBefore,edit.actorPatrolBefore);retire();message("Previous NPC destination restored.");return;
  }
  if(edit.actorOrderEdit){
   jar::npcOrder::Settings current,restored;
   if(!contextActorOrder(e,current)||!jar::npcOrder::Merge(current,edit.actorOrderAfter,edit.actorOrderBefore,restored)){
    message("Revert requires the same NPC and the applied order values.");return;
   }
   applyContextActorOrder(e,current,restored);
   if(current.mode!=restored.mode)G_WorkshopWriteNPCPatrol(*e.NPC,edit.actorPatrolBefore);
   retire();message("Previous NPC order restored.");return;
  }
  if(edit.actorWeaponEdit){
   const uint32_t choices=contextActorWeaponChoices(e);
   const int oldWeapon=edit.actorWeaponBefore.weapon;
   if(!choices||!(choices&(1u<<oldWeapon))||e.client->ps.weapon!=edit.actorWeaponAfter){
    message("Revert requires the same NPC and equipped weapon.");return;
   }
   const auto& ps=e.client->ps;
   const uint32_t weapons=uint32_t(ps.stats[STAT_WEAPONS])&~edit.actorWeaponGrant;
   if(oldWeapon!=WP_NONE&&oldWeapon!=WP_MELEE&&!(weapons&(1u<<oldWeapon))){
    message("The NPC no longer carries the previous weapon.");return;
   }
   // Leave the live pool alone if shots or pickups changed the starting grant.
   const int pool=ps.ammo[edit.actorWeaponPool]==edit.actorWeaponGrantedAmmo?edit.actorWeaponPool:AMMO_NONE;
   if(!G_WorkshopApplyActorWeapon(e,edit.actorWeaponBefore,weapons,pool,0)){
    message("The previous weapon model could not be attached.");return;
   }
   retire();message("Previous NPC weapon restored.");return;
  }
  if(edit.actorFireEdit){
   WorkshopActorFire current,restored;
   if(!contextActorFire(e,current)||!G_WorkshopMergeActorFire(current,edit.actorFireAfter,edit.actorFireBefore,restored)){
    message("Revert requires the same NPC weapon and the previously applied fields.");return;
   }
   G_WorkshopApplyActorFire(e,current,restored);retire();message("Previous NPC weapon settings restored.");return;
  }
  if(edit.actorStatusEdit){
   jar::duplicate::ActorStatus current;
   if(!contextActorStatus(e,current)){message("Revert requires a living Sandbox NPC with free controls.");return;}
   jar::duplicate::ActorStatus restored;
   if(!mergeWorkshopActorStatus(current,edit.actorStatusAfter,edit.actorStatusBefore,restored)){
    message("NPC settings changed after this edit. Revert requires the applied values.");return;
   }
   if(!applyContextActorStatus(e,current,restored)){message("This NPC's target is locked by its controller.");return;}
   retire();message("Previous NPC settings restored.");return;
  }
  if(edit.vehicleResourcesEdit){
   jar::vehicleStatus::Values current;
   const bool animalHealthEdited=edit.vehicleResourcesBefore.value[0]!=edit.vehicleResourcesAfter.value[0]&&
    e.m_pVehicle&&e.m_pVehicle->m_pVehicleInfo&&e.m_pVehicle->m_pVehicleInfo->type==VH_ANIMAL;
   if(!contextVehicleResources(e,current)||
      current.value!=edit.vehicleResourcesAfter.value||(animalHealthEdited&&e.health!=current.value[0])){
    message("Revert requires a settled vehicle with the previously applied resources.");return;
   }
   if(!G_WorkshopSetVehicleResources(e,edit.vehicleResourcesBefore))return;
   if(animalHealthEdited){e.health=edit.health;e.client->ps.stats[STAT_HEALTH]=e.health;}
   retire();message("Previous vehicle resources restored.");return;
  }
  if(edit.healthEdit){
   if(!contextHealthEditable(e)||e.health!=edit.healthAfter){
    retire();message("Health changed after this edit. Current health was kept.");return;
   }
   e.health=edit.health;if(e.client)e.client->ps.stats[STAT_HEALTH]=e.health;retire();message("Previous health restored.");return;
  }
  if(!editable(e)){message("Release control or dismount before reverting the property.");return;}
  if(edit.vehiclePoseEdit){
   const auto control=nativeControls.find(e.s.number);
   if(control==nativeControls.end()||!control->second.frozen||held==unsigned(e.s.number)||
      Constrained(state,attachmentKey(e))){message("Freeze and unlink the vehicle before reverting its pose.");return;}
   if(!VectorCompare(e.currentOrigin,edit.appliedVehicleOrigin)||!VectorCompare(e.currentAngles,edit.appliedVehicleAngles)){
    message("The vehicle moved after this property edit. Revert would overwrite the new pose.");return;
   }
   VehicleTeleportPlan plan;
   if(!nativeVehiclePose(e,edit.origin,edit.angles,level.time,plan,false)){
    message("The vehicle or a seat cannot return to that pose. Revert is still available.");return;
   }
   nativeCommitVehiclePose(plan);retire();message("Previous vehicle pose restored.");return;
  }
  if(edit.freezeEdit){
   if(held==unsigned(e.s.number)){message("Release the Physgun before reverting Freeze or Gravity.");return;}
   auto* o=owned(edit.entity);
   if(o&&prop(o->catalog)){
    if(o->frozen==edit.frozen){retire();continue;}
    o->frozen=edit.frozen;physicsInvalidate(edit.entity);
   }else {
    if(bool(nativeControls.count(edit.entity))==edit.controlled){retire();continue;}
    if(edit.controlled&&((e.m_pVehicle&&!nativeVehicleHoldReady(e))||(e.client&&!e.m_pVehicle&&e.s.m_iVehicleNum))){
     message("The vehicle or rider is unavailable. Freeze revert is still available.");return;
    }
    if(edit.controlled)nativeBegin(e,true);else nativeRelease(edit.entity,false);
   }
   retire();message("Previous freeze state restored.");return;
  }
  if(auto*o=owned(edit.entity))if(prop(o->catalog)&&Constrained(state,o->id)){
   if(!edit.linked){message("Unlink this prop before restoring an earlier transform.");return;}
   // Connected transforms belong to the solver. Undo motion tools by restoring
   // the frozen flag at the current pose, preserving the joint's local frames.
   releaseHeld();o->frozen=edit.frozen;physicsInvalidate(edit.entity);retire();message("Previous freeze/gravity state restored at the current linked pose.");return;
  }
  trace_t tr{};
  // Authored models may deliberately intersect their mounting brush. Preserve
  // that original state, but never restore through a newly occupying actor.
  const bool section=jar::bspSection::Portable(e),door=G_SandboxSlidePortable(e),button=G_ButtonPortable(e),bob=G_BobbingPortable(e),angular=G_SandboxAngular(e);
  vec3_t lo,hi;VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);
  if(section)jar::bspSection::Hull(e.mins,e.maxs,edit.angles,lo,hi);
  if(door)G_SandboxSlideHull(edit.angles,lo,hi,G_LiftPortable(e));
  if(button)G_ButtonHull(edit.angles,lo,hi);
  if(bob)G_BobbingHull(edit.angles,lo,hi);
  if(angular)G_SandboxAngularHull(edit.angles,lo,hi);
  const bool moved=!VectorCompare(edit.origin,e.currentOrigin);
  const bool rotated=!VectorCompare(edit.angles,e.currentAngles);
  const auto* model=editedProp(e);
  std::vector<SandboxChargeMove> charges;
  if(model&&(moved||rotated)){
   if(!propTransformClear(e,*model,e.s.frame,edit.origin,edit.angles,charges,false)){
    message("Previous position is blocked. Clear space around the prop and its mounted charges.");return;
   }
  }else if(G_JAR_PushableMover(e)&&(moved||rotated)){
   if(Constrained(state,attachmentKey(e))){message("Unlink before reverting this mover's pose.");return;}
   if(held==unsigned(e.s.number)||!contextMoveMover(e,edit.origin,edit.angles)){
    message("Previous mover pose is blocked. Release the Physgun or clear the path and retry.");return;
   }
  }else if(!jar::volumeEntity::Portable(e)&&(moved||((door||button||bob)&&rotated))){
   BSPSectionMotionScope sectionMotion(e);if(!sectionMotion){message("Clear the section's moving brushes and riders before reverting its pose.");return;}
   gi.trace(&tr,edit.origin,lo,hi,edit.origin,e.s.number,(edit.embedded?0:MASK_SOLID)|CONTENTS_BODY,(EG2_Collision)0,0);
  }
  if(tr.startsolid || tr.allsolid){message("Previous position is blocked. Clear the space and try again.");return;}
  RackStockPlan rack;
  if((moved||rotated)&&!rackStockPose(e,edit.origin,edit.angles,false,rack)){
   message("Rack stock cannot fit at the previous pose. Clear the space and retry.");return;
  }
  releaseHeld();nativeRelease(e.s.number,false);G_SetOrigin(&e,edit.origin);G_SetAngles(&e,edit.angles);e.s.pos=edit.position;e.s.apos=edit.rotation;setupMoved(e);
  const int elapsed=std::max(0,level.time-edit.time);e.s.pos.trTime+=elapsed;e.s.apos.trTime+=elapsed;
  if(!jar::bspSection::Portable(e)){VectorCopy(edit.pos1,e.pos1);VectorCopy(edit.pos2,e.pos2);}
  if(edit.hasMoverMotion||G_JAR_MoverMotionActive(e))G_JAR_MoverMotionRestore(e,edit.moverMotion);
  if(!G_LaserArmPortable(e)&&!G_SandboxLaser(e)&&!G_SandboxStation(e)&&!G_SandboxBreakable(e)&&
     !G_SandboxUsable(e)&&!G_SandboxSpeaker(e)&&e.e_ThinkFunc!=thinkF_portable_shield_think)e.nextthink=edit.nextthink>0?edit.nextthink+elapsed:edit.nextthink;
  e.next_roff_time=edit.nextRoff>0?edit.nextRoff+elapsed:edit.nextRoff;
  if(G_SandboxLaser(e))G_SandboxLaserAimChanged(e);
  if(edit.controlled){nativeControls[e.s.number]=edit.native;nativeBegin(e,true);}
  setupMoved(e);
  if(auto*o=owned(e.s.number))o->frozen=edit.frozen;
  if(e.client){VectorCopy(edit.origin,e.client->ps.origin);SetClientViewAngle(&e,e.currentAngles);}
  physicsInvalidate(e.s.number);gi.linkentity(&e);placeEditedCharges(charges);commitRackStock(rack);retire();message("Last transform restored.");
  if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","undo_transform","entity=%d xyz=%.2f,%.2f,%.2f yaw=%.1f",e.s.number,e.currentOrigin[0],e.currentOrigin[1],e.currentOrigin[2],e.currentAngles[1]);return;
 }
 message("No property changes to revert on the selected entity.");
}
