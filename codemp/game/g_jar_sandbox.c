#include "../../shared/qcommon/jar_thruster.h"
// Server-authoritative JA:R multiplayer Sandbox. GPL-2.0-only.
#include "g_local.h"
#include "../../shared/game/jar_fall_cleanup.h"
#include "../../shared/game/jar_rotation_sweep.h"
#include "../../shared/qcommon/jar_tool_style.h"
#include "../../shared/qcommon/jar_logic_marker.h"
#include "../../shared/qcommon/jar_sandbox_menu.h"
#include "g_jar_sandbox.h"
#include "bg_jar_physgun_input.h"
#include "jar_mp_catalog.h"
#include "jar_mp_section_admission.h"
#include "jar_mp_policy.h"
#include "jar_mp_context.h"
#include "jar_mp_prop.h"
#include "jar_mp_emplaced.h"
#include "jar_mp_push_volume.h"
#include "anims.h"
#include "bg_vehicles.h"
#include "b_local.h"
#include "qcommon/qfiles.h"
#include "jar_mp_physics.h"
#include "../../shared/qcommon/jar_rope_path.h"

typedef struct {int triggerHelper,triggerExtent,triggerActive,activationActor;unsigned activationActorEpoch;int itemConsumed,transientPickup;int laserHelper,laserActive;char controllerText[64];int mediaHelper,mediaActive;int output[2],distanceInput[2],activationClient;unsigned activationEpoch;int jointKind;vec4_t jointA,jointB;int owner,catalog,heldBy,frozen,first,second,thruster,thrustBy,noGravity,nextImpact,copyPending,copyHealth,copyFrozen,copyDeadline,thrustMounted,copyVehicleMotion;unsigned order,poseRevision,contextIdentity,propertyRevision;vec3_t velocity,angular,anchorA,anchorB,copyOrigin,copyAngles,thrustAnchor,thrustAxis,copyVehicleDrift;float length,mass,thrustForce,copyVehicleSpeed;jarRopePath_t ropePath;} jarObject_t;
enum { JAR_GRIP_EXPIRY_MS=100, JAR_GRIP_SAMPLE_MS=10, JAR_GRIP_SAMPLES=JAR_GRIP_EXPIRY_MS/JAR_GRIP_SAMPLE_MS+2 };
typedef struct {
 int time;
 vec3_t origin;
} jarGripSample_t;
typedef struct {
 jarGripSample_t samples[JAR_GRIP_SAMPLES];
 int first,count,lastMoveTime;
} jarGripMotion_t;
typedef struct {int firstTool,linkOutput;jarPropertyUndo_t propertyUndo;int contextEchoAt,contextOpen;vec4_t firstRotation;int poseUndoEntity,poseUndoFrame;unsigned poseUndoIdentity,poseUndoRevision;int held,nextRequest,tool,first,beam,beamUntil,lastButtons,selected,freezeLatch,noTools,noGrabUntil;float distance,ropeSlack,thrustForce,rotationStep;qboolean settingsInitialized;vec3_t firstAnchor;jarMPGrip_t grip;jarGripMotion_t motion;qboolean turnInput,thrustReverse;float turnYaw,turnPitch;} jarPlayer_t;
static jarObject_t objects[MAX_GENTITIES];
// A mount has its own creation order, independent of the body it is on.
static struct { unsigned order; int owner; } thrustCreation[MAX_GENTITIES];
static int thrusterVisual[MAX_GENTITIES];
static jarPlayer_t players[MAX_CLIENTS];
static vmCvar_t sandbox,sharedEditing,cleanupDisconnect,playerTools,mapEditing,worldEditing,worldLoad,worldAutoSave,worldAutoLoad,rigidEnabled;
static unsigned order;
static qboolean spawnStatsDirty;
static int spawnStatsPhysics;
static float mapFallBottom, fallFloor;
extern gentity_t *NPC_SpawnTypeAt(gentity_t *,char *,char *,qboolean,const vec3_t,const vec3_t);
static qboolean SkeletalModel(const char*name){const char*ext=strrchr(name,'.');return ext&&!Q_stricmp(ext,".glm");}
static void RopeReset(void);
static void RopeForget(int);
static qboolean RopeAdmission(int,float*);
static void RopePrepare(void);
static qboolean RopeRigid(int);
static void PhysicsReset(void);
static void JointReset(void);
static void JointClassify(qboolean rigid);
static qboolean JointNative(int n);
static void JointRecord(int n,jarMPJoint_t*);
static void JointFrame(float dt,qboolean rigid);
static void SelectionEcho(gentity_t*);
static void SelectionChanged(int);
static unsigned ContextIdentity(int n);
static int PhysicsFrameCount(int catalog);
static qboolean PhysicsModelBounds(int catalog,int frame,vec3_t mins,vec3_t maxs);
static qboolean PhysicsBodyBounds(int catalog,int frame,vec3_t mins,vec3_t maxs);
static qboolean PhysicsPoseHull(int catalog,int frame,const vec3_t angle,vec3_t mins,vec3_t maxs);
static qboolean PhysicsSetPose(gentity_t*,int);
static void PhysicsForget(int entity);
static qboolean PhysicsProp(int entity);
static unsigned PhysicsIdentity(int entity);
static qboolean PhysicsSolid(int entity);
static qboolean CullFallenObject(gentity_t *entity, const vec3_t origin, float localTop);
static void TrackTemporaryPickup(gentity_t*,int,unsigned);
static qboolean RackInit(gentity_t*,int);
static void RackState(int,jarControllerState_t*);
static void RackSync(int);
static void RackForget(int);
static void RackReset(void);
static qboolean DoorCameraInit(gentity_t*,const jarControllerState_t*);
static void DoorCameraState(int,jarControllerState_t*);
static qboolean DoorCameraApply(int,const jarControllerState_t*);
static void DoorCameraSync(int);
static void DoorCameraForget(int);
static void DoorCameraReset(void);
static void DoorCameraFrame(gentity_t*);
static int DoorCameraRoot(int);
static void DoorCameraBounds(const vec3_t,float,vec3_t,vec3_t);
static qboolean DoorCameraHull(int,const vec3_t,vec3_t,vec3_t);
static qboolean DoorCameraPickBounds(int,vec3_t,vec3_t);
static qboolean WelderInit(gentity_t*);
static void WelderState(int,jarControllerState_t*);
static qboolean WelderApply(int,const jarControllerState_t*);
static void WelderFrame(gentity_t*);
static int WelderRemaining(const gentity_t*);
static void WelderRestoreDeadline(gentity_t*,int);
static void WelderSetEnabled(gentity_t*);
static void SearchlightInit(gentity_t*);
static void SearchlightState(int,jarControllerState_t*);
static qboolean SearchlightApply(int,const jarControllerState_t*);
static void SearchlightClearClient(int);
static qboolean CrystalCrateInit(gentity_t*);
static void CrystalCrateState(int,jarControllerState_t*);
static qboolean CrystalCrateApply(int,const jarControllerState_t*);
static void CrystalCrateSync(int);
static void CrystalCrateForget(int);
static void CrystalCrateReset(void);
static void PhysicsCrystalCrateBounds(vec3_t,vec3_t);
static int PhysicsMoveMask(const gentity_t*);
static qboolean BreakInit(gentity_t*);
static void BreakReset(void);
static void BreakState(int,jarControllerState_t*);
static qboolean BreakApply(int,const jarControllerState_t*);
static qboolean EmplacedInit(gentity_t*,int);
static void EmplacedState(int,jarControllerState_t*);
static qboolean EmplacedApply(int,const jarControllerState_t*);
static void EmplacedEnabled(gentity_t*);
static void EmplacedSyncPose(gentity_t*);
static qboolean ShooterInit(gentity_t*,int);
static void ShooterState(int,jarControllerState_t*);
static qboolean ShooterApply(int,const jarControllerState_t*);
static void ShooterStop(gentity_t*);
static qboolean AnimatedInit(gentity_t*,int,qboolean);
static void AnimatedState(int,jarControllerState_t*);
static qboolean AnimatedApply(int,const jarControllerState_t*);
static void AnimatedSetEnabled(gentity_t*);
static void AnimatedFrame(gentity_t*,int,int);
static int AnimatedRemaining(const gentity_t*);
static qboolean UsableInit(gentity_t*);
static void UsableState(int,jarControllerState_t*);
static qboolean UsableApply(int,const jarControllerState_t*);
static int ControllerKind(int n);
static qboolean BodyDormant(int n){
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD)return qfalse;
 if(g_entities[n].s.eType==ET_GENERAL&&g_entities[n].s.generic1==250&&g_entities[n].s.bolt2==JAR_MP_WALL_OFF_TAG)return qtrue;
 if(!(g_entities[n].s.eFlags&EF_NODRAW))return qfalse;
 const int kind=ControllerKind(n);
 return kind==29||kind==JAR_BEACON_CONTROLLER||JAR_ControllerBreakable(kind);
}
static qboolean PhysicsSetAngles(gentity_t*entity,const vec3_t angles);
static qboolean PhysicsRotateAt(gentity_t*entity,const vec3_t origin,const vec3_t angles,const vec3_t contact);
static void PhysicsHull(gentity_t*entity,const vec3_t angles,vec3_t mins,vec3_t maxs);
static void PhysicsPublish(gentity_t*entity);
static void PortalState(int,jarControllerState_t*);
static qboolean PortalApply(int,const jarControllerState_t*);
static void PortalInit(gentity_t*,int,const jarControllerState_t*);
static void PortalSync(gentity_t*);
static jarMPPose_t PhysicsPose(int n);
static void PhysicsVelocity(int entity,const vec3_t velocity);
static void PhysicsStatus(void);
static qboolean PhysicsRope(int entity);
static qboolean PhysicsConstrained(int entity);
static void ForceDetach(int entity);
static void ForceRemoveThruster(int entity);
static void ForceClientReset(int client);
static void CopyReset(int client);
static void CopyForgot(int entity,int owner,unsigned group);
static void CopyFrame(void);
static void WorldReset(void);
static void WorldFrame(void);
static void WorldRestoreFailed(void);
static qboolean CopyOccupied(gentity_t*e);
static qboolean RemovalOccupied(gentity_t*e);
static qboolean CopyBusy(int owner,unsigned group);
static qboolean CopyBusyExcept(int owner,unsigned group,int departingClient);
static qboolean ForcePermission(int owner,int entity);
static float ForceMass(int entity);
static void ControllerForget(int entity);
static void MusicReset(void);
static qboolean MediaInit(gentity_t*e);
static void MediaState(int n,jarControllerState_t*s);
static qboolean MediaApply(int n,const jarControllerState_t*s);
static qboolean MediaValidate(int kind,const char*path);
static void MediaForget(int n);
static void MediaReset(void);
static void MediaFrame(void);
static void LaserReset(void);
static void LaserForget(int n);
static void LaserState(int n,jarControllerState_t*s);
static qboolean LaserApply(int n,const jarControllerState_t*s);
static qboolean LaserInit(gentity_t*e);
static qboolean LaserSync(int n,trace_t*out);
static qboolean MediaSync(int n);
static qboolean TriggerInit(gentity_t*);
static qboolean TriggerApply(int,const jarControllerState_t*);
static void TriggerState(int,jarControllerState_t*);
static void TriggerSync(int);
static qboolean DirectionalInit(gentity_t*);
static qboolean DirectionalApply(int,const jarControllerState_t*);
static void DirectionalState(int,jarControllerState_t*);
static void DirectionalSync(int);
static void TriggerPublishPush(gentity_t*,gentity_t*);
static void TriggerFrame(void);
static void TriggerPropBegin(void);
static void TriggerPropFrame(void);
static void TriggerPropReset(void);
static void TriggerTransportTouch(gentity_t*,gentity_t*,gentity_t*,trace_t*);
static qboolean StationInit(gentity_t*);
static void StationStop(gentity_t*);
static void StationState(int,jarControllerState_t*);
static qboolean StationApply(int,const jarControllerState_t*);
static void TriggerReset(void);
static void TriggerForget(int);
static unsigned controllerEntityEpoch[MAX_GENTITIES];
static unsigned controllerClientEpoch[MAX_CLIENTS];
static byte controllerCalling[MAX_GENTITIES];static int controllerDepth;
static qboolean LaserDamageAllowed(int owner,int n);
static qboolean HazardPlayerAllowed(int owner,int n);
static qboolean SectionCard(const jarMPEntry_t*);
static qboolean SectionRoot(int);
static void SectionForget(int);
static void SectionReset(void);
static void SectionFireSync(int);
static qboolean CatalogBody(const jarMPEntry_t*e){return e->kind==1||(e->kind==5&&!SectionCard(e));}
static qboolean NativeMapProp(int n){return objects[n].owner==MAX_CLIENTS+1&&g_entities[n].classname&&
 !strcmp(g_entities[n].classname,"jar_construct_furniture");}

