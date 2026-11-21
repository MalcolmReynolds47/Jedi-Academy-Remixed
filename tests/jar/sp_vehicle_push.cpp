// Run the native SP step/slide code against deterministic hull contacts.
#include "game/g_local.h"
#include "game/bg_local.h"
#include "game/g_vehicles.h"
#include "game/g_sandbox_vehicle_push.h"
#include "game/g_workshop_vehicle_thruster.h"
#include "test_support.h"

pmove_t* pm;
pml_t pml{};
int c_pmove;
level_locals_t level{};
game_export_t globals{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray& _TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static cvar_t stepSlideFix{};
cvar_t* g_stepSlideFix=&stepSlideFix;
static int impacts,touches;
static float wall;
static bool floorContact;
enum class ImpactChange {None,Remove,Reuse};
static ImpactChange impactChange;
static uint64_t moverLifetime=1;
uint64_t G_EntityLifetime(const gentity_t*){return moverLifetime;}
static int blockedThrustMember=ENTITYNUM_NONE;
bool G_JAR_WorkshopTransportAllowed(const gentity_t& member){return member.s.number!=blockedThrustMember;}

qboolean PM_ControlledByPlayer(){return qfalse;}
qboolean PM_InReboundHold(int){return qfalse;}
qboolean PM_InSpecialJump(int){return qfalse;}
qboolean PM_GroundSlideOkay(float){return qtrue;}
qboolean PM_CheckGrabWall(trace_t*){return qfalse;}
static void DoImpact(gentity_t* mover,gentity_t*,qboolean,trace_t*){
 ++impacts;
 if(impactChange==ImpactChange::Remove){
  mover->inuse=qfalse;
  mover->client=nullptr;
  mover->m_pVehicle=nullptr;
  pm->ps=nullptr; // Simulate inaccessible storage after a synchronous free.
 }else if(impactChange==ImpactChange::Reuse){
  ++moverLifetime;
  VectorSet(mover->client->ps.velocity,10,20,30);
  VectorSet(mover->m_pVehicle->jarPushDrift,40,50,0);
 }
}
#include "game/bg_client_impact.inc"
void PM_AddTouchEnt(int){++touches;}
void PM_AddEvent(int){}
void G_DebugLine(vec3_t,vec3_t,int,int,qboolean){}
void PM_ClipVelocity(vec3_t in,vec3_t normal,vec3_t out,float overbounce){
 const float dot=DotProduct(in,normal);
 VectorMA(in,-(dot<0?dot*overbounce:dot/overbounce),normal,out);
}

#include "game/bg_jar_vehicle_push.inc"

static void Trace(trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,
 const vec3_t end,int pass,int,EG2_Collision,int){
 CHECK(pass==MAX_CLIENTS&&mins[0]==-8&&maxs[0]==8);
 *hit={};hit->fraction=1;hit->entityNum=ENTITYNUM_WORLD;VectorCopy(end,hit->endpos);
 if(floorContact&&end[2]<8&&end[2]<start[2]){
  hit->fraction=(start[2]-8)/(start[2]-end[2]);
  vec3_t travel;VectorSubtract(end,start,travel);VectorMA(start,hit->fraction,travel,hit->endpos);
  VectorSet(hit->plane.normal,0,0,1);
  return;
 }
 if(end[0]+maxs[0]>wall&&end[0]>start[0]){
  hit->fraction=std::max(0.f,(wall-maxs[0]-start[0])/(end[0]-start[0]));
  vec3_t travel;VectorSubtract(end,start,travel);VectorMA(start,hit->fraction,travel,hit->endpos);
  VectorSet(hit->plane.normal,-1,0,0);
 }
}

int main(){return RunTests("SP vehicle Push uses native step/slide and keeps throttle, drift and gravity separate",[]{
 pmove_t move{};gentity_t hull{},pilot{};gclient_t client{};Vehicle_t vehicle{};vehicleInfo_t info{};
 auto& ps=client.ps;
 const auto reset=[&](decltype(info.type) type){
  move={};ps={};pml={};wall=100000;impacts=touches=0;
  pm=&move;move.ps=&ps;move.gent=&hull;move.trace=Trace;
  hull.client=&client;client.NPC_class=CLASS_VEHICLE;hull.m_pVehicle=&vehicle;
  hull.inuse=qtrue;hull.mass=80;floorContact=false;impactChange=ImpactChange::None;
  vehicle.m_pVehicleInfo=&info;vehicle.m_pPilot=&pilot;pilot.s.number=0;
  info.type=type;info.hoverHeight=(type==VH_SPEEDER||type==VH_FIGHTER)?32:0;info.maxSlope=.7f;
  ps.clientNum=MAX_CLIENTS;ps.gravity=800;ps.groundEntityNum=ENTITYNUM_NONE;ps.pm_type=PM_NORMAL;
  VectorSet(move.mins,-8,-8,-8);VectorSet(move.maxs,8,8,8);
  VectorClear(vehicle.jarPushDrift);move.cmd.forwardmove=127;vehicle.m_vOrientation[YAW]=45;
 };
 for(const auto type:{VH_SPEEDER,VH_WALKER,VH_ANIMAL,VH_FIGHTER}){
  for(const int rate:{20,40,125}){
   reset(type);pml.frametime=1.f/rate;
   ps.speed=300;VectorSet(ps.velocity,300,1000,0);VectorSet(vehicle.jarPushDrift,0,1000,0);
   for(int step=0;step<rate;++step){
    CHECK(PM_BeginVehiclePush(&vehicle)==&vehicle&&std::fabs(ps.velocity[1])<.001f);
    VectorSet(ps.velocity,300,0,0);PM_StepSlideMove(0);
    PM_MoveVehiclePush(vehicle);
   }
   CHECK(std::fabs(ps.origin[0]-300)<.01f&&ps.origin[1]>883&&ps.origin[1]<891);
   CHECK(std::fabs(vehicle.jarPushDrift[1]-1000*std::exp(-.25f))<.01f);
   CHECK(ps.speed==300&&move.cmd.forwardmove==127&&vehicle.m_vOrientation[YAW]==45&&vehicle.m_pPilot==&pilot);
  }
  reset(type);pml.frametime=.05f;
  VectorSet(ps.velocity,0,100,0);VectorSet(vehicle.jarPushDrift,0,100,0);
  CHECK(PM_BeginVehiclePush(&vehicle)==&vehicle);PM_StepSlideMove(1);
  const float fall=ps.origin[2];PM_MoveVehiclePush(vehicle);
  CHECK(ps.origin[2]==fall&&ps.velocity[2]==-40&&vehicle.jarPushDrift[2]==0);

  reset(type);wall=40;pml.frametime=.05f;
  VectorSet(ps.velocity,1900,200,0);VectorCopy(ps.velocity,vehicle.jarPushDrift);
  CHECK(PM_BeginVehiclePush(&vehicle)==&vehicle);PM_MoveVehiclePush(vehicle);
  CHECK(ps.origin[0]<=32.01f&&ps.origin[1]>9&&std::fabs(vehicle.jarPushDrift[0])<3&&impacts&&touches);
 }
 reset(VH_FIGHTER);vec3_t drift{},previous={0,100,0},result={50,175,250};
 VectorSet(ps.velocity,50,100,100);
 CHECK(G_VehiclePushDrift(ps,previous,result,true,drift)&&drift[0]==0&&drift[1]==175&&drift[2]==0);
 CHECK(G_VehiclePushDrift(ps,previous,result,false,drift)&&drift[0]==50&&drift[1]==175&&drift[2]==0);
 result[0]=NAN;CHECK(!G_VehiclePushDrift(ps,previous,result,false,drift));

 // Drive a real hull entity through the same drift, seat and movement code.
 auto& occupiedHull=g_entities[MAX_CLIENTS];
 auto& occupiedPilot=g_entities[0];
 auto& mountedDroid=g_entities[MAX_CLIENTS+1];
 gclient_t occupiedClient{},droidClient{};
 globals.num_entities=MAX_CLIENTS+2;
 const auto mount=[&](decltype(info.type) type){
  reset(type);
  occupiedHull={};occupiedPilot={};mountedDroid={};occupiedClient={};droidClient={};
  occupiedHull.s.number=MAX_CLIENTS;occupiedHull.inuse=qtrue;occupiedHull.health=100;occupiedHull.mass=80;
  occupiedHull.client=&client;occupiedHull.m_pVehicle=&vehicle;vehicle.m_pParentEntity=&occupiedHull;
  occupiedPilot.client=&occupiedClient;mountedDroid.client=&droidClient;
  mountedDroid.s.number=MAX_CLIENTS+1;
  for(auto* rider:{&occupiedPilot,&mountedDroid}){
   rider->inuse=qtrue;rider->health=100;rider->owner=&occupiedHull;rider->s.m_iVehicleNum=MAX_CLIENTS;
  }
  vehicle.m_pPilot=&occupiedPilot;vehicle.m_pDroidUnit=&mountedDroid;
  move.gent=&occupiedHull;
  ps.speed=300;ps.velocity[0]=300;
 };
 const vec3_t sideways={0,120,0},up={0,0,1200};
 for(const auto type:{VH_SPEEDER,VH_WALKER,VH_ANIMAL,VH_FIGHTER}){
  for(int rate:{20,40,125}){
   mount(type);pml.frametime=1.f/rate;
   for(int tick=0;tick<rate;++tick){
    CHECK(G_WorkshopVehicleThrust(occupiedHull,sideways,pml.frametime));
    CHECK(PM_BeginVehiclePush(&vehicle)==&vehicle&&std::fabs(ps.velocity[1])<.001f);
    PM_StepSlideMove(0);PM_MoveVehiclePush(vehicle);
   }
   CHECK(std::fabs(ps.origin[0]-300)<.01f&&ps.origin[1]>53&&ps.origin[1]<60);
   CHECK(vehicle.jarPushDrift[1]>105&&vehicle.jarPushDrift[1]<107&&ps.speed==300);
   CHECK(occupiedPilot.owner==&occupiedHull&&mountedDroid.owner==&occupiedHull&&
         vehicle.m_pPilot==&occupiedPilot&&vehicle.m_pDroidUnit==&mountedDroid&&vehicle.m_vOrientation[YAW]==45);
  }
  mount(type);pml.frametime=.05f;
  ps.groundEntityNum=occupiedHull.s.groundEntityNum=ENTITYNUM_WORLD;
  CHECK(G_WorkshopVehicleThrust(occupiedHull,up,.05f));
  CHECK(ps.velocity[2]==60&&ps.groundEntityNum==ENTITYNUM_NONE&&occupiedHull.s.groundEntityNum==ENTITYNUM_NONE&&
        (ps.pm_flags&PMF_TRIGGER_PUSHED)&&!VectorLengthSquared(vehicle.jarPushDrift));
  PM_StepSlideMove(1);
  CHECK(ps.origin[2]>1&&ps.velocity[2]==20);

  mount(type);pml.frametime=.05f;wall=40;
  const vec3_t intoWall={60000,4000,0};
  CHECK(G_WorkshopVehicleThrust(occupiedHull,intoWall,.05f));
  CHECK(PM_BeginVehiclePush(&vehicle)==&vehicle);PM_MoveVehiclePush(vehicle);
  CHECK(ps.origin[0]<=32.01f&&ps.origin[1]>0&&std::fabs(vehicle.jarPushDrift[0])<3&&impacts);
 }
 mount(VH_FIGHTER);
 const vec3_t initial={300,0,0};
 vehicle.m_iBoarding=1;CHECK(!G_WorkshopVehicleThrust(occupiedHull,sideways,.05f));vehicle.m_iBoarding=0;
 blockedThrustMember=0;CHECK(!G_WorkshopVehicleThrust(occupiedHull,sideways,.05f));blockedThrustMember=ENTITYNUM_NONE;
 droidClient.ps.eFlags|=EF_FORCE_GRIPPED;CHECK(!G_WorkshopVehicleThrust(occupiedHull,sideways,.05f));droidClient.ps.eFlags=0;
 occupiedPilot.s.m_iVehicleNum=0;CHECK(!G_WorkshopVehicleThrust(occupiedHull,sideways,.05f));occupiedPilot.s.m_iVehicleNum=MAX_CLIENTS;
 CHECK(VectorCompare(ps.velocity,initial)&&!VectorLengthSquared(vehicle.jarPushDrift));
 vehicle.m_pPilot=vehicle.m_pDroidUnit=nullptr;
 const vec3_t reverse={0,-120,0};
 CHECK(G_WorkshopVehicleThrust(occupiedHull,reverse,.05f)&&ps.velocity[1]==-6&&vehicle.jarPushDrift[1]==-6);

 // A downward Physgun release reaches the floor through the first native
 // slide. The impact callback frees the mover before step recovery.
 mount(VH_SPEEDER);floorContact=true;impactChange=ImpactChange::Remove;
 pml.frametime=.05f;VectorSet(ps.origin,0,0,32);VectorSet(ps.velocity,0,0,-1024);
 PM_StepSlideMove(1);
 CHECK(impacts==1&&touches==1&&move.moverRemoved&&!occupiedHull.inuse&&!move.ps);

 // The lateral drift pass can hit a wall after native steering has finished.
 // Reusing the same slot and the same backing pointers must leave the new
 // controller and its velocity untouched.
 mount(VH_SPEEDER);wall=40;impactChange=ImpactChange::Reuse;
 pml.frametime=.05f;
 VectorSet(vehicle.jarPushDrift,1400,0,0);
 PM_MoveVehiclePush(vehicle);
 const vec3_t replacementVelocity={10,20,30},replacementDrift={40,50,0};
 CHECK(impacts==1&&touches==1&&move.moverRemoved&&occupiedHull.inuse&&
       VectorCompare(ps.velocity,replacementVelocity)&&VectorCompare(vehicle.jarPushDrift,replacementDrift));
});}
