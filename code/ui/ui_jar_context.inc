// Hold-C entity editor. The world remains unpaused; text entry captures movement.
#include "qcommon/jar_context.h"
#include "qcommon/jar_npc_weapon.h"
#include "qcommon/jar_npc_order.h"
#include "../game/teams.h"
#include "qcommon/jar_logic_settings.h"
#include "qcommon/jar_contact_trigger.h"
#include "qcommon/jar_push.h"
#include "qcommon/jar_teleport.h"
#include "../../shared/qcommon/jar_context_menu.h"
#include "../../shared/qcommon/jar_always_trigger.h"
#include "../../shared/qcommon/jar_visible_trigger.h"
#include "../../shared/qcommon/jar_distance_trigger.h"
#include "../../shared/qcommon/jar_sliding_door.h"
#include "../../shared/qcommon/jar_button.h"
#include "qcommon/jar_rail.h"
#include "qcommon/jar_reference.h"
#include "qcommon/jar_combat_point.h"
#include "qcommon/jar_cloud.h"
#include "qcommon/jar_reflection_probe.h"
#include "qcommon/jar_laser_arm.h"
#include "qcommon/jar_sandbox_laser.h"
#include "qcommon/jar_sandbox_station.h"
#include "qcommon/jar_sandbox_breakable.h"
#include "qcommon/jar_sandbox_usable.h"
#include "qcommon/jar_portal.h"
#include "qcommon/jar_spotlight.h"
#include "qcommon/jar_welder.h"
#include "qcommon/jar_door_camera.h"
#include "qcommon/jar_crystal_crate.h"
#include "qcommon/jar_weather.h"
#include "qcommon/jar_music.h"
#include "qcommon/jar_sandbox_speaker.h"
#include "qcommon/jar_sandbox_angular.h"
#include "qcommon/jar_level_change.h"
#include "qcommon/jar_script_parms.h"
#include "qcommon/jar_scriptrunner.h"
#include "qcommon/jar_volume.h"
#include "qcommon/jar_workshop_state.h"
#include "qcommon/jar_vehicle_status.h"
#include "qcommon/jar_shooter.h"
#include "qcommon/jar_replicator.h"
#include "qcommon/jar_pickup_quantity.h"
#include "qcommon/jar_pickup_lifetime.h"
#include "qcommon/jar_shield_copy.h"
#include "qcommon/jar_charge_copy.h"
static char contextLabel[128],contextInfo[96],contextLocation[96];
static uint32_t contextUiToken=0;
static char contextReflectionState[32],contextReflectionStatus[96];
static void ContextLoadReflection(){for(const char*key:{"Radius","Enabled"})Cvar_Set(va("ui_jar_contextReflection%s",key),Cvar_VariableString(va("jar_contextReflection%s",key)));}
static char contextRailState[32],contextRailCenter[40];
static void ContextLoadRail(){for(const char*key:{"Kind","Length","Width","Cell","Speed","Delay","Count","Enabled","Center"})Cvar_Set(va("ui_jar_contextRail%s",key),Cvar_VariableString(va("jar_contextRail%s",key)));}
static char contextWaypointState[32],contextWaypointStatus[96];
static void ContextLoadWaypoint(){for(const char*key:{"Range","Enabled"})Cvar_Set(va("ui_jar_contextWaypoint%s",key),Cvar_VariableString(va("jar_contextWaypoint%s",key)));}
static char contextSceneLoop[32];
static void ContextLoadScene(){for(const char*key:{"Speed","Turn","Wait","Loop"})Cvar_Set(va("ui_jar_contextScene%s",key),Cvar_VariableString(va("jar_contextScene%s",key)));}
static char contextSkyState[32],contextSkyEntities[32];
static void ContextLoadSky(){for(const char*key:{"Enabled","Entities"})Cvar_Set(va("ui_jar_contextSky%s",key),Cvar_VariableString(va("jar_contextSky%s",key)));}
static char contextCombatState[32],contextCombatStatus[96],contextCombatFlags[6][32];
static void ContextLoadCombat(){for(const char*key:{"Flags","Enabled"})Cvar_Set(va("ui_jar_contextCombat%s",key),Cvar_VariableString(va("jar_contextCombat%s",key)));}
static char contextReferenceState[32];
static void ContextLoadReference(){for(const char*key:{"Name","Owner","Radius","Enabled"})Cvar_Set(va("ui_jar_contextReference%s",key),Cvar_VariableString(va("jar_contextReference%s",key)));}
static char contextPoseRange[64];
static void ContextLoadPose(){Cvar_Set("ui_jar_contextPoseFrame",Cvar_VariableString("jar_contextPoseFrame"));}
static char contextTravelMode[64];
static void ContextLoadTravel(){for(const char*key:{"Map","Spawn","Hub"})Cvar_Set(va("ui_jar_contextTravel%s",key),Cvar_VariableString(va("jar_contextTravel%s",key)));}
static unsigned contextPage=0,contextParmSlot=0;
static char contextSlideDirection[48],contextSlideOutput[48],contextSlideStatus[80],contextSlideTab[12],contextSlideAction[24];
static char contextBobbingMotion[24],contextBobbingEnabled[24],contextBobbingStatus[80];
static char contextMovementEnabled[24],contextMovementStatus[64];
static const char* const vehicleResourceNames[]={"Hull armor","Shields","Primary ammo","Secondary ammo","Turret 1 ammo","Turret 2 ammo"};
static char contextVehicleResourceLabels[jar::vehicleStatus::fields][48];
static const char* const contextNPCTeams[TEAM_NUM_TEAMS]={"Free","Player","Enemy","Neutral"};
static char contextNPCTeamLabel[40],contextNPCTargetLabel[40];
static char contextNPCFireLabel[40],contextNPCAmmoSupply[40];
static int contextNPCWeapon=WP_NONE;
static char contextNPCWeaponHeld[64],contextNPCWeaponChoice[64];
static char contextNPCOrderLabel[64];
static char contextNPCGoalLabel[64],contextNPCPatrolLabel[64];
static void ContextLoadNPC(){
 for(const char* key:{"Team","TargetTeam","Armor","Ammo","Alternate","ManualAmmo","OrderMode","OrderDistance"})
  Cvar_Set(va("ui_jar_contextNPC%s",key),Cvar_VariableString(va("jar_contextNPC%s",key)));
 contextNPCWeapon=atoi(Cvar_VariableString("jar_contextNPCWeapon"));
}
static int ContextNPCOrderMode(){
 const int value=atoi(Cvar_VariableString("ui_jar_contextNPCOrderMode"));
 return value>=jar::npcOrder::Autonomous&&value<jar::npcOrder::ModeCount?value:jar::npcOrder::Autonomous;
}
static int ContextNPCTeam(const char* name){
 const int value=atoi(Cvar_VariableString(name));
 return value>=TEAM_FREE&&value<TEAM_NUM_TEAMS?value:TEAM_FREE;
}
static char contextShooterAlternate[24],contextShooterRepeat[24],contextShooterEnabled[24];
static char contextSecretMode[24],contextSecretStatus[24],contextSecretEnabled[24];
static char contextReplicatorStatus[24],contextReplicatorEnabled[24];
static char contextDeployedShieldStatus[64];
static char contextMountedChargeStatus[64];
static char contextRopeLabel[48],contextRopeEnd[76];
static void ContextLoadRope(){Cvar_Set("ui_jar_contextRopeLength",Cvar_VariableString("jar_contextRopeLength"));}
static char contextThrusterEnabled[24],contextThrusterDirection[32];
static void ContextLoadThruster(){
 Cvar_Set("ui_jar_contextThrusterForce",Cvar_VariableString("jar_contextThrusterForce"));
 Cvar_Set("ui_jar_contextThrusterEnabled",Cvar_VariableString("jar_contextThrusterEnabled"));
 Cvar_Set("ui_jar_contextThrusterReverse","0");
}
static void ContextLoadMountedCharge(){
 for(const char* key:{"Health","Damage","Radius"})
  Cvar_Set(va("ui_jar_contextMountedCharge%s",key),Cvar_VariableString(va("jar_contextMountedCharge%s",key)));
 Cvar_Set("ui_jar_contextMountedChargeRemaining","0");
}
static void ContextLoadDeployedShield(){
 for(const char* key:{"Health","Left","Right","Height"})
  Cvar_Set(va("ui_jar_contextDeployedShield%s",key),Cvar_VariableString(va("jar_contextDeployedShield%s",key)));
 Cvar_Set("ui_jar_contextDeployedShieldRemaining","0");
}
static void ContextLoadReplicator(){
 for(const char* key:{"Heal","Delay"})
  Cvar_Set(va("ui_jar_contextReplicator%s",key),Cvar_VariableString(va("jar_contextReplicator%s",key)));
}
static void ContextLoadShooter(){
 for(const char* key:{"Interval","Variation"})
  Cvar_Set(va("ui_jar_contextShooter%s",key),Cvar_VariableString(va("jar_contextShooter%s",key)));
}
static void ContextLoadVehicle(){
 for(int i=0;i<jar::vehicleStatus::fields;++i)
  Cvar_Set(va("ui_jar_contextVehicleValue%d",i),Cvar_VariableString(va("jar_contextVehicleValue%d",i)));
}
static char contextLogicTab[16],contextLogicEnabled[24],contextLogicStatus[48],contextLogicRepeat[32],contextLogicOutput[32];
static char contextTimerRun[24],contextLogicSlot[24],contextLogicTarget[72],contextKillMode[32];
static int ContextLogicSlot(jar::logic::Kind kind){
 const int slot=atoi(Cvar_VariableString(jar::logic::SlotCvar(kind)));
 return slot>=0&&slot<jar::logic::outputCount?slot:0;
}
static char contextContactSubject[32],contextContactUse[32],contextContactEnabled[32],contextContactStatus[48],contextContactMode[32];
static char contextContactArmed[24];
static char contextPushAim[40],contextPushSubject[32],contextPushAdd[32],contextPushEnabled[32],contextPushTarget[80];
static char contextPushVehicles[32];
static char contextTeleportSubject[32],contextTeleportView[32],contextTeleportVelocity[32],contextTeleportEnabled[32],contextTeleportTarget[96];
static char contextTeleportProps[32],contextTeleportProjectiles[32],contextTeleportVehicles[32];
static char contextTeleportAssemblies[32];
static char contextTeleportFrozenProps[32];
static char contextTeleportMovers[32];
static char contextTeleportWorldAnchors[32];
static char contextStationCharge[48],contextStationUnlimited[32],contextStationEnabled[32];
static char contextBreakableStatus[64],contextBreakableMode[40],contextBreakableMaterial[40];
static char contextBreakablePlayerUse[24],contextBreakableEnabled[24],contextBreakableExplosion[24];
static char contextUsableStatus[80],contextUsableMode[24],contextUsableDamage[32],contextUsableDirect[32];
static char contextUsableSolid[24],contextUsableEnabled[24],contextUsableRole[24],contextUsableOutput[80];
static char contextSpeakerCue[48],contextSpeakerMode[48],contextSpeakerState[48],contextSpeakerEnabled[24];
static char contextAngularTab[16],contextAngularAxis[32],contextAngularStatus[64],contextAngularEnabled[24],contextAngularRun[24];
static void ContextLoadAngular(){
 for(const char* key:{"Rate","Period"})
  Cvar_Set(va("ui_jar_contextAngular%s",key),Cvar_VariableString(va("jar_contextAngular%s",key)));
}
static void ContextLoadSpeaker(){
 for(const char* key:{"Period","Variation"})
  Cvar_Set(va("ui_jar_contextSpeaker%s",key),Cvar_VariableString(va("jar_contextSpeaker%s",key)));
}
static void ContextLoadUsable(){
 for(const char* key:{"Capacity","Reuse"})
  Cvar_Set(va("ui_jar_contextUsable%s",key),Cvar_VariableString(va("jar_contextUsable%s",key)));
}
static void ContextLoadBreakable(){
 for(const char* key:{"Capacity","Fuse","Chunks","Splash","Radius"})
  Cvar_Set(va("ui_jar_contextBreakable%s",key),Cvar_VariableString(va("jar_contextBreakable%s",key)));
}
static void ContextLoadStation(){
 for(const char* key:{"Capacity","Charge","Recharge"})
  Cvar_Set(va("ui_jar_contextStation%s",key),Cvar_VariableString(va("jar_contextStation%s",key)));
}
static void ContextLoadTeleport(){
 Cvar_Set("ui_jar_contextTeleportInterval",Cvar_VariableString("jar_contextTeleportInterval"));
}
static void ContextLoadPush(){
 Cvar_Set("ui_jar_contextPushSpeed",Cvar_VariableString("jar_contextPushSpeed"));
 Cvar_Set("ui_jar_contextPushInterval",Cvar_VariableString("jar_contextPushInterval"));
}
static void ContextLoadContact(){
 for(const char* key:{"Delay","Interval","Variation","Damage"})
  Cvar_Set(va("ui_jar_contextContact%s",key),Cvar_VariableString(va("jar_contextContact%s",key)));
}
static void ContextLoadLogic(){
 for(const char* key:{"Delay","Jitter","Hits","Cooldown","Points"})
  Cvar_Set(va("ui_jar_contextLogic%s",key),Cvar_VariableString(va("jar_contextLogic%s",key)));
}
static char contextSlideEnabled[32];
static char contextButtonEnabled[48],contextButtonOutput[48],contextButtonStatus[48];
static void ContextLoadSlide(){
 for(const char* key:{"Travel","Speed","Direction","Wait"})
  Cvar_Set(va("ui_jar_contextSlide%s",key),Cvar_VariableString(va("jar_contextSlide%s",key)));
}
static void ContextLoadBobbing(){
 Cvar_Set("ui_jar_contextBobbingAmplitude",Cvar_VariableString("jar_contextBobbingAmplitude"));
 Cvar_Set("ui_jar_contextBobbingPeriod",Cvar_VariableString("jar_contextBobbingPeriod"));
}
static void ContextLoadMovement(){
 Cvar_Set("ui_jar_contextMovementGravity",Cvar_VariableString("jar_contextMovementGravity"));
 Cvar_Set("ui_jar_contextMovementFriction",Cvar_VariableString("jar_contextMovementFriction"));
}
static char contextScriptReceiver[32],contextBoundaryPolicy[48],contextCloudShape[32],contextCloudStyle[32],contextCloudState[32];
static char contextWeatherMode[32],contextWeatherState[32],contextWeatherHint[96],contextWeatherHint2[96];
static char contextLaserAction[48];
static char contextLaserEmitterFiring[32],contextLaserEmitterEnabled[32],contextLaserEmitterAim[96];
static char contextTrailAim[48];
static char contextPortalMode[48],contextPortalState[32],contextPortalLink[64];
static char contextCrystalSolid[32];
static char contextWelderState[32],contextDoorState[32],contextDoorMotion[64];
static char contextSpotMode[32],contextSpotState[32],contextSpotOutput[48];
static char contextVisibleWalls[40],contextVisibleSight[40],contextVisibleEnabled[32];
static char contextDistanceSubjects[40],contextDistanceArmed[40],contextDistanceReference[40],contextDistanceNPC[40],contextDistanceLinkRole[48];
static char contextWallVisible[32],contextWallEnabled[32];
static char contextSpaceArmed[24],contextSpaceEnabled[24];
static void ContextLoadCrystal(){for(const char*key:{"HitDamage","HitRadius","BlastDamage","BlastRadius","Solid"})Cvar_Set(va("ui_jar_contextCrystal%s",key),Cvar_VariableString(va("jar_contextCrystal%s",key)));}
static void ContextLoadDoor(){for(const char*key:{"Travel","Speed","Extended"})Cvar_Set(va("ui_jar_contextDoor%s",key),Cvar_VariableString(va("jar_contextDoor%s",key)));}
static void ContextLoadWelder(){for(const char*key:{"Reach","Radius","Interval","Damage","Enabled"})Cvar_Set(va("ui_jar_contextWelder%s",key),Cvar_VariableString(va("jar_contextWelder%s",key)));}
static void ContextLoadSpot(){for(const char*key:{"Range","Cone","Light","Color0","Color1","Color2","Tracking","Enabled"})Cvar_Set(va("ui_jar_contextSpot%s",key),Cvar_VariableString(va("jar_contextSpot%s",key)));}
static void ContextLoadPortal(){for(const char*key:{"Camera","Width","Height","Mirror","Enabled"})Cvar_Set(va("ui_jar_contextPortal%s",key),Cvar_VariableString(va("jar_contextPortal%s",key)));}
static void ContextLoadLaser(){
 for(const char*key:{"Action","Yaw","Pitch","Speed","Damage","Duration","Range"})Cvar_Set(va("ui_jar_contextLaser%s",key),Cvar_VariableString(va("jar_contextLaser%s",key)));
 Cvar_Set("ui_jar_contextLaserEmitterDamage",Cvar_VariableString("jar_contextLaserEmitterDamage"));
}
static void ContextLoadWeather(){for(const char*key:{"Kind","Mode","Density","Size","Alpha","Speed","Enabled"})Cvar_Set(va("ui_jar_contextWeather%s",key),Cvar_VariableString(va("jar_contextWeather%s",key)));}
static void ContextLoadCloud(){for(const char*key:{"Radius","Inner","Curve","Tube","Alternate","Enabled"})Cvar_Set(va("ui_jar_contextCloud%s",key),Cvar_VariableString(va("jar_contextCloud%s",key)));}
static void ContextLoadBoundary(){Cvar_Set("ui_jar_contextBoundaryTime",Cvar_VariableString("jar_contextBoundaryTime"));Cvar_Set("ui_jar_contextBoundaryPreserve",Cvar_VariableString("jar_contextBoundaryPreserve"));}
static void ContextLoadScript(){for(const char*key:{"Script","ScriptDelay","ScriptWait","ScriptCount","ScriptActivator"})Cvar_Set(va("ui_jar_context%s",key),Cvar_VariableString(va("jar_context%s",key)));}
static char contextParmLabel[40];
static void ContextLoadParm(){
 Com_sprintf(contextParmLabel,sizeof(contextParmLabel),"Script variable %u / 16",contextParmSlot+1);
 Cvar_Set("ui_jar_contextParm",Cvar_VariableString(va("jar_contextParm%u",contextParmSlot)));
}