typedef struct {int owned[MAX_CLIENTS],actors[MAX_CLIENTS],props;} jarSpawnStats_t;
static jarSpawnStats_t StatsRead(void){
 jarSpawnStats_t stats={0};
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  if(!g_entities[n].inuse||!objects[n].owner)continue;
  if(PhysicsProp(n))++stats.props;
  const int client=objects[n].owner-1;
  if(client>=MAX_CLIENTS||objects[n].transientPickup||g_entities[n].s.generic1==251)continue;
  ++stats.owned[client];
  if(g_entities[n].NPC)++stats.actors[client];
 }
 return stats;
}
static void StatsSend(int client,const jarSpawnStats_t*stats){
 trap->SendServerCommand(client,va("jarstats %d %d %d %d",stats->owned[client],stats->props,
  rigidEnabled.integer?1:0,stats->actors[client]));
}
static void StatsEcho(gentity_t*p){const jarSpawnStats_t stats=StatsRead();StatsSend(p->s.number,&stats);}
static void StatsFrame(void){
 const int physics=rigidEnabled.integer?1:0;
 if(!spawnStatsDirty&&spawnStatsPhysics==physics)return;
 spawnStatsDirty=qfalse;spawnStatsPhysics=physics;
 const jarSpawnStats_t stats=StatsRead();
 for(int c=0;c<MAX_CLIENTS;++c){
  const gentity_t* p=&g_entities[c];
  if(p->inuse&&p->client&&p->client->pers.connected==CON_CONNECTED)StatsSend(c,&stats);
 }
}
static void Tell(gentity_t*p,const char*s){
 if(!p){trap->Print("JA:R: %s\n",s);return;}
 char clean[240];int i;for(i=0;s[i]&&i<(int)sizeof(clean)-1;++i){unsigned char c=(unsigned char)s[i];clean[i]=c<32||c=='"'||c=='\\'?' ':c;}clean[i]=0;
 trap->SendServerCommand(p->s.number,va("jarstatus \"%s\"",clean));trap->SendServerCommand(p->s.number,va("print \"JA:R: %s\n\"",clean));
}
static qboolean Alive(gentity_t*p){return p&&p->s.number>=0&&p->s.number<MAX_CLIENTS&&p->inuse&&p->client&&p->client->pers.connected==CON_CONNECTED&&p->health>0&&p->client->sess.sessionTeam!=TEAM_SPECTATOR&&!p->client->ps.m_iVehicleNum;}
static qboolean Edit(gentity_t*p,int n){return n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&g_entities[n].inuse&&(objects[n].owner==MAX_CLIENTS+1?worldEditing.integer:JAR_MP_CanEdit(p->s.number,objects[n].owner,sharedEditing.integer));}
static qboolean RoomExtra(int extra){int free=0;for(int i=MAX_CLIENTS;i<ENTITYNUM_WORLD;++i)if(!g_entities[i].inuse&&++free>=JAR_MP_ENTITY_RESERVE+16+extra)return qtrue;return qfalse;}
static qboolean Room(void){return RoomExtra(0);}
static qboolean ModelRoom(const char *model){int available=0;char path[MAX_QPATH];for(int i=1;i<MAX_MODELS;++i){trap->GetConfigstring(CS_MODELS+i,path,sizeof(path));if(*model&&!Q_stricmp(path,model))return qtrue;if(!*path)++available;}return available>=64;}
static const jarMPEntry_t *Entry(unsigned id,int *index){for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].id==id){*index=i;return &jarMPCatalog[i];}return NULL;}
static void AimWithFlags(gentity_t*p,trace_t*tr,float range,int flags){vec3_t from,to,forward;VectorCopy(p->client->ps.origin,from);from[2]+=p->client->ps.viewheight;AngleVectors(p->client->ps.viewangles,forward,NULL,NULL);VectorMA(from,range,forward,to);trap->Trace(tr,from,NULL,NULL,to,p->s.number,MASK_PLAYERSOLID,qfalse,flags,0);}
static void Aim(gentity_t*p,trace_t*tr,float range){AimWithFlags(p,tr,range,0);}
static void Move(gentity_t*e,const vec3_t pos){G_SetOrigin(e,(float*)pos);if(e->client){VectorCopy(pos,e->client->ps.origin);VectorClear(e->client->ps.velocity);}EmplacedSyncPose(e);e->s.pos.trType=TR_INTERPOLATE;DoorCameraSync(e->s.number);trap->LinkEntity((sharedEntity_t*)e);RackSync(e->s.number);CrystalCrateSync(e->s.number);}
static void Beam(gentity_t *p,const vec3_t end,qboolean tool){
 jarPlayer_t *cl=&players[p->s.number];gentity_t *beam;
 if(cl->beam==ENTITYNUM_NONE){if(!Room())return;beam=G_TrySpawn();if(!beam)return;cl->beam=beam->s.number;beam->classname="jar_tool_effect";beam->s.eType=ET_GENERAL;beam->r.svFlags|=SVF_BROADCAST;}
 beam=&g_entities[cl->beam];vec3_t start,forward,right;AngleVectors(p->client->ps.viewangles,forward,right,NULL);VectorCopy(p->client->ps.origin,start);start[2]+=p->client->ps.viewheight-5;VectorMA(start,14,forward,start);VectorMA(start,5,right,start);
 beam->s.generic1=tool?253:252;beam->s.otherEntityNum=p->s.number;beam->s.otherEntityNum2=tool?ENTITYNUM_NONE:cl->held;
 // Mark local-space endpoints explicitly so newer clients can also draw beams from older servers.
 beam->s.time2=tool?0:1;
 if(!tool)VectorCopy(cl->grip.localPoint,beam->s.angles2);
 VectorCopy(end,beam->s.origin2);Move(beam,start);cl->beamUntil=level.time+(tool?140:100);
}
static void EndActor(int n,qboolean fling);
static void ClearGrip(int client){
 jarPlayer_t*p=&players[client];p->held=ENTITYNUM_NONE;memset(&p->grip,0,sizeof(p->grip));
 p->motion.count=0;
 p->turnInput=qfalse;p->turnYaw=p->turnPitch=0;
 if(g_entities[client].client)g_entities[client].client->ps.stats[STAT_JAR_PHYSGUN_HOLD]=0;
 if(p->beam<ENTITYNUM_WORLD&&g_entities[p->beam].inuse&&g_entities[p->beam].s.generic1==252){int beam=p->beam;p->beam=ENTITYNUM_NONE;G_FreeEntity(&g_entities[beam]);}
}
static void Drop(int client,qboolean freeze){
 jarPlayer_t*p=&players[client];int n=p->held;
 if(n>=0&&n<ENTITYNUM_WORLD){
  if(SectionRoot(n)){objects[n].heldBy=0;objects[n].frozen=1;}
  else if(g_entities[n].client){
   if(freeze&&JAR_MP_ActorControlled(&g_entities[n])){objects[n].heldBy=0;objects[n].frozen=1;VectorClear(objects[n].velocity);}
   else{
    if(p->motion.count&&level.time-p->motion.lastMoveTime>=JAR_GRIP_EXPIRY_MS)
     VectorClear(objects[n].velocity);
    EndActor(n,qtrue);
   }
  }else{objects[n].heldBy=0;JAR_RigidRelease(n,objects[n].mass,objects[n].noGravity?0:1);if(freeze){objects[n].frozen=1;VectorClear(objects[n].velocity);VectorClear(objects[n].angular);}}
 }
 ClearGrip(client);
}
#include "bg_jar_vehicle_drift.h"
#include "g_jar_actor.inc"
#include "g_jar_location.inc"
void JAR_MP_Forget(int entity){
 if(entity<0||entity>=MAX_GENTITIES)return;
 thrustCreation[entity].order=0;
 if(JAR_MP_EmplacedState(&g_entities[entity].s))JAR_MP_NativeEmplacedDetach(&g_entities[entity]);
 int visual=thrusterVisual[entity]-1;thrusterVisual[entity]=0;
 if(visual>=MAX_CLIENTS&&visual<ENTITYNUM_WORLD&&g_entities[visual].inuse&&g_entities[visual].s.generic1==254)G_FreeEntity(&g_entities[visual]);
 if(g_entities[entity].s.generic1==254){int source=g_entities[entity].s.otherEntityNum;if(source>=0&&source<ENTITYNUM_WORLD&&thrusterVisual[source]==entity+1)thrusterVisual[source]=0;}
 const int copyOwner=objects[entity].owner;const unsigned copyGroup=objects[entity].order;
 if(copyOwner&&!objects[entity].transientPickup&&g_entities[entity].s.generic1!=251)spawnStatsDirty=qtrue;
 RackForget(entity);DoorCameraForget(entity);CrystalCrateForget(entity);LocationForget(entity);++controllerEntityEpoch[entity];SectionForget(entity);RopeForget(entity);TriggerForget(entity);LaserForget(entity);MediaForget(entity);ControllerForget(entity);PhysicsForget(entity);EndActor(entity,qfalse);
 trap->UnlinkEntity((sharedEntity_t*)&g_entities[entity]); // Actor restoration publishes a final snapshot; deletion must remain unlinked.
 if(copyOwner&&g_entities[entity].NPC){gentity_t*goal=g_entities[entity].NPC->tempGoal;if(goal&&goal->inuse&&goal->parent==&g_entities[entity]){g_entities[entity].NPC->tempGoal=NULL;G_FreeEntity(goal);}}
 if(copyOwner&&g_entities[entity].m_pVehicle){gentity_t*droid=(gentity_t*)g_entities[entity].m_pVehicle->m_pDroidUnit;g_entities[entity].m_pVehicle->m_pDroidUnit=NULL;if(droid&&droid->inuse&&droid->s.number>=MAX_CLIENTS&&droid->client&&droid->client->ps.m_iVehicleNum==entity){if(droid->NPC){gentity_t*goal=droid->NPC->tempGoal;if(goal&&goal->inuse&&goal->parent==droid){droid->NPC->tempGoal=NULL;G_FreeEntity(goal);}}G_FreeEntity(droid);}}
 for(int c=0;c<MAX_CLIENTS;++c){if(players[c].held==entity)Drop(c,qfalse);if(players[c].first==entity)players[c].first=ENTITYNUM_NONE;if(players[c].beam==entity)players[c].beam=ENTITYNUM_NONE;if(players[c].selected==entity){players[c].selected=ENTITYNUM_NONE;if(g_entities[c].inuse&&g_entities[c].client&&g_entities[c].client->pers.connected==CON_CONNECTED)SelectionEcho(&g_entities[c]);}}
 CopyForgot(entity,copyOwner,copyGroup);
 memset(&objects[entity],0,sizeof(objects[entity]));
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner&&(objects[n].first==entity+1||objects[n].second==entity+1))G_FreeEntity(&g_entities[n]);
}
void JAR_MP_Disconnect(int client){
 for(int n=0;n<level.num_entities;++n)if(thrustCreation[n].owner==client+1)thrustCreation[n].order=0;
 ++controllerClientEpoch[client];byte retain[MAX_GENTITIES]={0};CopyReset(client);JAR_MP_ClientReset(client);
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==client+1&&objects[n].copyPending&&g_entities[n].inuse&&!CopyBusyExcept(client+1,objects[n].order,client))G_FreeEntity(&g_entities[n]);
 if(players[client].beam<ENTITYNUM_WORLD)G_FreeEntity(&g_entities[players[client].beam]);
 // Decide whole-group retention before changing ownership, including support
 // used by an actor who will remain in the world.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==client+1)retain[n]=!cleanupDisconnect.integer||CopyBusyExcept(client+1,objects[n].order,client);
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==client+1&&g_entities[n].inuse){if(!retain[n])G_FreeEntity(&g_entities[n]);else {objects[n].owner=MAX_CLIENTS+1;spawnStatsDirty=qtrue;if(objects[n].thrustBy==client+1)objects[n].thrustBy=MAX_CLIENTS+1;}}
 memset(&players[client],0,sizeof(players[client]));players[client].tool=7;players[client].held=players[client].first=players[client].beam=players[client].selected=ENTITYNUM_NONE;
}
void JAR_MP_Init(void){
 memset(thrustCreation,0,sizeof(thrustCreation));
 spawnStatsDirty=qtrue;spawnStatsPhysics=0;
 memset(locationOwners,0,sizeof(locationOwners));
 SectionReset();RopeReset();TriggerReset();memset(controllerEntityEpoch,0,sizeof(controllerEntityEpoch));LaserReset();MediaReset();MusicReset();BreakReset();RackReset();DoorCameraReset();CrystalCrateReset();memset(thrusterVisual,0,sizeof(thrusterVisual));memset(controllerClientEpoch,0,sizeof(controllerClientEpoch));memset(controllerCalling,0,sizeof(controllerCalling));controllerDepth=0;WorldReset();PhysicsReset();
 for(int c=0;c<MAX_CLIENTS;++c)CopyReset(c);
 memset(objects,0,sizeof(objects));memset(players,0,sizeof(players));memset(actors,0,sizeof(actors));order=0;
 mapFallBottom=MIN_WORLD_COORD;
 if(trap->Cvar_VariableStringBuffer){
  char value[32],*end;
  trap->Cvar_VariableStringBuffer("sv_jarMapBottom",value,sizeof(value));
  const float bottom=strtof(value,&end);
  if(end!=value&&!*end&&isfinite(bottom)&&bottom>=MIN_WORLD_COORD&&bottom<=MAX_WORLD_COORD)mapFallBottom=bottom;
 }
 fallFloor=JAR_FallFloor(mapFallBottom);
 for(int i=0;i<MAX_CLIENTS;++i){players[i].tool=7;players[i].held=players[i].first=players[i].beam=players[i].selected=ENTITYNUM_NONE;}
 trap->Cvar_Register(&sandbox,"jar_sandbox","1",CVAR_SERVERINFO|CVAR_ARCHIVE);
 trap->Cvar_Register(&sharedEditing,"jar_sharedEditing","1",CVAR_SERVERINFO|CVAR_ARCHIVE);
 trap->Cvar_Register(&cleanupDisconnect,"jar_cleanupDisconnect","0",CVAR_ARCHIVE);
 trap->Cvar_Register(&playerTools,"jar_playerTools","1",CVAR_SERVERINFO|CVAR_ARCHIVE);
 trap->Cvar_Register(&mapEditing,"jar_mapActorEditing","1",CVAR_SERVERINFO|CVAR_ARCHIVE);
 trap->Cvar_Register(&worldEditing,"jar_worldEditing","1",CVAR_SERVERINFO|CVAR_ARCHIVE);
 trap->Cvar_Register(&worldLoad,"jar_worldLoad","",CVAR_ARCHIVE);
 trap->Cvar_Register(&worldAutoSave,"jar_worldAutoSave","120",CVAR_ARCHIVE);
 trap->Cvar_Register(&worldAutoLoad,"jar_worldAutoLoad","1",CVAR_ARCHIVE);
 trap->Cvar_Register(&rigidEnabled,"jar_mpBox3D","1",CVAR_ARCHIVE);
 trap->Cvar_Register(NULL,"jar_mpVersion","100",CVAR_SERVERINFO|CVAR_ROM);
 trap->Cvar_Set("jar_mpVersion","100");
}
static void Track(gentity_t*e,int client,int catalog){jarObject_t*o=&objects[e->s.number];memset(o,0,sizeof(*o));o->owner=client+1;o->catalog=catalog+1;o->order=++order;o->mass=80;o->thrustForce=3000;VectorSet(o->thrustAxis,0,0,1);if(catalog>=0){const jarMPEntry_t*item=&jarMPCatalog[catalog];o->mass=fminf(1500,fmaxf(5,item->radius*item->radius*4*item->height*.001f));spawnStatsDirty=qtrue;}}
void JAR_MP_TrackConsoleNPC(gentity_t* player,gentity_t* entity){
 if(!sandbox.integer||!Alive(player)||!entity||!entity->inuse||!entity->NPC||!entity->client||!entity->NPC_type)return;
 const int kind=entity->m_pVehicle?3:2;
 const int tab=kind==3?4:1;
 int catalog=-1;
 for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==kind&&
    jarMPCatalog[i].tab==tab&&!Q_stricmp(jarMPCatalog[i].name,entity->NPC_type)){
  catalog=(int)i;
  break;
 }
 Track(entity,player->s.number,catalog);
 spawnStatsDirty=qtrue;
 StatsEcho(player);
 if(catalog<0)Tell(player,"NPC added to Undo and Despawn All. World saves require a catalog entry for this NPC.");
}
qboolean JAR_MP_AutonomousNPC(const gentity_t *entity)
{
 return sandbox.integer && !objects[entity->s.number].copyPending;
}

