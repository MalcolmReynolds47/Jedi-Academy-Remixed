// Production MP first-person and chase-camera paths with controlled trace/ideal
// boundaries. No renderer or graphical game is launched. GPL-2.0-only.
#include "cgame/cg_local.h"
#include <assert.h>
#include <stdarg.h>
cg_t cg;centity_t cg_entities[MAX_GENTITIES];
vmCvar_t jar_cameraPreset,com_cameraMode,cg_thirdPersonSpecialCam;
vmCvar_t cg_thirdPersonTargetDamp,cg_thirdPersonCameraDamp,cg_smoothCamera;
vmCvar_t cg_runPitch,cg_runRoll,cg_bobPitch,cg_bobRoll,cg_bobUp;
static snapshot_t snapshot;
static int traces,lerps,obstruction,special;
static void Error(int n,const char*s,...){abort();}static void Print(const char*s,...){}
void(*Com_Error)(int,const char*,...)=Error;void(*Com_Printf)(const char*,...)=Print;
qboolean BG_SaberInSpecial(int move){return special?qtrue:qfalse;}
#define CAMERA_DAMP_INTERVAL 50
#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
static vec3_t cameramins={-4,-4,-4},cameramaxs={4,4,4};
static vec3_t cameraFocusAngles,cameraFocusLoc,cameraIdealTarget,cameraIdealLoc,cameraCurTarget,cameraCurLoc;
static int cameraLastFrame;static float cameraStiffFactor;
static void CG_CalcIdealThirdPersonViewTarget(void){}
static void CG_CalcIdealThirdPersonViewLocation(void){}
void CG_Trace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int skip,int mask){
 ++traces;assert(mins[0]==-4&&maxs[0]==4&&skip==0&&mask==MASK_CAMERACLIP);memset(t,0,sizeof(*t));t->fraction=obstruction?.5f:1;t->entityNum=obstruction==2?100:ENTITYNUM_WORLD;VectorCopy(end,t->endpos);if(obstruction)t->endpos[0]=25;
}
void CG_CalcEntityLerpPositions(centity_t*c){++lerps;assert(c==&cg_entities[100]);VectorSet(c->lerpOrigin,5,6,7);}
#include "cgame/cg_jar_camera.inc"
#include "cgame/cg_jar_native_camera_step.inc"
#include "cgame/cg_jar_native_camera_target.inc"
#include "cgame/cg_jar_native_camera_damp.inc"
#include "cgame/cg_jar_native_camera_first.inc"
static void Reset(int preset){
 memset(&cg,0,sizeof(cg));memset(&snapshot,0,sizeof(snapshot));cg.snap=&snapshot;cg.time=1000;snapshot.ps.stats[STAT_HEALTH]=cg.predictedPlayerState.stats[STAT_HEALTH]=100;cg.predictedPlayerState.viewheight=26;
 jar_cameraPreset.integer=preset;com_cameraMode.integer=cg_thirdPersonSpecialCam.integer=cg_smoothCamera.integer=0;traces=lerps=obstruction=special=0;
 cg_runPitch.value=cg_bobPitch.value=cg_bobRoll.value=.002f;cg_runRoll.value=cg_bobUp.value=.005f;cg_thirdPersonTargetDamp.value=.5f;cg_thirdPersonCameraDamp.value=.3f;
 VectorSet(cg.predictedPlayerState.velocity,100,50,0);AxisClear(cg.refdef.viewaxis);cg.xyspeed=200;cg.bobfracsin=.5f;
 VectorClear(cameraFocusAngles);VectorClear(cameraFocusLoc);VectorClear(cameraCurTarget);VectorClear(cameraCurLoc);VectorSet(cameraIdealTarget,100,0,0);VectorSet(cameraIdealLoc,100,0,0);cameraLastFrame=950;cameraStiffFactor=0;
}
static void Protect(int reason){
 switch(reason){case 0:cg.predictedPlayerState.zoomMode=1;break;case 1:snapshot.ps.zoomMode=2;break;case 2:cg.predictedPlayerState.m_iVehicleNum=90;break;case 3:snapshot.ps.m_iVehicleNum=90;break;case 4:cg.predictedPlayerState.emplacedIndex=90;break;case 5:snapshot.ps.emplacedIndex=90;break;case 6:cg.predictedPlayerState.pm_type=PM_SPECTATOR;break;case 7:snapshot.ps.pm_flags=PMF_FOLLOW;break;case 8:cg.predictedPlayerState.eFlags2=EF2_HELD_BY_MONSTER;break;case 9:snapshot.ps.eFlags2=EF2_HELD_BY_MONSTER;break;case 10:cg.predictedPlayerState.fallingToDeath=1;break;case 11:com_cameraMode.integer=1;break;case 12:cg_thirdPersonSpecialCam.integer=1;special=1;break;}
}
int main(void){
 for(int preset=0;preset<=2;++preset){
  Reset(preset);float motion=JAR_CameraMotion(preset);CG_OffsetFirstPersonView();assert(fabsf(cg.refdef.viewangles[PITCH]-.4f*motion)<.00001f&&fabsf(cg.refdef.viewangles[ROLL]+.05f*motion)<.00001f&&fabsf(cg.refdef.vieworg[2]-(26+.5f*motion))<.00001f);
  assert(cg_runPitch.value==.002f&&cg_runRoll.value==.005f&&cg_bobUp.value==.005f);
  Reset(preset);cg.predictedPlayerState.pm_flags=PMF_DUCKED;cg.bobcycle=1;CG_OffsetFirstPersonView();assert(fabsf(cg.refdef.viewangles[PITCH]-.8f*motion)<.00001f&&fabsf(cg.refdef.viewangles[ROLL]+.85f*motion)<.00001f);
  for(int smooth=0;smooth<=1;++smooth){
   Reset(preset);cg_smoothCamera.integer=smooth;CG_UpdateThirdPersonTargetDamp();CG_UpdateThirdPersonCameraDamp();assert(fabsf(cameraCurTarget[0]-100*JAR_CameraDamping(preset,.5f))<.001f&&fabsf(cameraCurLoc[0]-100*JAR_CameraDamping(preset,.3f))<.001f&&traces==2);
  }
 }
 // Protected native camera modes bypass a chosen preset, including during the
 // one-snapshot delay between native and predicted state changes.
 for(int reason=0;reason<13;++reason){
  Reset(0);Protect(reason);CG_OffsetFirstPersonView();vec3_t angles,origin;VectorCopy(cg.refdef.viewangles,angles);VectorCopy(cg.refdef.vieworg,origin);CG_UpdateThirdPersonTargetDamp();CG_UpdateThirdPersonCameraDamp();vec3_t target,location;VectorCopy(cameraCurTarget,target);VectorCopy(cameraCurLoc,location);
  Reset(2);Protect(reason);assert(CG_JARCameraPreset()==0);CG_OffsetFirstPersonView();assert(VectorCompare(cg.refdef.viewangles,angles)&&VectorCompare(cg.refdef.vieworg,origin));CG_UpdateThirdPersonTargetDamp();CG_UpdateThirdPersonCameraDamp();assert(VectorCompare(target,cameraCurTarget)&&VectorCompare(location,cameraCurLoc));
 }
 // The preset scales run/bob only: native weapon/damage kick, duck, landing,
 // step, dead and intermission behavior remains independent.
 vec3_t nativeAngles,nativeOrigin;
 for(int preset=0;preset<=2;++preset){Reset(preset);cg_runPitch.value=cg_runRoll.value=cg_bobPitch.value=cg_bobRoll.value=cg_bobUp.value=0;cg.kick_time=900;VectorSet(cg.kick_angles,2,4,6);VectorSet(cg.kick_origin,1,2,3);cg.damageTime=950;cg.v_dmg_pitch=10;cg.v_dmg_roll=5;cg.duckTime=950;cg.duckChange=12;cg.landTime=950;cg.landChange=-6;cg.stepTime=950;cg.stepChange=4;CG_OffsetFirstPersonView();if(!preset){VectorCopy(cg.refdef.viewangles,nativeAngles);VectorCopy(cg.refdef.vieworg,nativeOrigin);}else assert(VectorCompare(nativeAngles,cg.refdef.viewangles)&&VectorCompare(nativeOrigin,cg.refdef.vieworg));}
 Reset(2);snapshot.ps.stats[STAT_HEALTH]=0;snapshot.ps.stats[STAT_DEAD_YAW]=75;CG_OffsetFirstPersonView();assert(cg.refdef.viewangles[ROLL]==40&&cg.refdef.viewangles[PITCH]==-15&&cg.refdef.viewangles[YAW]==75&&cg.refdef.vieworg[2]==26);
 Reset(2);snapshot.ps.pm_type=PM_INTERMISSION;CG_OffsetFirstPersonView();assert(!VectorLength(cg.refdef.vieworg)&&!VectorLength(cg.refdef.viewangles));
 // Collision clipping is still applied even when Crisp snaps to its ideal.
 Reset(2);obstruction=1;CG_UpdateThirdPersonTargetDamp();CG_UpdateThirdPersonCameraDamp();assert(cameraCurTarget[0]==25&&cameraCurLoc[0]==25&&traces==2);
 Reset(2);obstruction=2;centity_t*m=&cg_entities[100];memset(m,0,sizeof(*m));m->currentState.solid=SOLID_BMODEL;m->currentState.eType=ET_MOVER;m->currentState.pos.trType=TR_SINE;VectorSet(m->currentState.pos.trBase,7,8,9);entityState_t before=m->currentState;CG_UpdateThirdPersonCameraDamp();assert(lerps==1&&traces==2&&cameraCurLoc[0]==25&&!memcmp(&before,&m->currentState,sizeof(before)));
 Reset(1);cg.thisFrameTeleport=qtrue;CG_UpdateThirdPersonTargetDamp();CG_UpdateThirdPersonCameraDamp();assert(cameraCurTarget[0]==100&&cameraCurLoc[0]==100);
 Reset(2);cg.snap=NULL;assert(CG_JARCameraPreset()==0);Reset(99);assert(CG_JARCameraPreset()==0);CG_OffsetFirstPersonView();assert(fabsf(cg.refdef.viewangles[PITCH]-.4f)<.00001f);
 puts("MP camera presets: production first-person/chase paths, shared SP coefficients, crouch/bob/run, native kick/landing/death, protected views, clipping/mover retrace, teleport and untouched cvars passed.");
}
