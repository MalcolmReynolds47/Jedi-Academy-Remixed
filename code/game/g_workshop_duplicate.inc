// Immutable clipboard plus transactional assembly paste. GPL-2.0-only.
#include "g_workshop_charge_copy.inc"
#include "g_portable_shield.h"

jar::duplicate::Snapshot duplicateSnapshot;
std::map<uint32_t,jar::bspSection::CopyState> duplicateSections;

bool duplicateWorldAnchor(const jar::constraint::Joint& joint,vec3_t normal){
 if(!jar::constraint::Valid(joint))return false;
 const auto& frame=joint.first==jar::duplicate::worldAnchor?joint.a:joint.b;
 vec3_t facing{};
 if(joint.kind==jar::constraint::Rope){
  jar::physics::Vec point,axis;
  if(!jar::physics::FrameTransform({},frame,point,axis))return false;
  VectorSet(facing,axis.x,axis.y,axis.z);
 }
 return JAR_WorldAnchorSurface(frame.position,facing,G_JAR_WorkshopAnchorTrace,normal)!=qfalse;
}
void duplicateClear(){duplicateSnapshot={};duplicateSections.clear();gi.cvar_set("jar_duplicateStatus","Copy an entity or connected assembly");}
uint32_t duplicateDefinition(const gentity_t&e){
 if(const auto*o=owned(e.s.number))if(o->catalog!=consoleCatalog)return matches(*o)?o->catalog:0;
 if(e.NPC_type)for(const auto&a:actors)if(!Q_stricmp(e.NPC_type,a.name))return a.id;
 if(e.item){if(e.item->giType==IT_WEAPON&&CatalogValid(2000+e.item->giTag))return 2000+e.item->giTag;
  for(const auto&s:supplies)if(e.item==supplyItem(s))return s.id;}
 if(e.classname&&jar::entities::Find(e.classname))for(const auto&c:jar::catalog::entries)if(c.kind==5&&!c.reason[0]&&!strcmp(c.name,e.classname))return c.id;
 if(e.model)for(const auto&p:props)if(!Q_stricmp(e.model,p.model))return p.id;
 return 0;
}
bool duplicateCapture(const gentity_t&e,uint32_t id,uint32_t parent,const vec3_t anchor,jar::duplicate::Object&out){
 if(!editable(e)||occupied(e))return false;
 out.source=id;out.parent=parent;VectorSubtract(e.currentOrigin,anchor,out.offset);VectorCopy(e.currentAngles,out.angles);
 if(G_PortableShield(e)){
  if(!G_PortableShieldCopyState(e,out.deployedShield))return false;
  out.catalog=consoleCatalog;out.hasDeployedShield=true;
  jar::volume::Hull(out.deployedShield.mins,out.deployedShield.maxs,out.angles,out.mins,out.maxs);
  return true;
 }
 if(G_SandboxChargeCaptureStandalone(e,out.standaloneCharge)){
  out.catalog=consoleCatalog;out.hasStandaloneCharge=true;
  const float half=jar::chargeCopy::HalfSize(out.standaloneCharge.mode);
  VectorSet(out.mins,-half,-half,-half);VectorSet(out.maxs,half,half,half);
  return true;
 }
 out.catalog=duplicateDefinition(e);if(!out.catalog)return false;
 if(e.m_pVehicle){
  if(!actor(out.catalog)||!G_WorkshopCaptureVehicleStatus(e,out.vehicleStatus))return false;
  out.hasVehicleStatus=true;
 }else if(actor(out.catalog)){
  out.hasActorLoadout=G_WorkshopCaptureActorLoadout(e,out.actorLoadout);
  if(e.NPC&&e.client&&!e.m_pVehicle){
   out.hasActorStatus=readWorkshopActorStatus(e,out.actorStatus);
   out.actorOrder=G_WorkshopReadNPCOrder(*e.NPC);
   out.hasActorOrder=out.actorOrder.mode!=jar::npcOrder::Autonomous&&jar::npcOrder::Valid(out.actorOrder);
   const auto goal=G_WorkshopReadNPCGoal(*e.NPC);
   if(G_WorkshopResolveNPCGoal(goal))out.actorGoal=goal.owner;
   const auto cursor=G_WorkshopReadNPCPatrol(*e.NPC);
   if(out.actorOrder.mode==jar::npcOrder::Patrol&&G_WorkshopResolveNPCGoal(cursor))out.actorPatrolGoal=cursor.owner;
   const auto control=nativeControls.find(e.s.number);
   const bool workshopGrip=control!=nativeControls.end()&&!control->second.gripFlag;
   if(G_MovementActorRead(e,out.actorMovement,workshopGrip)){
    if(workshopGrip)out.actorMovement.gravity=control->second.gravity;
    if(!out.actorMovement.customGravity)out.actorMovement.gravity=0;
    out.hasActorMovement=jar::duplicate::ValidActorMovement(out.actorMovement);
   }
  }
 }
 VectorCopy(e.mins,out.mins);VectorCopy(e.maxs,out.maxs);
 if(jar::bspSection::Portable(e))jar::bspSection::Hull(e.mins,e.maxs,e.currentAngles,out.mins,out.maxs);
 if(const auto*p=prop(out.catalog)){out.frozen=owned(e.s.number)?owned(e.s.number)->frozen!=0:true;
  out.poseFrame=owned(e.s.number)?e.s.frame:0;
  if(const auto* body=owned(e.s.number))out.propPhysics=body->physics;
  if(!G_SandboxChargesCapture(e,out.charges))return false;
  PhysicsBounds bounds;if(!modelBounds(*p,bounds,out.poseFrame))return false;{
   vec3_t half,center;nativeVector(bounds.half,half);nativeVector(bounds.center,center);vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);
   for(int a=0;a<3;++a){float middle=0,radius=0;for(int k=0;k<3;++k){middle+=axis[k][a]*center[k];radius+=std::fabs(axis[k][a])*half[k];}out.mins[a]=middle-radius;out.maxs[a]=middle+radius;}
  }
 }else {auto control=nativeControls.find(e.s.number);out.frozen=control!=nativeControls.end()&&control->second.frozen;
  out.hasHealth=e.takedamage!=qfalse&&!G_SandboxBreakable(e)&&!G_SandboxUsable(e);if(out.hasHealth){if(e.health<=0)return false;out.health=e.health;}
 }
 if(G_RailPortable(e)){out.hasRail=true;if(!G_RailRead(e,out.rail))return false;}
 if(G_WaypointPortable(e)){out.hasWaypoint=true;if(!G_WaypointRead(e,out.waypoint))return false;}
 if(G_SceneCameraPortable(e)){out.hasSceneCamera=true;if(!G_SceneCameraRead(e,out.sceneCamera))return false;}
 if(G_SkyPortalPortable(e)){out.hasSky=true;if(!G_SkyPortalRead(e,out.sky))return false;out.sky.enabled=false;}
 if(G_CombatPointPortable(e)){out.hasCombatPoint=true;if(!G_CombatPointRead(e,out.combat))return false;}
 if(G_ReferencePortable(e)){out.hasReference=true;if(!G_ReferenceRead(e,out.reference))return false;}
 if(jar::levelChange::Class(e.classname)){out.hasLevelChange=true;if(!G_LevelChangeRead(e,out.levelChange))return false;}
 if(jar::scriptRunner::Class(e.classname)){out.hasScript=true;if(!G_ScriptRunnerRead(e,out.script))return false;}
 out.hasParms=true;for(unsigned i=0;i<jar::parms::count;++i)if(!jar::parms::Read(e.parms?e.parms->parm[i]:nullptr,out.parms[i]))return false;
 if(jar::music::Class(e.classname)&&e.message&&jar::music::Valid(e.message)){out.hasMusic=true;out.music=e.message;}
 if(editableLabel(e)&&e.message&&jar::context::Label(e.message)){out.hasLabel=true;out.label=e.message;}
 if(G_AlwaysPortable(e)){
  const auto control=nativeControls.find(e.s.number);
  out.alwaysDelay=e.delay;out.alwaysRemaining=G_AlwaysRemaining(e,control==nativeControls.end()?level.time:control->second.started);
  out.alwaysInactive=(e.svFlags&SVF_INACTIVE)!=0;
 }
 if(G_VisiblePortable(e)){
  out.hasVisible=true;
  out.visible=G_VisibleSettings(e);
  out.visibleArmed=e.e_ThinkFunc==thinkF_trigger_visible_check_player_visibility;
  out.visibleInactive=(e.svFlags&SVF_INACTIVE)!=0;
 }
 if(G_DistancePortable(e)){
  out.distanceRange=e.count;out.distanceSubjects=e.spawnflags;
  out.distanceInactive=(e.svFlags&SVF_INACTIVE)!=0;
  out.distanceReferenceRequired=e.ownername&&*e.ownername;
  if(!JAR_DistanceSettingsValid(out.distanceRange,out.distanceSubjects))return false;
 }
 if(G_WallPortable(e)){out.wallVisibility=e.count?1:0;out.wallInactive=(e.svFlags&SVF_INACTIVE)!=0;}
 if(e.classname&&(!strcmp(e.classname,"target_interest")||!strcmp(e.classname,"target_location")||G_JAR_PortableStart(e))){out.nativeMarker=true;out.markerInactive=(e.svFlags&SVF_INACTIVE)!=0;}
 out.hasCount=e.item!=nullptr;if(out.hasCount)out.count=e.count;
 out.rackShelfStock=parent&&rackStockChild(e)&&rackStockSuspended(e);
 if(pickupLifetimeEditable(e))out.pickupRemainingMs=pickupLifetimeRemaining(e);
 else if(e.item&&e.e_ThinkFunc==thinkF_G_FreeEntity&&e.nextthink==givePickup::reservedDeadline)
  out.pickupRemainingMs=e.setTime>0&&e.setTime<=jar::pickupLifetime::maxMs?e.setTime:givePickup::expiryMs;
 if(jar::volumeEntity::Class(e.classname)){out.triggerVolume=true;out.volumeEnabled=(e.contents&CONTENTS_TRIGGER)!=0;out.volumeInactive=(e.svFlags&SVF_INACTIVE)!=0;}
 if(G_WeatherPortable(e)){out.hasWeather=true;if(!G_WeatherRead(e,out.weather))return false;}
 if(G_JAR_MoverMotionActive(e)){out.hasMoverOffset=true;G_JAR_MoverPathOffset(e,out.moverOffset);}
 if(G_CrystalCratePortable(e)){out.hasCrystal=true;if(!G_CrystalCrateRead(e,out.crystal))return false;}
 if(G_DoorCameraPortable(e)){out.hasDoorCamera=true;out.doorExtension=e.random;if(!G_DoorCameraRead(e,out.doorCamera))return false;}
 if(G_SandboxSlidePortable(e)){out.hasSlidingDoor=true;if(!G_SandboxSlideRead(e,out.slidingDoor))return false;}
 if(G_ButtonPortable(e)){out.hasButton=true;out.buttonEnabled=!(e.svFlags&SVF_INACTIVE);if(!G_ButtonRead(e,out.button))return false;}
 if(G_BobbingPortable(e)){out.hasBobbing=true;if(!G_BobbingRead(e,out.bobbing))return false;}
 if(G_SandboxAngular(e)&&!G_SandboxAngularRead(e,out.angular))return false;
 if(G_MovementMarkerPortable(e)&&!G_MovementMarkerRead(e,out.movementMarker))return false;
 if(G_LogicKind(e)!=jar::logic::None&&!G_LogicRead(e,out.logicMarker))return false;
 if(G_ContactKind(e)){
  if(!G_ContactRead(e,out.contact))return false;
  out.contactSpent=G_ContactSpent(e);
 }
 const auto logicThink=G_LogicScheduledThink(out.logicMarker.kind);
 if(G_SandboxShooter(e)){
  if(!G_SandboxShooterRead(e,out.shooterSettings))return false;
  out.hasShooter=true;
  const auto control=nativeControls.find(e.s.number);
  if(out.shooterSettings.enabled&&out.shooterSettings.repeat)
   out.shooterPendingMs=G_SandboxShooterPending(e,control==nativeControls.end()?level.time:control->second.started);
 }
 const bool contactOutput=out.contact.kind==jar::contactTrigger::Repeat||
  (out.contact.kind==jar::contactTrigger::Once&&!out.contactSpent);
 if((contactOutput&&G_ContactPending(e))||(logicThink!=thinkF_NULL&&e.e_ThinkFunc==logicThink&&e.nextthink>0)){
  const auto control=nativeControls.find(e.s.number);
  const int clock=control==nativeControls.end()?level.time:control->second.started;
  out.pendingOutputMs=std::max(1,std::min(e.nextthink-clock,jar::logic::maxDelay));
 }
 if((out.logicMarker.kind==jar::logic::Relay||out.contact.kind==jar::contactTrigger::Repeat)&&
    e.painDebounceTime>level.time)
  out.reuseCooldownMs=std::min(e.painDebounceTime-level.time,jar::logic::maxDelay);
 if(G_PushKind(e)&&!G_PushRead(e,out.push))return false;
 if(G_TeleportKind(e)&&!G_TeleportRead(e,out.teleport))return false;
 if(owned(e.s.number)&&G_LogicCanSetUse(e)){
  out.hasUseState=true;out.useEnabled=G_LogicUseEnabled(e);
 }
 if(G_SandboxSecret(e))out.secretFlags=e.spawnflags&JAR_SECRET_FLAGS;
 if(G_SandboxReplicator(e)){
  const auto control=nativeControls.find(e.s.number);
  out.replicatorHeal=G_ReplicatorHeal(e);out.replicatorDelay=G_ReplicatorDelay(e);
  out.replicatorState=G_ReplicatorPhase(e,control==nativeControls.end()?level.time:control->second.started);
 }
 if(G_WelderPortable(e)){
  out.hasWelder=true;
  if(!G_WelderRead(e,out.welder))return false;
  out.welderCooldownMs=std::max(0,std::min(e.attackDebounceTime-level.time,jar::welder::maxCooldownMs));
 }
 if(G_SpotlightPortable(e)){out.hasSpot=true;if(!G_SpotlightRead(e,out.spot))return false;}
 if(G_PortalPortable(e)){out.hasPortal=true;if(!G_PortalRead(e,out.portal))return false;}
 if(G_LaserArmPortable(e)){
  out.hasLaser=true;
  if(!G_LaserArmRead(e,out.laser))return false;
  if(e.alt_fire&&e.attackDebounceTime>level.time){
   out.laserBurstMs=std::min(e.attackDebounceTime-level.time,jar::laserArm::maxBurstMs);
   out.laserSampleMs=std::max(1,std::min(e.nextthink-level.time,FRAMETIME));
  }
 }
 if(G_SandboxLaser(e)){out.hasLaserEmitter=true;if(!G_SandboxLaserRead(e,out.laserEmitter))return false;}
 if(G_SandboxStation(e)){
  if(!G_SandboxStationRead(e,out.station))return false;
  if(jar::sandboxStation::Recharging(out.station)&&e.painDebounceTime>0)
   out.stationRechargeMs=std::max(1,std::min(e.painDebounceTime-level.time,
      out.station.recharge+jar::sandboxStation::useSoundDuration));
  // Native Use rejects commands through setTime, including that millisecond.
  if(e.setTime>0&&e.setTime>=level.time)
   out.stationUseCooldownMs=std::min(e.setTime-level.time,jar::sandboxStation::maxUseCooldown)+1;
 }
 if(G_SandboxBreakable(e)&&!G_SandboxBreakableRead(e,out.breakable))return false;
 if(G_SandboxUsable(e)){out.hasUsable=true;if(!G_SandboxUsableRead(e,out.usable))return false;}
 if(G_SandboxSpeaker(e)){
  if(!G_SandboxSpeakerRead(e,out.speakerSettings))return false;
  out.hasSpeaker=true;
 }
 if(G_ReflectionPortable(e)){out.hasReflection=true;if(!G_ReflectionRead(e,out.reflection))return false;}
 if(G_CloudClass(e)){out.hasCloud=true;if(!G_CloudRead(e,out.cloud))return false;}
 if(G_ShipBoundaryClass(e)){out.shipBoundary=true;out.turnSeconds=e.count/1000.f;out.preserveEmpty=(e.spawnflags&1)!=0;}
 if(e.classname&&!strcmp(e.classname,"trigger_bidirectional")){out.directionalVolume=true;out.wait=e.wait;out.remaining=e.count;out.playerOnly=(e.spawnflags&1)!=0;if(e.NPC_targetname)out.npcFilter=e.NPC_targetname;}
 if(jar::volumeEntity::Portable(e)&&jar::volume::Valid(e.pos1,e.pos2)){out.orientedVolume=true;VectorCopy(e.pos1,out.volumeMins);VectorCopy(e.pos2,out.volumeMaxs);}
 return true;
}
void duplicateTool(gentity_t&e){
 const auto* support=G_SandboxChargeSupport(e);
 const auto* root=owned(support?support->s.number:e.s.number);
 const uint32_t rootKey=root?root->id:jar::constraint::NativeId(e.s.number);
 std::set<uint32_t> shields;
 const auto admitShield=[&](uint32_t key){
  if(!jar::constraint::Native(key))return;
  const auto slot=jar::constraint::NativeEntity(key);
  if(slot<ENTITYNUM_WORLD&&G_PortableShield(g_entities[slot]))shields.insert(key);
 };
 admitShield(rootKey);
 for(unsigned i=0;i<state.jointCount;++i){admitShield(state.joints[i].first);admitShield(state.joints[i].second);}
 jar::duplicate::Snapshot candidate;std::vector<uint32_t> members;std::vector<std::pair<uint32_t,uint32_t>> links;unsigned external=0;
 std::map<uint32_t,jar::bspSection::CopyState> sections;
 const auto captureSection=[&](const gentity_t& entity,uint32_t id){
  return !jar::bspSection::Portable(entity)||jar::bspSection::CaptureCopyState(entity,sections[id]);
 };
 std::map<std::string,uint32_t> targetNames;
 for(const auto&o:state.owned)if(matches(o)&&g_entities[o.entity].targetname)targetNames.emplace(g_entities[o.entity].targetname,o.id);
 for(unsigned i=0;i<state.ownedCount;++i){
  const auto&o=state.owned[i];if(!matches(o))continue;
  const auto&e=g_entities[o.entity];
  if(e.NPC){
   for(auto goal:{G_WorkshopReadNPCGoal(*e.NPC),G_WorkshopReadNPCPatrol(*e.NPC)})
    if(G_WorkshopResolveNPCGoal(goal))links.emplace_back(o.id,goal.owner);
  }
  if(jar::logic::Slotted(G_LogicKind(e))){
   for(int slot=0;slot<jar::logic::outputCount;++slot){
    const char* name=G_LogicOutput(e,slot);
    if(name&&targetNames.count(name))links.emplace_back(o.id,targetNames[name]);
   }
   continue;
  }
  const auto*target=jar::levelChange::Class(e.classname)?nullptr:e.target;
  if(target&&targetNames.count(target))links.emplace_back(o.id,targetNames[target]);
  if(G_DistancePortable(e)){
   for(int role=JAR_DISTANCE_OUTSIDE;role<JAR_DISTANCE_LINKS;++role){
    const char* name=G_DistanceLink(e,role);
    if(name&&targetNames.count(name))links.emplace_back(o.id,targetNames[name]);
   }
  }else if(G_WaypointPortable(e)||G_SceneCameraPortable(e)||G_SpotlightPortable(e)||G_SandboxSlidePortable(e)||G_ButtonPortable(e)||G_SandboxUsable(e)||
           G_LogicKind(e)==jar::logic::Counter||(e.classname&&!strcmp(e.classname,"trigger_bidirectional"))){
   const auto*back=!strcmp(e.classname,"trigger_bidirectional")?e.paintarget:e.target2;
   if(back&&targetNames.count(back))links.emplace_back(o.id,targetNames[back]);
  }
 }
 if(root||shields.count(rootKey)){
  if(!jar::duplicate::Members(state,rootKey,members,links,shields)){message("Assembly ownership is incomplete; copy unchanged.");return;}
  const auto resolve=[&](uint32_t id)->const gentity_t*{
   if(shields.count(id))return &g_entities[jar::constraint::NativeEntity(id)];
   const auto* entry=OwnedId(state,id);return entry&&matches(*entry)?&g_entities[entry->entity]:nullptr;
  };
  const auto* anchor=resolve(members[0]);if(!anchor){message("Copy target no longer exists.");return;}
  VectorCopy(anchor->currentOrigin,candidate.origin);
  for(auto id:members){const auto*o=OwnedId(state,id);jar::duplicate::Object node;
   if(o&&matches(*o)&&duplicateChargeChild(*o,g_entities[o->entity])){
    const auto& charge=g_entities[o->entity];
    const auto* mount=G_SandboxChargeSupport(charge);
    const auto* parent=OwnedId(state,o->parent);
    if(charge.e_ThinkFunc==thinkF_G_FreeEntity||(mount&&parent&&mount->s.number==parent->entity))continue;
    message("An assembly charge is detached from its support; copy unchanged.");return;
   }
   const auto* entity=resolve(id);
   if(!entity||!duplicateCapture(*entity,id,o?o->parent:0,anchor->currentOrigin,node)||
      !captureSection(*entity,id)){message("A member is occupied, expired or cannot be copied.");return;}
   const auto& member=*entity;
   const auto linkedId=[&](const char*name){const auto it=name?targetNames.find(name):targetNames.end();return it==targetNames.end()?0u:it->second;};
   const auto*target=member.target;
   node.target=linkedId(target);
   node.targetCaptured=!node.hasLevelChange&&(!target||node.target||node.distanceRange||node.push.kind||
      node.teleport.kind||node.contact.kind||G_LogicScheduledThink(node.logicMarker.kind)!=thinkF_NULL);
   if(node.hasLevelChange)node.target=0;
   if(jar::logic::Slotted(node.logicMarker.kind)){
    node.target=0;node.targetCaptured=false;
    for(int slot=0;slot<jar::logic::outputCount;++slot)
     node.slottedOutputs[slot]=linkedId(G_LogicOutput(member,slot));
   }
   if(node.distanceRange){
    node.backwardCaptured=true;node.backwardTarget=linkedId(member.target2);
    node.distanceReference=linkedId(member.ownername);node.distanceSubject=linkedId(member.NPC_target);
   }else if(node.directionalVolume||node.hasSpot||node.hasSceneCamera||node.hasWaypoint||node.hasSlidingDoor||node.hasButton||node.hasUsable||
            node.logicMarker.kind==jar::logic::Counter){
    const auto*back=node.directionalVolume?member.paintarget:member.target2;
    node.backwardTarget=linkedId(back);node.backwardCaptured=!back||node.backwardTarget;
   }
   candidate.objects.push_back(node);
  }
 }else {
  jar::duplicate::Object node;
  if(!duplicateCapture(e,rootKey,0,e.currentOrigin,node)||!captureSection(e,rootKey)){
   message("No supported spawn definition for this entity, or it is occupied/dead.");return;
  }
  VectorCopy(e.currentOrigin,candidate.origin);
  candidate.objects.push_back(node);
  if(!node.hasStandaloneCharge)members.push_back(rootKey);
 }
 for(unsigned i=0;i<state.jointCount;++i){
  const auto& j=state.joints[i];
  const bool first=std::find(members.begin(),members.end(),j.first)!=members.end();
  const bool second=std::find(members.begin(),members.end(),j.second)!=members.end();
  const bool fixed=(first&&j.second==jar::duplicate::worldAnchor)||(second&&j.first==jar::duplicate::worldAnchor);
  vec3_t normal;
  if(first&&second)candidate.joints.push_back(j);
  else if(fixed&&duplicateWorldAnchor(j,normal)){
   candidate.joints.push_back(j);
   candidate.worldNormals.emplace(j.id,std::array<float,3>{normal[0],normal[1],normal[2]});
  }
  else if(first||second)++external;
 }
 if(!jar::duplicate::Valid(candidate)){message("Assembly properties exceed supported native bounds; copy unchanged.");return;}
 size_t count=0;for(const auto& node:candidate.objects)count+=1+node.charges.size();
 duplicateSnapshot=std::move(candidate);
 duplicateSections=std::move(sections);
 gi.cvar_set("jar_duplicateStatus",va("Copied: %u objects / %u links",unsigned(count),unsigned(duplicateSnapshot.joints.size())));
 message(external?va("Copied selection. %u attachment(s) without a portable endpoint excluded.",external):"Copied properties and connected assembly. Left/Paste places it; Undo removes the complete copy.");
}
bool undoSpawnGroup(const std::vector<SpawnedEdit>&group){
 std::unordered_set<uint32_t> checked;
 for(const auto& member:group){
  const auto* entry=spawnGroupMember(member);
  if(entry&&!removalAvailable(*entry,&checked)){
   message("Move clear of the occupied vehicle, controller or section before undoing this assembly.");
   return false;
  }
 }
 for(auto i=group.rbegin();i!=group.rend();++i)if(spawnGroupMember(*i)){
  if(held==i->entity)releaseHeld();
  nativeRelease(i->entity,false);
  if(!remove(i->entity,true))return false;
 }
 message("Copied assembly removed.");
 return true;
}
bool duplicateScenerySolid(const jar::duplicate::Object& node){
 return !node.triggerVolume&&node.breakable.phase!=jar::sandboxBreakable::Broken&&(!node.hasUsable||node.usable.solid);
}