gentity_t *JAR_MP_NPCLeader(const gentity_t *entity)
{
 const int owner = objects[entity->s.number].owner - 1;
 if (owner < 0 || owner >= MAX_CLIENTS || !Alive(&g_entities[owner]))
  return NULL;
 gentity_t *player = &g_entities[owner];
 return player->client->playerTeam == entity->client->playerTeam ? player : NULL;
}

qboolean JAR_MP_AllowNPCOverlap(const gentity_t* entity){
 const int n=entity->s.number;
 return sandbox.integer&&n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&objects[n].owner?qtrue:qfalse;
}

qboolean JAR_MP_ConstructFurniture(gentity_t *entity){
 if(!level.spawning||Q_stricmp(level.rawmapname,"maps/gm_construct")||entity->spawnflags!=1||entity->health||!entity->model)return qfalse;
 int index=-1;
 for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i){
  if(jarMPCatalog[i].tab<0&&!Q_stricmp(jarMPCatalog[i].model,entity->model)){index=i;break;}
 }
 if(index<0)return qfalse;

 char *massText,*end;
 float gravity,mass;
 vec3_t authoredMins,authoredMaxs,mins,maxs;
 if(!G_SpawnFloat("gravity","0",&gravity)||gravity!=1||
    !G_SpawnString("mass","0",&massText)||
    !G_SpawnVector("mins","0 0 0",authoredMins)||!G_SpawnVector("maxs","0 0 0",authoredMaxs)||
    !PhysicsPoseHull(index+1,0,entity->s.angles,mins,maxs))return qfalse;
 mass=strtof(massText,&end);
 if(end==massText||*end||!isfinite(mass)||mass<JAR_MPCatalogMinMass(&jarMPCatalog[index])||mass>1500)return qfalse;
 for(int a=0;a<3;++a){
  if(!isfinite(authoredMins[a])||!isfinite(authoredMaxs[a])||authoredMins[a]>=authoredMaxs[a]||
     authoredMins[a]<mins[a]-.125f||authoredMaxs[a]>maxs[a]+.125f)return qfalse;
 }

 // Seat the authored model against the floor using the same initial hull as SP.
 vec3_t top,bottom;
 VectorCopy(entity->s.origin,top);
 VectorCopy(top,bottom);
 top[2]+=1;
 bottom[2]=MIN_WORLD_COORD;
 trace_t floor;
 trap->Trace(&floor,top,authoredMins,authoredMaxs,bottom,entity->s.number,MASK_PLAYERSOLID,qfalse,0,0);
 if(!floor.startsolid&&!floor.allsolid&&floor.fraction<1)VectorCopy(floor.endpos,entity->s.origin);

 entity->classname="jar_construct_furniture";
 entity->s.eType=ET_GENERAL;
 entity->r.contents=CONTENTS_SOLID;
 entity->clipmask=MASK_PLAYERSOLID;
 entity->mass=mass;
 VectorCopy(authoredMins,entity->r.mins);
 VectorCopy(authoredMaxs,entity->r.maxs);
 VectorCopy(entity->s.angles,entity->r.currentAngles);
 VectorCopy(entity->s.angles,entity->s.apos.trBase);
 entity->s.apos.trType=TR_INTERPOLATE;
 Track(entity,MAX_CLIENTS,index);
 objects[entity->s.number].mass=mass;
 PhysicsPublish(entity);
 Move(entity,entity->s.origin);
 return qtrue;
}
#include "g_jar_music.inc"
#include "g_jar_controllers.inc"
#include "g_jar_searchlight.inc"
#include "g_jar_portal.inc"
#include "g_jar_visible.inc"
#include "g_jar_shooter.inc"
#include "g_jar_actions.inc"
#include "g_jar_animated.inc"
#include "g_jar_stations.inc"
#include "g_jar_usable.inc"
#include "g_jar_breakable.inc"
#include "g_jar_emplaced.inc"
#include "g_jar_triggers.inc"
#include "g_jar_directional.inc"
#include "g_jar_transport.inc"

