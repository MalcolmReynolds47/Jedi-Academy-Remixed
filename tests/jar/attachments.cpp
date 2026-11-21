// Production native/mixed attachment forces and creation against engine services.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_workshop_vehicle_thruster.h"
#include "game/jar_physics.h"
#include "qcommon/jar_workshop_state.h"
#include "qcommon/jar_duplicate.h"
#include "test_support.h"
#include "bsp_section_motion_stub.h"
#include <map>
#include <string>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
using namespace jar::workshop;
State state;jar::physics::World physicsWorld;
std::map<unsigned,int> nativeControls;
unsigned entityGeneration[MAX_GENTITIES]{};
uint64_t G_EntityLifetime(const gentity_t* e){return e&&e->inuse?uint64_t(entityGeneration[e->s.number])+1:0;}
bool G_JAR_WorkshopControls(const gentity_t* e){return e&&nativeControls.count(e->s.number)!=0;}
bool G_JAR_WorkshopPropDepth(const gentity_t&,const vec3_t,const vec3_t,const vec3_t,const JarSweepHull*,float&){return false;}
constexpr unsigned noEntity=ENTITYNUM_NONE;unsigned held=noEntity,selected=noEntity,jointFirst=0;int jointPendingMode=0;
struct {struct {vec3_t vieworg{},viewaxis[3]{{1,0,0},{0,1,0},{0,0,1}};}refdef;}cg;
struct Edit{unsigned joint=0;};std::vector<Edit>edits;
void rememberAction(Edit edit){edits.push_back(edit);}
void forgetJointEdit(unsigned id){
 edits.erase(std::remove_if(edits.begin(),edits.end(),[id](const Edit& edit){return edit.joint==id;}),edits.end());
}
struct Prop{}propValue;const Prop*prop(unsigned catalog){return PropCatalog(catalog)?&propValue:nullptr;}
struct PhysicsBounds {jar::physics::Vec half{8,8,8},center{};};
static PhysicsBounds modelShape;
bool modelBounds(const Prop&,PhysicsBounds& bounds,int){bounds=modelShape;return true;}
bool propHull(const Prop&,const vec3_t,vec3_t lo,vec3_t hi,int=0){VectorSet(lo,-8,-8,-8);VectorSet(hi,8,8,8);return true;}
static bool on=true,blocked=false,busy=false,playerLocked=false;static int aim=ENTITYNUM_WORLD,reverseThrust=0;static std::string status;
bool usable(){return !playerLocked;}
bool enabled(){return on;}bool sandboxReady(){return on;}bool occupied(const gentity_t&){return busy;}
bool editable(const gentity_t&e){return e.inuse&&e.s.number<ENTITYNUM_WORLD;}
Owned*owned(unsigned n){for(unsigned i=0;i<state.ownedCount;++i)if(state.owned[i].entity==n)return &state.owned[i];return nullptr;}
bool matches(const Owned&o){return g_entities[o.entity].inuse;}
void message(const char*s){status=s;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);e->s.pos.trType=TR_STATIONARY;e->s.pos.trTime=level.time;}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);e->s.apos.trTime=level.time;}
void SetClientViewAngle(gentity_t*e,vec3_t p){VectorCopy(p,e->client->ps.viewangles);}
bool setupMoved(gentity_t&){return false;}
void EvaluateTrajectoryDelta(const trajectory_t*t,int now,vec3_t out){if(t->trType==TR_STATIONARY){VectorClear(out);return;}VectorCopy(t->trDelta,out);if(t->trType==TR_GRAVITY)out[2]-=800*(now-t->trTime)*.001f;}
jar::physics::Vec pv(const float*p){return {p[0],p[1],p[2]};}
void nativeVector(jar::physics::Vec v,float*p){p[0]=v.x;p[1]=v.y;p[2]=v.z;}
bool removeJoint(unsigned id);
void G_JAR_WorkshopViewRay(vec3_t origin,vec3_t direction){VectorCopy(cg.refdef.vieworg,origin);VectorCopy(cg.refdef.viewaxis[0],direction);}
int selectControlNode(const vec3_t,const vec3_t,float limit,int fallback,float* distance){*distance=limit;return fallback;}
gentity_t* G_SubBSPOwner(const gentity_t*){return nullptr;}
gentity_t* G_BSPSectionBreakableRoot(const gentity_t&){return nullptr;}
gentity_t* G_LaserArmRoot(gentity_t*){return nullptr;}
#include "game/g_mover_motion.h"
bool G_JAR_MoverTranslate(gentity_t&e,const vec3_t move,vec3_t actual){if(blocked){VectorClear(actual);return false;}VectorCopy(move,actual);VectorAdd(e.currentOrigin,move,e.currentOrigin);VectorAdd(e.s.pos.trBase,move,e.s.pos.trBase);return true;}
bool G_JAR_MoverRotate(gentity_t&e,const vec3_t angles){if(blocked)return false;VectorCopy(angles,e.currentAngles);return true;}
const JarSweepHull* propMovingHull(const gentity_t&){return nullptr;}
static std::shared_ptr<const jar::physics::PropGeometry> trialGeometry;
std::shared_ptr<const jar::physics::PropGeometry> propTransformGeometry(const Prop&,int){return trialGeometry;}
#include "game/g_workshop_prop_clearance.inc"
#include "game/g_mover_motion.inc"
#include "game/g_workshop_attachments.inc"
#include "game/g_workshop_constraints.inc"
bool G_JAR_WorkshopTransportAllowed(const gentity_t& entity){
 return !G_JAR_WorkshopControls(&entity)&&held!=unsigned(entity.s.number)&&!Constrained(state,attachmentKey(entity));
}
std::map<unsigned,gclient_t> clients;
gentity_t&Entity(unsigned n,bool actor,float x,float mass){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.health=100;e.mass=int(mass);e.client=actor?&clients[n]:nullptr;e.s.eType=actor?ET_PLAYER:ET_GENERAL;e.e_ThinkFunc=actor?thinkF_NPC_Think:thinkF_NULL;
 VectorSet(e.currentOrigin,x,0,100);VectorSet(e.mins,-8,-8,-8);VectorSet(e.maxs,8,8,8);if(actor){e.client->ps.gravity=800;VectorCopy(e.currentOrigin,e.client->ps.origin);}return e;}
