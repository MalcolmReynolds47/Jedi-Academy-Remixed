// Production native pusher and additive-force controller against bounded world services.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include "game/g_mover_motion.h"
#include "game/g_teleport_mover_motion.h"
#include "game/g_angular_cargo.h"
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected model allocation");}
static gclient_t clients[2];static int damageCalls=0,touchCalls=0,blockedActor=-1;static float wall=1e6f,maxSweep=0;
static bool permit=true,sectionExclusion=false;static std::vector<gentity_t*> candidates;
static bool cargoCommitBlocked=false,blockOnlyDestination=false,blockRiderSweep=false;
static int cargoPlans=0;
bool G_AngularCargoPlan(gentity_t&,const vec3_t,const vec3_t,AngularCargo&){++cargoPlans;return true;}
bool G_AngularCargoCommit(gentity_t&,const AngularCargo&){return !cargoCommitBlocked;}
void G_SandboxChargesRefresh(const std::vector<SandboxChargeMove>& moves){CHECK(moves.empty());}
bool G_SubBSPMembers(const gentity_t*e,std::vector<gentity_t*>&members){CHECK(e==&g_entities[2]);members={&g_entities[3]};return true;}
gentity_t* G_SubBSPOwner(const gentity_t*){return nullptr;}
qboolean Q3_TaskIDPending(gentity_t*,taskID_t){return qfalse;}
bool G_BobbingRead(const gentity_t&,jarBobState_t&){throw std::runtime_error("Expected authored brush");}
bool G_SandboxSlideRead(const gentity_t&,jar::slidingDoor::State&){throw std::runtime_error("Expected authored brush");}
void G_Error(const char*,...){throw std::runtime_error("Native mover error");}
int G_ModelIndex(const char* model){CHECK(!strcmp(model,"models/map_objects/imperial/crate1.md3"));return 5;}
char* G_NewString(const char* model){CHECK(!strcmp(model,jar::sandboxAngular::model));return const_cast<char*>(jar::sandboxAngular::model);}
void G_FreeEntity(gentity_t*e){G_JAR_MoverMotionForget(e);e->inuse=qfalse;}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){++damageCalls;}
void GEntity_TouchFunc(gentity_t*,gentity_t*,trace_t*){++touchCalls;}
qboolean G_OkayToRemoveCorpse(gentity_t*){return qtrue;}
qboolean G_BoundsOverlap(const vec3_t a,const vec3_t b,const vec3_t c,const vec3_t d){for(int i=0;i<3;++i)if(a[i]>=d[i]||b[i]<=c[i])return qfalse;return qtrue;}
#include "game/g_mover_push.inc"
#include "game/g_sandbox_angular_motion.inc"
#include "game/g_mover_motion.inc"
#include "game/g_bsp_section_bob.inc"
gentity_t* G_BSPSectionBreakableRoot(const gentity_t& child){
 return &child==&g_entities[3]&&child.classname&&!strcmp(child.classname,"func_breakable")?&g_entities[2]:nullptr;
}
#include "game/g_bsp_section_motion.inc"
void EvaluateTrajectory(const trajectory_t*t,int now,vec3_t p){if(jar::angular::Evaluate(*t,now,p))return;CHECK(now==t->trTime||t->trType==TR_STATIONARY);VectorCopy(t->trBase,p);}
void G_PlayDoorLoopSound(gentity_t*){}
void G_PlayDoorSound(gentity_t*,int){}
int BMS_START=0,BMS_END=2;
void G_UseTargets(gentity_t*,gentity_t*){}
#include "game/g_mover_train.inc"
#include "game/g_mover_func_bobbing_use.inc"
bool G_RailCanMove(const gentity_t&,const vec3_t){return true;}
void GEntity_ReachedFunc(gentity_t*){}
void GEntity_BlockedFunc(gentity_t*,gentity_t*){}
#include "game/g_mover_team.inc"
static bool allowed(const gentity_t&){return permit;}
static gentity_t&reset(){
 G_JAR_MoverMotionReset();candidates.clear();permit=true;wall=1e6f;maxSweep=0;blockedActor=-1;blockOnlyDestination=blockRiderSweep=false;damageCalls=touchCalls=0;level.time=1000;
 auto&e=g_entities[2];e.inuse=qtrue;e.client=nullptr;e.s.number=2;e.classname=const_cast<char*>("func_train");e.s.eType=ET_MOVER;e.bmodel=qtrue;e.contents=CONTENTS_SOLID;e.e_ThinkFunc=thinkF_NULL;e.nextthink=1200;e.next_roff_time=0;e.damage=40;e.spawnflags=0;
 e.model=nullptr;e.e_UseFunc=useF_NULL;
 VectorClear(e.currentOrigin);VectorClear(e.currentAngles);VectorSet(e.mins,-8,-8,-8);VectorSet(e.maxs,8,8,8);e.s.apos={};e.s.pos={};e.s.pos.trType=TR_LINEAR_STOP;e.s.pos.trTime=500;e.s.pos.trDuration=1000;e.s.pos.trDelta[0]=40;VectorClear(e.s.origin);VectorClear(e.pos1);VectorSet(e.pos2,40,0,0);gi.linkentity(&e);return e;
}
static gentity_t&rider(int id,float x,float y,float z,bool riding){auto&e=g_entities[id];e.inuse=qtrue;e.s.number=id;e.s.eType=ET_PLAYER;e.client=&clients[id-10];*e.client={};e.health=100;e.clipmask=MASK_SOLID|CONTENTS_BODY;VectorSet(e.currentOrigin,x,y,z);VectorCopy(e.currentOrigin,e.s.pos.trBase);VectorCopy(e.currentOrigin,e.client->ps.origin);VectorSet(e.mins,-2,-2,-4);VectorSet(e.maxs,2,2,4);e.s.groundEntityNum=riding?2:ENTITYNUM_NONE;e.s.eFlags=0;gi.linkentity(&e);candidates.push_back(&e);return e;}
int main(){return RunTests("native mover forces, rider rollback, controller continuity and save chunks",[]{
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;VectorAdd(e->currentOrigin,e->mins,e->absmin);VectorAdd(e->currentOrigin,e->maxs,e->absmax);};gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};
 gi.EntitiesInBox=[](const vec3_t,const vec3_t,gentity_t**out,int cap){if(sectionExclusion)CHECK(!g_entities[3].linked);CHECK(int(candidates.size())<cap);int n=0;for(auto*e:candidates)if(e->inuse&&e->linked)out[n++]=e;return n;};
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t lo,const vec3_t hi,const vec3_t end,int pass,int mask,EG2_Collision,int){*t={};t->fraction=1;VectorCopy(end,t->endpos);
  if(pass==2){if(sectionExclusion)CHECK(!g_entities[3].linked&&(mask&CONTENTS_BODY));maxSweep=std::max(maxSweep,Distance(start,end));if(start[0]+hi[0]>wall+.001f){t->startsolid=qtrue;return;}if(end[0]+hi[0]>wall&&end[0]>start[0]){t->fraction=std::max(0.f,(wall-hi[0]-start[0])/(end[0]-start[0]));for(int a=0;a<3;++a)t->endpos[a]=start[a]+(end[a]-start[a])*t->fraction;}return;}
  if(pass==10&&blockRiderSweep&&!VectorCompare(start,end)){CHECK(!g_entities[2].linked);t->fraction=.5f;return;}
  vec3_t a,b;VectorAdd(end,lo,a);VectorAdd(end,hi,b);auto&m=g_entities[2];const bool inside=G_BoundsOverlap(a,b,m.absmin,m.absmax)!=qfalse;
  if(inside||(pass==blockedActor&&(!blockOnlyDestination||end[0]>0))){t->startsolid=qtrue;t->entityNum=inside?2:ENTITYNUM_WORLD;}
 };
 // Force is independent of the native path's own velocity and clock.
 for(float dt:{.05f,1.f/60,1.f/120}){auto&e=reset();const int frames=int(std::lround(1.f/dt));vec3_t accel={100,0,0};
  for(int i=0;i<frames;++i){G_JAR_MoverForce(e,accel);G_JAR_MoverMotionFrame(dt,allowed);}
  CHECK(std::fabs(e.currentOrigin[0]-50)<.002f&&std::fabs(e.s.pos.trBase[0]-50)<.002f&&e.s.pos.trTime==500&&e.s.pos.trDuration==1000&&e.s.pos.trDelta[0]==40&&e.nextthink==1200&&e.s.pos.trType==TR_LINEAR_STOP);
  CHECK(std::fabs(e.pos1[0]-50)<.002f&&std::fabs(e.pos2[0]-90)<.002f);vec3_t offset;G_JAR_MoverPathOffset(e,offset);CHECK(std::fabs(offset[0]-50)<.002f);
  // Run the actual arrival/state callbacks: a new leg must not snap back
  // to the map-authored path after force displaced its reference frame.
  auto&corner=g_entities[30];auto&next=g_entities[31];corner.nextTrain=&next;corner.speed=40;corner.wait=0;corner.spawnflags=0;VectorSet(corner.s.origin,40,0,0);VectorSet(next.s.origin,80,0,0);e.nextTrain=&corner;Reached_Train(&e);CHECK(std::fabs(e.currentOrigin[0]-90)<.002f&&std::fabs(e.s.pos.trBase[0]-90)<.002f&&std::fabs(e.pos2[0]-130)<.002f&&e.s.pos.trDuration==1000&&e.nextTrain==&next);
  e.nextTrain=&corner;corner.wait=2;Reached_Train(&e);CHECK(e.nextthink==level.time+2000&&e.e_ThinkFunc==thinkF_Think_BeginMoving&&e.s.pos.trType==TR_STATIONARY);
 }
 {
  auto& train=reset();globals.num_entities=32;
  train.model=const_cast<char*>("*2");train.s.modelindex=2;train.e_ReachedFunc=reachedF_Reached_Train;
  auto& corner=g_entities[30];auto& next=g_entities[31];
  corner.inuse=next.inuse=qtrue;corner.s.number=30;next.s.number=31;
  corner.classname=next.classname=const_cast<char*>("path_corner");
  corner.wait=0;corner.nextTrain=&next;train.nextTrain=&corner;
  VectorSet(train.currentOrigin,20,0,0);VectorSet(train.s.origin,0,0,0);
  const vec3_t landing={600,0,30};
  TeleportMoverMotion plan;
  CHECK(G_TeleportMoverKind(train)==TeleportMoverType::Train&&G_TeleportMoverReady(train,TeleportMoverType::Train));
  CHECK(G_PlanTeleportMover(train,TeleportMoverType::Train,landing,true,plan));
  G_CommitTeleportMover(train,landing,plan);
  CHECK(train.currentOrigin[0]==600&&train.s.pos.trBase[0]==580&&train.s.pos.trTime==500&&
        train.s.pos.trDuration==1000&&train.nextTrain==&corner&&train.s.pos.trDelta[0]==40);
  MemorySave routeSave;ojk::SavedGameHelper routeIO(&routeSave);
  G_JAR_MoverMotionExport(false,routeIO);G_JAR_MoverMotionReset();routeSave.reset_buffer_offset();
  CHECK(G_JAR_MoverMotionImport(false,routeIO));routeIO.ensure_all_data_read();
  Reached_Train(&train);
  CHECK(train.currentOrigin[0]==620&&train.pos2[0]==660&&train.currentOrigin[2]==30&&train.nextTrain==&next&&
        corner.s.origin[0]==40&&next.s.origin[0]==80);
  auto& bob=reset();bob.classname=const_cast<char*>("func_bobbing");
  bob.s.pos.trType=TR_SINE;bob.s.pos.trDuration=1000;bob.s.pos.trTime=750;bob.s.pos.trDelta[2]=20;
  VectorSet(bob.currentOrigin,0,0,20);
  CHECK(G_PlanTeleportMover(bob,TeleportMoverType::Brush,landing,true,plan));
  G_CommitTeleportMover(bob,landing,plan);
  func_bobbing_use(&bob,nullptr,nullptr);
  CHECK(bob.s.pos.trType==TR_INTERPOLATE&&bob.s.pos.trBase[2]==30&&bob.radius==.25f);
  func_bobbing_use(&bob,nullptr,nullptr);
  CHECK(bob.s.pos.trType==TR_SINE&&bob.s.pos.trBase[0]==600&&bob.s.pos.trBase[2]==10&&bob.s.pos.trTime==750);
 }
 auto&e=reset();auto&p=rider(10,0,0,12,true);vec3_t delta={4,0,0},actual;
 CHECK(G_JAR_MoverTranslate(e,delta,actual)&&e.currentOrigin[0]==4&&p.currentOrigin[0]==4&&p.client->ps.origin[0]==4&&p.s.pos.trBase[0]==4&&!damageCalls&&!touchCalls);
 reset();auto&first=rider(10,10,-3,0,false);auto&second=rider(11,10,3,0,false);blockedActor=11;
 CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&e.currentOrigin[0]==0&&first.currentOrigin[0]==10&&first.client->ps.origin[0]==10&&first.s.pos.trBase[0]==10&&second.currentOrigin[0]==10&&!damageCalls&&!touchCalls);
 // The original native path still has its crushing callback behavior.
 pushed_p=pushed;gentity_t*obstacle=nullptr;vec3_t zero={0,0,0};CHECK(!G_MoverPush(&e,delta,zero,&obstacle)&&damageCalls>0);
 // Exact Context placement refuses partial pushes and never drops a blocked rider.
 reset();auto& exactRider=rider(10,0,0,12,false);exactRider.client->ps.groundEntityNum=e.s.number;
 wall=10;
 CHECK(!G_JAR_MoverRotateAt(e,zero,delta,nullptr)&&e.currentOrigin[0]==0&&exactRider.currentOrigin[0]==0&&e.pos2[0]==40);
 wall=1e6f;blockedActor=10;blockOnlyDestination=true;
 CHECK(!G_JAR_MoverRotateAt(e,zero,delta,nullptr)&&e.currentOrigin[0]==0&&exactRider.currentOrigin[0]==0&&e.pos2[0]==40);
 CHECK(exactRider.client->ps.groundEntityNum==e.s.number&&!damageCalls&&!touchCalls);
 blockedActor=-1;
 blockRiderSweep=true;
 CHECK(!G_JAR_MoverRotateAt(e,zero,delta,nullptr)&&e.linked&&e.currentOrigin[0]==0&&exactRider.currentOrigin[0]==0);
 blockRiderSweep=false;
 CHECK(G_JAR_MoverRotateAt(e,zero,delta,nullptr)&&e.currentOrigin[0]==4&&exactRider.currentOrigin[0]==4&&e.pos2[0]==44);
 for(const bool lift:{true,false}){
  reset();e.bmodel=qfalse;e.s.eType=ET_GENERAL;
  e.classname=lift?"func_plat":"func_bobbing";e.model=const_cast<char*>(lift?JAR_LIFT_PORTABLE:JAR_BOB_PORTABLE);
  G_JAR_ProjectedDeckHull(e,zero,e.mins,e.maxs);gi.linkentity(&e);
  const float top=e.maxs[2];
  auto& standing=rider(10,4,0,top+4.125f,false);standing.client->ps.groundEntityNum=e.s.number;
  const int planned=cargoPlans;
  CHECK(G_JAR_MoverTranslate(e,delta,actual)&&actual[0]==4&&standing.currentOrigin[0]==8&&standing.currentOrigin[2]==top+4.125f);
  const vec3_t turn={0,90,0};
  blockRiderSweep=true;
  CHECK(!G_JAR_MoverRotate(e,turn)&&e.linked&&e.currentAngles[YAW]==0&&standing.currentOrigin[0]==8);
  blockRiderSweep=false;
  cargoCommitBlocked=true;
  CHECK(!G_JAR_MoverRotate(e,turn)&&e.currentAngles[YAW]==0&&standing.currentOrigin[0]==8&&standing.currentOrigin[1]==0);
  cargoCommitBlocked=false;
  CHECK(G_JAR_MoverRotate(e,turn)&&e.currentAngles[YAW]==90&&std::fabs(standing.currentOrigin[0]-4)<.001f&&std::fabs(standing.currentOrigin[1]-4)<.001f);
  CHECK(cargoPlans==planned+4&&!damageCalls&&!touchCalls);
 }
 reset();wall=10;vec3_t accel={1600,0,0};G_JAR_MoverForce(e,accel);G_JAR_MoverMotionFrame(.1f,allowed);CHECK(std::fabs(e.currentOrigin[0]-2)<.001f&&moverMotion[2].velocity[0]==0);const float stopped=e.currentOrigin[0];G_JAR_MoverMotionFrame(.1f,allowed);CHECK(e.currentOrigin[0]==stopped);
 reset();vec3_t huge={3200000,0,0};G_JAR_MoverForce(e,huge);G_JAR_MoverMotionFrame(.1f,allowed);CHECK(moverMotion[2].velocity[0]<=2000&&maxSweep<=8.001f);
 permit=false;G_JAR_MoverMotionFrame(.05f,allowed);CHECK(moverMotion[2].velocity[0]==0);permit=true;const float frozen=e.currentOrigin[0];G_JAR_MoverMotionFrame(.05f,allowed);CHECK(e.currentOrigin[0]==frozen);
 // Scripted models keep their thinker/timer. ROFF pos2 is an angle accumulator.
 reset();e.bmodel=qfalse;e.s.eType=ET_GENERAL;e.e_ThinkFunc=thinkF_fx_runner_think;e.next_roff_time=1100;VectorSet(e.pos2,10,20,30);G_JAR_MoverForce(e,accel);G_JAR_MoverMotionFrame(.05f,allowed);
 CHECK(e.e_ThinkFunc==thinkF_fx_runner_think&&e.nextthink==1200&&e.next_roff_time==1100&&e.pos1[0]>1.9f&&e.pos2[0]==10&&e.pos2[1]==20&&e.pos2[2]==30);
 MemorySave save;ojk::SavedGameHelper h(&save);G_JAR_MoverMotionExport(false,h);save.reset_buffer_offset();G_JAR_MoverMotionReset();CHECK(G_JAR_MoverMotionImport(false,h));h.ensure_all_data_read();CHECK(moverMotion.size()==1&&std::fabs(moverMotion[2].velocity[0]-80)<.001f&&std::fabs(moverMotion[2].offset[0]-2)<.001f);
 MemorySave automatic;ojk::SavedGameHelper ah(&automatic);G_JAR_MoverMotionExport(true,ah);automatic.reset_buffer_offset();CHECK(G_JAR_MoverMotionImport(true,ah)&&!G_JAR_MoverMotionPresent());ah.ensure_all_data_read();
 G_JAR_MoverForce(e,accel);G_JAR_MoverMotionFrame(.05f,allowed);MemorySave bad;ojk::SavedGameHelper bh(&bad);bh.write<uint32_t>(1);bh.write<int32_t>(2);vec3_t nan={std::numeric_limits<float>::quiet_NaN(),0,0};bh.write<float>(nan,3);bh.write<float>(zero,3);bh.write<float>(zero,3);bad.reset_buffer_offset();CHECK(!G_JAR_MoverMotionImport(false,bh)&&moverMotion.size()==1);
 G_JAR_MoverMotionForget(&e);e.inuse=qfalse;CHECK(!G_JAR_MoverMotionPresent());e.inuse=qtrue;G_JAR_MoverMotionFrame(.05f,allowed);CHECK(!G_JAR_MoverMotionActive(e));
 // Angular force uses radians/second, advances independently of native clocks,
 // coasts when the force is removed, and stops when controlled/disabled.
 for(float dt:{.05f,1.f/60,1.f/120}){reset();e.s.apos.trType=TR_LINEAR;e.s.apos.trTime=500;e.s.apos.trDelta[1]=30;vec3_t angularForce={0,0,1};
  for(int i=0;i<int(std::lround(1.f/dt));++i){G_JAR_MoverTorque(e,angularForce);G_JAR_MoverMotionFrame(dt,allowed);}
  CHECK(std::fabs(e.currentAngles[YAW]-28.64789f)<.003f&&std::fabs(moverMotion[2].angular[2]-1)<.001f&&e.s.apos.trTime==500&&e.s.apos.trDelta[1]==30);
  CHECK(std::fabs(AngleSubtract(e.s.apos.trBase[YAW],e.currentAngles[YAW]))<.0001f);
  G_JAR_MoverMotionFrame(.05f,allowed);CHECK(e.currentAngles[YAW]>31.5f);permit=false;G_JAR_MoverMotionFrame(.05f,allowed);CHECK(VectorLength(moverMotion[2].angular)==0);
 }
 reset();auto&rotatingRider=rider(10,4,0,12,true);vec3_t yaw={0,30,0};CHECK(G_JAR_MoverRotate(e,yaw));
 CHECK(std::fabs(rotatingRider.currentOrigin[0]-3.4641f)<.001f&&std::fabs(rotatingRider.currentOrigin[1]-2)<.001f&&rotatingRider.client->ps.delta_angles[YAW]!=0&&!damageCalls&&!touchCalls);
 // A pitch singularity is a small physical rotation, even though equivalent
 // Euler yaw/roll jump by 180 degrees. Rider travel must remain small.
 reset();VectorSet(e.currentAngles,89,0,0);rider(10,4,0,12,true);const auto crossed=jar::physics::AdvanceAngles({89,0,0},{0,1,0},.035f);vec3_t crossing={crossed.x,crossed.y,crossed.z};
 CHECK(G_JAR_MoverRotate(e,crossing)&&Distance(rotatingRider.currentOrigin,vec3_t{4,0,12})<.5f);
 reset();wall=8.1f;CHECK(!G_JAR_MoverRotate(e,yaw)&&e.currentAngles[YAW]==0);
 reset();rider(10,4,-3,12,true);rider(11,4,3,12,true);blockedActor=11;
 CHECK(!G_JAR_MoverRotate(e,yaw)&&e.currentAngles[YAW]==0&&clients[0].ps.delta_angles[YAW]==0&&clients[1].ps.delta_angles[YAW]==0&&!damageCalls&&!touchCalls);
 reset();e.next_roff_time=1100;VectorSet(e.pos1,1,2,3);VectorSet(e.pos2,10,20,30);CHECK(G_JAR_MoverRotate(e,yaw)&&e.pos1[0]==1&&e.pos2[YAW]==50&&e.next_roff_time==1100);
 // A rider directly above the contact keeps its horizontal position while the mover turns.
 const vec3_t pivot={4,0,0},aroundGrip={4-3.4641016f,-2,0},riderAboveGrip={4,0,12};
 reset();rider(10,4,0,12,true);e.next_roff_time=1100;VectorSet(e.pos1,1,2,3);e.pos2[YAW]=20;
 CHECK(G_JAR_MoverRotateAt(e,yaw,aroundGrip,pivot)&&Distance(e.currentOrigin,aroundGrip)<.001f&&
       Distance(rotatingRider.currentOrigin,riderAboveGrip)<.001f&&e.currentAngles[YAW]==30&&
       std::fabs(e.pos1[0]-1-aroundGrip[0])<.001f&&e.pos1[1]==0&&e.pos2[YAW]==50);
 reset();rider(10,4,0,12,true);blockedActor=10;
 CHECK(!G_JAR_MoverRotateAt(e,yaw,aroundGrip,pivot)&&VectorCompare(e.currentOrigin,zero)&&
       VectorCompare(e.currentAngles,zero)&&VectorCompare(rotatingRider.currentOrigin,riderAboveGrip)&&!damageCalls&&!touchCalls);
 {
  using namespace jar::sandboxAngular;
  const auto portable=[&](const char* name,Kind kind){
   reset();e.classname=const_cast<char*>(name);e.model=const_cast<char*>(model);e.s.pos={};
   CHECK(G_SandboxAngularInit(e,Defaults(kind)));gi.linkentity(&e);
  };
  portable("func_rotating",Spinner);
  const float originalWidth=e.maxs[0]-e.mins[0];
  G_SandboxAngularUse(e);G_SandboxAngularFrame(e,100);
  CHECK(std::fabs(e.currentAngles[YAW]-10)<.002f&&e.maxs[0]-e.mins[0]>originalWidth&&e.s.apos.trType==TR_AXIS_INTERPOLATE);
  Settings settings;CHECK(G_SandboxAngularRead(e,settings));settings.rate=-100;
  CHECK(G_SandboxAngularSet(e,settings));G_SandboxAngularFrame(e,100);CHECK(std::fabs(e.currentAngles[YAW])<.002f);
  portable("func_pendulum",Pendulum);
  auto& supported=rider(10,4,0,e.maxs[2]+4.125f,true);supported.client->ps.groundEntityNum=e.s.number;
  G_SandboxAngularFrame(e,100);
  CHECK(e.count==100&&e.currentAngles[ROLL]>4&&std::fabs(supported.currentOrigin[2]-e.maxs[2]-4.125f)<.001f);
  vec3_t at,oldHull,riderAt;VectorCopy(e.currentAngles,at);VectorCopy(e.maxs,oldHull);VectorCopy(supported.currentOrigin,riderAt);
  cargoCommitBlocked=true;G_SandboxAngularFrame(e,100);cargoCommitBlocked=false;
  CHECK(e.count==100&&VectorCompare(e.currentAngles,at)&&VectorCompare(supported.currentOrigin,riderAt));
  blockedActor=10;G_SandboxAngularFrame(e,100);
  CHECK(e.count==100&&VectorCompare(e.currentAngles,at)&&Distance(e.maxs,oldHull)<.001f&&VectorCompare(supported.currentOrigin,riderAt)&&
        (e.s.eFlags&EF_BLOCKED_MOVER)&&!damageCalls&&!touchCalls);
  blockedActor=-1;wall=0;G_SandboxAngularFrame(e,100);CHECK(e.count==100&&VectorCompare(e.currentAngles,at));
  wall=1e6f;G_SandboxAngularUse(e);G_SandboxAngularFrame(e,100);CHECK(e.count==100&&VectorCompare(e.currentAngles,at));
  G_SandboxAngularUse(e);G_SandboxAngularFrame(e,100);CHECK(e.count==200&&e.currentAngles[ROLL]>at[ROLL]);
  MemorySave saved;ojk::SavedGameHelper io(&saved);gentity_t loaded{};
  e.sg_export(io);saved.reset_buffer_offset();loaded.sg_import(io);io.ensure_all_data_read();
  loaded.classname=e.classname;loaded.model=e.model;
  CHECK(G_SandboxAngularRestore(loaded,loaded.s.apos,level.time)&&G_SandboxAngularRead(loaded,settings)&&settings.phase==200&&settings.running);
  const vec3_t exit={300,20,100};TeleportMoverMotion plan;
  CHECK(G_TeleportMoverKind(e)==TeleportMoverType::Angular&&G_PlanTeleportMover(e,TeleportMoverType::Angular,exit,true,plan));
  G_CommitTeleportMover(e,exit,plan);CHECK(VectorCompare(e.currentOrigin,exit)&&e.count==200&&e.s.apos.trType==TR_AXIS_INTERPOLATE);
  reset();e.classname=const_cast<char*>("func_pendulum");e.model=const_cast<char*>("@jar_box");e.bmodel=qfalse;
  e.s.apos.trType=TR_SINE;e.s.apos.trDuration=1200;e.s.apos.trTime=800;e.s.apos.trDelta[ROLL]=15;
  gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
  CHECK(G_SandboxAngularRestore(e,e.s.apos,1000)&&G_SandboxAngularRead(e,settings)&&settings.phase==200&&settings.period==1200);
 }
 // V30's linear-only record remains readable; V31 round-trips angular motion.
 MemorySave legacy;ojk::SavedGameHelper lh(&legacy);lh.write<uint32_t>(1);lh.write<int32_t>(2);lh.write<float>(zero,3);lh.write<float>(delta,3);legacy.reset_buffer_offset();CHECK(G_JAR_MoverMotionImport(false,lh,false));lh.ensure_all_data_read();CHECK(moverMotion[2].offset[0]==4&&VectorLength(moverMotion[2].angular)==0);
 vec3_t spin={0,0,2};G_JAR_MoverSpin(e,spin);MemorySave angular;ojk::SavedGameHelper sh(&angular);G_JAR_MoverMotionExport(false,sh);angular.reset_buffer_offset();G_JAR_MoverMotionReset();CHECK(G_JAR_MoverMotionImport(false,sh));sh.ensure_all_data_read();CHECK(moverMotion[2].angular[2]==2);
 JarMoverMotion invalid;CHECK(G_JAR_MoverMotionRead(e,invalid));invalid.angular[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!G_JAR_MoverMotionRestore(e,invalid)&&moverMotion[2].angular[2]==2);
 // Linear and angular contacts stop independently; a blocked linear sweep
 // must not truncate the remaining angular integration for this frame.
 reset();e.bmodel=qfalse;wall=8;vec3_t angularForce={0,0,1};G_JAR_MoverForce(e,accel);G_JAR_MoverTorque(e,angularForce);G_JAR_MoverMotionFrame(.1f,allowed);
 CHECK(e.currentOrigin[0]==0&&std::fabs(e.currentAngles[YAW]-.286479f)<.001f);
 reset();VectorSet(e.mins,-1000,-1000,-1000);VectorSet(e.maxs,1000,1000,1000);vec3_t strongSpin={0,0,1e8f};G_JAR_MoverTorque(e,strongSpin);G_JAR_MoverMotionFrame(.1f,allowed);CHECK(VectorLength(moverMotion[2].angular)*RadiusFromBounds(e.mins,e.maxs)<=2000.01f);
 // Native team movement uses the actual orientation change across a gimbal
 // crossing. Riders must follow that rotation without a spurious yaw flip.
 reset();e.s.pos.trType=TR_STATIONARY;e.s.apos.trType=TR_AXIS_LINEAR;e.s.apos.trTime=1000;VectorSet(e.s.apos.trBase,89,0,0);VectorSet(e.s.apos.trDelta,0,40,0);VectorCopy(e.s.apos.trBase,e.currentAngles);e.teamchain=nullptr;e.e_BlockedFunc=blockedF_NULL;
 auto&teamRider=rider(10,4,0,16,true);const vec3_t riderStart={4,0,16};const int yawBefore=teamRider.client->ps.delta_angles[YAW];level.previousTime=1000;level.time=1050;G_MoverTeam(&e);vec3_t expectedAngles;jar::angular::Evaluate(e.s.apos,level.time,expectedAngles);float rotation[3][3];jar::angular::DeltaMatrix(e.s.apos.trBase,expectedAngles,rotation);vec3_t expectedRider;for(int a=0;a<3;++a)expectedRider[a]=DotProduct(riderStart,rotation[a]);CHECK(Distance(teamRider.currentOrigin,expectedRider)<.0001f&&teamRider.client->ps.delta_angles[YAW]==yawBefore);
 // Blocked team motion pauses the angular clock and restores rider state.
 const auto path=e.s.apos;vec3_t beforeRider,beforeAngles;VectorCopy(teamRider.currentOrigin,beforeRider);VectorCopy(e.currentAngles,beforeAngles);blockedActor=10;level.previousTime=1050;level.time=1100;G_MoverTeam(&e);CHECK(e.s.apos.trTime==path.trTime+50&&Distance(teamRider.currentOrigin,beforeRider)<.0001f&&Distance(e.currentAngles,beforeAngles)<.001f&&(e.s.eFlags&EF_BLOCKED_MOVER));
 // A portable section excludes its own brush during both the clearance trace
 // and the native pusher, but rejects motion with unsafe children or riders.
 reset();e.classname="misc_bsp";e.model="#mp/duel1";e.e_UseFunc=useF_target_teleporter_use;
 VectorSet(e.mins,-100,-100,-100);VectorSet(e.maxs,100,100,100);VectorClear(e.pos1);VectorClear(e.pos2);gi.linkentity(&e);
 auto&bob=g_entities[3];bob.inuse=bob.bmodel=qtrue;bob.s.number=3;bob.s.eType=ET_MOVER;bob.classname="func_bobbing";bob.model="*1";bob.contents=CONTENTS_SOLID;
 bob.s.pos.trType=TR_SINE;bob.s.pos.trDuration=3000;VectorSet(bob.s.pos.trDelta,0,0,20);VectorSet(bob.mins,-16,-16,-8);VectorSet(bob.maxs,16,16,8);gi.linkentity(&bob);
 globals.num_entities=12;g_entities[10].inuse=g_entities[11].inuse=qfalse;sectionExclusion=true;
 CHECK(G_JAR_MoverTranslate(e,delta,actual)&&e.currentOrigin[0]==4&&bob.linked);
 CHECK(G_JAR_MoverRotate(e,yaw)&&bob.linked&&e.currentAngles[YAW]==30);
 wall=0;CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&!G_JAR_MoverRotate(e,zero)&&bob.linked&&e.currentOrigin[0]==4&&e.currentAngles[YAW]==30);
 wall=1e6f;auto&childRider=rider(10,0,0,20,false);childRider.client->ps.groundEntityNum=bob.s.number;
 CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&!G_JAR_MoverRotate(e,zero)&&bob.linked&&childRider.currentOrigin[0]==0);
 childRider.inuse=qfalse;bob.s.eFlags|=EF_FORCE_GRIPPED;CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&bob.linked);bob.s.eFlags=0;
 bob.s.pos.trDelta[2]=95;CHECK(!G_JAR_MoverRotate(e,zero)&&bob.linked);bob.s.pos.trDelta[2]=20;
 G_JAR_MoverForce(bob,accel);CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&bob.linked);G_JAR_MoverMotionForget(&bob);
 gi.unlinkentity(&bob);sectionExclusion=false;CHECK(G_JAR_MoverTranslate(e,delta,actual)&&!bob.linked);

 // Repeat the clearance and native pusher checks with a stationary breakable.
 e.model="#mp/duel8";VectorCopy(e.currentOrigin,e.pos1);VectorCopy(e.currentAngles,e.pos2);
 auto&block=bob;block.classname="func_breakable";block.e_DieFunc=dieF_funcBBrushDie;
 block.s.pos={};block.s.apos={};gi.linkentity(&block);sectionExclusion=true;
 CHECK(G_JAR_MoverTranslate(e,delta,actual)&&block.linked);
 CHECK(G_JAR_MoverRotate(e,zero)&&block.linked);
 childRider.inuse=qtrue;
 CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&!G_JAR_MoverRotate(e,yaw)&&block.linked);
 childRider.inuse=qfalse;
 block.currentOrigin[0]=200;
 CHECK(!G_JAR_MoverTranslate(e,delta,actual)&&block.linked);
 block.contents=0;gi.unlinkentity(&block);sectionExclusion=false;
 CHECK(G_JAR_MoverTranslate(e,delta,actual)&&!block.linked);
 });}