gentity_t *JAR_MP_SelectPortableStart(const gentity_t *player,qboolean entering,
 const vec3_t mins,const vec3_t maxs,vec3_t origin,vec3_t angles){
 if(!player||!player->client||!sandbox.integer||(level.gametype!=GT_FFA&&level.gametype!=GT_TEAM)||
    player->client->sess.sessionTeam==TEAM_SPECTATOR)return NULL;
 const int expected=entering?JAR_PLAYER_START_CONTROLLER:JAR_DEATHMATCH_START_CONTROLLER;
 gentity_t *best=NULL;
 float bestDistance=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  if(ControllerKind(n)!=expected||!objects[n].frozen||objects[n].heldBy||objects[n].copyPending)continue;
  gentity_t *marker=&g_entities[n];
  if(!(marker->spawnflags&1)||(marker->flags&FL_INACTIVE))continue;
  vec3_t unused,arrival;
  if(!JAR_OwnedTeleportArrival(marker->r.currentOrigin,0,mins[2],unused,arrival))continue;
  const float distance=DistanceSquared(player->client->ps.origin,arrival);
  if(!isfinite(distance)||distance<=bestDistance)continue;
  trace_t space;
  trap->Trace(&space,arrival,mins,maxs,arrival,player->s.number,MASK_PLAYERSOLID,qfalse,0,0);
  if(space.startsolid||space.allsolid)continue;
  best=marker;bestDistance=distance;
  VectorCopy(arrival,origin);VectorCopy(marker->r.currentAngles,angles);
 }
 return best;
}
#include "g_jar_trigger_transport.inc"
#include "g_jar_push_prediction.inc"
#include "g_jar_media.inc"
#include "g_jar_welder.inc"
#include "g_jar_crystal_crate.inc"
#include "g_jar_laser.inc"
#include "g_jar_section.inc"
// A native drop belongs to the spawn group for Undo and cleanup. Its
// native pickup and timeout stay independent of the world journal.
static void TrackTemporaryPickup(gentity_t*pickup,int owner,unsigned group){
 Track(pickup,owner-1,-1);
 jarObject_t*drop=&objects[pickup->s.number];
 drop->order=group;
 drop->transientPickup=1;
}
#include "g_jar_rack.inc"
#include "g_jar_door_camera.inc"
static gentity_t *SpawnAtPoseState(gentity_t*p,unsigned id,const vec3_t point,const vec3_t angles,int frame,const jarControllerState_t*saved){
 int index;const jarMPEntry_t*item=Entry(id,&index);gentity_t*e=NULL;vec3_t mins,maxs;
 if(SectionCard(item)){
  jarMPSectionAdmission_t data;
  return !frame&&SectionRead(p,SectionKind(item),&data)?SectionSpawn(p,index,point,angles,&data):NULL;
 }
 if(!item||!Room()||!ModelRoom(CatalogBody(item)?item->model:"")){Tell(p,"Spawn unavailable or entity/model reserve reached.");return NULL;}
 const int controller=item->kind==5?ControllerClass(item->name):0;
 if(controller==JAR_PORTAL_SURFACE_CONTROLLER&&PortalSurfaceCount()>=JAR_PORTAL_SURFACE_LIMIT){
  Tell(p,"The map already has 128 portable portal surfaces.");return NULL;
 }
 const qboolean hiddenBeacon=controller==JAR_BEACON_CONTROLLER&&saved&&saved->active==JAR_MP_BEACON_HIDDEN;
 const qboolean shotOnly=controller==JAR_CRYSTAL_CRATE_CONTROLLER&&saved&&(saved->flags&JAR_MP_CRYSTAL_CRATE_SHOT_ONLY);
 if(frame&&(JAR_ControllerEmplaced(controller)||controller==JAR_REFLECTION_CONTROLLER||controller==JAR_BOBBING_CONTROLLER||JAR_ControllerAngular(controller)))return NULL;
 if(controller==JAR_REFLECTION_CONTROLLER){
  jarProbe_t probe={1,1,{point[0],point[1],point[2]},JAR_PROBE_DEFAULT_RADIUS};
  if(!JAR_ProbeValid(&probe)){Tell(p,"Reflection probe is outside supported world bounds.");return NULL;}
  if(ReflectionCount(-1)>=JAR_PROBE_LIMIT){Tell(p,"The reflection probe capacity is 64 per map.");return NULL;}
 }
 VectorSet(mins,-item->radius,-item->radius,item->bottom);VectorSet(maxs,item->radius,item->radius,item->bottom+item->height);
 if(CatalogBody(item)){
  int props=0;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(PhysicsProp(n))++props;
  if(props>=JAR_MP_PROP_LIMIT){Tell(p,"The fixed Sandbox prop capacity is 4096.");return NULL;}
  if(!PhysicsPoseHull(index+1,frame,angles,mins,maxs)){Tell(p,"Required model pose/collision bounds are unavailable.");return NULL;}
  if(controller==JAR_WALL_CONTROLLER&&saved&&!saved->active)WallMarkerHull(mins,maxs);
  if(controller==JAR_DOOR_CAMERA_CONTROLLER&&saved)
   DoorCameraBounds(angles,saved->pushSpeed/(float)JAR_MP_DOOR_CAMERA_MILLI,mins,maxs);
  if(controller==JAR_PORTAL_SURFACE_CONTROLLER&&saved)
   JAR_MP_PortalHull(saved->speed,saved->pushSpeed,angles,mins,maxs);
  if(!hiddenBeacon){
   trace_t space;trap->Trace(&space,point,mins,maxs,point,ENTITYNUM_NONE,shotOnly?MASK_SOLID:MASK_PLAYERSOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);
   if(space.startsolid||space.allsolid){Tell(p,"Not enough clear space for the prop's full hull.");return NULL;}
  }
 }else if(frame)return NULL;
 if(item->kind==2||item->kind==3){
  e=NPC_SpawnTypeAt(p,(char*)item->name,NULL,item->kind==3,point,angles);
 }else if(item->kind==4){const char*classname=JAR_MPCatalogPickupClass(item);gitem_t*pickup=NULL;for(int i=1;i<bg_numItems;++i)if(bg_itemlist[i].classname&&!Q_stricmp(bg_itemlist[i].classname,classname)){pickup=&bg_itemlist[i];break;}if(!pickup)return NULL;
  e=G_TrySpawn();if(!e)return NULL;e->classname=pickup->classname;VectorCopy(point,e->s.origin);
  // Runtime pickups must not inherit the final BSP entity's wait/random keys.
  int oldCount=level.numSpawnVars;qboolean oldSpawning=level.spawning;
  level.numSpawnVars=0;level.spawning=qtrue;G_SpawnItem(e,pickup);level.numSpawnVars=oldCount;level.spawning=oldSpawning;
  if(!e->inuse||e->item!=pickup){if(e->inuse)G_FreeEntity(e);Tell(p,"This pickup is disabled by the server's item/weapon rules.");return NULL;}SaveRegisteredItems();
 }else if(CatalogBody(item)){
  e=G_TrySpawn();if(!e)return NULL;e->classname=JAR_ControllerStart(controller)?"jar_start_marker":item->kind==5?(char*)item->name:"jar_prop";e->model=(char*)item->model;e->s.modelindex=G_ModelIndex(e->model);e->s.eType=ET_GENERAL;e->s.frame=frame;e->s.modelGhoul2=SkeletalModel(item->model);
  e->r.contents=JAR_ControllerPointGizmo(controller)?CONTENTS_ITEM:CONTENTS_SOLID;
  if(JAR_MPCatalogLogicMarker(item))e->s.bolt2=JAR_MP_LOGIC_MARKER_TAG;
  if(controller==JAR_AMMO_RACK_CONTROLLER){
   e->s.bolt2=JAR_MP_AMMO_RACK_CLIP_TAG;
   e->r.contents=JAR_MP_AMMO_RACK_CONTENTS;
  }else if(controller==JAR_DOOR_CAMERA_CONTROLLER){
   e->s.bolt2=JAR_MP_DOOR_CAMERA_ROOT_TAG;
   e->r.contents=CONTENTS_ITEM;
  }else if(controller==JAR_WELDER_CONTROLLER){
   e->s.bolt2=JAR_MP_WELDER_TAG;
   e->r.contents=CONTENTS_ITEM;
  }else if(controller==JAR_CRYSTAL_CRATE_CONTROLLER){
   e->s.bolt2=JAR_MP_CRYSTAL_CRATE_TAG;
   e->r.contents=shotOnly?CONTENTS_CORPSE:JAR_MP_CRYSTAL_CRATE_SOLID_CONTENTS;
  }
  e->clipmask=MASK_PLAYERSOLID;VectorCopy(mins,e->r.mins);VectorCopy(maxs,e->r.maxs);e->health=100;e->takedamage=qfalse;
 }
 if(!e||!e->inuse){Tell(p,"The multiplayer NPC/item parser could not create that entry.");return NULL;}
 if(controller==JAR_EWEB_CONTROLLER||JAR_ControllerAnimatedTag(controller)){
  G_SetOrigin(e,(float*)point);
  VectorCopy(angles,e->r.currentAngles);VectorCopy(angles,e->s.angles);VectorCopy(angles,e->s.apos.trBase);
 }
 Track(e,p?p->s.number:MAX_CLIENTS,index);if(item->kind==5){objects[e->s.number].frozen=1;if(!ControllerInit(e,item,hiddenBeacon,saved)){G_FreeEntity(e);Tell(p,"Sound/effect asset, helper capacity or resource reserve unavailable.");return NULL;}}VectorCopy(angles,e->r.currentAngles);VectorCopy(angles,e->s.angles);VectorCopy(angles,e->s.apos.trBase);if(e->client)SetClientViewAngle(e,(float*)angles);VehicleOrientation(e);
 if(e->m_pVehicle){if(!VehicleBounds(e,angles,mins,maxs)){G_FreeEntity(e);Tell(p,"Vehicle collision bounds are unavailable.");return NULL;}VectorCopy(mins,e->r.mins);VectorCopy(maxs,e->r.maxs);}
 if(CatalogBody(item)){
  vec3_t low,high;PhysicsBodyBounds(index+1,frame,low,high);float volume=1;
  for(int a=0;a<3;++a)volume*=fmaxf(.5f,high[a]-low[a]);objects[e->s.number].mass=fminf(1500,fmaxf(5,volume*.001f));
  PhysicsPublish(e);
 }
 Move(e,point);
 if(controller==JAR_DOOR_CAMERA_CONTROLLER&&!DoorCameraInit(e,saved)){
  G_FreeEntity(e);Tell(p,"Door camera parts or resource reserve unavailable.");return NULL;
 }
 if(controller==JAR_WELDER_CONTROLLER&&!WelderInit(e)){
  G_FreeEntity(e);Tell(p,"Stock welder assets or resource reserve unavailable.");return NULL;
 }
 if(controller==JAR_CRYSTAL_CRATE_CONTROLLER&&!CrystalCrateInit(e)){
  G_FreeEntity(e);Tell(p,"Crystal crate assets or resource reserve unavailable.");return NULL;
 }
 if(JAR_ControllerRack(controller)&&!RackInit(e,saved?saved->flags:0)){
  G_FreeEntity(e);Tell(p,"Rack stock is unavailable or entity/model reserve reached.");return NULL;
 }
 if(JAR_ControllerTrigger(ControllerKind(e->s.number))||ControllerKind(e->s.number)==JAR_DIRECTIONAL_CONTROLLER)TriggerSync(e->s.number);if(ControllerKind(e->s.number)==13)LaserSync(e->s.number,NULL);if(item->kind==5&&JAR_ControllerMedia(ControllerClass(item->name)))MediaSync(e->s.number);
 if(e->m_pVehicle){trace_t space;trap->Trace(&space,point,e->r.mins,e->r.maxs,point,e->s.number,MASK_PLAYERSOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);if(space.startsolid||space.allsolid){G_FreeEntity(e);Tell(p,"Not enough space for the native vehicle hull.");return NULL;}}
 return e;
}
static gentity_t *SpawnAtPose(gentity_t*p,unsigned id,const vec3_t point,const vec3_t angles,int frame){return SpawnAtPoseState(p,id,point,angles,frame,NULL);}
static gentity_t *SpawnAt(gentity_t*p,unsigned id,const vec3_t point,const vec3_t angles){return SpawnAtPose(p,id,point,angles,0);}
static qboolean SpawnPairedFeedback(gentity_t*p,gentity_t*root,const vec3_t point,const vec3_t angles,qboolean directional){
 vec3_t forward,right;AngleVectors(angles,forward,right,NULL);
 for(int role=JAR_DISTANCE_INSIDE;role<=JAR_DISTANCE_OUTSIDE;++role){
  vec3_t next;
  if(directional){VectorMA(point,role==0?128:-128,forward,next);next[2]+=16;}
  else{VectorMA(point,72,forward,next);VectorMA(next,role==JAR_DISTANCE_INSIDE?-48:48,right,next);next[2]+=16;}
  gentity_t*helper=SpawnAt(p,87615608u,next,angles); // target_print
  if(!helper&&directional){
   // A player can stand where the reverse feedback would normally go.
   // Try a compact placement above the root before rolling back.
   VectorMA(point,role==0?48:-48,forward,next);
   next[2]+=64;
   helper=SpawnAt(p,87615608u,next,angles);
  }
  if(!helper){
   const int first=objects[root->s.number].output[JAR_DISTANCE_INSIDE];
   if(first)G_FreeEntity(&g_entities[first-1]);
   return qfalse;
  }
  jarObject_t*owned=&objects[helper->s.number];
  owned->order=objects[root->s.number].order;
  if(directional)helper->spawnflags=4; // Private target_print feedback.
  Q_strncpyz(owned->controllerText,directional?(role==0?"Forward crossing":"Reverse crossing"):
             (role==JAR_DISTANCE_INSIDE?"Distance trigger: inside range":"Distance trigger: outside range"),sizeof(owned->controllerText));
  objects[root->s.number].output[role]=helper->s.number+1;
 }
 return qtrue;
}
static gentity_t*SpawnWithController(gentity_t*p,unsigned id,const vec3_t point,const vec3_t angles){
 gentity_t*e=SpawnAt(p,id,point,angles);if(!e)return NULL;
 const int kind=ControllerKind(e->s.number);
 if(JAR_ControllerPortal(kind)){
  const qboolean surface=kind==JAR_PORTAL_SURFACE_CONTROLLER;
  vec3_t forward,helperPoint,helperAngles;
  AngleVectors(angles,forward,NULL,NULL);
  VectorCopy(angles,helperAngles);
  // Keep a spawned screen on the viewer's side, behind the camera's feed.
  if(!surface){VectorScale(forward,-1,forward);vectoangles(forward,helperAngles);}
  VectorMA(point,128,forward,helperPoint);
  trace_t clearance;
  trap->Trace(&clearance,point,NULL,NULL,helperPoint,e->s.number,MASK_SOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);
  if(clearance.startsolid||clearance.allsolid||clearance.fraction<.25f){G_FreeEntity(e);Tell(p,"No clear space for the portal and camera.");return NULL;}
  VectorMA(point,128*clearance.fraction-(clearance.fraction<1?16:0),forward,helperPoint);
  gentity_t* helper=SpawnAt(p,surface?JAR_MP_PORTAL_CAMERA_ID:JAR_MP_PORTAL_SURFACE_ID,helperPoint,helperAngles);
  if(!helper){G_FreeEntity(e);Tell(p,"No clear space for the portal and camera.");return NULL;}
  gentity_t* panel=surface?e:helper;
  const gentity_t* camera=surface?helper:e;
  objects[helper->s.number].order=objects[e->s.number].order;
  objects[panel->s.number].output[0]=camera->s.number+1;
  PortalSync(panel);
  return e;
 }
 if(kind==JAR_SEARCHLIGHT_CONTROLLER){
  vec3_t forward,right,aim,feedback;
  AngleVectors(angles,forward,right,NULL);
  VectorMA(point,128,forward,aim);VectorMA(point,64,right,feedback);feedback[2]+=16;
  gentity_t*target=SpawnAt(p,95740442u,aim,angles); // target_position
  gentity_t*notice=target?SpawnAt(p,87615608u,feedback,angles):NULL; // target_print
  if(!target||!notice){if(target)G_FreeEntity(target);G_FreeEntity(e);Tell(p,"No room for the searchlight and both markers.");return NULL;}
  notice->spawnflags=4;
  Q_strncpyz(objects[notice->s.number].controllerText,"Searchlight detected you",64);
  objects[target->s.number].order=objects[notice->s.number].order=objects[e->s.number].order;
  objects[e->s.number].output[0]=target->s.number+1;
  objects[e->s.number].output[1]=notice->s.number+1;
  return e;
 }
 if(kind==JAR_DISTANCE_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER){
  if(SpawnPairedFeedback(p,e,point,angles,kind==JAR_DIRECTIONAL_CONTROLLER))return e;
  G_FreeEntity(e);Tell(p,"No clear space for both feedback markers. Nothing spawned.");return NULL;
 }
 if(!ControllerOutput(e->s.number)||kind==10||kind==13||kind==JAR_BOMB_CONTROLLER||kind==JAR_SMALL_CARGO_CONTROLLER||kind==JAR_CRYSTAL_CRATE_CONTROLLER||kind==JAR_STATIC_BRUSH_CONTROLLER||JAR_ControllerSlide(kind)||JAR_ControllerStockExplosive(kind)||JAR_ControllerShooter(kind))return e;
 unsigned group=objects[e->s.number].order;qboolean spatial=kind==6||kind==7||kind==22||kind==23;vec3_t next,forward;AngleVectors(angles,forward,NULL,NULL);VectorMA(point,128,forward,next);if(spatial)next[2]+=96;
 unsigned feedback=0;for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].kind==(kind==16?4:5)&&!strcmp(jarMPCatalog[i].name,kind==16?"item_medpak_instant":spatial?((kind==7||kind==23)?"misc_teleporter_dest":"target_position"):"target_print")){feedback=jarMPCatalog[i].id;break;}
 gentity_t*h=feedback?SpawnAt(p,feedback,next,angles):NULL;if(!h){G_FreeEntity(e);Tell(p,"No clear space for the controller and its helper. Nothing spawned.");return NULL;}
 objects[h->s.number].order=group;objects[e->s.number].output[0]=h->s.number+1;return e;
}
static gentity_t *Spawn(gentity_t*p,unsigned id){
 int index;const jarMPEntry_t*item=Entry(id,&index);trace_t tr;vec3_t point,mins,maxs,angles={0,0,0};
 if(!item)return NULL;
 const int kind=item->kind==5?ControllerClass(item->name):0;
 if(kind==JAR_SEARCHLIGHT_CONTROLLER||JAR_ControllerPortal(kind)||JAR_ControllerAngular(kind))angles[YAW]=p->client->ps.viewangles[YAW];
 if(kind==JAR_PORTAL_SURFACE_CONTROLLER)angles[YAW]=AngleNormalize180(angles[YAW]+180);
 if(item->kind==2||item->kind==3||JAR_ControllerStart(kind)||JAR_ControllerEmplaced(kind)||JAR_ControllerShooter(kind)||JAR_ControllerAnimatedTag(kind)||kind==JAR_SMALL_CARGO_CONTROLLER||JAR_ControllerRack(kind)||kind==JAR_DOOR_CAMERA_CONTROLLER||kind==JAR_WELDER_CONTROLLER||kind==JAR_CRYSTAL_CRATE_CONTROLLER||kind==JAR_WALL_CONTROLLER||JAR_ControllerSlide(kind)||kind==JAR_BUTTON_CONTROLLER||kind==JAR_DIRECTIONAL_CONTROLLER)angles[YAW]=p->client->ps.viewangles[YAW];
 const jarMPSectionKind_t section=SectionKind(item);
 const int range=(section==JAR_MP_SECTION_HOTH||section==JAR_MP_SECTION_YAVIN)?SECTION_LARGE_AIM_RANGE:section?SECTION_AIM_RANGE:CatalogBody(item)?2048:512;
 AimWithFlags(p,&tr,range,item->kind==2||item->kind==3?JAR_TRACE_SPAWN_OVERLAP:0);
 if(tr.startsolid||tr.allsolid)return NULL;
 if(kind==JAR_PORTAL_SURFACE_CONTROLLER&&tr.fraction<1&&fabsf(tr.plane.normal[2])<.7f)
  vectoangles(tr.plane.normal,angles);
 if(SectionCard(item)){
  if(tr.fraction>=1){Tell(p,va("Aim at a clear surface within %d units.",range));return NULL;}
  jarMPSectionAdmission_t data;if(!SectionRead(p,SectionKind(item),&data))return NULL;
  angles[YAW]=p->client->ps.viewangles[YAW];
  SectionPlacement(&data,angles,&tr,point);
  gentity_t*section=SectionSpawn(p,index,point,angles,&data);
  if(section)Tell(p,va("%s placed. Activate enters. Undo removes the section.",item->name));
  return section;
 }
 if(CatalogBody(item)){
  if(!PhysicsPoseHull(index+1,0,angles,mins,maxs)){Tell(p,"Required model pose/collision bounds are unavailable.");return NULL;}
  // Support distance keeps offset-origin models flush with the aimed surface.
  float offset=4;for(int a=0;a<3;++a)offset-=tr.plane.normal[a]*(tr.plane.normal[a]>=0?mins[a]:maxs[a]);
  if(kind==JAR_PENDULUM_CONTROLLER)offset+=8;
  if(kind==JAR_SPINNER_CONTROLLER)offset+=10*hypotf(tr.plane.normal[0],tr.plane.normal[1]);
  VectorMA(tr.endpos,offset,tr.plane.normal,point);return SpawnWithController(p,id,point,angles);
 }
 VectorMA(tr.endpos,item->radius+4,tr.plane.normal,point);point[2]-=item->bottom;
 VectorSet(mins,-item->radius,-item->radius,item->bottom);VectorSet(maxs,item->radius,item->radius,item->bottom+item->height);
 trap->Trace(&tr,point,mins,maxs,point,p->s.number,MASK_PLAYERSOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);
 if(tr.startsolid||tr.allsolid){Tell(p,"Not enough clear space at the crosshair.");return NULL;}
 return SpawnWithController(p,id,point,angles);
}
static void Undo(gentity_t*p,qboolean all){
 unsigned latest=0;
 int mount=ENTITYNUM_NONE;
 qboolean attachment=qfalse;
 const int owner=p->s.number+1;
 // Creation order belongs to spawned entities and physical attachments.
 // Native drops can follow a group out of the world but cannot select it.
 for(int n=0;!all&&n<level.num_entities;++n){
  if(!g_entities[n].inuse)continue;
  if(n>=MAX_CLIENTS&&objects[n].owner==owner&&!objects[n].transientPickup){
   const unsigned creation=objects[n].order;
   if(creation>latest){
    latest=creation;mount=ENTITYNUM_NONE;
    attachment=g_entities[n].s.generic1==251?qtrue:qfalse;
   }else if(creation==latest&&g_entities[n].s.generic1!=251)attachment=qfalse;
  }
  if(thrustCreation[n].owner==owner&&thrustCreation[n].order>latest&&objects[n].thrustMounted){
   latest=thrustCreation[n].order;mount=n;
  }
 }
 if(mount!=ENTITYNUM_NONE){
  ForceRemoveThruster(mount);
  ++objects[mount].propertyRevision;SelectionChanged(mount);
  Tell(p,"Undo: last thruster removed.");trap->SendServerCommand(p->s.number,"jarundo");return;
 }
 if(!latest&&!all){Tell(p,"Undo: no spawned objects or Toolgun creations to remove.");return;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner==p->s.number+1&&(all||objects[n].order==latest)&&RemovalOccupied(&g_entities[n])){Tell(p,"Dismount or leave the supported section before Undo or cleanup. No objects removed.");return;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner==p->s.number+1&&(all||objects[n].order==latest))G_FreeEntity(&g_entities[n]);
 // A mount can belong to this builder even when its host belongs to someone else.
 if(all)for(int n=0;n<level.num_entities;++n){
  if(!g_entities[n].inuse||thrustCreation[n].owner!=owner||!thrustCreation[n].order||!objects[n].thrustMounted)continue;
  ForceRemoveThruster(n);
  ++objects[n].propertyRevision;SelectionChanged(n);
 }
 Tell(p,all?"Cleanup: spawned objects and Toolgun creations removed.":
  attachment?"Undo: last attachment removed.":"Undo: last spawned object or assembly removed.");
 if(!all)trap->SendServerCommand(p->s.number,"jarundo");
}
static void Anchor(int entity,const vec3_t world,vec3_t local){
 if(entity==ENTITYNUM_WORLD){VectorCopy(world,local);return;}
 vec3_t axis[3],delta;AnglesToAxis(g_entities[entity].r.currentAngles,axis);VectorSubtract(world,g_entities[entity].r.currentOrigin,delta);
 for(int a=0;a<3;++a)local[a]=DotProduct(delta,axis[a]);
}
static void Endpoint(int entity,const vec3_t local,vec3_t world){
 if(entity==ENTITYNUM_WORLD){VectorCopy(local,world);return;}
 vec3_t axis[3];AnglesToAxis(g_entities[entity].r.currentAngles,axis);VectorCopy(g_entities[entity].r.currentOrigin,world);
 for(int a=0;a<3;++a)VectorMA(world,local[a],axis[a],world);
}
#include "g_jar_duplicate.inc"