static bool ContextField(float&out,const char*name,float low,float high){return jar::context::Number(Cvar_VariableString(name),out,low,high);}
static char contextPropGravity[32];
static void ContextUpdate(){
 uint32_t token=0;jar::context::Token(Cvar_VariableString("jar_contextToken"),token);
 const bool laserArm=atoi(Cvar_VariableString("jar_contextLaserAllowed"))==1;
 const bool laserEmitter=atoi(Cvar_VariableString("jar_contextLaserEmitterAllowed"))==1;
 const bool trail=token&&atoi(Cvar_VariableString("jar_contextTrailAllowed"))==1;
 const bool trailLinked=trail&&atoi(Cvar_VariableString("jar_contextTrailLinked"))==1;
 const bool station=token&&atoi(Cvar_VariableString("jar_contextStationAllowed"))==1;
 const bool breakable=token&&atoi(Cvar_VariableString("jar_contextBreakableAllowed"))==1;
 const bool usable=token&&atoi(Cvar_VariableString("jar_contextUsableAllowed"))==1;
 const bool speaker=token&&atoi(Cvar_VariableString("jar_contextSpeakerAllowed"))==1;
 const bool angular=token&&atoi(Cvar_VariableString("jar_contextAngularAllowed"))==1;
 const bool propPhysics=token&&atoi(Cvar_VariableString("jar_contextPropPhysicsAllowed"))==1;
 const bool swing=atoi(Cvar_VariableString("jar_contextAngularKind"))==jar::sandboxAngular::Pendulum;
 if(token!=contextUiToken){
  ContextLoadStation();
  ContextLoadBreakable();
  ContextLoadUsable();
  ContextLoadSpeaker();
  ContextLoadAngular();ContextLoadVehicle();ContextLoadNPC();ContextLoadShooter();ContextLoadReplicator();
  ContextLoadDeployedShield();
  ContextLoadMountedCharge();
  ContextLoadRope();
  ContextLoadThruster();
  UI_JAR_EndTextEntry();contextUiToken=token;ContextLoadReflection();ContextLoadRail();ContextLoadWaypoint();ContextLoadScene();ContextLoadSky();ContextLoadCombat();ContextLoadReference();ContextLoadPose();ContextLoadTravel();ContextLoadParm();ContextLoadScript();ContextLoadBoundary();ContextLoadCloud();ContextLoadWeather();ContextLoadLaser();ContextLoadPortal();ContextLoadSpot();ContextLoadWelder();ContextLoadDoor();ContextLoadCrystal();ContextLoadSlide();ContextLoadBobbing();ContextLoadMovement();ContextLoadLogic();ContextLoadContact();ContextLoadPush();ContextLoadTeleport();Cvar_Set("ui_jar_contextButtonWait",Cvar_VariableString("jar_contextButtonWait"));
  if((contextPage==20&&strcmp(Cvar_VariableString("jar_contextReflectionAllowed"),"1"))||(contextPage==19&&strcmp(Cvar_VariableString("jar_contextRailAllowed"),"1"))||(contextPage==18&&strcmp(Cvar_VariableString("jar_contextWaypointAllowed"),"1"))||(contextPage==17&&strcmp(Cvar_VariableString("jar_contextSceneAllowed"),"1"))||(contextPage==16&&strcmp(Cvar_VariableString("jar_contextSkyAllowed"),"1"))||(contextPage==15&&strcmp(Cvar_VariableString("jar_contextCombatAllowed"),"1"))||(contextPage==14&&strcmp(Cvar_VariableString("jar_contextReferenceAllowed"),"1"))||(contextPage==13&&strcmp(Cvar_VariableString("jar_contextPoseAllowed"),"1"))||(contextPage==12&&strcmp(Cvar_VariableString("jar_contextTravelAllowed"),"1"))||(contextPage==11&&strcmp(Cvar_VariableString("jar_contextCrystalAllowed"),"1"))||(contextPage==10&&strcmp(Cvar_VariableString("jar_contextDoorAllowed"),"1"))||(contextPage==9&&strcmp(Cvar_VariableString("jar_contextWelderAllowed"),"1"))||(contextPage==8&&strcmp(Cvar_VariableString("jar_contextSpotAllowed"),"1"))||(contextPage==7&&strcmp(Cvar_VariableString("jar_contextPortalAllowed"),"1"))||(contextPage==6&&!laserArm&&!laserEmitter)||(contextPage==5&&strcmp(Cvar_VariableString("jar_contextWeatherAllowed"),"1"))||(contextPage==4&&strcmp(Cvar_VariableString("jar_contextCloudAllowed"),"1"))||(contextPage==2&&strcmp(Cvar_VariableString("jar_contextScriptAllowed"),"1"))||(contextPage==3&&strcmp(Cvar_VariableString("jar_contextBoundaryAllowed"),"1")))contextPage=0;
  for(int i=0;i<3;++i){Cvar_Set(va("ui_jar_contextPosition%d",i),Cvar_VariableString(va("jar_contextPosition%d",i)));Cvar_Set(va("ui_jar_contextAngle%d",i),Cvar_VariableString(va("jar_contextAngle%d",i)));}
  for(int i=0;i<3;++i)Cvar_Set(va("ui_jar_contextVolume%d",i),Cvar_VariableString(va("jar_contextVolume%d",i)));
  Cvar_Set("ui_jar_contextAlwaysDelay",Cvar_VariableString("jar_contextAlwaysDelay"));
  Cvar_Set("ui_jar_contextVisibleRange",Cvar_VariableString("jar_contextVisibleRange"));
  Cvar_Set("ui_jar_contextVisibleCone",Cvar_VariableString("jar_contextVisibleCone"));
  Cvar_Set("ui_jar_contextDistanceRange",Cvar_VariableString("jar_contextDistanceRange"));
  Cvar_Set("ui_jar_contextText",Cvar_VariableString("jar_contextText"));
  Cvar_Set("ui_jar_contextHealth",Cvar_VariableString("jar_contextHealth"));
  Cvar_Set("ui_jar_contextPropMass",Cvar_VariableString("jar_contextPropMass"));
  Cvar_Set("ui_jar_contextHolocronLevel",Cvar_VariableString("jar_contextHolocronLevel"));
  Cvar_Set("ui_jar_contextPickupQuantity",Cvar_VariableString("jar_contextPickupQuantity"));
  Cvar_Set("ui_jar_contextPickupLifetime",Cvar_VariableString("jar_contextPickupLifetime"));
 }
 Com_sprintf(contextPropGravity,sizeof(contextPropGravity),"Gravity %s",atoi(Cvar_VariableString("jar_contextPropGravity"))?"on":"off");
 Com_sprintf(contextReflectionState,sizeof(contextReflectionState),"Reflections: %s",!strcmp(Cvar_VariableString("ui_jar_contextReflectionEnabled"),"1")?"On":"Off");
 Q_strncpyz(contextReflectionStatus,Cvar_VariableString("jar_reflectionStatus"),sizeof(contextReflectionStatus));
 Com_sprintf(contextRailState,sizeof(contextRailState),"Traffic: %s",!strcmp(Cvar_VariableString("ui_jar_contextRailEnabled"),"1")?"On":"Off");
 Com_sprintf(contextRailCenter,sizeof(contextRailCenter),"Placement: %s",!strcmp(Cvar_VariableString("ui_jar_contextRailCenter"),"1")?"Centered":"Random grid column");
 Com_sprintf(contextWaypointState,sizeof(contextWaypointState),"Waypoint: %s",!strcmp(Cvar_VariableString("ui_jar_contextWaypointEnabled"),"1")?"On":"Off");
 Q_strncpyz(contextWaypointStatus,Cvar_VariableString("jar_contextWaypointStatus"),sizeof(contextWaypointStatus));
 Com_sprintf(contextSceneLoop,sizeof(contextSceneLoop),"Repeat route: %s",!strcmp(Cvar_VariableString("ui_jar_contextSceneLoop"),"1")?"On":"Off");
 Com_sprintf(contextSkyState,sizeof(contextSkyState),"Viewpoint: %s",!strcmp(Cvar_VariableString("ui_jar_contextSkyEnabled"),"1")?"On":"Off");
 Com_sprintf(contextSkyEntities,sizeof(contextSkyEntities),"Entities in sky: %s",!strcmp(Cvar_VariableString("ui_jar_contextSkyEntities"),"1")?"On":"Off");
 const int combatFlags=atoi(Cvar_VariableString("ui_jar_contextCombatFlags"));const char*combatLabels[]={"Duck","Flee","Skip investigation","Squad","Lean","Snipe"};for(int i=0;i<6;++i)Com_sprintf(contextCombatFlags[i],sizeof(contextCombatFlags[i]),"%s: %s",combatLabels[i],combatFlags&(1<<i)?"On":"Off");
 Com_sprintf(contextCombatState,sizeof(contextCombatState),"Point: %s",!strcmp(Cvar_VariableString("ui_jar_contextCombatEnabled"),"1")?"On":"Off");
 Q_strncpyz(contextCombatStatus,Cvar_VariableString("jar_contextCombatStatus"),sizeof(contextCombatStatus));
 Com_sprintf(contextReferenceState,sizeof(contextReferenceState),"Reference: %s",!strcmp(Cvar_VariableString("ui_jar_contextReferenceEnabled"),"1")?"On":"Off");
 Com_sprintf(contextPoseRange,sizeof(contextPoseRange),"Frames: 0 - %d",std::max(0,atoi(Cvar_VariableString("jar_contextPoseLast"))));
 Com_sprintf(contextTravelMode,sizeof(contextTravelMode),"Travel: %s",!strcmp(Cvar_VariableString("ui_jar_contextTravelHub"),"1")?"Hub (save map state)":"Normal transition");
 Q_strncpyz(contextLabel,Cvar_VariableString("jar_contextLabel"),sizeof(contextLabel));
 Q_strncpyz(contextLocation,Cvar_VariableString("jar_contextLocation"),sizeof(contextLocation));
 Q_strncpyz(contextInfo,token?Cvar_VariableString("jar_contextInfo"):"",sizeof(contextInfo));
 Q_strncpyz(sandboxOverlayStatus,Cvar_VariableString("jar_workshopStatus"),sizeof(sandboxOverlayStatus));
 Com_sprintf(contextScriptReceiver,sizeof(contextScriptReceiver),"Run on: %s",!strcmp(Cvar_VariableString("ui_jar_contextScriptActivator"),"1")?"Activator":"Self");
 Com_sprintf(contextBoundaryPolicy,sizeof(contextBoundaryPolicy),"Empty/damaged: %s",!strcmp(Cvar_VariableString("ui_jar_contextBoundaryPreserve"),"1")?"Ignore":"Destroy");
 Com_sprintf(contextCrystalSolid,sizeof(contextCrystalSolid),"Collision: %s",!strcmp(Cvar_VariableString("ui_jar_contextCrystalSolid"),"1")?"Solid":"Shot-only");
 Com_sprintf(contextDoorState,sizeof(contextDoorState),"Eye: %s",!strcmp(Cvar_VariableString("ui_jar_contextDoorExtended"),"1")?"Extended":"Retracted");
 Q_strncpyz(contextDoorMotion,Cvar_VariableString("jar_contextDoorMotion"),sizeof(contextDoorMotion));
 Com_sprintf(contextWelderState,sizeof(contextWelderState),"Welder: %s",!strcmp(Cvar_VariableString("ui_jar_contextWelderEnabled"),"1")?"On":"Off");
 Com_sprintf(contextSpotMode,sizeof(contextSpotMode),"Aim: %s",!strcmp(Cvar_VariableString("ui_jar_contextSpotTracking"),"1")?"Linked target":"Angles");
 Com_sprintf(contextSpotState,sizeof(contextSpotState),"Light: %s",!strcmp(Cvar_VariableString("ui_jar_contextSpotEnabled"),"1")?"On":"Off");
 Com_sprintf(contextSpotOutput,sizeof(contextSpotOutput),"Link output: %s",!strcmp(Cvar_VariableString("jar_linkOutput"),"1")?"Detection":"Aim");
 const bool portalCamera=!strcmp(Cvar_VariableString("ui_jar_contextPortalCamera"),"1"),portalMirror=!strcmp(Cvar_VariableString("ui_jar_contextPortalMirror"),"1");
 Com_sprintf(contextPortalMode,sizeof(contextPortalMode),"%s: %s",portalCamera?"Look":"View",portalCamera?(portalMirror?"Angles":"Linked target"):portalMirror?"Mirror":"Camera");
 Com_sprintf(contextPortalState,sizeof(contextPortalState),"%s: %s",portalCamera?"Camera":"Panel",!strcmp(Cvar_VariableString("ui_jar_contextPortalEnabled"),"1")?"On":"Off");
 Q_strncpyz(contextPortalLink,Cvar_VariableString("jar_contextPortalLink"),sizeof(contextPortalLink));
 Com_sprintf(contextLaserAction,sizeof(contextLaserAction),"Action: %s",jar::laserArm::Action(atoi(Cvar_VariableString("ui_jar_contextLaserAction"))));
 Q_strncpyz(contextLaserEmitterFiring,atoi(Cvar_VariableString("jar_contextLaserEmitterFiring"))?"Firing: On":"Firing: Off",sizeof(contextLaserEmitterFiring));
 Q_strncpyz(contextLaserEmitterEnabled,atoi(Cvar_VariableString("jar_contextLaserEmitterEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextLaserEmitterEnabled));
 Q_strncpyz(contextLaserEmitterAim,Cvar_VariableString("jar_contextLaserEmitterAim"),sizeof(contextLaserEmitterAim));
 Q_strncpyz(contextTrailAim,Cvar_VariableString("jar_contextTrailAim"),sizeof(contextTrailAim));
 Com_sprintf(contextCloudShape,sizeof(contextCloudShape),"Shape: %s",!strcmp(Cvar_VariableString("ui_jar_contextCloudTube"),"1")?"Hollow":"Disk");
 Com_sprintf(contextCloudStyle,sizeof(contextCloudStyle),"Style: %s",!strcmp(Cvar_VariableString("ui_jar_contextCloudAlternate"),"1")?"Alternate":"Standard");
 Com_sprintf(contextCloudState,sizeof(contextCloudState),"Layer: %s",!strcmp(Cvar_VariableString("ui_jar_contextCloudEnabled"),"1")?"On":"Off");
 const auto weatherKind=static_cast<jar::weather::Kind>(atoi(Cvar_VariableString("ui_jar_contextWeatherKind")));
 Com_sprintf(contextWeatherMode,sizeof(contextWeatherMode),"Mode: %s",jar::weather::Mode(weatherKind,atoi(Cvar_VariableString("ui_jar_contextWeatherMode"))));
 Com_sprintf(contextWeatherState,sizeof(contextWeatherState),"Weather: %s",!strcmp(Cvar_VariableString("ui_jar_contextWeatherEnabled"),"1")?"On":"Off");
 Q_strncpyz(contextWeatherHint,weatherKind==jar::weather::WindZone?"Inside volume: local winds add together.":weatherKind==jar::weather::Exposure?"Inside volume: latest applied rule wins.":"Latest enabled controller per type wins.",sizeof(contextWeatherHint));
 Q_strncpyz(contextWeatherHint2,weatherKind>=jar::weather::WindZone?"Resize in Properties; rotate with Physgun.":"Authored map weather stays active.",sizeof(contextWeatherHint2));
 auto*m=Menus_FindByName("jarEntityContext");if(!m)return;
 const bool sight=token&&!strcmp(Cvar_VariableString("jar_contextVisibleAllowed"),"1");
 if(contextPage==21&&!sight)contextPage=0;
 const bool distance=token&&!strcmp(Cvar_VariableString("jar_contextDistanceAllowed"),"1");
 if(contextPage==22&&!distance)contextPage=0;
 const bool slide=token&&!strcmp(Cvar_VariableString("jar_contextSlideAllowed"),"1");
 if(contextPage==23&&!slide)contextPage=0;
 const bool lift=slide&&!strcmp(Cvar_VariableString("jar_contextSlideLift"),"1");
 Q_strncpyz(contextSlideTab,lift?"Lift":"Door",sizeof(contextSlideTab));
 Q_strncpyz(contextSlideAction,lift?"Raise / lower":"Open / close",sizeof(contextSlideAction));
 Q_strncpyz(contextSlideEnabled,atoi(Cvar_VariableString("jar_contextSlideEnabled"))?"Activation: Enabled":"Activation: Disabled",sizeof(contextSlideEnabled));
 const bool button=token&&!strcmp(Cvar_VariableString("jar_contextButtonAllowed"),"1");
 if(contextPage==24&&!button)contextPage=0;
 Com_sprintf(contextButtonEnabled,sizeof(contextButtonEnabled),"Activation: %s",atoi(Cvar_VariableString("jar_contextButtonEnabled"))?"Enabled":"Disabled");
 Com_sprintf(contextButtonOutput,sizeof(contextButtonOutput),"Link output: %s",atoi(Cvar_VariableString("jar_linkOutput"))==1?"Released":"Pressed");
 Q_strncpyz(contextButtonStatus,Cvar_VariableString("jar_contextButtonStatus"),sizeof(contextButtonStatus));
 const bool bobbing=token&&!strcmp(Cvar_VariableString("jar_contextBobbingAllowed"),"1");
 if(contextPage==25&&!bobbing)contextPage=0;
 Q_strncpyz(contextBobbingMotion,atoi(Cvar_VariableString("jar_contextBobbingRunning"))?"Pause motion":"Resume motion",sizeof(contextBobbingMotion));
 Q_strncpyz(contextBobbingEnabled,atoi(Cvar_VariableString("jar_contextBobbingEnabled"))?"Disable platform":"Enable platform",sizeof(contextBobbingEnabled));
 Q_strncpyz(contextBobbingStatus,Cvar_VariableString("jar_contextBobbingStatus"),sizeof(contextBobbingStatus));
 const int movement=token?atoi(Cvar_VariableString("jar_contextMovementKind")):0;
 const bool actorMovement=movement==jar::movement::Actor;
 const bool customGravity=atoi(Cvar_VariableString("jar_contextMovementCustomGravity"))!=0;
 const bool movementEnabled=atoi(Cvar_VariableString("jar_contextMovementEnabled"))!=0;
 if(contextPage==26&&!movement)contextPage=0;
 const auto logic=static_cast<jar::logic::Kind>(token?atoi(Cvar_VariableString("jar_contextLogicKind")):0);
 const bool counter=logic==jar::logic::Counter;
 const bool timer=logic==jar::logic::Timer;
 const bool random=logic==jar::logic::Random;
 const bool relay=logic==jar::logic::Relay;
 const bool score=logic==jar::logic::Score;
 const bool kill=logic==jar::logic::Kill;
 const bool give=logic==jar::logic::Give;
 const bool gate=jar::logic::UseGate(logic);
 const bool pending=atoi(Cvar_VariableString("jar_contextLogicPending"))!=0;
 const bool logicEnabled=atoi(Cvar_VariableString("jar_contextLogicEnabled"))!=0;
 if(contextPage==27&&!logic)contextPage=0;
 const int contact=token?atoi(Cvar_VariableString("jar_contextContactKind")):0;
 if(contextPage==28&&!contact)contextPage=0;
 const int push=token?atoi(Cvar_VariableString("jar_contextPushKind")):0;
 if(contextPage==29&&!push)contextPage=0;
 const int pushAim=atoi(Cvar_VariableString("jar_contextPushAim"));
 Q_strncpyz(contextPushAim,pushAim==jar::push::Linked?"Aim: Linked direction":
  pushAim==jar::push::Toward?"Aim: Toward linked target":"Aim: Entity orientation",sizeof(contextPushAim));
 const int pushSubject=atoi(Cvar_VariableString("jar_contextPushSubject"));
 const char* const pushSubjects[]={"Affects: Player and NPCs","Affects: Player","Affects: NPCs","Affects: Props","Affects: All"};
 Q_strncpyz(contextPushSubject,pushSubjects[pushSubject>=0&&pushSubject<jar::push::SubjectCount?pushSubject:0],sizeof(contextPushSubject));
 Q_strncpyz(contextPushAdd,atoi(Cvar_VariableString("jar_contextPushAdd"))?"Velocity: Add":"Velocity: Replace",sizeof(contextPushAdd));
 Q_strncpyz(contextPushEnabled,atoi(Cvar_VariableString("jar_contextPushEnabled"))?"Disable push":"Enable push",sizeof(contextPushEnabled));
 Q_strncpyz(contextPushVehicles,atoi(Cvar_VariableString("jar_contextPushVehicles"))?"Vehicles: Enabled":"Vehicles: Disabled",sizeof(contextPushVehicles));
 Q_strncpyz(contextPushTarget,Cvar_VariableString("jar_contextPushTarget"),sizeof(contextPushTarget));
 const int teleport=token?atoi(Cvar_VariableString("jar_contextTeleportKind")):0;
 if(contextPage==30&&!teleport)contextPage=0;
 if(contextPage==31&&!station)contextPage=0;
 if(contextPage==32&&!breakable)contextPage=0;
 if(contextPage==33&&!usable)contextPage=0;
 if(contextPage==34&&!speaker)contextPage=0;
 if(contextPage==35&&!angular)contextPage=0;
 const bool vehicle=token&&atoi(Cvar_VariableString("jar_contextVehicleAllowed"));
 const bool npcStatus=token&&atoi(Cvar_VariableString("jar_contextNPCAllowed"));
 const bool npcTeams=npcStatus&&atoi(Cvar_VariableString("jar_contextNPCTeamsAllowed"));
 const bool npcFire=token&&atoi(Cvar_VariableString("jar_contextNPCFireAllowed"));
 const bool npcAlternate=npcFire&&atoi(Cvar_VariableString("jar_contextNPCAltAllowed"));
 const bool npcWeapon=token&&atoi(Cvar_VariableString("jar_contextNPCWeaponChoices"));
 const bool npcOrder=token&&atoi(Cvar_VariableString("jar_contextNPCOrderAllowed"));
 if(contextPage==45&&!npcStatus&&!npcFire)contextPage=0;
 if(contextPage==46&&!npcWeapon)contextPage=0;
 if(contextPage==47&&!npcOrder)contextPage=0;
 Com_sprintf(contextNPCOrderLabel,sizeof(contextNPCOrderLabel),"Order: %s",jar::npcOrder::names[ContextNPCOrderMode()]);
 Q_strncpyz(contextNPCGoalLabel,Cvar_VariableString("jar_contextNPCGoalStatus"),sizeof(contextNPCGoalLabel));
 Q_strncpyz(contextNPCPatrolLabel,Cvar_VariableString("jar_contextNPCPatrolStatus"),sizeof(contextNPCPatrolLabel));
 const auto* heldWeapon=jar::npcWeapon::Name(atoi(Cvar_VariableString("jar_contextNPCWeapon")));
 const auto* chosenWeapon=jar::npcWeapon::Name(contextNPCWeapon);
 Com_sprintf(contextNPCWeaponHeld,sizeof(contextNPCWeaponHeld),"Holding: %s",heldWeapon?heldWeapon:"");
 Com_sprintf(contextNPCWeaponChoice,sizeof(contextNPCWeaponChoice),"Equip: %s",chosenWeapon?chosenWeapon:"");
 Com_sprintf(contextNPCTeamLabel,sizeof(contextNPCTeamLabel),"Team: %s",contextNPCTeams[ContextNPCTeam("ui_jar_contextNPCTeam")]);
 Com_sprintf(contextNPCTargetLabel,sizeof(contextNPCTargetLabel),"Targets: %s",contextNPCTeams[ContextNPCTeam("ui_jar_contextNPCTargetTeam")]);
 Q_strncpyz(contextNPCFireLabel,atoi(Cvar_VariableString("ui_jar_contextNPCAlternate"))?"Fire mode: Alternate":"Fire mode: Primary",sizeof(contextNPCFireLabel));
 Q_strncpyz(contextNPCAmmoSupply,atoi(Cvar_VariableString("ui_jar_contextNPCManualAmmo"))?"Ammo supply: Manual":"Ammo supply: Native refill",sizeof(contextNPCAmmoSupply));
 const bool mountedVehicle=vehicle&&atoi(Cvar_VariableString("jar_contextMountedVehicle"));
 if(mountedVehicle)contextPage=36;
 if(contextPage==36&&!vehicle)contextPage=0;
 const bool shooter=token&&atoi(Cvar_VariableString("jar_contextShooterAllowed"));
 if(contextPage==37&&!shooter)contextPage=0;
 const bool secret=token&&atoi(Cvar_VariableString("jar_contextSecretAllowed"));
 const bool secretFound=secret&&atoi(Cvar_VariableString("jar_contextSecretFound"));
 if(contextPage==38&&!secret)contextPage=0;
 const bool replicator=token&&atoi(Cvar_VariableString("jar_contextReplicatorAllowed"));
 if(contextPage==39&&!replicator)contextPage=0;
 const bool pickupLifetime=token&&atoi(Cvar_VariableString("jar_contextPickupLifetimeAllowed"));
 if(contextPage==40&&!pickupLifetime)contextPage=0;
 const bool deployedShield=token&&atoi(Cvar_VariableString("jar_contextDeployedShieldAllowed"));
 if(contextPage==41&&!deployedShield)contextPage=0;
 Q_strncpyz(contextDeployedShieldStatus,Cvar_VariableString("jar_contextDeployedShieldStatus"),sizeof(contextDeployedShieldStatus));
 const bool mountedCharge=token&&atoi(Cvar_VariableString("jar_contextMountedChargeAllowed"));
 const bool mountedChargeEditable=mountedCharge&&atoi(Cvar_VariableString("jar_contextMountedChargeEditable"));
 const bool mountedChargeProximity=mountedCharge&&atoi(Cvar_VariableString("jar_contextMountedChargeProximity"));
 if(contextPage==42&&!mountedCharge)contextPage=0;
 const int ropeCount=token?atoi(Cvar_VariableString("jar_contextRopeCount")):0;
 if(contextPage==43&&!ropeCount)contextPage=0;
 uint32_t thrusterId=0;
 const bool hasThruster=token&&jar::context::Token(Cvar_VariableString("jar_contextThrusterId"),thrusterId);
 if(contextPage==44&&!hasThruster)contextPage=0;
 Q_strncpyz(contextThrusterEnabled,atoi(Cvar_VariableString("ui_jar_contextThrusterEnabled"))?"Power: On":"Power: Off",sizeof(contextThrusterEnabled));
 Q_strncpyz(contextThrusterDirection,atoi(Cvar_VariableString("ui_jar_contextThrusterReverse"))?"Direction: Reverse":"Direction: Keep",sizeof(contextThrusterDirection));
 Q_strncpyz(contextRopeLabel,Cvar_VariableString("jar_contextRopeLabel"),sizeof(contextRopeLabel));
 Q_strncpyz(contextRopeEnd,Cvar_VariableString("jar_contextRopeEnd"),sizeof(contextRopeEnd));
 Q_strncpyz(contextMountedChargeStatus,Cvar_VariableString("jar_contextMountedChargeStatus"),sizeof(contextMountedChargeStatus));
 const int replicatorPhase=atoi(Cvar_VariableString("jar_contextReplicatorPhase"));
 Q_strncpyz(contextReplicatorStatus,replicatorPhase==jar::replicator::Forming?"Forming":replicatorPhase==jar::replicator::Ready?"Ready to collect":"Idle",sizeof(contextReplicatorStatus));
 Q_strncpyz(contextReplicatorEnabled,atoi(Cvar_VariableString("jar_contextReplicatorEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextReplicatorEnabled));
 Q_strncpyz(contextSecretMode,atoi(Cvar_VariableString("jar_contextSecretRepeat"))?"Mode: Repeat":"Mode: Once",sizeof(contextSecretMode));
 Q_strncpyz(contextSecretStatus,secretFound?"Discovered":"Ready",sizeof(contextSecretStatus));
 Q_strncpyz(contextSecretEnabled,atoi(Cvar_VariableString("jar_contextSecretEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextSecretEnabled));
 Q_strncpyz(contextShooterAlternate,atoi(Cvar_VariableString("jar_contextShooterAlternate"))?"Fire: Alternate":"Fire: Primary",sizeof(contextShooterAlternate));
 Q_strncpyz(contextShooterRepeat,atoi(Cvar_VariableString("jar_contextShooterRepeat"))?"Mode: Repeat":"Mode: Single shot",sizeof(contextShooterRepeat));
 Q_strncpyz(contextShooterEnabled,atoi(Cvar_VariableString("jar_contextShooterEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextShooterEnabled));
 for(int i=0;i<jar::vehicleStatus::fields;++i)
  Com_sprintf(contextVehicleResourceLabels[i],sizeof(contextVehicleResourceLabels[i]),"%s (max %d): ",
   vehicleResourceNames[i],atoi(Cvar_VariableString(va("jar_contextVehicleMax%d",i))));
 Q_strncpyz(contextAngularTab,swing?"Pendulum":"Spinner",sizeof(contextAngularTab));
 Q_strncpyz(contextAngularAxis,jar::sandboxAngular::AxisLabel(atoi(Cvar_VariableString("jar_contextAngularAxis"))),sizeof(contextAngularAxis));
 Q_strncpyz(contextAngularStatus,Cvar_VariableString("jar_contextAngularStatus"),sizeof(contextAngularStatus));
 Q_strncpyz(contextAngularEnabled,atoi(Cvar_VariableString("jar_contextAngularEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextAngularEnabled));
 Q_strncpyz(contextAngularRun,atoi(Cvar_VariableString("jar_contextAngularRunning"))?"Pause":"Start",sizeof(contextAngularRun));
 Q_strncpyz(contextSpeakerCue,Cvar_VariableString("jar_contextSpeakerCue"),sizeof(contextSpeakerCue));
 Q_strncpyz(contextSpeakerMode,Cvar_VariableString("jar_contextSpeakerMode"),sizeof(contextSpeakerMode));
 Q_strncpyz(contextSpeakerState,Cvar_VariableString("jar_contextSpeakerState"),sizeof(contextSpeakerState));
 Q_strncpyz(contextSpeakerEnabled,atoi(Cvar_VariableString("jar_contextSpeakerEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextSpeakerEnabled));
 Q_strncpyz(contextUsableStatus,Cvar_VariableString("jar_contextUsableStatus"),sizeof(contextUsableStatus));
 Q_strncpyz(contextUsableMode,atoi(Cvar_VariableString("jar_contextUsableMode"))?"Mode: Pulse":"Mode: Toggle",sizeof(contextUsableMode));
 Q_strncpyz(contextUsableDamage,jar::sandboxUsable::DamageLabel(atoi(Cvar_VariableString("jar_contextUsableDamage"))),sizeof(contextUsableDamage));
 Q_strncpyz(contextUsableDirect,jar::sandboxUsable::DirectLabel(atoi(Cvar_VariableString("jar_contextUsableDirect"))),sizeof(contextUsableDirect));
 Q_strncpyz(contextUsableSolid,atoi(Cvar_VariableString("jar_contextUsablePending"))?"Cancel return":
  atoi(Cvar_VariableString("jar_contextUsableSolid"))?"Solid: On":"Solid: Off",sizeof(contextUsableSolid));
 Q_strncpyz(contextUsableEnabled,atoi(Cvar_VariableString("jar_contextUsableEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextUsableEnabled));
 const bool usableSecondary=atoi(Cvar_VariableString("jar_linkOutput"))==1;
 Q_strncpyz(contextUsableRole,usableSecondary?"Link: On":"Link: Off / Pulse",sizeof(contextUsableRole));
 Q_strncpyz(contextUsableOutput,Cvar_VariableString(usableSecondary?"jar_contextUsableOutput1":"jar_contextUsableOutput0"),sizeof(contextUsableOutput));
 Q_strncpyz(contextBreakableStatus,Cvar_VariableString("jar_contextBreakableStatus"),sizeof(contextBreakableStatus));
 Q_strncpyz(contextBreakableMode,jar::sandboxBreakable::Mode(atoi(Cvar_VariableString("jar_contextBreakableMode"))),sizeof(contextBreakableMode));
 Q_strncpyz(contextBreakableMaterial,jar::sandboxBreakable::Material(atoi(Cvar_VariableString("jar_contextBreakableMaterial"))),sizeof(contextBreakableMaterial));
 Q_strncpyz(contextBreakablePlayerUse,atoi(Cvar_VariableString("jar_contextBreakablePlayerUse"))?"Player Use: On":"Player Use: Off",sizeof(contextBreakablePlayerUse));
 Q_strncpyz(contextBreakableEnabled,atoi(Cvar_VariableString("jar_contextBreakableEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextBreakableEnabled));
 Q_strncpyz(contextBreakableExplosion,atoi(Cvar_VariableString("jar_contextBreakableExplosion"))?"Effect: On":"Effect: Off",sizeof(contextBreakableExplosion));
 Com_sprintf(contextStationCharge,sizeof(contextStationCharge),"Charge: %d / %d",
  atoi(Cvar_VariableString("jar_contextStationCharge")),atoi(Cvar_VariableString("jar_contextStationCapacity")));
 Q_strncpyz(contextStationUnlimited,atoi(Cvar_VariableString("jar_contextStationUnlimited"))?"Unlimited: On":"Unlimited: Off",sizeof(contextStationUnlimited));
 Q_strncpyz(contextStationEnabled,atoi(Cvar_VariableString("jar_contextStationEnabled"))?"Enabled: On":"Enabled: Off",sizeof(contextStationEnabled));
 const int teleportSubject=atoi(Cvar_VariableString("jar_contextTeleportSubject"));
 Q_strncpyz(contextTeleportSubject,teleportSubject==jar::teleport::Player?"Affects: Player":
  teleportSubject==jar::teleport::NPCs?"Affects: NPCs":teleportSubject==jar::teleport::NoActors?
  "Affects: No actors":"Affects: Player and NPCs",sizeof(contextTeleportSubject));
 Q_strncpyz(contextTeleportView,atoi(Cvar_VariableString("jar_contextTeleportView"))?"View: Keep":"View: Face destination",sizeof(contextTeleportView));
 Q_strncpyz(contextTeleportVelocity,atoi(Cvar_VariableString("jar_contextTeleportVelocity"))?"Velocity: Keep":"Velocity: Stop",sizeof(contextTeleportVelocity));
 Q_strncpyz(contextTeleportEnabled,atoi(Cvar_VariableString("jar_contextTeleportEnabled"))?"Disable teleport":"Enable teleport",sizeof(contextTeleportEnabled));
 Q_strncpyz(contextTeleportTarget,Cvar_VariableString("jar_contextTeleportTarget"),sizeof(contextTeleportTarget));
 Q_strncpyz(contextTeleportProps,atoi(Cvar_VariableString("jar_contextTeleportProps"))?"Props: Enabled":"Props: Disabled",sizeof(contextTeleportProps));
 Q_strncpyz(contextTeleportProjectiles,atoi(Cvar_VariableString("jar_contextTeleportProjectiles"))?"Projectiles: Enabled":"Projectiles: Disabled",sizeof(contextTeleportProjectiles));
 Q_strncpyz(contextTeleportVehicles,atoi(Cvar_VariableString("jar_contextTeleportVehicles"))?"Vehicles: Enabled":"Vehicles: Disabled",sizeof(contextTeleportVehicles));
 Q_strncpyz(contextTeleportAssemblies,atoi(Cvar_VariableString("jar_contextTeleportAssemblies"))?"Linked groups: On":"Linked groups: Off",sizeof(contextTeleportAssemblies));
 Q_strncpyz(contextTeleportFrozenProps,atoi(Cvar_VariableString("jar_contextTeleportFrozenProps"))?"Frozen props: On":"Frozen props: Off",sizeof(contextTeleportFrozenProps));
 Q_strncpyz(contextTeleportMovers,atoi(Cvar_VariableString("jar_contextTeleportMovers"))?"Movers: On":"Movers: Off",sizeof(contextTeleportMovers));
 Q_strncpyz(contextTeleportWorldAnchors,atoi(Cvar_VariableString("jar_contextTeleportWorldAnchors"))?"World anchors: On":"World anchors: Off",sizeof(contextTeleportWorldAnchors));
 const int subject=atoi(Cvar_VariableString("jar_contextContactSubject"));
 Q_strncpyz(contextContactSubject,subject==jar::contactTrigger::Player?"Contact: Player":
  subject==jar::contactTrigger::NPCs?"Contact: NPCs":"Contact: Player and NPCs",sizeof(contextContactSubject));
 Q_strncpyz(contextContactUse,atoi(Cvar_VariableString("jar_contextContactUse"))?"Contact: Hold Use":"Contact: Touch",sizeof(contextContactUse));
 Q_strncpyz(contextContactEnabled,atoi(Cvar_VariableString("jar_contextContactEnabled"))?"Disable trigger":"Enable trigger",sizeof(contextContactEnabled));
 Q_strncpyz(contextContactStatus,Cvar_VariableString("jar_contextContactStatus"),sizeof(contextContactStatus));
 Q_strncpyz(contextContactArmed,atoi(Cvar_VariableString("jar_contextContactArmed"))?"Disarm damage":"Arm damage",sizeof(contextContactArmed));
 const int damageMode=atoi(Cvar_VariableString("jar_contextContactMode"));
 Q_strncpyz(contextContactMode,damageMode==jar::contactTrigger::Falling?"Damage type: Falling":
  damageMode==jar::contactTrigger::Electrical?"Damage type: Electrical":"Damage type: Normal",sizeof(contextContactMode));
 Q_strncpyz(contextLogicTab,counter?"Counter":timer?"Timer":random?"Random":
  relay?"Relay":score?"Score":kill?"Kill":give?"Give":logic==jar::logic::Enable?"Enable":gate?"Disable":"Delay",sizeof(contextLogicTab));
 const int killMode=atoi(Cvar_VariableString("jar_contextLogicKillMode"));
 Q_strncpyz(contextKillMode,killMode==jar::logic::Falling?"Mode: Falling":
  killMode==jar::logic::Electrical?"Mode: Electrical":"Mode: Normal",sizeof(contextKillMode));
 Q_strncpyz(contextTimerRun,pending?"Stop timer":"Start timer",sizeof(contextTimerRun));
 const int slot=ContextLogicSlot(logic);
 Com_sprintf(contextLogicSlot,sizeof(contextLogicSlot),give?"Link: Pickup %d":gate?"Link: Destination %d":"Link: Output %d",slot+1);
 Q_strncpyz(contextLogicTarget,Cvar_VariableString(va("jar_contextLogicOutput%d",slot)),sizeof(contextLogicTarget));
 Q_strncpyz(contextLogicEnabled,timer?(logicEnabled?"Mute output":"Unmute output"):
  logicEnabled?"Disable marker":"Enable marker",sizeof(contextLogicEnabled));
 Q_strncpyz(contextLogicRepeat,atoi(Cvar_VariableString("jar_contextLogicRepeat"))?"Mode: Repeat":"Mode: Once",sizeof(contextLogicRepeat));
 Q_strncpyz(contextLogicOutput,atoi(Cvar_VariableString("jar_linkOutput"))==1?"Link: Progress":"Link: Complete",sizeof(contextLogicOutput));
 if(counter){
  const int remaining=atoi(Cvar_VariableString("jar_contextLogicRemaining"));
  if(remaining)Com_sprintf(contextLogicStatus,sizeof(contextLogicStatus),"%d hits remaining",remaining);
  else Q_strncpyz(contextLogicStatus,"Complete. Reset to rearm.",sizeof(contextLogicStatus));
 }else if(random){
  Q_strncpyz(contextLogicStatus,atoi(Cvar_VariableString("jar_contextLogicRemaining"))?"Armed":"Spent. Rearm to use again.",sizeof(contextLogicStatus));
 }else if(relay){
  const bool armed=atoi(Cvar_VariableString("jar_contextLogicRemaining"))!=0;
  const bool cooling=atoi(Cvar_VariableString("jar_contextLogicCooling"))!=0;
  Q_strncpyz(contextLogicStatus,!logicEnabled?"Disabled":pending?"Output pending":
   !armed?"Spent. Rearm to use again.":cooling?"Cooling down":"Ready",sizeof(contextLogicStatus));
 }else if(give){
  Q_strncpyz(contextLogicStatus,logicEnabled?"Gives pickups to the activating character.":"Disabled",sizeof(contextLogicStatus));
 }else if(gate){
  Q_strncpyz(contextLogicStatus,!logicEnabled?"Disabled":
   logic==jar::logic::Enable?"Allows use on linked entities.":"Blocks use on linked entities.",sizeof(contextLogicStatus));
 }else if(score||kill){
  Q_strncpyz(contextLogicStatus,logicEnabled?"Applies to the activating character.":"Disabled",sizeof(contextLogicStatus));
 }else if(timer){
  Q_strncpyz(contextLogicStatus,pending?(logicEnabled?"Running":"Running with output muted"):"Stopped",sizeof(contextLogicStatus));
 }else Q_strncpyz(contextLogicStatus,pending?"Output pending":"Ready",sizeof(contextLogicStatus));
 Q_strncpyz(contextMovementEnabled,movementEnabled?"Disable marker":"Enable marker",sizeof(contextMovementEnabled));
 Q_strncpyz(contextMovementStatus,actorMovement?(customGravity?"Custom gravity":"Gravity follows the map"):
  movementEnabled?"Applies to the actor that activates it.":"Disabled",sizeof(contextMovementStatus));
 const char* directions[]={"Up","Left","Right","Down"};
 const int direction=std::min(JAR_SLIDE_DIRECTIONS-1,std::max(0,atoi(Cvar_VariableString("ui_jar_contextSlideDirection"))));
 Com_sprintf(contextSlideDirection,sizeof(contextSlideDirection),"Direction: %s",directions[direction]);
 const bool secondary=atoi(Cvar_VariableString("jar_linkOutput"))==1;
 Com_sprintf(contextSlideOutput,sizeof(contextSlideOutput),"Link output: %s",lift?(secondary?"Lowered":"Raised"):(secondary?"Closed":"Opened"));
 Q_strncpyz(contextSlideStatus,Cvar_VariableString("jar_contextSlideStatus"),sizeof(contextSlideStatus));
 const int sightFlags=atoi(Cvar_VariableString("jar_contextVisibleFlags"));
 Com_sprintf(contextVisibleWalls,sizeof(contextVisibleWalls),"Ignore walls: %s",sightFlags&JAR_VISIBLE_IGNORE_WALLS?"On":"Off");
 Com_sprintf(contextVisibleSight,sizeof(contextVisibleSight),"Require Force Sight: %s",sightFlags&JAR_VISIBLE_FORCE_SIGHT?"On":"Off");
 Com_sprintf(contextVisibleEnabled,sizeof(contextVisibleEnabled),"Enabled: %s",!strcmp(Cvar_VariableString("jar_contextVisibleEnabled"),"1")?"On":"Off");
 const int subjects=atoi(Cvar_VariableString("jar_contextDistanceSubjects"));
 const int reference=atoi(Cvar_VariableString("jar_contextDistanceReference"));
 const int npc=atoi(Cvar_VariableString("jar_contextDistanceSubject"));
 const int linkRole=atoi(Cvar_VariableString("jar_linkOutput"));
 static const char*const roles[]={"Inside","Outside","Reference","NPC"};
 Com_sprintf(contextDistanceSubjects,sizeof(contextDistanceSubjects),"Subjects: %s",subjects==3?"Either":subjects==JAR_DISTANCE_NPC?"NPC":"Player");
 Q_strncpyz(contextDistanceArmed,!strcmp(Cvar_VariableString("jar_contextDistanceArmed"),"1")?"Armed - click to disarm":"Disarmed - click to arm",sizeof(contextDistanceArmed));
 Com_sprintf(contextDistanceReference,sizeof(contextDistanceReference),"Reference: %s",reference==1?"Linked":reference==2?"Missing":"This marker");
 Com_sprintf(contextDistanceNPC,sizeof(contextDistanceNPC),"NPC: %s",npc==1?"Linked":npc==2?"Missing":"Choose a subject");
 Com_sprintf(contextDistanceLinkRole,sizeof(contextDistanceLinkRole),"Link role: %s",roles[linkRole>=0&&linkRole<JAR_DISTANCE_LINKS?linkRole:0]);
 Q_strncpyz(contextWallVisible,!strcmp(Cvar_VariableString("jar_contextWallVisible"),"1")?"Wall: Solid":"Wall: Off",sizeof(contextWallVisible));
 Q_strncpyz(contextWallEnabled,!strcmp(Cvar_VariableString("jar_contextWallEnabled"),"1")?"Activation: Enabled":"Activation: Disabled",sizeof(contextWallEnabled));
 Q_strncpyz(contextSpaceArmed,!strcmp(Cvar_VariableString("jar_contextSpaceArmed"),"1")?"Vacuum: On":"Vacuum: Off",sizeof(contextSpaceArmed));
 Q_strncpyz(contextSpaceEnabled,!strcmp(Cvar_VariableString("jar_contextSpaceEnabled"),"1")?"Enabled: On":"Enabled: Off",sizeof(contextSpaceEnabled));
 if(auto*b=Menu_FindItemByName(m,"tabReflection")){if(token&&!strcmp(Cvar_VariableString("jar_contextReflectionAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabRail")){if(token&&!strcmp(Cvar_VariableString("jar_contextRailAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabWaypoint")){if(token&&!strcmp(Cvar_VariableString("jar_contextWaypointAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabScene")){if(token&&!strcmp(Cvar_VariableString("jar_contextSceneAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabSky")){if(token&&!strcmp(Cvar_VariableString("jar_contextSkyAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabCombat")){if(token&&!strcmp(Cvar_VariableString("jar_contextCombatAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabReference")){if(token&&!strcmp(Cvar_VariableString("jar_contextReferenceAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabPose")){if(token&&!strcmp(Cvar_VariableString("jar_contextPoseAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabTravel")){if(token&&!strcmp(Cvar_VariableString("jar_contextTravelAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabCrystal")){if(token&&!strcmp(Cvar_VariableString("jar_contextCrystalAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabDoor")){if(token&&!strcmp(Cvar_VariableString("jar_contextDoorAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabWelder")){if(token&&!strcmp(Cvar_VariableString("jar_contextWelderAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabSpot")){if(token&&!strcmp(Cvar_VariableString("jar_contextSpotAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabPortal")){if(token&&!strcmp(Cvar_VariableString("jar_contextPortalAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabLaser")){if(token&&(laserArm||laserEmitter))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabWeather")){if(token&&!strcmp(Cvar_VariableString("jar_contextWeatherAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabCloud")){if(token&&!strcmp(Cvar_VariableString("jar_contextCloudAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabBoundary")){if(token&&!strcmp(Cvar_VariableString("jar_contextBoundaryAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*b=Menu_FindItemByName(m,"tabScript")){if(token&&!strcmp(Cvar_VariableString("jar_contextScriptAllowed"),"1"))b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 for(int i=0;i<m->itemCount;++i){auto*b=m->items[i];const char*n=b->window.name;
  if(!strcmp(n,"tabVisible")||!strncmp(n,"visible",7)){
   if(sight&&(!strcmp(n,"tabVisible")||contextPage==21))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabDistance")||!strncmp(n,"distance",8)){
   if(distance&&(!strcmp(n,"tabDistance")||contextPage==22))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabSlide")||!strncmp(n,"slide",5)){
   if(slide&&(!strcmp(n,"tabSlide")||(contextPage==23&&(!lift||strcmp(n,"slideDirection")))))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabButton")||!strncmp(n,"button",6)){
   if(button&&(!strcmp(n,"tabButton")||contextPage==24))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabBobbing")||!strncmp(n,"bobbing",7)){
   if(bobbing&&(!strcmp(n,"tabBobbing")||contextPage==25))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabContact")||!strncmp(n,"contact",7)){
   bool visible=contact&&(!strcmp(n,"tabContact")||contextPage==28);
   if(!strncmp(n,"contactRepeat",13))visible=visible&&contact==jar::contactTrigger::Repeat;
   if(!strncmp(n,"contactHurt",11))visible=visible&&contact==jar::contactTrigger::Hurt;
   if(!strcmp(n,"contactDelay")||!strcmp(n,"contactApply"))visible=visible&&contact!=jar::contactTrigger::Hurt;
   if(!strcmp(n,"contactRearm"))visible=visible&&atoi(Cvar_VariableString("jar_contextContactSpent"));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabPush")||!strncmp(n,"push",4)){
   if(push&&(!strcmp(n,"tabPush")||contextPage==29))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabTeleport")||!strncmp(n,"teleport",8)){
   const bool objectOption=!strcmp(n,"teleportProps")||!strcmp(n,"teleportProjectiles")||
    !strcmp(n,"teleportAssemblies")||!strcmp(n,"teleportFrozenProps")||!strcmp(n,"teleportMovers")||
    !strcmp(n,"teleportWorldAnchors");
   if(teleport&&(!strcmp(n,"tabTeleport")||contextPage==30)&&(!objectOption||teleport==jar::teleport::Volume))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabStation")||!strncmp(n,"station",7)){
   if(station&&(!strcmp(n,"tabStation")||contextPage==31))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabBreakable")||!strncmp(n,"breakable",9)){
   if(breakable&&(!strcmp(n,"tabBreakable")||contextPage==32))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabAngular")||!strncmp(n,"angular",7)){
   const bool visible=angular&&(!strcmp(n,"tabAngular")||(contextPage==35&&
    (strcmp(n,"angularSwing")||swing)&&(strcmp(n,"angularPeriod")||swing)&&(strcmp(n,"angularSpeed")||!swing)));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
   continue;
  }
  if(!strcmp(n,"tabSpeaker")||!strncmp(n,"speaker",7)){
   if(speaker&&(!strcmp(n,"tabSpeaker")||contextPage==34))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
   continue;
  }
  if(!strcmp(n,"tabUsable")||!strncmp(n,"usable",6)){
   if(usable&&(!strcmp(n,"tabUsable")||contextPage==33))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabLogic")||!strncmp(n,"logic",5)){
   bool visible=logic&&(!strcmp(n,"tabLogic")||contextPage==27);
   if(!strncmp(n,"logicCounter",12))visible=visible&&counter;
   if(!strncmp(n,"logicDelay",10))visible=visible&&logic==jar::logic::Delay;
   if(!strncmp(n,"logicTimer",10))visible=visible&&timer;
   if(!strncmp(n,"logicRandom",11))visible=visible&&random;
   if(!strcmp(n,"logicVariation"))visible=visible&&(timer||logic==jar::logic::Delay);
   if(!strncmp(n,"logicRelay",10))visible=visible&&relay;
   if(!strncmp(n,"logicScore",10))visible=visible&&score;
   if(!strncmp(n,"logicKill",9))visible=visible&&kill;
   if(!strncmp(n,"logicSlot",9))visible=visible&&jar::logic::Slotted(logic);
   if(!strcmp(n,"logicApply"))visible=visible&&!jar::logic::Slotted(logic)&&!kill;
   if(!strcmp(n,"logicTimerRun"))visible=visible&&(logicEnabled||pending);
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabPickupLifetime")||!strncmp(n,"pickupLifetime",14)){
   if(pickupLifetime&&(!strcmp(n,"tabPickupLifetime")||contextPage==40))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabDeployedShield")||!strncmp(n,"deployedShield",14)){
   if(deployedShield&&(!strcmp(n,"tabDeployedShield")||contextPage==41))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabMountedCharge")||!strncmp(n,"mountedCharge",13)){
   const bool visible=mountedCharge&&(!strcmp(n,"tabMountedCharge")||(contextPage==42&&
    (!strcmp(n,"mountedChargeStatus")||(mountedChargeEditable&&(strcmp(n,"mountedChargeRemaining")||mountedChargeProximity)))));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"openRopes")||!strncmp(n,"ropeProperty",12)){
   const bool visible=ropeCount>0&&(!strcmp(n,"openRopes")||(contextPage==43&&
    (strcmp(n,"ropePropertyNext")||ropeCount>1)));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"openThruster")||!strncmp(n,"thrusterProperty",16)){
   const bool visible=hasThruster&&(!strcmp(n,"openThruster")||contextPage==44);
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabReplicator")||!strncmp(n,"replicator",10)){
   if(replicator&&(!strcmp(n,"tabReplicator")||contextPage==39))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabSecret")||!strncmp(n,"secret",6)){
   const bool visible=secret&&(!strcmp(n,"tabSecret")||(contextPage==38&&(strcmp(n,"secretRearm")||secretFound)));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabShooter")||!strncmp(n,"shooter",7)){
   if(shooter&&(!strcmp(n,"tabShooter")||contextPage==37))b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabVehicle")||!strncmp(n,"vehicle",7)){
   bool visible=vehicle&&(!strcmp(n,"tabVehicle")||contextPage==36);
   if(visible&&!strncmp(n,"vehicleValue",12))
    visible=atoi(Cvar_VariableString(va("jar_contextVehicleMax%c",n[12])))>0;
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"npcStatus",9)){
   bool visible=!strcmp(n,"npcStatusOpen")?npcStatus||npcFire:npcStatus&&contextPage==45;
   if(!strcmp(n,"npcStatusLocked"))visible=visible&&!npcTeams;
   if(!strcmp(n,"npcStatusTeam")||!strcmp(n,"npcStatusTarget"))visible=visible&&npcTeams;
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"npcFire",7)){
   const bool visible=npcFire&&contextPage==45&&(strcmp(n,"npcFireMode")||npcAlternate);
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"npcWeapon",9)){
   const bool visible=npcWeapon&&(!strcmp(n,"npcWeaponOpen")||contextPage==46);
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"trail",5)){
   const bool visible=trail&&contextPage==0&&(!strcmp(n,"trailAim")||trailLinked);
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"npcOrder",8)){
   bool visible=npcOrder&&(!strcmp(n,"npcOrderOpen")||contextPage==47);
   const auto mode=static_cast<jar::npcOrder::Mode>(ContextNPCOrderMode());
   if(!strcmp(n,"npcOrderDistance"))visible=visible&&mode==jar::npcOrder::Follow;
   if(!strcmp(n,"npcOrderGoal"))visible=visible&&jar::npcOrder::UsesDestination(mode);
   if(!strcmp(n,"npcOrderPatrol"))visible=visible&&mode==jar::npcOrder::Patrol;
   if(!strcmp(n,"npcOrderClearGoal"))visible=visible&&jar::npcOrder::UsesDestination(mode)&&
    atoi(Cvar_VariableString("jar_contextNPCGoalLinked"));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strcmp(n,"tabMovement")||!strncmp(n,"movement",8)){
   const bool visible=movement&&(!strcmp(n,"tabMovement")||(contextPage==26&&
    (!strncmp(n,"movementGravity",15)?actorMovement||movement==jar::movement::Gravity:
     !strncmp(n,"movementFriction",16)?actorMovement||movement==jar::movement::Friction:
     !strcmp(n,"movementMap")?actorMovement&&customGravity:
     !strcmp(n,"movementEnabled")?!actorMovement:true)));
   if(visible)b->window.flags|=WINDOW_VISIBLE;
   else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
  if(!strncmp(n,"reflection",10)||!strncmp(n,"rail",4)||!strncmp(n,"waypoint",8)||!strncmp(n,"scene",5)||!strncmp(n,"sky",3)||!strncmp(n,"combat",6)||!strncmp(n,"reference",9)||!strncmp(n,"pose",4)||!strncmp(n,"travel",6)||!strncmp(n,"edit",4)||!strncmp(n,"apply",5)||!strncmp(n,"action",6)||!strncmp(n,"volume",6)||!strncmp(n,"parm",4)||!strncmp(n,"script",6)||!strncmp(n,"boundary",8)||!strncmp(n,"cloud",5)||!strncmp(n,"weather",7)||!strncmp(n,"laser",5)||!strncmp(n,"portal",6)||!strncmp(n,"spot",4)||!strncmp(n,"welder",6)||!strncmp(n,"door",4)||!strncmp(n,"crystal",7)){
   if(!strcmp(n,"editMusic"))b->descText=!strcmp(Cvar_VariableString("jar_contextMusicPortable"),"1")?"Retail music path, or intro then loop. Activate toggles; removal restores earlier music. Prev/Next chooses one track.":"Retail intro and optional loop path. Activate replaces the music using this map entity's native one-shot behavior.";
   const bool parmItem=!strncmp(n,"parm",4);
   const bool propertyItem=!strncmp(n,"edit",4)||!strncmp(n,"apply",5)||!strncmp(n,"volume",6)||strstr(n,"Music");
   const bool scriptItem=!strncmp(n,"script",6),boundaryItem=!strncmp(n,"boundary",8);
   bool visible=token!=0&&(strncmp(n,"reflection",10)||(contextPage==20&&!strcmp(Cvar_VariableString("jar_contextReflectionAllowed"),"1")))&&(strncmp(n,"rail",4)||(contextPage==19&&!strcmp(Cvar_VariableString("jar_contextRailAllowed"),"1")))&&(strncmp(n,"waypoint",8)||(contextPage==18&&!strcmp(Cvar_VariableString("jar_contextWaypointAllowed"),"1")))&&(strncmp(n,"scene",5)||(contextPage==17&&!strcmp(Cvar_VariableString("jar_contextSceneAllowed"),"1")))&&(strncmp(n,"sky",3)||(contextPage==16&&!strcmp(Cvar_VariableString("jar_contextSkyAllowed"),"1")))&&(strncmp(n,"combat",6)||(contextPage==15&&!strcmp(Cvar_VariableString("jar_contextCombatAllowed"),"1")))&&(strncmp(n,"reference",9)||(contextPage==14&&!strcmp(Cvar_VariableString("jar_contextReferenceAllowed"),"1")))&&(strncmp(n,"pose",4)||(contextPage==13&&!strcmp(Cvar_VariableString("jar_contextPoseAllowed"),"1")))&&(strncmp(n,"travel",6)||(contextPage==12&&!strcmp(Cvar_VariableString("jar_contextTravelAllowed"),"1")))&&((strncmp(n,"crystal",7)!=0)||(contextPage==11&&!strcmp(Cvar_VariableString("jar_contextCrystalAllowed"),"1")))&&((strncmp(n,"door",4)!=0)||(contextPage==10&&!strcmp(Cvar_VariableString("jar_contextDoorAllowed"),"1")))&&((strncmp(n,"welder",6)!=0)||(contextPage==9&&!strcmp(Cvar_VariableString("jar_contextWelderAllowed"),"1")))&&((strncmp(n,"spot",4)!=0)||(contextPage==8&&!strcmp(Cvar_VariableString("jar_contextSpotAllowed"),"1")))&&((strncmp(n,"portal",6)!=0)||(contextPage==7&&!strcmp(Cvar_VariableString("jar_contextPortalAllowed"),"1")))&&(strncmp(n,"laser",5)||(contextPage==6&&(!strncmp(n,"laserEmitter",12)?laserEmitter:laserArm)))&&((strncmp(n,"weather",7)!=0)||(contextPage==5&&!strcmp(Cvar_VariableString("jar_contextWeatherAllowed"),"1")))&&((strncmp(n,"cloud",5)!=0)||(contextPage==4&&!strcmp(Cvar_VariableString("jar_contextCloudAllowed"),"1")))&&(!boundaryItem||(contextPage==3&&!strcmp(Cvar_VariableString("jar_contextBoundaryAllowed"),"1")))&&(!parmItem||contextPage==1)&&(!propertyItem||contextPage==0)&&(!scriptItem||(contextPage==2&&!strcmp(Cvar_VariableString("jar_contextScriptAllowed"),"1")));
   if(!strncmp(n,"railTrack",9)&&strcmp(Cvar_VariableString("jar_contextRailKind"),"1"))visible=false;
   if(!strcmp(n,"railWidth")&&!strcmp(Cvar_VariableString("jar_contextRailKind"),"3"))visible=false;
   if(!strcmp(n,"railCenter")&&strcmp(Cvar_VariableString("jar_contextRailKind"),"3"))visible=false;
   if(!strcmp(n,"sceneSpeed")||!strcmp(n,"sceneWait")||!strcmp(n,"sceneLoop"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextSceneTrack"),"1");
   if(!strcmp(n,"referenceOwner"))visible=visible&&strcmp(Cvar_VariableString("jar_contextReferenceGoal"),"1");
   if(!strcmp(n,"portalWidth")||!strcmp(n,"portalHeight"))visible=visible&&!portalCamera;
   if(!strcmp(n,"portalUnlink"))visible=visible&&atoi(Cvar_VariableString("jar_contextPortalCanUnlink"));
   if(!strcmp(n,"weatherDensity")||!strcmp(n,"weatherSize")||!strcmp(n,"weatherAlpha"))visible=visible&&weatherKind<jar::weather::Wind;
   if(!strcmp(n,"weatherSpeed"))visible=visible&&(weatherKind==jar::weather::Wind||weatherKind==jar::weather::WindZone);
   if(!strcmp(n,"parmEdit")||!strcmp(n,"parmApply")||!strcmp(n,"parmClear"))visible=visible&&!strcmp(Cvar_VariableString(va("jar_contextParmAllowed%u",contextParmSlot)),"1");
   if(!strcmp(n,"editHealth")||!strcmp(n,"applyHealth"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextHealthAllowed"),"1");
   if(!strncmp(n,"editProp",8)||!strncmp(n,"applyProp",9))visible=visible&&propPhysics;
   if(!strcmp(n,"editHolocron")||!strcmp(n,"applyHolocron"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextHolocronAllowed"),"1");
   if(!strcmp(n,"editPickupQuantity")||!strcmp(n,"applyPickupQuantity")){
    visible=visible&&!strcmp(Cvar_VariableString("jar_contextPickupQuantityAllowed"),"1");
    const bool duration=atoi(Cvar_VariableString("jar_contextPickupDuration"))!=0;
    if(!strcmp(n,"editPickupQuantity"))b->text=duration?"Duration (sec): ":"Quantity: ";
    else b->text=duration?"Apply duration":"Apply quantity";
    b->descText=duration?"Whole seconds from 0 to 1000. Zero uses the retail 25 seconds.":
     atoi(Cvar_VariableString("jar_contextPickupWeapon"))?
     "Whole ammo quantity from 0 to 1000. Zero grants the weapon without ammo.":
     "Whole quantity from 0 to 1000. Zero uses the stock item amount.";
   }
   if(!strcmp(n,"editText")||!strcmp(n,"applyText"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextLabelAllowed"),"1");
   if(strstr(n,"Music"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextMusicAllowed"),"1");
   if(strstr(n,"Volume")||!strncmp(n,"volume",6))visible=visible&&!strcmp(Cvar_VariableString("jar_contextVolumeAllowed"),"1");
   if(!strncmp(n,"volumeSpace",11))visible=visible&&!strcmp(Cvar_VariableString("jar_contextSpaceAllowed"),"1");
   if(strstr(n,"Always"))visible=visible&&!strcmp(Cvar_VariableString("jar_contextAlwaysAllowed"),"1");
   if(strstr(n,"Wall"))visible=visible&&contextPage==0&&!strcmp(Cvar_VariableString("jar_contextWallAllowed"),"1");
   if(visible)b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
 }
 // While driving, the native editor can only operate on the current hull's resources.
 for(const char* name:{"world","player","tabProperties","tabParms"})
  if(auto* item=Menu_FindItemByName(m,name))item->window.flags|=WINDOW_VISIBLE;
 if(mountedVehicle){
  for(int i=0;i<m->itemCount;++i){auto* item=m->items[i];const char* name=item->window.name;
   if(strncmp(name,"vehicle",7)&&strcmp(name,"tabVehicle")&&strcmp(name,"panel")&&strcmp(name,"selection")&&
      strcmp(name,"info")&&strcmp(name,"actionRefresh")&&strcmp(name,"undo")&&strcmp(name,"close")&&strcmp(name,"status"))
    item->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
  }
 }
}
static void ContextMenu(){
 if(Menus_FindByName("jarEntityContext"))return;auto*m=Menu_CreateJAR("jarEntityContext");if(!m)return;
 m->fullScreen=qfalse;m->window.style=WINDOW_STYLE_EMPTY;m->window.rect={0,0,640,480};m->onESC="uiScript jarOverlayClose ;";VectorSet4(m->focusColor,.35f,.9f,1,1);
 auto wr=JAR_ContextRect(JC_WORLD,0);auto*world=AddItem(m,"world",nullptr,wr.x,wr.y,wr.w,"uiScript jarContext pick ;");world->window.rectClient.h=wr.h;
 auto pr=JAR_ContextRect(JC_PANEL,0);auto*panel=AddItem(m,"panel",nullptr,pr.x,pr.y,pr.w,nullptr);panel->window.rectClient.h=pr.h;panel->window.style=WINDOW_STYLE_FILLED;panel->window.flags|=WINDOW_BACKCOLORSET;VectorSet4(panel->window.backColor,.04f,.04f,.04f,.95f);
 auto add=[&](const char*n,const char*t,float x,float y,float w,const char*a){auto*b=AddItem(m,n,t,x,y,w,a);b->textscale=.5f;b->window.rectClient.h=20;return b;};
 add("tabProperties","Properties",402,50,72,"uiScript jarContext properties ;");
 add("tabParms","Script vars",476,50,72,"uiScript jarContext parms ;");
 add("tabScript","Script",550,50,68,"uiScript jarContext script_page ;");
 add("tabReflection","Reflect",550,50,68,"uiScript jarContext reflection_page ;");
 add("tabRail","Rail",550,50,68,"uiScript jarContext rail_page ;");
 add("tabWaypoint","Nav",550,50,68,"uiScript jarContext waypoint_page ;");
 add("tabScene","Camera",550,50,68,"uiScript jarContext scene_page ;");
 add("tabSky","Sky",550,50,68,"uiScript jarContext sky_page ;");
 add("tabCombat","Combat",550,50,68,"uiScript jarContext combat_page ;");
 add("tabReference","Tag",550,50,68,"uiScript jarContext reference_page ;");
 add("tabPose","Pose",550,50,68,"uiScript jarContext pose_page ;");
 add("tabTravel","Travel",550,50,68,"uiScript jarContext travel_page ;");
 add("tabCrystal","Blast",550,50,68,"uiScript jarContext crystal_page ;");
 add("tabDoor","Camera",550,50,68,"uiScript jarContext door_page ;");
 add("tabWelder","Welder",550,50,68,"uiScript jarContext welder_page ;");
 add("tabSpot","Light",550,50,68,"uiScript jarContext spot_page ;");
 add("tabPortal","Portal",550,50,68,"uiScript jarContext portal_page ;");
 add("tabLaser","Laser",550,50,68,"uiScript jarContext laser_page ;");
 add("tabWeather","Weather",550,50,68,"uiScript jarContext weather_page ;");
 add("tabCloud","Cloud",550,50,68,"uiScript jarContext cloud_page ;");
 add("tabBoundary","Boundary",550,50,68,"uiScript jarContext boundary_page ;");
 add("tabVisible","Visibility",550,50,68,"uiScript jarContext visible_page ;");
 add("tabDistance","Distance",550,50,68,"uiScript jarContext distance_page ;");
 add("tabSlide",contextSlideTab,550,50,68,"uiScript jarContext slide_page ;");
 add("tabButton","Button",550,50,68,"uiScript jarContext button_page ;");
 add("tabBobbing","Platform",550,50,68,"uiScript jarContext bobbing_page ;");
 add("tabContact","Trigger",550,50,68,"uiScript jarContext contact_page ;");
 add("tabPush","Push",550,50,68,"uiScript jarContext push_page ;");
 add("tabTeleport","Teleport",550,50,68,"uiScript jarContext teleport_page ;");
 add("tabStation","Station",550,50,68,"uiScript jarContext station_page ;");
 add("tabBreakable","Breakable",550,50,68,"uiScript jarContext breakable_page ;");
 add("tabUsable","Usable",550,50,68,"uiScript jarContext usable_page ;");
 add("tabSpeaker","Speaker",550,50,68,"uiScript jarContext speaker_page ;");
 add("tabAngular",contextAngularTab,550,50,68,"uiScript jarContext angular_page ;");
 add("tabLogic",contextLogicTab,550,50,68,"uiScript jarContext logic_page ;");
 add("tabMovement","Movement",550,50,68,"uiScript jarContext movement_page ;");
 add("tabVehicle","Vehicle",550,50,68,"uiScript jarContext vehicle_page ;");
 add("tabShooter","Shooter",550,50,68,"uiScript jarContext shooter_page ;");
 add("tabSecret","Secret",550,50,68,"uiScript jarContext secret_page ;");
 add("tabReplicator","Replicator",550,50,68,"uiScript jarContext replicator_page ;");
 add("tabPickupLifetime","Lifetime",550,50,68,"uiScript jarContext pickup_lifetime_page ;");
 add("tabDeployedShield","Shield",550,50,68,"uiScript jarContext deployed_shield_page ;");
 add("tabMountedCharge","Charge",550,50,68,"uiScript jarContext mounted_charge_page ;");
 add("selection",contextLabel,402,76,216,nullptr)->textscale=.43f;
 add("info",contextInfo,402,96,216,nullptr)->descText=contextLocation;
 add("player","Select player",402,118,102,"uiScript jarContext player ;");
 add("actionRefresh","Refresh",514,118,49,"uiScript jarContext refresh ;");
 auto* thrusterButton=add("openThruster","Thruster",569,118,49,"uiScript jarContext thruster_page ;");
 thrusterButton->textscale=.43f;thrusterButton->descText="Edit the mounted thruster's force, power and direction.";
 auto field=[&](const char*n,const char*label,const char*cvar,float y){
  auto*b=add(n,label,402,y,220,"");b->type=ITEM_TYPE_EDITFIELD;b->cvar=String_Alloc(cvar);
  auto*d=static_cast<editFieldDef_t*>(UI_Alloc(sizeof(editFieldDef_t)));if(d){memset(d,0,sizeof(*d));d->maxChars=14;d->maxPaintChars=14;b->typeData=d;}
  return b;
 };
 for(int i=0;i<jar::vehicleStatus::fields;++i)
  field(va("vehicleValue%d",i),contextVehicleResourceLabels[i],va("ui_jar_contextVehicleValue%d",i),148+26*i);
 add("vehicleApply","Apply vehicle resources",402,310,220,"uiScript jarContext vehicle_resources ;");
 add("npcStatusOpen","NPC settings",402,351,72,"uiScript jarContext npc_page ;");
 add("npcWeaponOpen","NPC weapon",478,351,72,"uiScript jarContext npc_weapon_page ;");
 add("npcOrderOpen","NPC orders",554,351,64,"uiScript jarContext npc_order_page ;");
 add("npcOrderMode",contextNPCOrderLabel,402,148,220,"uiScript jarContext npc_order_mode ;")
  ->descText="Follow requires the Player team. Go moves to one marker. Patrol follows a chain of waypoints.";
 field("npcOrderDistance","Follow distance: ","ui_jar_contextNPCOrderDistance",178)
  ->descText="Desired separation from the player, from 72 to 1024 units.";
 add("npcOrderGoal",contextNPCGoalLabel,402,178,220,nullptr)
  ->descText="With Toolgun Link set to Primary, click the NPC, then its Nav goal or first waypoint.";
 add("npcOrderPatrol",contextNPCPatrolLabel,402,205,220,nullptr)
  ->descText="Link each waypoint to the next with Primary. Link back to the first waypoint to make a loop.";
 add("npcOrderClearGoal","Clear goal link",402,232,220,"uiScript jarContext npc_goal_clear ;");
 add("npcOrderApply","Apply order",402,259,220,"uiScript jarContext npc_order ;");
 add("npcWeaponHeld",contextNPCWeaponHeld,402,148,220,nullptr);
 add("npcWeaponPrevious","Previous",402,180,105,"uiScript jarContext npc_weapon_previous ;");
 add("npcWeaponNext","Next",516,180,102,"uiScript jarContext npc_weapon_next ;");
 add("npcWeaponEquip",contextNPCWeaponChoice,402,212,220,"uiScript jarContext npc_weapon ;")
  ->descText="Equip immediately. This keeps other carried weapons and existing ammo.";
 add("npcStatusTeam",contextNPCTeamLabel,402,148,220,"uiScript jarContext npc_team ;");
 add("npcStatusTarget",contextNPCTargetLabel,402,176,220,"uiScript jarContext npc_target ;")->descText="Free uses the native free-for-all targeting rules.";
 add("npcStatusLocked","Target locked by controller",402,148,220,nullptr);
 field("npcStatusArmor","Armor: ","ui_jar_contextNPCArmor",204)->descText="Whole armor from 0 to 32767.";
 add("npcStatusApply","Apply NPC settings",402,232,220,"uiScript jarContext npc_status ;");
 field("npcFireAmmo","Held weapon ammo: ","ui_jar_contextNPCAmmo",258)->descText="Whole ammo from 0 to 32767. -1 gives unlimited ammo.";
 add("npcFireSupply",contextNPCAmmoSupply,402,281,220,"uiScript jarContext npc_supply ;")->descText="Manual uses the available ammo. Native refill lets AI top up a low pool.";
 add("npcFireMode",contextNPCFireLabel,402,304,220,"uiScript jarContext npc_alt_fire ;");
 add("npcFireApply","Apply weapon settings",402,327,220,"uiScript jarContext npc_fire ;");
 field("shooterInterval","Interval (ms): ","ui_jar_contextShooterInterval",148);
 field("shooterVariation","Variation (ms): ","ui_jar_contextShooterVariation",174);
 add("shooterApply","Apply cadence",402,200,220,"uiScript jarContext shooter_interval ;");
 add("shooterAlternate",contextShooterAlternate,402,228,220,"uiScript jarContext shooter_alternate ;");
 add("shooterRepeat",contextShooterRepeat,402,256,220,"uiScript jarContext shooter_repeat ;");
 add("shooterEnabled",contextShooterEnabled,402,284,220,"uiScript jarContext shooter_enabled ;");
 add("shooterHint","Link sets the aim target.",402,312,220,nullptr)->textscale=.43f;
 add("secretStatus",contextSecretStatus,402,148,220,nullptr);
 add("secretMode",contextSecretMode,402,176,220,"uiScript jarContext secret_repeat ;");
 add("secretEnabled",contextSecretEnabled,402,204,220,"uiScript jarContext secret_enabled ;");
 add("secretRearm","Rearm",402,232,220,"uiScript jarContext secret_rearm ;");
 add("secretHint","Link Primary to a discovery output.",402,260,220,nullptr)->textscale=.43f;
 add("replicatorStatus",contextReplicatorStatus,402,148,220,nullptr);
 field("replicatorHeal","Healing: ","ui_jar_contextReplicatorHeal",176);
 field("replicatorDelay","Delay (ms): ","ui_jar_contextReplicatorDelay",204);
 add("replicatorApply","Apply settings",402,232,220,"uiScript jarContext replicator_apply ;");
 add("replicatorEnabled",contextReplicatorEnabled,402,260,220,"uiScript jarContext replicator_enabled ;");
 field("pickupLifetimeValue","Remaining (ms): ","ui_jar_contextPickupLifetime",148)->descText=
  "Whole milliseconds from 0 to 600000. Zero keeps this pickup until collected or removed.";
 add("pickupLifetimeApply","Apply lifetime",402,176,220,"uiScript jarContext pickup_lifetime ;");
 add("deployedShieldStatus",contextDeployedShieldStatus,402,148,220,nullptr);
 field("deployedShieldHealth","Health: ","ui_jar_contextDeployedShieldHealth",176);
 field("deployedShieldRemaining","Remaining (ms): ","ui_jar_contextDeployedShieldRemaining",204)->descText="Set up to 25000 ms. Zero keeps the current expiry.";
 field("deployedShieldLeft","Left span (units): ","ui_jar_contextDeployedShieldLeft",232);
 field("deployedShieldRight","Right span (units): ","ui_jar_contextDeployedShieldRight",260);
 field("deployedShieldHeight","Height (units): ","ui_jar_contextDeployedShieldHeight",288);
 add("deployedShieldApply","Apply shield properties",402,316,220,"uiScript jarContext deployed_shield ;");
 add("mountedChargeStatus",contextMountedChargeStatus,402,148,220,nullptr);
 field("mountedChargeHealth","Health: ","ui_jar_contextMountedChargeHealth",176);
 field("mountedChargeDamage","Blast damage: ","ui_jar_contextMountedChargeDamage",204)->descText="Whole number from 0 to 10000.";
 field("mountedChargeRadius","Blast radius: ","ui_jar_contextMountedChargeRadius",232)->descText="Whole number from 0 to 4096 world units.";
 field("mountedChargeRemaining","Remaining (ms): ","ui_jar_contextMountedChargeRemaining",260)->descText="Up to 120000 ms for a proximity mine. Zero keeps its current expiry.";
 add("mountedChargeApply","Apply charge properties",402,288,220,"uiScript jarContext mounted_charge ;");
 add("ropePropertyLabel",contextRopeLabel,402,148,220,nullptr);
 add("ropePropertyEnd",contextRopeEnd,402,173,220,nullptr)->textscale=.4f;
 field("ropePropertyLength","Length: ","ui_jar_contextRopeLength",200)->descText="0.01 to 8192 world units. Shortening can pull released endpoints together.";
 add("ropePropertyApply","Apply rope length",402,228,220,"uiScript jarContext rope_length ;");
 add("ropePropertyNext","Next attached rope",402,256,220,"uiScript jarContext rope_next ;");
 field("thrusterPropertyForce","Force: ","ui_jar_contextThrusterForce",148)->descText="1 to 100000. Uses the existing mount point.";
 add("thrusterPropertyPower",contextThrusterEnabled,402,178,220,"uiScript jarContext thruster_power ;");
 add("thrusterPropertyDirection",contextThrusterDirection,402,208,220,"uiScript jarContext thruster_direction ;")->descText="Reverse force and exhaust when you apply the change.";
 add("thrusterPropertyApply","Apply thruster properties",402,238,220,"uiScript jarContext thruster_properties ;");
 field("visibleRange","Range (32-4096): ","ui_jar_contextVisibleRange",148)->descText="Maximum viewing distance in world units.";
 field("visibleCone","Half-angle (5-85): ","ui_jar_contextVisibleCone",176)->descText="Maximum angle away from the center of the player's view.";
 add("visibleApply","Apply sight settings",402,204,220,"uiScript jarContext visible ;");
 add("visibleWalls",contextVisibleWalls,402,232,220,"uiScript jarContext visible_walls ;")->descText="Skip the line-of-sight trace within the map's potentially visible area.";
 add("visibleSight",contextVisibleSight,402,260,220,"uiScript jarContext visible_sight ;");
 add("visibleEnabled",contextVisibleEnabled,402,288,220,"uiScript jarContext visible_enabled ;");
 add("visibleHint","Link Primary. Activate to rearm.",402,316,220,nullptr)->textscale=.43f;
 field("distanceRange","Range (1-4096): ","ui_jar_contextDistanceRange",148)->descText="Distance in world units. Activate measures once and then disarms.";
 add("distanceApply","Apply range",402,172,220,"uiScript jarContext distance ;");
 add("distanceSubjects",contextDistanceSubjects,402,196,220,"uiScript jarContext distance_subjects ;")->descText="Player, linked NPC, or either. NPC and Either require an NPC link.";
 add("distanceArmed",contextDistanceArmed,402,220,220,"uiScript jarContext distance_arm ;")->descText="Arm here or with an Enable controller. Activate checks the distance.";
 add("distanceReference",contextDistanceReference,402,244,139,nullptr)->textscale=.43f;
 add("distanceNPC",contextDistanceNPC,402,266,139,nullptr)->textscale=.43f;
 add("distanceClearReference","Reset",552,244,66,"uiScript jarContext distance_clear_reference ;");
 add("distanceClearSubject","Clear",552,266,66,"uiScript jarContext distance_clear_subject ;");
 add("distanceLinkRole",contextDistanceLinkRole,402,289,220,nullptr)->textscale=.43f;
 add("distanceLinkInside","Inside",402,310,104,"uiScript jarContext distance_link_0 ;");
 add("distanceLinkOutside","Outside",514,310,104,"uiScript jarContext distance_link_1 ;");
 add("distanceLinkReference","Reference",402,332,104,"uiScript jarContext distance_link_2 ;");
 add("distanceLinkNPC","NPC",514,332,104,"uiScript jarContext distance_link_3 ;");
 add("distanceLinkHint","Choose role, then use the Link tool.",402,355,220,nullptr)->textscale=.4f;
 field("slideTravel","Travel: ","ui_jar_contextSlideTravel",148)->descText="16-384 world units from the rest position.";
 field("slideSpeed","Speed: ","ui_jar_contextSlideSpeed",176)->descText="25-500 world units per second.";
 field("slideWait","Auto-return (ms): ","ui_jar_contextSlideWait",204)->descText="0 toggles manually. Use up to 30000 milliseconds for auto-return.";
 add("slideDirection",contextSlideDirection,402,232,220,"uiScript jarContext slide_direction ;")->descText="Close the door before changing direction.";
 add("slideApply","Apply movement settings",402,260,220,"uiScript jarContext slide ;");
 add("slideActivate",contextSlideAction,402,288,220,"uiScript jarContext activate ;");
 add("slideOutput",contextSlideOutput,402,316,220,"uiScript jarContext slide_output ;")->descText="Choose an output, then connect it with the Link tool.";
 add("slideStatus",contextSlideStatus,402,344,220,nullptr)->textscale=.43f;
 add("slideEnabled",contextSlideEnabled,402,370,220,"uiScript jarContext slide_enabled ;");
 field("buttonWait","Hold (ms): ","ui_jar_contextButtonWait",148)->descText="0-30000 milliseconds between pressing and returning.";
 add("buttonApply","Apply hold delay",402,176,220,"uiScript jarContext button ;");
 add("buttonEnabled",contextButtonEnabled,402,204,220,"uiScript jarContext button_enabled ;");
 add("buttonPress","Press",402,232,220,"uiScript jarContext activate ;");
 add("buttonOutput",contextButtonOutput,402,260,220,"uiScript jarContext button_output ;")->descText="Choose an output, then connect it with the Link tool.";
 add("buttonStatus",contextButtonStatus,402,288,220,nullptr);
 field("bobbingAmplitude","Amplitude: ","ui_jar_contextBobbingAmplitude",148)->descText="Whole amplitude from 1 to 128 world units.";
 field("bobbingPeriod","Cycle (ms): ","ui_jar_contextBobbingPeriod",176)->descText="One full cycle in 500 to 30000 milliseconds.";
 add("bobbingApply","Apply motion settings",402,204,220,"uiScript jarContext bobbing ;");
 add("bobbingMotion",contextBobbingMotion,402,232,220,"uiScript jarContext bobbing_running ;");
 add("bobbingEnabled",contextBobbingEnabled,402,260,220,"uiScript jarContext bobbing_enabled ;");
 add("bobbingStatus",contextBobbingStatus,402,288,220,nullptr)->textscale=.43f;
 field("contactDelay","Delay (ms): ","ui_jar_contextContactDelay",148)->descText="Whole milliseconds from 0 to 60000. Zero fires immediately.";
 field("contactRepeatInterval","Interval (ms): ","ui_jar_contextContactInterval",176)->descText="Time after firing before a new contact can trigger it.";
 field("contactRepeatVariation","Variation (ms): ","ui_jar_contextContactVariation",204)->descText="Random variation keeps the total interval between 100 and 60000 ms.";
 add("contactApply","Apply settings",402,232,220,"uiScript jarContext contact_apply ;");
 field("contactHurtDamage","Damage per hit: ","ui_jar_contextContactDamage",148)->descText="Whole numbers from 1 to 10000.";
 field("contactHurtInterval","Interval (ms): ","ui_jar_contextContactInterval",176)->descText="Each actor has a separate interval, from 100 to 60000 ms.";
 add("contactHurtMode",contextContactMode,402,204,220,"uiScript jarContext contact_hurt_mode ;");
 add("contactHurtApply","Apply settings",402,232,220,"uiScript jarContext contact_hurt_apply ;");
 add("contactSubject",contextContactSubject,402,260,220,"uiScript jarContext contact_subject ;");
 add("contactUse",contextContactUse,402,288,220,"uiScript jarContext contact_use ;")->descText="Choose touch or require the bound Use action while in the volume.";
 add("contactEnabled",contextContactEnabled,402,316,220,"uiScript jarContext contact_enabled ;");
 add("contactRearm","Rearm",402,344,220,"uiScript jarContext contact_rearm ;");
 add("contactHurtArm",contextContactArmed,402,344,220,"uiScript jarContext contact_hurt_arm ;");
 add("contactStatus",contextContactStatus,402,372,220,nullptr)->textscale=.43f;
 field("pushSpeed","Speed: ","ui_jar_contextPushSpeed",148)->descText="Game units per second, from 1 to 2000.";
 field("pushInterval","Interval (ms): ","ui_jar_contextPushInterval",176)->descText="Minimum time between pushes for each actor, prop or vehicle.";
 add("pushApply","Apply settings",402,204,220,"uiScript jarContext push_apply ;");
 add("pushAim",contextPushAim,402,232,220,"uiScript jarContext push_aim ;")->descText="Choose orientation, a fixed direction to the target, or aim from each contact.";
 add("pushSubject",contextPushSubject,402,260,220,"uiScript jarContext push_subject ;");
 add("pushAdd",contextPushAdd,402,288,220,"uiScript jarContext push_add ;");
 add("pushEnabled",contextPushEnabled,402,316,220,"uiScript jarContext push_enabled ;");
 add("pushUnlink","Clear aim link",402,344,220,"uiScript jarContext push_unlink ;");
 add("pushTarget",contextPushTarget,402,372,220,nullptr)->textscale=.43f;
 add("pushVehicles",contextPushVehicles,402,400,220,"uiScript jarContext push_vehicles ;")->descText="Push the hull with its riders seated, independently of the subject setting.";
 field("teleportInterval","Interval (ms): ","ui_jar_contextTeleportInterval",148)->descText="Minimum time between teleports for each traveler.";
 add("teleportApply","Apply interval",402,176,220,"uiScript jarContext teleport_apply ;");
 add("teleportSubject",contextTeleportSubject,402,204,220,"uiScript jarContext teleport_subject ;");
 add("teleportView",contextTeleportView,402,232,220,"uiScript jarContext teleport_view ;")->descText="Keep actor view and projectile direction, or use the destination's facing.";
 add("teleportVelocity",contextTeleportVelocity,402,260,220,"uiScript jarContext teleport_velocity ;")->descText="Choose whether actors, vehicles and released props keep momentum. Projectiles retain speed.";
 add("teleportEnabled",contextTeleportEnabled,402,288,104,"uiScript jarContext teleport_enabled ;")->textscale=.43f;
 auto* teleportAnchors=add("teleportWorldAnchors",contextTeleportWorldAnchors,514,288,108,"uiScript jarContext teleport_world_anchors ;");
 teleportAnchors->textscale=.43f;teleportAnchors->descText="Move linked anchors to matching world surfaces at the exit. A missing surface blocks the group.";
 add("teleportUnlink","Clear destination link",402,316,220,"uiScript jarContext teleport_unlink ;");
 add("teleportTarget",contextTeleportTarget,402,344,220,nullptr)->textscale=.43f;
 auto* teleportProps=add("teleportProps",contextTeleportProps,402,372,104,"uiScript jarContext teleport_props ;");
 teleportProps->textscale=.43f;teleportProps->descText="Allow released props through this volume.";
 auto* teleportAssemblies=add("teleportAssemblies",contextTeleportAssemblies,514,372,108,"uiScript jarContext teleport_assemblies ;");
 teleportAssemblies->textscale=.43f;teleportAssemblies->descText="With Props enabled, move linked props and allowed actors together. Enable Vehicles to include hulls and riders. Every member and rope must fit.";
 auto* teleportProjectiles=add("teleportProjectiles",contextTeleportProjectiles,402,400,104,"uiScript jarContext teleport_projectiles ;");
 teleportProjectiles->textscale=.43f;teleportProjectiles->descText="Allow shots, loose thermals, and thrown trip mines or det packs through this volume.";
 auto* teleportFrozen=add("teleportFrozenProps",contextTeleportFrozenProps,514,400,108,"uiScript jarContext teleport_frozen_props ;");
 teleportFrozen->textscale=.43f;teleportFrozen->descText="With Props and Linked groups enabled, carry frozen props with their group. They stay frozen at the destination.";
 auto* teleportVehicles=add("teleportVehicles",contextTeleportVehicles,402,423,104,"uiScript jarContext teleport_vehicles ;");
 teleportVehicles->textscale=.43f;teleportVehicles->descText="Move the vehicle and its riders together when every arrival fits.";
 auto* teleportMovers=add("teleportMovers",contextTeleportMovers,514,423,108,"uiScript jarContext teleport_movers ;");
 teleportMovers->textscale=.43f;teleportMovers->descText="Carry linked Sandbox movers and map sections. Supported map doors, platforms and buttons take their team and automatic triggers.";
 add("stationStatus",contextStationCharge,402,148,220,nullptr)->descText="Current finite reserve. It is retained when unlimited supply is on.";
 field("stationCapacity","Capacity: ","ui_jar_contextStationCapacity",176)->descText="1 to 10000. Lowering capacity clamps the reserve.";
 field("stationRecharge","Recharge (ms): ","ui_jar_contextStationRecharge",204)->descText="0 disables recharge. Set 100 to 60000 ms per unit.";
 add("stationApply","Apply capacity and recharge",402,232,220,"uiScript jarContext station_apply ;");
 field("stationCharge","Set charge: ","ui_jar_contextStationCharge",260)->descText="A whole number from zero to capacity.";
 add("stationApplyCharge","Apply charge",402,288,104,"uiScript jarContext station_charge ;");
 add("stationRefill","Refill",514,288,108,"uiScript jarContext station_refill ;");
 add("stationUnlimited",contextStationUnlimited,402,316,220,"uiScript jarContext station_unlimited ;");
 add("stationEnabled",contextStationEnabled,402,344,220,"uiScript jarContext station_enabled ;")->descText="Pause both dispensing and recharge.";
 add("breakableStatus",contextBreakableStatus,402,148,220,nullptr);
 field("breakableCapacity","Max health: ","ui_jar_contextBreakableCapacity",174)->descText="1 to 10000. Repair fills the current health.";
 field("breakableFuse","Fuse (ms): ","ui_jar_contextBreakableFuse",200)->descText="0 to 60000 ms after a lethal hit or activation. Zero breaks immediately.";
 field("breakableChunks","Debris (%): ","ui_jar_contextBreakableChunks",226)->descText="0 to 300 percent of the native fragment count.";
 field("breakableSplash","Splash damage: ","ui_jar_contextBreakableSplash",252)->descText="0 to 1000 damage when the radius is above zero.";
 field("breakableRadius","Splash radius: ","ui_jar_contextBreakableRadius",278)->descText="0 to 1024 world units.";
 add("breakableApply","Apply breakable settings",402,304,220,"uiScript jarContext breakable_apply ;");
 auto* breakMode=add("breakableMode",contextBreakableMode,402,330,104,"uiScript jarContext breakable_mode ;");
 breakMode->textscale=.43f;breakMode->descText="Cycle which weapons can break it. Tool and linked activation still work.";
 auto* breakMaterial=add("breakableMaterial",contextBreakableMaterial,514,330,108,"uiScript jarContext breakable_material ;");
 breakMaterial->textscale=.43f;breakMaterial->descText="Cycle the stock debris material.";
 add("breakableUse",contextBreakablePlayerUse,402,356,104,"uiScript jarContext breakable_use ;")->textscale=.43f;
 add("breakableEnabled",contextBreakableEnabled,514,356,108,"uiScript jarContext breakable_enabled ;")->descText="Disable new damage and activation. An armed fuse still runs.";
 add("breakableExplosion",contextBreakableExplosion,402,382,104,"uiScript jarContext breakable_explosion ;")->descText="Toggle the native explosion effect. Debris and splash have their own settings.";
 add("breakableRepair","Repair",514,382,108,"uiScript jarContext breakable_repair ;")->descText="Cancel the fuse or restore the body at full health if its space is clear.";
 add("usableStatus",contextUsableStatus,402,148,220,nullptr)->textscale=.43f;
 field("usableCapacity","Max health: ","ui_jar_contextUsableCapacity",176)->descText="0 to 10000. Zero disables damage activation.";
 field("usableReuse","Pulse reuse (ms): ","ui_jar_contextUsableReuse",204)->descText="0 to 60000. Zero fires once until rearmed.";
 add("usableApply","Apply health and reuse",402,232,220,"uiScript jarContext usable_apply ;");
 add("usableMode",contextUsableMode,402,260,104,"uiScript jarContext usable_mode ;")->descText="Toggle changes solidity. Pulse fires Primary and keeps solidity.";
 add("usableSolid",contextUsableSolid,514,260,108,"uiScript jarContext usable_solid ;")->descText="Set solidity or cancel a blocked return without firing outputs. Turning on requires clear space.";
 add("usableDirect",contextUsableDirect,402,288,104,"uiScript jarContext usable_direct ;")->textscale=.4f;
 add("usableDamage",contextUsableDamage,514,288,108,"uiScript jarContext usable_damage ;")->textscale=.43f;
 add("usableEnabled",contextUsableEnabled,402,316,104,"uiScript jarContext usable_enabled ;")->textscale=.43f;
 add("usableRearm","Rearm",514,316,108,"uiScript jarContext usable_rearm ;")->descText="Restore health and reset a spent or cooling pulse.";
 add("usableRole",contextUsableRole,402,344,104,"uiScript jarContext usable_role ;")->textscale=.43f;
 add("usableUnlink","Clear link",514,344,108,"uiScript jarContext usable_unlink ;");
 add("usableOutput",contextUsableOutput,402,372,220,nullptr)->descText="Choose an output, then connect a destination with Link.";
 add("speakerCue",contextSpeakerCue,402,148,220,nullptr)->textscale=.43f;
 add("speakerPrev","Previous sound",402,176,104,"uiScript jarContext speaker_prev ;");
 add("speakerNext","Next sound",514,176,108,"uiScript jarContext speaker_next ;");
 add("speakerMode",contextSpeakerMode,402,204,220,"uiScript jarContext speaker_mode ;")->descText="Cycle spatial, global, activator, loop and repeat playback.";
 field("speakerPeriod","Repeat interval (ms): ","ui_jar_contextSpeakerPeriod",232)->descText="500 to 60000 ms. Only applies in Spatial repeat mode.";
 field("speakerVariation","Variation (ms): ","ui_jar_contextSpeakerVariation",260)->descText="Random variation on either side of the interval, with a minimum 500 ms gap.";
 add("speakerApply","Apply timing",402,288,220,"uiScript jarContext speaker_apply ;");
 add("speakerEnabled",contextSpeakerEnabled,402,316,104,"uiScript jarContext speaker_enabled ;")->descText="Pause playback and block linked activation until enabled.";
 add("speakerState",contextSpeakerState,514,316,108,nullptr)->textscale=.43f;
 add("speakerHint","Use or Activate to play. Loops toggle.",402,344,220,nullptr)->textscale=.38f;
 add("angularStatus",contextAngularStatus,402,148,220,nullptr)->textscale=.43f;
 field("angularSpeed","Speed (deg/s): ","ui_jar_contextAngularRate",176)->descText="-180 to 180, excluding zero. Negative reverses the spin.";
 field("angularSwing","Swing (degrees): ","ui_jar_contextAngularRate",176)->descText="1 to 75 degrees on each side of the swing center.";
 field("angularPeriod","Cycle (ms): ","ui_jar_contextAngularPeriod",204)->descText="750 to 30000 ms for a full swing cycle.";
 add("angularApply","Apply motion",402,232,220,"uiScript jarContext angular_apply ;");
 add("angularAxis",contextAngularAxis,402,260,220,"uiScript jarContext angular_axis ;")->descText="Cycle the crate's local rotation axis.";
 add("angularRun",contextAngularRun,402,288,104,"uiScript jarContext angular_running ;");
 add("angularEnabled",contextAngularEnabled,514,288,108,"uiScript jarContext angular_enabled ;")->descText="Disable motion and linked activation. Enabling resumes its saved setting.";
 add("angularHint","Use or Activate toggles motion.",402,316,220,nullptr)->textscale=.4f;
 field("logicDelayMs","Delay (ms): ","ui_jar_contextLogicDelay",148)->descText="Whole milliseconds from 0 to 60000. Zero fires on the next simulation tick.";
 field("logicTimerMs","Interval (ms): ","ui_jar_contextLogicDelay",148)->descText="Whole milliseconds from 100 to 60000.";
 field("logicVariation","Variation (ms): ","ui_jar_contextLogicJitter",176)->descText="A random amount added or subtracted. Keep the total between 0 and 60000 ms.";
 field("logicCounterHits","Hits to complete: ","ui_jar_contextLogicHits",148)->descText="Changing the required count resets progress.";
 add("logicSlotSelect",contextLogicSlot,402,148,220,"uiScript jarContext logic_next_slot ;")->descText="Choose a slot, then connect its destination using the Link tool.";
 add("logicSlotDestination",contextLogicTarget,402,176,220,nullptr)->textscale=.43f;
 add("logicRandomRepeat",contextLogicRepeat,402,232,220,"uiScript jarContext logic_repeat ;")->descText="Once spends the marker after firing an output. Repeat keeps it armed.";
 add("logicRandomReset","Rearm",402,260,220,"uiScript jarContext logic_reset ;");
 add("logicSlotClear","Clear this link",402,316,220,"uiScript jarContext logic_clear_output ;");
 add("logicTimerRun",contextTimerRun,402,232,220,"uiScript jarContext logic_running ;")->descText="Starting sends an output immediately. Stopping cancels the next tick.";
 field("logicRelayDelay","Delay (ms): ","ui_jar_contextLogicDelay",148)->descText="Whole milliseconds from 0 to 60000. Zero fires immediately.";
 field("logicRelayCooldown","Cooldown (ms): ","ui_jar_contextLogicCooldown",176)->descText="Minimum time after firing before it accepts another activation.";
 add("logicRelayRepeat",contextLogicRepeat,402,232,220,"uiScript jarContext logic_repeat ;")->descText="Once spends the relay after its output fires.";
 add("logicRelayReset","Rearm",402,260,220,"uiScript jarContext logic_reset ;");
 field("logicScorePoints","Points: ","ui_jar_contextLogicPoints",148)->descText="Whole points from -10000 to 10000. Negative values subtract points.";
 add("logicKillMode",contextKillMode,402,148,220,"uiScript jarContext logic_kill_mode ;")->descText="Choose Normal, Falling, or Electrical. Activate acts on the player.";
 add("logicApply","Apply settings",402,204,220,"uiScript jarContext logic_apply ;");
 add("logicCounterRepeat",contextLogicRepeat,402,232,220,"uiScript jarContext logic_repeat ;")->descText="Repeat rearms the counter after completion.";
 add("logicCounterReset","Reset progress",402,260,220,"uiScript jarContext logic_reset ;");
 add("logicEnabled",contextLogicEnabled,402,288,220,"uiScript jarContext logic_enabled ;");
 add("logicCounterOutput",contextLogicOutput,402,316,220,"uiScript jarContext logic_output ;")->descText="Choose an output, then connect it with the Link tool. Progress fires on each hit before completion.";
 add("logicStatus",contextLogicStatus,402,344,220,nullptr)->textscale=.43f;
 field("movementGravity","Gravity: ","ui_jar_contextMovementGravity",148)->descText="Whole gravity from 0 to 3200.";
 field("movementFriction","Ground friction: ","ui_jar_contextMovementFriction",176)->descText="Whole ground friction from 0 to 10.";
 add("movementGravityApply","Apply gravity",402,204,220,"uiScript jarContext movement_gravity ;");
 add("movementFrictionApply","Apply friction",402,232,220,"uiScript jarContext movement_friction ;");
 add("movementMap","Use map gravity",402,260,220,"uiScript jarContext movement_map ;");
 add("movementEnabled",contextMovementEnabled,402,260,220,"uiScript jarContext movement_enabled ;");
 add("movementStatus",contextMovementStatus,402,288,220,nullptr)->textscale=.43f;
 field("railTrackLength","Length: ","ui_jar_contextRailLength",148)->descText="Track length: 128-8192 units.";
 field("railWidth","Width: ","ui_jar_contextRailWidth",176)->descText="Track/lane width: 64-2048 units. Move a lane sideways to choose its columns.";
 field("railTrackCell","Grid: ","ui_jar_contextRailCell",204)->descText="Whole grid size: 32-256 units, no wider than the track.";
 field("railTrackSpeed","Speed: ","ui_jar_contextRailSpeed",232)->descText="Mover speed: 16-2000 units/second.";
 field("railTrackCount","Per row: ","ui_jar_contextRailCount",260)->descText="Launch up to 1-8 waiting movers per grid interval.";
 field("railDelay","Delay: ","ui_jar_contextRailDelay",288)->descText="0-30 seconds. Track/lane activation delay, or mover delay between trips.";
 add("railEnabled",contextRailState,402,316,220,"uiScript jarContext rail_enabled ;");
 add("railCenter",contextRailCenter,402,204,220,"uiScript jarContext rail_center ;");
 add("railApply","Apply rail settings",402,344,220,"uiScript jarContext rail ;");
 add("railHint","Link: lane to track; mover to either.",402,380,220,nullptr)->textscale=.38f;
 add("railHint2","Riders prevent recycling at the end.",402,404,220,nullptr)->textscale=.38f;
 field("waypointRange","Auto range: ","ui_jar_contextWaypointRange",148)->descText="0-1024 units. Zero keeps only explicit Primary/Secondary links.";
 add("waypointEnabled",contextWaypointState,402,180,220,"uiScript jarContext waypoint_enabled ;");
 add("waypointApply","Apply navigation",402,212,220,"uiScript jarContext waypoint ;");
 add("waypointStatus",contextWaypointStatus,402,252,220,nullptr)->textscale=.35f;
 add("waypointHint","Place near the floor for walking AI.",402,284,220,nullptr)->textscale=.4f;
 add("waypointHint2","Link: Primary / Secondary adds routes.",402,308,220,nullptr)->textscale=.4f;
 add("waypointHint3","Walls and unsupported gaps block travel.",402,332,220,nullptr)->textscale=.4f;
 add("waypointHint4","Activate toggles. Physgun moves points.",402,356,220,nullptr)->textscale=.4f;
 field("sceneTurn","Turn deg/s: ","ui_jar_contextSceneTurn",148);
 field("sceneSpeed","Travel units/s: ","ui_jar_contextSceneSpeed",180);
 field("sceneWait","Corner wait sec: ","ui_jar_contextSceneWait",212);
 add("sceneLoop",contextSceneLoop,402,244,220,"uiScript jarContext scene_loop ;");
 add("sceneApply","Apply camera settings",402,276,220,"uiScript jarContext scene ;");
 add("sceneHint","Activate to view; USE or move exits.",402,308,220,nullptr)->textscale=.4f;
 add("sceneHint2","Track: move the supplied path points.",402,332,220,nullptr)->textscale=.4f;
 add("sceneHint3","Track Secondary link: choose focus.",402,356,220,nullptr)->textscale=.4f;
 add("sceneHint4","Focus Primary link: choose subject.",402,380,220,nullptr)->textscale=.4f;
 add("skyEnabled",contextSkyState,402,148,220,"uiScript jarContext sky_enabled ;");
 add("skyEntities",contextSkyEntities,402,180,220,"uiScript jarContext sky_entities ;");
 add("skyApply","Apply sky view",402,212,220,"uiScript jarContext sky ;");
 add("skyHint","Move and rotate with the Physgun.",402,260,220,nullptr)->textscale=.42f;
 add("skyHint2","Latest enabled viewpoint wins.",402,284,220,nullptr)->textscale=.42f;
 add("skyHint3","Changes the view through map sky surfaces.",402,308,220,nullptr)->textscale=.42f;
 add("skyHint4","Off restores the previous sky.",402,332,220,nullptr)->textscale=.42f;
 for(int i=0;i<6;++i)add(va("combatFlag%d",i),contextCombatFlags[i],402,148+i*24,220,va("uiScript jarContext combat_flag%d ;",i))->descText="Native selection flags. Skip investigation excludes this point from investigation searches; other flags depend on NPC behavior.";
 add("combatEnabled",contextCombatState,402,292,220,"uiScript jarContext combat_enabled ;");
 add("combatApply","Apply combat point",402,320,220,"uiScript jarContext combat ;");
 add("combatStatus",contextCombatStatus,402,350,220,nullptr)->textscale=.43f;
 add("combatHint","Native AI decides when to use it.",402,372,220,nullptr)->textscale=.43f;
 for(const char*key:{"Name","Owner"}){auto*b=field(va("reference%s",key),!strcmp(key,"Name")?"Tag: ":"Owner: ",va("ui_jar_contextReference%s",key),!strcmp(key,"Name")?148:180);if(b->typeData){auto*d=static_cast<editFieldDef_t*>(b->typeData);d->maxChars=31;d->maxPaintChars=22;}b->descText="1-31 letters, digits, underscores or hyphens. Tag names ignore case. Owner is an exact script ownername; blank is world scope.";}
 field("referenceRadius","Goal radius: ","ui_jar_contextReferenceRadius",212)->descText="Whole radius 0-1024 used by native NPC navgoal scripts. Navigation goals always use world scope.";
 add("referenceEnabled",contextReferenceState,402,244,220,"uiScript jarContext reference_enabled ;");
 add("referenceApply","Apply tag",402,276,220,"uiScript jarContext reference ;");
 add("referenceHint","Link sets its look-at target.",402,308,220,nullptr)->textscale=.43f;
 add("referenceHint2","Copy creates a unique tag name.",402,330,220,nullptr)->textscale=.43f;
 field("poseFrame","Frame: ","ui_jar_contextPoseFrame",148)->descText="Static pose from this model's animation frames. Apply updates visible geometry and collision together. Move into clear space if blocked.";
 add("poseRange",contextPoseRange,402,180,220,nullptr);
 add("posePrevious","Previous",402,214,104,"uiScript jarContext pose_prev ;");
 add("poseNext","Next",514,214,104,"uiScript jarContext pose_next ;");
 add("poseApply","Apply frame",402,246,220,"uiScript jarContext pose ;");
 add("poseReset","Reset to frame 0",402,278,220,"uiScript jarContext pose_reset ;");
 add("poseHint","Copy and full saves keep this pose.",402,310,220,nullptr)->textscale=.43f;
 add("poseHint2","Revert property restores the previous frame.",402,332,220,nullptr)->textscale=.43f;
 auto*travelMap=field("travelMap","Map: ","ui_jar_contextTravelMap",148);if(travelMap->typeData){auto*d=static_cast<editFieldDef_t*>(travelMap->typeData);d->maxChars=jar::levelChange::mapLimit;d->maxPaintChars=24;}
 travelMap->descText="Installed map name, for example t1_surprise. Omit maps/ and .bsp. Spawn/edit never travels; Activate does.";
 auto*travelSpawn=field("travelSpawn","Spawn: ","ui_jar_contextTravelSpawn",180);if(travelSpawn->typeData){auto*d=static_cast<editFieldDef_t*>(travelSpawn->typeData);d->maxChars=jar::levelChange::spawnLimit;d->maxPaintChars=24;}
 travelSpawn->descText="Optional named player spawn point in the destination map. Blank uses its default start.";
 add("travelMode",contextTravelMode,402,214,220,"uiScript jarContext travel_hub ;")->textscale=.43f;
 add("travelApply","Apply destination",402,246,220,"uiScript jarContext travel ;");
 add("travelHint","Activate below changes map.",402,278,220,nullptr)->textscale=.43f;
 add("travelHint2","Hub restores a saved destination.",402,300,220,nullptr)->textscale=.43f;
 add("travelHint3","Revert settings before departure.",402,322,220,nullptr)->textscale=.43f;
 const char*axes[]={"X: ","Y: ","Z: "};const char*angles[]={"Pitch: ","Yaw: ","Roll: "};
 for(int i=0;i<3;++i){field(va("editPosition%d",i),axes[i],va("ui_jar_contextPosition%d",i),148+20.f*i);field(va("editAngle%d",i),angles[i],va("ui_jar_contextAngle%d",i),234+20.f*i);}
 add("applyPosition","Apply position",402,210,212,"uiScript jarContext position ;");
 add("applyAngles","Apply angles",402,296,212,"uiScript jarContext angles ;");
 auto*textField=field("editText","Label: ","ui_jar_contextText",318);textField->window.rectClient.w=144;
 if(textField->typeData){auto*d=static_cast<editFieldDef_t*>(textField->typeData);d->maxChars=63;d->maxPaintChars=18;}
 textField->descText="Name this location or edit this feedback marker text. 1-63 characters; no quotes, semicolons or backslashes.";
 add("applyText","Apply label",550,318,68,"uiScript jarContext label ;")->textscale=.43f;
 auto*musicField=field("editMusic","Music: ","ui_jar_contextText",322);if(musicField->typeData){auto*d=static_cast<editFieldDef_t*>(musicField->typeData);d->maxChars=jar::music::textLimit;d->maxPaintChars=24;}
 musicField->descText="Retail music path, or intro path then loop path. Activate toggles playback. Prev/Next selects a single track. Removing the controller restores prior music.";
 add("actionMusicPrevious","Prev",402,346,68,"uiScript jarContext music_prev ;");
 add("applyMusic","Apply",476,346,68,"uiScript jarContext music ;");
 add("actionMusicNext","Next",550,346,68,"uiScript jarContext music_next ;");
 add("actionWallVisible",contextWallVisible,402,322,216,"uiScript jarContext wall_visible ;")->descText="An off wall remains selectable as a gizmo. Clear its space before making it solid.";
 add("actionWallEnabled",contextWallEnabled,402,346,216,"uiScript jarContext wall_enabled ;");
 field("editHealth","Health: ","ui_jar_contextHealth",322);
 field("editHolocron","Force level: ","ui_jar_contextHolocronLevel",322)->descText="Grant level 1, 2 or 3 when picked up. Never lowers an already known power. Revert property restores this pickup's level.";
 field("editPickupQuantity","Quantity: ","ui_jar_contextPickupQuantity",322);
 add("applyPickupQuantity","Apply quantity",402,346,212,"uiScript jarContext pickup_quantity ;");
 add("applyHolocron","Apply Force level",402,346,212,"uiScript jarContext holocron ;");
 add("applyHealth","Apply health",402,346,212,"uiScript jarContext health ;");
 field("editPropMass","Mass: ","ui_jar_contextPropMass",322)->descText="1 to 100000. Set 0 to use the model's default mass.";
 add("applyPropMass","Apply mass",402,346,104,"uiScript jarContext prop_physics ;");
 add("applyPropGravity",contextPropGravity,514,346,104,"uiScript jarContext prop_gravity ;")->descText="Toggle map gravity for this prop. A frozen prop stays frozen until released.";
 const char*dimensions[]={"W: ","D: ","H: "};
 for(int i=0;i<3;++i){
  auto*b=field(va("editVolume%d",i),dimensions[i],va("ui_jar_contextVolume%d",i),339);b->window.rectClient.x=402+i*74.f;b->window.rectClient.w=70;b->window.rectClient.h=16;b->textscale=.43f;
  if(b->typeData)static_cast<editFieldDef_t*>(b->typeData)->maxPaintChars=7;
  b->descText="Width, depth and height in local axes (8-8192 units). Apply angles or Physgun USE rotation turns the volume. Resizing retains its lower center.";
 }
 add("applyVolume","Apply dimensions",402,357,212,"uiScript jarContext volume ;")->window.rectClient.h=16;
 add("volumeSpaceArmed",contextSpaceArmed,402,317,104,"uiScript jarContext space_armed ;")->descText="Turn the vacuum on or off. Revert property restores the previous setting.";
 add("volumeSpaceEnabled",contextSpaceEnabled,514,317,104,"uiScript jarContext space_enabled ;")->descText="Enable or disable the controller while keeping its vacuum setting.";
 field("editAlwaysDelay","Delay (ms): ","ui_jar_contextAlwaysDelay",320)->descText="50 to 60000 ms. Applies to the next shot. Activate arms one shot. Link connects its output.";
 add("applyAlwaysDelay","Apply delay",402,346,212,"uiScript jarContext always_delay ;");
 add("trailAim",contextTrailAim,402,379,216,nullptr)->textscale=.43f;
 add("trailUnlink","Aim forward",402,405,216,"uiScript jarContext trail_unlink ;")->descText="Clear its target. Rotate the emitter to change the launch direction. Primary Link can set a target again.";
 add("parmLabel",contextParmLabel,402,148,220,nullptr);
 add("parmPrevious","Prev",402,172,104,"uiScript jarContext parm_prev ;");
 add("parmNext","Next",514,172,104,"uiScript jarContext parm_next ;");
 auto*parmField=field("parmEdit","Value: ","ui_jar_contextParm",206);
 if(parmField->typeData){auto*d=static_cast<editFieldDef_t*>(parmField->typeData);d->maxChars=jar::parms::limit;d->maxPaintChars=24;}
 parmField->descText="Up to 63 printable characters. Apply stores literal text. A parameter controller copies nonempty slots to its activator; +5 or -2 changes that activator's numeric value.";
 add("parmApply","Apply parameter",402,234,212,"uiScript jarContext parm ;");
 add("parmClear","Clear parameter",402,260,212,"uiScript jarContext parm_clear ;")->descText="Clear this slot. Parameter controllers skip empty slots when activated. Revert property restores the previous value.";
 add("parmHint","Script variables; empty is normal.",402,296,220,nullptr)->textscale=.4f;
 add("parmUndoHint","Use Properties for position / health.",402,320,220,nullptr)->textscale=.4f;
 field("boundaryTime","Turn time (sec): ","ui_jar_contextBoundaryTime",148)->descText="Steer toward the linked marker for 0.1-60 seconds. Remaining contacts do not renew the timer; leaving rearms the volume.";
 add("boundaryPolicy",contextBoundaryPolicy,402,176,220,"uiScript jarContext boundary_policy ;")->descText="Ignore leaves empty or damaged fighters unchanged. Destroy applies the native boundary destruction behavior to them.";
 add("boundaryApply","Apply boundary properties",402,204,220,"uiScript jarContext boundary ;");
 add("boundaryHint1","Move its destination with the Physgun.",402,246,220,nullptr)->textscale=.4f;
 add("boundaryHint2","Link selects a different destination.",402,272,220,nullptr)->textscale=.4f;
 add("boundaryHint3","Properties edits volume size and rotation.",402,298,220,nullptr)->textscale=.37f;
 add("boundaryHint4","Activate toggles the boundary.",402,324,220,nullptr)->textscale=.4f;
 auto*scriptField=field("scriptPath","Script: ","ui_jar_contextScript",148);
 if(scriptField->typeData){auto*d=static_cast<editFieldDef_t*>(scriptField->typeData);d->maxChars=jar::scriptRunner::textLimit;d->maxPaintChars=24;}
 scriptField->descText="Choose a retail compiled script or enter an installed relative script path. Campaign scripts can require their original map's entities. Apply changes properties; Activate runs it.";
 add("scriptPrevious","Prev",402,172,104,"uiScript jarContext script_prev ;");
 add("scriptNext","Next",514,172,104,"uiScript jarContext script_next ;");
 add("scriptReceiver",contextScriptReceiver,402,198,212,"uiScript jarContext script_receiver ;")->descText="Self runs on this controller. Activator runs on the player when using Activate here.";
 field("scriptDelay","Delay (sec): ","ui_jar_contextScriptDelay",224);
 field("scriptWait","Cooldown (sec): ","ui_jar_contextScriptWait",248);
 field("scriptCount","Uses left: ","ui_jar_contextScriptCount",272)->descText="-1 means unlimited; 0 means exhausted. 1-100000 limits the remaining activations.";
 add("scriptApply","Apply runner properties",402,302,212,"uiScript jarContext script ;");
 add("scriptHint","Apply cancels a pending activation.",402,328,220,nullptr)->textscale=.4f;
 add("scriptUndoHint","Revert property restores script settings.",402,350,220,nullptr)->textscale=.37f;
 field("crystalHitDamage","Hit damage: ","ui_jar_contextCrystalHitDamage",148)->descText="Whole damage per hit pulse, 0-1000. Pulses are limited to one per 0.1 second to prevent recursive chain reactions.";
 field("crystalHitRadius","Hit radius: ","ui_jar_contextCrystalHitRadius",178)->descText="1-1024 world units, with native damage falloff and line of sight.";
 field("crystalBlastDamage","Blast damage: ","ui_jar_contextCrystalBlastDamage",208)->descText="Whole damage on destruction or Activate, 0-1000. Zero retains the visible explosion.";
 field("crystalBlastRadius","Blast radius: ","ui_jar_contextCrystalBlastRadius",238)->descText="Whole radius, 1-1024 world units.";
 add("crystalSolid",contextCrystalSolid,402,268,212,"uiScript jarContext crystal_solid ;");
 add("crystalApply","Apply blast properties",402,298,212,"uiScript jarContext crystal ;");
 add("crystalHint","Activate detonates the crate.",402,328,220,nullptr)->textscale=.4f;
 add("crystalHint2","Health is on the Properties page.",402,350,220,nullptr)->textscale=.39f;
 field("doorTravel","Travel: ","ui_jar_contextDoorTravel",148)->descText="8-256 world units beyond the retracted position. Reducing travel also limits the current extension.";
 field("doorSpeed","Speed: ","ui_jar_contextDoorSpeed",178)->descText="1-512 units per second. The moving eye stops at solid surfaces and actors; it retries when clear.";
 add("doorState",contextDoorState,402,208,212,"uiScript jarContext door_extended ;");
 add("doorApply","Apply camera properties",402,240,212,"uiScript jarContext door ;");
 add("doorMotion",contextDoorMotion,402,278,220,nullptr)->textscale=.4f;
 add("doorHint","Refresh reads the current extension.",402,304,220,nullptr)->textscale=.39f;
 add("doorHint2","Activate extends or retracts the eye.",402,330,220,nullptr)->textscale=.39f;
 add("doorHint3","Physgun moves and rotates every part.",402,352,220,nullptr)->textscale=.37f;
 field("welderReach","Reach: ","ui_jar_contextWelderReach",148)->descText="1-256 world units along the barrel; nearby solid surfaces stop the emission point.";
 field("welderRadius","Damage radius: ","ui_jar_contextWelderRadius",178)->descText="1-256 world units around each spark pulse. Native radial damage checks visibility.";
 field("welderInterval","Interval (sec): ","ui_jar_contextWelderInterval",208)->descText="0.1-10 seconds between pulses. Movement and property edits preserve the current cooldown.";
 field("welderDamage","Damage: ","ui_jar_contextWelderDamage",238)->descText="Whole damage per pulse, 0-1000. Zero is decorative sparks and sound.";
 add("welderState",contextWelderState,402,268,212,"uiScript jarContext welder_enabled ;");
 add("welderApply","Apply welder properties",402,298,212,"uiScript jarContext welder ;");
 add("welderHint","Activate toggles sparks and sound.",402,328,220,nullptr)->textscale=.4f;
 add("welderHint2","Rotate with Physgun USE to aim.",402,350,220,nullptr)->textscale=.4f;
 add("spotMode",contextSpotMode,402,148,212,"uiScript jarContext spot_tracking ;");
 field("spotRange","Range: ","ui_jar_contextSpotRange",174)->descText="64-8192 world units. Solid surfaces stop the cone.";
 field("spotCone","Cone angle: ","ui_jar_contextSpotCone",198)->descText="Half angle in degrees, 1-30. Detection uses this cone and line of sight.";
 field("spotLight","Light radius: ","ui_jar_contextSpotLight",222)->descText="0-1024. Zero hides the impact light while retaining the visible cone.";
 const char*colors[]={"R: ","G: ","B: "};for(int i=0;i<3;++i){auto*b=field(va("spotColor%d",i),colors[i],va("ui_jar_contextSpotColor%d",i),246);b->window.rectClient.x=402+i*74.f;b->window.rectClient.w=70;b->textscale=.43f;if(b->typeData)static_cast<editFieldDef_t*>(b->typeData)->maxPaintChars=5;b->descText="Red, green and blue values from 0 to 1.";}
 add("spotState",contextSpotState,402,272,212,"uiScript jarContext spot_enabled ;");
 add("spotApply","Apply light properties",402,298,212,"uiScript jarContext spot ;");
 add("spotOutput",contextSpotOutput,402,324,212,"uiScript jarContext spot_output ;")->descText="Choose Aim or Detection here, then use the Link tool to select the spotlight and a spawned destination.";
 add("spotHint","Detection fires once when you enter.",402,350,220,nullptr)->textscale=.38f;
 add("portalMode",contextPortalMode,402,148,212,"uiScript jarContext portal_mode ;")->descText="Use Primary Link from a panel to a camera, or from a camera to its aim target. Panels can also show a mirror. Cameras can use their own angles.";
 field("portalWidth","Width: ","ui_jar_contextPortalWidth",178)->descText="16-1024 world units. Resize the window without changing its camera.";
 field("portalHeight","Height: ","ui_jar_contextPortalHeight",208)->descText="16-1024 world units. Rotation uses the panel's local axes.";
 add("portalState",contextPortalState,402,238,212,"uiScript jarContext portal_enabled ;");
 add("portalApply","Apply portal properties",402,270,212,"uiScript jarContext portal ;");
 add("portalLink",contextPortalLink,402,300,220,nullptr)->textscale=.4f;
 add("portalUnlink","Clear link",402,326,212,"uiScript jarContext portal_unlink ;")
  ->descText="Use Primary Link from a panel to a camera or from a camera to an aim target. Revert property restores a valid previous link.";
 add("portalHint","Up to four distinct camera feeds.",402,354,220,nullptr)->textscale=.4f;
 add("laserAction",contextLaserAction,402,148,212,"uiScript jarContext laser_action ;")->descText="Activate fires or turns the arm according to this choice. Apply stops the current beam.";
 field("laserYaw","Aim yaw: ","ui_jar_contextLaserYaw",174)->descText="-180 to 180 degrees relative to the base.";
 field("laserPitch","Aim pitch: ","ui_jar_contextLaserPitch",198)->descText="-45 to 90 degrees relative to the base.";
 field("laserSpeed","Turn speed: ","ui_jar_contextLaserSpeed",222)->descText="1-360 degrees per second. Each turn activation advances one native 100 ms step.";
 field("laserDamage","Damage / tick: ","ui_jar_contextLaserDamage",246)->descText="0-1000 every 100 ms while firing. Zero draws a harmless beam.";
 field("laserDuration","Duration (sec): ","ui_jar_contextLaserDuration",270)->descText="0.1-30 seconds per firing activation.";
 field("laserRange","Range: ","ui_jar_contextLaserRange",294)->descText="64-8192 world units; collision stops the beam.";
 add("laserApply","Apply laser properties",402,326,212,"uiScript jarContext laser ;");
 add("laserHint","Activate performs the selected action.",402,350,220,nullptr)->textscale=.38f;
 field("laserEmitterDamage","Damage / tick: ","ui_jar_contextLaserEmitterDamage",148)->descText="Whole numbers from 0 to 1000. Zero draws a harmless beam.";
 add("laserEmitterApply","Apply damage",402,176,220,"uiScript jarContext laser_emitter_damage ;");
 add("laserEmitterFiring",contextLaserEmitterFiring,402,204,220,"uiScript jarContext laser_emitter_firing ;")->descText="The beam fires every 100 ms while enabled. Activate or Use also toggles firing.";
 add("laserEmitterEnabled",contextLaserEmitterEnabled,402,232,220,"uiScript jarContext laser_emitter_enabled ;")->descText="Pause the emitter and disable activation. Re-enabling restores its firing setting.";
 add("laserEmitterUnlink","Clear aim link",402,260,220,"uiScript jarContext laser_emitter_unlink ;")->descText="Primary Link aims at a spawned entity. Clear the link to aim along the emitter's rotation.";
 add("laserEmitterAim",contextLaserEmitterAim,402,288,220,nullptr)->textscale=.43f;
 add("weatherMode",contextWeatherMode,402,148,212,"uiScript jarContext weather_mode ;")->descText="Cycle this effect type. Acid rain damages exposed actors using the native outdoor check.";
 field("weatherDensity","Density: ","ui_jar_contextWeatherDensity",174)->descText="0.1-4. Limited to 4000 particles per effect; r_weatherScale also controls density.";
 field("weatherSize","Size: ","ui_jar_contextWeatherSize",200)->descText="Particle size multiplier, 0.1-8.";
 field("weatherAlpha","Opacity: ","ui_jar_contextWeatherAlpha",226)->descText="0-1. Zero makes this effect invisible.";
 field("weatherSpeed","Wind speed: ","ui_jar_contextWeatherSpeed",174)->descText="0-5000. Rotate the controller to steer wind. Wind-zone forces apply only inside its volume.";
 add("weatherState",contextWeatherState,402,252,212,"uiScript jarContext weather_enabled ;");
 add("weatherApply","Apply weather",402,278,212,"uiScript jarContext weather ;");
 add("weatherHint",contextWeatherHint,402,308,220,nullptr)->textscale=.38f;
 add("weatherHint2",contextWeatherHint2,402,328,220,nullptr)->textscale=.4f;
 add("weatherHint3","Activate toggles. Rotate to steer wind.",402,348,220,nullptr)->textscale=.38f;
 field("reflectionRadius","Radius: ","ui_jar_contextReflectionRadius",148)->descText="16-32768 units. Nearby materials use this reflection and parallax radius.";
 add("reflectionState",contextReflectionState,402,180,220,"uiScript jarContext reflection_enabled ;");
 add("reflectionApply","Apply reflection properties",402,212,220,"uiScript jarContext reflection ;");
 add("reflectionRefresh","Recapture map",402,244,220,"uiScript jarContext reflection_refresh ;");
 add("reflectionStatus",contextReflectionStatus,402,278,220,nullptr)->textscale=.38f;
 add("reflectionHint","Move the marker to place the probe.",402,306,220,nullptr)->textscale=.4f;
 add("reflectionHint2","Captures the static map, not actors.",402,330,220,nullptr)->textscale=.4f;
 add("reflectionHint3","Rend2 + Cubemap reflections required.",402,354,220,nullptr)->textscale=.38f;
 field("cloudRadius","Outer radius: ","ui_jar_contextCloudRadius",148)->descText="16-32768 units. The stock effect has fixed, low geometry cost.";
 field("cloudInner","Inner radius: ","ui_jar_contextCloudInner",174)->descText="Hollow center radius, at least 8 units smaller than outer radius. Disk ignores it.";
 field("cloudCurve","Curvature: ","ui_jar_contextCloudCurve",200)->descText="-20 to 20. Zero is flat; the native hollow shape curves in the opposite direction.";
 add("cloudShape",contextCloudShape,402,226,212,"uiScript jarContext cloud_tube ;");
 add("cloudStyle",contextCloudStyle,402,252,212,"uiScript jarContext cloud_alternate ;");
 add("cloudState",contextCloudState,402,278,212,"uiScript jarContext cloud_enabled ;");
 add("cloudApply","Apply cloud properties",402,304,212,"uiScript jarContext cloud ;");
 add("cloudHint","Move / rotate the marker to position it.",402,330,220,nullptr)->textscale=.39f;
 add("cloudHint2","Activate toggles the cloud layer.",402,350,220,nullptr)->textscale=.4f;
 const auto&labels=jarContextActions;const char*actions[]={"freeze","release","activate","remove","duplicate","paste","rope","thruster","weld","hinge","unlink"};
 for(int i=0;i<11;++i){auto r=JAR_ContextRect(JC_ACTION,i);add(va("action%d",i),labels[i],r.x,r.y,r.w,String_Alloc(va("uiScript jarContext %s ;",actions[i])));}
 const auto ropeButton=JAR_ContextRect(JC_ACTION,11);
 add("openRopes","Ropes",ropeButton.x,ropeButton.y,ropeButton.w,"uiScript jarContext rope_page ;")->descText="Inspect and adjust ropes already attached to this entity.";
 add("undo","Revert property",402,446,104,"uiScript jarContext revert ;");add("close","Close",514,446,104,"uiScript jarOverlayClose ;");
 if(auto*b=Menu_FindItemByName(m,"action4"))b->descText="Copy properties, connected props and owned target/helper links.";
 if(auto*b=Menu_FindItemByName(m,"action5"))b->descText="Paste the complete copy at the crosshair. One Undo removes it.";
 if(auto*b=Menu_FindItemByName(m,"action6"))b->descText="Set a rope anchor at this entity's origin, including the player. Select a second entity or use the Rope tool for a world anchor.";
 if(auto*b=Menu_FindItemByName(m,"action7"))b->descText="Attach or toggle a thruster at the origin. Unlink removes it. Q sets force and direction for new mounts.";
 if(auto*b=Menu_FindItemByName(m,"action8"))b->descText="Set the first weld anchor at this entity's origin, including the player. Select another entity and Weld again. Undo removes the link.";
 if(auto*b=Menu_FindItemByName(m,"action9"))b->descText="Set a vertical hinge at this entity's origin, including the player. Select another entity and Hinge again. Toolgun uses the aimed face's axis instead.";
 auto*status=add("status",sandboxOverlayStatus,8,8,624,nullptr);status->window.style=WINDOW_STYLE_FILLED;status->window.flags|=WINDOW_BACKCOLORSET;VectorSet4(status->window.backColor,.08f,.08f,.08f,.96f);status->textalignx=8;
 Menu_PostParse(m);
}
void UI_JAR_ContextHeld(bool down){
 if(!down){if(UI_JAR_OverlayTyping())sandboxPinned=true;if(sandboxContextOpen&&!sandboxPinned)UI_JAR_OverlayClose();return;}
 if(sandboxOverlayOpen||strcmp(Cvar_VariableString("sv_running"),"1")||strcmp(Cvar_VariableString("jar_sandbox"),"1")||UI_JAR_DisplayTrialActive())return;
 ContextMenu();Menus_CloseAll();sandboxOverlayOpen=sandboxContextOpen=true;contextUiToken=0;contextPage=0;contextParmSlot=0;ContextLoadParm();
 Cvar_Set("jar_contextToken","0");Cvar_Set("jar_contextMountedVehicle","0");Cvar_Set("jar_contextLabel","Click an entity in the world");Cvar_Set("jar_contextOpen","1");
 ui.Cmd_ExecuteText(EXEC_APPEND,"jar_sb context open\n");
 uiInfo.uiDC.cursorShow=qtrue;Cvar_Set("cl_paused","0");ui.Key_SetCatcher(KEYCATCH_UI);Menus_ActivateByName("jarEntityContext");ContextUpdate();
}
static qboolean ContextScript(const char*name,const char**args){
 if(strcmp(name,"jarContext"))return qfalse;const char*action;
 if(!sandboxContextOpen||!String_Parse(args,&action))return qtrue;
 if(atoi(Cvar_VariableString("jar_contextMountedVehicle"))&&strcmp(action,"refresh")&&strcmp(action,"revert")&&
    strcmp(action,"vehicle_page")&&strcmp(action,"vehicle_resources"))return qtrue;
 if(!strcmp(action,"pick")){ui.Cmd_ExecuteText(EXEC_APPEND,va("jar_sb context pick %.3f %.3f\n",std::max(0.f,std::min(640.f,float(uiInfo.uiDC.cursorx))),std::max(0.f,std::min(480.f,float(uiInfo.uiDC.cursory)))));return qtrue;}
 if(!strcmp(action,"player")){ui.Cmd_ExecuteText(EXEC_APPEND,"jar_sb context player\n");return qtrue;}
 if(!strcmp(action,"revert")){if(contextUiToken)ui.Cmd_ExecuteText(EXEC_APPEND,va("jar_sb context revert %u\n",contextUiToken));return qtrue;}
 if(!strcmp(action,"reflection_page")||!strcmp(action,"rail_page")||!strcmp(action,"waypoint_page")||!strcmp(action,"scene_page")||!strcmp(action,"sky_page")||!strcmp(action,"combat_page")||!strcmp(action,"reference_page")||!strcmp(action,"pose_page")||!strcmp(action,"travel_page")||!strcmp(action,"properties")||!strcmp(action,"parms")||!strcmp(action,"script_page")||!strcmp(action,"boundary_page")||!strcmp(action,"cloud_page")||!strcmp(action,"weather_page")||!strcmp(action,"laser_page")||!strcmp(action,"portal_page")||!strcmp(action,"spot_page")||!strcmp(action,"welder_page")||!strcmp(action,"door_page")||!strcmp(action,"crystal_page")){UI_JAR_EndTextEntry();contextPage=!strcmp(action,"reflection_page")?20:!strcmp(action,"rail_page")?19:!strcmp(action,"waypoint_page")?18:!strcmp(action,"scene_page")?17:!strcmp(action,"sky_page")?16:!strcmp(action,"combat_page")?15:!strcmp(action,"reference_page")?14:!strcmp(action,"pose_page")?13:!strcmp(action,"travel_page")?12:!strcmp(action,"crystal_page")?11:!strcmp(action,"door_page")?10:!strcmp(action,"welder_page")?9:!strcmp(action,"parms")?1:!strcmp(action,"script_page")?2:!strcmp(action,"boundary_page")?3:!strcmp(action,"cloud_page")?4:!strcmp(action,"weather_page")?5:!strcmp(action,"laser_page")?6:!strcmp(action,"spot_page")?8:!strcmp(action,"portal_page")?7:0;ContextLoadReflection();ContextLoadRail();ContextLoadWaypoint();ContextLoadScene();ContextLoadSky();ContextLoadCombat();ContextLoadReference();ContextLoadPose();ContextLoadTravel();ContextLoadParm();ContextLoadScript();ContextLoadBoundary();ContextLoadCloud();ContextLoadWeather();ContextLoadLaser();ContextLoadPortal();ContextLoadSpot();ContextLoadWelder();ContextLoadDoor();ContextLoadCrystal();ContextUpdate();return qtrue;}
 if(!contextUiToken)return qtrue;
 if(!strcmp(action,"pickup_lifetime_page")&&atoi(Cvar_VariableString("jar_contextPickupLifetimeAllowed"))){
  UI_JAR_EndTextEntry();contextPage=40;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"deployed_shield_page")&&atoi(Cvar_VariableString("jar_contextDeployedShieldAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadDeployedShield();contextPage=41;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"mounted_charge_page")&&atoi(Cvar_VariableString("jar_contextMountedChargeAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadMountedCharge();contextPage=42;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"rope_page")&&atoi(Cvar_VariableString("jar_contextRopeCount"))){
  UI_JAR_EndTextEntry();ContextLoadRope();contextPage=43;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"thruster_page")){
  uint32_t id;
  if(jar::context::Token(Cvar_VariableString("jar_contextThrusterId"),id)){
   UI_JAR_EndTextEntry();ContextLoadThruster();contextPage=44;ContextUpdate();
  }
  return qtrue;
 }
 if(!strcmp(action,"thruster_power")||!strcmp(action,"thruster_direction")){
  const char* key=!strcmp(action,"thruster_power")?"ui_jar_contextThrusterEnabled":"ui_jar_contextThrusterReverse";
  Cvar_Set(key,atoi(Cvar_VariableString(key))?"0":"1");ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"replicator_page")&&atoi(Cvar_VariableString("jar_contextReplicatorAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadReplicator();contextPage=39;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"secret_page")&&atoi(Cvar_VariableString("jar_contextSecretAllowed"))){
  UI_JAR_EndTextEntry();contextPage=38;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"shooter_page")&&atoi(Cvar_VariableString("jar_contextShooterAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadShooter();contextPage=37;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"vehicle_page")&&atoi(Cvar_VariableString("jar_contextVehicleAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadVehicle();contextPage=36;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"npc_page")&&(atoi(Cvar_VariableString("jar_contextNPCAllowed"))||atoi(Cvar_VariableString("jar_contextNPCFireAllowed")))){
  UI_JAR_EndTextEntry();ContextLoadNPC();contextPage=45;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"npc_weapon_page")&&atoi(Cvar_VariableString("jar_contextNPCWeaponChoices"))){
  UI_JAR_EndTextEntry();ContextLoadNPC();contextPage=46;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"npc_order_page")&&atoi(Cvar_VariableString("jar_contextNPCOrderAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadNPC();contextPage=47;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"npc_order_mode")){
  if(atoi(Cvar_VariableString("jar_contextNPCOrderAllowed"))){
   Cvar_Set("ui_jar_contextNPCOrderMode",va("%d",(ContextNPCOrderMode()+1)%jar::npcOrder::ModeCount));
   ContextUpdate();
  }
  return qtrue;
 }
 if(!strcmp(action,"npc_weapon_previous")||!strcmp(action,"npc_weapon_next")){
  const uint32_t choices=atoi(Cvar_VariableString("jar_contextNPCWeaponChoices"));
  const int direction=!strcmp(action,"npc_weapon_previous")?-1:1;
  int index=0;
  for(int i=0;i<jar::npcWeapon::count;++i)
   if(jar::npcWeapon::choices[i].weapon==contextNPCWeapon){index=i;break;}
  for(int i=0;i<jar::npcWeapon::count;++i){
   index=(index+direction+jar::npcWeapon::count)%jar::npcWeapon::count;
   const int weapon=jar::npcWeapon::choices[index].weapon;
   if(choices&(1u<<weapon)){contextNPCWeapon=weapon;break;}
  }
  ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"npc_alt_fire")){
  if(atoi(Cvar_VariableString("jar_contextNPCAltAllowed"))){
   Cvar_Set("ui_jar_contextNPCAlternate",atoi(Cvar_VariableString("ui_jar_contextNPCAlternate"))?"0":"1");ContextUpdate();
  }
  return qtrue;
 }
 if(!strcmp(action,"npc_supply")){
  if(atoi(Cvar_VariableString("jar_contextNPCFireAllowed"))){
   Cvar_Set("ui_jar_contextNPCManualAmmo",atoi(Cvar_VariableString("ui_jar_contextNPCManualAmmo"))?"0":"1");ContextUpdate();
  }
  return qtrue;
 }
 if(!strcmp(action,"npc_team")||!strcmp(action,"npc_target")){
  if(atoi(Cvar_VariableString("jar_contextNPCTeamsAllowed"))){
   const char* key=!strcmp(action,"npc_team")?"ui_jar_contextNPCTeam":"ui_jar_contextNPCTargetTeam";
   Cvar_Set(key,va("%d",(ContextNPCTeam(key)+1)%TEAM_NUM_TEAMS));ContextUpdate();
  }
  return qtrue;
 }
 if(!strcmp(action,"teleport_page")&&atoi(Cvar_VariableString("jar_contextTeleportKind"))){
  UI_JAR_EndTextEntry();ContextLoadTeleport();contextPage=30;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"station_page")&&atoi(Cvar_VariableString("jar_contextStationAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadStation();contextPage=31;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"breakable_page")&&atoi(Cvar_VariableString("jar_contextBreakableAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadBreakable();contextPage=32;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"angular_page")&&atoi(Cvar_VariableString("jar_contextAngularAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadAngular();contextPage=35;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"speaker_page")&&atoi(Cvar_VariableString("jar_contextSpeakerAllowed"))){
  UI_JAR_EndTextEntry();ContextLoadSpeaker();contextPage=34;ContextUpdate();return qtrue;
 }
 if(atoi(Cvar_VariableString("jar_contextUsableAllowed"))){
  if(!strcmp(action,"usable_page")){
   UI_JAR_EndTextEntry();ContextLoadUsable();contextPage=33;ContextUpdate();return qtrue;
  }
  if(!strcmp(action,"usable_role")){
   Cvar_Set("jar_linkOutput",atoi(Cvar_VariableString("jar_linkOutput"))==1?"0":"1");
   ContextUpdate();return qtrue;
  }
 }
 if(!strcmp(action,"push_page")&&atoi(Cvar_VariableString("jar_contextPushKind"))){
  UI_JAR_EndTextEntry();ContextLoadPush();contextPage=29;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"contact_page")&&atoi(Cvar_VariableString("jar_contextContactKind"))){
  UI_JAR_EndTextEntry();ContextLoadContact();contextPage=28;ContextUpdate();return qtrue;
 }
 if(atoi(Cvar_VariableString("jar_contextLogicKind"))){
  if(!strcmp(action,"logic_page")){
   UI_JAR_EndTextEntry();ContextLoadLogic();contextPage=27;ContextUpdate();return qtrue;
  }
  const auto kind=static_cast<jar::logic::Kind>(atoi(Cvar_VariableString("jar_contextLogicKind")));
  if(!strcmp(action,"logic_next_slot")&&jar::logic::Slotted(kind)){
   Cvar_Set(jar::logic::SlotCvar(kind),va("%d",(ContextLogicSlot(kind)+1)%jar::logic::outputCount));
   ContextUpdate();return qtrue;
  }
  if(!strcmp(action,"logic_output")&&atoi(Cvar_VariableString("jar_contextLogicKind"))==jar::logic::Counter){
   Cvar_Set("jar_linkOutput",atoi(Cvar_VariableString("jar_linkOutput"))==1?"0":"1");
   ContextUpdate();return qtrue;
  }
 }
 if(!strcmp(action,"movement_page")&&atoi(Cvar_VariableString("jar_contextMovementKind"))){
  UI_JAR_EndTextEntry();ContextLoadMovement();contextPage=26;ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"bobbing_page")&&!strcmp(Cvar_VariableString("jar_contextBobbingAllowed"),"1")){
  UI_JAR_EndTextEntry();ContextLoadBobbing();contextPage=25;ContextUpdate();return qtrue;
 }
 if(!strcmp(Cvar_VariableString("jar_contextButtonAllowed"),"1")){
  if(!strcmp(action,"button_page")){
   UI_JAR_EndTextEntry();contextPage=24;
   Cvar_Set("ui_jar_contextButtonWait",Cvar_VariableString("jar_contextButtonWait"));ContextUpdate();return qtrue;
  }
  if(!strcmp(action,"button_output")){
   Cvar_Set("jar_linkOutput",atoi(Cvar_VariableString("jar_linkOutput"))==1?"0":"1");ContextUpdate();return qtrue;
  }
 }
 if(!strcmp(Cvar_VariableString("jar_contextSlideAllowed"),"1")){
  if(!strcmp(action,"slide_page")){UI_JAR_EndTextEntry();ContextLoadSlide();contextPage=23;ContextUpdate();return qtrue;}
  if(!strcmp(action,"slide_direction")&&strcmp(Cvar_VariableString("jar_contextSlideLift"),"1")){
   const int direction=std::min(JAR_SLIDE_DIRECTIONS-1,std::max(0,atoi(Cvar_VariableString("ui_jar_contextSlideDirection"))));
   Cvar_Set("ui_jar_contextSlideDirection",va("%d",(direction+1)%JAR_SLIDE_DIRECTIONS));ContextUpdate();return qtrue;
  }
  if(!strcmp(action,"slide_output")){
   Cvar_Set("jar_linkOutput",atoi(Cvar_VariableString("jar_linkOutput"))==1?"0":"1");ContextUpdate();return qtrue;
  }
 }
 if(!strcmp(action,"visible_page")&&!strcmp(Cvar_VariableString("jar_contextVisibleAllowed"),"1")){
  UI_JAR_EndTextEntry();contextPage=21;ContextUpdate();return qtrue;
 }
 if(!strcmp(Cvar_VariableString("jar_contextDistanceAllowed"),"1")){
  if(!strcmp(action,"distance_page")){UI_JAR_EndTextEntry();contextPage=22;ContextUpdate();return qtrue;}
  if(!strncmp(action,"distance_link_",14)&&strlen(action)==15&&action[14]>='0'&&action[14]<='3'){
   Cvar_Set("jar_linkOutput",action+14);ContextUpdate();return qtrue;
  }
 }
 if(!strcmp(action,"rail_enabled")||!strcmp(action,"rail_center")){const char*name=!strcmp(action,"rail_enabled")?"ui_jar_contextRailEnabled":"ui_jar_contextRailCenter";Cvar_Set(name,!strcmp(Cvar_VariableString(name),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"waypoint_enabled")){Cvar_Set("ui_jar_contextWaypointEnabled",!strcmp(Cvar_VariableString("ui_jar_contextWaypointEnabled"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"scene_loop")){Cvar_Set("ui_jar_contextSceneLoop",!strcmp(Cvar_VariableString("ui_jar_contextSceneLoop"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"reflection_enabled")){const char*key="ui_jar_contextReflectionEnabled";Cvar_Set(key,!strcmp(Cvar_VariableString(key),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"sky_enabled")||!strcmp(action,"sky_entities")){const char*key=!strcmp(action,"sky_enabled")?"ui_jar_contextSkyEnabled":"ui_jar_contextSkyEntities";Cvar_Set(key,!strcmp(Cvar_VariableString(key),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strncmp(action,"combat_flag",11)&&strlen(action)==12&&action[11]>='0'&&action[11]<='5'){const int bit=action[11]-'0';Cvar_Set("ui_jar_contextCombatFlags",va("%d",atoi(Cvar_VariableString("ui_jar_contextCombatFlags"))^(1<<bit)));ContextUpdate();return qtrue;}
 if(!strcmp(action,"combat_enabled")){Cvar_Set("ui_jar_contextCombatEnabled",!strcmp(Cvar_VariableString("ui_jar_contextCombatEnabled"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"reference_enabled")){Cvar_Set("ui_jar_contextReferenceEnabled",!strcmp(Cvar_VariableString("ui_jar_contextReferenceEnabled"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"parm_prev")||!strcmp(action,"parm_next")){UI_JAR_EndTextEntry();contextParmSlot=(contextParmSlot+(!strcmp(action,"parm_prev")?15:1))%jar::parms::count;ContextLoadParm();ContextUpdate();return qtrue;}
 if(!strcmp(action,"script_prev")||!strcmp(action,"script_next")){UI_JAR_EndTextEntry();Cvar_Set("ui_jar_contextScript",jar::scriptRunner::Cycle(Cvar_VariableString("ui_jar_contextScript"),!strcmp(action,"script_prev")?-1:1));return qtrue;}
 if(!strcmp(action,"script_receiver")){Cvar_Set("ui_jar_contextScriptActivator",!strcmp(Cvar_VariableString("ui_jar_contextScriptActivator"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"crystal_solid")){Cvar_Set("ui_jar_contextCrystalSolid",!strcmp(Cvar_VariableString("ui_jar_contextCrystalSolid"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"door_extended")){Cvar_Set("ui_jar_contextDoorExtended",!strcmp(Cvar_VariableString("ui_jar_contextDoorExtended"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"welder_enabled")){Cvar_Set("ui_jar_contextWelderEnabled",!strcmp(Cvar_VariableString("ui_jar_contextWelderEnabled"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"spot_tracking")||!strcmp(action,"spot_enabled")||!strcmp(action,"spot_output")){const char*key=!strcmp(action,"spot_tracking")?"ui_jar_contextSpotTracking":!strcmp(action,"spot_enabled")?"ui_jar_contextSpotEnabled":"jar_linkOutput";Cvar_Set(key,!strcmp(Cvar_VariableString(key),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"portal_mode")||!strcmp(action,"portal_enabled")){const char*key=!strcmp(action,"portal_mode")?"ui_jar_contextPortalMirror":"ui_jar_contextPortalEnabled";Cvar_Set(key,!strcmp(Cvar_VariableString(key),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"laser_action")){Cvar_Set("ui_jar_contextLaserAction",va("%d",(atoi(Cvar_VariableString("ui_jar_contextLaserAction"))+1)%5));ContextUpdate();return qtrue;}
 if(!strcmp(action,"weather_mode")){const int kind=atoi(Cvar_VariableString("ui_jar_contextWeatherKind"));const int count=jar::weather::Modes(static_cast<jar::weather::Kind>(kind));Cvar_Set("ui_jar_contextWeatherMode",va("%d",(atoi(Cvar_VariableString("ui_jar_contextWeatherMode"))+1)%count));ContextUpdate();return qtrue;}
 if(!strcmp(action,"weather_enabled")){Cvar_Set("ui_jar_contextWeatherEnabled",!strcmp(Cvar_VariableString("ui_jar_contextWeatherEnabled"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"cloud_tube")||!strcmp(action,"cloud_alternate")||!strcmp(action,"cloud_enabled")){
  const char*key=!strcmp(action,"cloud_tube")?"ui_jar_contextCloudTube":!strcmp(action,"cloud_alternate")?"ui_jar_contextCloudAlternate":"ui_jar_contextCloudEnabled";
  Cvar_Set(key,!strcmp(Cvar_VariableString(key),"1")?"0":"1");ContextUpdate();return qtrue;
 }
 if(!strcmp(action,"boundary_policy")){Cvar_Set("ui_jar_contextBoundaryPreserve",!strcmp(Cvar_VariableString("ui_jar_contextBoundaryPreserve"),"1")?"0":"1");ContextUpdate();return qtrue;}
 if(!strcmp(action,"travel_hub")){Cvar_Set("ui_jar_contextTravelHub",!strcmp(Cvar_VariableString("ui_jar_contextTravelHub"),"1")?"0":"1");ContextUpdate();return qtrue;}
 char command[384]{};
 if(!strcmp(action,"rail")){
  float length,width,cell,speed,delay,count,on,center;
  if(!jar::context::Number(Cvar_VariableString("ui_jar_contextRailLength"),length,128,8192)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailWidth"),width,64,2048)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailCell"),cell,32,256)||cell!=std::floor(cell)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailSpeed"),speed,16,2000)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailDelay"),delay,0,30)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailCount"),count,1,8)||count!=std::floor(count)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailEnabled"),on,0,1)||on!=std::floor(on)||!jar::context::Number(Cvar_VariableString("ui_jar_contextRailCenter"),center,0,1)||center!=std::floor(center))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb context rail %u %.3f %.3f %.0f %.3f %.3f %.0f %.0f %.0f\n",contextUiToken,length,width,cell,speed,delay,count,on,center);
 }else if(!strcmp(action,"waypoint")){
  float range,on;if(!ContextField(range,"ui_jar_contextWaypointRange",0,1024)||!ContextField(on,"ui_jar_contextWaypointEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Use range 0-1024 and On/Off. Zero range uses manual links.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context waypoint %u %.3f %.0f\n",contextUiToken,range,on);
 }else if(!strcmp(action,"scene")){
  float speed,turn,wait,loop;if(!ContextField(speed,"ui_jar_contextSceneSpeed",0,1024)||!ContextField(turn,"ui_jar_contextSceneTurn",1,720)||!ContextField(wait,"ui_jar_contextSceneWait",0,30)||!ContextField(loop,"ui_jar_contextSceneLoop",0,1)||loop!=std::floor(loop)){Cvar_Set("jar_workshopStatus","Camera: speed 0-1024, turn 1-720, wait 0-30, loop On/Off.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context scene %u %.3f %.3f %.3f %.0f\n",contextUiToken,speed,turn,wait,loop);
 }else if(!strcmp(action,"sky")){
  float on,entities;if(!ContextField(on,"ui_jar_contextSkyEnabled",0,1)||on!=std::floor(on)||!ContextField(entities,"ui_jar_contextSkyEntities",0,1)||entities!=std::floor(entities)){Cvar_Set("jar_workshopStatus","Choose sky On/Off and entities On/Off.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context sky %u %.0f %.0f\n",contextUiToken,on,entities);
 }else if(!strcmp(action,"combat")){
  float flags,on;if(!ContextField(flags,"ui_jar_contextCombatFlags",0,63)||flags!=std::floor(flags)||!ContextField(on,"ui_jar_contextCombatEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Choose combat flags and On/Off.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context combat %u %.0f %.0f\n",contextUiToken,flags,on);
 }else if(!strcmp(action,"reference")){
  jar::reference::Settings s;s.name=Cvar_VariableString("ui_jar_contextReferenceName");s.owner=Cvar_VariableString("ui_jar_contextReferenceOwner");float radius,on;
  if(!jar::reference::Valid(s)||!ContextField(radius,"ui_jar_contextReferenceRadius",0,1024)||radius!=std::floor(radius)||!ContextField(on,"ui_jar_contextReferenceEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Use a tag name and optional owner (1-31 letters, digits, _ or -), whole radius 0-1024 and On/Off.");return qtrue;}
  const auto tag=jar::workshop::Hex(s.name),owner=jar::workshop::Hex(s.owner);Com_sprintf(command,sizeof(command),"jar_sb context reference %u %s \"%s\" %.0f %.0f\n",contextUiToken,tag.c_str(),owner.c_str(),radius,on);
 }else if(!strcmp(action,"pose")||!strcmp(action,"pose_prev")||!strcmp(action,"pose_next")||!strcmp(action,"pose_reset")){
  float value,last;if(strcmp(Cvar_VariableString("jar_contextPoseAllowed"),"1")||!ContextField(last,"jar_contextPoseLast",1,65535)||last!=std::floor(last))return qtrue;
  if(!strcmp(action,"pose_reset"))value=0;
  else {if(!ContextField(value,!strcmp(action,"pose")?"ui_jar_contextPoseFrame":"jar_contextPoseFrame",0,last)||value!=std::floor(value)){Cvar_Set("jar_workshopStatus","Enter a whole pose frame within the displayed range.");return qtrue;}
   if(!strcmp(action,"pose_prev"))value=std::max(0.f,value-1);if(!strcmp(action,"pose_next"))value=std::min(last,value+1);}
  Com_sprintf(command,sizeof(command),"jar_sb context pose %u %.0f\n",contextUiToken,value);
 }else if(!strcmp(action,"travel")){
  jar::levelChange::Settings value;value.map=Cvar_VariableString("ui_jar_contextTravelMap");value.spawn=Cvar_VariableString("ui_jar_contextTravelSpawn");float hub;
  if(!jar::levelChange::Valid(value)||!ContextField(hub,"ui_jar_contextTravelHub",0,1)||hub!=std::floor(hub)){Cvar_Set("jar_workshopStatus","Use a map name and optional spawn point containing letters, digits, underscores or hyphens. Map subfolders use /.");return qtrue;}
  const auto map=jar::workshop::Hex(value.map),spawn=jar::workshop::Hex(value.spawn);
  Com_sprintf(command,sizeof(command),"jar_sb context travel %u %s \"%s\" %.0f\n",contextUiToken,map.c_str(),spawn.c_str(),hub);
 }else if(!strcmp(action,"mounted_charge")){
  using namespace jar::chargeCopy;
  if(!atoi(Cvar_VariableString("jar_contextMountedChargeEditable")))return qtrue;
  const bool proximity=atoi(Cvar_VariableString("jar_contextMountedChargeProximity"));
  float health,damage,radius,remaining=0;
  if(!ContextField(health,"ui_jar_contextMountedChargeHealth",1,maxEditHealth)||health!=std::floor(health)||
     !ContextField(damage,"ui_jar_contextMountedChargeDamage",0,maxEditDamage)||damage!=std::floor(damage)||
     !ContextField(radius,"ui_jar_contextMountedChargeRadius",0,maxEditRadius)||radius!=std::floor(radius)||
     (proximity&&(!ContextField(remaining,"ui_jar_contextMountedChargeRemaining",0,maxEditTimeout)||remaining!=std::floor(remaining)))){
   Cvar_Set("jar_workshopStatus","Use whole values within the charge settings limits.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context mounted_charge %u %.0f %.0f %.0f %.0f\n",contextUiToken,health,damage,radius,remaining);
 }else if(!strcmp(action,"deployed_shield")){
  if(!atoi(Cvar_VariableString("jar_contextDeployedShieldAllowed")))return qtrue;
  float health,remaining,left,right,height;
  if(!ContextField(health,"ui_jar_contextDeployedShieldHealth",1,jar::shieldCopy::maxHealth)||health!=std::floor(health)||
     !ContextField(remaining,"ui_jar_contextDeployedShieldRemaining",0,jar::shieldCopy::lifetime)||remaining!=std::floor(remaining)||
     !ContextField(left,"ui_jar_contextDeployedShieldLeft",3,254)||!ContextField(right,"ui_jar_contextDeployedShieldRight",3,254)||
     !ContextField(height,"ui_jar_contextDeployedShieldHeight",32,253)){
   Cvar_Set("jar_workshopStatus","Check health, remaining time, spans of 3 to 254 units and height of 32 to 253 units.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context deployed_shield %u %.0f %.0f %.3f %.3f %.3f\n",contextUiToken,health,remaining,left,right,height);
 }else if(!strcmp(action,"pickup_lifetime")){
  float value;
  if(!ContextField(value,"ui_jar_contextPickupLifetime",0,jar::pickupLifetime::maxMs)||value!=std::floor(value)){
   Cvar_Set("jar_workshopStatus","Enter a whole pickup lifetime from 0 to 600000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context pickup_lifetime %u %.0f\n",contextUiToken,value);
 }else if(!strcmp(action,"pickup_quantity")){
  float value;
  if(!ContextField(value,"ui_jar_contextPickupQuantity",0,jar::pickupQuantity::maximum)||value!=std::floor(value)){
   Cvar_Set("jar_workshopStatus",atoi(Cvar_VariableString("jar_contextPickupDuration"))?
    "Enter whole seconds from 0 to 1000.":"Enter a whole pickup quantity from 0 to 1000.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context pickup_quantity %u %.0f\n",contextUiToken,value);
 }else if(!strcmp(action,"holocron")){
  float value;if(!ContextField(value,"ui_jar_contextHolocronLevel",1,3)||value!=std::floor(value)){Cvar_Set("jar_workshopStatus","Force level must be a whole number from 1 to 3.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context holocron %u %.0f\n",contextUiToken,value);
 }else if(!strcmp(action,"position")||!strcmp(action,"angles")){
  const bool position=!strcmp(action,"position");float v[3];
  for(int i=0;i<3;++i)if(!ContextField(v[i],va("ui_jar_context%s%d",position?"Position":"Angle",i),position?-65535.f:-360.f,position?65535.f:360.f)){
   Cvar_Set("jar_workshopStatus","Enter numeric coordinates or angles within range.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u %.3f %.3f %.3f\n",action,contextUiToken,v[0],v[1],v[2]);
 }else if(!strcmp(action,"button")){
  float delay;
  if(!ContextField(delay,"ui_jar_contextButtonWait",0,JAR_BUTTON_MAX_WAIT)||delay!=std::floor(delay)){
   Cvar_Set("jar_workshopStatus","Use a whole hold delay from 0 to 30000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context button %u %.0f\n",contextUiToken,delay);
 }else if(!strcmp(action,"movement_gravity")||!strcmp(action,"movement_friction")){
  const bool gravity=!strcmp(action,"movement_gravity");
  float value;
  if(!ContextField(value,gravity?"ui_jar_contextMovementGravity":"ui_jar_contextMovementFriction",
      0,gravity?jar::movement::maxGravity:jar::movement::maxFriction)||value!=std::floor(value)){
   Cvar_Set("jar_workshopStatus",gravity?"Enter whole gravity from 0 to 3200.":"Enter whole ground friction from 0 to 10.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u %.0f\n",action,contextUiToken,value);
 }else if(!strcmp(action,"movement_map")||!strcmp(action,"movement_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"bobbing")){
  float amplitude,period;
  if(!ContextField(amplitude,"ui_jar_contextBobbingAmplitude",JAR_BOB_MIN_AMPLITUDE,JAR_BOB_MAX_AMPLITUDE)||
     !ContextField(period,"ui_jar_contextBobbingPeriod",JAR_BOB_MIN_PERIOD,JAR_BOB_MAX_PERIOD)||
     amplitude!=std::floor(amplitude)||period!=std::floor(period)){
   Cvar_Set("jar_workshopStatus","Use whole values. Amplitude: 1-128 units. Cycle: 500-30000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context bobbing %u %.0f %.0f\n",contextUiToken,amplitude,period);
 }else if(!strcmp(action,"button_enabled")||!strcmp(action,"slide_enabled")||
           !strcmp(action,"bobbing_running")||!strcmp(action,"bobbing_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"slide")){
  const char* fields[]={"Travel","Speed","Direction","Wait"};
  const int limits[][2]={{JAR_SLIDE_MIN_TRAVEL,JAR_SLIDE_MAX_TRAVEL},{JAR_SLIDE_MIN_SPEED,JAR_SLIDE_MAX_SPEED},
                       {0,JAR_SLIDE_DIRECTIONS-1},{0,JAR_SLIDE_MAX_WAIT}};
  float values[4];
  for(int i=0;i<4;++i){
   if(!ContextField(values[i],va("ui_jar_contextSlide%s",fields[i]),limits[i][0],limits[i][1])||values[i]!=std::floor(values[i])){
    Cvar_Set("jar_workshopStatus","Use whole travel, speed and delay values within range.");return qtrue;
   }
  }
  Com_sprintf(command,sizeof(command),"jar_sb context slide %u %.0f %.0f %.0f %.0f\n",contextUiToken,values[0],values[1],values[2],values[3]);
 }else if(!strcmp(action,"wall_visible")||!strcmp(action,"wall_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"distance")){
  float range;
  if(!ContextField(range,"ui_jar_contextDistanceRange",JAR_DISTANCE_MIN,JAR_DISTANCE_MAX)||range!=std::floor(range)){
   Cvar_Set("jar_workshopStatus","Enter a whole distance from 1 to 4096 units.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context distance %u %.0f\n",contextUiToken,range);
 }else if(!strcmp(action,"distance_subjects")||!strcmp(action,"distance_arm")||
          !strcmp(action,"distance_clear_reference")||!strcmp(action,"distance_clear_subject")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"visible")){
  float range,cone;
  if(!ContextField(range,"ui_jar_contextVisibleRange",JAR_VISIBLE_RANGE_MIN,JAR_VISIBLE_RANGE_MAX)||range!=std::floor(range)||
     !ContextField(cone,"ui_jar_contextVisibleCone",JAR_VISIBLE_CONE_MIN,JAR_VISIBLE_CONE_MAX)||cone!=std::floor(cone)){
   Cvar_Set("jar_workshopStatus","Enter a whole range from 32 to 4096 and half-angle from 5 to 85 degrees.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context visible %u %.0f %.0f\n",contextUiToken,range,cone);
 }else if(!strcmp(action,"visible_walls")||!strcmp(action,"visible_sight")||!strcmp(action,"visible_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"always_delay")){
  float value;if(!ContextField(value,"ui_jar_contextAlwaysDelay",JAR_ALWAYS_MIN_MS,JAR_ALWAYS_MAX_MS)||value!=std::floor(value)){
   Cvar_Set("jar_workshopStatus","Enter a whole delay from 50 to 60000 ms.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context always_delay %u %.0f\n",contextUiToken,value);
 }else if(!strcmp(action,"space_armed")||!strcmp(action,"space_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"volume")){float v[3];
  for(int i=0;i<3;++i)if(!ContextField(v[i],va("ui_jar_contextVolume%d",i),jar::volume::minimum,jar::volume::maximum)){Cvar_Set("jar_workshopStatus","Volume dimensions must be numbers from 8 to 8192 units.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context volume %u %.3f %.3f %.3f\n",contextUiToken,v[0],v[1],v[2]);
 }else if(!strcmp(action,"music")){
  std::string text;if(!jar::music::Canonical(Cvar_VariableString("ui_jar_contextText"),text)){Cvar_Set("jar_workshopStatus","Use Prev/Next or enter a retail intro and optional loop path.");return qtrue;}
  const auto encoded=jar::workshop::Hex(text);Com_sprintf(command,sizeof(command),"jar_sb context music %u %s\n",contextUiToken,encoded.c_str());
 }else if(!strcmp(action,"weather")){
  jar::weather::Settings s;float mode,on,kind;
  if(!ContextField(kind,"ui_jar_contextWeatherKind",0,5)||kind!=std::floor(kind)||!ContextField(mode,"ui_jar_contextWeatherMode",0,3)||mode!=std::floor(mode)||!ContextField(on,"ui_jar_contextWeatherEnabled",0,1)||on!=std::floor(on)||!ContextField(s.density,"ui_jar_contextWeatherDensity",.1f,4)||!ContextField(s.size,"ui_jar_contextWeatherSize",.1f,8)||!ContextField(s.alpha,"ui_jar_contextWeatherAlpha",0,1)||!ContextField(s.speed,"ui_jar_contextWeatherSpeed",0,5000)){Cvar_Set("jar_workshopStatus","Check weather numbers and mode.");return qtrue;}
  s.kind=static_cast<jar::weather::Kind>(int(kind));s.mode=int(mode);if(!jar::weather::Valid(s)){Cvar_Set("jar_workshopStatus","Choose a mode supported by this weather type.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context weather %u %.0f %.3f %.3f %.3f %.3f %.0f\n",contextUiToken,mode,s.density,s.size,s.alpha,s.speed,on);
 }else if(!strcmp(action,"crystal")){
  jar::crystalCrate::Settings s;float hit,blast,solid;
  if(!ContextField(hit,"ui_jar_contextCrystalHitDamage",0,1000)||hit!=std::floor(hit)||!ContextField(s.hitRadius,"ui_jar_contextCrystalHitRadius",1,1024)||!ContextField(blast,"ui_jar_contextCrystalBlastDamage",0,1000)||blast!=std::floor(blast)||!ContextField(s.blastRadius,"ui_jar_contextCrystalBlastRadius",1,1024)||s.blastRadius!=std::floor(s.blastRadius)||!ContextField(solid,"ui_jar_contextCrystalSolid",0,1)||solid!=std::floor(solid)){Cvar_Set("jar_workshopStatus","Use whole damage 0-1000, radii 1-1024 (whole blast radius), and Solid/Shot-only.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context crystal %u %.0f %.3f %.0f %.0f %.0f\n",contextUiToken,hit,s.hitRadius,blast,s.blastRadius,solid);
 }else if(!strcmp(action,"door")){
  jar::doorCamera::Settings s;float extended;
  if(!ContextField(s.travel,"ui_jar_contextDoorTravel",8,256)||!ContextField(s.speed,"ui_jar_contextDoorSpeed",1,512)||!ContextField(extended,"ui_jar_contextDoorExtended",0,1)||extended!=std::floor(extended)){Cvar_Set("jar_workshopStatus","Use travel 8-256, speed 1-512 and Extended/Retracted.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context door %u %.3f %.3f %.0f\n",contextUiToken,s.travel,s.speed,extended);
 }else if(!strcmp(action,"welder")){
  jar::welder::Settings s;float damage,on;
  if(!ContextField(s.reach,"ui_jar_contextWelderReach",1,256)||!ContextField(s.radius,"ui_jar_contextWelderRadius",1,256)||!ContextField(s.interval,"ui_jar_contextWelderInterval",.1f,10)||!ContextField(damage,"ui_jar_contextWelderDamage",0,1000)||damage!=std::floor(damage)||!ContextField(on,"ui_jar_contextWelderEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Use reach/radius 1-256, interval 0.1-10 and whole damage 0-1000.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context welder %u %.3f %.3f %.3f %.0f %.0f\n",contextUiToken,s.reach,s.radius,s.interval,damage,on);
 }else if(!strcmp(action,"spot")){
  jar::spotlight::Settings s;float tracking,on;
  if(!ContextField(s.range,"ui_jar_contextSpotRange",64,8192)||!ContextField(s.cone,"ui_jar_contextSpotCone",1,30)||!ContextField(s.light,"ui_jar_contextSpotLight",0,1024)||!ContextField(tracking,"ui_jar_contextSpotTracking",0,1)||tracking!=std::floor(tracking)||!ContextField(on,"ui_jar_contextSpotEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Check spotlight range, cone angle, light radius and mode.");return qtrue;}
  for(int i=0;i<3;++i)if(!ContextField(s.rgb[i],va("ui_jar_contextSpotColor%d",i),0,1)){Cvar_Set("jar_workshopStatus","Spotlight colors must be 0-1.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context spot %u %.3f %.3f %.3f %.3f %.3f %.3f %.0f %.0f\n",contextUiToken,s.range,s.cone,s.light,s.rgb[0],s.rgb[1],s.rgb[2],tracking,on);
 }else if(!strcmp(action,"portal_unlink")){
  if(!atoi(Cvar_VariableString("jar_contextPortalCanUnlink")))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb context portal_unlink %u\n",contextUiToken);
 }else if(!strcmp(action,"portal")){
  jar::portal::Settings s;float mirror,on;
  if(!ContextField(s.width,"ui_jar_contextPortalWidth",16,1024)||!ContextField(s.height,"ui_jar_contextPortalHeight",16,1024)||!ContextField(mirror,"ui_jar_contextPortalMirror",0,1)||mirror!=std::floor(mirror)||!ContextField(on,"ui_jar_contextPortalEnabled",0,1)||on!=std::floor(on)){
   Cvar_Set("jar_workshopStatus","Use portal dimensions 16-1024 and the supplied mode/On/Off choices.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context portal %u %.3f %.3f %.0f %.0f\n",contextUiToken,s.width,s.height,mirror,on);
 }else if(!strcmp(action,"angular_apply")){
  using namespace jar::sandboxAngular;
  const bool swing=atoi(Cvar_VariableString("jar_contextAngularKind"))==Pendulum;
  float rate,period=2000;
  if(!ContextField(rate,"ui_jar_contextAngularRate",swing?1:-maxSpeed,swing?maxSwing:maxSpeed)||
     !rate||rate!=std::floor(rate)||(swing&&(!ContextField(period,"ui_jar_contextAngularPeriod",minPeriod,maxPeriod)||period!=std::floor(period)))){
   Cvar_Set("jar_workshopStatus","Enter whole numbers within the displayed motion limits.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context angular_apply %u %d %d\n",contextUiToken,int(rate),int(period));
 }else if(!strcmp(action,"angular_axis")||!strcmp(action,"angular_running")||!strcmp(action,"angular_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"speaker_apply")){
  using namespace jar::speaker;
  float period,variation;
  if(!ContextField(period,"ui_jar_contextSpeakerPeriod",minDelay,maxPeriod)||period!=std::floor(period)||
     !ContextField(variation,"ui_jar_contextSpeakerVariation",0,period-minDelay)||variation!=std::floor(variation)){
   Cvar_Set("jar_workshopStatus","Repeat requires 500 to 60000 ms and a minimum gap of 500 ms after variation.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context speaker_apply %u %d %d\n",contextUiToken,int(period),int(variation));
 }else if(!strcmp(action,"speaker_prev")||!strcmp(action,"speaker_next")||!strcmp(action,"speaker_mode")||!strcmp(action,"speaker_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"usable_apply")){
  using namespace jar::sandboxUsable;
  float capacity,reuse;
  if(!ContextField(capacity,"ui_jar_contextUsableCapacity",0,maxHealth)||capacity!=std::floor(capacity)||
     !ContextField(reuse,"ui_jar_contextUsableReuse",0,maxReuse)||reuse!=std::floor(reuse)){
   Cvar_Set("jar_workshopStatus","Enter whole health from 0 to 10000 and reuse from 0 to 60000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context usable_apply %u %d %d\n",contextUiToken,int(capacity),int(reuse));
 }else if(!strcmp(action,"usable_mode")||!strcmp(action,"usable_damage")||!strcmp(action,"usable_direct")||
          !strcmp(action,"usable_solid")||!strcmp(action,"usable_rearm")||!strcmp(action,"usable_enabled")||!strcmp(action,"usable_unlink")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"breakable_apply")){
  using namespace jar::sandboxBreakable;
  const char* keys[]={"Capacity","Fuse","Chunks","Splash","Radius"};
  const int lower[]={1,0,0,0,0},upper[]={maxHealth,maxFuse,maxChunks,maxSplash,maxRadius};
  float values[5];
  for(int i=0;i<5;++i)if(!ContextField(values[i],va("ui_jar_contextBreakable%s",keys[i]),lower[i],upper[i])||
     values[i]!=std::floor(values[i])){
   Cvar_Set("jar_workshopStatus","Check whole-number health, fuse, debris, splash damage and radius.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context breakable_apply %u %d %d %d %d %d\n",contextUiToken,
   int(values[0]),int(values[1]),int(values[2]),int(values[3]),int(values[4]));
 }else if(!strcmp(action,"breakable_mode")||!strcmp(action,"breakable_material")||
          !strcmp(action,"breakable_use")||!strcmp(action,"breakable_enabled")||
          !strcmp(action,"breakable_explosion")||!strcmp(action,"breakable_repair")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"station_apply")){
  using namespace jar::sandboxStation;
  float capacity,recharge;
  if(!ContextField(capacity,"ui_jar_contextStationCapacity",1,maxCapacity)||capacity!=std::floor(capacity)||
     !ContextField(recharge,"ui_jar_contextStationRecharge",0,maxRecharge)||recharge!=std::floor(recharge)||
     (recharge>0&&recharge<minRecharge)){
   Cvar_Set("jar_workshopStatus","Enter capacity 1 to 10000 and recharge 0 or 100 to 60000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context station_apply %u %d %d\n",contextUiToken,int(capacity),int(recharge));
 }else if(!strcmp(action,"station_charge")){
  float charge;
  if(!ContextField(charge,"ui_jar_contextStationCharge",0,atoi(Cvar_VariableString("jar_contextStationCapacity")))||charge!=std::floor(charge)){
   Cvar_Set("jar_workshopStatus","Enter whole charge from zero to capacity.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context station_charge %u %d\n",contextUiToken,int(charge));
 }else if(!strcmp(action,"station_refill")||!strcmp(action,"station_unlimited")||!strcmp(action,"station_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"laser_emitter_damage")){
  float damage;
  if(!ContextField(damage,"ui_jar_contextLaserEmitterDamage",0,jar::sandboxLaser::maxDamage)||damage!=std::floor(damage)){
   Cvar_Set("jar_workshopStatus","Enter whole laser damage from 0 to 1000.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context laser_emitter_damage %u %d\n",contextUiToken,int(damage));
 }else if(!strcmp(action,"laser_emitter_firing")||!strcmp(action,"laser_emitter_enabled")||!strcmp(action,"laser_emitter_unlink")||!strcmp(action,"trail_unlink")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"laser")){
  jar::laserArm::Settings s;float operation,damage;
  if(!ContextField(operation,"ui_jar_contextLaserAction",0,4)||operation!=std::floor(operation)||!ContextField(s.yaw,"ui_jar_contextLaserYaw",-180,180)||!ContextField(s.pitch,"ui_jar_contextLaserPitch",-45,90)||!ContextField(s.speed,"ui_jar_contextLaserSpeed",1,360)||!ContextField(damage,"ui_jar_contextLaserDamage",0,1000)||damage!=std::floor(damage)||!ContextField(s.duration,"ui_jar_contextLaserDuration",.1f,30)||!ContextField(s.range,"ui_jar_contextLaserRange",64,8192)){
   Cvar_Set("jar_workshopStatus","Check laser aim, speed, damage, duration and range.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context laser %u %.0f %.3f %.3f %.3f %.0f %.3f %.3f\n",contextUiToken,operation,s.yaw,s.pitch,s.speed,damage,s.duration,s.range);
 }else if(!strcmp(action,"reflection_refresh")){
  Com_sprintf(command,sizeof(command),"jar_sb context reflection_refresh %u\n",contextUiToken);
 }else if(!strcmp(action,"reflection")){
  jar::reflection::Settings value;float on;
  if(!ContextField(value.radius,"ui_jar_contextReflectionRadius",16,32768)||!ContextField(on,"ui_jar_contextReflectionEnabled",0,1)||on!=std::floor(on)){Cvar_Set("jar_workshopStatus","Use a reflection radius of 16-32768 and choose On/Off.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context reflection %u %.3f %.0f\n",contextUiToken,value.radius,on);
 }else if(!strcmp(action,"cloud")){
  jar::cloud::Settings s;float tube,alternate,on;
  if(!ContextField(s.radius,"ui_jar_contextCloudRadius",16,32768)||!ContextField(s.inner,"ui_jar_contextCloudInner",0,32760)||!ContextField(s.curve,"ui_jar_contextCloudCurve",-20,20)||
     !ContextField(tube,"ui_jar_contextCloudTube",0,1)||tube!=std::floor(tube)||!ContextField(alternate,"ui_jar_contextCloudAlternate",0,1)||alternate!=std::floor(alternate)||!ContextField(on,"ui_jar_contextCloudEnabled",0,1)||on!=std::floor(on)||!jar::cloud::Valid(s)){
   Cvar_Set("jar_workshopStatus","Use radius 16-32768, inner at least 8 smaller, curvature -20 to 20 and the supplied choices.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context cloud %u %.3f %.3f %.3f %.0f %.0f %.0f\n",contextUiToken,s.radius,s.inner,s.curve,tube,alternate,on);
 }else if(!strcmp(action,"boundary")){float seconds,preserve;
  if(!ContextField(seconds,"ui_jar_contextBoundaryTime",.1f,60)||!ContextField(preserve,"ui_jar_contextBoundaryPreserve",0,1)||preserve!=std::floor(preserve)){Cvar_Set("jar_workshopStatus","Use 0.1-60 seconds and Ignore or Destroy for empty/damaged fighters.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context boundary %u %.3f %.0f\n",contextUiToken,seconds,preserve);
 }else if(!strcmp(action,"script")){
  jar::scriptRunner::Settings s;float count,activator;
  if(!jar::scriptRunner::Canonical(Cvar_VariableString("ui_jar_contextScript"),s.script)||!ContextField(s.delay,"ui_jar_contextScriptDelay",0,3600)||!ContextField(s.wait,"ui_jar_contextScriptWait",0,3600)||
     !ContextField(count,"ui_jar_contextScriptCount",-1,100000)||count!=std::floor(count)||!ContextField(activator,"ui_jar_contextScriptActivator",0,1)||activator!=std::floor(activator)){
   Cvar_Set("jar_workshopStatus","Choose a script, delay/cooldown 0-3600 seconds and whole uses -1 to 100000.");return qtrue;
  }
  const auto encoded=jar::workshop::Hex(s.script);Com_sprintf(command,sizeof(command),"jar_sb context script %u %s %.3f %.3f %.0f %.0f\n",contextUiToken,encoded.c_str(),s.delay,s.wait,count,activator);
 }else if(!strcmp(action,"parm")||!strcmp(action,"parm_clear")){
  const std::string text=!strcmp(action,"parm_clear")?"":Cvar_VariableString("ui_jar_contextParm");
  if(!jar::parms::Valid(text)){Cvar_Set("jar_workshopStatus","Use up to 63 printable characters, or Clear for an empty parameter.");return qtrue;}
  const auto encoded=jar::workshop::Hex(text);Com_sprintf(command,sizeof(command),"jar_sb context parm %u %u %s\n",contextUiToken,contextParmSlot+1,encoded.c_str());
 }else if(!strcmp(action,"label")){const char*text=Cvar_VariableString("ui_jar_contextText");
  if(!jar::context::Label(text)){Cvar_Set("jar_workshopStatus","Enter 1-63 printable characters without quotes, semicolons or backslashes.");return qtrue;}
  const auto encoded=jar::workshop::Hex(text);Com_sprintf(command,sizeof(command),"jar_sb context label %u %s\n",contextUiToken,encoded.c_str());
 }else if(!strcmp(action,"teleport_apply")){
  using namespace jar::teleport;
  float interval;
  if(!ContextField(interval,"ui_jar_contextTeleportInterval",minInterval,maxInterval)||interval!=std::floor(interval)){
   Cvar_Set("jar_workshopStatus","Enter the interval in whole milliseconds.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context teleport_apply %u %d\n",contextUiToken,int(interval));
 }else if(!strcmp(action,"teleport_subject")||!strcmp(action,"teleport_view")||!strcmp(action,"teleport_velocity")||
          !strcmp(action,"teleport_enabled")||!strcmp(action,"teleport_unlink")||
          !strcmp(action,"teleport_props")||!strcmp(action,"teleport_projectiles")||!strcmp(action,"teleport_vehicles")||
          !strcmp(action,"teleport_assemblies")||!strcmp(action,"teleport_frozen_props")||!strcmp(action,"teleport_movers")||
          !strcmp(action,"teleport_world_anchors")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"push_apply")){
  using namespace jar::push;
  float speed,interval;
  if(!ContextField(speed,"ui_jar_contextPushSpeed",1,maxSpeed)||speed!=std::floor(speed)||
     !ContextField(interval,"ui_jar_contextPushInterval",minInterval,maxInterval)||interval!=std::floor(interval)){
   Cvar_Set("jar_workshopStatus","Enter speed and interval as whole numbers within range.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context push_apply %u %d %d\n",contextUiToken,int(speed),int(interval));
 }else if(!strcmp(action,"push_aim")||!strcmp(action,"push_subject")||!strcmp(action,"push_add")||
          !strcmp(action,"push_enabled")||!strcmp(action,"push_unlink")||!strcmp(action,"push_vehicles")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"contact_apply")){
  using namespace jar::contactTrigger;
  Settings settings;
  settings.kind=static_cast<Kind>(atoi(Cvar_VariableString("jar_contextContactKind")));
  float delay,interval=0,variation=0;
  if(!ContextField(delay,"ui_jar_contextContactDelay",0,maxTime)||delay!=std::floor(delay)||
     (settings.kind==Repeat&&(!ContextField(interval,"ui_jar_contextContactInterval",minInterval,maxTime)||interval!=std::floor(interval)||
      !ContextField(variation,"ui_jar_contextContactVariation",0,maxTime)||variation!=std::floor(variation)))){
   Cvar_Set("jar_workshopStatus","Enter timing in whole milliseconds.");return qtrue;
  }
  settings.delay=int(delay);settings.interval=int(interval);settings.variation=int(variation);
  if(!Valid(settings)){
   Cvar_Set("jar_workshopStatus","Variation must keep the repeat interval between 100 and 60000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context contact_apply %u %d %d %d\n",contextUiToken,
   settings.delay,settings.interval,settings.variation);
 }else if(!strcmp(action,"contact_hurt_apply")){
  using namespace jar::contactTrigger;
  float damage,interval;
  if(!ContextField(damage,"ui_jar_contextContactDamage",1,maxDamage)||damage!=std::floor(damage)||
     !ContextField(interval,"ui_jar_contextContactInterval",minInterval,maxTime)||interval!=std::floor(interval)){
   Cvar_Set("jar_workshopStatus","Enter damage and interval as whole numbers within range.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context contact_hurt_apply %u %d %d\n",contextUiToken,int(damage),int(interval));
 }else if(!strcmp(action,"contact_subject")||!strcmp(action,"contact_use")||!strcmp(action,"contact_hurt_mode")||!strcmp(action,"contact_hurt_arm")||
          !strcmp(action,"contact_enabled")||!strcmp(action,"contact_rearm")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"logic_apply")){
  using namespace jar::logic;
  const int kind=atoi(Cvar_VariableString("jar_contextLogicKind"));
  State value;
  float first,second=0;
  if(kind==Delay||kind==Timer||kind==Relay){
   value.kind=static_cast<Kind>(kind);
   if(!ContextField(first,"ui_jar_contextLogicDelay",kind==Timer?minInterval:0,maxDelay)||first!=std::floor(first)||
      !ContextField(second,kind==Relay?"ui_jar_contextLogicCooldown":"ui_jar_contextLogicJitter",0,maxDelay)||second!=std::floor(second)){
    Cvar_Set("jar_workshopStatus","Use whole milliseconds from 0 to 60000.");return qtrue;
   }
   value.delayMs=int(first);
   if(kind==Relay){value.cooldownMs=int(second);value.repeat=true;value.remaining=1;}
   else value.jitterMs=int(second);
   if(!Valid(value)){
    Cvar_Set("jar_workshopStatus",kind==Timer?"Variation must keep the interval between 100 and 60000 ms.":
     "Variation must keep the delay between 0 and 60000 ms.");return qtrue;
   }
   Com_sprintf(command,sizeof(command),"jar_sb context logic_apply %u %d %d\n",contextUiToken,int(first),int(second));
  }else if(kind==Counter||kind==Score){
   const bool score=kind==Score;
   if(!ContextField(first,score?"ui_jar_contextLogicPoints":"ui_jar_contextLogicHits",score?-maxPoints:1,score?maxPoints:maxHits)||
      first!=std::floor(first)){
    Cvar_Set("jar_workshopStatus",score?"Use whole points from -10000 to 10000.":"Use a whole hit count from 1 to 1000.");return qtrue;
   }
   Com_sprintf(command,sizeof(command),"jar_sb context logic_apply %u %.0f\n",contextUiToken,first);
  }else return qtrue;
 }else if(!strcmp(action,"logic_clear_output")){
  const auto kind=static_cast<jar::logic::Kind>(atoi(Cvar_VariableString("jar_contextLogicKind")));
  if(!jar::logic::Slotted(kind))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb context logic_clear_output %u %d\n",contextUiToken,ContextLogicSlot(kind)+1);
 }else if(!strcmp(action,"logic_reset")||!strcmp(action,"logic_repeat")||!strcmp(action,"logic_enabled")||!strcmp(action,"logic_running")||!strcmp(action,"logic_kill_mode")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"prop_physics")||!strcmp(action,"prop_gravity")){
  if(strcmp(Cvar_VariableString("jar_contextPropPhysicsAllowed"),"1"))return qtrue;
  if(!strcmp(action,"prop_gravity"))Com_sprintf(command,sizeof(command),"jar_sb context prop_gravity %u\n",contextUiToken);
  else {
   float mass;
   if(!ContextField(mass,"ui_jar_contextPropMass",0,jar::workshop::maxPropMass)||mass!=std::floor(mass)){
    Cvar_Set("jar_workshopStatus","Use a whole mass from 1 to 100000, or 0 for the model default.");return qtrue;
   }
   Com_sprintf(command,sizeof(command),"jar_sb context prop_physics %u %.0f\n",contextUiToken,mass);
  }
 }else if(!strcmp(action,"replicator_apply")){
  float heal,delay;
  if(!ContextField(heal,"ui_jar_contextReplicatorHeal",0,jar::replicator::maxHeal)||heal!=std::floor(heal)||
     !ContextField(delay,"ui_jar_contextReplicatorDelay",jar::replicator::minDelay,jar::replicator::maxDelay)||delay!=std::floor(delay)){
   Cvar_Set("jar_workshopStatus","Enter whole healing from 0 to 100 and delay from 100 to 10000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context replicator_apply %u %.0f %.0f\n",contextUiToken,heal,delay);
 }else if(!strcmp(action,"shooter_interval")){
  float interval,variation;
  if(!ContextField(interval,"ui_jar_contextShooterInterval",jar::shooter::minInterval,jar::shooter::maxInterval)||interval!=std::floor(interval)||
     !ContextField(variation,"ui_jar_contextShooterVariation",0,jar::shooter::maxVariation)||variation!=std::floor(variation)){
   Cvar_Set("jar_workshopStatus","Use a whole interval of 100 to 30000 ms and variation of 0 to 30000 ms.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context shooter_interval %u %.0f %.0f\n",contextUiToken,interval,variation);
 }else if(!strcmp(action,"shooter_alternate")||!strcmp(action,"shooter_repeat")||!strcmp(action,"shooter_enabled")||
          !strcmp(action,"secret_repeat")||!strcmp(action,"secret_rearm")||!strcmp(action,"secret_enabled")||
          !strcmp(action,"replicator_enabled")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else if(!strcmp(action,"npc_goal_clear")){
  if(!atoi(Cvar_VariableString("jar_contextNPCOrderAllowed"))||!atoi(Cvar_VariableString("jar_contextNPCGoalLinked")))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb context npc_goal_clear %u\n",contextUiToken);
 }else if(!strcmp(action,"npc_order")){
  if(!atoi(Cvar_VariableString("jar_contextNPCOrderAllowed")))return qtrue;
  const int mode=ContextNPCOrderMode();
  float distance=jar::npcOrder::defaultDistance;
  if(mode==jar::npcOrder::Follow&&!ContextField(distance,"ui_jar_contextNPCOrderDistance",jar::npcOrder::minDistance,jar::npcOrder::maxDistance)){
   Cvar_Set("jar_workshopStatus","Use a follow distance from 72 to 1024 units.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context npc_order %u %d %.3f\n",contextUiToken,mode,distance);
 }else if(!strcmp(action,"npc_weapon")){
  const uint32_t choices=atoi(Cvar_VariableString("jar_contextNPCWeaponChoices"));
  if(!jar::npcWeapon::Name(contextNPCWeapon)||!(choices&(1u<<contextNPCWeapon)))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb context npc_weapon %u %d\n",contextUiToken,contextNPCWeapon);
 }else if(!strcmp(action,"npc_fire")){
  if(!atoi(Cvar_VariableString("jar_contextNPCFireAllowed")))return qtrue;
  float ammo,alternate,manualAmmo;
  if(!ContextField(ammo,"ui_jar_contextNPCAmmo",-1,32767)||ammo!=std::floor(ammo)||
     !ContextField(alternate,"ui_jar_contextNPCAlternate",0,1)||alternate!=std::floor(alternate)||
     !ContextField(manualAmmo,"ui_jar_contextNPCManualAmmo",0,1)||manualAmmo!=std::floor(manualAmmo)){
   Cvar_Set("jar_workshopStatus","Use whole ammo from -1 to 32767 and the listed fire and ammo modes.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context npc_fire %u %d %d %d\n",contextUiToken,int(ammo),int(alternate),int(manualAmmo));
 }else if(!strcmp(action,"npc_status")){
  if(!atoi(Cvar_VariableString("jar_contextNPCAllowed")))return qtrue;
  float team,target,armor;
  if(!ContextField(team,"ui_jar_contextNPCTeam",TEAM_FREE,TEAM_NUM_TEAMS-1)||team!=std::floor(team)||
     !ContextField(target,"ui_jar_contextNPCTargetTeam",TEAM_FREE,TEAM_NUM_TEAMS-1)||target!=std::floor(target)||
     !ContextField(armor,"ui_jar_contextNPCArmor",0,32767)||armor!=std::floor(armor)){
   Cvar_Set("jar_workshopStatus","Choose native NPC teams and whole armor from 0 to 32767.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context npc_status %u %d %d %d\n",contextUiToken,int(team),int(target),int(armor));
 }else if(!strcmp(action,"vehicle_resources")){
  if(!atoi(Cvar_VariableString("jar_contextVehicleAllowed")))return qtrue;
  jar::vehicleStatus::Values values;
  for(int i=0;i<jar::vehicleStatus::fields;++i){
   const int limit=atoi(Cvar_VariableString(va("jar_contextVehicleMax%d",i)));
   if(!limit&&i>0)continue;
   if(!jar::vehicleStatus::Field(Cvar_VariableString(va("ui_jar_contextVehicleValue%d",i)),i,limit,values.value[i])){
    Cvar_Set("jar_workshopStatus","Enter whole vehicle resource values within the displayed limits.");return qtrue;
   }
  }
  const auto& v=values.value;
  Com_sprintf(command,sizeof(command),"jar_sb context vehicle_resources %u %d %d %d %d %d %d\n",
   contextUiToken,v[0],v[1],v[2],v[3],v[4],v[5]);
 }else if(!strcmp(action,"thruster_properties")){
  uint32_t id;float force,on,reverse;
  if(!jar::context::Token(Cvar_VariableString("jar_contextThrusterId"),id))return qtrue;
  if(!ContextField(force,"ui_jar_contextThrusterForce",jar::constraint::thrusterMinForce,jar::constraint::thrusterMaxForce)||
     !ContextField(on,"ui_jar_contextThrusterEnabled",0,1)||on!=std::floor(on)||
     !ContextField(reverse,"ui_jar_contextThrusterReverse",0,1)||reverse!=std::floor(reverse)){
   Cvar_Set("jar_workshopStatus","Use force from 1 to 100000 and the supplied power and direction choices.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context thruster_properties %u %u %.6f %.0f %.0f\n",contextUiToken,id,force,on,reverse);
 }else if(!strcmp(action,"rope_length")){
  uint32_t ropeId;float length;
  if(!jar::context::Token(Cvar_VariableString("jar_contextRopeId"),ropeId))return qtrue;
  if(!ContextField(length,"ui_jar_contextRopeLength",jar::constraint::ropeMinLength,jar::constraint::ropeMaxLength)){
   Cvar_Set("jar_workshopStatus","Enter a rope length from 0.01 to 8192 world units.");return qtrue;
  }
  Com_sprintf(command,sizeof(command),"jar_sb context rope_length %u %u %.6f\n",contextUiToken,ropeId,length);
 }else if(!strcmp(action,"rope_next")){
  Com_sprintf(command,sizeof(command),"jar_sb context rope_next %u\n",contextUiToken);
 }else if(!strcmp(action,"health")){float health;if(!ContextField(health,"ui_jar_contextHealth",1,100000)||health!=std::floor(health)){Cvar_Set("jar_workshopStatus","Health must be a whole number from 1 to 100000.");return qtrue;}
  Com_sprintf(command,sizeof(command),"jar_sb context health %u %.0f\n",contextUiToken,health);
 }else if(!strcmp(action,"refresh")||!strcmp(action,"freeze")||!strcmp(action,"release")||!strcmp(action,"activate")||!strcmp(action,"remove")||!strcmp(action,"duplicate")||!strcmp(action,"paste")||!strcmp(action,"rope")||!strcmp(action,"thruster")||!strcmp(action,"unlink")||!strcmp(action,"weld")||!strcmp(action,"hinge")||!strcmp(action,"music_prev")||!strcmp(action,"music_next")){
  Com_sprintf(command,sizeof(command),"jar_sb context %s %u\n",action,contextUiToken);
 }else return qtrue;
 UI_JAR_EndTextEntry();ui.Cmd_ExecuteText(EXEC_APPEND,command);return qtrue;
}