void duplicatePlacementBounds(const jar::duplicate::Object& node,vec3_t mins,vec3_t maxs){
 VectorCopy(node.mins,mins);VectorCopy(node.maxs,maxs);
 if(node.catalog==JAR_LIFT_ID&&node.hasSlidingDoor)mins[2]-=node.slidingDoor.extension/float(JAR_SLIDE_MILLI);
 if(node.hasBobbing){
  const float base=JAR_BobHeight(node.bobbing.amplitude,0,node.bobbing.phase,0);
  mins[2]=std::min(mins[2],base+node.mins[2]-node.bobbing.amplitude);
  maxs[2]=std::max(maxs[2],base+node.maxs[2]+node.bobbing.amplitude);
 }
}

float duplicateSurfaceOffset(const jar::duplicate::Snapshot& copy,const vec3_t normal,float fallback){
 bool physical=false;
 float offset=0;
 const auto consider=[&](float candidate){offset=physical?std::max(offset,candidate):candidate;physical=true;};
 for(const auto& node:copy.objects){
  const float shift=DotProduct(node.offset,normal);
  if(duplicateScenerySolid(node)){
   vec3_t mins,maxs;duplicatePlacementBounds(node,mins,maxs);
   JarSweepHull convex;
   const auto* definition=prop(node.catalog);
   const auto* shape=definition&&propSceneryHull(*definition,node.poseFrame,node.angles,convex)?&convex:nullptr;
   consider(G_JAR_WorkshopSurfaceOffset(mins,maxs,normal,shape)-shift);
  }
  for(const auto& charge:node.charges){
   vec3_t mount;jar::angular::RebaseVector(charge.origin,vec3_origin,node.angles,mount);
   const float half=jar::chargeCopy::HalfSize(charge.mode);
   const vec3_t mins={-half,-half,-half},maxs={half,half,half};
   consider(G_JAR_WorkshopSurfaceOffset(mins,maxs,normal,nullptr)-shift-DotProduct(mount,normal));
  }
 }
 return physical?offset:fallback;
}