static qboolean CullFallenObject(gentity_t *entity, const vec3_t origin, float localTop){
 const int n=entity->s.number;
 if(!entity->inuse||!objects[n].owner||sectionParent[n]||entity->neverFree||entity->isSaberEntity||
    entity->s.generic1==251||!JAR_OutsideMap(origin,localTop,fallFloor)||RemovalOccupied(entity))return qfalse;
 G_FreeEntity(entity);
 return qtrue;
}

static void UpdateFallFloor(void){
 float bottom = mapFallBottom;
 for (int n = MAX_CLIENTS; n < level.num_entities; ++n) {
  const gentity_t *entity = &g_entities[n];
  if (SectionRoot(n) && entity->r.linked) {
   bottom = fminf(bottom, entity->r.absmin[2]);
  }
 }
 fallFloor = JAR_FallFloor(bottom);
}

static void CullFallenObjects(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t *entity=&g_entities[n];
  const float top=SectionRoot(n)&&entity->r.linked?entity->r.absmax[2]-entity->r.currentOrigin[2]:entity->r.maxs[2];
  CullFallenObject(entity,entity->r.currentOrigin,top);
 }
}

#include "g_jar_world.inc"
#include "g_jar_forces.inc"
#include "g_jar_physics.inc"
#include "g_jar_joints.inc"
#include "g_jar_context.inc"
static qboolean PhysgunMoveVehicle(gentity_t*,const vec3_t,const vec3_t,const vec3_t,float);
static qboolean ToolRotate(int n,const vec3_t angles,const vec3_t contact){
 gentity_t* e=&g_entities[n];
 jarMPGrip_t pivot;
 vec3_t origin;
 if(!JAR_GripBegin(&pivot,e->r.currentOrigin,e->r.currentAngles,contact,0))return qfalse;
 JAR_GripOrigin(&pivot,contact,angles,origin);
 if(!TeleportVector(origin,65535))return qfalse;
 if(SectionRoot(n))return SectionPose(e,origin,angles,qtrue);
 if(e->m_pVehicle){
  const float turn=fabsf(AngleSubtract(angles[YAW],e->r.currentAngles[YAW]))*(M_PI/180.f);
  if(!PhysgunMoveVehicle(e,origin,angles,contact,turn))return qfalse;
 }else if(!ActorRotateAt(e,origin,angles,contact))return qfalse;
 if(e->client)ActorSnapshot(e);else PhysicsPublish(e);
 return qtrue;
}
static void PoseProperty(gentity_t*p,const char*arg){
 jarPlayer_t*cl=&players[p->s.number];const int n=cl->selected;gentity_t*e=&g_entities[n];int frame=e->s.frame;
 if(!strcmp(arg,"pose_prev"))--frame;else if(!strcmp(arg,"pose_next"))++frame;else if(!strcmp(arg,"pose_reset"))frame=0;
 else if(!strncmp(arg,"pose_set_",9)){unsigned value;if(!JAR_MP_ParseId(arg+9,&value)||value<1||value>65536){Tell(p,"Pose frame must be 1..65536.");SelectionEcho(p);return;}frame=(int)value-1;}
 else if(!strcmp(arg,"pose_undo")){
  if(cl->poseUndoEntity!=n||cl->poseUndoIdentity!=PhysicsIdentity(n)||cl->poseUndoRevision!=objects[n].poseRevision){Tell(p,"Reverting pose requires an unchanged edit on this object.");SelectionEcho(p);return;}frame=cl->poseUndoFrame;
 }else return;
 int previous=e->s.frame;if(frame==previous){SelectionEcho(p);return;}
 if(!PhysicsSetPose(e,frame)){Tell(p,"Pose is unavailable or would intersect the world or an entity.");SelectionEcho(p);return;}
 if(!strcmp(arg,"pose_undo")){cl->poseUndoEntity=-1;Tell(p,"Previous model pose restored.");}
 else {cl->poseUndoEntity=n;cl->poseUndoFrame=previous;cl->poseUndoIdentity=PhysicsIdentity(n);cl->poseUndoRevision=objects[n].poseRevision;}
}
static void SettingsDefaults(jarPlayer_t*p){
 if(p->settingsInitialized)return;
 p->ropeSlack=1.1f;p->thrustForce=JAR_THRUSTER_DEFAULT;p->rotationStep=15;p->settingsInitialized=qtrue;
}
static void SettingsEcho(gentity_t*p){jarPlayer_t*c=&players[p->s.number];SettingsDefaults(c);trap->SendServerCommand(p->s.number,va("jarsettings %.9g %.9g %d %d %.9g",c->ropeSlack,c->thrustForce,c->linkOutput,c->thrustReverse,c->rotationStep));trap->SendServerCommand(p->s.number,va("jartool %d 0",c->tool));StatsEcho(p);}
static void ToolTarget(gentity_t*p,qboolean alt,const trace_t*target){
 jarPlayer_t*cl=&players[p->s.number];SettingsDefaults(cl);trace_t tr=*target;int n=tr.entityNum=SectionToolTarget(tr.entityNum);
 if(cl->tool==4||cl->tool==5||cl->tool==6||cl->tool==12){if(cl->firstTool!=cl->tool)cl->first=ENTITYNUM_NONE;cl->firstTool=cl->tool;}
 if(cl->tool==11){ControllerActivate(p,n,alt);return;}
 if(cl->tool==12){ControllerLink(p,n,alt);return;}
 if(cl->tool==3){if(alt||!copies[p->s.number].count)CopyAssembly(p,n);else PasteAssembly(p);return;}
 if(cl->tool==1&&alt){Undo(p,qfalse);StatsEcho(p);return;}
 if(cl->tool==7||cl->tool==8||cl->tool==10){
  if(!Manipulate(p,n)){Tell(p,"No editable entity selected.");return;}
  if(cl->tool==7||(cl->tool==10&&alt)){cl->selected=n;SelectionEcho(p);Tell(p,va("Selected %s (#%d).",g_entities[n].classname?g_entities[n].classname:"entity",n));return;}
  if(cl->tool==10){ForceDetach(n);SelectionChanged(n);Tell(p,"Attachments and thruster removed.");return;}
  if(JAR_MP_NativeEmplacedOccupied(&g_entities[n])){Tell(p,"Dismount the emplaced gun before rotating it.");return;}
  gentity_t*e=&g_entities[n];if(!e->client&&!PhysicsProp(n)&&!SectionRoot(n)){Tell(p,"This entity's multiplayer transform controller is not available yet.");return;}
  vec3_t angle;VectorCopy(e->r.currentAngles,angle);angle[YAW]=AngleNormalize360(angle[YAW]+(alt?-1:1)*fmodf(cl->rotationStep,360.f));
  if(objects[n].heldBy){Tell(p,"Release the Physgun before using the Rotate tool.");return;}
  if(tr.startsolid||tr.allsolid||tr.fraction>=1||!ToolRotate(n,angle,tr.endpos)){
   Tell(p,"Rotation is blocked by the world or another entity.");return;
  }
  ++objects[n].propertyRevision;SelectionChanged(n);return;
 }
 const int action=cl->tool==9?0:cl->tool;if(cl->tool==9)alt=!alt;
 if(SectionRoot(n)&&(action==0||action==2||action>=4)&&!(action>=4&&alt)){
  if(action==0&&Manipulate(p,n)){if(objects[n].heldBy)Drop(objects[n].heldBy-1,qtrue);Tell(p,"Section parked.");}
  else Tell(p,"Sections support Physgun, Rotate, placement and duplication.");
  return;
 }
 if(action>=4&&alt){cl->first=ENTITYNUM_NONE;return;}
 if((action>=4||(action==2&&!alt))&&(tr.startsolid||tr.allsolid||tr.fraction>=1||VectorLengthSquared(tr.plane.normal)<.5f)){Tell(p,"Aim at a solid surface for an attachment.");return;}
 if(!((action==0||action==2||action>=4)?Manipulate(p,n):Edit(p,n))&&(action<4||n!=ENTITYNUM_WORLD)){Tell(p,"Select an editable sandbox object. Shared editing is a server option.");return;}
 if(n>=MAX_CLIENTS&&n<ENTITYNUM_WORLD&&JAR_MP_NativeEmplacedOccupied(&g_entities[n])){Tell(p,"Dismount the emplaced gun before using this tool on it.");return;}
 if(((action==2||action==0)&&!g_entities[n].client)&&(!objects[n].catalog||!CatalogBody(&jarMPCatalog[objects[n].catalog-1]))){Tell(p,"This MP tool action currently applies to props.");return;}
 if(action==0){
  if(cl->tool==9&&alt)objects[n].noGravity=0;
  ++objects[n].propertyRevision;
  if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);
  if(g_entities[n].client){if(alt)EndActor(n,qfalse);else BeginActor(p,&g_entities[n]);}
  objects[n].frozen=alt?0:1;VectorClear(objects[n].velocity);VectorClear(objects[n].angular);
  if(g_entities[n].client&&!alt)JAR_MP_ActorThink(&g_entities[n],NULL);
 }
 else if(action==1){if(!alt){if(RemovalOccupied(&g_entities[n]))Tell(p,"Dismount or leave the supported section before removing this entity.");else G_FreeEntity(&g_entities[n]);}}
 else if(action==2){
  if(alt&&!objects[n].thrustMounted&&!objects[n].thruster)return;
  if(objects[n].heldBy)Drop(objects[n].heldBy-1,qfalse);
  const int on=alt?0:!objects[n].thruster;if(g_entities[n].client)EndActor(n,qfalse);
  jarObject_t*o=&objects[n];
  if(alt)ForceRemoveThruster(n);
  else if(!o->thrustMounted){
   vec3_t axis[3];Anchor(n,tr.endpos,o->thrustAnchor);AnglesToAxis(g_entities[n].r.currentAngles,axis);
   for(int a=0;a<3;++a)o->thrustAxis[a]=DotProduct(tr.plane.normal,axis[a]);
   VectorNormalize(o->thrustAxis);
   if(cl->thrustReverse)VectorScale(o->thrustAxis,-1.f,o->thrustAxis);
   o->thrustMounted=1;o->thrustForce=cl->thrustForce;
   thrustCreation[n].order=++order;thrustCreation[n].owner=p->s.number+1;
  }
  objects[n].thruster=on;objects[n].thrustBy=(on||o->thrustMounted)?p->s.number+1:0;objects[n].frozen=0;
  ++o->propertyRevision;
 }
 else if(action>=5){JointTool(p,&tr);}
 else if(action==4){
  if(cl->first==ENTITYNUM_NONE){cl->first=n;Anchor(n,tr.endpos,cl->firstAnchor);VectorCopy(tr.plane.normal,cl->firstRotation);cl->firstRotation[3]=1;Tell(p,"First rope anchor set; select a second surface.");return;}
  if(cl->first==n||!Room())return;
  int ropes=0;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(objects[i].owner&&g_entities[i].s.generic1==251)++ropes;
  if(ropes>=JAR_MP_ATTACHMENT_LIMIT){Tell(p,"Rope capacity reached.");return;}
  if(cl->first!=ENTITYNUM_WORLD&&!Manipulate(p,cl->first)){cl->first=ENTITYNUM_NONE;return;}
  gentity_t*r=G_TrySpawn();if(!r)return;Track(r,p->s.number,-1);jarObject_t*o=&objects[r->s.number];o->first=cl->first+1;o->second=n+1;
  VectorCopy(cl->firstAnchor,o->anchorA);Anchor(n,tr.endpos,o->anchorB);vec3_t start;Endpoint(cl->first,o->anchorA,start);o->length=(float)fmin(131072,fmax(.01,(double)Distance(start,tr.endpos)*cl->ropeSlack));
  if(cl->first==ENTITYNUM_WORLD)memcpy(o->jointA,cl->firstRotation,sizeof(o->jointA));if(n==ENTITYNUM_WORLD){VectorCopy(tr.plane.normal,o->jointB);o->jointB[3]=1;}
  float routeLength;if(!RopeAdmission(r->s.number,&routeLength)){G_FreeEntity(r);Tell(p,"Rope could not find a clear route around scenery. Move its anchors and try again.");return;}
  o->length=(float)fmin(131072,fmax(o->length,(double)routeLength*cl->ropeSlack));
  r->classname="jar_rope";r->s.jarRopeState=1;r->s.eType=ET_GENERAL;r->s.generic1=251;r->r.svFlags|=SVF_BROADCAST;r->s.time2=(int)o->length;Move(r,start);VectorCopy(tr.endpos,r->s.origin2);cl->first=ENTITYNUM_NONE;
 }
}
// Clip an oriented stock part to the ray segment already bounded by solid
// scenery. Shelf markers and unrelated item entities never enter this query.
static qboolean AimPart(const gentity_t*part,const vec3_t mins,const vec3_t maxs,
                        const vec3_t from,const vec3_t delta,float limit,float *fraction,vec3_t normal){
 vec3_t axis[3],offset;
 AnglesToAxis(part->r.currentAngles,axis);
 VectorSubtract(from,part->r.currentOrigin,offset);
 float enter=0,leave=limit;
 int entryAxis=-1;
 float entrySign=0;
 for(int a=0;a<3;++a){
  const float start=DotProduct(offset,axis[a]),direction=DotProduct(delta,axis[a]);
  if(fabsf(direction)<.0001f){
   if(start<mins[a]||start>maxs[a])return qfalse;
   continue;
  }
  const float near=(mins[a]-start)/direction,far=(maxs[a]-start)/direction;
  const float first=fminf(near,far);
  if(first>enter){enter=first;entryAxis=a;entrySign=direction>0?-1:1;}
  leave=fminf(leave,fmaxf(near,far));
  if(enter>leave)return qfalse;
 }
 if(enter>=limit)return qfalse;
 *fraction=enter;
 VectorClear(normal);
 if(entryAxis>=0)VectorScale(axis[entryAxis],entrySign,normal);
 return qtrue;
}
static qboolean NonsolidAimPart(int n,const vec3_t from,const vec3_t delta,float limit,
                              float *fraction,vec3_t normal){
 const gentity_t*part=&g_entities[n];
 vec3_t mins,maxs;
 const int kind=ControllerKind(n);
 if(kind==JAR_WALL_CONTROLLER&&BodyDormant(n))WallMarkerHull(mins,maxs);
 else if(JAR_MP_PortalSurface(&part->s))
  JAR_MP_PortalHull(part->genericValue1,part->genericValue2,vec3_origin,mins,maxs);
 else if(kind==JAR_PORTAL_CAMERA_CONTROLLER){
  if(!PhysicsModelBounds(objects[n].catalog,0,mins,maxs))return qfalse;
 }
 else if(JAR_MP_WelderRootState(&part->s)||JAR_ControllerPointGizmo(kind)){
  if(!PhysicsModelBounds(objects[n].catalog,0,mins,maxs))return qfalse;
 }else if(JAR_MP_CrystalCrateRootState(&part->s))PhysicsCrystalCrateBounds(mins,maxs);
 else if(!DoorCameraPickBounds(n,mins,maxs))return qfalse;
 return AimPart(part,mins,maxs,from,delta,limit,fraction,normal);
}
static void NonsolidToolAim(gentity_t*p,const vec3_t from,trace_t*tr){
 if(tr->startsolid||tr->allsolid)return;
 vec3_t delta,normal;
 VectorSubtract(tr->endpos,from,delta);
 float best=1;
 int root=ENTITYNUM_NONE;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const gentity_t*e=&g_entities[n];
  const qboolean shotOnly=JAR_MP_CrystalCrateRootState(&e->s)&&e->r.contents==CONTENTS_CORPSE;
  const int kind=ControllerKind(n);
  const int parent=e->inuse&&(JAR_MP_WelderRootState(&e->s)||shotOnly||JAR_ControllerPointGizmo(kind)||JAR_ControllerPortal(kind)||(kind==JAR_WALL_CONTROLLER&&BodyDormant(n)))?n:DoorCameraRoot(n);
  if(parent==ENTITYNUM_NONE||!Edit(p,parent)||objects[parent].copyPending)continue;
  float fraction;
  vec3_t partNormal;
  if(NonsolidAimPart(n,from,delta,best,&fraction,partNormal)){
   best=fraction;root=parent;VectorCopy(partNormal,normal);
  }
 }
 if(root==ENTITYNUM_NONE)return;
 tr->entityNum=root;
 tr->contents=CONTENTS_ITEM;
 tr->fraction*=best;
 VectorMA(from,best,delta,tr->endpos);
 VectorCopy(normal,tr->plane.normal);
}
static void PhysgunAim(gentity_t*p,trace_t*tr){
 vec3_t from,forward,end;
 VectorCopy(p->client->ps.origin,from);from[2]+=p->client->ps.viewheight;
 AngleVectors(p->client->ps.viewangles,forward,NULL,NULL);
 VectorMA(from,JAR_TOOL_AIM_RANGE,forward,end);
 trap->Trace(tr,from,NULL,NULL,end,p->s.number,JAR_PHYSGUN_MP_AIM_MASK,qfalse,JAR_TRACE_PHYS_GUN,0);
 if(!tr->startsolid&&!tr->allsolid){
  vec3_t delta,low,high;
  VectorSubtract(tr->endpos,from,delta);
  for(int a=0;a<3;++a){low[a]=fminf(from[a],tr->endpos[a])-1;high[a]=fmaxf(from[a],tr->endpos[a])+1;}
  int hits[MAX_GENTITIES];
  const int count=trap->EntitiesInBox(low,high,hits,MAX_GENTITIES);
  float best=1;
  for(int i=0;i<count;++i){
   const int n=hits[i];const gentity_t*e=&g_entities[n];
   if(n==tr->entityNum)continue;
   if(!e->inuse||!objects[n].catalog||!(e->r.contents&JAR_PHYSGUN_MP_AIM_MASK)||!JAR_MP_PhysgunBox(&e->s,e->classname))continue;
   vec3_t mins,maxs,normal;float hit;
   PhysicsLocalBounds(n,mins,maxs);
   for(int a=0;a<3;++a){
    const float center=(mins[a]+maxs[a])*.5f,half=fmaxf(.25f,(maxs[a]-mins[a])*.5f);
    mins[a]=center-half;maxs[a]=center+half;
   }
   if(!AimPart(e,mins,maxs,from,delta,best,&hit,normal))continue;
   if(trap->JAR_PickPropModel&&e->model&&!JAR_MP_PortalSurface(&e->s)){
    float surface=best;
    if(trap->JAR_PickPropModel(e->model,e->s.frame,e->r.currentOrigin,e->r.currentAngles,from,tr->endpos,&surface,normal)){
     if(surface>=best)continue;
     hit=surface;
    }
   }
   best=hit;tr->entityNum=n;tr->contents=e->r.contents;VectorCopy(normal,tr->plane.normal);
  }
  if(best<1){tr->fraction*=best;VectorMA(from,best,delta,tr->endpos);}
 }
 NonsolidToolAim(p,from,tr);
}
static void Tool(gentity_t*p,qboolean alt){trace_t tr;PhysgunAim(p,&tr);Beam(p,tr.endpos,qtrue);G_Sound(p,CHAN_WEAPON,G_SoundIndex(JAR_TOOLGUN_SOUND));ToolTarget(p,alt,&tr);}
static qboolean ContextTool(gentity_t*p,const char*action){
 jarPlayer_t*cl=&players[p->s.number];int n=cl->selected,mode=-1;
 if(!strcmp(action,"refresh")){SelectionEcho(p);return qtrue;}
 if(n==p->s.number){if(!strcmp(action,"release")||!strcmp(action,"detach")){VehicleRiderRelease(p);JAR_MP_NativeEmplacedDetachUser(p);EndActor(n,qfalse);ForceDetach(n);cl->noGrabUntil=level.time+1000;SelectionEcho(p);}else Tell(p,"Use the player fields to edit your properties.");return qtrue;}
 if(!strcmp(action,"copy")){CopyAssembly(p,n);return qtrue;}
 if(!strcmp(action,"paste")){PasteAssembly(p);SelectionEcho(p);return qtrue;}
 if(!strcmp(action,"activate")){ControllerActivate(p,n,qfalse);SelectionChanged(n);return qtrue;}
 if(!strcmp(action,"rope"))mode=4;else if(!strcmp(action,"thruster"))mode=2;else if(!strcmp(action,"weld"))mode=5;else if(!strcmp(action,"hinge"))mode=6;
 if(mode<0)return qfalse;
 if(JAR_MP_BeaconState(&g_entities[n].s)&&BodyDormant(n)){Tell(p,"Deploy the beacon before attaching anything.");return qtrue;}
 trace_t tr={0};tr.entityNum=n;tr.fraction=.5f;tr.plane.normal[2]=1;VectorCopy(g_entities[n].r.currentOrigin,tr.endpos);
 int old=cl->tool;cl->tool=mode;ToolTarget(p,qfalse,&tr);cl->tool=old;SelectionChanged(n);return qtrue;
}
// A hidden beacon has no engine collider. Pick only an editable beacon
// intersecting the already checked segment in front of the first obstruction.
static int ContextHiddenBeacon(gentity_t*p,const vec3_t from,const vec3_t end){
 vec3_t delta;VectorSubtract(end,from,delta);
 int result=ENTITYNUM_NONE;
 float best=1.0f;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  const gentity_t*e=&g_entities[n];
  if(!e->inuse||!JAR_MP_BeaconState(&e->s)||!BodyDormant(n)||!Edit(p,n)||objects[n].copyPending)continue;
  float enter=0,leave=best;
  for(int a=0;a<3&&enter<=leave;++a){
   const float lo=e->r.currentOrigin[a]+e->r.mins[a],hi=e->r.currentOrigin[a]+e->r.maxs[a];
   if(fabsf(delta[a])<.0001f){if(from[a]<lo||from[a]>hi)leave=-1;continue;}
   const float first=(lo-from[a])/delta[a],last=(hi-from[a])/delta[a];
   enter=fmaxf(enter,fminf(first,last));leave=fminf(leave,fmaxf(first,last));
  }
  if(enter<=leave&&enter<best){best=enter;result=n;}
 }
 return result;
}
static void ContextSelect(gentity_t*p,const char*arg){
 jarPlayer_t*cl=&players[p->s.number];int n=ENTITYNUM_NONE;
 if(!strcmp(arg,"self"))n=p->s.number;
 else {trace_t tr;vec3_t from;
  if(!strncmp(arg,"ray_",4)){
   vec3_t direction,eye,end;const char*at=arg+4;
   for(int i=0;i<6;++i){char*next;float v=strtof(at,&next);if(next==at||!isfinite(v)||fabsf(v)>(i<3?65535:1.001f)||(i<5?*next!='_':*next!=0)){Tell(p,"Invalid selection ray.");return;}(i<3?from:direction)[i%3]=v;at=next+1;}
   VectorCopy(p->client->ps.origin,eye);eye[2]+=p->client->ps.viewheight;
   if(DistanceSquared(eye,from)>512*512||fabsf(VectorLengthSquared(direction)-1)>.01f){Tell(p,"Selection camera is out of range.");return;}
   trap->Trace(&tr,eye,NULL,NULL,from,p->s.number,MASK_SOLID,qfalse,0,0);
   if(tr.startsolid||tr.allsolid||tr.fraction<1){Tell(p,"Selection camera is obstructed.");return;}
   VectorMA(from,4096,direction,end);trap->Trace(&tr,from,NULL,NULL,end,p->s.number,MASK_PLAYERSOLID,qfalse,0,0);
  }else if(!*arg){VectorCopy(p->client->ps.origin,from);from[2]+=p->client->ps.viewheight;Aim(p,&tr,2048);}else return;
  if(!tr.startsolid&&!tr.allsolid){
   NonsolidToolAim(p,from,&tr);
   n=SectionToolTarget(tr.entityNum);
   if(n>=MAX_CLIENTS&&!ContextEditable(p,n))n=ContextHiddenBeacon(p,from,tr.endpos);
  }
 }
 cl->contextOpen=1;
 cl->selected=ContextEditable(p,n)?n:ENTITYNUM_NONE;
 const char*message="No editable object selected.";
 if(cl->selected!=ENTITYNUM_NONE)
  message=n==p->s.number?"Editing your player.":n<MAX_CLIENTS?"Player selected.":"Object selected. Context actions apply to this object.";
 else if(n>=0&&n<MAX_CLIENTS&&n!=p->s.number&&ActorAlive(&g_entities[n])){
  if(!playerTools.integer)message="The server disabled tools on other players.";
  else if(players[n].noTools)message="That player blocked tools on themselves.";
  else if(level.time<players[n].noGrabUntil)message="That player just released themselves. Try again shortly.";
 }
 Tell(p,message);SelectionEcho(p);
}
#include "g_jar_weapons.inc"
qboolean JAR_MP_Command(gentity_t*p,const char*command){
 if(!JAR_MP_IsCommand(command))return qfalse;
 if(!p||p->s.number<0||p->s.number>=MAX_CLIENTS)return qtrue;
 jarPlayer_t*cl=&players[p->s.number];
 if(!strcmp(command,"jar_setting")){char value[32];trap->Argv(1,value,sizeof(value));if(!strcmp(value,"show")){SettingsEcho(p);return qtrue;}}
 if(!strcmp(command,"jar_select")){char mode[16];trap->Argv(1,mode,sizeof(mode));if(!strcmp(mode,"close")){cl->contextOpen=0;cl->selected=ENTITYNUM_NONE;return qtrue;}}
 // A player can always revoke control of their own body, even while frozen.
 if(!strcmp(command,"jar_release")){VehicleRiderRelease(p);EndActor(p->s.number,qfalse);ForceDetach(p->s.number);cl->noGrabUntil=level.time+1000;return qtrue;}
 if(!strcmp(command,"jar_allowTools")){char value[8];trap->Argv(1,value,sizeof(value));
  if(!strcmp(value,"0")||!strcmp(value,"1")){cl->noTools=value[0]=='0';if(cl->noTools){VehicleRiderRelease(p);EndActor(p->s.number,qfalse);ForceDetach(p->s.number);}Tell(p,cl->noTools?"Player tools disabled for you.":"Player tools allowed by you (subject to server policy).");}return qtrue;
 }
 if(!JAR_MP_RequestReady(level.time,&cl->nextRequest))return qtrue;
 char arg[MAX_STRING_CHARS];unsigned id;trap->Argv(1,arg,sizeof(arg));
 // Seated players may select and activate controllers through C without
 // dismounting. Other Sandbox editing/equipment commands retain their guards.
 gentity_t*vehicle=TransportMover(p);
 qboolean riderContext=vehicle&&vehicle!=p&&vehicle->m_pVehicle&&ActorAlive(vehicle)&&!JAR_MP_ActorControlled(vehicle)&&
  VehicleSeatValid(vehicle,(bgEntity_t*)p)&&(!Q_stricmp(command,"jar_select")||
   (!Q_stricmp(command,"jar_property")&&(!strcmp(arg,"refresh")||(!strcmp(arg,"activate")&&ControllerKind(cl->selected)))));
 // A Sandbox hold blocks tools on the world, but the held player can still
 // use the identity-checked properties and release actions on their own body.
 const qboolean selfContext=ContextEditable(p,p->s.number)&&
  ((!strcmp(command,"jar_select")&&!strcmp(arg,"self"))||
   (!strcmp(command,"jar_property")&&cl->selected==p->s.number));
 if(!sandbox.integer||(!selfContext&&((!Alive(p)&&!riderContext)||JAR_MP_ActorControlled(p)))){Tell(p,"Sandbox is disabled or your player is inactive/held. Use jar_release to release yourself.");return qtrue;}
 if(!Q_stricmp(command,"jar_spawn")){if(JAR_MP_ParseId(arg,&id))Spawn(p,id);}
 else if(!Q_stricmp(command,"jar_equip")){if(JAR_MP_ParseId(arg,&id))EquipWeapon(p,id);}
 else if(!Q_stricmp(command,"jar_setting")){
  SettingsDefaults(cl);
  if(!strcmp(arg,"slack_less")){if(cl->ropeSlack>.1f)cl->ropeSlack-=.1f;}
  else if(!strcmp(arg,"slack_more"))cl->ropeSlack+=.1f;
  else if(!strcmp(arg,"slack_reset"))cl->ropeSlack=1.1f;
  else if(!strcmp(arg,"force_less"))cl->thrustForce-=JAR_THRUSTER_STEP;
  else if(!strcmp(arg,"force_more"))cl->thrustForce+=JAR_THRUSTER_STEP;
  else if(!strcmp(arg,"force_reset"))cl->thrustForce=JAR_THRUSTER_DEFAULT;
  else if(!strncmp(arg,"slack_",6)||!strncmp(arg,"force_",6)||!strncmp(arg,"rotation_",9)){
   float value;
   const qboolean slack=arg[0]=='s',rotation=arg[0]=='r';
   if(!JAR_ParseFloat(arg+(rotation?9:6),&value)||(slack&&value<=0)){
    Tell(p,slack?"Rope slack needs a positive number.":"Enter a valid number.");return qtrue;
   }
   if(slack)cl->ropeSlack=value;else if(rotation)cl->rotationStep=value;else cl->thrustForce=value;
  }
  else if(!strcmp(arg,"thrust_in"))cl->thrustReverse=qfalse;else if(!strcmp(arg,"thrust_out"))cl->thrustReverse=qtrue;
  if(!strcmp(arg,"output_primary"))cl->linkOutput=JAR_DISTANCE_INSIDE;else if(!strcmp(arg,"output_secondary"))cl->linkOutput=JAR_DISTANCE_OUTSIDE;
  else if(!strcmp(arg,"output_reference"))cl->linkOutput=JAR_DISTANCE_REFERENCE;
  else if(!strcmp(arg,"output_subject"))cl->linkOutput=JAR_DISTANCE_SUBJECT;
  SettingsEcho(p);
 }
 else if(!Q_stricmp(command,"jar_tool")){if(JAR_MP_ParseId(arg,&id)&&id<13){
  for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(!strcmp(jarMPCatalog[i].name,"weapon_toolgun")){
   if(EquipWeapon(p,jarMPCatalog[i].id)){if(cl->tool!=(int)id)cl->first=ENTITYNUM_NONE;cl->tool=(int)id;trap->SendServerCommand(p->s.number,va("jartool %d 1",cl->tool));}break;
  }
 }}
 else if(!Q_stricmp(command,"jar_undo"))Undo(p,qfalse);
 else if(!Q_stricmp(command,"jar_cleanup"))Undo(p,qtrue);
 else if(!Q_stricmp(command,"jar_distance")){
  const float maximum=SectionRoot(cl->held)?SECTION_AIM_RANGE:JAR_PHYSGUN_MAX_DISTANCE;
  if(!strcmp(arg,"in"))cl->distance=JAR_PhysgunWheelDistance(cl->distance,maximum,-1);
  if(!strcmp(arg,"out"))cl->distance=JAR_PhysgunWheelDistance(cl->distance,maximum,1);
 }
 else if(!Q_stricmp(command,"jar_select"))ContextSelect(p,arg);
 else if(!Q_stricmp(command,"jar_property")){
  if(!ContextRequest(p))return qtrue;
  if(ContextScalar(p,arg)||ContextTool(p,arg))return qtrue;
  int n=cl->selected;if(!ContextEditable(p,n)){Tell(p,"Select an editable object first.");SelectionEcho(p);return qtrue;}jarObject_t*o=&objects[n];gentity_t*e=&g_entities[n];
  if(!strcmp(arg,"detach")){ForceDetach(n);Tell(p,"Attachments and thruster removed from the selected entity.");SelectionChanged(n);return qtrue;}
  if(!strcmp(arg,"remove")){if(ContextCaps(p,n)&JAR_CONTEXT_REMOVE)G_FreeEntity(e);else Tell(p,"This entity cannot be removed with your current permissions.");return qtrue;}
  if(e->client){
   if(!strcmp(arg,"freeze")||!strcmp(arg,"release"))++o->propertyRevision;
   if(!strcmp(arg,"freeze")){if(o->heldBy)Drop(o->heldBy-1,qfalse);BeginActor(p,e);o->frozen=1;VectorClear(o->velocity);JAR_MP_ActorThink(e,NULL);}
   else if(!strcmp(arg,"release")){if(o->heldBy)Drop(o->heldBy-1,qfalse);EndActor(n,qfalse);}
   else if(!strcmp(arg,"rotate")){if(!JAR_MP_ActorControlled(e)){Tell(p,"Hold or freeze the actor before rotating it.");return qtrue;}vec3_t angle;VectorCopy(e->r.currentAngles,angle);angle[YAW]=AngleNormalize360(angle[YAW]+15);if(!ActorRotate(e,angle))Tell(p,"Rotation is blocked by the world or another entity.");else ++o->propertyRevision;ActorSnapshot(e);}
   else Tell(p,"Actors and vehicles with consenting riders support Freeze, Release and rotation while held/frozen. Mass and gravity apply to props.");
   SelectionChanged(n);return qtrue;
  }
  if(!o->catalog||!CatalogBody(&jarMPCatalog[o->catalog-1])){Tell(p,"MP context properties currently apply to props and actors.");return qtrue;}
  if(JAR_MP_NativeEmplacedOccupied(e)){Tell(p,"Dismount the emplaced gun before moving it.");return qtrue;}
  if(!strncmp(arg,"pose_",5)){PoseProperty(p,arg);return qtrue;}
  if(!strcmp(arg,"freeze")||!strcmp(arg,"release"))++o->propertyRevision;
  if(!strcmp(arg,"freeze")){if(o->heldBy)Drop(o->heldBy-1,qfalse);o->frozen=1;VectorClear(o->velocity);VectorClear(o->angular);}
  else if(!strcmp(arg,"release"))o->frozen=0;
  else if(!strcmp(arg,"rotate")){vec3_t angles;VectorCopy(e->r.currentAngles,angles);angles[YAW]=AngleNormalize360(angles[YAW]+15);if(!PhysicsSetAngles(e,angles))Tell(p,"Rotation is blocked by the world or another entity.");else ++o->propertyRevision;}
  SelectionChanged(n);
 }
 StatsEcho(p);
 return qtrue;
}
void JAR_MP_ReceivePhysgunInput(gentity_t* p,usercmd_t* cmd,qboolean allow){
 if(p->s.number<0||p->s.number>=MAX_CLIENTS||!p->client)return;
 jarPlayer_t* cl=&players[p->s.number];
 if(!(cmd->buttons&BUTTON_JAR_PHYSGUN_TURN)){
  cl->turnInput=qfalse;
  if(!allow||!(cmd->buttons&BUTTON_ATTACK)||(cmd->buttons&BUTTON_ALT_ATTACK))cl->turnYaw=cl->turnPitch=0;
  return;
 }
 const int packed=cmd->angles[ROLL];
 JAR_RestorePhysgunCommandView(&p->client->ps,cmd);
 const int held=cl->held;
 cl->turnInput=allow&&sandbox.integer&&Alive(p)&&
  p->client->ps.weapon==WP_PHYSGUN&&!p->client->ps.m_iVehicleNum&&
  (cmd->buttons&BUTTON_ATTACK)&&!(cmd->buttons&BUTTON_ALT_ATTACK)&&
  held>=0&&held<ENTITYNUM_WORLD&&p->client->ps.stats[STAT_JAR_PHYSGUN_HOLD]==held+1&&
  objects[held].heldBy==p->s.number+1&&Manipulate(p,held)?qtrue:qfalse;
 if(!cl->turnInput){cl->turnYaw=cl->turnPitch=0;return;}
 cmd->buttons&=~BUTTON_USE;
 cl->turnYaw=Com_Clamp(-180,180,cl->turnYaw+JAR_UnpackPhysgunAxis(packed));
 cl->turnPitch=Com_Clamp(-180,180,cl->turnPitch+JAR_UnpackPhysgunAxis(packed>>8));
}


