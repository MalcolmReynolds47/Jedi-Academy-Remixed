// Execute production trigger prediction, including the authored-map branch.
#include "cgame/cg_local.h"
#include <assert.h>
#include <stdarg.h>
cg_t cg;static pmove_t cg_pmove;
static int cg_numTriggerEntities;static centity_t*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];
static cgameImport_t imports;cgameImport_t*trap=&imports;
static int items,brushes,contacts;
static void CG_TouchItem(centity_t*c){++items;}
static clipHandle_t Inline(int n){++brushes;assert(n==9);return 1;}
static void Trace(trace_t*t,const vec3_t a,const vec3_t b,const vec3_t mins,const vec3_t maxs,clipHandle_t model,int mask,int capsule){memset(t,0,sizeof(*t));t->startsolid=contacts;}
#include "game/bg_jar_native_jumppad.inc"
#include "cgame/cg_jar_push_prediction.inc"
#include "cgame/cg_jar_native_trigger_prediction.inc"
static void Error(int n,const char*s,...){abort();}static void Print(const char*s,...){ }
void(*Com_Error)(int,const char*,...)=Error;void(*Com_Printf)(const char*,...)=Print;
static centity_t volume,native,item;
static void Reset(void){
 memset(&cg,0,sizeof(cg));memset(&volume,0,sizeof(volume));cg_numTriggerEntities=1;cg_triggerEntities[0]=&volume;
 playerState_t*p=&cg.predictedPlayerState;p->clientNum=0;p->stats[STAT_HEALTH]=100;p->pm_type=PM_NORMAL;p->commandTime=1000;p->pmove_framecount=10;
 entityState_t*s=&volume.currentState;s->number=100;s->eType=ET_PUSH_TRIGGER;s->generic1=247;s->modelindex=64;s->frame=4;s->speed=1000;s->angles2[0]=800;s->trickedentindex=1;s->time=-59000;s->pos.trTime=1000;s->pos.trType=s->apos.trType=TR_INTERPOLATE;
 VectorSet(cg_pmove.mins,-15,-15,-24);VectorSet(cg_pmove.maxs,15,15,32);CG_JAR_BeginPushPrediction();
}
static centity_t driven;static Vehicle_t drivenVehicle;static vehicleInfo_t drivenInfo;static pmove_t drivenMove;
static void VehicleReset(vehicleType_t type){
 Reset();memset(&driven,0,sizeof(driven));memset(&drivenVehicle,0,sizeof(drivenVehicle));memset(&drivenInfo,0,sizeof(drivenInfo));memset(&drivenMove,0,sizeof(drivenMove));
 driven.currentValid=qtrue;driven.currentState.number=90;driven.currentState.eType=ET_NPC;driven.currentState.NPC_class=CLASS_VEHICLE;driven.m_pVehicle=&drivenVehicle;drivenVehicle.m_pVehicleInfo=&drivenInfo;drivenInfo.type=type;
 playerState_t*v=&cg.predictedVehicleState;*v=cg.predictedPlayerState;v->clientNum=90;v->m_iVehicleNum=1;v->speed=700;v->groundEntityNum=ENTITYNUM_WORLD;
 cg.predictedPlayerState.m_iVehicleNum=90;drivenMove.ps=v;VectorSet(drivenMove.mins,-80,-30,-40);VectorSet(drivenMove.maxs,80,30,40);drivenMove.cmd.forwardmove=127;drivenMove.cmd.rightmove=64;drivenMove.cmd.buttons=BUTTON_ATTACK;
 volume.currentState.trickedentindex=0;volume.currentState.trickedentindex3=1;
}
static void VehiclePredictionCases(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int family=0;family<4;++family)for(int mode=0;mode<3;++mode){
  VehicleReset(types[family]);entityState_t*s=&volume.currentState;playerState_t*v=&cg.predictedVehicleState;
  s->frame=mode==0?4:mode==1?20:0;s->bolt2=1;VectorSet(s->origin2,128,0,96);v->origin[0]=32;
  vec3_t expected;assert(JAR_PushVolumeVelocity(s->frame,s->pos.trBase,s->apos.trBase,s->origin2,v->origin,s->speed,s->angles2[0],expected)==1);
  usercmd_t command=drivenMove.cmd;playerState_t pilot=cg.predictedPlayerState;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);
  assert(Distance(v->velocity,expected)<.001f&&v->speed==0&&v->jarVehicleDrift[0]==v->velocity[0]&&v->jarVehicleDrift[2]==0&&v->groundEntityNum==ENTITYNUM_NONE&&!memcmp(&command,&drivenMove.cmd,sizeof(command))&&!memcmp(&pilot,&cg.predictedPlayerState,sizeof(pilot)));
  if(!s->frame)assert(v->jumppad_ent==100&&v->jumppad_frame==10);
 }
 // A vehicle hull can contact when its pilot and origin are outside the volume.
 VehicleReset(VH_FIGHTER);cg.predictedVehicleState.origin[0]=130;cg.predictedPlayerState.origin[0]=500;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(cg.predictedVehicleState.velocity[0]==1000);cg.predictedVehicleState.origin[0]=150;VectorClear(cg.predictedVehicleState.velocity);CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!VectorLength(cg.predictedVehicleState.velocity));
 // Independent pilot masks cover every client slot; player permission alone
 // never permits vehicle prediction. Stale ownership/seat/boarding refuses it.
 for(int client=0;client<32;++client){
  VehicleReset(VH_SPEEDER);entityState_t*s=&volume.currentState;playerState_t*v=&cg.predictedVehicleState;cg.predictedPlayerState.clientNum=driven.currentState.owner=client;v->m_iVehicleNum=client+1;
  s->trickedentindex3=client<16?1u<<client:0;s->trickedentindex4=client>=16?1u<<(client-16):0;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(v->velocity[0]==1000);
  s->trickedentindex3=s->trickedentindex4=0;s->trickedentindex=s->trickedentindex2=65535;VectorClear(v->velocity);CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!VectorLength(v->velocity));
 }
 for(int refusal=0;refusal<14;++refusal){
  VehicleReset(VH_WALKER);playerState_t*v=&cg.predictedVehicleState,*p=&cg.predictedPlayerState;
  switch(refusal){case 0:driven.currentState.owner=1;break;case 1:v->m_iVehicleNum=2;break;case 2:p->m_iVehicleNum=91;break;case 3:v->vehBoarding=1;break;case 4:v->pm_type=PM_FREEZE;break;case 5:p->pm_type=PM_FREEZE;break;case 6:p->stats[STAT_HEALTH]=0;break;case 7:v->stats[STAT_HEALTH]=0;break;case 8:driven.currentValid=qfalse;break;case 9:driven.m_pVehicle=NULL;break;case 10:v->clientNum=91;break;case 11:p->duelInProgress=1;break;case 12:p->fd.forceGripBeingGripped=1001;break;case 13:volume.currentState.speed=NAN;break;}
  playerState_t before=*v;jarPushPrediction_t pending=jarPushPrediction[0];CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!memcmp(&before,v,sizeof(before))&&!memcmp(&pending,&jarPushPrediction[0],sizeof(pending)));
 }
 VehicleReset(VH_ANIMAL);entityState_t*s=&volume.currentState;playerState_t*v=&cg.predictedVehicleState;s->time2=500;CG_JAR_BeginPushPrediction();CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(v->velocity[0]==1000);VectorClear(v->velocity);v->commandTime=1100;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!VectorLength(v->velocity));v->commandTime=1500;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(v->velocity[0]==1000);
 v->commandTime=1000;VectorClear(v->velocity);CG_JAR_BeginPushPrediction();CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(v->velocity[0]==1000);s->time=1000;v->commandTime=1200;VectorClear(v->velocity);CG_JAR_BeginPushPrediction();CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!VectorLength(v->velocity));
 VehicleReset(VH_SPEEDER);s->time2=-1;CG_JAR_BeginPushPrediction();CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(jarPushPrediction[0].spent);VectorClear(v->velocity);CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(!VectorLength(v->velocity));CG_JAR_BeginPushPrediction();CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(v->velocity[0]==1000);
 VehicleReset(VH_FIGHTER);s->pos.trBase[0]=400;VectorSet(s->pos.trDelta,-4000,0,0);s->apos.trDelta[YAW]=900;v->commandTime=1100;CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(fabsf(v->velocity[1]-1000)<.001f);v->commandTime=1500;VectorClear(v->velocity);CG_JAR_PredictVehiclePushVolumes(&drivenMove,&driven);assert(fabsf(v->velocity[1]-1000)<.001f);
 puts("MP vehicle push prediction: production client loop, four families, linear/relative/arc paths, actual hull contact, independent pilot masks, seat refusals, unchanged input and cooldown/once replay passed.");
}
#include "mp_teleport_prediction.inc"
#include "mp_emplaced_presentation.inc"
#include "mp_animated_presentation.inc"
#include "mp_door_camera_presentation.inc"
#include "mp_rack_item_presentation.inc"
int main(void){
 VehiclePredictionCases();
 imports.CM_InlineModel=Inline;imports.CM_Trace=Trace;Reset();entityState_t*s=&volume.currentState;playerState_t*p=&cg.predictedPlayerState;
 CG_TouchTriggerPrediction();assert(p->velocity[0]==1000&&!brushes);
 // Relative aim uses the predicted actor position; linear aim uses the source.
 s->bolt2=1;VectorSet(s->origin2,128,0,96);VectorSet(p->origin,32,0,0);s->frame=20;CG_TouchTriggerPrediction();assert(fabsf(p->velocity[0]-sqrtf(500000))<.002f);
 s->frame=4;CG_TouchTriggerPrediction();assert(fabsf(p->velocity[0]-800)<.001f&&fabsf(p->velocity[2]-600)<.001f);
 s->frame=0;p->fd.forcePowersActive=1<<FP_LEVITATION;CG_TouchTriggerPrediction();assert(fabsf(p->velocity[0]-128/sqrtf(.24f))<.001f&&fabsf(p->velocity[2]-sqrtf(153600))<.001f&&p->jumppad_ent==100&&p->jumppad_frame==10&&!p->fd.forcePowersActive);
 // Shared SAT rejects the enclosing AABB's extra corners after rotation.
 Reset();s->apos.trBase[YAW]=45;VectorSet(p->origin,85,85,0);CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));VectorSet(p->origin,60,0,0);CG_TouchTriggerPrediction();assert(fabsf(p->velocity[0]-sqrtf(500000))<.002f);
 // Position and rotation advance only through the bounded 100 ms horizon.
 Reset();s->pos.trBase[0]=200;VectorSet(s->pos.trDelta,-2000,0,0);s->apos.trDelta[YAW]=900;p->commandTime=1100;CG_TouchTriggerPrediction();assert(fabsf(p->velocity[1]-1000)<.001f);VectorClear(p->velocity);p->commandTime=1000;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->commandTime=1300;CG_TouchTriggerPrediction();assert(fabsf(p->velocity[1]-1000)<.001f);
 // Timers are replay-local. Redrawing from the same snapshot is deterministic;
 // the next snapshot's authoritative cooldown supersedes local guesses.
 Reset();s->time2=500;CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);VectorClear(p->velocity);p->commandTime=1100;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->commandTime=1500;CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);
 p->commandTime=1000;VectorClear(p->velocity);CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);s->time=1000;p->commandTime=1200;VectorClear(p->velocity);CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));
 Reset();s->frame=2052;s->time2=500;s->time=1000;CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);VectorClear(p->velocity);p->commandTime=1001;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));
 Reset();s->time2=-1;CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(jarPushPrediction[0].spent);VectorClear(p->velocity);p->commandTime=1050;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);
 // Permission changes, death, control, vehicle boarding and malformed snapshots
 // fail closed without accidentally treating the extent as a BSP model index.
 Reset();for(int client=0;client<32;++client){p->clientNum=client;s->trickedentindex=client<16?1u<<client:0;s->trickedentindex2=client>=16?1u<<(client-16):0;VectorClear(p->velocity);CG_TouchTriggerPrediction();assert(p->velocity[0]==1000);s->trickedentindex=s->trickedentindex2=0;VectorClear(p->velocity);CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));}
 Reset();p->m_iVehicleNum=90;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->m_iVehicleNum=0;p->pm_type=PM_FREEZE;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->pm_type=PM_NORMAL;p->fd.forceGripBeingGripped=1001;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->fd.forceGripBeingGripped=0;p->stats[STAT_HEALTH]=0;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));p->stats[STAT_HEALTH]=100;
 s->modelindex=513;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));s->modelindex=64;s->frame=16;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));s->frame=4;s->speed=NAN;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity)&&!brushes);
 Reset();s->frame=0;s->bolt2=1;VectorSet(s->origin2,0,0,-1);CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));s->origin2[2]=100;s->angles2[0]=0;CG_TouchTriggerPrediction();assert(!VectorLength(p->velocity));
 // Authored BSP pads/items still execute the unmodified native path.
 Reset();memset(&native,0,sizeof(native));memset(&item,0,sizeof(item));native.currentState.number=110;native.currentState.eType=ET_PUSH_TRIGGER;native.currentState.solid=SOLID_BMODEL;native.currentState.modelindex=9;VectorSet(native.currentState.origin2,1,2,3);item.currentState.eType=ET_ITEM;cg_triggerEntities[1]=&native;cg_triggerEntities[2]=&item;cg_numTriggerEntities=3;contacts=1;CG_JAR_BeginPushPrediction();CG_TouchTriggerPrediction();assert(p->velocity[0]==1&&p->velocity[1]==2&&p->velocity[2]==3&&items==1&&brushes==1);native.currentState.eType=ET_TELEPORT_TRIGGER;CG_TouchTriggerPrediction();assert(cg.hyperspace);
 TeleportPredictionCases();
 EmplacedPresentationCases();
 AnimatedPresentationCases();
 DoorCameraPresentationCases();
 RackItemPresentationCases();
 puts("MP push prediction: actual trigger loop, shared SAT/aim, rotated/moving volumes, linear/relative/arc velocity, cooldown/once replay, all 32 consent bits, malformed refusal and native brush/item paths passed.");
}
