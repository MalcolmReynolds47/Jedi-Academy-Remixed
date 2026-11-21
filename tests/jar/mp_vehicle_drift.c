// Real native step/slide and drift integration; deterministic BSP/impact boundary.
// Native controller output is supplied here, not a claim of human driving QA.
#include "qcommon/q_shared.h"
#include "game/bg_public.h"
#include "game/bg_local.h"
#include "game/bg_vehicles.h"
#include <assert.h>
#include <stdarg.h>
pmove_t*pm;
pml_t pml;
bgEntity_t*pm_entSelf;
int c_pmove;
static int impacts,touches,traces;
static float wall=100000;
void QDECL Com_Printf(const char*fmt,...){}
void QDECL Com_Error(int code,const char*fmt,...){abort();}
void PM_AddEvent(int e){}
void PM_AddTouchEnt(int n){++touches;}
void PM_VehicleImpact(bgEntity_t*e,trace_t*t){assert(e==pm_entSelf);++impacts;}
qboolean BG_InReboundHold(int anim){return qfalse;}
static qboolean PM_GroundSlideOkay(float z){return qtrue;}
#include "game/bg_jar_native_clip.inc"
#include "game/bg_jar_native_slide.inc"
#include "game/bg_jar_vehicle_drift.h"
#include "game/bg_jar_native_jumppad.inc"
#include "game/jar_mp_push_volume.h"
#include "game/bg_jar_vehicle_drift.inc"
static void Trace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask){
 ++traces;assert(pass==MAX_CLIENTS&&mins[0]==-8&&maxs[0]==8);memset(t,0,sizeof(*t));t->fraction=1;t->entityNum=ENTITYNUM_WORLD;VectorCopy(end,t->endpos);
 // Half-space x <= wall, with the full vehicle hull in the sweep.
 if(end[0]+maxs[0]>wall&&end[0]>start[0]){t->fraction=fmaxf(0,(wall-maxs[0]-start[0])/(end[0]-start[0]));VectorMA(start,t->fraction,(float[3]){end[0]-start[0],end[1]-start[1],end[2]-start[2]},t->endpos);t->plane.normal[0]=-1;}
}
static pmove_t move;static playerState_t ps;static bgEntity_t ent;static Vehicle_t vehicle;static vehicleInfo_t info;
static void Reset(vehicleType_t type){
 memset(&move,0,sizeof(move));memset(&ps,0,sizeof(ps));memset(&ent,0,sizeof(ent));memset(&vehicle,0,sizeof(vehicle));memset(&info,0,sizeof(info));memset(&pml,0,sizeof(pml));
 pm=&move;move.ps=&ps;pm_entSelf=&ent;move.trace=Trace;move.stepSlideFix=qtrue;ps.clientNum=MAX_CLIENTS;ps.groundEntityNum=ENTITYNUM_NONE;ps.gravity=800;ps.speed=300;
 VectorSet(move.mins,-8,-8,-8);VectorSet(move.maxs,8,8,8);ent.s.eType=ET_NPC;ent.s.NPC_class=CLASS_VEHICLE;ent.m_pVehicle=&vehicle;vehicle.m_pVehicleInfo=&info;info.type=type;info.hoverHeight=type==VH_SPEEDER||type==VH_FIGHTER?32:0;
 wall=100000;impacts=touches=traces=0;move.cmd.forwardmove=127;move.cmd.rightmove=64;move.cmd.buttons=BUTTON_ATTACK;ps.vehOrientation[YAW]=45;
}
int main(void){
 const vehicleType_t types[]={VH_SPEEDER,VH_FIGHTER,VH_ANIMAL,VH_WALKER};
 for(int type=0;type<4;++type){
  Reset(types[type]);VectorSet(ps.velocity,300,100,0);VectorSet(ps.jarVehicleDrift,0,100,0);usercmd_t command=move.cmd;
  // Native throttle may replace the base velocity each command. External
  // transverse motion survives that replacement without changing the command.
  for(int i=0;i<40;++i){pml.frametime=.025f;assert(PM_JARVehicleDriftBegin());assert(fabsf(ps.velocity[1])<.001f);VectorSet(ps.velocity,300,0,0);PM_StepSlideMove(qfalse);PM_JARVehicleDriftMove();}
  assert(fabsf(ps.origin[0]-300)<.01f&&ps.origin[1]>87&&ps.origin[1]<90&&fabsf(ps.jarVehicleDrift[1]-100*expf(-.25f))<.01f);
  assert(ps.speed==300&&ps.vehOrientation[YAW]==45&&!memcmp(&command,&move.cmd,sizeof(command)));
  // Release/dismount need no special controller to retain remaining momentum.
  vehicle.m_pPilot=NULL;assert(PM_JARVehicleDriftBegin());VectorClear(ps.velocity);PM_JARVehicleDriftMove();assert(ps.jarVehicleDrift[1]>70);
  // Native gravity runs exactly once; the lateral sweep adds no second fall.
  Reset(types[type]);pml.frametime=.05f;VectorSet(ps.jarVehicleDrift,0,100,0);VectorCopy(ps.jarVehicleDrift,ps.velocity);assert(PM_JARVehicleDriftBegin());PM_StepSlideMove(qtrue);float nativeZ=ps.velocity[2],positionZ=ps.origin[2];PM_JARVehicleDriftMove();assert(ps.velocity[2]==nativeZ&&ps.origin[2]==positionZ&&nativeZ==-40);
  // Full swept hull contacts block drift at a wall and publish native impacts.
  Reset(types[type]);wall=40;pml.frametime=.05f;VectorSet(ps.jarVehicleDrift,1000,200,0);VectorCopy(ps.jarVehicleDrift,ps.velocity);assert(PM_JARVehicleDriftBegin());PM_JARVehicleDriftMove();assert(ps.origin[0]<=32.01f&&ps.origin[1]>9&&fabsf(ps.jarVehicleDrift[0])<2&&impacts&&touches&&traces);
 }
 // A predicted push enters the same native drift/step-slide path next command.
 for(int type=0;type<4;++type){
  Reset(types[type]);entityState_t sensor={0};sensor.number=100;sensor.eType=ET_PUSH_TRIGGER;sensor.generic1=247;sensor.modelindex=64;sensor.frame=4;sensor.speed=1000;sensor.angles2[0]=800;sensor.trickedentindex3=1;sensor.apos.trBase[YAW]=90;
  playerState_t pilot={0};pilot.stats[STAT_HEALTH]=100;pilot.pm_type=PM_NORMAL;pilot.m_iVehicleNum=MAX_CLIENTS;ps.stats[STAT_HEALTH]=100;ps.pm_type=PM_NORMAL;ps.m_iVehicleNum=1;jarPushPrediction_t pending={0};usercmd_t command=move.cmd;
  assert(JAR_PushVolumePredictVehicle(&sensor,&pending,&ps,&pilot,move.mins,move.maxs));
  for(int i=0;i<40;++i){pml.frametime=.025f;assert(PM_JARVehicleDriftBegin());VectorSet(ps.velocity,300,0,0);PM_StepSlideMove(qfalse);PM_JARVehicleDriftMove();}
  assert(fabsf(ps.origin[0]-300)<.1f&&ps.origin[1]>880&&ps.origin[1]<890&&ps.jarVehicleDrift[1]>778&&!memcmp(&command,&move.cmd,sizeof(command))&&ps.vehOrientation[YAW]==45);
 }
 // Controller pushes survive the next native driving output at all supported
 // command rates. Collision still uses the full vehicle hull at maximum speed.
 const int pushRates[]={20,40,125};
 for(int type=0;type<4;++type)for(int rate=0;rate<3;++rate){
  Reset(types[type]);usercmd_t input=move.cmd;vec3_t launch={4000,4000,4000};VectorSet(ps.velocity,300,10,-200);
  assert(JAR_VehiclePushVelocity(&ps,launch)&&ps.speed==0&&VectorCompare(ps.velocity,launch)&&ps.jarVehicleDrift[2]==0);
  pml.frametime=1.f/pushRates[rate];assert(PM_JARVehicleDriftBegin());assert(ps.velocity[0]==0&&ps.velocity[1]==0&&ps.velocity[2]==4000);
  ps.velocity[0]=100;PM_StepSlideMove(qtrue);float nativeHeight=ps.origin[2],nativeFall=ps.velocity[2];PM_JARVehicleDriftMove();
  assert(ps.origin[0]>4000*pml.frametime&&ps.origin[1]>3999*pml.frametime&&ps.origin[2]==nativeHeight&&ps.velocity[2]==nativeFall);
  assert(ps.jarVehicleDrift[0]>3900&&!memcmp(&input,&move.cmd,sizeof(input)));
  vec3_t unchanged;VectorCopy(ps.velocity,unchanged);JAR_VehicleDriftVelocity(&ps,unchanged);assert(VectorCompare(unchanged,ps.velocity));
  Reset(types[type]);wall=30;pml.frametime=1.f/pushRates[rate];assert(JAR_VehiclePushVelocity(&ps,launch));assert(PM_JARVehicleDriftBegin());PM_JARVehicleDriftMove();
  assert(ps.origin[0]<=22.01f&&impacts&&touches&&traces&&ps.jarVehicleDrift[0]<5);
  Reset(types[type]);pml.frametime=1.f/pushRates[rate];VectorSet(launch,0,0,4000);assert(JAR_VehiclePushVelocity(&ps,launch));assert(!PM_JARVehicleDriftBegin());PM_StepSlideMove(qtrue);assert(ps.velocity[2]<4000&&ps.velocity[2]>0&&ps.origin[2]>0);
 }
 Reset(VH_FIGHTER);vec3_t invalid={4001,0,0};playerState_t rejected=ps;assert(!JAR_VehiclePushVelocity(&ps,invalid)&&!memcmp(&ps,&rejected,sizeof(ps)));invalid[0]=NAN;assert(!JAR_VehiclePushVelocity(&ps,invalid)&&!memcmp(&ps,&rejected,sizeof(ps)));
 // Sustained upward force must keep overcoming native gravity rather than
 // accumulating a separately capped upward component against unlimited fall.
 Reset(VH_SPEEDER);pml.frametime=.025f;
 for(int i=0;i<160;++i){vec3_t upward;VectorCopy(ps.velocity,upward);upward[2]+=30;JAR_VehicleDriftVelocity(&ps,upward);qboolean active=PM_JARVehicleDriftBegin();PM_StepSlideMove(qtrue);if(active)PM_JARVehicleDriftMove();}
 assert(ps.velocity[2]>0&&ps.origin[2]>0);
 // Identical damping at different command rates, with bounded integration error.
 const int rates[]={20,40,125};float positions[3];
 for(int k=0;k<3;++k){Reset(VH_SPEEDER);VectorSet(ps.jarVehicleDrift,0,100,0);VectorCopy(ps.jarVehicleDrift,ps.velocity);for(int i=0;i<rates[k];++i){pml.frametime=1.f/rates[k];pml.walking=pml.groundPlane=qtrue;pml.groundTrace.plane.normal[2]=1;assert(PM_JARVehicleDriftBegin());PM_JARVehicleDriftMove();}assert(fabsf(ps.jarVehicleDrift[1]-100*expf(-4))<.001f);positions[k]=ps.origin[1];}assert(fabsf(positions[0]-positions[2])<2.2f);
 Reset(VH_SPEEDER);playerState_t before=ps;assert(!PM_JARVehicleDriftBegin()&&!memcmp(&before,&ps,sizeof(ps)));ent.s.NPC_class=CLASS_NONE;ps.jarVehicleDrift[1]=20;before=ps;assert(!PM_JARVehicleDriftBegin()&&!memcmp(&before,&ps,sizeof(ps)));
 Reset(VH_SPEEDER);vec3_t force={5000,0,0};JAR_VehicleDriftVelocity(&ps,force);assert(ps.jarVehicleDrift[0]==1500&&ps.velocity[0]==1500&&ps.speed==300);
 force[1]=NAN;before=ps;JAR_VehicleDriftVelocity(&ps,force);assert(!memcmp(&before,&ps,sizeof(ps)));
 puts("MP vehicle drift: native swept step/slide, four hull families, impact handoff, throttle separation, one gravity pass, rate-independent damping, bounded impulses and maximum-speed controller pushes passed.");
}