const char* duplicatePositionError(const jar::duplicate::Snapshot& copy,const vec3_t anchor){
 for(const auto& node:copy.objects){
  vec3_t point;VectorAdd(anchor,node.offset,point);
  for(float value:point)if(!std::isfinite(value)||std::fabs(value)>65535)
   return "Copied assembly extends outside the world coordinate limits.";
  if(jar::duplicate::MapSection(node.catalog)&&!G_JAR_WorkshopSectionInWorld(point,node.mins,node.maxs))
   return "The copied map section extends outside the world limits.";
 }
 return nullptr;
}

bool duplicateSceneryPlacement(const jar::duplicate::Snapshot& copy,const vec3_t startAnchor,
 const vec3_t endAnchor,float& fraction){
 fraction=1;
 const auto clear=[&](const vec3_t offset,const vec3_t mins,const vec3_t maxs,const JarSweepHull* shape,bool grounded){
  vec3_t start,end;VectorAdd(startAnchor,offset,start);VectorAdd(endAnchor,offset,end);
  for(int i=0;i<3;++i)if(!(std::fabs(start[i])<=65535&&std::fabs(end[i])<=65535))return false;
  trace_t hit{};
  G_JAR_WorkshopSceneryTrace(hit,start,end,mins,maxs,ENTITYNUM_NONE,shape);
  if(hit.startsolid||hit.allsolid||(grounded&&hit.fraction<1&&hit.plane.normal[2]<.7f))return false;
  fraction=std::min(fraction,hit.fraction);
  return true;
 };
 for(const auto& node:copy.objects){
  if(duplicateScenerySolid(node)){
   JarSweepHull convex;
   const auto* definition=prop(node.catalog);
   const auto* shape=definition&&propSceneryHull(*definition,node.poseFrame,node.angles,convex)?&convex:nullptr;
   if(!clear(node.offset,node.mins,node.maxs,shape,actor(node.catalog)!=nullptr))return false;
  }
  for(const auto& charge:node.charges){
   vec3_t offset;jar::angular::RebaseVector(charge.origin,vec3_origin,node.angles,offset);VectorAdd(offset,node.offset,offset);
   const float half=jar::chargeCopy::HalfSize(charge.mode);
   const vec3_t mins={-half,-half,-half},maxs={half,half,half};
   if(!clear(offset,mins,maxs,nullptr,false))return false;
  }
 }
 return true;
}