void JAR_MP_Fire(gentity_t*p,qboolean alt){
 if(!sandbox.integer||!Alive(p)||JAR_MP_ActorControlled(p))return;
 jarPlayer_t*cl=&players[p->s.number];if(p->client->ps.weapon==WP_TOOLGUN){Tool(p,alt);return;}
 if(alt){
  if(cl->freezeLatch)return;
  cl->freezeLatch=1;
  if(cl->held!=ENTITYNUM_NONE)Drop(p->s.number,qtrue);
  else {trace_t tr;PhysgunAim(p,&tr);int n=SectionToolTarget(tr.entityNum);
   if(!tr.startsolid&&!tr.allsolid&&Manipulate(p,n)&&!objects[n].heldBy&&!SectionRoot(n)&&!JAR_MP_NativeEmplacedOccupied(&g_entities[n])){
    ++objects[n].propertyRevision;
    if(g_entities[n].client)EndActor(n,qfalse);
    objects[n].frozen=0;objects[n].noGravity=0;JAR_RigidRelease(n,objects[n].mass,1);
   }
  }return;
 }
 if(cl->freezeLatch)return;
 if(cl->held!=ENTITYNUM_NONE)return;
 trace_t tr;PhysgunAim(p,&tr);tr.entityNum=SectionToolTarget(tr.entityNum);if(tr.allsolid||tr.startsolid||!Manipulate(p,tr.entityNum)||objects[tr.entityNum].heldBy||JAR_MP_NativeEmplacedOccupied(&g_entities[tr.entityNum]))return;
 gentity_t *held=&g_entities[tr.entityNum];
 const float *view=p->client->ps.viewangles;
 if(!JAR_GripBegin(&cl->grip,held->r.currentOrigin,held->r.currentAngles,tr.endpos,view[YAW]))return;
 if(held->m_pVehicle&&!PhysgunMoveVehicle(held,held->r.currentOrigin,held->r.currentAngles,NULL,0))return;
 if(held->client)BeginActor(p,held);
 ++objects[tr.entityNum].propertyRevision;
 cl->held=tr.entityNum;
 vec3_t eye;VectorCopy(p->client->ps.origin,eye);eye[2]+=p->client->ps.viewheight;
 cl->distance=fmaxf(.01f,Distance(eye,tr.endpos));
 objects[cl->held].heldBy=p->s.number+1;objects[cl->held].frozen=0;
 p->client->ps.stats[STAT_JAR_PHYSGUN_HOLD]=cl->held+1;
 if(held->client)JAR_MP_ActorThink(held,NULL);
}