void Reset(){G_JAR_MoverMotionReset();nativeJointsReset();ropeReset();physicsWorld.Stop();state={};jointFirst=0;edits.clear();nativeControls.clear();clients.clear();on=true;blocked=busy=false;
 G_SandboxChargesReset();globals.num_entities=22;
 for(unsigned n:{0u,1u,2u,3u,4u,5u,10u}){auto&e=g_entities[n];e.inuse=qfalse;e.client=nullptr;e.item=nullptr;e.bmodel=qfalse;VectorClear(e.currentAngles);e.s.pos={};}
 g_entities[ENTITYNUM_WORLD].s.number=ENTITYNUM_WORLD;}
void Rope(gentity_t&a,gentity_t&b,float length){vec3_t axis={0,0,1};attachmentRope(a,a.currentOrigin,axis);attachmentRope(b,b.currentOrigin,axis);CHECK(state.jointCount==1);state.joints[0].length=length;}
int main(){return RunTests("native, player, world and Box3D attachments, forces, lifecycle and saves",[]{
 gi.cvar=[](const char*n,const char*,int)->cvar_t*{static cvar_t slack{},force{},reverse{};slack.value=1.1f;force.value=5000;reverse.integer=reverseThrust;return !strcmp(n,"jar_ropeSlack")?&slack:!strcmp(n,"jar_thrusterReverse")?&reverse:&force;};
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};
 gi.EntitiesInBox=[](const vec3_t low,const vec3_t high,gentity_t** list,int limit){
  int count=0;
  for(int n=0;n<=21&&count<limit;++n){
   auto& e=g_entities[n];if(!e.inuse||!e.linked)continue;
   bool overlaps=true;
   for(int k=0;k<3;++k)overlaps&=low[k]<=e.currentOrigin[k]+e.maxs[k]&&high[k]>=e.currentOrigin[k]+e.mins[k];
   if(overlaps)list[count++]=&e;
  }
  return count;
 };
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t mins,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=mins?1:.5f;t->entityNum=aim;t->plane.normal[2]=aim==ENTITYNUM_WORLD?-1:1;t->startsolid=blocked?qtrue:qfalse;if(mins)VectorCopy(end,t->endpos);else VectorSet(t->endpos,0,0,200);};
 Reset();auto&a=Entity(1,true,0,80);auto&b=Entity(2,true,120,160);Rope(a,b,100);a.client->ps.velocity[0]=-20;b.client->ps.velocity[0]=10;attachmentTick(.05f);
 CHECK(std::fabs(Distance(a.currentOrigin,b.currentOrigin)-100)<.01f);
 CHECK(std::fabs(a.client->ps.velocity[0]*80+b.client->ps.velocity[0]*160)<.1f);
 CHECK(std::fabs(a.client->ps.velocity[0])<.01f&&std::fabs(b.client->ps.velocity[0])<.01f&&a.health==100&&a.client->ps.gravity==800&&a.e_ThinkFunc==thinkF_NPC_Think&&!a.client->ps.eFlags);
 // Slack produces no force; changing one body's motion to frozen preserves it.
 VectorClear(a.client->ps.velocity);VectorClear(b.client->ps.velocity);state.joints[0].length=200;attachmentTick(.05f);CHECK(VectorLength(a.client->ps.velocity)==0);
 state.joints[0].length=80;nativeControls[1]=1;vec3_t fixed;VectorCopy(a.currentOrigin,fixed);attachmentTick(.05f);CHECK(VectorCompare(fixed,a.currentOrigin)&&Distance(a.currentOrigin,b.currentOrigin)<80.01f);nativeControls.clear();
 // Engine collision vetoes projection through geometry.
 b.currentOrigin[0]+=50;blocked=true;VectorCopy(b.currentOrigin,fixed);attachmentTick(.05f);CHECK(VectorCompare(fixed,b.currentOrigin));blocked=false;
 // Player zero and a world anchor survive a full record round trip, but not
 // autosaves/transitions. Reject native endpoints in a falsely labelled v5 blob.
 Reset();auto&p=Entity(0,true,0,80);vec3_t up={0,0,1};attachmentRope(p,p.currentOrigin,up);CHECK(attachmentTool("rope")&&state.jointCount==1);
 CHECK(state.joints[0].first==jar::constraint::NativeId(0)&&state.joints[0].second==jar::constraint::NativeId(ENTITYNUM_WORLD));
 auto bytes=Encode(state);State decoded;CHECK(!bytes.empty()&&Decode(bytes.data(),bytes.size(),decoded,ENTITYNUM_WORLD));
 auto previous=bytes;previous[0]=5;CHECK(!Decode(previous.data(),previous.size(),decoded,ENTITYNUM_WORLD));
 auto automatic=Encode(state,true);CHECK(Decode(automatic.data(),automatic.size(),decoded,ENTITYNUM_WORLD)&&!decoded.jointCount);
 // Simulated native gravity and pmove boundaries at multiple frame rates.
 state.joints[0].length=100;for(float dt:{1.f/20,1.f/60,1.f/120}){p.currentOrigin[2]=100;VectorClear(p.client->ps.velocity);
  for(int i=0;i<int(2.f/dt);++i){p.client->ps.velocity[2]-=800*dt;p.currentOrigin[2]+=p.client->ps.velocity[2]*dt;attachmentTick(dt);}
  CHECK(std::fabs(p.currentOrigin[2]-100)<.1f&&std::fabs(p.client->ps.origin[2]-p.currentOrigin[2])<.1f&&std::fabs(p.client->ps.velocity[2])<.01f);
 }
 // Mount on an underside (-Z outward normal) to thrust upward.
 const vec3_t down={0,0,-1};
 // A released slot cannot inherit a rope or thruster after reuse.
 CHECK(removeJoint(state.joints[0].id));Forget(state,0);CHECK(!state.jointCount);CHECK(!AddJoint(state,jar::constraint::Joint{}));
 attachmentThruster(p,false,p.currentOrigin,down);CHECK(state.jointCount==1);VectorClear(p.client->ps.velocity);attachmentTick(.05f);CHECK(std::fabs(p.client->ps.velocity[2]-100)<.01f&&p.client->ps.groundEntityNum==ENTITYNUM_NONE);
 {
  auto* jet=ThrusterFor(state,attachmentKey(p));CHECK(jet);
  vec3_t mount,originalAxis,reversedMount,reversedAxis;
  CHECK(attachmentPoint(p,jet->a,mount,originalAxis));
  jar::constraint::ReverseAxis(jet->a);jar::constraint::ReverseAxis(jet->b);
  CHECK(attachmentPoint(p,jet->a,reversedMount,reversedAxis));
  CHECK(VectorCompare(mount,reversedMount)&&DotProduct(originalAxis,reversedAxis)<-.999f);
  jet->force=2500;VectorClear(p.client->ps.velocity);attachmentTick(.05f);
  CHECK(std::fabs(p.client->ps.velocity[2]+50)<.01f);
  jet->force=5000;jar::constraint::ReverseAxis(jet->a);jar::constraint::ReverseAxis(jet->b);
  VectorSet(p.client->ps.velocity,0,0,100);
 }
 attachmentThruster(p,false,p.currentOrigin,down);attachmentTick(.05f);CHECK(std::fabs(p.client->ps.velocity[2]-100)<.01f);
 CHECK(edits.size()==1&&edits.back().joint==state.joints[0].id);
 attachmentThruster(p,false,p.currentOrigin,down);VectorClear(p.client->ps.velocity);p.currentAngles[PITCH]=90;attachmentTick(.05f);CHECK(std::fabs(p.client->ps.velocity[0]-100)<.01f&&std::fabs(p.client->ps.velocity[2])<.01f);
 nativeControls[0]=1;VectorClear(p.client->ps.velocity);attachmentTick(.05f);CHECK(VectorLength(p.client->ps.velocity)==0);nativeControls.clear();
 playerLocked=true;attachmentTick(.05f);CHECK(VectorLength(p.client->ps.velocity)==0);playerLocked=false;
 on=false;attachmentTick(.05f);CHECK(VectorLength(p.client->ps.velocity)==0);on=true;
 attachmentThruster(p,true,p.currentOrigin,down);CHECK(!state.jointCount&&edits.empty());
 reverseThrust=1;p.currentAngles[PITCH]=0;VectorClear(p.client->ps.velocity);
 attachmentThruster(p,false,p.currentOrigin,up);CHECK(state.jointCount==1);
 reverseThrust=0;auto reverseBytes=Encode(state);
 CHECK(Decode(reverseBytes.data(),reverseBytes.size(),decoded,ENTITYNUM_WORLD));state=decoded;
 attachmentTick(.05f);CHECK(std::fabs(p.client->ps.velocity[2]-100)<.01f);
 attachmentThruster(p,true,p.currentOrigin,up);CHECK(!state.jointCount);
 {
  auto& hull=Entity(20,true,0,80);
  auto& pilot=Entity(21,true,0,80);
  vehicleInfo_t info{};
  info.type=VH_SPEEDER;
  Vehicle_t vehicle{};
  vehicle.m_pVehicleInfo=&info;
  vehicle.m_pParentEntity=&hull;
  vehicle.m_pPilot=&pilot;
  hull.m_pVehicle=&vehicle;
  hull.client->NPC_class=CLASS_VEHICLE;
  hull.client->ps.speed=300;
  VectorSet(hull.client->ps.velocity,300,0,0);
  pilot.owner=&hull;
  pilot.s.m_iVehicleNum=hull.s.number;
  const vec3_t exhaust={0,1,0};
  attachmentThruster(hull,false,hull.currentOrigin,exhaust);
  CHECK(state.jointCount==1&&edits.size()==1);
  CHECK(G_TransportVehicleReady(hull));
  CHECK(attachmentEntity(state.joints[0].first)==&hull);
  vec3_t mountPoint,mountExhaust;
  CHECK(attachmentPoint(hull,state.joints[0].a,mountPoint,mountExhaust)&&DotProduct(exhaust,mountExhaust)>.999f);
  attachmentTick(.05f);
  CHECK(std::fabs(vehicle.jarPushDrift[1]+100)<.01f&&std::fabs(hull.client->ps.velocity[1]+100)<.01f);
  CHECK(hull.client->ps.speed==300&&hull.client->ps.velocity[0]==300&&vehicle.m_pPilot==&pilot);
  CHECK(pilot.owner==&hull&&pilot.s.m_iVehicleNum==hull.s.number&&VectorLength(pilot.client->ps.velocity)==0);
  nativeControls[pilot.s.number]=1;
  attachmentTick(.05f);
  CHECK(std::fabs(vehicle.jarPushDrift[1]+100)<.01f);
  nativeControls.clear();
  attachmentThruster(hull,false,hull.currentOrigin,exhaust);
  attachmentTick(.05f);
  CHECK(std::fabs(vehicle.jarPushDrift[1]+100)<.01f);
  attachmentThruster(hull,true,hull.currentOrigin,exhaust);
  CHECK(!state.jointCount&&edits.empty());
  hull={};
  pilot={};
 }
 // Native pickup velocity is rebased at the current time before the impulse.
 auto&item=Entity(3,false,0,10);item.s.eType=ET_ITEM;item.s.pos.trType=TR_GRAVITY;item.s.pos.trDelta[2]=200;item.s.pos.trTime=0;level.time=100;
 vec3_t impulse={0,0,100};attachmentImpulse(item,impulse,item.currentOrigin);CHECK(item.s.pos.trType==TR_GRAVITY&&item.s.pos.trTime==100&&std::fabs(item.s.pos.trDelta[2]-130)<.01f);
 item.s.eType=ET_MISSILE;item.s.pos.trType=TR_LINEAR;vec3_t displacement={10,0,0};attachmentMove(item,displacement);CHECK(item.s.pos.trType==TR_LINEAR&&std::fabs(item.s.pos.trDelta[2]-130)<.01f);
 auto&mover=Entity(4,false,0,100);mover.s.eType=ET_MOVER;attachmentThruster(mover,false,mover.currentOrigin,down);CHECK(state.jointCount==1);CHECK(!attachmentMovable(mover));const auto controllerThink=mover.e_ThinkFunc;attachmentTick(.05f);CHECK(mover.currentOrigin[2]>101.9f&&mover.currentOrigin[2]<102.1f&&mover.e_ThinkFunc==controllerThink&&mover.s.eType==ET_MOVER);attachmentThruster(mover,true,mover.currentOrigin,down);CHECK(!state.jointCount);const float coast=mover.currentOrigin[2];attachmentTick(.05f);CHECK(mover.currentOrigin[2]>coast);nativeControls[4]=1;const float heldMover=mover.currentOrigin[2];attachmentTick(.05f);CHECK(mover.currentOrigin[2]==heldMover);nativeControls.clear();attachmentTick(.05f);CHECK(mover.currentOrigin[2]==heldMover);
 // Forced anchors advance before rope projection, including their added
 // velocity, so the attached actor follows without a one-frame delay.
 Reset();auto&anchor=Entity(4,false,0,100);anchor.s.eType=ET_MOVER;auto&follower=Entity(5,true,0,80);follower.currentOrigin[2]=0;
 Rope(anchor,follower,100);attachmentThruster(anchor,false,anchor.currentOrigin,down);attachmentTick(.05f);
 CHECK(std::fabs(follower.currentOrigin[2]-2)<.01f&&std::fabs(follower.client->ps.velocity[2]-80)<.01f);
 // Off-center native thrust supplies torque; centered thrust does not.
 Reset();auto&spinning=Entity(4,false,0,100);spinning.s.eType=ET_MOVER;vec3_t edge={4,0,100};attachmentThruster(spinning,false,edge,down);attachmentTick(.05f);
 JarMoverMotion motion;CHECK(G_JAR_MoverMotionRead(spinning,motion)&&std::fabs(motion.angular[1]+7.5f)<.01f&&spinning.currentAngles[PITCH]<-10&&spinning.currentOrigin[2]>101.9f);
 attachmentThruster(spinning,true,edge,down);const float pitch=spinning.currentAngles[PITCH];attachmentTick(.05f);CHECK(spinning.currentAngles[PITCH]<pitch);
 nativeControls[4]=1;attachmentTick(.05f);CHECK(G_JAR_MoverMotionRead(spinning,motion)&&VectorLength(motion.angular)==0);nativeControls.clear();
 Reset();auto&ordinary=Entity(4,false,0,100);attachmentThruster(ordinary,false,edge,down);attachmentTick(.05f);
 CHECK(G_JAR_MoverMotionRead(ordinary,motion)&&motion.angular[1]<-7.49f&&motion.velocity[2]==0&&ordinary.s.pos.trDelta[2]>79.9f&&ordinary.e_ThinkFunc==thinkF_G_RunObject);
 // A native weld absorbs the torque into its projected angular velocity;
 // the external angular record must not be added again next frame.
 auto&pin=g_entities[ENTITYNUM_WORLD];pin.s.number=ENTITYNUM_WORLD;constraintLink(ordinary,ordinary.currentOrigin,up,jar::constraint::Weld);constraintLink(pin,ordinary.currentOrigin,up,jar::constraint::Weld);
 attachmentTick(.05f);CHECK(G_JAR_MoverMotionRead(ordinary,motion)&&VectorLength(motion.angular)==0);vec3_t constrainedSpin;attachmentSpin(ordinary,constrainedSpin);CHECK(VectorLength(constrainedSpin)<.01f);
 {
  Reset();auto& offsetProp=Entity(1,false,0,100);CHECK(Own(state,1,1001));
  auto& body=*owned(1);body.frozen=0;body.physics.mass=100;
  offsetProp.s.apos={};
  modelShape={{16,4,2},{7,0,0}};
  VectorSet(offsetProp.currentAngles,0,90,0);
  VectorCopy(offsetProp.currentOrigin,offsetProp.s.pos.trBase);
  VectorSet(offsetProp.mins,-4,-9,-2);VectorSet(offsetProp.maxs,4,23,2);
  const vec3_t center={0,7,100},offCenter={0,9,100};
  attachmentThruster(offsetProp,false,center,down);attachmentTick(.05f);
  CHECK(VectorLength(offsetProp.s.apos.trDelta)<.0001f&&offsetProp.s.pos.trDelta[2]>79.9f);
  attachmentThruster(offsetProp,true,center,down);
  attachmentThruster(offsetProp,false,offCenter,down);attachmentTick(.05f);
  const float expectedSpin=(2*5000*32.f*.05f)/(100*(16*16+2*2)/3.f);
  CHECK(std::fabs(offsetProp.s.apos.trDelta[0]-expectedSpin)<.001f);
  CHECK(std::fabs(offsetProp.s.apos.trDelta[1])<.001f&&std::fabs(offsetProp.s.apos.trDelta[2])<.001f);
  vec3_t velocity,start;VectorCopy(offsetProp.s.pos.trDelta,velocity);VectorCopy(offsetProp.currentOrigin,start);
  blocked=true;attachmentAdvanceProp(offsetProp,body,*prop(body.catalog),.05f);
  CHECK(VectorCompare(start,offsetProp.currentOrigin)&&VectorCompare(velocity,offsetProp.s.pos.trDelta)&&offsetProp.currentAngles[YAW]==90);
  blocked=false;attachmentTick(.05f);VectorCopy(offsetProp.s.pos.trDelta,velocity);
  attachmentAdvanceProp(offsetProp,body,*prop(body.catalog),.05f);
  vec3_t centerAfter;
  attachmentPropCenterOffset(modelShape.center,offsetProp.currentAngles,centerAfter);
  VectorAdd(centerAfter,offsetProp.currentOrigin,centerAfter);
  CHECK(Distance(center,centerAfter)<.001f&&Distance(start,offsetProp.currentOrigin)>.1f);
  CHECK(VectorCompare(velocity,offsetProp.s.pos.trDelta)&&Distance(offsetProp.s.pos.trBase,offsetProp.currentOrigin)<.001f);
  jar::constraint::Joint weld;
  weld.kind=jar::constraint::Weld;weld.first=body.id;weld.second=jar::constraint::NativeId(ENTITYNUM_WORLD);
  CHECK(attachmentFrameAt(offsetProp,center,up,weld.a));
  CHECK(attachmentFrameAt(g_entities[ENTITYNUM_WORLD],center,up,weld.b));
  CHECK(AddJoint(state,weld));
  VectorCopy(offsetProp.currentOrigin,start);
  VectorSet(offsetProp.s.apos.trDelta,0,0,1);
  attachmentAdvanceProp(offsetProp,body,*prop(body.catalog),.05f);
  CHECK(Distance(start,offsetProp.currentOrigin)<.001f);
  offsetProp.s.apos={};modelShape={};
 }
 Reset();auto&fallback=Entity(1,false,0,100);CHECK(Own(state,1,1001));owned(1)->frozen=0;attachmentThruster(fallback,false,edge,down);attachmentTick(.05f);CHECK(fallback.s.apos.trDelta[1]<-7.49f&&!G_JAR_MoverMotionActive(fallback));
 blocked=true;attachmentAdvanceProp(fallback,*owned(fallback.s.number),*prop(owned(fallback.s.number)->catalog),.05f);CHECK(fallback.currentAngles[PITCH]==0&&VectorLength(fallback.s.apos.trDelta)==0);blocked=false;attachmentTick(.05f);attachmentAdvanceProp(fallback,*owned(fallback.s.number),*prop(owned(fallback.s.number)->catalog),.05f);CHECK(fallback.currentAngles[PITCH]<-20);
 {
  auto& support=fallback;auto& charge=Entity(10,false,9,0);
  const auto* supportClass=support.classname;
  support.classname=const_cast<char*>("jar_sandbox_prop");
  VectorClear(support.currentAngles);gi.linkentity(&support);
  charge.classname=const_cast<char*>("detpack");charge.s.weapon=WP_DET_PACK;charge.s.eFlags=EF_MISSILE_STICK;
  charge.e_DieFunc=dieF_WP_ExplosiveDie;charge.e_ThinkFunc=thinkF_WP_Explode;charge.nextthink=1700;
  charge.contents=CONTENTS_SHOTCLIP;charge.s.pos.trType=TR_STATIONARY;
  gi.linkentity(&charge);G_SandboxChargeAttach(charge,support);
  CHECK(charge.s.groundEntityNum==support.s.number);
  static bool stopCharge=false;
  const auto previousTrace=gi.trace;
  gi.trace=[](trace_t* tr,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int,EG2_Collision,int){
   *tr={};tr->fraction=1;
   if(pass==10){CHECK(!g_entities[1].linked&&!g_entities[10].linked);tr->fraction=.5f;tr->startsolid=stopCharge?qtrue:qfalse;}
   vec3_t travel;VectorSubtract(end,start,travel);VectorMA(start,tr->fraction,travel,tr->endpos);
  };
  const vec3_t step={20,0,0};
  const float before=support.currentOrigin[0],chargeBefore=charge.currentOrigin[0];
  attachmentMove(support,step);
  CHECK(support.currentOrigin[0]==before+10&&charge.currentOrigin[0]==chargeBefore+10&&charge.nextthink==1700);
  stopCharge=true;
  VectorSet(support.s.apos.trDelta,0,0,1);
  attachmentAdvanceProp(support,*owned(support.s.number),*prop(owned(support.s.number)->catalog),.05f);
  CHECK(support.currentAngles[YAW]==0&&VectorLength(support.s.apos.trDelta)==0&&charge.currentOrigin[0]==chargeBefore+10);
  auto old=ropePose(support);old.position.x-=20;
  ropeRestore(support,old);
  CHECK(support.currentOrigin[0]==before+10&&support.linked&&charge.linked&&charge.nextthink==1700);
  stopCharge=false;gi.trace=previousTrace;support.classname=const_cast<char*>(supportClass);
 }
 {
  fallback.classname=const_cast<char*>("jar_sandbox_prop");
  auto& peer=Entity(2,false,52,100);peer.classname=const_cast<char*>("jar_sandbox_prop");
  auto& mine=Entity(5,false,44,0);mine.classname=const_cast<char*>("tripmine");
  mine.s.weapon=WP_TRIP_MINE;mine.s.eFlags=EF_MISSILE_STICK;mine.s.pos.trType=TR_STATIONARY;
  mine.e_DieFunc=dieF_WP_ExplosiveDie;mine.contents=CONTENTS_SHOTCLIP;
  gi.linkentity(&peer);gi.linkentity(&mine);G_SandboxChargeAttach(mine,peer);
  CHECK(mine.s.groundEntityNum==peer.s.number);
  const auto previousTrace=gi.trace;
  static bool foreignContact=true;
  gi.trace=[](trace_t* hit,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision,int){
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(pass==1){
    CHECK(mask&SANDBOX_MOUNTED_CHARGE_CONTENTS);
    CHECK(!g_entities[10].linked&&g_entities[5].linked);
    if(foreignContact&&(g_entities[5].contents&SANDBOX_MOUNTED_CHARGE_CONTENTS))hit->startsolid=qtrue;
   }
  };
  {
   SandboxMountedChargeCollision collision;
   VectorSet(fallback.s.apos.trDelta,0,0,1);attachmentAdvanceProp(fallback,*owned(fallback.s.number),*prop(owned(fallback.s.number)->catalog),.05f);
   CHECK(fallback.currentAngles[YAW]==0&&VectorLength(fallback.s.apos.trDelta)==0);
   foreignContact=false;
   VectorSet(fallback.s.apos.trDelta,0,0,1);attachmentAdvanceProp(fallback,*owned(fallback.s.number),*prop(owned(fallback.s.number)->catalog),.05f);
   CHECK(fallback.currentAngles[YAW]>2.8f&&g_entities[10].linked&&mine.linked);
  }
  CHECK(!(mine.contents&SANDBOX_MOUNTED_CHARGE_CONTENTS));gi.trace=previousTrace;
 }
 // A mixed rope applies reciprocal impulse and swept position correction to
 // the real Box3D body; it must not be sent to the rigid-only joint restore path.
 Reset();auto&box=Entity(1,false,0,0);auto&npc=Entity(2,true,120,80);CHECK(Own(state,1,1001));owned(1)->frozen=0;
 CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000},{-1000,-1000,-1000},{1000,1000,-1000},{-1000,1000,-1000}}));
 jar::physics::Pose pose;pose.position=pv(box.currentOrigin);pose.linear.x=-20;CHECK(physicsWorld.Prop(1,owned(1)->id,{8,8,8},{},pose,false));npc.client->ps.velocity[0]=20*physicsWorld.Mass(1)/80;Rope(box,npc,100);CHECK(!attachmentBoxJoint(state.joints[0]));
 attachmentTick(.05f);CHECK(physicsWorld.Read(1,pose));CHECK(std::fabs(pose.position.x-box.currentOrigin[0])<.01f&&Distance(box.currentOrigin,npc.currentOrigin)<100.01f);
 CHECK(std::fabs(pose.linear.x)<.01f&&std::fabs(npc.client->ps.velocity[0])<.01f);CHECK(std::fabs(pose.linear.x*physicsWorld.Mass(1)+npc.client->ps.velocity[0]*80)<.2f);
 CHECK(Encode(state).size()>32);Forget(state,2);CHECK(!state.jointCount);
 // Both physics backends must arrest separation at the tie point. A center
 // velocity of zero would overcorrect once the same impulse adds rotation.
 for(bool rigid:{false,true}){
  Reset();auto& propBody=Entity(1,false,0,80);auto& actorBody=Entity(2,true,100,80);
  CHECK(Own(state,1,1001));owned(1)->frozen=0;
  propBody.s.pos.trDelta[0]=-40;actorBody.client->ps.velocity[0]=40;
  if(rigid){
   CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{-1000,1000,-1000}}));
   jar::physics::Pose boxPose;boxPose.position=pv(propBody.currentOrigin);boxPose.linear.x=-40;
   CHECK(physicsWorld.Prop(1,owned(1)->id,{8,8,8},{0,2,0},boxPose,false));
   CHECK(physicsWorld.Configure(1,80,0));
  }
  const vec3_t tieA={0,6,100},tieB={100,6,100};
  attachmentRope(propBody,tieA,up);attachmentRope(actorBody,tieB,up);
  CHECK(state.jointCount==1);state.joints[0].length=100;
  attachmentTick(.05f);
  vec3_t tieVelocity,angular;
  attachmentVelocity(propBody,tieVelocity,tieA);
  if(rigid){jar::physics::Pose boxPose;CHECK(physicsWorld.Read(1,boxPose));nativeVector(boxPose.angular,angular);}
  else VectorCopy(propBody.s.apos.trDelta,angular);
  CHECK(std::fabs(tieVelocity[0]-actorBody.client->ps.velocity[0])<.01f);
  CHECK(angular[2]<-.1f&&actorBody.client->ps.velocity[0]>1);
 }
 Reset();Entity(1,false,0,0);Entity(2,true,120,80);
 CHECK(Own(state,1,1001));owned(1)->frozen=0;
 // Owned native actors participate in duplicate graphs; external map/player
 // attachments are omitted rather than pulling a pasted copy to old entities.
 CHECK(Own(state,2,3001));Rope(box,npc,100);std::vector<unsigned> members;CHECK(jar::duplicate::Members(state,owned(1)->id,members)&&members.size()==2);
 auto&q=Entity(5,true,200,80);attachmentRope(npc,npc.currentOrigin,up);attachmentRope(q,q.currentOrigin,up);CHECK(jar::duplicate::Members(state,owned(1)->id,members)&&members.size()==2);
 // Removing a native entity before the next tick also prunes stale endpoints.
 q.inuse=qfalse;attachmentTick(.05f);CHECK(state.jointCount==1);
 npc.health=0;attachmentTick(.05f);CHECK(!state.jointCount);
 // Actual tool creation accepts player zero and world, without replacing
 // native gravity, think callbacks or controller ownership.
 Reset();level.time=100;auto&actor=Entity(0,true,50,80);auto&world=g_entities[ENTITYNUM_WORLD];world.s.number=ENTITYNUM_WORLD;
 vec3_t pivot={0,0,100};constraintLink(actor,pivot,up,jar::constraint::Weld);CHECK(jointFirst==jar::constraint::NativeId(0));aim=ENTITYNUM_WORLD;
 const auto worldShot=toolSurfaceTrace();
 CHECK(attachmentTool("weld",&worldShot)&&state.jointCount==1&&edits.size()==1);
 const auto joint=state.joints[0];CHECK(joint.kind==jar::constraint::Weld&&jar::constraint::Native(joint.first)&&jar::constraint::Native(joint.second));
 vec3_t worldPivot,bodyPivot,unusedAxis;
 CHECK(attachmentPoint(world,joint.b,worldPivot,unusedAxis)&&VectorCompare(worldPivot,worldShot.endpos));
 CHECK(attachmentPoint(actor,joint.a,bodyPivot,unusedAxis)&&VectorCompare(bodyPivot,worldShot.endpos));
 auto saved=Encode(state);CHECK(!saved.empty()&&Decode(saved.data(),saved.size(),decoded,ENTITYNUM_WORLD));auto oldWire=saved;oldWire[0]=6;CHECK(!Decode(oldWire.data(),oldWire.size(),decoded,ENTITYNUM_WORLD));
 for(int i=0;i<60;++i){level.time+=50;actor.client->ps.velocity[2]-=40;VectorMA(actor.currentOrigin,.05f,actor.client->ps.velocity,actor.currentOrigin);attachmentTick(.05f);}
 CHECK(std::fabs(actor.currentOrigin[2]-100)<.1f&&std::fabs(actor.client->ps.velocity[2])<.1f&&actor.client->ps.gravity==800&&actor.e_ThinkFunc==thinkF_NPC_Think&&nativeControls.empty());
 selected=0;unlinkTool();CHECK(!state.jointCount&&edits.empty());
 // A Toolgun can start a hinge on bare scenery, cancel there, or
 // finish on a body while retaining the first clicked surface frame.
 CHECK(attachmentTool("hinge",&worldShot)&&jointFirst==jar::constraint::NativeId(ENTITYNUM_WORLD));
 CHECK(attachmentTool("hinge",nullptr,true)&&!jointFirst);
 CHECK(attachmentTool("hinge",&worldShot)&&jointFirst==jar::constraint::NativeId(ENTITYNUM_WORLD));
 aim=0;const auto actorShot=toolSurfaceTrace();
 CHECK(attachmentTool("hinge",&actorShot)&&state.jointCount==1&&state.joints[0].kind==jar::constraint::Hinge);
 CHECK(state.joints[0].first==jar::constraint::NativeId(ENTITYNUM_WORLD)&&state.joints[0].second==jar::constraint::NativeId(0));
 unlinkTool();CHECK(!state.jointCount);
 const vec3_t firstAxis={1,0,0};
 constraintLink(actor,actor.currentOrigin,firstAxis,jar::constraint::Hinge);
 CHECK(attachmentTool("hinge",&worldShot)&&state.jointCount==1);
 CHECK(attachmentPoint(world,state.joints[0].b,worldPivot,unusedAxis));
 CHECK(VectorCompare(worldPivot,worldShot.endpos)&&DotProduct(unusedAxis,firstAxis)>.999f);
 unlinkTool();CHECK(!state.jointCount);
 // Axis comes from the first face. A displaced native model is projected to
 // the hinge while engine collision may veto correction into solid geometry.
 auto&model=Entity(3,false,100,40);model.linked=qtrue;constraintLink(actor,pivot,up,jar::constraint::Hinge);constraintLink(model,model.currentOrigin,up,jar::constraint::Hinge);CHECK(state.jointCount==1);
 nativeControls[0]=1;model.currentOrigin[0]+=30;blocked=true;VectorCopy(model.currentOrigin,fixed);attachmentTick(.05f);CHECK(VectorCompare(fixed,model.currentOrigin)&&actor.linked);blocked=false;
 for(int i=0;i<10;++i){level.time+=50;attachmentTick(.05f);}CHECK(std::fabs(model.currentOrigin[0]-100)<.2f&&model.e_ThinkFunc==thinkF_G_RunObject);
 Forget(state,0);++entityGeneration[0];actor.inuse=qfalse;attachmentTick(.05f);CHECK(!state.jointCount&&nativeJointHistory.empty());
 // A mixed component includes its attached rigid-only links too. They must
 // not be solved a second time in the collision world's joint restore pass.
 Reset();auto&first=Entity(1,false,0,0);auto&second=Entity(2,false,40,0);auto&native=Entity(3,true,80,80);CHECK(Own(state,1,1001)&&Own(state,2,1002));owned(1)->frozen=owned(2)->frozen=0;
 CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000}}));for(auto*e:{&first,&second}){jar::physics::Pose p;p.position=pv(e->currentOrigin);CHECK(physicsWorld.Prop(e->s.number,owned(e->s.number)->id,{8,8,8},{},p,false));}
 constraintLink(first,first.currentOrigin,up,jar::constraint::Weld);constraintLink(second,second.currentOrigin,up,jar::constraint::Weld);CHECK(attachmentBoxJoint(state.joints[0]));
 constraintLink(second,second.currentOrigin,up,jar::constraint::Hinge);constraintLink(native,native.currentOrigin,up,jar::constraint::Hinge);CHECK(state.jointCount==2&&!attachmentBoxJoint(state.joints[0])&&!attachmentBoxJoint(state.joints[1]));
 nativeControls[3]=1;native.currentOrigin[0]+=20;for(int i=0;i<10;++i){level.time+=50;attachmentTick(.05f);}CHECK(first.currentOrigin[0]>19&&second.currentOrigin[0]>59);
 CHECK(physicsWorld.Read(1,pose)&&std::fabs(pose.position.x-first.currentOrigin[0])<.01f);
 // The live joint projection uses the model hull and preserves the previous
 // pose when the sweep is blocked. Its peer is relinked in either case.
 auto geometry=std::make_shared<jar::physics::PropGeometry>();
 for(float x:{-8.f,8.f})for(float y:{-2.f,2.f})for(float z:{-3.f,3.f})geometry->Add({x,y,z});
 trialGeometry=geometry;
 static int hullChecks=0;static bool blockHull=true;
 gi.linkentity(&second);
 gi.JAR_TraceMovingHull=[](trace_t* hit,const vec3_t,const vec3_t end,const vec3_t low,const vec3_t high,
  int pass,int mask,const JarSweepHull& hull){
   *hit={};hit->fraction=1;VectorCopy(end,hit->endpos);
   if(pass!=1)return;
   CHECK(!g_entities[2].linked&&(mask&SANDBOX_MOUNTED_CHARGE_CONTENTS));
   CHECK(hull.pointCount>=8&&high[1]-low[1]<16);
   ++hullChecks;if(blockHull)hit->startsolid=qtrue;
 };
 VectorCopy(first.currentOrigin,fixed);
 native.currentOrigin[0]+=20;level.time+=50;attachmentTick(.05f);
 CHECK(hullChecks>0&&VectorCompare(fixed,first.currentOrigin)&&second.linked);
 blockHull=false;
 for(int i=0;i<10;++i){level.time+=50;attachmentTick(.05f);}
 CHECK(first.currentOrigin[0]>fixed[0]+10&&second.linked&&physicsWorld.Read(1,pose));
 CHECK(std::fabs(pose.position.x-first.currentOrigin[0])<.01f);
 gi.JAR_TraceMovingHull=nullptr;trialGeometry.reset();
 selected=3;unlinkTool();CHECK(state.jointCount==1&&attachmentBoxJoint(state.joints[0]));
 physicsWorld.Stop();attachmentClassifyJoints();CHECK(!attachmentBoxJoint(state.joints[0]));state.jointCount=0;VectorSet(first.s.apos.trDelta,0,0,1);VectorClear(first.currentAngles);attachmentAdvanceProp(first,*owned(first.s.number),*prop(owned(first.s.number)->catalog),.05f);CHECK(std::fabs(first.currentAngles[1]-2.864789f)<.01f); // Angular momentum continues after Unlink.
 // Sharing the map as an anchor does not connect otherwise separate builds.
 Reset();auto& rigid=Entity(1,false,0,0);auto& anchoredActor=Entity(2,true,160,80);
 CHECK(Own(state,1,1001));owned(1)->frozen=0;
 CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000}}));
 pose={};pose.position=pv(rigid.currentOrigin);
 CHECK(physicsWorld.Prop(1,owned(1)->id,{8,8,8},{},pose,false));
 constraintLink(world,rigid.currentOrigin,up,jar::constraint::Weld);
 constraintLink(rigid,rigid.currentOrigin,up,jar::constraint::Weld);
 constraintLink(anchoredActor,anchoredActor.currentOrigin,up,jar::constraint::Hinge);
 constraintLink(world,anchoredActor.currentOrigin,up,jar::constraint::Hinge);
 CHECK(state.jointCount==2&&attachmentBoxJoint(state.joints[0])&&attachmentNativeJoint(state.joints[1]));
 const auto& anchoredWeld=state.joints[0];
 CHECK(physicsWorld.Joint(anchoredWeld,jar::physics::World::worldAnchor,1));
 anchoredActor.currentOrigin[2]-=16;
 for(int i=0;i<60;++i){level.time+=50;physicsWorld.Step(.05f,800);attachmentTick(.05f);}
 CHECK(physicsWorld.Read(1,pose)&&std::fabs(pose.position.z-100)<.5f);
 CHECK(std::fabs(anchoredActor.currentOrigin[2]-100)<.2f&&physicsWorld.Statistics().props==1);
 physicsWorld.Stop();attachmentClassifyJoints();
 CHECK(attachmentNativeJoint(state.joints[0])&&attachmentNativeJoint(state.joints[1]));
 // A clear ceiling rope limits the real Box3D body at its saved length.
 // Obstructing its route suspends that joint, and removing the obstacle resumes it.
 for(const bool worldFirst:{false,true}){
  Reset();auto& hanging=Entity(1,false,0,80);CHECK(Own(state,1,1001));owned(1)->frozen=0;
  CHECK(physicsWorld.Start({{-1000,-1000,-1000},{1000,-1000,-1000},{1000,1000,-1000}}));
  pose={};pose.position=pv(hanging.currentOrigin);
  CHECK(physicsWorld.Prop(1,owned(1)->id,{8,8,8},{},pose,false));
  const vec3_t ceiling={0,0,200};
  const auto worldKey=jar::constraint::NativeId(ENTITYNUM_WORLD);
  const auto bodyKey=owned(1)->id;
  jar::constraint::Joint tether;
  tether.kind=jar::constraint::Rope;tether.length=130;
  tether.first=worldFirst?worldKey:bodyKey;tether.second=worldFirst?bodyKey:worldKey;
  CHECK(attachmentFrameAt(world,ceiling,down,worldFirst?tether.a:tether.b));
  CHECK(attachmentFrameAt(hanging,hanging.currentOrigin,up,worldFirst?tether.b:tether.a));
  const auto id=AddJoint(state,tether);CHECK(id);
  for(int i=0;i<120;++i){
   ++level.time;attachmentPrepareRopes();CHECK(attachmentSyncBoxJoints());
   physicsWorld.Step(1.f/120,800);CHECK(physicsWorld.Read(1,pose));nativeVector(pose.position,hanging.currentOrigin);
  }
  CHECK(std::fabs(pose.position.z-70)<.5f&&physicsWorld.Statistics().joints==1&&physicsWorld.Statistics().props==1);
  auto& link=state.joints[0];
  CHECK(!resizeRope(link,8193)&&link.length==130&&physicsWorld.Statistics().joints==1);
  CHECK(resizeRope(link,155)&&link.length==155&&link.id==id&&link.first==tether.first&&link.second==tether.second);
  CHECK(physicsWorld.Statistics().joints==0);
  for(int i=0;i<120;++i){
   ++level.time;attachmentPrepareRopes();CHECK(attachmentSyncBoxJoints());
   physicsWorld.Step(1.f/120,800);CHECK(physicsWorld.Read(1,pose));nativeVector(pose.position,hanging.currentOrigin);
  }
  CHECK(std::fabs(pose.position.z-45)<.5f&&physicsWorld.Statistics().joints==1);
  auto& obstruction=Entity(3,false,0,0);obstruction.currentOrigin[2]=145;obstruction.contents=CONTENTS_SOLID;
  gi.linkentity(&obstruction);blocked=true;++level.time;attachmentPrepareRopes();
  CHECK(!ropeRigid(state.joints[0])&&attachmentSyncBoxJoints()&&physicsWorld.Statistics().joints==0);
  blocked=false;gi.unlinkentity(&obstruction);++level.time;attachmentPrepareRopes();
  CHECK(ropeRigid(state.joints[0])&&attachmentSyncBoxJoints()&&physicsWorld.Statistics().joints==1);
  CHECK(removeJoint(id)&&physicsWorld.Statistics().joints==0);
 }
 // The visible crystal creates anchors on its crate, not its decorative helper.
 Reset();auto&crate=Entity(1,false,0,40);auto&crystal=Entity(2,false,0,1);crate.classname=const_cast<char*>("misc_crystal_crate");crate.model=const_cast<char*>("@jar_crystal_crate");crate.lastEnemy=&crystal;crystal.classname=const_cast<char*>("info_notnull");crystal.model=const_cast<char*>(jar::crystalCrate::crystal);crystal.owner=&crate;
 aim=2;CHECK(attachmentTool("rope")&&jointFirst==jar::constraint::NativeId(1));
 jointFirst=0;CHECK(attachmentTool("weld")&&jointFirst==jar::constraint::NativeId(1)&&jointPendingMode==jar::constraint::Weld);
 // Aiming at the door-camera head creates anchors on the movable root.
 Reset();auto&camera=Entity(1,false,0,20);auto&shaft=Entity(2,false,0,1);auto&head=Entity(3,false,20,1);
 camera.classname=const_cast<char*>("misc_model_jabba_cam");camera.model=const_cast<char*>("@jar_door_camera");camera.lastEnemy=&shaft;
 shaft.classname=head.classname=const_cast<char*>("info_notnull");shaft.model=const_cast<char*>(jar::doorCamera::shaft);head.model=const_cast<char*>(jar::doorCamera::head);shaft.owner=head.owner=&camera;shaft.lastEnemy=&head;
 aim=3;CHECK(attachmentTool("rope")&&jointFirst==jar::constraint::NativeId(1));
 jointFirst=0;CHECK(attachmentTool("hinge")&&jointFirst==jar::constraint::NativeId(1)&&jointPendingMode==jar::constraint::Hinge);
 CHECK(attachmentTool("thruster")&&state.jointCount==1&&state.joints[0].first==jar::constraint::NativeId(1));
 CHECK(attachmentTool("thruster",nullptr,true)&&!state.jointCount);

 Reset();auto& usableBody=Entity(1,false,0,40);auto& ropeRider=Entity(2,true,80,80);
 usableBody.classname="func_usable";usableBody.model="@jar_box";usableBody.targetname="jar_sb_1";
 usableBody.s.modelindex=5;CHECK(G_SandboxUsableInitialize(usableBody));
 nativeControls[1]=1;Rope(usableBody,ropeRider,35);
 const auto retained=state.joints[0].id;
 jar::sandboxUsable::Settings usableState;
 CHECK(G_SandboxUsableRead(usableBody,usableState));usableState.solid=false;
 CHECK(G_SandboxUsableSet(usableBody,usableState));
 attachmentTick(.05f);CHECK(state.jointCount==1&&state.joints[0].id==retained&&
  ropeRider.currentOrigin[0]==80&&!ropeStates.count(retained));
 usableState.solid=true;CHECK(G_SandboxUsableSet(usableBody,usableState));
 level.time+=50;attachmentTick(.05f);
 CHECK(state.jointCount==1&&state.joints[0].id==retained&&ropeRider.currentOrigin[0]<80);

#include "rope_attachments.inc"
 });}
