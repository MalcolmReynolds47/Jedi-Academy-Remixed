// Entity editor transactions. Included after the native/tool implementations.
// All UI requests carry a selection token; entity-slot reuse invalidates it.
#include <climits>
uint32_t contextEntity=noEntity,contextGeneration=0,contextToken=0,contextSequence=0,contextRopeId=0;
bool contextMountedVehicle=false;
jar::duplicate::ActorStatus contextPublishedActorStatus;
bool contextPublishedActor=false;
WorkshopActorFire contextPublishedActorFire;
bool contextPublishedFire=false;
jar::npcOrder::Settings contextPublishedOrder;
jar::npcOrder::Destination contextPublishedNPCGoal;
bool contextPublishedOrderAllowed=false;
int contextPublishedHealth=0;
void contextNPCGoalStatus(jar::npcOrder::Destination goal,const gNPC_t* npc=nullptr){
 const auto* marker=G_WorkshopResolveNPCGoal(goal);
 gi.cvar_set("jar_contextNPCGoalLinked",goal.owner?"1":"0");
 gi.cvar_set("jar_contextNPCGoalStatus",!goal.owner?"No goal linked":!marker?"Goal missing":
  marker->svFlags&SVF_INACTIVE?"Goal disabled":
  marker->message?va("Goal: %s",marker->message):va("Goal: waypoint %d",marker->s.number));
 const auto cursor=npc?G_WorkshopReadNPCPatrol(*npc):jar::npcOrder::Destination{};
 const auto* point=cursor.owner?G_WorkshopResolveNPCGoal(cursor):marker;
 gi.cvar_set("jar_contextNPCPatrolStatus",!npc||G_WorkshopReadNPCOrder(*npc).mode!=jar::npcOrder::Patrol?"Ready to start":
  !point?"Current waypoint missing":point->svFlags&SVF_INACTIVE?"Current waypoint disabled":va("Current waypoint: %d",point->s.number));
}
int contextPublishedActorWeapon=WP_NONE;
uint32_t contextPublishedActorWeapons=0;
void contextClear(){contextEntity=noEntity;contextToken=0;contextRopeId=0;contextMountedVehicle=false;gi.cvar_set("jar_contextMountedVehicle","0");gi.cvar_set("jar_contextToken","0");gi.cvar_set("jar_contextLabel","Click an entity in the world");}
bool contextValid(){return contextEntity<ENTITYNUM_WORLD && editable(g_entities[contextEntity]) && contextGeneration==entityGeneration[contextEntity];}
gentity_t* contextPlayerVehicle(){
 auto* player=workshopPlayer();
 if(!player||!player->s.m_iVehicleNum||in_camera||cg.overrides.active||player->client->ps.viewEntity)return nullptr;
 auto* hull=G_TransportVehicleHull(player);
 jar::vehicleStatus::Values values;
 if(!hull||hull->m_pVehicle->m_pPilot!=player||!contextVehicleResources(*hull,values))return nullptr;
 return hull;
}
gentity_t* attachmentEntity(uint32_t);
void contextRopePublish(const gentity_t& e,bool advance){
 const auto key=attachmentKey(e);
 const jar::constraint::Joint* ropes[jar::constraint::limit];
 unsigned count=0,index=0;
 for(unsigned i=0;i<state.jointCount;++i){const auto& joint=state.joints[i];
  if(joint.kind!=jar::constraint::Rope||(joint.first!=key&&joint.second!=key))continue;
  if(joint.id==contextRopeId)index=count+(advance?1:0);
  ropes[count++]=&joint;
 }
 if(count)index%=count;
 const auto* rope=count?ropes[index]:nullptr;
 contextRopeId=rope?rope->id:0;
 gi.cvar_set("jar_contextRopeId",va("%u",contextRopeId));
 gi.cvar_set("jar_contextRopeCount",va("%u",count));
 gi.cvar_set("jar_contextRopeLength",rope?va("%.6f",rope->length):"");
 gi.cvar_set("jar_contextRopeLabel",rope?va("Rope %u of %u (#%u)",index+1,count,rope->id):"");
 const auto* other=rope?attachmentEntity(rope->first==key?rope->second:rope->first):nullptr;
 gi.cvar_set("jar_contextRopeEnd",!other?"":other->s.number==ENTITYNUM_WORLD?"Other end: World":
  va("Other end: %d %.35s",other->s.number,other->classname?other->classname:"entity"));
}
void contextShieldStatus(const gentity_t& shield){
 jar::shieldCopy::Properties values;
 const bool live=G_PortableShieldProperties(shield,values);
 gi.cvar_set("jar_contextDeployedShieldAllowed",live?"1":"0");
 gi.cvar_set("jar_contextDeployedShieldStatus",live?va("%s | %d health | %.1f s left",shield.count?"Raised":"Lowered",values.health,(values.deadline-level.time)*.001f):"");
}
bool contextChargeStatus(const gentity_t& charge,bool publishing=false){
 using namespace jar::chargeCopy;
 Properties values;
 const bool available=contextChargeProperties(charge,values,G_SandboxChargeSupport(charge));
 const bool proximity=available&&values.mode==Mode::Proximity;
 const bool pending=available&&Detonating(values.mode);
 const bool editable=available&&!pending&&!G_JAR_WorkshopControls(&charge)&&charge.takedamage&&values.health>0&&
  (!proximity||values.expiry>level.time);
 gi.cvar_set("jar_contextMountedChargeAllowed",available?"1":"0");
 gi.cvar_set("jar_contextMountedChargeEditable",editable?"1":"0");
 gi.cvar_set("jar_contextMountedChargeProximity",proximity?"1":"0");
 const char* status="";
 if(pending)status="Detonation pending";
 else if(proximity)status=va("Proximity %s | %.1f s left",charge.s.eFlags&EF_PROX_TRIP?"armed":"arming",
                           std::max<int64_t>(0,int64_t(values.expiry)-level.time)*.001f);
 else if(available)status=values.mode==Mode::DetPack?"Remote det pack ready":
  charge.s.eFlags&EF_FIRING?"Tripwire armed":"Tripwire arming";
 gi.cvar_set("jar_contextMountedChargeStatus",status);
 if(!publishing)return available;
 gi.cvar_set("jar_contextMountedChargeHealth",va("%d",values.health));
 gi.cvar_set("jar_contextMountedChargeDamage",va("%d",values.splash));
 gi.cvar_set("jar_contextMountedChargeRadius",va("%d",values.radius));
 return available;
}
void contextButtonStatus(const gentity_t& button){
 gi.cvar_set("jar_contextButtonStatus",nativeOwnedControl(button.s.number)?"Paused":
            button.svFlags&SVF_INACTIVE?"Disabled":JAR_ButtonPhaseName(button.count));
}
void contextBobbingStatus(const gentity_t& deck){
 gi.cvar_set("jar_contextBobbingRunning",deck.alt_fire?"1":"0");
 gi.cvar_set("jar_contextBobbingEnabled",deck.svFlags&SVF_INACTIVE?"0":"1");
 gi.cvar_set("jar_contextBobbingStatus",deck.svFlags&SVF_INACTIVE?"Disabled":
  nativeOwnedControl(deck.s.number)||!deck.alt_fire?"Paused":
  deck.fly_sound_debounce_time==JAR_BOB_BLOCKED?"Blocked. Waiting for clear space.":
  deck.fly_sound_debounce_time==JAR_BOB_MOVING?"Moving":"Ready");
}
void contextAngularStatus(const gentity_t& e){
 jar::sandboxAngular::Settings settings;
 if(!G_SandboxAngularRead(e,settings))return;
 gi.cvar_set("jar_contextAngularRunning",settings.running?"1":"0");
 gi.cvar_set("jar_contextAngularEnabled",settings.enabled?"1":"0");
 gi.cvar_set("jar_contextAngularStatus",!settings.enabled?"Disabled":
  nativeOwnedControl(e.s.number)||!settings.running?"Paused":
  e.s.eFlags&EF_BLOCKED_MOVER?"Blocked. Waiting for clearance.":"Running");
}
void contextSlideStatus(const gentity_t& door){
 const char* status="Idle";
 if(nativeOwnedControl(door.s.number)||door.fly_sound_debounce_time==JAR_SLIDE_PAUSED)status="Paused";
 else if(door.fly_sound_debounce_time==JAR_SLIDE_BLOCKED)status="Blocked";
 else if(door.fly_sound_debounce_time==JAR_SLIDE_MOVING)status="Moving";
 else if(door.delay>0)status="Waiting to close";
 gi.cvar_set("jar_contextSlideStatus",va("%s | %.1f / %d units",status,door.count/1000.f,door.damage));
}
void contextContactStatus(const gentity_t& source){
 const bool disarmed=G_ContactKind(source)==jar::contactTrigger::Hurt&&!(source.contents&CONTENTS_TRIGGER);
 gi.cvar_set("jar_contextContactEnabled",(source.svFlags&SVF_INACTIVE)?"0":"1");
 gi.cvar_set("jar_contextContactArmed",disarmed?"0":"1");
 gi.cvar_set("jar_contextContactSpent",G_ContactSpent(source)?"1":"0");
 gi.cvar_set("jar_contextContactStatus",source.svFlags&SVF_INACTIVE?"Disabled":disarmed?"Disarmed":
  G_ContactSpent(source)?"Spent. Rearm to use again.":
  G_ContactPending(source)?"Output pending":
  source.painDebounceTime>level.time?"Cooling down":"Ready");
}
void contextPushStatus(const gentity_t& source){
 const auto* target=G_PushTarget(source);
 gi.cvar_set("jar_contextPushTarget",target?va("Aim target: %d %s",target->s.number,target->classname?target->classname:"entity"):
  source.damage==jar::push::Forward?"Aim uses the entity's orientation":"No aim target linked");
 gi.cvar_set("jar_contextPushEnabled",G_LogicUseEnabled(source)?"1":"0");
}
void contextTeleportStatus(const gentity_t& source){
 const auto* target=G_TeleportDestination(source);
 const char* label=target&&editableLabel(*target)&&target->message&&*target->message?target->message:
  target&&target->classname?target->classname:"entity";
 gi.cvar_set("jar_contextTeleportTarget",target?va("Destination: %d %s",target->s.number,label):"No destination linked");
 gi.cvar_set("jar_contextTeleportEnabled",G_LogicUseEnabled(source)?"1":"0");
}
void contextStationStatus(const gentity_t& source){
 jar::sandboxStation::Settings station;
 if(!G_SandboxStationRead(source,station))return;
 gi.cvar_set("jar_contextStationCharge",va("%d",station.charge));
 gi.cvar_set("jar_contextStationCapacity",va("%d",station.capacity));
 gi.cvar_set("jar_contextStationUnlimited",station.unlimited?"1":"0");
 gi.cvar_set("jar_contextStationEnabled",station.enabled?"1":"0");
}
void contextBreakableStatus(const gentity_t& source){
 using namespace jar::sandboxBreakable;
 Settings state;
 if(!G_SandboxBreakableRead(source,state))return;
 gi.cvar_set("jar_contextBreakableStatus",state.phase==Broken?"Broken":state.phase==Pending?"Fuse armed":
  va("Health: %d / %d",state.health,state.capacity));
 gi.cvar_set("jar_contextBreakableEnabled",state.enabled?"1":"0");
}
void contextUsableStatus(const gentity_t& source){
 jar::sandboxUsable::Settings settings;
 if(!G_SandboxUsableRead(source,settings))return;
 const char* state=settings.pending?"Waiting for clearance":settings.spent?"Spent":
  settings.remaining?"Cooling down":settings.solid?"Solid":"Off";
 gi.cvar_set("jar_contextUsableStatus",va("%s | Health %d / %d",state,settings.health,settings.capacity));
 gi.cvar_set("jar_contextUsableEnabled",settings.enabled?"1":"0");
 gi.cvar_set("jar_contextUsableSolid",settings.solid?"1":"0");
 gi.cvar_set("jar_contextUsablePending",settings.pending?"1":"0");
}
void contextLogicLinks(const gentity_t& source,jar::logic::Kind kind){
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  const char* name=G_LogicOutput(source,slot);
  const auto* target=name?G_Find(nullptr,FOFS(targetname),name):nullptr;
  const bool available=target&&owned(target->s.number)&&
   (kind!=jar::logic::Give||G_LogicPickup(*target))&&
   (!jar::logic::UseGate(kind)||G_LogicCanSetUse(*target));
  const char* status=name?"Unavailable":"Unlinked";
  if(available){
   const char* type=target->classname?target->classname:"entity";
   status=jar::logic::UseGate(kind)?
    va("%d: %s (%s)",target->s.number,type,G_LogicUseEnabled(*target)?"use on":"use off"):
    va("%d: %s",target->s.number,type);
  }
  gi.cvar_set(va("jar_contextLogicOutput%d",slot),status);
 }
}
void contextLogicStatus(const gentity_t& source){
 const auto kind=G_LogicKind(source);
 const int remaining=jar::logic::OneShotMode(kind)?(source.count?0:1):source.count;
 gi.cvar_set("jar_contextLogicRemaining",va("%d",remaining));
 const bool pending=source.nextthink>0&&
  (source.e_ThinkFunc==thinkF_Think_Target_Delay||source.e_ThinkFunc==thinkF_func_timer_think||
   source.e_ThinkFunc==thinkF_target_relay_use_go);
 gi.cvar_set("jar_contextLogicPending",pending?"1":"0");
 if(kind==jar::logic::Relay)gi.cvar_set("jar_contextLogicCooling",source.painDebounceTime>level.time?"1":"0");
 if(jar::logic::UseGate(kind))contextLogicLinks(source,kind);
 gi.cvar_set("jar_contextLogicEnabled",source.svFlags&SVF_INACTIVE?"0":"1");
}
void contextPortalLinkStatus(gentity_t& e,bool allowed){
 const bool linked=allowed&&e.target&&*e.target;
 const auto* target=linked?G_PortalResolveLink(e):nullptr;
 gi.cvar_set("jar_contextPortalCanUnlink",linked&&G_JAR_WorkshopOwns(&e)?"1":"0");
 if(!allowed)gi.cvar_set("jar_contextPortalLink","");
 else if(G_PortalCamera(e))gi.cvar_set("jar_contextPortalLink",target?
  va("Aim link: #%d",target->s.number):linked?"Aim target missing. Using angles.":"Aim link: None");
 else gi.cvar_set("jar_contextPortalLink",target?
  va("Camera link: #%d (%s)",target->s.number,target->spawnflags&128?"Off":"On"):
  linked?"Linked camera missing.":"Camera link: None");
}
void contextPublish(bool nextRope=false){
 if(!contextValid()){contextClear();return;}auto&e=g_entities[contextEntity];
 if(!++contextSequence)++contextSequence;contextToken=contextSequence;
 gi.cvar_set("jar_contextToken",va("%u",contextToken));
 gi.cvar_set("jar_contextMountedVehicle",contextMountedVehicle?"1":"0");
 gi.cvar_set("jar_contextLabel",contextMountedVehicle?va("%d: Your vehicle",e.s.number):va("%d: %s",e.s.number,e.classname?e.classname:"entity"));
 contextRopePublish(e,nextRope);
 const auto* thruster=ThrusterFor(state,attachmentKey(e));
 gi.cvar_set("jar_contextThrusterId",va("%u",thruster?thruster->id:0));
 gi.cvar_set("jar_contextThrusterForce",thruster?va("%.6f",thruster->force):"");
 gi.cvar_set("jar_contextThrusterEnabled",thruster&&thruster->enabled?"1":"0");
 jar::skyPortal::Settings sky;gi.cvar_set("jar_contextSkyAllowed",G_SkyPortalRead(e,sky)?"1":"0");gi.cvar_set("jar_contextSkyEnabled",sky.enabled?"1":"0");gi.cvar_set("jar_contextSkyEntities",sky.entities?"1":"0");
 jar::combatPoint::Settings combat;const bool combatAllowed=G_CombatPointRead(e,combat);gi.cvar_set("jar_contextCombatAllowed",combatAllowed?"1":"0");gi.cvar_set("jar_contextCombatFlags",va("%d",combat.flags));gi.cvar_set("jar_contextCombatEnabled",combat.enabled?"1":"0");gi.cvar_set("jar_contextCombatStatus",combatAllowed?G_CombatPointStatus(e):"");
 jar::rail::Settings rail;const bool railAllowed=G_RailRead(e,rail);gi.cvar_set("jar_contextRailAllowed",railAllowed?"1":"0");gi.cvar_set("jar_contextRailKind",va("%d",int(rail.kind)));
 gi.cvar_set("jar_contextRailLength",va("%.3f",rail.length));gi.cvar_set("jar_contextRailWidth",va("%.3f",rail.width));gi.cvar_set("jar_contextRailCell",va("%.0f",rail.cell));gi.cvar_set("jar_contextRailSpeed",va("%.3f",rail.speed));gi.cvar_set("jar_contextRailDelay",va("%.3f",rail.delay));gi.cvar_set("jar_contextRailCount",va("%d",rail.count));gi.cvar_set("jar_contextRailEnabled",rail.enabled?"1":"0");gi.cvar_set("jar_contextRailCenter",rail.center?"1":"0");
 jar::waypoint::Settings waypoint;const bool waypointAllowed=G_WaypointRead(e,waypoint);gi.cvar_set("jar_contextWaypointAllowed",waypointAllowed?"1":"0");gi.cvar_set("jar_contextWaypointRange",va("%.3f",waypoint.range));gi.cvar_set("jar_contextWaypointEnabled",waypoint.enabled?"1":"0");gi.cvar_set("jar_contextWaypointStatus",waypointAllowed?G_WaypointStatus(e):"");
 jar::sceneCamera::Settings scene;const bool sceneAllowed=G_SceneCameraRead(e,scene);gi.cvar_set("jar_contextSceneAllowed",sceneAllowed?"1":"0");gi.cvar_set("jar_contextSceneTrack",jar::sceneCamera::Track(e.classname)?"1":"0");gi.cvar_set("jar_contextSceneSpeed",va("%.3f",scene.speed));gi.cvar_set("jar_contextSceneTurn",va("%.3f",scene.turn));gi.cvar_set("jar_contextSceneWait",va("%.3f",scene.wait));gi.cvar_set("jar_contextSceneLoop",scene.loop?"1":"0");
 jar::reference::Settings reference;const bool referenceAllowed=G_ReferenceRead(e,reference);gi.cvar_set("jar_contextReferenceAllowed",referenceAllowed?"1":"0");gi.cvar_set("jar_contextReferenceGoal",e.classname&&!strcmp(e.classname,"waypoint_navgoal")?"1":"0");gi.cvar_set("jar_contextReferenceName",reference.name.c_str());gi.cvar_set("jar_contextReferenceOwner",reference.owner.c_str());gi.cvar_set("jar_contextReferenceRadius",va("%d",reference.radius));gi.cvar_set("jar_contextReferenceEnabled",reference.enabled?"1":"0");
 const int poseFrames=propPoseFrameCount(e);gi.cvar_set("jar_contextPoseAllowed",poseFrames>1?"1":"0");gi.cvar_set("jar_contextPoseFrame",va("%d",poseFrames?e.s.frame:0));gi.cvar_set("jar_contextPoseLast",va("%d",std::max(0,poseFrames-1)));
 const auto* body=owned(e.s.number);
 const bool propPhysics=body&&matches(*body)&&prop(body->catalog);
 gi.cvar_set("jar_contextPropPhysicsAllowed",propPhysics?"1":"0");
 gi.cvar_set("jar_contextPropMass",va("%u",propPhysics?body->physics.mass:0u));
 gi.cvar_set("jar_contextPropGravity",propPhysics&&body->physics.noGravity?"0":"1");
 jar::levelChange::Settings travel;const bool travelAllowed=G_LevelChangeRead(e,travel);gi.cvar_set("jar_contextTravelAllowed",travelAllowed?"1":"0");
 gi.cvar_set("jar_contextTravelMap",travel.map.c_str());gi.cvar_set("jar_contextTravelSpawn",travel.spawn.c_str());gi.cvar_set("jar_contextTravelHub",travel.hub?"1":"0");
 const bool holocron=G_HolocronEditable(e);gi.cvar_set("jar_contextHolocronAllowed",holocron?"1":"0");
 gi.cvar_set("jar_contextHolocronLevel",va("%d",holocron?e.count:1));
 const bool pickupQuantity=G_WorkshopPickupQuantityEditable(e);
 gi.cvar_set("jar_contextPickupQuantityAllowed",pickupQuantity?"1":"0");
 gi.cvar_set("jar_contextPickupQuantity",va("%d",pickupQuantity?e.count:0));
 gi.cvar_set("jar_contextPickupWeapon",pickupQuantity&&e.item->giType==IT_WEAPON?"1":"0");
 gi.cvar_set("jar_contextPickupDuration",pickupQuantity&&e.item->giType==IT_POWERUP?"1":"0");
 const bool pickupLifetime=pickupLifetimeEditable(e);
 gi.cvar_set("jar_contextPickupLifetimeAllowed",pickupLifetime?"1":"0");
 gi.cvar_set("jar_contextPickupLifetime",va("%d",pickupLifetime?pickupLifetimeRemaining(e):0));
 jar::shieldCopy::Properties deployedShield;
 G_PortableShieldProperties(e,deployedShield);
 contextShieldStatus(e);
 gi.cvar_set("jar_contextDeployedShieldHealth",va("%d",deployedShield.health));
 gi.cvar_set("jar_contextDeployedShieldLeft",va("%.3f",deployedShield.left));
 gi.cvar_set("jar_contextDeployedShieldRight",va("%.3f",deployedShield.right));
 gi.cvar_set("jar_contextDeployedShieldHeight",va("%.3f",deployedShield.height));
 const bool chargeAllowed=contextChargeStatus(e,true);
 jar::scriptRunner::Settings runner;const bool script=G_ScriptRunnerRead(e,runner);
 gi.cvar_set("jar_contextScriptAllowed",script?"1":"0");
 gi.cvar_set("jar_contextScript",script?runner.script.c_str():"");
 gi.cvar_set("jar_contextScriptDelay",va("%.3f",runner.delay));gi.cvar_set("jar_contextScriptWait",va("%.3f",runner.wait));
 gi.cvar_set("jar_contextScriptCount",va("%d",runner.count));gi.cvar_set("jar_contextScriptActivator",runner.activator?"1":"0");
 jar::weather::Settings weather;const bool weatherAllowed=G_WeatherRead(e,weather);gi.cvar_set("jar_contextWeatherAllowed",weatherAllowed?"1":"0");
 gi.cvar_set("jar_contextWeatherKind",va("%d",weather.kind));gi.cvar_set("jar_contextWeatherMode",va("%d",weather.mode));
 gi.cvar_set("jar_contextWeatherDensity",va("%.3f",weather.density));gi.cvar_set("jar_contextWeatherSize",va("%.3f",weather.size));gi.cvar_set("jar_contextWeatherAlpha",va("%.3f",weather.alpha));gi.cvar_set("jar_contextWeatherSpeed",va("%.3f",weather.speed));gi.cvar_set("jar_contextWeatherEnabled",weather.enabled?"1":"0");
 jar::crystalCrate::Settings crystal;gi.cvar_set("jar_contextCrystalAllowed",G_CrystalCrateRead(e,crystal)?"1":"0");
 gi.cvar_set("jar_contextCrystalHitDamage",va("%d",crystal.hitDamage));gi.cvar_set("jar_contextCrystalHitRadius",va("%.3f",crystal.hitRadius));gi.cvar_set("jar_contextCrystalBlastDamage",va("%d",crystal.blastDamage));gi.cvar_set("jar_contextCrystalBlastRadius",va("%.0f",crystal.blastRadius));gi.cvar_set("jar_contextCrystalSolid",crystal.solid?"1":"0");
 jar::doorCamera::Settings door;const bool doorAllowed=G_DoorCameraRead(e,door);gi.cvar_set("jar_contextDoorAllowed",doorAllowed?"1":"0");
 gi.cvar_set("jar_contextDoorTravel",va("%.3f",door.travel));gi.cvar_set("jar_contextDoorSpeed",va("%.3f",door.speed));gi.cvar_set("jar_contextDoorExtended",door.extended?"1":"0");
 gi.cvar_set("jar_contextDoorMotion",doorAllowed?va("Extension: %.1f / %.1f%s",e.random,door.travel,e.alt_fire?" | Blocked":""):"");
 jar::welder::Settings welder;const bool welderAllowed=G_WelderRead(e,welder);gi.cvar_set("jar_contextWelderAllowed",welderAllowed?"1":"0");
 gi.cvar_set("jar_contextWelderReach",va("%.3f",welder.reach));gi.cvar_set("jar_contextWelderRadius",va("%.3f",welder.radius));gi.cvar_set("jar_contextWelderInterval",va("%.3f",welder.interval));gi.cvar_set("jar_contextWelderDamage",va("%d",welder.damage));gi.cvar_set("jar_contextWelderEnabled",welder.enabled?"1":"0");
 jar::spotlight::Settings spot;const bool spotAllowed=G_SpotlightRead(e,spot);gi.cvar_set("jar_contextSpotAllowed",spotAllowed?"1":"0");
 gi.cvar_set("jar_contextSpotRange",va("%.3f",spot.range));gi.cvar_set("jar_contextSpotCone",va("%.3f",spot.cone));gi.cvar_set("jar_contextSpotLight",va("%.3f",spot.light));
 for(int i=0;i<3;++i)gi.cvar_set(va("jar_contextSpotColor%d",i),va("%.3f",spot.rgb[i]));gi.cvar_set("jar_contextSpotTracking",spot.tracking?"1":"0");gi.cvar_set("jar_contextSpotEnabled",spot.enabled?"1":"0");
 jar::portal::Settings portal;const bool portalAllowed=G_PortalRead(e,portal);gi.cvar_set("jar_contextPortalAllowed",portalAllowed?"1":"0");gi.cvar_set("jar_contextPortalCamera",G_PortalCamera(e)?"1":"0");
 gi.cvar_set("jar_contextPortalWidth",va("%.3f",portal.width));gi.cvar_set("jar_contextPortalHeight",va("%.3f",portal.height));gi.cvar_set("jar_contextPortalMirror",portal.mirror?"1":"0");gi.cvar_set("jar_contextPortalEnabled",portal.enabled?"1":"0");
 contextPortalLinkStatus(e,portalAllowed);
 jar::laserArm::Settings laser;const bool laserAllowed=G_LaserArmRead(e,laser);gi.cvar_set("jar_contextLaserAllowed",laserAllowed?"1":"0");
 gi.cvar_set("jar_contextLaserAction",va("%d",laser.action));gi.cvar_set("jar_contextLaserDamage",va("%d",laser.damage));
 gi.cvar_set("jar_contextLaserYaw",va("%.3f",laser.yaw));gi.cvar_set("jar_contextLaserPitch",va("%.3f",laser.pitch));gi.cvar_set("jar_contextLaserSpeed",va("%.3f",laser.speed));gi.cvar_set("jar_contextLaserDuration",va("%.3f",laser.duration));gi.cvar_set("jar_contextLaserRange",va("%.3f",laser.range));
 jar::sandboxLaser::Settings emitter;
 const bool emitterAllowed=owned(e.s.number)&&G_SandboxLaserRead(e,emitter);
 gi.cvar_set("jar_contextLaserEmitterAllowed",emitterAllowed?"1":"0");
 gi.cvar_set("jar_contextLaserEmitterDamage",va("%d",emitter.damage));
 gi.cvar_set("jar_contextLaserEmitterFiring",emitter.firing?"1":"0");
 gi.cvar_set("jar_contextLaserEmitterEnabled",emitter.enabled?"1":"0");
 const auto* emitterAim=emitterAllowed?G_SandboxLaserAim(e):nullptr;
 gi.cvar_set("jar_contextLaserEmitterAim",emitterAim?va("Aim: %d (%s)",emitterAim->s.number,emitterAim->classname?emitterAim->classname:"entity"):
  emitterAllowed&&e.target?"Aim: target missing":"Aim: forward");
 const bool trail=G_WorkshopTrailClass(e)&&G_JAR_WorkshopOwns(&e);
 const auto* trailAim=trail?G_WorkshopFxEndpoint(e):nullptr;
 gi.cvar_set("jar_contextTrailAllowed",trail?"1":"0");
 gi.cvar_set("jar_contextTrailLinked",trail&&e.target?"1":"0");
 gi.cvar_set("jar_contextTrailAim",trailAim?va("Aim: entity %d",trailAim->s.number):"Aim: emitter facing");
 jar::reflection::Settings reflection;const bool probe=G_ReflectionRead(e,reflection);gi.cvar_set("jar_contextReflectionAllowed",probe?"1":"0");
 gi.cvar_set("jar_contextReflectionRadius",va("%.3f",reflection.radius));gi.cvar_set("jar_contextReflectionEnabled",reflection.enabled?"1":"0");
 jar::cloud::Settings cloud;const bool clouds=G_CloudRead(e,cloud);gi.cvar_set("jar_contextCloudAllowed",clouds?"1":"0");
 gi.cvar_set("jar_contextCloudRadius",va("%.3f",cloud.radius));gi.cvar_set("jar_contextCloudInner",va("%.3f",cloud.inner));gi.cvar_set("jar_contextCloudCurve",va("%.3f",cloud.curve));
 gi.cvar_set("jar_contextCloudTube",cloud.tube?"1":"0");gi.cvar_set("jar_contextCloudAlternate",cloud.alternate?"1":"0");gi.cvar_set("jar_contextCloudEnabled",cloud.enabled?"1":"0");
 const bool boundary=G_ShipBoundaryClass(e);gi.cvar_set("jar_contextBoundaryAllowed",boundary?"1":"0");
 gi.cvar_set("jar_contextBoundaryTime",va("%.3f",boundary?e.count/1000.f:3.f));gi.cvar_set("jar_contextBoundaryPreserve",e.spawnflags&1?"1":"0");
 const bool music=jar::music::Class(e.classname);gi.cvar_set("jar_contextMusicAllowed",music?"1":"0");
 gi.cvar_set("jar_contextMusicPortable",music&&G_PortableMusic(e)?"1":"0");
 const bool label=editableLabel(e);gi.cvar_set("jar_contextLabelAllowed",label?"1":"0");
 const bool always=G_AlwaysPortable(e);gi.cvar_set("jar_contextAlwaysAllowed",always?"1":"0");
 gi.cvar_set("jar_contextAlwaysDelay",va("%d",always?e.delay:JAR_ALWAYS_DEFAULT_MS));
 const bool visible=G_VisiblePortable(e);gi.cvar_set("jar_contextVisibleAllowed",visible?"1":"0");
 const auto sight=visible?G_VisibleSettings(e):jarVisibleSettings_t{JAR_VISIBLE_RANGE_DEFAULT,JAR_VISIBLE_CONE_DEFAULT,0};
 gi.cvar_set("jar_contextVisibleRange",va("%d",sight.range));gi.cvar_set("jar_contextVisibleCone",va("%d",sight.cone));
 gi.cvar_set("jar_contextVisibleFlags",va("%d",sight.flags));
 gi.cvar_set("jar_contextVisibleEnabled",visible&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 const bool distance=G_DistancePortable(e);gi.cvar_set("jar_contextDistanceAllowed",distance?"1":"0");
 gi.cvar_set("jar_contextDistanceRange",va("%d",distance?e.count:JAR_DISTANCE_DEFAULT));
 gi.cvar_set("jar_contextDistanceSubjects",va("%d",distance?e.spawnflags:JAR_DISTANCE_PLAYER));
 gi.cvar_set("jar_contextDistanceArmed",distance&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 const auto endpointStatus=[](const char* name){
  if(!name||!*name)return "0";
  return strcmp(name,JAR_DISTANCE_MISSING_REFERENCE_NAME)&&G_Find(nullptr,FOFS(targetname),name)?"1":"2";
 };
 gi.cvar_set("jar_contextDistanceReference",distance?endpointStatus(e.ownername):"0");
 gi.cvar_set("jar_contextDistanceSubject",distance?endpointStatus(e.NPC_target):"0");
 const bool wall=G_WallPortable(e);
 gi.cvar_set("jar_contextWallAllowed",wall?"1":"0");
 gi.cvar_set("jar_contextWallVisible",wall&&e.count?"1":"0");
 gi.cvar_set("jar_contextWallEnabled",wall&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 jar::slidingDoor::State slide;
 const bool sliding=G_SandboxSlideRead(e,slide);
 gi.cvar_set("jar_contextSlideAllowed",sliding?"1":"0");
 gi.cvar_set("jar_contextSlideLift",sliding&&G_LiftPortable(e)?"1":"0");
 gi.cvar_set("jar_contextSlideEnabled",sliding&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 gi.cvar_set("jar_contextSlideTravel",va("%d",slide.settings.travel));
 gi.cvar_set("jar_contextSlideSpeed",va("%d",slide.settings.speed));
 gi.cvar_set("jar_contextSlideDirection",va("%d",slide.settings.direction));
 gi.cvar_set("jar_contextSlideWait",va("%d",slide.settings.waitMs));
 if(sliding)contextSlideStatus(e);
 jarButtonState_t button{};
 const bool buttonAllowed=G_ButtonRead(e,button);
 gi.cvar_set("jar_contextButtonAllowed",buttonAllowed?"1":"0");
 gi.cvar_set("jar_contextButtonWait",va("%d",button.waitMs));
 gi.cvar_set("jar_contextButtonEnabled",buttonAllowed&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 if(buttonAllowed)contextButtonStatus(e);
 jarBobState_t bob{};
 const bool bobAllowed=owned(e.s.number)&&G_BobbingRead(e,bob);
 gi.cvar_set("jar_contextBobbingAllowed",bobAllowed?"1":"0");
 gi.cvar_set("jar_contextBobbingAmplitude",va("%d",bob.amplitude));
 gi.cvar_set("jar_contextBobbingPeriod",va("%d",bob.periodMs));
 if(bobAllowed)contextBobbingStatus(e);
 jar::movement::Marker movement;
 jar::movement::ActorState actorMovement;
 const bool movementMarker=owned(e.s.number)&&G_MovementMarkerRead(e,movement);
 const bool movementActor=G_MovementActorRead(e,actorMovement);
 const auto movementKind=movementMarker?movement.kind:movementActor?jar::movement::Actor:jar::movement::None;
 gi.cvar_set("jar_contextMovementKind",va("%d",int(movementKind)));
 gi.cvar_set("jar_contextMovementGravity",va("%d",movement.kind==jar::movement::Gravity?movement.value:actorMovement.gravity));
 gi.cvar_set("jar_contextMovementFriction",va("%d",movement.kind==jar::movement::Friction?movement.value:actorMovement.friction));
 gi.cvar_set("jar_contextMovementCustomGravity",actorMovement.customGravity?"1":"0");
 gi.cvar_set("jar_contextMovementEnabled",movement.enabled?"1":"0");
 jar::contactTrigger::Settings contact;
 const bool contactAllowed=owned(e.s.number)&&G_ContactRead(e,contact);
 gi.cvar_set("jar_contextContactKind",va("%d",contactAllowed?int(contact.kind):0));
 gi.cvar_set("jar_contextContactDelay",va("%d",contact.delay));
 gi.cvar_set("jar_contextContactInterval",va("%d",contact.interval));
 gi.cvar_set("jar_contextContactVariation",va("%d",contact.variation));
 gi.cvar_set("jar_contextContactDamage",va("%d",contact.damage));
 gi.cvar_set("jar_contextContactMode",va("%d",contact.mode));
 gi.cvar_set("jar_contextContactSubject",va("%d",contact.subject));
 gi.cvar_set("jar_contextContactUse",contact.useRequired?"1":"0");
 if(contactAllowed)contextContactStatus(e);
 jar::push::Settings push;
 const bool pushAllowed=owned(e.s.number)&&G_PushRead(e,push);
 gi.cvar_set("jar_contextPushKind",va("%d",pushAllowed?int(push.kind):0));
 gi.cvar_set("jar_contextPushSpeed",va("%d",push.speed));
 gi.cvar_set("jar_contextPushInterval",va("%d",push.interval));
 gi.cvar_set("jar_contextPushAim",va("%d",push.aim));
 gi.cvar_set("jar_contextPushSubject",va("%d",push.subject));
 gi.cvar_set("jar_contextPushAdd",push.additive?"1":"0");
 gi.cvar_set("jar_contextPushVehicles",push.vehicles?"1":"0");
 if(pushAllowed)contextPushStatus(e);
 jar::teleport::Settings teleport;
 const bool teleportAllowed=owned(e.s.number)&&G_TeleportRead(e,teleport);
 gi.cvar_set("jar_contextTeleportKind",va("%d",teleportAllowed?int(teleport.kind):0));
 gi.cvar_set("jar_contextTeleportInterval",va("%d",teleport.interval));
 gi.cvar_set("jar_contextTeleportSubject",va("%d",teleport.subject));
 gi.cvar_set("jar_contextTeleportView",teleport.keepView?"1":"0");
 gi.cvar_set("jar_contextTeleportVelocity",teleport.keepVelocity?"1":"0");
 gi.cvar_set("jar_contextTeleportProps",teleport.props?"1":"0");
 gi.cvar_set("jar_contextTeleportProjectiles",teleport.projectiles?"1":"0");
 gi.cvar_set("jar_contextTeleportVehicles",teleport.vehicles?"1":"0");
 gi.cvar_set("jar_contextTeleportAssemblies",teleport.assemblies?"1":"0");
 gi.cvar_set("jar_contextTeleportFrozenProps",teleport.frozenProps?"1":"0");
 gi.cvar_set("jar_contextTeleportMovers",teleport.movers?"1":"0");
 gi.cvar_set("jar_contextTeleportWorldAnchors",teleport.worldAnchors?"1":"0");
 if(teleportAllowed)contextTeleportStatus(e);
 jar::sandboxStation::Settings station;
 const bool stationAllowed=owned(e.s.number)&&G_SandboxStationRead(e,station);
 gi.cvar_set("jar_contextStationAllowed",stationAllowed?"1":"0");
 gi.cvar_set("jar_contextStationRecharge",va("%d",station.recharge));
 if(stationAllowed)contextStationStatus(e);
 jar::sandboxBreakable::Settings breakable;
 const bool breakableAllowed=owned(e.s.number)&&G_SandboxBreakableRead(e,breakable);
 gi.cvar_set("jar_contextBreakableAllowed",breakableAllowed?"1":"0");
 gi.cvar_set("jar_contextBreakableCapacity",va("%d",breakable.capacity));
 gi.cvar_set("jar_contextBreakableFuse",va("%d",breakable.fuse));
 gi.cvar_set("jar_contextBreakableChunks",va("%d",breakable.chunks));
 gi.cvar_set("jar_contextBreakableSplash",va("%d",breakable.splash));
 gi.cvar_set("jar_contextBreakableRadius",va("%d",breakable.radius));
 gi.cvar_set("jar_contextBreakableMode",va("%d",breakable.mode));
 gi.cvar_set("jar_contextBreakableMaterial",va("%d",breakable.material));
 gi.cvar_set("jar_contextBreakablePlayerUse",breakable.playerUse?"1":"0");
 gi.cvar_set("jar_contextBreakableExplosion",breakable.explosion?"1":"0");
 if(breakableAllowed)contextBreakableStatus(e);
 jar::sandboxAngular::Settings angular;
 const bool angularAllowed=owned(e.s.number)&&G_SandboxAngularRead(e,angular);
 gi.cvar_set("jar_contextAngularAllowed",angularAllowed?"1":"0");
 if(angularAllowed){
  gi.cvar_set("jar_contextAngularKind",va("%d",angular.kind));
  gi.cvar_set("jar_contextAngularAxis",va("%d",angular.axis));
  gi.cvar_set("jar_contextAngularRate",va("%d",angular.rate));
  gi.cvar_set("jar_contextAngularPeriod",va("%d",angular.period));
  contextAngularStatus(e);
 }
 jar::speaker::Settings speaker;
 const bool speakerAllowed=owned(e.s.number)&&G_SandboxSpeakerRead(e,speaker);
 gi.cvar_set("jar_contextSpeakerAllowed",speakerAllowed?"1":"0");
 if(speakerAllowed){
  gi.cvar_set("jar_contextSpeakerCue",jar::speaker::cues[speaker.cue].label);
  gi.cvar_set("jar_contextSpeakerMode",jar::speaker::ModeLabel(speaker.mode));
  gi.cvar_set("jar_contextSpeakerEnabled",speaker.enabled?"1":"0");
  gi.cvar_set("jar_contextSpeakerState",speaker.active?(speaker.enabled?"Playing":"Paused"):"Idle");
  gi.cvar_set("jar_contextSpeakerPeriod",va("%d",speaker.period));
  gi.cvar_set("jar_contextSpeakerVariation",va("%d",speaker.variation));
 }
 jar::sandboxUsable::Settings usable;
 const bool usableAllowed=owned(e.s.number)&&G_SandboxUsableRead(e,usable);
 gi.cvar_set("jar_contextUsableAllowed",usableAllowed?"1":"0");
 gi.cvar_set("jar_contextUsableCapacity",va("%d",usable.capacity));
 gi.cvar_set("jar_contextUsableReuse",va("%d",usable.reuse));
 gi.cvar_set("jar_contextUsableMode",va("%d",usable.mode));
 gi.cvar_set("jar_contextUsableDamage",va("%d",usable.damage));
 gi.cvar_set("jar_contextUsableDirect",va("%d",usable.direct));
 if(usableAllowed){
  contextUsableStatus(e);
  for(int slot=0;slot<2;++slot){
   const char* name=G_LogicOutput(e,slot);
   const auto* target=name?G_Find(nullptr,FOFS(targetname),name):nullptr;
   gi.cvar_set(va("jar_contextUsableOutput%d",slot),
    target?va("%d: %s",target->s.number,target->classname?target->classname:"entity"):name?"Unavailable":"Unlinked");
  }
 }
 jar::logic::State logic;
 const bool logicAllowed=owned(e.s.number)&&G_LogicRead(e,logic);
 gi.cvar_set("jar_contextLogicKind",va("%d",logicAllowed?int(logic.kind):0));
 gi.cvar_set("jar_contextLogicDelay",va("%d",logic.delayMs));
 gi.cvar_set("jar_contextLogicJitter",va("%d",logic.jitterMs));
 gi.cvar_set("jar_contextLogicHits",va("%d",logic.hits));
 gi.cvar_set("jar_contextLogicCooldown",va("%d",logic.cooldownMs));
 gi.cvar_set("jar_contextLogicPoints",va("%d",logic.points));
 gi.cvar_set("jar_contextLogicKillMode",va("%d",logic.killMode));
 if(logicAllowed)contextLogicStatus(e);
 if(logicAllowed&&jar::logic::Slotted(logic.kind)&&!jar::logic::UseGate(logic.kind))contextLogicLinks(e,logic.kind);
 gi.cvar_set("jar_contextLogicRepeat",logic.repeat?"1":"0");
 gi.cvar_set("jar_contextText",(label||music)&&e.message?e.message:"");
 const char*location=G_GetLocationForEnt(&e);gi.cvar_set("jar_contextLocation",location?location:"No named location here");
 vec3_t dimensions;const bool volume=jar::volumeEntity::Dimensions(e,dimensions);
 gi.cvar_set("jar_contextVolumeAllowed",volume?"1":"0");
 const bool space=volume&&G_SpacePortable(e);
 gi.cvar_set("jar_contextSpaceAllowed",space?"1":"0");
 gi.cvar_set("jar_contextSpaceArmed",space&&(e.contents&CONTENTS_TRIGGER)?"1":"0");
 gi.cvar_set("jar_contextSpaceEnabled",space&&!(e.svFlags&SVF_INACTIVE)?"1":"0");
 for(int axis=0;axis<3;++axis)gi.cvar_set(va("jar_contextVolume%d",axis),va("%.3f",volume?dimensions[axis]:0));
 jar::vehicleStatus::Values vehicleValues;
 const bool vehicleAllowed=contextVehicleResources(e,vehicleValues);
 const auto vehicleLimits=vehicleAllowed?G_WorkshopVehicleLimits(*e.m_pVehicle->m_pVehicleInfo):jar::vehicleStatus::Values{};
 gi.cvar_set("jar_contextVehicleAllowed",vehicleAllowed?"1":"0");
 for(int i=0;i<jar::vehicleStatus::fields;++i){
  gi.cvar_set(va("jar_contextVehicleValue%d",i),va("%d",vehicleValues.value[i]));
  gi.cvar_set(va("jar_contextVehicleMax%d",i),va("%d",vehicleLimits.value[i]));
 }
 contextPublishedActorStatus={};contextPublishedActor=contextActorStatus(e,contextPublishedActorStatus);
 gi.cvar_set("jar_contextNPCAllowed",contextPublishedActor?"1":"0");
 gi.cvar_set("jar_contextNPCTeamsAllowed",contextPublishedActor&&!(e.svFlags&SVF_LOCKEDENEMY)?"1":"0");
 gi.cvar_set("jar_contextNPCTeam",va("%d",contextPublishedActorStatus.team));
 gi.cvar_set("jar_contextNPCTargetTeam",va("%d",contextPublishedActorStatus.enemyTeam));
 gi.cvar_set("jar_contextNPCArmor",va("%d",contextPublishedActorStatus.armor));
 contextPublishedActorFire={};contextPublishedFire=contextActorFire(e,contextPublishedActorFire);
 gi.cvar_set("jar_contextNPCFireAllowed",contextPublishedFire?"1":"0");
 gi.cvar_set("jar_contextNPCAltAllowed",contextPublishedFire&&contextPublishedActorFire.hasAlternate?"1":"0");
 gi.cvar_set("jar_contextNPCAmmo",va("%d",contextPublishedActorFire.ammo));
 gi.cvar_set("jar_contextNPCAlternate",contextPublishedActorFire.alternate?"1":"0");
 gi.cvar_set("jar_contextNPCManualAmmo",contextPublishedActorFire.manualAmmo?"1":"0");
 contextPublishedActorWeapons=contextActorWeaponChoices(e);
 contextPublishedActorWeapon=contextPublishedActorWeapons?e.client->ps.weapon:WP_NONE;
 gi.cvar_set("jar_contextNPCWeaponChoices",va("%u",contextPublishedActorWeapons));
 gi.cvar_set("jar_contextNPCWeapon",va("%d",contextPublishedActorWeapon));
 contextPublishedOrder={};contextPublishedOrderAllowed=contextActorOrder(e,contextPublishedOrder);
 gi.cvar_set("jar_contextNPCOrderAllowed",contextPublishedOrderAllowed?"1":"0");
 gi.cvar_set("jar_contextNPCOrderMode",va("%d",int(contextPublishedOrder.mode)));
 gi.cvar_set("jar_contextNPCOrderDistance",va("%.3f",contextPublishedOrder.distance?contextPublishedOrder.distance:float(jar::npcOrder::defaultDistance)));
 contextPublishedNPCGoal=contextPublishedOrderAllowed?G_WorkshopReadNPCGoal(*e.NPC):jar::npcOrder::Destination{};
 contextNPCGoalStatus(contextPublishedNPCGoal,contextPublishedOrderAllowed?e.NPC:nullptr);
 gi.cvar_set("jar_contextHealthAllowed",!travelAllowed&&!holocron&&!volume&&!label&&!music&&contextHealthEditable(e)?"1":"0");
 for(int axis=0;axis<3;++axis){gi.cvar_set(va("jar_contextPosition%d",axis),va("%.3f",e.currentOrigin[axis]));gi.cvar_set(va("jar_contextAngle%d",axis),va("%.3f",e.currentAngles[axis]));}
 contextPublishedHealth=e.health;
 gi.cvar_set("jar_contextHealth",va("%d",contextPublishedHealth));
 jar::shooter::Settings shooter;
 const bool shooterAllowed=G_SandboxShooterRead(e,shooter);
 gi.cvar_set("jar_contextShooterAllowed",shooterAllowed?"1":"0");
 gi.cvar_set("jar_contextShooterInterval",va("%d",shooter.interval));
 gi.cvar_set("jar_contextShooterVariation",va("%d",shooter.variation));
 gi.cvar_set("jar_contextShooterAlternate",shooter.alternate?"1":"0");
 gi.cvar_set("jar_contextShooterRepeat",shooter.repeat?"1":"0");
 gi.cvar_set("jar_contextShooterEnabled",shooter.enabled?"1":"0");
 const bool secret=G_SandboxSecret(e);
 gi.cvar_set("jar_contextSecretAllowed",secret?"1":"0");
 gi.cvar_set("jar_contextSecretRepeat",secret&&(e.spawnflags&JAR_SECRET_REPEAT)?"1":"0");
 gi.cvar_set("jar_contextSecretFound",secret&&(e.spawnflags&JAR_SECRET_FOUND)?"1":"0");
 gi.cvar_set("jar_contextSecretEnabled",secret&&G_LogicUseEnabled(e)?"1":"0");
 const bool replicator=G_SandboxReplicator(e);
 const auto replication=replicator?G_ReplicatorPhase(e,level.time):jar::replicator::State{};
 gi.cvar_set("jar_contextReplicatorAllowed",replicator?"1":"0");
 gi.cvar_set("jar_contextReplicatorPhase",va("%d",replication.phase));
 gi.cvar_set("jar_contextReplicatorHeal",va("%d",replicator?G_ReplicatorHeal(e):0));
 gi.cvar_set("jar_contextReplicatorDelay",va("%d",replicator?G_ReplicatorDelay(e):0));
 gi.cvar_set("jar_contextReplicatorEnabled",replicator&&G_LogicUseEnabled(e)?"1":"0");
 for(unsigned i=0;i<jar::parms::count;++i){std::string value;const bool valid=jar::parms::Read(e.parms?e.parms->parm[i]:nullptr,value);gi.cvar_set(va("jar_contextParm%u",i),valid?value.c_str():"");gi.cvar_set(va("jar_contextParmAllowed%u",i),valid?"1":"0");}
 const auto*o=owned(contextEntity);const auto native=nativeControls.find(contextEntity);
 const bool frozen=o&&prop(o->catalog)?o->frozen!=0:native!=nativeControls.end()&&native->second.frozen;
 gi.cvar_set("jar_contextInfo",va("%s | %s%s",chargeAllowed?"Player-planted":o?"Spawned":"Map entity",frozen?"Frozen":"Released",music?(G_PortableMusic(e)?(G_MusicActive(e)?" | Music on":" | Music off"):" | Native music"):""));
}
void contextSelect(){
 if(contextEntity!=selected||(selected<ENTITYNUM_WORLD&&contextGeneration!=entityGeneration[selected]))contextRopeId=0;
 contextEntity=selected;if(contextEntity<ENTITYNUM_WORLD)contextGeneration=entityGeneration[contextEntity];contextPublish();
}
void contextCommand(){
 const char*action=gi.argv(2);
 if(!strcmp(action,"clear")){contextClear();return;}
 const bool opening=!strcmp(action,"open");
 if(opening){
  contextClear();
  if(enabled())if(auto* hull=contextPlayerVehicle()){
   contextMountedVehicle=true;
   selected=hull->s.number;
   contextSelect();return;
  }
 }
 if(contextMountedVehicle){
  const auto* hull=enabled()?contextPlayerVehicle():nullptr;
  if(!hull||!contextValid()||hull->s.number!=int(contextEntity)){contextClear();return;}
  if(strcmp(action,"refresh")&&strcmp(action,"revert")&&strcmp(action,"vehicle_resources"))return;
 }
 // A player frozen by this editor must still be able to select/release itself.
 const bool releasePlayer=enabled()&&nativeControls.count(0)&&g_entities[0].health>0&&
  (!strcmp(action,"player")||((!strcmp(action,"release")||!strcmp(action,"refresh"))&&contextEntity==0));
 if(!contextMountedVehicle&&!releasePlayer&&!sandboxReady())return;
 if(opening){selectAt(320,240);contextSelect();return;}
 if(!strcmp(action,"pick")){float x,y;
  if(jar::context::Number(gi.argv(3),x,0,640)&&jar::context::Number(gi.argv(4),y,0,480)){selectAt(x,y);contextSelect();}return;
 }
 if(!strcmp(action,"player")){selected=0;contextSelect();return;}
 uint32_t token=0;
 if(!jar::context::Token(gi.argv(3),token)||token!=contextToken||!contextValid()){
  message("Selection changed. Select the entity again before editing.");return;
 }
 auto&e=g_entities[contextEntity];selected=contextEntity;
 if(!strcmp(action,"refresh")){contextPublish();return;}
 if(!strcmp(action,"revert")){revertProperty(contextMountedVehicle);contextPublish();return;}
 if(!strcmp(action,"rope_next")){contextPublish(true);return;}
 const WorkshopActionScope contextEdit(recordingContextEdit);
 if(!strcmp(action,"thruster_properties")){
  uint32_t id;float force,on,reverse;
  auto* thruster=ThrusterFor(state,attachmentKey(e));
  if(!jar::context::Token(gi.argv(4),id)||!thruster||thruster->id!=id){
   message("Select an entity with this mounted thruster.");contextPublish();return;
  }
  if(!jar::context::Number(gi.argv(5),force,jar::constraint::thrusterMinForce,jar::constraint::thrusterMaxForce)||
     !jar::context::Number(gi.argv(6),on,0,1)||on!=std::floor(on)||
     !jar::context::Number(gi.argv(7),reverse,0,1)||reverse!=std::floor(reverse)){
   message("Use force from 1 to 100000 and the supplied power and direction choices.");return;
  }
  if(thruster->force==force&&thruster->enabled==unsigned(on)&&!reverse)return;
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];
  edit.thrusterId=id;edit.thrusterBefore=ThrusterProperties(*thruster);
  thruster->force=force;thruster->enabled=unsigned(on);
  if(reverse){jar::constraint::ReverseAxis(thruster->a);jar::constraint::ReverseAxis(thruster->b);}
  edit.thrusterAfter=ThrusterProperties(*thruster);
  rememberAction(std::move(edit));message("Thruster properties applied.");
 }else if(!strcmp(action,"rope_length")){
  uint32_t id;float length;
  auto* rope=jar::context::Token(gi.argv(4),id)&&id==contextRopeId?selectedRope(e,id):nullptr;
  if(!rope){
   message("Select an attached rope before editing its length.");contextPublish();return;
  }
  if(!jar::context::Number(gi.argv(5),length,jar::constraint::ropeMinLength,jar::constraint::ropeMaxLength)){
   message("Enter a rope length from 0.01 to 8192 world units.");return;
  }
  const float before=rope->length;
  if(length==before)return;
  if(!resizeRope(*rope,length)){message("Rope length could not be applied around scenery. Clear its route and retry.");return;}
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];
  edit.ropeLengthId=id;edit.ropeLengthBefore=before;edit.ropeLengthAfter=length;
  rememberAction(std::move(edit));message(va("Rope %u length set to %.2f units.",id,length));
 }else if(!strcmp(action,"prop_physics")||!strcmp(action,"prop_gravity")){
  auto* body=owned(e.s.number);
  if(!body||!matches(*body)||!prop(body->catalog)){message("Select a spawned prop to edit its physics.");return;}
  auto next=body->physics;
  if(!strcmp(action,"prop_gravity"))next.noGravity=!next.noGravity;
  else {
   float mass;
   if(!jar::context::Number(gi.argv(4),mass,0,maxPropMass)||mass!=std::floor(mass)){
    message("Use a whole mass from 1 to 100000, or 0 for the model default.");return;
   }
   next.mass=uint32_t(mass);
  }
  if(next.mass==body->physics.mass&&next.noGravity==body->physics.noGravity)return;
  rememberEdit(e);edits.back().propPhysicsEdit=true;edits.back().propPhysics=body->physics;
  body->physics=next;message("Prop physics applied. Revert property restores the previous settings.");
 }else if(!strcmp(action,"reflection_refresh")){
  if(G_ReflectionRefresh(e))message("Reflection capture queued. The marker must settle before capturing the map.");
  else message("Select a spawned reflection probe.");
 }else if(!strcmp(action,"reflection")){
  jar::reflection::Settings before,next;float on;
  if(!G_ReflectionRead(e,before)||!jar::context::Number(gi.argv(4),next.radius,16,32768)||!jar::context::Number(gi.argv(5),on,0,1)||on!=std::floor(on)){message("Reflection radius: 16-32768 units. Choose On or Off.");return;}
  next.enabled=on!=0;rememberEdit(e);edits.back().reflectionEdit=true;edits.back().reflection=before;G_ReflectionSet(e,next);
  message("Reflection properties applied. Rend2 and Cubemap reflections must be enabled in Graphics; Revert property restores properties.");
 }else if(!strcmp(action,"rail")){
  jar::rail::Settings before,next;float count,on,center;
  if(!G_RailRead(e,before)){message("Select a spawned rail controller.");return;}next=before;
  if(!jar::context::Number(gi.argv(4),next.length,128,8192)||!jar::context::Number(gi.argv(5),next.width,64,2048)||!jar::context::Number(gi.argv(6),next.cell,32,256)||!jar::context::Number(gi.argv(7),next.speed,16,2000)||!jar::context::Number(gi.argv(8),next.delay,0,30)||!jar::context::Number(gi.argv(9),count,1,8)||count!=std::floor(count)||!jar::context::Number(gi.argv(10),on,0,1)||on!=std::floor(on)||!jar::context::Number(gi.argv(11),center,0,1)||center!=std::floor(center)){message("Rail values are outside the displayed limits.");return;}
  next.count=int(count);next.enabled=on!=0;next.center=center!=0;if(!jar::rail::Valid(next)){message("Grid size must be a whole number no larger than track width.");return;}
  rememberEdit(e);edits.back().railEdit=true;edits.back().rail=before;G_RailSet(e,next);message("Rail properties applied. Active trips pause when disabled; new trips use the edited track.");
 }else if(!strcmp(action,"waypoint")){
  jar::waypoint::Settings before,next;float on;
  if(!G_WaypointRead(e,before)||!jar::context::Number(gi.argv(4),next.range,0,1024)||!jar::context::Number(gi.argv(5),on,0,1)||on!=std::floor(on)){message("Navigation: connection range 0-1024, enabled On/Off. Zero range uses manual links.");return;}
  next.enabled=on!=0;if(!G_WaypointSet(e,next))return;rememberEdit(e);edits.back().waypointEdit=true;edits.back().waypoint=before;message("Navigation settings applied; routes refresh between simulation frames.");
 }else if(!strcmp(action,"scene")){
  jar::sceneCamera::Settings before,next;float loop;
  if(!G_SceneCameraRead(e,before)||!jar::context::Number(gi.argv(4),next.speed,0,1024)||!jar::context::Number(gi.argv(5),next.turn,1,720)||!jar::context::Number(gi.argv(6),next.wait,0,30)||!jar::context::Number(gi.argv(7),loop,0,1)||loop!=std::floor(loop)){message("Camera: speed 0-1024, turn 1-720, wait 0-30, loop On/Off.");return;}
  next.loop=loop!=0;if(!G_SceneCameraSet(e,next))return;rememberEdit(e);edits.back().sceneCameraEdit=true;edits.back().sceneCamera=before;message("Camera properties applied. Activate to view; move or press USE to exit.");
 }else if(!strcmp(action,"sky")){
  jar::skyPortal::Settings before,next;float on,entities;if(!G_SkyPortalRead(e,before)||!jar::context::Number(gi.argv(4),on,0,1)||on!=std::floor(on)||!jar::context::Number(gi.argv(5),entities,0,1)||entities!=std::floor(entities)){message("Choose sky On/Off and entities On/Off.");return;}
  next.enabled=on!=0;next.entities=entities!=0;const int order=G_SkyPortalOrder(e);if(!G_SkyPortalSet(e,next)){message("Sky viewpoint is unavailable or outside world limits.");return;}rememberEdit(e);edits.back().skyEdit=true;edits.back().sky=before;edits.back().skyOrder=order;message("Sky properties applied. Latest enabled viewpoint wins; removal restores the previous sky.");
 }else if(!strcmp(action,"combat")){
  jar::combatPoint::Settings before,next;float flags,on;
  if(!G_CombatPointRead(e,before)||!jar::context::Number(gi.argv(4),flags,0,63)||flags!=std::floor(flags)||!jar::context::Number(gi.argv(5),on,0,1)||on!=std::floor(on)){message("Use native combat flags 0-63 and On/Off.");return;}
  next.flags=int(flags);next.enabled=on!=0;if(!G_CombatPointSet(e,next)){message("Combat point is no longer registered.");return;}
  rememberEdit(e);edits.back().combatPointEdit=true;edits.back().combatPoint=before;message(G_CombatPointStatus(e));
 }else if(!strcmp(action,"reference")){
  jar::reference::Settings before,next;float radius,on;
  if(!G_ReferenceRead(e,before)||!Unhex(gi.argv(4),next.name,31)||!Unhex(gi.argv(5),next.owner,31)||!jar::context::Number(gi.argv(6),radius,0,1024)||radius!=std::floor(radius)||!jar::context::Number(gi.argv(7),on,0,1)||on!=std::floor(on)){message("Use a tag name, optional owner, whole radius 0-1024 and On/Off.");return;}
  next.radius=int(radius);next.enabled=on!=0;
  if(!G_ReferenceSet(e,next)){message("Tag name is invalid or already reserved in that scope. Mission tags cannot be replaced.");return;}
  rememberEdit(e);edits.back().referenceEdit=true;edits.back().reference=before;message("Tag properties applied. Scripts can query its current pose; navgoal scripts use its goal radius.");
 }else if(!strcmp(action,"pose")){
  float frame;const int count=propPoseFrameCount(e);
  if(count<1||!jar::context::Number(gi.argv(4),frame,0,float(count-1))||frame!=std::floor(frame)){message("Choose a whole pose frame within this model's displayed range.");return;}
  if(e.s.frame==int(frame))return;const int previous=e.s.frame;
  if(!setPropPose(e,int(frame))){message("Pose is obstructed or has no usable collision bounds. Move the prop into clear space first.");return;}
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];
  edit.poseEdit=true;edit.poseFrame=previous;rememberAction(std::move(edit));
  message("Model pose and collision updated. Revert property restores the previous frame when its space is clear.");
 }else if(!strcmp(action,"travel")){
  jar::levelChange::Settings before,next;float hub;
  if(!G_LevelChangeRead(e,before)||!Unhex(gi.argv(4),next.map,jar::levelChange::mapLimit)||!Unhex(gi.argv(5),next.spawn,jar::levelChange::spawnLimit)||!jar::context::Number(gi.argv(6),hub,0,1)||hub!=std::floor(hub)||!G_LevelChangeAvailable(next)){message("Choose an installed map name without maps/ or .bsp, an optional spawn point, and normal or hub travel.");return;}
  next.hub=hub!=0;rememberEdit(e);edits.back().levelChangeEdit=true;edits.back().levelChange=before;
  if(!G_LevelChangeSet(e,next)){edits.pop_back();return;}
  message("Travel settings applied. Activate changes map; Revert property restores these settings before departure.");
 }else if(!strcmp(action,"deployed_shield")){
  jar::shieldCopy::Properties before,next;
  float health,remaining;
  if(!G_PortableShieldProperties(e,before)||
     !jar::context::Number(gi.argv(4),health,1,jar::shieldCopy::maxHealth)||health!=std::floor(health)||
     !jar::context::Number(gi.argv(5),remaining,0,jar::shieldCopy::lifetime)||remaining!=std::floor(remaining)||
     !jar::context::Number(gi.argv(6),next.left,3,254)||!jar::context::Number(gi.argv(7),next.right,3,254)||
     !jar::context::Number(gi.argv(8),next.height,32,253)||int64_t(level.time)+int(remaining)>INT_MAX){
   message("Use valid shield dimensions, whole health and up to 25000 ms. Zero keeps the current expiry.");return;
  }
  next.health=int(health);next.deadline=remaining?level.time+int(remaining):before.deadline;
  if(jar::shieldCopy::Same(before,next))return;
  if(!G_PortableShieldSetProperties(e,next)){message("The new shield hull is obstructed. Properties unchanged.");return;}
  rememberEdit(e);auto& edit=edits.back();
  edit.portableShieldEdit=true;edit.portableShieldBefore=before;edit.portableShieldAfter=next;
  message("Shield properties applied.");
 }else if(!strcmp(action,"mounted_charge")){
  using namespace jar::chargeCopy;
  Properties before,next;
  const auto* support=G_SandboxChargeSupport(e);
  float health,damage,radius,remaining;
  if(!contextChargeProperties(e,before,support)||Detonating(before.mode)||
     !jar::context::Number(gi.argv(4),health,1,maxEditHealth)||health!=std::floor(health)||
     !jar::context::Number(gi.argv(5),damage,0,maxEditDamage)||damage!=std::floor(damage)||
     !jar::context::Number(gi.argv(6),radius,0,maxEditRadius)||radius!=std::floor(radius)||
     !jar::context::Number(gi.argv(7),remaining,0,before.mode==Mode::Proximity?maxEditTimeout:0)||
     remaining!=std::floor(remaining)||int64_t(level.time)+int(remaining)>INT_MAX){
   message("Use whole values within the charge settings limits. Zero time keeps the current expiry.");return;
  }
  next=before;next.health=int(health);next.splash=int(damage);next.radius=int(radius);
  if(remaining)next.expiry=level.time+int(remaining);
  if(Same(before,next))return;
  if(!G_SandboxChargeSetProperties(e,next)){message("The charge is already expiring or detonating.");return;}
  rememberEdit(e);auto& edit=edits.back();
  edit.mountedChargeEdit=true;edit.mountedChargeBefore=before;edit.mountedChargeAfter=next;
  if(support){
   edit.mountedChargeSupport=support->s.number;
   edit.mountedChargeSupportGeneration=entityGeneration[edit.mountedChargeSupport];
  }
  message("Charge properties applied.");
 }else if(!strcmp(action,"pickup_lifetime")){
  float value;
  if(!pickupLifetimeEditable(e)||!jar::context::Number(gi.argv(4),value,0,jar::pickupLifetime::maxMs)||value!=std::floor(value)){
   message("Select an eligible dropped pickup and enter 0 to 600000 milliseconds.");return;
  }
  const int before=pickupLifetimeRemaining(e);
  if(before==int(value))return;
  rememberEdit(e);auto& edit=edits.back();
  edit.pickupLifetimeItem=e.item;edit.pickupLifetimeBefore=before;edit.pickupLifetimeAfter=int(value);
  edit.pickupLifetimeEditClock=pickupLifetimeClock(e);
  pickupLifetimeSet(e,int(value));message("Pickup lifetime applied.");
 }else if(!strcmp(action,"pickup_quantity")){
  float value;
  const bool duration=e.item&&e.item->giType==IT_POWERUP;
  if(!G_WorkshopPickupQuantityEditable(e)||!jar::context::Number(gi.argv(4),value,0,jar::pickupQuantity::maximum)||value!=std::floor(value)){
   message(duration?"Select a spawned Force pickup and enter whole seconds from 0 to 1000.":
    "Select a spawned pickup and enter a whole quantity from 0 to 1000.");return;
  }
  if(e.count==int(value))return;
  rememberEdit(e);auto& edit=edits.back();
  edit.pickupQuantityItem=e.item;edit.pickupQuantityBefore=e.count;edit.pickupQuantityAfter=int(value);
  e.count=int(value);message(duration?"Force pickup duration applied.":"Pickup quantity applied.");
 }else if(!strcmp(action,"holocron")){
  float value;if(!G_HolocronEditable(e)||!jar::context::Number(gi.argv(4),value,1,NUM_FORCE_POWER_LEVELS-1)||value!=std::floor(value)){
   message("Select a holocron and choose a whole Force level from 1 to 3.");return;
  }
  if(e.count==int(value))return;rememberEdit(e);edits.back().holocronEdit=true;edits.back().holocronLevel=e.count;edits.back().holocronPower=e.item->giTag;e.count=int(value);
  message("Holocron level applied. Pickup grants this level without lowering a known power; Revert property restores the setting.");
 }else if(!strcmp(action,"position")||!strcmp(action,"angles")){
  const bool position=!strcmp(action,"position");vec3_t value;
  for(int axis=0;axis<3;++axis)if(!jar::context::Number(gi.argv(4+axis),value[axis],position?-65535.f:-360.f,position?65535.f:360.f)){message("Invalid transform. Position: +/-65535; angles: +/-360 degrees.");return;}
  auto*o=owned(e.s.number);
  if(Constrained(state,attachmentKey(e))){message("Unlink before setting an exact pose. Physgun rotation respects attached joints.");return;}
  if(e.m_pVehicle){
   const auto control=nativeControls.find(e.s.number);
   if(control==nativeControls.end()||!control->second.frozen||held==unsigned(e.s.number)){
    message("Freeze the vehicle before editing its position or orientation.");return;
   }
   VehicleTeleportPlan plan;
   if(!nativeVehiclePose(e,position?value:e.currentOrigin,position?e.currentAngles:value,level.time,plan,false)){
    message("The vehicle or a seat is obstructed or unavailable. Pose unchanged.");return;
   }
   rememberEdit(e);edits.back().vehiclePoseEdit=true;
   nativeCommitVehiclePose(plan);
   VectorCopy(e.currentOrigin,edits.back().appliedVehicleOrigin);
   VectorCopy(e.currentAngles,edits.back().appliedVehicleAngles);
   message("Vehicle pose applied. Revert property restores the previous pose.");
   contextPublish();return;
  }
  if(e.client&&e.s.m_iVehicleNum){message("Select the vehicle to move its rider with it.");return;}
  if(occupied(e)&&!nativeControls.count(e.s.number)){message("Exit this vehicle or native control before setting its pose.");return;}
  if(G_JAR_PushableMover(e)){
   if(held==unsigned(e.s.number)){message("Release the Physgun before editing this mover's pose.");return;}
   if(VectorCompare(value,position?e.currentOrigin:e.currentAngles))return;
   Edit previous=capturePropertyEdit(e);
   if(!contextMoveMover(e,position?value:e.currentOrigin,position?e.currentAngles:value)){
    message("Mover or rider path is blocked. Pose unchanged.");return;
   }
   rememberAction(std::move(previous));
   message("Mover pose applied. Revert property restores the previous pose.");
   contextPublish();return;
  }
  std::vector<SandboxChargeMove> charges;
  if(const auto* model=editedProp(e)){
   if(!propTransformClear(e,*model,e.s.frame,position?value:e.currentOrigin,position?e.currentAngles:value,charges,false)){
    message("Pose is blocked. Clear space around the prop and its mounted charges.");return;
   }
  }else if(G_SandboxSlidePortable(e)){
   if(!G_SandboxSlideClear(e,position?value:e.currentOrigin,position?e.currentAngles:value)){
    message(position?"Destination is blocked.":"Rotation is blocked.");return;
   }
  }else if(G_ButtonPortable(e)){
   if(!G_ButtonClear(e,position?value:e.currentOrigin,position?e.currentAngles:value)){
    message(position?"Button destination is blocked.":"Button rotation is blocked.");return;
   }
  }else if(position&&!jar::volumeEntity::Portable(e)){BSPSectionMotionScope sectionMotion(e);
   if(!sectionMotion){message("Clear the section's moving brushes and riders before repositioning it.");return;}
   vec3_t lo,hi;VectorCopy(e.mins,lo);VectorCopy(e.maxs,hi);if(jar::bspSection::Portable(e))jar::bspSection::Hull(e.mins,e.maxs,e.currentAngles,lo,hi);
   trace_t tr{};gi.trace(&tr,value,lo,hi,value,e.s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);
   if(tr.startsolid||tr.allsolid){message("Destination is blocked; position unchanged.");return;}}
  if(!position&&G_WallPortable(e)&&e.count&&!G_WallClear(e,e.currentOrigin,value)){message("Wall rotation is blocked.");return;}
  RackStockPlan rack;
  if(!rackStockPose(e,position?value:e.currentOrigin,position?e.currentAngles:value,false,rack)){
   message("Rack stock cannot fit at that pose.");return;
  }
  releaseHeld();rememberEdit(e);
  const bool native=!(o&&prop(o->catalog));const bool controlled=nativeControls.count(e.s.number)!=0;
  if(native&&!controlled)nativeBegin(e,true);
  if(position){G_SetOrigin(&e,value);if(e.client)VectorCopy(value,e.client->ps.origin);}
  else {G_SetAngles(&e,value);if(e.client)SetClientViewAngle(&e,value);}
  setupMoved(e);if(native&&!controlled)nativeRelease(e.s.number,false);
  if(o&&prop(o->catalog))physicsInvalidate(e.s.number);gi.linkentity(&e);placeEditedCharges(charges);commitRackStock(rack);
  message(position?"Position applied. Revert property restores the previous pose.":"Orientation applied. Revert property restores the previous pose.");
 }else if(!strncmp(action,"teleport_",9)){
  using namespace jar::teleport;
  Settings before;
  if(!owned(e.s.number)||!G_TeleportRead(e,before))return;
  if(!strcmp(action,"teleport_unlink")){
   if(!e.target)return;
   editTeleportLink(e,nullptr);message("Teleport destination cleared.");
  }else{
   auto after=before;
   if(!strcmp(action,"teleport_apply")){
    float interval;
    if(!jar::context::Number(gi.argv(4),interval,minInterval,maxInterval)||interval!=std::floor(interval)){
     message("Enter the interval in whole milliseconds.");return;
    }
    after.interval=int(interval);
   }else if(!strcmp(action,"teleport_subject"))after.subject=(after.subject+1)%(after.kind==Volume?int(SubjectCount):int(NoActors));
   else if(!strcmp(action,"teleport_view"))after.keepView=!after.keepView;
   else if(!strcmp(action,"teleport_velocity"))after.keepVelocity=!after.keepVelocity;
   else if(!strcmp(action,"teleport_props")&&after.kind==Volume)after.props=!after.props;
   else if(!strcmp(action,"teleport_projectiles")&&after.kind==Volume)after.projectiles=!after.projectiles;
   else if(!strcmp(action,"teleport_vehicles"))after.vehicles=!after.vehicles;
   else if(!strcmp(action,"teleport_assemblies")&&after.kind==Volume)after.assemblies=!after.assemblies;
   else if(!strcmp(action,"teleport_frozen_props")&&after.kind==Volume)after.frozenProps=!after.frozenProps;
   else if(!strcmp(action,"teleport_movers")&&after.kind==Volume)after.movers=!after.movers;
   else if(!strcmp(action,"teleport_world_anchors")&&after.kind==Volume)after.worldAnchors=!after.worldAnchors;
   else if(!strcmp(action,"teleport_enabled"))after.enabled=!after.enabled;
   else return;
   if(Same(before,after)||!G_TeleportSet(e,after))return;
   rememberEdit(e);edits.back().teleportBefore=before;edits.back().teleportAfter=after;
   message("Teleport settings updated.");
  }
 }else if(!strncmp(action,"push_",5)){
  using namespace jar::push;
  Settings before;
  if(!owned(e.s.number)||!G_PushRead(e,before))return;
  if(!strcmp(action,"push_unlink")){
   if(!e.target)return;
   editPushAimLink(e,nullptr);message("Push aim target cleared.");
  }else{
   auto after=before;
   if(!strcmp(action,"push_apply")){
    float speed,interval;
    if(!jar::context::Number(gi.argv(4),speed,1,maxSpeed)||speed!=std::floor(speed)||
       !jar::context::Number(gi.argv(5),interval,minInterval,maxInterval)||interval!=std::floor(interval)){
     message("Enter speed and interval as whole numbers within range.");return;
    }
    after.speed=int(speed);after.interval=int(interval);
   }else if(!strcmp(action,"push_aim"))after.aim=(after.aim+1)%AimCount;
   else if(!strcmp(action,"push_subject"))after.subject=(after.subject+1)%(after.kind==Volume?int(SubjectCount):int(Props));
   else if(!strcmp(action,"push_add"))after.additive=!after.additive;
   else if(!strcmp(action,"push_vehicles"))after.vehicles=!after.vehicles;
   else if(!strcmp(action,"push_enabled"))after.enabled=!after.enabled;
   else return;
   if(Same(before,after)||!G_PushSet(e,after))return;
   rememberEdit(e);edits.back().pushBefore=before;edits.back().pushAfter=after;
   message("Push settings updated.");
  }
 }else if(!strncmp(action,"contact_",8)){
  using namespace jar::contactTrigger;
  Settings before;
  if(!owned(e.s.number)||!G_ContactRead(e,before))return;
  if(!strcmp(action,"contact_rearm")){
   if(!G_ContactSpent(e))return;
   rememberEdit(e);edits.back().contactRearmed=true;
   G_ContactSetSpent(e,false);
   message("Once trigger rearmed.");
  }else{
   auto next=before;
   if(!strcmp(action,"contact_hurt_apply")&&before.kind==Hurt){
    float damage,interval;
    if(!jar::context::Number(gi.argv(4),damage,1,maxDamage)||damage!=std::floor(damage)||
       !jar::context::Number(gi.argv(5),interval,minInterval,maxTime)||interval!=std::floor(interval)){
     message("Enter damage and interval as whole numbers within range.");return;
    }
    next.damage=int(damage);next.interval=int(interval);
   }else if(!strcmp(action,"contact_hurt_arm")&&before.kind==Hurt){
    next.armed=!next.armed;
   }else if(!strcmp(action,"contact_hurt_mode")&&before.kind==Hurt){
    next.mode=(next.mode+1)%DamageModeCount;
   }else if(!strcmp(action,"contact_apply")&&before.kind!=Hurt){
    float delay,interval=0,variation=0;
    if(!jar::context::Number(gi.argv(4),delay,0,maxTime)||delay!=std::floor(delay)||
       (before.kind==Repeat&&(!jar::context::Number(gi.argv(5),interval,minInterval,maxTime)||interval!=std::floor(interval)||
        !jar::context::Number(gi.argv(6),variation,0,maxTime)||variation!=std::floor(variation)))){
     message("Enter timing in whole milliseconds.");return;
    }
    next.delay=int(delay);next.interval=int(interval);next.variation=int(variation);
   }else if(!strcmp(action,"contact_subject"))next.subject=(next.subject+1)%SubjectCount;
   else if(!strcmp(action,"contact_use"))next.useRequired=!next.useRequired;
   else if(!strcmp(action,"contact_enabled"))next.enabled=!next.enabled;
   else return;
   if(!Valid(next)){message("Variation must keep the repeat interval between 100 and 60000 ms.");return;}
   if(Same(before,next)||!G_ContactSettings(e,next))return;
   rememberEdit(e);edits.back().contactBefore=before;edits.back().contactAfter=next;
   message("Contact trigger updated.");
  }
 }else if(!strcmp(action,"logic_running")){
  if(!owned(e.s.number)||G_LogicKind(e)!=jar::logic::Timer||e.e_UseFunc!=useF_func_timer_use)return;
  if(e.nextthink>0){G_LogicTimerStop(e);message("Timer stopped.");}
  else if(!(e.svFlags&SVF_INACTIVE))tool("activate");
  else return;
 }else if(!strcmp(action,"logic_clear_output")){
  uint32_t slot=0;
  if(!owned(e.s.number)||!jar::logic::Slotted(G_LogicKind(e))||
     !jar::context::Token(gi.argv(4),slot)||slot>jar::logic::outputCount)return;
  editSlottedLink(e,int(slot)-1,nullptr);
  message(va("Link %u cleared.",slot));
 }else if(!strcmp(action,"logic_apply")||!strcmp(action,"logic_reset")||
          !strcmp(action,"logic_repeat")||!strcmp(action,"logic_enabled")||!strcmp(action,"logic_kill_mode")){
  using namespace jar::logic;
  jar::logic::State before;
  if(!owned(e.s.number)||!G_LogicRead(e,before))return;
  auto next=before;
  if(!strcmp(action,"logic_apply")){
   float first,second=0;
   if(Slotted(before.kind)||before.kind==Kill)return;
   const bool timing=before.kind==Delay||before.kind==Timer||before.kind==Relay;
   const int minimum=before.kind==Timer?minInterval:timing?0:before.kind==Score?-maxPoints:1;
   const int maximum=timing?maxDelay:before.kind==Score?maxPoints:maxHits;
   if(!jar::context::Number(gi.argv(4),first,minimum,maximum)||first!=std::floor(first)||
      (timing&&(!jar::context::Number(gi.argv(5),second,0,maxDelay)||second!=std::floor(second)))){
    message(timing?"Check the whole millisecond values.":before.kind==Score?
     "Use whole points from -10000 to 10000.":"Use a whole hit count from 1 to 1000.");return;
   }
   if(timing){
    next.delayMs=int(first);
    if(before.kind==Relay)next.cooldownMs=int(second);
    else next.jitterMs=int(second);
   }else if(before.kind==Score)next.points=int(first);
   else if(next.hits!=int(first)){next.hits=int(first);next.remaining=next.hits;}
  }else if(!strcmp(action,"logic_enabled"))next.enabled=!before.enabled;
  else if(!strcmp(action,"logic_kill_mode")){
   if(before.kind!=Kill)return;
   next.killMode=(next.killMode+1)%KillModes;
  }else if(before.kind==Counter||OneShotMode(before.kind)){
   if(!strcmp(action,"logic_reset"))next.remaining=before.kind==Counter?next.hits:1;
   else {
    next.repeat=!before.repeat;
    if(OneShotMode(before.kind))next.remaining=1;
   }
  }else return;
  if(!Valid(next)){
   message(before.kind==Timer?"Variation must keep the interval between 100 and 60000 ms.":
    "Variation must keep the delay between 0 and 60000 ms.");return;
  }
  if(SameSettings(before,next)&&before.remaining==next.remaining)return;
  if(!G_LogicSet(e,next))return;
  rememberEdit(e);edits.back().logicBefore=before;edits.back().logicAfter=next;
  message("Logic marker updated.");
 }else if(!strcmp(action,"movement_gravity")||!strcmp(action,"movement_friction")||
          !strcmp(action,"movement_map")||!strcmp(action,"movement_enabled")){
  using namespace jar::movement;
  const Kind property=!strcmp(action,"movement_gravity")?Gravity:!strcmp(action,"movement_friction")?Friction:None;
  const bool toggle=!strcmp(action,"movement_enabled");
  float value=0;
  if(property!=None&&(!jar::context::Number(gi.argv(4),value,0,property==Gravity?maxGravity:maxFriction)||
                     value!=std::floor(value))){
   message(property==Gravity?"Enter whole gravity from 0 to 3200.":"Enter whole ground friction from 0 to 10.");return;
  }
  Marker marker;
  ActorState actor;
  if(owned(e.s.number)&&G_MovementMarkerRead(e,marker)){
   auto next=marker;
   if(toggle)next.enabled=!marker.enabled;
   else if(property==marker.kind)next.value=int(value);
   else return;
   if(Same(marker,next)||!G_MovementMarkerSet(e,next))return;
   rememberEdit(e);edits.back().movementMarkerBefore=marker;edits.back().movementMarkerAfter=next;
   message("Movement marker updated. Activate it to apply the value.");
  }else if(!toggle&&G_MovementActorRead(e,actor)){
   auto next=actor;
   if(property==Gravity){next.gravity=int(value);next.customGravity=true;}
   else if(property==Friction)next.friction=int(value);
   else next.customGravity=false;
   if(Same(actor,next)||!G_MovementActorSet(e,next))return;
   rememberEdit(e);edits.back().actorMovementEdit=true;edits.back().movementBefore=actor;edits.back().movementAfter=next;
   message("Actor movement updated.");
  }else return;
 }else if(!strcmp(action,"bobbing")||!strcmp(action,"bobbing_running")||!strcmp(action,"bobbing_enabled")){
  jarBobState_t before{};
  if(!owned(e.s.number)||!G_BobbingRead(e,before))return;
  auto next=before;
  if(!strcmp(action,"bobbing")){
   float amplitude,period;
   if(!jar::context::Number(gi.argv(4),amplitude,JAR_BOB_MIN_AMPLITUDE,JAR_BOB_MAX_AMPLITUDE)||
      !jar::context::Number(gi.argv(5),period,JAR_BOB_MIN_PERIOD,JAR_BOB_MAX_PERIOD)||
      amplitude!=std::floor(amplitude)||period!=std::floor(period)){
    message("Use whole values. Amplitude: 1-128 units. Cycle: 500-30000 ms.");return;
   }
   next.amplitude=int(amplitude);next.periodMs=int(period);
  }else if(!strcmp(action,"bobbing_running"))next.running=!before.running;
  else next.inactive=!before.inactive;
  if(BobbingSameSettings(before,next))return;
  if(!G_BobbingSettings(e,next))return;
  rememberEdit(e);edits.back().bobbingEdit=true;edits.back().bobbingBefore=before;edits.back().bobbingAfter=next;
  message("Platform settings applied.");
 }else if(!strcmp(action,"button")||!strcmp(action,"button_enabled")){
  jarButtonState_t previous;
  if(!G_ButtonRead(e,previous))return;
  const bool enabled=!(e.svFlags&SVF_INACTIVE);
  const bool toggle=!strcmp(action,"button_enabled");
  float delay=previous.waitMs;
  if(!toggle&&(!jar::context::Number(gi.argv(4),delay,0,JAR_BUTTON_MAX_WAIT)||delay!=std::floor(delay))){
   message("Enter a whole button hold delay from 0 to 30000 ms.");return;
  }
  if(!G_ButtonSettings(e,(int)delay,toggle?!enabled:enabled))return;
  rememberEdit(e);edits.back().buttonWait=previous.waitMs;edits.back().buttonEnabled=enabled;
  message("Button settings applied.");
 }else if(!strcmp(action,"slide_enabled")){
  jar::slidingDoor::State before;
  if(!G_SandboxSlideRead(e,before))return;
  rememberEdit(e);edits.back().slidingDoorEdit=true;edits.back().slidingDoor=before.settings;
  edits.back().slidingDoorInactive=before.inactive;
  e.svFlags^=SVF_INACTIVE;e.s.loopSound=0;
  message(e.svFlags&SVF_INACTIVE?"Movement disabled.":"Movement enabled.");
 }else if(!strcmp(action,"slide")){
  jar::slidingDoor::State before;
  if(!G_SandboxSlideRead(e,before))return;
  const int limits[][2]={{JAR_SLIDE_MIN_TRAVEL,JAR_SLIDE_MAX_TRAVEL},{JAR_SLIDE_MIN_SPEED,JAR_SLIDE_MAX_SPEED},
                       {0,JAR_SLIDE_DIRECTIONS-1},{0,JAR_SLIDE_MAX_WAIT}};
  int values[4];
  for(int i=0;i<4;++i){
   float value;
   if(!jar::context::Number(gi.argv(i+4),value,limits[i][0],limits[i][1])||value!=std::floor(value)){
    message("Enter whole travel, speed, direction and delay values within range.");return;
   }
   values[i]=(int)value;
  }
  if(!G_SandboxSlideSettings(e,{values[0],values[1],values[2],values[3]})){
   message(G_LiftPortable(e)?"Lower the lift before shortening its travel past the current height.":"Close the door to change direction or shorten travel past its current position.");return;
  }
  rememberEdit(e);edits.back().slidingDoorEdit=true;edits.back().slidingDoor=before.settings;
  edits.back().slidingDoorInactive=before.inactive;
  message(G_LiftPortable(e)?"Lift settings applied.":"Sliding door settings applied.");
 }else if(!strcmp(action,"wall_visible")||!strcmp(action,"wall_enabled")){
  if(!G_WallPortable(e))return;
  rememberEdit(e);
  if(!strcmp(action,"wall_visible")){
   edits.back().wallVisibility=e.count?1:0;
   if(!G_WallSetVisible(e,!e.count)){edits.pop_back();message("Wall is blocked. Clear the space and try again.");return;}
  }else{
   edits.back().wallEnabledEdit=true;edits.back().wallWasInactive=(e.svFlags&SVF_INACTIVE)!=0;
   e.svFlags^=SVF_INACTIVE;
  }
  message("Wall property applied.");
 }else if(!strcmp(action,"distance")||!strcmp(action,"distance_subjects")||!strcmp(action,"distance_arm")||
          !strcmp(action,"distance_clear_reference")||!strcmp(action,"distance_clear_subject")){
  if(!G_DistancePortable(e))return;
  if(!strcmp(action,"distance_arm")){
   rememberEdit(e);edits.back().distanceArmEdit=true;edits.back().distanceWasInactive=(e.svFlags&SVF_INACTIVE)!=0;
   e.svFlags^=SVF_INACTIVE;
   message(e.svFlags&SVF_INACTIVE?"Distance trigger disarmed.":"Distance trigger armed. Activate to measure.");
  }else if(!strncmp(action,"distance_clear_",15)){
   const bool reference=!strcmp(action,"distance_clear_reference");
   distanceLink(e,reference?JAR_DISTANCE_REFERENCE:JAR_DISTANCE_SUBJECT,nullptr);
   message(reference?"This marker is now the distance reference.":"NPC link cleared. Link a new subject or choose Player.");
  }else{
   int range=e.count,subjects=e.spawnflags;
   if(!strcmp(action,"distance_subjects"))subjects=subjects%3+1;
   else{
    float value;
    if(!jar::context::Number(gi.argv(4),value,JAR_DISTANCE_MIN,JAR_DISTANCE_MAX)||value!=std::floor(value)){
     message("Enter a whole distance from 1 to 4096 units.");return;
    }
    range=int(value);
   }
   if(!JAR_DistanceSettingsValid(range,subjects)||(range==e.count&&subjects==e.spawnflags))return;
   rememberEdit(e);edits.back().distanceRange=e.count;edits.back().distanceSubjects=e.spawnflags;
   e.count=range;e.spawnflags=subjects;message("Distance settings applied.");
  }
 }else if(!strcmp(action,"visible")||!strcmp(action,"visible_walls")||!strcmp(action,"visible_sight")||!strcmp(action,"visible_enabled")){
  if(!G_VisiblePortable(e))return;
  const auto before=G_VisibleSettings(e);
  auto next=before;
  const bool toggleEnabled=!strcmp(action,"visible_enabled");
  if(!strcmp(action,"visible")){
   float range,cone;
   if(!jar::context::Number(gi.argv(4),range,JAR_VISIBLE_RANGE_MIN,JAR_VISIBLE_RANGE_MAX)||range!=std::floor(range)||
      !jar::context::Number(gi.argv(5),cone,JAR_VISIBLE_CONE_MIN,JAR_VISIBLE_CONE_MAX)||cone!=std::floor(cone)){
    message("Enter a whole range from 32 to 4096 and half-angle from 5 to 85 degrees.");return;
   }
   next.range=int(range);next.cone=int(cone);
  }else if(!toggleEnabled)next.flags^=!strcmp(action,"visible_walls")?JAR_VISIBLE_IGNORE_WALLS:JAR_VISIBLE_FORCE_SIGHT;
  if(!toggleEnabled&&next.range==before.range&&next.cone==before.cone&&next.flags==before.flags)return;
  rememberEdit(e);edits.back().visibleEdit=true;edits.back().visible=before;
  edits.back().visibleInactive=(e.svFlags&SVF_INACTIVE)!=0;
  G_VisibleSet(e,next);if(toggleEnabled)e.svFlags^=SVF_INACTIVE;
  message("Sight settings applied. Activate rearms a spent trigger.");
 }else if(!strcmp(action,"always_delay")){
  float delay;
  if(!G_AlwaysPortable(e)||!jar::context::Number(gi.argv(4),delay,JAR_ALWAYS_MIN_MS,JAR_ALWAYS_MAX_MS)||delay!=std::floor(delay)){
   message("Select a one-shot trigger and enter a whole delay from 50 to 60000 ms.");return;
  }
  if(e.delay==int(delay))return;
  rememberEdit(e);edits.back().alwaysDelay=e.delay;e.delay=int(delay);
  message("Delay saved for the next activation. Revert property restores the previous delay.");
 }else if(!strcmp(action,"space_armed")||!strcmp(action,"space_enabled")){
  if(!G_SpacePortable(e)){message("Select a portable Space volume first.");return;}
  const bool armed=(e.contents&CONTENTS_TRIGGER)!=0,enabled=(e.svFlags&SVF_INACTIVE)==0;
  const bool toggleArmed=!strcmp(action,"space_armed");
  rememberEdit(e);
  auto&edit=edits.back();edit.spaceEdit=true;edit.spaceArmed=armed;edit.spaceEnabled=enabled;
  G_SpaceSet(e,toggleArmed?!armed:armed,toggleArmed?enabled:!enabled);
  message("Space volume activation updated.");
 }else if(!strcmp(action,"volume")){
  vec3_t value,previous;if(!jar::volumeEntity::Dimensions(e,previous)){message("Dimensions are available on portable trigger volumes.");return;}
  for(int axis=0;axis<3;++axis)if(!jar::context::Number(gi.argv(4+axis),value[axis],jar::volume::minimum,jar::volume::maximum)){message("Volume dimensions must be numbers from 8 to 8192 units.");return;}
  // Validate the proposed local bounds before recording an undo transaction.
  vec3_t lo,hi;if(!jar::volumeEntity::ResizedBounds(e,value,lo,hi)){message("Dimensions exceed the supported local volume bounds.");return;}
  releaseHeld();rememberEdit(e);edits.back().volumeEdit=true;jar::volumeEntity::Resize(e,value);gi.linkentity(&e);
  message("Volume dimensions applied. Revert property restores its previous size.");
 }else if(!strcmp(action,"angular_apply")||!strcmp(action,"angular_axis")||
          !strcmp(action,"angular_running")||!strcmp(action,"angular_enabled")){
  using namespace jar::sandboxAngular;
  Settings before;
  if(!owned(e.s.number)||!G_SandboxAngularRead(e,before))return;
  auto after=before;
  if(!strcmp(action,"angular_axis"))after.axis=(before.axis+1)%AxisCount;
  else if(!strcmp(action,"angular_running"))after.running=!before.running;
  else if(!strcmp(action,"angular_enabled"))after.enabled=!before.enabled;
  else {
   const bool swing=before.kind==Pendulum;
   float rate,period=2000;
   if(!jar::context::Number(gi.argv(4),rate,swing?1:-maxSpeed,swing?maxSwing:maxSpeed)||rate!=std::floor(rate)||!rate||
      (swing&&(!jar::context::Number(gi.argv(5),period,minPeriod,maxPeriod)||period!=std::floor(period)))){
    message("Use a nonzero speed within +/-180 degrees/s, or a 1-75 degree swing with a 750-30000 ms period.");return;
   }
   after.rate=int(rate);after.period=int(period);
   after.phase=int(int64_t(before.phase)*after.period/before.period);
  }
  if(!SameControls(before,after)&&G_SandboxAngularSet(e,after)){
   rememberEdit(e);
   auto& edit=edits.back();edit.angularBefore=before;edit.angularAfter=after;
   message("Motion settings applied at the current pose.");
  }
 }else if(!strcmp(action,"speaker_apply")||!strcmp(action,"speaker_prev")||!strcmp(action,"speaker_next")||
          !strcmp(action,"speaker_mode")||!strcmp(action,"speaker_enabled")){
  using namespace jar::speaker;
  Settings before;
  if(!owned(e.s.number)||!G_SandboxSpeakerRead(e,before))return;
  auto after=before;
  if(!strcmp(action,"speaker_mode")){
   after.mode=(before.mode+1)%ModeCount;
   after.active=false;after.remaining=0;
   if(after.mode==Loop&&!cues[after.cue].loop)after.cue=defaultLoopCue;
  }else if(!strcmp(action,"speaker_enabled"))after.enabled=!before.enabled;
  else if(!strcmp(action,"speaker_prev")||!strcmp(action,"speaker_next"))
   after.cue=CycleCue(before.cue,!strcmp(action,"speaker_prev")?-1:1,before.mode);
  else {
   float period,variation;
   if(!jar::context::Number(gi.argv(4),period,minDelay,maxPeriod)||period!=std::floor(period)||
      !jar::context::Number(gi.argv(5),variation,0,period-minDelay)||variation!=std::floor(variation)){
    message("Repeat requires 500 to 60000 ms and a minimum gap of 500 ms after variation.");return;
   }
   after.period=int(period);after.variation=int(variation);
   after.remaining=std::min(after.remaining,after.period+after.variation);
  }
  if(!SameControl(before,after)){
   if(!G_SandboxSpeakerSet(e,after)){message("No sound slot is available for this cue.");return;}
   rememberEdit(e);
   auto& edit=edits.back();edit.speakerEdit=true;edit.speakerBefore=before;edit.speakerAfter=after;
   message("Speaker updated. Use or Activate to play the selected cue.");
  }
 }else if(!strcmp(action,"music")||!strcmp(action,"music_prev")||!strcmp(action,"music_next")){
  std::string text,normalized;if(!jar::music::Class(e.classname))return;
  if(!strcmp(action,"music")){if(!Unhex(gi.argv(4),text,jar::music::textLimit)||!jar::music::Valid(text)){message("Choose one retail music path, or an intro path followed by a loop path.");return;}}
  else text=jar::music::Cycle(e.message,!strcmp(action,"music_prev")?-1:1);
  jar::music::Canonical(text.c_str(),normalized);if(e.message&&normalized==e.message)return;
  if(!e.message||!jar::music::Valid(e.message)){message("This authored cue is outside the retail editor catalog; its native playback is retained.");return;}
  rememberEdit(e);edits.back().musicEdit=true;edits.back().music=e.message;
  if(!G_MusicSetTrack(e,normalized.c_str())){edits.pop_back();return;}
  message("Music track applied. Activate toggles playback; Revert property restores the previous track.");
 }else if(!strcmp(action,"weather")){
  jar::weather::Settings before,next;float mode,on;
  if(!G_WeatherRead(e,before)||!jar::context::Number(gi.argv(4),mode,0,3)||mode!=std::floor(mode)||!jar::context::Number(gi.argv(5),next.density,.1f,4)||!jar::context::Number(gi.argv(6),next.size,.1f,8)||!jar::context::Number(gi.argv(7),next.alpha,0,1)||!jar::context::Number(gi.argv(8),next.speed,0,5000)||!jar::context::Number(gi.argv(9),on,0,1)||on!=std::floor(on)){
   message("Weather density 0.1-4, size 0.1-8, opacity 0-1, wind speed 0-5000; choose a valid mode and On/Off.");return;
  }
  next.kind=before.kind;next.mode=int(mode);next.enabled=on!=0;if(!jar::weather::Valid(next)){message("Mode is not available for this weather type.");return;}
  rememberEdit(e);edits.back().weatherEdit=true;edits.back().weather=before;edits.back().weatherOrder=e.delay;G_WeatherSet(e,next);
  message(next.kind==jar::weather::WindZone?"Local wind applied. Rotate to steer; resize its volume in Properties.":next.kind==jar::weather::Exposure?"Weather boundary applied. Latest overlapping boundary takes priority; removal restores the map rules.":"Weather applied. Latest enabled controller of each type takes priority; authored weather remains.");
 }else if(!strcmp(action,"crystal")){
  jar::crystalCrate::Settings before,next;float hit,blast,solid;
  if(!G_CrystalCrateRead(e,before)||!jar::context::Number(gi.argv(4),hit,0,1000)||hit!=std::floor(hit)||!jar::context::Number(gi.argv(5),next.hitRadius,1,1024)||!jar::context::Number(gi.argv(6),blast,0,1000)||blast!=std::floor(blast)||!jar::context::Number(gi.argv(7),next.blastRadius,1,1024)||next.blastRadius!=std::floor(next.blastRadius)||!jar::context::Number(gi.argv(8),solid,0,1)||solid!=std::floor(solid)){message("Crystal crate: whole damage 0-1000, radii 1-1024 (whole blast radius), and Solid/Shot-only.");return;}
  next.hitDamage=int(hit);next.blastDamage=int(blast);next.solid=solid!=0;rememberEdit(e);edits.back().crystalEdit=true;edits.back().crystal=before;
  if(!G_CrystalCrateSet(e,next)){edits.pop_back();message("Solidity is obstructed; move the crate clear first.");return;}
  message("Crystal-crate properties applied. Activate detonates it; Revert property restores properties before destruction.");
 }else if(!strcmp(action,"door")){
  jar::doorCamera::Settings before,next;float extended;
  if(!G_DoorCameraRead(e,before)||!jar::context::Number(gi.argv(4),next.travel,8,256)||!jar::context::Number(gi.argv(5),next.speed,1,512)||!jar::context::Number(gi.argv(6),extended,0,1)||extended!=std::floor(extended)){message("Door camera: travel 8-256, speed 1-512 and Extended/Retracted.");return;}
  next.extended=extended!=0;rememberEdit(e);edits.back().doorCameraEdit=true;edits.back().doorCamera=before;G_DoorCameraSet(e,next);
  message("Door-camera properties applied. Activate toggles extension; blocked motion retries until clear. Revert property restores properties.");
 }else if(!strcmp(action,"welder")){
  jar::welder::Settings before,next;float damage,on;
  if(!G_WelderRead(e,before)||!jar::context::Number(gi.argv(4),next.reach,1,256)||!jar::context::Number(gi.argv(5),next.radius,1,256)||!jar::context::Number(gi.argv(6),next.interval,.1f,10)||!jar::context::Number(gi.argv(7),damage,0,1000)||damage!=std::floor(damage)||!jar::context::Number(gi.argv(8),on,0,1)||on!=std::floor(on)){message("Welder: reach/radius 1-256, interval 0.1-10 seconds, whole damage 0-1000 and On/Off.");return;}
  next.damage=int(damage);next.enabled=on!=0;rememberEdit(e);edits.back().welderEdit=true;edits.back().welder=before;G_WelderSet(e,next);
  message("Welder properties applied. Zero damage keeps sparks and sound. Activate toggles; Revert property restores properties.");
 }else if(!strcmp(action,"spot")){
  jar::spotlight::Settings before,next;float tracking,on;
  if(!G_SpotlightRead(e,before)||!jar::context::Number(gi.argv(4),next.range,64,8192)||!jar::context::Number(gi.argv(5),next.cone,1,30)||!jar::context::Number(gi.argv(6),next.light,0,1024)||!jar::context::Number(gi.argv(10),tracking,0,1)||tracking!=std::floor(tracking)||!jar::context::Number(gi.argv(11),on,0,1)||on!=std::floor(on)){message("Spotlight: range 64-8192, cone angle 1-30, light radius 0-1024, RGB 0-1; use Tracking/On choices.");return;}
  for(int i=0;i<3;++i)if(!jar::context::Number(gi.argv(7+i),next.rgb[i],0,1)){message("Spotlight colors must be 0-1.");return;}
  next.tracking=tracking!=0;next.enabled=on!=0;rememberEdit(e);edits.back().spotEdit=true;edits.back().spot=before;G_SpotlightSet(e,next);
  message("Spotlight properties applied. Primary Link aims; Secondary Link selects the detection output.");
 }else if(!strcmp(action,"portal_unlink")){
  if(!G_PortalPortable(e)||!G_JAR_WorkshopOwns(&e)||!e.target||!*e.target)return;
  editPortalLink(e,nullptr);
  message(G_PortalCamera(e)?"Aim link cleared. The camera uses its angles.":"Camera link cleared. Link another camera to show its feed.");
 }else if(!strcmp(action,"portal")){
  jar::portal::Settings before,next;float mirror,on;
  if(!G_PortalRead(e,before)||!jar::context::Number(gi.argv(4),next.width,16,1024)||!jar::context::Number(gi.argv(5),next.height,16,1024)||!jar::context::Number(gi.argv(6),mirror,0,1)||mirror!=std::floor(mirror)||!jar::context::Number(gi.argv(7),on,0,1)||on!=std::floor(on)){
   message("Portal dimensions are 16-1024 units; use the supplied mode and On/Off choices.");return;
  }
  next.mirror=mirror!=0;next.enabled=on!=0;
  if(jar::portal::Same(before,next)||!G_PortalSet(e,next))return;
  rememberEdit(e);auto& edit=edits.back();edit.portalEdit=true;edit.portalBefore=before;edit.portalAfter=next;
  G_PortalFrame();
  message("Portal properties applied. Move its camera separately; Revert property restores properties.");
 }else if(!strcmp(action,"usable_apply")||!strcmp(action,"usable_mode")||!strcmp(action,"usable_damage")||
          !strcmp(action,"usable_direct")||!strcmp(action,"usable_solid")||!strcmp(action,"usable_rearm")||
          !strcmp(action,"usable_enabled")||!strcmp(action,"usable_unlink")){
  using namespace jar::sandboxUsable;
  Settings before;
  if(!owned(e.s.number)||!G_SandboxUsableRead(e,before))return;
  if(!strcmp(action,"usable_unlink")){
   const int slot=gi.cvar("jar_linkOutput","0",CVAR_ARCHIVE|CVAR_NORESTART)->integer==1?1:0;
   editSlottedLink(e,slot,nullptr);message("Usable output cleared.");
  }else{
   auto after=before;
   if(!strcmp(action,"usable_mode")){
    after.mode=before.mode==Toggle?Pulse:Toggle;after.pending=after.spent=false;after.remaining=0;
   }else if(!strcmp(action,"usable_damage"))after.damage=static_cast<Damage>((before.damage+1)%DamageCount);
   else if(!strcmp(action,"usable_direct"))after.direct=static_cast<DirectUse>((before.direct+1)%DirectUseCount);
   else if(!strcmp(action,"usable_solid")){
    after.solid=!before.solid&&!before.pending;after.pending=false;
    if(after.solid)after.health=after.capacity;
   }else if(!strcmp(action,"usable_rearm")){after.spent=false;after.remaining=0;after.health=after.capacity;}
   else if(!strcmp(action,"usable_enabled"))after.enabled=!before.enabled;
   else{
    float capacity,reuse;
    if(!jar::context::Number(gi.argv(4),capacity,0,maxHealth)||capacity!=std::floor(capacity)||
       !jar::context::Number(gi.argv(5),reuse,0,maxReuse)||reuse!=std::floor(reuse)){
     message("Enter whole health from 0 to 10000 and reuse from 0 to 60000 ms.");return;
    }
    after.capacity=int(capacity);after.reuse=int(reuse);
    after.health=before.capacity?std::min(after.capacity,before.health):after.capacity;
   }
   if(!Same(before,after)){
    if(!G_SandboxUsableSet(e,after)){message("Clear the space around the usable before making it solid.");return;}
    rememberEdit(e);
    auto& edit=edits.back();edit.usableEdit=true;edit.usableBefore=before;edit.usableAfter=after;
    message("Usable settings applied. Revert property restores the previous settings.");
   }
  }
 }else if(!strcmp(action,"breakable_apply")||!strcmp(action,"breakable_mode")||!strcmp(action,"breakable_material")||
          !strcmp(action,"breakable_use")||!strcmp(action,"breakable_enabled")||
          !strcmp(action,"breakable_explosion")||!strcmp(action,"breakable_repair")){
  using namespace jar::sandboxBreakable;
  Settings before;
  if(!owned(e.s.number)||!G_SandboxBreakableRead(e,before))return;
  auto after=before;
  if(!strcmp(action,"breakable_repair")){after.phase=Intact;after.remaining=0;after.health=after.capacity;}
  else if(!strcmp(action,"breakable_mode"))after.mode=static_cast<Damage>((before.mode+1)%DamageCount);
  else if(!strcmp(action,"breakable_material"))after.material=(before.material+1)%materialCount;
  else if(!strcmp(action,"breakable_use"))after.playerUse=!before.playerUse;
  else if(!strcmp(action,"breakable_enabled"))after.enabled=!before.enabled;
  else if(!strcmp(action,"breakable_explosion"))after.explosion=!before.explosion;
  else{
   float values[5];
   const int lower[]={1,0,0,0,0},upper[]={maxHealth,maxFuse,maxChunks,maxSplash,maxRadius};
   for(int i=0;i<5;++i)if(!jar::context::Number(gi.argv(4+i),values[i],lower[i],upper[i])||values[i]!=std::floor(values[i])){
    message("Check whole-number health, fuse, debris, splash damage and radius.");return;
   }
   after.capacity=int(values[0]);after.fuse=int(values[1]);after.chunks=int(values[2]);
   after.splash=int(values[3]);after.radius=int(values[4]);after.health=std::min(after.capacity,before.health);
  }
  if(!Same(before,after)){
   if(!G_SandboxBreakableSet(e,after)){message("Clear the space around this breakable before repairing it.");return;}
   rememberEdit(e);
   edits.back().breakableBefore=before;edits.back().breakableAfter=after;
   message("Breakable settings applied. Revert property restores the previous state.");
  }
 }else if(!strcmp(action,"station_apply")||!strcmp(action,"station_charge")||!strcmp(action,"station_refill")||
          !strcmp(action,"station_unlimited")||!strcmp(action,"station_enabled")){
  using namespace jar::sandboxStation;
  Settings before;
  if(!owned(e.s.number)||!G_SandboxStationRead(e,before))return;
  auto after=before;
  if(!strcmp(action,"station_refill"))after.charge=before.capacity;
  else if(!strcmp(action,"station_unlimited"))after.unlimited=!before.unlimited;
  else if(!strcmp(action,"station_enabled"))after.enabled=!before.enabled;
  else if(!strcmp(action,"station_charge")){
   float charge;
   if(!jar::context::Number(gi.argv(4),charge,0,before.capacity)||charge!=std::floor(charge)){
    message("Enter whole charge from zero to capacity.");return;
   }
   after.charge=int(charge);
  }else{
   float capacity,recharge;
   if(!jar::context::Number(gi.argv(4),capacity,1,maxCapacity)||capacity!=std::floor(capacity)||
      !jar::context::Number(gi.argv(5),recharge,0,maxRecharge)||recharge!=std::floor(recharge)||
      (recharge>0&&recharge<minRecharge)){
    message("Enter capacity 1 to 10000 and recharge 0 or 100 to 60000 ms.");return;
   }
   after.capacity=int(capacity);after.recharge=int(recharge);
   after.charge=std::min(before.charge,after.capacity);
  }
  if(!SameProperties(before,after)||before.charge!=after.charge){
   rememberEdit(e);
   edits.back().stationBefore=before;edits.back().stationAfter=after;
   G_SandboxStationSet(e,after);
   message("Station settings applied. Revert property restores the previous settings.");
  }
 }else if(!strcmp(action,"trail_unlink")){
  if(!G_WorkshopTrailClass(e)||!G_JAR_WorkshopOwns(&e))return;
  editExplosionTrailAim(e,nullptr);message("Explosion Trail now uses the emitter's facing.");
 }else if(!strcmp(action,"laser_emitter_damage")||!strcmp(action,"laser_emitter_firing")||
          !strcmp(action,"laser_emitter_enabled")||!strcmp(action,"laser_emitter_unlink")){
  jar::sandboxLaser::Settings before;
  if(!owned(e.s.number)||!G_SandboxLaserRead(e,before))return;
  if(!strcmp(action,"laser_emitter_unlink")){
   editLaserEmitterAim(e,nullptr);message("Laser points forward.");
  }else{
   auto after=before;
   if(!strcmp(action,"laser_emitter_firing"))after.firing=!before.firing;
   else if(!strcmp(action,"laser_emitter_enabled"))after.enabled=!before.enabled;
   else{
    float damage;
    if(!jar::context::Number(gi.argv(4),damage,0,jar::sandboxLaser::maxDamage)||damage!=std::floor(damage)){
     message("Enter whole laser damage from 0 to 1000.");return;
    }
    after.damage=int(damage);
   }
   if(!jar::sandboxLaser::Same(before,after)){
    rememberEdit(e);
    auto& edit=edits.back();edit.laserEmitterEdit=true;edit.laserEmitterBefore=before;edit.laserEmitterAfter=after;
    G_SandboxLaserSet(e,after);
    message("Laser emitter properties applied. Revert property restores the previous settings.");
   }
  }
 }else if(!strcmp(action,"laser")){
  jar::laserArm::Settings before,next;float operation,damage;
  if(!G_LaserArmRead(e,before)||!jar::context::Number(gi.argv(4),operation,0,4)||operation!=std::floor(operation)||!jar::context::Number(gi.argv(5),next.yaw,-180,180)||!jar::context::Number(gi.argv(6),next.pitch,-45,90)||!jar::context::Number(gi.argv(7),next.speed,1,360)||!jar::context::Number(gi.argv(8),damage,0,1000)||damage!=std::floor(damage)||!jar::context::Number(gi.argv(9),next.duration,.1f,30)||!jar::context::Number(gi.argv(10),next.range,64,8192)){
   message("Laser: yaw +/-180, pitch -45 to 90, speed 1-360, damage 0-1000, duration 0.1-30 seconds, range 64-8192.");return;
  }
  next.action=int(operation);next.damage=int(damage);rememberEdit(e);edits.back().laserEdit=true;edits.back().laser=before;G_LaserArmSet(e,next);
  message("Laser properties applied; firing stopped. Activate performs the selected action. Revert property restores properties.");
 }else if(!strcmp(action,"cloud")){
  jar::cloud::Settings before,next;float tube,alternate,on;
  if(!G_CloudRead(e,before)||!jar::context::Number(gi.argv(4),next.radius,16,32768)||!jar::context::Number(gi.argv(5),next.inner,0,32760)||!jar::context::Number(gi.argv(6),next.curve,-20,20)||
     !jar::context::Number(gi.argv(7),tube,0,1)||tube!=std::floor(tube)||!jar::context::Number(gi.argv(8),alternate,0,1)||alternate!=std::floor(alternate)||!jar::context::Number(gi.argv(9),on,0,1)||on!=std::floor(on)){
   message("Cloud radius is 16-32768, inner radius at least 8 smaller, curvature -20 to 20; use Disk/Hollow, style and On/Off choices.");return;
  }
  next.tube=tube!=0;next.alternate=alternate!=0;next.enabled=on!=0;
  if(!jar::cloud::Valid(next)){message("Inner cloud radius must be at least 8 units smaller than outer radius.");return;}
  rememberEdit(e);edits.back().cloudEdit=true;edits.back().cloud=before;G_CloudSet(e,next);message("Cloud properties applied. Move or rotate the marker to position the layer; Revert property restores properties.");
 }else if(!strcmp(action,"boundary")){
  float seconds,preserve;if(!G_ShipBoundaryClass(e)||!jar::context::Number(gi.argv(4),seconds,.1f,60)||!jar::context::Number(gi.argv(5),preserve,0,1)||preserve!=std::floor(preserve)){message("Flight boundaries use 0.1-60 seconds and an Ignore/Destroy choice for empty or damaged ships.");return;}
  if(e.count<100||e.count>60000){message("This authored boundary's duration cannot be represented by the editor.");return;}
  rememberEdit(e);edits.back().boundaryEdit=true;edits.back().boundaryTime=e.count/1000.f;edits.back().boundaryPreserve=(e.spawnflags&1)!=0;
  if(!G_ShipBoundarySet(e,seconds,preserve!=0)){edits.pop_back();return;}message("Flight-boundary properties applied. Current turns cancelled; Revert property restores properties.");
 }else if(!strcmp(action,"script")){
  jar::scriptRunner::Settings previous,next;std::string path;float count,activator;
  if(!G_ScriptRunnerRead(e,previous)){message("This entity has no editable script runner properties.");return;}
  if(!Unhex(gi.argv(4),path,jar::scriptRunner::textLimit)||path.size()!=strlen(path.c_str())||!jar::scriptRunner::Canonical(path.c_str(),next.script)||
     !jar::context::Number(gi.argv(5),next.delay,0,3600)||!jar::context::Number(gi.argv(6),next.wait,0,3600)||
     !jar::context::Number(gi.argv(7),count,-1,100000)||count!=std::floor(count)||!jar::context::Number(gi.argv(8),activator,0,1)||activator!=std::floor(activator)){
   message("Choose a script, delay/cooldown 0-3600 seconds, uses -1 to 100000 and Self or Activator.");return;
  }
  next.count=int(count);next.activator=activator!=0;rememberEdit(e);edits.back().scriptEdit=true;edits.back().script=previous;
  if(!G_ScriptRunnerSet(e,next)){edits.pop_back();message("Compiled script is missing or has an invalid IBI header. Properties unchanged.");return;}
  message("Runner properties applied; pending activation cancelled. Activate runs the script; Revert property restores properties.");
 }else if(!strcmp(action,"parm")){
  uint32_t slot=0;std::string text,previous;
  if(!jar::context::Token(gi.argv(4),slot)||slot>jar::parms::count||!Unhex(gi.argv(5),text,jar::parms::limit)||!jar::parms::Valid(text)){message("Script parameters need a slot from 1 to 16 and up to 63 printable characters.");return;}
  --slot;if(!jar::parms::Read(e.parms?e.parms->parm[slot]:nullptr,previous)){message("This authored parameter cannot be represented by the editor; its value is retained.");return;}
  if(text==previous)return;
  rememberEdit(e);edits.back().parmSlot=slot;edits.back().parm=previous;
  jar::entityParms::Literal(e,slot,text);
  message("Script parameter saved literally. Activate a parameter controller to copy values to its activator.");
 }else if(!strcmp(action,"label")){
  std::string text;if(!editableLabel(e)||!Unhex(gi.argv(4),text,63)||text.size()!=strlen(text.c_str())||!jar::context::Label(text.c_str())){message("Labels need 1-63 printable characters, without quotes, semicolons or backslashes.");return;}
  rememberEdit(e);edits.back().labelEdit=true;edits.back().label=e.message?e.message:"";
  if(e.message&&gi.bIsFromZone(e.message,TAG_G_ALLOC))gi.Free(e.message);e.message=G_NewString(text.c_str());
  message("Label applied. Revert property restores its previous text.");
 }else if(!strcmp(action,"replicator_apply")||!strcmp(action,"replicator_enabled")){
  if(!G_SandboxReplicator(e)){message("Select a spawned Item Replicator.");return;}
  const int beforeHeal=G_ReplicatorHeal(e),beforeDelay=G_ReplicatorDelay(e);
  const bool enabledBefore=G_LogicUseEnabled(e);
  int heal=beforeHeal,delay=beforeDelay;
  bool enabledAfter=enabledBefore;
  if(!strcmp(action,"replicator_enabled"))enabledAfter=!enabledBefore;
  else{
   float healValue,delayValue;
   if(!jar::context::Number(gi.argv(4),healValue,0,jar::replicator::maxHeal)||healValue!=std::floor(healValue)||
      !jar::context::Number(gi.argv(5),delayValue,jar::replicator::minDelay,jar::replicator::maxDelay)||delayValue!=std::floor(delayValue)){
    message("Enter whole healing from 0 to 100 and delay from 100 to 10000 ms.");return;
   }
   heal=int(healValue);delay=int(delayValue);
  }
  if(heal==beforeHeal&&delay==beforeDelay&&enabledAfter==enabledBefore)return;
  rememberEdit(e);auto& edit=edits.back();
  edit.replicatorHealBefore=beforeHeal;edit.replicatorDelayBefore=beforeDelay;
  edit.replicatorHealAfter=heal;edit.replicatorDelayAfter=delay;
  edit.replicatorEnabledBefore=enabledBefore;edit.replicatorEnabledAfter=enabledAfter;
  e.damage=heal;e.wait=delay;G_LogicSetUseEnabled(e,enabledAfter);
  message("Replicator settings applied. Delay applies to the next cycle.");
 }else if(!strcmp(action,"secret_repeat")||!strcmp(action,"secret_rearm")||!strcmp(action,"secret_enabled")){
  if(!G_SandboxSecret(e)){message("Select a spawned Secret marker.");return;}
  const int before=e.spawnflags&JAR_SECRET_FLAGS;
  const bool enabledBefore=G_LogicUseEnabled(e);
  int flags=before;
  bool enabledAfter=enabledBefore;
  if(!strcmp(action,"secret_enabled"))enabledAfter=!enabledBefore;
  else if(!strcmp(action,"secret_rearm"))flags&=~JAR_SECRET_FOUND;
  else flags=(flags^JAR_SECRET_REPEAT)&~JAR_SECRET_FOUND;
  if(flags==before&&enabledAfter==enabledBefore)return;
  rememberEdit(e);auto& edit=edits.back();
  edit.secretFlagsBefore=before;edit.secretFlagsAfter=flags;
  edit.secretEnabledBefore=enabledBefore;edit.secretEnabledAfter=enabledAfter;
  e.spawnflags=(e.spawnflags&~JAR_SECRET_FLAGS)|flags;
  G_LogicSetUseEnabled(e,enabledAfter);
  message("Secret settings applied. Link Primary to trigger another entity on discovery.");
 }else if(!strcmp(action,"shooter_interval")||!strcmp(action,"shooter_alternate")||
          !strcmp(action,"shooter_repeat")||!strcmp(action,"shooter_enabled")){
  jar::shooter::Settings before;
  if(!G_SandboxShooterRead(e,before)){message("Select a spawned weapon shooter.");return;}
  auto next=before;
  if(!strcmp(action,"shooter_alternate"))next.alternate=!before.alternate;
  else if(!strcmp(action,"shooter_repeat"))next.repeat=!before.repeat;
  else if(!strcmp(action,"shooter_enabled"))next.enabled=!before.enabled;
  else{
   float interval,variation;
   if(!jar::context::Number(gi.argv(4),interval,jar::shooter::minInterval,jar::shooter::maxInterval)||interval!=std::floor(interval)||
      !jar::context::Number(gi.argv(5),variation,0,jar::shooter::maxVariation)||variation!=std::floor(variation)){
    message("Use a whole interval of 100 to 30000 ms and variation of 0 to 30000 ms.");return;
   }
   next.interval=int(interval);next.variation=int(variation);
  }
  if(jar::shooter::Same(before,next))return;
  const auto control=nativeControls.find(e.s.number);
  const int clock=control==nativeControls.end()?level.time:control->second.started;
  const int pending=before.enabled&&before.repeat?G_SandboxShooterPending(e,clock):0;
  if(!G_SandboxShooterSet(e,next))return;
  rememberEdit(e);auto& edit=edits.back();edit.shooterEdit=true;
  edit.shooterBefore=before;edit.shooterAfter=next;edit.shooterPendingMs=pending;
  message("Shooter settings applied. Activate to fire or stop a running repeat.");
 }else if(!strcmp(action,"npc_goal_clear")){
  jar::npcOrder::Settings order;
  if(!contextActorOrder(e,order)){message("Select a compatible Sandbox NPC.");return;}
  if(!jar::npcOrder::Same(contextPublishedNPCGoal,G_WorkshopReadNPCGoal(*e.NPC))){
   message("The destination changed. Refresh and try again.");return;
  }
  editContextActorGoal(e,{});
  message("NPC destination cleared.");
 }else if(!strcmp(action,"npc_order")){
  using namespace jar::npcOrder;
  Settings current,merged;
  if(!contextPublishedOrderAllowed||!contextActorOrder(e,current)){
   message("Select a living Sandbox NPC with free controls.");return;
  }
  float mode;
  if(!jar::context::Number(gi.argv(4),mode,Autonomous,ModeCount-1)||mode!=std::floor(mode)){
   message("Choose one of the listed NPC orders.");return;
  }
  Settings requested=contextPublishedOrder;
  requested.mode=static_cast<Mode>(int(mode));
  if(UsesDestination(requested.mode)){
   const auto* goal=G_WorkshopResolveNPCGoal(contextPublishedNPCGoal);
   if(!jar::npcOrder::Same(contextPublishedNPCGoal,G_WorkshopReadNPCGoal(*e.NPC))||!goal||
      (requested.mode==Patrol&&!G_WaypointPortable(*goal))){
    message(requested.mode==Patrol?"Link this NPC to the first waypoint with the Toolgun.":"Link this NPC to a spawned Nav goal or waypoint.");return;
   }
  }
  if(requested.mode==Follow){
   float distance;
   if(!G_WorkshopNPCOrderCanFollow(e,g_entities[0])){
    message("Set this NPC's team to Player before ordering it to follow.");return;
   }
   if(!jar::context::Number(gi.argv(5),distance,minDistance,maxDistance)){
    message("Use a follow distance from 72 to 1024 units.");return;
   }
   if(requested.distance||distance!=defaultDistance)requested.distance=distance;
  }
  if(!Merge(current,contextPublishedOrder,requested,merged)){
   message("The NPC order changed. Refresh and try again.");return;
  }
  if(current.mode==merged.mode&&current.distance==merged.distance)return;
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];edit.actorOrderEdit=true;
  edit.actorPatrolBefore=G_WorkshopReadNPCPatrol(*e.NPC);
  applyContextActorOrder(e,current,merged);
  edit.actorOrderBefore=current;edit.actorOrderAfter=merged;rememberAction(std::move(edit));
  message("NPC order applied.");
 }else if(!strcmp(action,"npc_weapon")){
  float weaponValue;
  if(!jar::context::Number(gi.argv(4),weaponValue,WP_NONE,WP_NATIVE_WEAPON_COUNT-1)||weaponValue!=std::floor(weaponValue)){
   message("Choose a listed NPC weapon.");return;
  }
  const int weapon=int(weaponValue);
  const uint32_t choices=contextActorWeaponChoices(e);
  const uint32_t bit=1u<<weapon;
  if(!(choices&contextPublishedActorWeapons&bit)||e.client->ps.weapon!=contextPublishedActorWeapon){
   message("The NPC or weapon choices changed. Refresh and try again.");return;
  }
  if(weapon==contextPublishedActorWeapon)return;

  Edit edit{};
  edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];edit.actorWeaponEdit=true;
  edit.actorWeaponBefore=G_WorkshopReadActorWeapon(e);edit.actorWeaponAfter=weapon;
  const auto& ps=e.client->ps;
  const uint32_t ownedWeapons=uint32_t(ps.stats[STAT_WEAPONS]);
  edit.actorWeaponGrant=weapon==WP_NONE?0u:bit&~ownedWeapons;
  const int pool=weaponData[weapon].ammoIndex;
  const auto* item=edit.actorWeaponGrant?G_WorkshopNPCWeaponItem(weapon):nullptr;
  if(item&&pool>AMMO_FORCE&&pool<AMMO_MAX&&ps.ammo[pool]==0){
   edit.actorWeaponPool=pool;
   edit.actorWeaponGrantedAmmo=std::min(item->quantity>0?item->quantity:50,32767);
  }
  if(!G_WorkshopApplyActorWeapon(e,{weapon,false,false},ownedWeapons|edit.actorWeaponGrant,
      edit.actorWeaponPool,edit.actorWeaponGrantedAmmo)){
   message("The selected weapon model could not be attached.");return;
  }
  rememberAction(std::move(edit));message("NPC weapon changed.");
 }else if(!strcmp(action,"npc_fire")){
  WorkshopActorFire current,next;
  if(!contextPublishedFire||!contextActorFire(e,current)){message("Select a Sandbox NPC holding a weapon that uses ammo.");return;}
  float ammo,alternate,manualAmmo;
  if(!jar::context::Number(gi.argv(4),ammo,-1,32767)||ammo!=std::floor(ammo)||
     !jar::context::Number(gi.argv(5),alternate,0,1)||alternate!=std::floor(alternate)||
     !jar::context::Number(gi.argv(6),manualAmmo,0,1)||manualAmmo!=std::floor(manualAmmo)){
   message("Use whole ammo from -1 to 32767 and the listed fire and ammo modes.");return;
  }
  auto requested=contextPublishedActorFire;
  requested.ammo=int(ammo);requested.alternate=alternate!=0;requested.manualAmmo=manualAmmo!=0;
  if(!G_WorkshopMergeActorFire(current,contextPublishedActorFire,requested,next)){
   message("The NPC weapon or edited fields changed. Refresh and try again.");return;
  }
  if(current.ammo==next.ammo&&current.alternate==next.alternate&&current.manualAmmo==next.manualAmmo)return;
  G_WorkshopApplyActorFire(e,current,next);
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];edit.actorFireEdit=true;
  edit.actorFireBefore=current;edit.actorFireAfter=next;rememberAction(std::move(edit));
  message("NPC weapon settings applied. Revert restores the edited values.");
 }else if(!strcmp(action,"npc_status")){
  jar::duplicate::ActorStatus current,next;
  if(!contextPublishedActor||!contextActorStatus(e,current)){message("Select a living Sandbox NPC with free controls.");return;}
  float team,target,armor;
  if(!jar::context::Number(gi.argv(4),team,TEAM_FREE,TEAM_NUM_TEAMS-1)||team!=std::floor(team)||
     !jar::context::Number(gi.argv(5),target,TEAM_FREE,TEAM_NUM_TEAMS-1)||target!=std::floor(target)||
     !jar::context::Number(gi.argv(6),armor,0,32767)||armor!=std::floor(armor)){
   message("Choose native NPC teams and whole armor from 0 to 32767.");return;
  }
  if(!mergeWorkshopActorStatus(current,contextPublishedActorStatus,{int(team),int(target),int(armor)},next)){
   message("NPC settings changed while the editor was open. Refresh and try again.");return;
  }
  if(current.team==next.team&&current.enemyTeam==next.enemyTeam&&current.armor==next.armor)return;
  if(!applyContextActorStatus(e,current,next)){message("This NPC's target is locked by its controller.");return;}
  Edit edit{};edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];edit.actorStatusEdit=true;
  edit.actorStatusBefore=current;edit.actorStatusAfter=next;rememberAction(std::move(edit));
  message("NPC settings applied. Revert restores the edited values.");
 }else if(!strcmp(action,"vehicle_resources")){
  jar::vehicleStatus::Values before,next;
  if(!contextVehicleResources(e,before)){
   message("Select a living vehicle with settled seats to edit its resources.");return;
  }
  const auto limits=G_WorkshopVehicleLimits(*e.m_pVehicle->m_pVehicleInfo);
  for(int i=0;i<jar::vehicleStatus::fields;++i)if(!jar::vehicleStatus::Field(gi.argv(4+i),i,limits.value[i],next.value[i])){
   message("Enter whole vehicle resource values within the displayed limits.");return;
  }
  if(next.value==before.value)return;
  rememberEdit(e);auto& edit=edits.back();edit.vehicleResourcesEdit=true;
  edit.vehicleResourcesBefore=before;edit.vehicleResourcesAfter=next;edit.health=e.health;
  if(!G_WorkshopSetVehicleResources(e,next)){edits.pop_back();return;}
  if(e.m_pVehicle->m_pVehicleInfo->type==VH_ANIMAL&&next.value[0]!=before.value[0]){
   e.health=next.value[0];e.client->ps.stats[STAT_HEALTH]=e.health;
  }
  message("Vehicle resources applied. Revert restores the previous values if they have not changed.");
 }else if(!strcmp(action,"health")){
  float value;
  if(!contextHealthEditable(e)||!jar::context::Number(gi.argv(4),value,1,100000)||value!=std::floor(value)){
   message("Health requires an eligible living entity and a whole number from 1 to 100000.");return;
  }
  if(e.health!=contextPublishedHealth){
   contextPublish();message("Health changed since it was displayed. Review the current value before applying.");return;
  }
  if(e.health==int(value))return;
  Edit edit{};
  edit.entity=e.s.number;edit.generation=entityGeneration[e.s.number];
  edit.healthEdit=true;edit.health=e.health;edit.healthAfter=int(value);
  rememberAction(std::move(edit));
  e.health=int(value);if(e.client)e.client->ps.stats[STAT_HEALTH]=e.health;message("Health applied. Revert property restores the previous value.");
 }else if(!strcmp(action,"rope")||!strcmp(action,"thruster")||!strcmp(action,"unlink")||!strcmp(action,"weld")||!strcmp(action,"hinge")){
  vec3_t axis={0,0,1};
  if(!strcmp(action,"rope"))attachmentRope(e,e.currentOrigin,axis);
  else if(!strcmp(action,"thruster"))attachmentThruster(e,false,e.currentOrigin,axis);
  else if(!strcmp(action,"weld")||!strcmp(action,"hinge"))constraintLink(e,e.currentOrigin,axis,!strcmp(action,"weld")?jar::constraint::Weld:jar::constraint::Hinge);
  else unlinkTool();
 }else if(!strcmp(action,"paste")){duplicatePaste();contextSelect();return;
 }else if(!strcmp(action,"freeze")||!strcmp(action,"release")||!strcmp(action,"activate")||!strcmp(action,"duplicate")||!strcmp(action,"remove")){
  tool(action);
 }else return;
 contextPublish();
}
void rotateHeld(){
 uint32_t token;float yaw,pitch;
 if(!enabled()||!usable()||!physgunGrip.active||held>=ENTITYNUM_WORLD||!editable(g_entities[held])||
    cg.weaponSelect!=WP_PHYSGUN||!jar::context::Token(gi.argv(2),token)||token!=holdToken||
    !jar::context::Number(gi.argv(3),yaw,-180,180)||!jar::context::Number(gi.argv(4),pitch,-180,180))return;
 physgunGrip.Turn(yaw,pitch,cg.refdefViewAngles[PITCH],cg.refdefViewAngles[ROLL]);
}