static void PhysgunLimitTarget(const vec3_t from,float limit,vec3_t target){
 vec3_t direction;VectorSubtract(target,from,direction);
 if(VectorNormalize(direction)>limit)VectorMA(from,limit,direction,target);
}

static qboolean PhysgunMoveVehicle(gentity_t*e,const vec3_t origin,const vec3_t angles,const vec3_t pivot,float turn){
 vec3_t mins,maxs;
 jarVehiclePoseMember_t members[VEH_MAX_PASSENGERS+3];int count;
 if(!TeleportVector(angles,3600)||!VehicleBounds(e,angles,mins,maxs)||
    !VehiclePosePlan(e,origin,angles,mins,maxs,members,&count))return qfalse;
 for(int i=0;i<count;++i)if(members[i].linked)trap->UnlinkEntity((sharedEntity_t*)members[i].entity);
 qboolean clear=qtrue;
 const float sagitta=1-cosf(turn*.5f);
 const qboolean yawOnly=angles[PITCH]==e->r.currentAngles[PITCH]&&angles[ROLL]==e->r.currentAngles[ROLL];
 for(int i=0;i<count&&clear;++i){
  const jarVehiclePoseMember_t*m=&members[i];const gentity_t*member=m->entity;
  vec3_t lo,hi;trace_t hit;
  // Bound the curved path around the grab point during this rotation step.
  const float pad=pivot?sagitta*fmaxf(Distance(pivot,member->r.currentOrigin),Distance(pivot,m->origin)):0;
  for(int a=0;a<3;++a){
   const float margin=a==2&&yawOnly?0:pad;
   lo[a]=fminf(member->r.mins[a],m->mins[a])-margin;
   hi[a]=fmaxf(member->r.maxs[a],m->maxs[a])+margin;
  }
  trap->Trace(&hit,member->r.currentOrigin,lo,hi,m->origin,member->s.number,PhysicsMoveMask(member),qfalse,0,0);
  clear=!hit.startsolid&&!hit.allsolid&&hit.fraction==1;
 }
 if(clear){
  VectorCopy(mins,e->r.mins);VectorCopy(maxs,e->r.maxs);
  VectorCopy(angles,e->r.currentAngles);VectorCopy(angles,e->s.apos.trBase);
  e->s.apos.trType=TR_INTERPOLATE;
  SetClientViewAngle(e,e->r.currentAngles);
  for(int i=0;i<count;++i){
   gentity_t*member=members[i].entity;
   VectorCopy(members[i].origin,member->client->ps.origin);
   VehicleRiderPose(member);
  }
  VehicleOrientation(e);
 }
 for(int i=0;i<count;++i)if(members[i].linked)trap->LinkEntity((sharedEntity_t*)members[i].entity);
 return clear;
}