bool duplicateRecoverPlacement(const jar::duplicate::Snapshot& copy,const vec3_t normal,vec3_t anchor){
 constexpr float searchDistance=64;
 vec3_t outer;VectorMA(anchor,searchDistance,normal,outer);
 float fraction;
 if(duplicatePositionError(copy,outer)||!duplicateSceneryPlacement(copy,outer,anchor,fraction)||fraction>=1)return false;
 // Every member sweeps the same distance. The first contact determines
 // a single translation for the assembly, including all mounted charges.
 vec3_t candidate;
 VectorMA(anchor,std::min(searchDistance,searchDistance*(1-fraction)+1),normal,candidate);
 if(duplicatePositionError(copy,candidate)||!duplicateSceneryPlacement(copy,candidate,candidate,fraction)||fraction<1)return false;
 VectorCopy(candidate,anchor);
 return true;
}

void duplicatePaste(){
 if(!sandboxReady())return;
 if(!jar::duplicate::Valid(duplicateSnapshot)){message("Copy an entity or assembly first.");return;}
 const auto&copy=duplicateSnapshot;
 size_t count=0;for(const auto& node:copy.objects)count+=1+node.charges.size();
 if(count>ownedLimit-state.ownedCount||copy.joints.size()>jar::constraint::limit-state.jointCount||
  count>jar::constraint::nativeBit-state.nextOwned||copy.joints.size()>UINT32_MAX-state.nextJoint){message("Not enough object/link capacity for the complete assembly.");return;}
 vec3_t mins,maxs;jar::duplicate::Bounds(copy,mins,maxs);
 for(const auto& node:copy.objects)if(node.hasSlidingDoor||node.hasBobbing){
  vec3_t low,high;duplicatePlacementBounds(node,low,high);
  mins[2]=std::min(mins[2],node.offset[2]+low[2]);
  maxs[2]=std::max(maxs[2],node.offset[2]+high[2]);
 }
 vec3_t end;VectorMA(cg.refdef.vieworg,4096,cg.refdef.viewaxis[0],end);trace_t aim{};
 gi.trace(&aim,cg.refdef.vieworg,nullptr,nullptr,end,0,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
 const bool hasActors=std::any_of(copy.objects.begin(),copy.objects.end(),[](const auto& node){return actor(node.catalog)!=nullptr;});
 if((aim.startsolid||aim.allsolid||aim.plane.normal[2]<.7f)&&hasActors)
  G_JAR_WorkshopSceneryTrace(aim,cg.refdef.vieworg,end,nullptr,nullptr,0);
 if(aim.startsolid||aim.allsolid||aim.fraction>=1){message("Aim at a surface within 4096 units to paste.");return;}
 const bool standaloneCharge=copy.objects.front().hasStandaloneCharge;
 if(standaloneCharge&&(aim.entityNum!=ENTITYNUM_WORLD||aim.surfaceFlags&SURF_NOIMPACT||
    !(std::fabs(VectorLengthSquared(aim.plane.normal)-1.f)<=.01f))){
  message("Aim at a solid map surface to paste a planted charge.");return;
 }
 const bool mapSection=std::any_of(copy.objects.begin(),copy.objects.end(),[](const auto& node){return jar::duplicate::MapSection(node.catalog);});
 const float enclosingOffset=G_JAR_WorkshopSurfaceOffset(mins,maxs,aim.plane.normal,nullptr);
 const float offset=mapSection?enclosingOffset:duplicateSurfaceOffset(copy,aim.plane.normal,enclosingOffset);
 vec3_t anchor;VectorMA(aim.endpos,offset,aim.plane.normal,anchor);
 if(mapSection){
  vec3_t center;for(int a=0;a<3;++a)center[a]=(mins[a]+maxs[a])*.5f;
  VectorMA(anchor,DotProduct(center,aim.plane.normal),aim.plane.normal,anchor);VectorSubtract(anchor,center,anchor);
 }
 // Translate fixed world frames by the same offset as their copied members.
 const auto world=jar::duplicate::worldAnchor;
 auto joints=copy.joints;
 for(const auto& joint:joints){
  if(joint.first!=world&&joint.second!=world)continue;
  const auto& frame=joint.first==world?joint.a:joint.b;
  if(DotProduct(copy.worldNormals.at(joint.id).data(),aim.plane.normal)<.98f)continue;
  vec3_t projected;VectorSubtract(frame.position,copy.origin,projected);VectorAdd(anchor,projected,projected);
  VectorSubtract(aim.endpos,projected,projected);
  VectorMA(anchor,DotProduct(projected,aim.plane.normal),aim.plane.normal,anchor);
  break;
 }
 vec3_t translation;
 VectorSubtract(anchor,copy.origin,translation);
 for(auto& joint:joints){
  if(joint.first!=world&&joint.second!=world)continue;
  auto& frame=joint.first==world?joint.a:joint.b;
  VectorAdd(frame.position,translation,frame.position);
  if(!jar::constraint::Valid(frame)){message("A copied world anchor is outside the world bounds. Nothing pasted.");return;}
  vec3_t normal;
  if(!JAR_WorldAnchorSurface(frame.position,copy.worldNormals.at(joint.id).data(),G_JAR_WorkshopAnchorTrace,normal)){
   message("A copied world anchor needs matching scenery. Nothing pasted.");return;
  }
 }
 if(const char* outside=duplicatePositionError(copy,anchor)){message(outside);return;}
 if(hasActors&&aim.plane.normal[2]<.7f){message("Copied actors/vehicles need a level surface.");return;}
 float fraction;
 if((!duplicateSceneryPlacement(copy,anchor,anchor,fraction)||fraction<1)&&
    (standaloneCharge||mapSection||!copy.worldNormals.empty()||!duplicateRecoverPlacement(copy,aim.plane.normal,anchor))){
  message("Not enough clear space for the complete assembly. Nothing pasted.");return;
 }
 const auto first=state.nextOwned,previousSelected=selected;
 const auto selectionLifetime=previousSelected<ENTITYNUM_WORLD?G_EntityLifetime(&g_entities[previousSelected]):0;
 const auto newMembers=[&](){
  std::vector<SpawnedEdit> members;
  for(const auto& entry:state.owned)if(entry.id>=first)
   members.push_back({entry.entity,entityGeneration[entry.entity],entry.id});
  return members;
 };
 auto rollback=[&](){
  JarActorRecoverySuspension recovery;
  const auto members=newMembers();
  for(auto it=members.rbegin();it!=members.rend();++it){
   const auto* entry=owned(it->entity);
   if(!entry||entry->id!=it->owner||entityGeneration[it->entity]!=it->generation)continue;
   if(matches(*entry))nativeRelease(it->entity,false);
   entry=owned(it->entity);
   if(entry&&entry->id==it->owner&&entityGeneration[it->entity]==it->generation)
    discardOwned(*entry);
  }
  selected=selectionLifetime&&G_EntityLifetime(&g_entities[previousSelected])==selectionLifetime?previousSelected:noEntity;
  Edit remaining{};
  remaining.group=newMembers();
  const bool complete=remaining.group.empty();
  if(!complete)rememberAction(std::move(remaining));
  list();
  message(complete?"Assembly paste failed and was rolled back. Originals and clipboard preserved.":
   "A native controller retained part of the failed paste. Undo can retry cleanup.");
 };
 std::map<uint32_t,uint32_t> translated;
 std::unordered_set<uint32_t> assigned;
 for(const auto&node:copy.objects){
  vec3_t pos;VectorAdd(anchor,node.offset,pos);gentity_t*e=nullptr;
  // Portable parents already create their owned helper(s). Reuse matching fresh
  // helpers rather than creating a second train corner, target or security key.
  if(node.parent){
   const auto parent=translated.find(node.parent);
   if(parent==translated.end()){rollback();return;}
   if(const auto* children=OwnedChildren(state,parent->second)){
    for(const auto id:*children){
     const auto* entry=OwnedId(state,id);
     if(id<first||!entry||assigned.count(id)||entry->catalog!=node.catalog||!matches(*entry))continue;
     e=&g_entities[entry->entity];
     break;
    }
   }
  }
  if(node.hasStandaloneCharge){
   auto& player=g_entities[0];
   if(player.inuse&&player.client)e=duplicateStandaloneCharge(node.standaloneCharge,player,pos,aim.plane.normal);
   if(e&&!Own(state,e->s.number,consoleCatalog)){G_FreeEntity(e);e=nullptr;}
   if(e)spawnCountDirty=true;
  }else if(node.hasDeployedShield){
   e=G_PortableShieldSpawnCopy(node.deployedShield,pos,node.angles);
   if(e&&!Own(state,e->s.number,consoleCatalog)){G_FreeEntity(e);e=nullptr;}
   if(e){e->targetname=G_NewString(va("jar_sb_%u",owned(e->s.number)->id));spawnCountDirty=true;}
  }else if(!e)e=spawn(node.catalog,pos,node.angles,node.poseFrame,node.wallVisibility==0,node.hasPortal,WorkshopSpawnFeedback::Assembly);
  if(!e||!e->inuse||!owned(e->s.number)){rollback();return;}
  auto*o=owned(e->s.number);
  if(node.parent){
   if(!SetOwnershipParent(state,*o,translated.at(node.parent))){rollback();return;}
   spawnCountDirty=true;
  }
  translated[node.source]=o->id;assigned.insert(o->id);
  if(node.parent&&rackStockChild(*e)){
   if(node.rackShelfStock){
    e->flags|=FL_FORCE_PULLABLE_ONLY;
    e->s.groundEntityNum=ENTITYNUM_NONE;
    if(e->e_ThinkFunc==thinkF_G_FreeEntity){e->e_ThinkFunc=thinkF_NULL;e->nextthink=0;}
   }else rackStockLoose(*e);
  }
  if(node.hasStandaloneCharge)continue;
  if(node.hasActorStatus||node.hasActorMovement||node.hasActorOrder){
   if(!actor(node.catalog)||!e->NPC||!e->client||e->m_pVehicle){rollback();return;}
   if(node.hasActorStatus)writeWorkshopActorStatus(*e,node.actorStatus);
   if(node.hasActorOrder)G_WorkshopWriteNPCOrder(*e->NPC,node.actorOrder);
   if(node.hasActorMovement&&!G_MovementActorSet(*e,node.actorMovement)){rollback();return;}
  }
  if(!prop(o->catalog))nativeBegin(*e,true);
  G_SetOrigin(e,pos);G_SetAngles(e,node.angles);
  if(e->client){VectorCopy(pos,e->client->ps.origin);SetClientViewAngle(e,e->currentAngles);}
  if(node.hasActorLoadout&&(!actor(node.catalog)||!G_WorkshopApplyActorLoadout(*e,node.actorLoadout))){rollback();return;}
  if(node.hasVehicleStatus&&!G_WorkshopApplyVehicleStatus(*e,node.vehicleStatus)){rollback();return;}
  if(node.hasHealth){
   if(!e->takedamage){rollback();return;}
   if(G_SandboxBreakable(*e)){
    jar::sandboxBreakable::Settings settings;
    if(!G_SandboxBreakableRead(*e,settings)){rollback();return;}
    settings.health=std::min(node.health,jar::sandboxBreakable::maxHealth);
    settings.capacity=std::max(settings.capacity,settings.health);
    if(!G_SandboxBreakableSet(*e,settings)){rollback();return;}
   }else if(G_SandboxUsable(*e)){
    jar::sandboxUsable::Settings settings;
    if(!G_SandboxUsableRead(*e,settings)){rollback();return;}
    settings.health=std::min(node.health,jar::sandboxUsable::maxHealth);
    settings.capacity=std::max(settings.capacity,settings.health);
    if(!G_SandboxUsableSet(*e,settings)){rollback();return;}
   }else{
    e->health=node.health;
    if(e->client)e->client->ps.stats[STAT_HEALTH]=e->health;
   }
  }
  if(node.hasCount&&e->item)e->count=node.count;
  if(node.hasLabel){if(!editableLabel(*e)){rollback();return;}if(e->message&&gi.bIsFromZone(e->message,TAG_G_ALLOC))gi.Free(e->message);e->message=G_NewString(node.label.c_str());}
  if(node.alwaysDelay){
   if(!G_AlwaysPortable(*e)){rollback();return;}
   e->delay=node.alwaysDelay;e->svFlags=node.alwaysInactive?e->svFlags|SVF_INACTIVE:e->svFlags&~SVF_INACTIVE;
   G_AlwaysSchedule(*e,node.alwaysRemaining);
  }
  if(node.hasVisible){
   if(!G_VisiblePortable(*e)){rollback();return;}
   G_VisibleSet(*e,node.visible);
   if(node.visibleInactive)e->svFlags|=SVF_INACTIVE;else e->svFlags&=~SVF_INACTIVE;
   G_VisibleArm(*e,node.visibleArmed);
  }
  if(node.distanceRange){
   if(!G_DistancePortable(*e)){rollback();return;}
   e->count=node.distanceRange;e->spawnflags=node.distanceSubjects;
   if(node.distanceInactive)e->svFlags|=SVF_INACTIVE;else e->svFlags&=~SVF_INACTIVE;
  }
  if(node.wallVisibility>=0){
   if(!G_WallSetVisible(*e,node.wallVisibility!=0)){rollback();return;}
   if(node.wallInactive)e->svFlags|=SVF_INACTIVE;else e->svFlags&=~SVF_INACTIVE;
  }
  if(node.hasSky&&!G_SkyPortalSet(*e,node.sky)){rollback();return;}
  if(node.hasCombatPoint&&!G_CombatPointSet(*e,node.combat)){rollback();return;}
  if(node.hasReference&&!G_ReferenceCopy(*e,node.reference)){rollback();return;}
  if(node.hasLevelChange&&!G_LevelChangeSet(*e,node.levelChange)){rollback();return;}
  if(node.hasMusic&&!G_MusicSetTrack(*e,node.music.c_str())){rollback();return;}
  if(node.hasScript&&!G_ScriptRunnerSet(*e,node.script)){rollback();return;}
  if(node.hasParms)for(unsigned i=0;i<jar::parms::count;++i)if(!jar::entityParms::Literal(*e,i,node.parms[i])){rollback();return;}
  if(node.nativeMarker){if(!e->classname||(strcmp(e->classname,"target_interest")&&strcmp(e->classname,"target_location")&&!G_JAR_PortableStart(*e))){rollback();return;}e->svFlags=node.markerInactive?e->svFlags|SVF_INACTIVE:e->svFlags&~SVF_INACTIVE;}
  if(node.triggerVolume){
   if(!jar::volumeEntity::Class(e->classname)){rollback();return;}
   if(!jar::volumeEntity::LocationClass(e->classname))e->contents=node.volumeEnabled?CONTENTS_TRIGGER:0;
   e->svFlags=node.volumeInactive?e->svFlags|SVF_INACTIVE:e->svFlags&~SVF_INACTIVE;
   VectorCopy(node.orientedVolume?node.volumeMins:node.mins,e->pos1);VectorCopy(node.orientedVolume?node.volumeMaxs:node.maxs,e->pos2);
   if(!jar::volumeEntity::Refresh(*e)){rollback();return;}
  }
  if(node.hasWeather&&!G_WeatherSet(*e,node.weather)){rollback();return;}
  if(node.hasMoverOffset&&!G_JAR_MoverSetOffset(*e,node.moverOffset)){rollback();return;}
  if(node.hasCrystal&&!G_CrystalCrateSet(*e,node.crystal)){rollback();return;}
  if(node.hasDoorCamera){if(!G_DoorCameraSet(*e,node.doorCamera)){rollback();return;}e->random=node.doorExtension;G_DoorCameraMoved(*e);}
  if(node.hasSlidingDoor&&!G_SandboxSlideRestore(*e,node.slidingDoor)){rollback();return;}
  if(node.hasButton){if(!G_ButtonRestore(*e,node.button)){rollback();return;}if(!node.buttonEnabled)e->svFlags|=SVF_INACTIVE;}
  if(node.hasBobbing&&!G_BobbingRestore(*e,node.bobbing)){rollback();return;}
  if(node.movementMarker.kind!=jar::movement::None&&!G_MovementMarkerSet(*e,node.movementMarker)){rollback();return;}
  if(node.logicMarker.kind!=jar::logic::None&&!G_LogicSet(*e,node.logicMarker)){rollback();return;}
  if(node.hasShooter&&!G_SandboxShooterSet(*e,node.shooterSettings)){rollback();return;}
  if(node.secretFlags!=-1){
   if(!G_SandboxSecret(*e)||(node.secretFlags&~JAR_SECRET_FLAGS)){rollback();return;}
   e->spawnflags=(e->spawnflags&~JAR_SECRET_FLAGS)|node.secretFlags;
  }
  if(node.contact.kind!=jar::contactTrigger::None){
   if(!G_ContactSettings(*e,node.contact)){rollback();return;}
   G_ContactSetSpent(*e,node.contactSpent);
  }
  if(node.push.kind!=jar::push::None&&!G_PushSet(*e,node.push)){rollback();return;}
  if(node.teleport.kind!=jar::teleport::None&&!G_TeleportSet(*e,node.teleport)){rollback();return;}
  if(node.hasWelder){
   if(!G_WelderSet(*e,node.welder)){rollback();return;}
   e->attackDebounceTime=level.time+node.welderCooldownMs;
  }
  if(node.hasSpot&&!G_SpotlightSet(*e,node.spot)){rollback();return;}
  if(node.hasRail&&!G_RailSet(*e,node.rail)){rollback();return;}
  if(node.hasWaypoint&&!G_WaypointSet(*e,node.waypoint)){rollback();return;}
  if(node.hasSceneCamera&&!G_SceneCameraSet(*e,node.sceneCamera)){rollback();return;}
  if(node.hasPortal&&!G_PortalSet(*e,node.portal)){rollback();return;}
  if(node.hasLaser&&!G_LaserArmSet(*e,node.laser)){rollback();return;}
  if(node.hasLaserEmitter&&!G_SandboxLaserSet(*e,node.laserEmitter)){rollback();return;}
  if(node.station.capacity){
   if(!G_SandboxStationSet(*e,node.station)){rollback();return;}
   if(node.stationRechargeMs)e->painDebounceTime=level.time+node.stationRechargeMs;
   e->setTime=node.stationUseCooldownMs?level.time+node.stationUseCooldownMs-1:0;
   G_SandboxStationSchedule(*e);
  }
  if(node.breakable.capacity&&!G_SandboxBreakableSet(*e,node.breakable)){rollback();return;}
  if(node.hasUsable&&!G_SandboxUsableSet(*e,node.usable)){rollback();return;}
  if(node.hasSpeaker&&!G_SandboxSpeakerSet(*e,node.speakerSettings)){rollback();return;}
  if(node.angular.kind&&!G_SandboxAngularSet(*e,node.angular)){rollback();return;}
  if(node.hasReflection&&!G_ReflectionSet(*e,node.reflection)){rollback();return;}
  if(node.hasCloud&&!G_CloudSet(*e,node.cloud)){rollback();return;}
  if(node.shipBoundary&&!G_ShipBoundarySet(*e,node.turnSeconds,node.preserveEmpty)){rollback();return;}
  if(node.directionalVolume){
   if(strcmp(e->classname,"trigger_bidirectional")){rollback();return;}
   e->wait=node.wait;e->count=node.remaining;e->spawnflags=node.playerOnly?e->spawnflags|1:e->spawnflags&~1;
   if(e->NPC_targetname&&gi.bIsFromZone(e->NPC_targetname,TAG_G_ALLOC))gi.Free(e->NPC_targetname);
   e->NPC_targetname=node.npcFilter.empty()?nullptr:G_NewString(node.npcFilter.c_str());
  }
  if(node.hasUseState&&G_LogicCanSetUse(*e))G_LogicSetUseEnabled(*e,node.useEnabled);
  if(prop(o->catalog)){o->physics=node.propPhysics;o->frozen=node.frozen?1:0;VectorCopy(node.mins,e->mins);VectorCopy(node.maxs,e->maxs);physicsInvalidate(e->s.number);}
  else if(!node.frozen)nativeRelease(e->s.number,false);
  if(!node.hasDeployedShield)setupMoved(*e);
  // Keep provisional members outside native collision queries until every
  // member is ready. Valid touching/overlapping source assemblies stay intact.
  for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].id>=first)gi.unlinkentity(&g_entities[state.owned[i].entity]);
 }
 // A native parent may regenerate children that were consumed/deleted before
 // copying (rack pickups, camera base). Keep only captured members.
 for(int i=int(state.ownedCount)-1;i>=0;--i)if(unsigned(i)<state.ownedCount){const auto&o=state.owned[i];
  if(o.id>=first&&std::find(assigned.begin(),assigned.end(),o.id)==assigned.end()){
   const auto entity=o.entity;nativeRelease(entity,false);remove(entity,true);
  }
 }
 for(const auto& section:duplicateSections){
  const auto target=translated.find(section.first);
  const auto* object=target==translated.end()?nullptr:OwnedId(state,target->second);
  if(!object||!jar::bspSection::RestoreCopyState(g_entities[object->entity],section.second)){
   rollback();return;
  }
 }
 // Validate actual native bounds, including regenerated helpers, against the
 // existing world. Do not reject legitimate contact between copied members.
 for(unsigned i=0;i<state.ownedCount;++i){const auto&o=state.owned[i];if(o.id<first)continue;auto&e=g_entities[o.entity];trace_t tr;
  if(jar::volumeEntity::Portable(e)||G_SandboxBreakableBroken(e)||G_SandboxUsableDormant(e))continue;
  // The enclosure was checked before spawning the section's own brush children.
  if(jar::bspSection::Portable(e))continue;
  JarSweepHull convex;
  const auto* definition=prop(o.catalog);
  const auto* shape=definition&&propSceneryHull(*definition,e.s.frame,e.currentAngles,convex)?&convex:nullptr;
  G_JAR_WorkshopSceneryTrace(tr,e.currentOrigin,e.currentOrigin,e.mins,e.maxs,e.s.number,shape);
  if(tr.startsolid||tr.allsolid||(G_PortableShield(e)&&!G_PortableShieldClear(e))){rollback();return;}
 }
 for(auto j:joints){
  if(j.first!=world)j.first=translated[j.first];
  if(j.second!=world)j.second=translated[j.second];
  j.path={}; // The pasted assembly must route around scenery at its new position.
  if(!ropeAdmission(j)||!AddJoint(state,j)){rollback();return;}
 }
 // Rewire only to new members. Never retain pointers/names into the original
 // assembly or inherit authored mission target chains.
 for(const auto& node:copy.objects)if(node.actorGoal||node.actorPatrolGoal){
  const auto* source=OwnedId(state,translated.at(node.source));
  if(!source||!g_entities[source->entity].NPC){rollback();return;}
  const auto resolve=[&](uint32_t id)->jar::npcOrder::Destination{
   const auto* target=id?OwnedId(state,translated.at(id)):nullptr;
   return target?jar::npcOrder::Destination{target->id,target->entity}:jar::npcOrder::Destination{};
  };
  const auto goal=resolve(node.actorGoal),cursor=resolve(node.actorPatrolGoal);
  if((node.actorGoal&&!G_WorkshopResolveNPCGoal(goal))||
     (node.actorPatrolGoal&&!G_WorkshopResolveNPCGoal(cursor))){rollback();return;}
  G_WorkshopWriteNPCGoal(*g_entities[source->entity].NPC,goal);
  G_WorkshopWriteNPCPatrol(*g_entities[source->entity].NPC,cursor);
 }
 for(const auto&node:copy.objects)if(node.targetCaptured){const auto*o=OwnedId(state,translated[node.source]);auto&e=g_entities[o->entity];
  if(e.target&&gi.bIsFromZone(e.target,TAG_G_ALLOC))gi.Free(e.target);e.target=nullptr;e.enemy=nullptr;
  if(node.target){const auto*target=OwnedId(state,translated[node.target]);e.target=G_NewString(g_entities[target->entity].targetname);}
  if(!node.hasDeployedShield)setupMoved(e);
 }
 for(const auto&node:copy.objects)if((node.directionalVolume||node.hasSpot||node.hasSceneCamera||node.hasWaypoint||
  node.distanceRange||node.hasSlidingDoor||node.hasButton||node.hasUsable||node.logicMarker.kind==jar::logic::Counter)&&node.backwardCaptured){
  const auto*o=OwnedId(state,translated[node.source]);auto&e=g_entities[o->entity];
  char*&secondary=node.directionalVolume?e.paintarget:e.target2;if(secondary&&gi.bIsFromZone(secondary,TAG_G_ALLOC))gi.Free(secondary);secondary=nullptr;
  if(node.backwardTarget){const auto*target=OwnedId(state,translated[node.backwardTarget]);secondary=G_NewString(g_entities[target->entity].targetname);}
  if(node.distanceRange){
   const auto copyName=[&](uint32_t id){
    const auto* endpoint=id?OwnedId(state,translated[id]):nullptr;
    return endpoint?g_entities[endpoint->entity].targetname:nullptr;
   };
   G_DistanceSetLink(e,JAR_DISTANCE_REFERENCE,node.distanceReference?copyName(node.distanceReference):node.distanceReferenceRequired?JAR_DISTANCE_MISSING_REFERENCE_NAME:nullptr);
   G_DistanceSetLink(e,JAR_DISTANCE_SUBJECT,copyName(node.distanceSubject));
  }
 }
 for(const auto& node:copy.objects)if(jar::logic::Slotted(node.logicMarker.kind)){
  const auto* source=OwnedId(state,translated[node.source]);
  auto& entity=g_entities[source->entity];
  for(int slot=0;slot<jar::logic::outputCount;++slot){
   const auto id=node.slottedOutputs[slot];
   const auto* target=id?OwnedId(state,translated[id]):nullptr;
   setSlottedLink(entity,slot,target?g_entities[target->entity].targetname:nullptr);
  }
 }
 // Restore queued logic after all links and attachments are in place.
 for(const auto& node:copy.objects)if(node.pendingOutputMs||node.reuseCooldownMs){
  const auto* object=OwnedId(state,translated[node.source]);
  auto& entity=g_entities[object->entity];
  if(node.reuseCooldownMs)entity.painDebounceTime=level.time+node.reuseCooldownMs;
  if(node.pendingOutputMs){
   const auto control=nativeControls.find(entity.s.number);
   const int clock=control==nativeControls.end()?level.time:control->second.started;
   entity.e_ThinkFunc=node.contact.kind?thinkF_multi_trigger_run:G_LogicScheduledThink(node.logicMarker.kind);
   entity.nextthink=clock+node.pendingOutputMs;
   entity.activator=&entity;
  }
 }
 for(const auto& node:copy.objects)if(node.hasShooter){
  const auto* object=OwnedId(state,translated[node.source]);
  const auto control=nativeControls.find(object->entity);
  G_SandboxShooterSchedule(g_entities[object->entity],control==nativeControls.end()?level.time:control->second.started,node.shooterPendingMs);
 }
 for(const auto& node:copy.objects)if(node.laserBurstMs){
  const auto* object=OwnedId(state,translated.at(node.source));
  if(!object||!G_LaserArmResumeBurst(g_entities[object->entity],node.laserBurstMs,node.laserSampleMs)){
   rollback();return;
  }
 }
 for(const auto& node:copy.objects)if(node.pickupRemainingMs>=0){
  const auto* object=OwnedId(state,translated[node.source]);
  auto& item=g_entities[object->entity];
  if(item.nextthink==givePickup::reservedDeadline){
   if(node.pickupRemainingMs==0){pickupLifetimeSet(item,0);item.setTime=0;}
   else item.setTime=node.pickupRemainingMs;
   continue;
  }
  if(!pickupLifetimeEditable(item)){rollback();return;}
  pickupLifetimeSet(item,node.pickupRemainingMs);
 }
 for(const auto& node:copy.objects)if(node.replicatorHeal!=-1){
  const auto* object=OwnedId(state,translated[node.source]);
  auto& entity=g_entities[object->entity];
  if(!G_SandboxReplicator(entity)){rollback();return;}
  entity.damage=node.replicatorHeal;entity.wait=node.replicatorDelay;
  entity.delay=node.replicatorDelay;
  G_ReplicatorModels(entity);
  const auto control=nativeControls.find(object->entity);
  G_ReplicatorApplyPhase(entity,node.replicatorState,control==nativeControls.end()?level.time:control->second.started);
 }
 std::vector<gentity_t*> pastedCharges;
 bool hasRemotePack=false;
 // setupMoved may relink members while rebuilding their outputs.
 if(count>copy.objects.size())for(const auto& entry:state.owned)
  if(entry.id>=first)gi.unlinkentity(&g_entities[entry.entity]);
 for(const auto& node:copy.objects)if(!node.charges.empty()){
  auto& player=g_entities[0];
  if(!player.inuse||!player.client){rollback();return;}
  const auto parent=translated.at(node.source);
  auto& support=g_entities[OwnedId(state,parent)->entity];
  for(const auto& saved:node.charges){
   auto* charge=duplicateMountedCharge(saved,support,player);
   if(!charge){rollback();return;}
   if(!Own(state,charge->s.number,consoleCatalog,parent)){G_FreeEntity(charge);rollback();return;}
   pastedCharges.push_back(charge);
   hasRemotePack|=saved.mode==jar::chargeCopy::Mode::DetPack;
  }
 }
 if(standaloneCharge){
  auto* entry=OwnedId(state,translated.at(copy.objects.front().source));
  pastedCharges.push_back(&g_entities[entry->entity]);
  hasRemotePack=copy.objects.front().standaloneCharge.mode==jar::chargeCopy::Mode::DetPack;
 }
 G_PortalFrame();
 Edit group{};
 group.group=newMembers();
 const auto pasted=group.group.size();
 if(!pasted){rollback();return;}
 for(const auto& member:group.group)gi.linkentity(&g_entities[member.entity]);
 for(auto* charge:pastedCharges)G_SandboxChargeRefreshBeam(*charge);
 if(hasRemotePack)g_entities[0].client->ps.eFlags|=EF_PLANTED_CHARGE;
 rememberAction(std::move(group));
 const auto*root=OwnedId(state,translated[copy.objects[0].source]);selected=root?root->entity:noEntity;
 list();message(va("Pasted %u objects with %u links. Undo removes this complete copy.",unsigned(pasted),unsigned(copy.joints.size())));
}