// Sample actual on-foot actor displacement across input packets. A server
// frame without a new aim command can otherwise erase a guest's entire throw.
static void PhysgunActorVelocity(jarGripMotion_t *m,const vec3_t before,const vec3_t after,
                                 qboolean obstructed,vec3_t velocity){
 if(obstructed){
  m->count=0;
  VectorClear(velocity);
  return;
 }
 if(!m->count){
  m->first=0;
  m->count=1;
  m->samples[0].time=level.previousTime;
  VectorCopy(before,m->samples[0].origin);
  m->lastMoveTime=level.time-JAR_GRIP_EXPIRY_MS;
 }
 const int cutoff=level.time-JAR_GRIP_EXPIRY_MS;
 while(m->count>1){
  const int next=(m->first+1)%JAR_GRIP_SAMPLES;
  if(m->samples[next].time>cutoff)break;
  m->first=next;
  --m->count;
 }
 const jarGripSample_t *old=&m->samples[m->first];
 if(DistanceSquared(before,after)>=.000001f){
  // Measure the same span on a smooth host and on a guest whose latest
  // packet covers several frames. A newly acquired grip starts at rest.
  vec3_t origin;VectorCopy(old->origin,origin);
  if(old->time<cutoff){
   const jarGripSample_t *next=&m->samples[(m->first+1)%JAR_GRIP_SAMPLES];
   const float *end=m->count>1?next->origin:after;
   const int endTime=m->count>1?next->time:level.time;
   vec3_t delta;VectorSubtract(end,origin,delta);
   VectorMA(origin,(float)(cutoff-old->time)/(endTime-old->time),delta,origin);
  }
  VectorSubtract(after,origin,velocity);
  VectorScale(velocity,1000.f/JAR_GRIP_EXPIRY_MS,velocity);
  m->lastMoveTime=level.time;
 }else if(m->lastMoveTime<=cutoff){
  VectorClear(velocity);
 }
 const int last=(m->first+m->count-1)%JAR_GRIP_SAMPLES;
 if(level.time-m->samples[last].time>=JAR_GRIP_SAMPLE_MS){
  jarGripSample_t *sample=&m->samples[(last+1)%JAR_GRIP_SAMPLES];
  sample->time=level.time;
  VectorCopy(after,sample->origin);
  ++m->count;
 }
}

static void PhysgunMoveNative(gentity_t*e,const jarMPGrip_t*grip,const vec3_t point,const vec3_t facing,float dt){
 const int n=e->s.number;
 qboolean vehicleMoved=qfalse;
 qboolean obstructed=qfalse;
 vec3_t before,contact,origin,spin;VectorCopy(e->r.currentOrigin,before);
 JAR_GripPoint(grip,before,e->r.currentAngles,contact);
 JAR_AngularVelocity(e->r.currentAngles,facing,1,spin);
 const float angle=VectorLength(spin);
 if(angle>.0001f&&dt>0){
  vec3_t next;
  const float rate=fminf(8,1024/fmaxf(1,Distance(before,contact)));
  if(angle<=rate*dt)VectorCopy(facing,next);
  else JAR_AdvanceAngles(e->r.currentAngles,spin,rate*dt/angle,next);
  JAR_GripOrigin(grip,contact,next,origin);
  if(SectionRoot(n))SectionPose(e,origin,next,qtrue);
  else if(e->m_pVehicle)vehicleMoved=PhysgunMoveVehicle(e,origin,next,contact,fminf(angle,rate*dt));
  else ActorRotateAt(e,origin,next,contact);
 }
 JAR_GripOrigin(grip,point,e->r.currentAngles,origin);
 if(SectionRoot(n)){
  PhysgunLimitTarget(e->r.currentOrigin,64,origin);
  SectionPose(e,origin,e->r.currentAngles,qtrue);
 }else{
  if(e->m_pVehicle){
   PhysgunLimitTarget(e->r.currentOrigin,1024*dt,origin);
   if(DistanceSquared(e->r.currentOrigin,origin)>.000001f&&PhysgunMoveVehicle(e,origin,e->r.currentAngles,NULL,0))vehicleMoved=qtrue;
  }else{
   trace_t tr;
   trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,origin,n,PhysicsMoveMask(e),qfalse,0,0);
   obstructed=tr.startsolid||tr.allsolid||tr.fraction<1;
   if(!tr.startsolid&&!tr.allsolid)Move(e,tr.endpos);
  }
  if(e->client&&!e->m_pVehicle&&objects[n].heldBy)
   PhysgunActorVelocity(&players[objects[n].heldBy-1].motion,before,e->r.currentOrigin,
                        obstructed,objects[n].velocity);
  else{
   VectorSubtract(e->r.currentOrigin,before,objects[n].velocity);
   if(dt>0)VectorScale(objects[n].velocity,1/dt,objects[n].velocity);
  }
  const float speed=VectorLength(objects[n].velocity);
  if(speed>1500)VectorScale(objects[n].velocity,1500/speed,objects[n].velocity);
  if(e->m_pVehicle){
   if(vehicleMoved)ActorPose(e);
  }else if(e->client)ActorSnapshot(e);
 }
}
void JAR_MP_Frame(void){
 trap->Cvar_Update(&sandbox);trap->Cvar_Update(&sharedEditing);trap->Cvar_Update(&cleanupDisconnect);trap->Cvar_Update(&playerTools);trap->Cvar_Update(&mapEditing);trap->Cvar_Update(&worldEditing);
 if(!sandbox.integer)MusicReleaseAll();
 if(sandbox.integer){UpdateFallFloor();CullFallenObjects();}
 const qboolean firstWorldFrame = !worldTried;
 WorldFrame();
 if(sandbox.integer && firstWorldFrame && worldTried)UpdateFallFloor();
 CopyFrame();ActorFrame();TriggerPropBegin();qboolean rigid=PhysicsPrepare();JointClassify(rigid);
 float dt=fminf(.1f,fmaxf(0,(level.time-level.previousTime)*.001f));
 for(int c=0;c<MAX_CLIENTS;++c){jarPlayer_t*cl=&players[c];gentity_t*p=&g_entities[c];
  if(cl->beam!=ENTITYNUM_NONE&&(cl->beamUntil<level.time||!Alive(p)||!sandbox.integer))G_FreeEntity(&g_entities[cl->beam]);
  if(!sandbox.integer||!Alive(p)){Drop(c,qfalse);cl->lastButtons=0;continue;}
  p->client->ps.stats[STAT_WEAPONS]|=(1<<WP_PHYSGUN)|(1<<WP_TOOLGUN);
  if(!(p->client->pers.cmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK)))cl->freezeLatch=0;
  if(cl->held!=ENTITYNUM_NONE){
   if(p->client->ps.weapon!=WP_PHYSGUN||!(p->client->pers.cmd.buttons&BUTTON_ATTACK)||!Manipulate(p,cl->held)){Drop(c,qfalse);}
   else{gentity_t*e=&g_entities[cl->held];vec3_t target,forward;const float* viewAngles=p->client->ps.viewangles;AngleVectors(viewAngles,forward,NULL,NULL);VectorMA(p->client->ps.origin,cl->distance,forward,target);target[2]+=p->client->ps.viewheight;
    const qboolean commandTurn=(cl->turnInput||cl->turnYaw||cl->turnPitch)?qtrue:qfalse;
    // Packed USE input rotates the held object without moving the camera.
    // A raw USE command has no rotation payload, so the hold follows the view normally.
    const float yaw=commandTurn?cl->turnYaw:0;
    const float pitch=commandTurn?cl->turnPitch:0;
    vec3_t facing;
    JAR_GripTurn(&cl->grip,yaw,pitch,viewAngles[YAW],viewAngles[PITCH],viewAngles[ROLL],facing);
    cl->turnYaw=cl->turnPitch=0;
    // Drive a held member toward the target without teleporting the rest of
    // its constraint graph across the map in a single frame.
    const qboolean rigidProp=rigid&&PhysicsSolid(cl->held);
    if(jointNativeKeys[cl->held]){
     vec3_t contact;JAR_GripPoint(&cl->grip,e->r.currentOrigin,e->r.currentAngles,contact);
     PhysgunLimitTarget(contact,rigidProp?50:600*dt,target);
    }
    if(rigidProp)JAR_RigidDragAt(cl->held,target,&cl->grip,facing);
    else PhysgunMoveNative(e,&cl->grip,target,facing,dt);
   }
  }
  cl->lastButtons=p->client->pers.cmd.buttons;
 }
 ForceFrame(dt);
 if(rigid)PhysicsFrame(dt);
 // Native fallback for unsupported collision maps or an explicitly disabled solver.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];if(!e->inuse||!o->owner)continue;
  if(e->s.generic1==251)continue;
  if(e->use==LightUse)LightSync(n);
  const int animated=JAR_MP_AnimatedTag(&e->s);
  if(animated==JAR_MP_BOMB_TAG||animated==JAR_MP_BEACON_TAG)AnimatedSetEnabled(e);
  if(JAR_MP_DoorCameraRootState(&e->s))DoorCameraFrame(e);
  if((rigid&&PhysicsSolid(n))||BodyDormant(n)||!sandbox.integer||!o->catalog||!CatalogBody(&jarMPCatalog[o->catalog-1])||o->frozen||o->heldBy)continue;
  float left=dt;while(left>0){float step=fminf(left,1.f/120);vec3_t next;trace_t tr;left-=step;o->velocity[2]+=-(o->noGravity?0:g_gravity.value)*step;VectorMA(e->r.currentOrigin,step,o->velocity,next);
   trap->Trace(&tr,e->r.currentOrigin,e->r.mins,e->r.maxs,next,n,PhysicsMoveMask(e),qfalse,0,0);if(tr.startsolid||tr.allsolid){VectorClear(o->velocity);break;}
   if(CullFallenObject(e,tr.endpos,e->r.maxs[2]))break;
   Move(e,tr.endpos);
   if(tr.fraction<1){float impact=DotProduct(o->velocity,tr.plane.normal);
    if(impact<-100&&level.time>=o->nextImpact){G_Sound(e,CHAN_BODY,G_SoundIndex("sound/movers/objects/objectHit.wav"));o->nextImpact=level.time+180;}
    int other=tr.entityNum;if(other>=MAX_CLIENTS&&other<ENTITYNUM_WORLD&&objects[other].catalog&&CatalogBody(&jarMPCatalog[objects[other].catalog-1])&&!objects[other].frozen&&!objects[other].heldBy){jarObject_t*b=&objects[other];float relative=impact-DotProduct(b->velocity,tr.plane.normal);if(relative<0){float impulse=-relative/(1/o->mass+1/b->mass);VectorMA(b->velocity,-impulse/b->mass,tr.plane.normal,b->velocity);if(rigid&&PhysicsSolid(other))PhysicsVelocity(other,b->velocity);VectorMA(o->velocity,impulse/o->mass,tr.plane.normal,o->velocity);}}
    else VectorMA(o->velocity,-impact,tr.plane.normal,o->velocity);VectorScale(o->velocity,.7f,o->velocity);
   }
  }
 }
 // Movers can drive attachments. Resolve their motion before publishing the
 // final poses to clients or running effects at the corrected positions.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t*e=&g_entities[n];if(!e->inuse)continue;
  if(e->s.eType==ET_GENERAL&&e->s.generic1==250&&
     (e->s.bolt2==JAR_MP_SLIDING_DOOR_TAG||e->s.bolt2==JAR_MP_LIFT_TAG))SlidingDoorFrame(e);
  else if(e->s.eType==ET_GENERAL&&e->s.generic1==250&&e->s.bolt2==JAR_MP_BOBBING_TAG)BobbingFrame(e);
  else if(JAR_MP_AngularMover(&e->s))AngularFrame(e);
 }
 JointFrame(dt,rigid&&JAR_RigidActive());
 if(sandbox.integer)CullFallenObjects();
 for(int c=0;c<MAX_CLIENTS;++c){
  const jarPlayer_t*cl=&players[c];const int n=cl->held;
  if(n<0||n>=ENTITYNUM_WORLD||!g_entities[n].inuse)continue;
  vec3_t contact;JAR_GripPoint(&cl->grip,g_entities[n].r.currentOrigin,g_entities[n].r.currentAngles,contact);
  Beam(&g_entities[c],contact,qfalse);
 }
 // Pulse from the final pose. Radius damage can run linked entity callbacks.
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){
  gentity_t*e=&g_entities[n];
  if(e->inuse&&JAR_MP_WelderRootState(&e->s))WelderFrame(e);
  else if(e->inuse&&JAR_MP_Searchlight(&e->s))SearchlightFrame(e);
  else if(e->inuse&&JAR_MP_PortalSurface(&e->s))PortalSync(e);
  else if(e->inuse&&JAR_MP_AnimatedTag(&e->s)==JAR_MP_BUTTON_TAG)ButtonTick(e);
  else if(e->inuse&&e->s.eType==ET_GENERAL&&e->s.generic1==250&&e->s.bolt2==JAR_MP_PROBE_TAG)ReflectionSync(e);
 }
 // Publish endpoints after prop movement so ropes share the current snapshot.
 ForceRopeVisuals();MediaFrame();LaserFrame();TriggerFrame();VisibleFrame();MusicFlush();ContextFrame();
 if(sandbox.integer)StatsFrame();
}
