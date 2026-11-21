// Actual native hold controller, serializers and swept actor contact math.
#include "game/g_local.h"
#include "game/g_laser_arm.h"
#include "game/g_volume_geometry.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "game/jar_actor_contact.h"
#include "qcommon/ojk_i_saved_game.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "test_support.h"
#include "bsp_section_motion_stub.h"
#include "memory_save.h"
#include <map>
#include <vector>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
unsigned held=ENTITYNUM_NONE;float holdDistance=144;
void eye(float*origin,float*direction){VectorClear(origin);VectorClear(direction);direction[0]=1;}
void NPC_SetAnim(gentity_t*e,int,int anim,int,int){e->client->ps.torsoAnim=anim;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);VectorClear(e->s.pos.trDelta);if(e->client)VectorCopy(p,e->client->ps.origin);}
void SetClientViewAngle(gentity_t* e,vec3_t angles){VectorCopy(angles,e->client->ps.viewangles);}
bool setupMoved(gentity_t&e){return jar::volumeEntity::Refresh(e);}
#include "game/g_mover_motion.h"
#include "game/g_workshop_native_control.inc"
#include "game/g_entity_angles.inc"
static float moverFraction=1;
bool G_JAR_MoverTranslate(gentity_t& e,const vec3_t requested,vec3_t actual){
 VectorScale(requested,moverFraction,actual);
 VectorAdd(e.currentOrigin,actual,e.currentOrigin);VectorAdd(e.s.pos.trBase,actual,e.s.pos.trBase);
 VectorAdd(e.pos1,actual,e.pos1);VectorAdd(e.pos2,actual,e.pos2);
 return moverFraction==1;
}
bool G_JAR_MoverRotate(gentity_t&,const vec3_t){return false;}
#include "game/g_mover_motion.inc"

class ChunkSave : public ojk::ISavedGame {
 std::vector<byte> data;size_t offset=0,index=0;
public:
 std::vector<std::pair<uint32_t,std::vector<byte>>> chunks;
 bool read_chunk(uint32_t id)override{CHECK(index<chunks.size()&&chunks[index].first==id);data=chunks[index++].second;offset=0;return true;}
 bool write_chunk(uint32_t id)override{chunks.push_back({id,data});return true;}
 bool is_all_data_read()const override{return offset==data.size();}
 void ensure_all_data_read()override{CHECK(is_all_data_read());}
 bool read(void*p,int n)override{CHECK(n>=0&&offset+n<=data.size());if(n)memcpy(p,data.data()+offset,n);offset+=n;return true;}
 bool write(const void*p,int n)override{CHECK(n>=0);if(n){auto*b=static_cast<const byte*>(p);data.insert(data.end(),b,b+n);}return true;}
 bool skip(int n)override{CHECK(n>=0&&offset+n<=data.size());offset+=n;return true;}
 void save_buffer()override{throw std::runtime_error("unexpected buffer operation");}
 void load_buffer()override{throw std::runtime_error("unexpected buffer operation");}
 const void*get_buffer_data()const override{return data.data();}
 int get_buffer_size()const override{return int(data.size());}
 void reset_buffer()override{data.clear();offset=0;}
 void reset_buffer_offset()override{offset=0;}
 bool is_failed()const override{return false;}void clear_error()override{}
 void throw_error()override{throw std::runtime_error("save failure");}
};
void QDECL Fail(int,const char*,...){throw std::runtime_error("native validation rejected");}
int main(){return RunTests("native holds, freeze save/load, trajectory restoration and actor collisions",[]{
 gi.Error=Fail;gi.linkentity=[](gentity_t*){};
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*t={};t->fraction=1;VectorCopy(end,t->endpos);};
 auto&npc=g_entities[7];gclient_t client{};npc.inuse=qtrue;npc.s.number=7;npc.client=&client;npc.health=100;client.ps.gravity=800;
 level.time=100;held=7;nativeBegin(npc,false);CHECK(client.ps.gravity==0&&client.ps.eFlags&EF_FORCE_GRIPPED);CHECK(npc.health==100);
 nativeFrame(.05f);CHECK(npc.currentOrigin[0]>51&&npc.currentOrigin[0]<52);CHECK(nativeControls[7].velocity[0]==1024);
 nativeRelease(7,true);CHECK(client.ps.gravity==800&&!(client.ps.eFlags&EF_FORCE_GRIPPED));CHECK(client.ps.velocity[0]==1024&&npc.health==100);
 // Freezing cancels the throw, survives a save and restores preexisting flags.
 client.ps.eFlags|=EF_FORCE_GRIPPED;nativeBegin(npc,false);nativeFrame(.05f);nativeBegin(npc,true);
 ChunkSave save;ojk::SavedGameHelper h(&save);nativeSave(false,h);nativeControls.clear();nativeRead(false,h);
 CHECK(nativeControls.size()==1&&nativeControls[7].frozen);nativeRelease(7,true);CHECK(client.ps.eFlags&EF_FORCE_GRIPPED);CHECK(client.ps.velocity[0]==0&&client.ps.gravity==800);
 // Player entity zero is not a sentinel and is serializable/freezeable.
 auto&player=g_entities[0];gclient_t playerClient{};player.inuse=qtrue;player.s.number=0;player.client=&playerClient;playerClient.ps.gravity=600;
 nativeBegin(player,true);CHECK(nativeControls.count(0));nativeRelease(0,false);CHECK(playerClient.ps.gravity==600);
 {
  auto&brush=g_entities[12];brush.inuse=qtrue;brush.s.number=12;brush.bmodel=qtrue;
  brush.s.eType=ET_MOVER;brush.s.pos.trType=TR_LINEAR_STOP;brush.s.pos.trDelta[0]=40;
  brush.s.pos.trTime=100;brush.s.pos.trDuration=1000;brush.pos2[0]=40;
  held=12;nativeBegin(brush,false);
  moverFraction=.5f;nativeFrame(.05f);
  CHECK(std::fabs(brush.currentOrigin[0]-25.6f)<.001f&&brush.pos1[0]==0&&brush.pos2[0]==40);
  CHECK(std::fabs(nativeControls[12].velocity[0]-512)<.001f);
  moverFraction=0;nativeFrame(.05f);
  CHECK(std::fabs(brush.currentOrigin[0]-25.6f)<.001f&&!VectorLength(nativeControls[12].velocity));
  level.time+=50;nativeRelease(12,true);held=ENTITYNUM_NONE;moverFraction=1;
  CHECK(brush.s.pos.trType==TR_LINEAR_STOP&&brush.s.pos.trTime==150&&brush.s.pos.trDelta[0]==40);
  CHECK(std::fabs(brush.s.pos.trBase[0]-25.6f)<.001f&&std::fabs(brush.pos1[0]-25.6f)<.001f&&std::fabs(brush.pos2[0]-65.6f)<.001f);
  level.time=100;
 }
 {
  auto& hull=g_entities[20];auto& pilot=g_entities[21];auto& droid=g_entities[22];
  gclient_t hullClient{},pilotClient{},droidClient{};
  vehicleInfo_t info{};info.type=VH_FIGHTER;
  Vehicle_t vehicle{};vehicle.m_pVehicleInfo=&info;vehicle.m_pParentEntity=&hull;
  vehicle.m_pPilot=&pilot;vehicle.m_pDroidUnit=&droid;vehicle.m_iDroidUnitTag=5;
  hull.s.number=20;hull.inuse=qtrue;hull.linked=qtrue;hull.health=100;
  hull.client=&hullClient;hullClient.NPC_class=CLASS_VEHICLE;hullClient.ps.gravity=800;
  hull.m_pVehicle=&vehicle;hull.playerModel=0;hull.crotchBolt=4;
  pilot.s.number=21;pilot.client=&pilotClient;droid.s.number=22;droid.client=&droidClient;
  globals.num_entities=23;
  for(auto* rider:{&pilot,&droid}){
   rider->owner=&hull;rider->s.m_iVehicleNum=20;rider->inuse=qtrue;rider->health=100;
  }
  pilot.linked=qtrue;droid.linked=qfalse;
  VectorSet(pilot.mins,-1,-1,0);VectorSet(pilot.maxs,1,1,2);
  VectorSet(pilot.currentOrigin,10,0,0);VectorSet(droid.currentOrigin,0,5,0);
  gi.linkentity=[](gentity_t* entity){entity->linked=qtrue;};
  gi.unlinkentity=[](gentity_t* entity){entity->linked=qfalse;};
  static bool boltMissing=false,checkFloor=false,blockSeatArc=false;
  static int blockedMember=-1;
  gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int model,int bolt,mdxaBone_t* matrix,const vec3_t angles,
     const vec3_t origin,int,qhandle_t*,const vec3_t){
   CHECK(model==0&&(bolt==4||bolt==5));
   if(boltMissing&&bolt==5)return qfalse;
   vec3_t axes[3],position;AnglesToAxis(angles,axes);
   VectorMA(origin,bolt==4?10:5,axes[bolt==4?0:1],position);
   for(int k=0;k<3;++k)matrix->matrix[k][3]=position[k];
   return qtrue;
  };
  gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t& matrix,Eorientations mode,vec3_t& out){
   CHECK(mode==ORIGIN);for(int k=0;k<3;++k)out[k]=matrix.matrix[k][3];
  };
  const auto oldTrace=gi.trace;
  gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t lo,const vec3_t hi,const vec3_t end,int pass,int,EG2_Collision,int){
   CHECK(!g_entities[20].linked&&!g_entities[21].linked&&!g_entities[22].linked);
   *hit={};hit->fraction=pass==blockedMember?.5f:1;VectorCopy(end,hit->endpos);
   if(checkFloor&&std::min(start[2],end[2])+lo[2]<-.001f)hit->startsolid=qtrue;
   if(blockSeatArc&&pass==21){
    const vec3_t obstacle={9.8f,0,0};
    bool inside=true;
    for(int i=0;i<3;++i)
     inside=inside&&obstacle[i]>=std::min(start[i],end[i])+lo[i]&&obstacle[i]<=std::max(start[i],end[i])+hi[i];
    if(inside)hit->startsolid=qtrue;
   }
  };
  const vec3_t goal={40,0,30},facing={0,90,0},expectedPilot={40,10,30},expectedDroid={35,0,30};
  CHECK(nativeVehicleHoldReady(hull));vehicle.m_iBoarding=1;CHECK(!nativeVehicleHoldReady(hull));vehicle.m_iBoarding=0;
  nativeBegin(hull,false);held=20;
  blockedMember=22;
  CHECK(!nativeMoveVehicle(hull,goal,facing,level.time)&&hull.currentOrigin[0]==0&&pilot.currentOrigin[0]==10);
  CHECK(hull.linked&&pilot.linked&&!droid.linked&&vehicle.m_vOrientation[YAW]==0);
  blockedMember=-1;boltMissing=true;CHECK(!nativeMoveVehicle(hull,goal,facing,level.time)&&hull.currentOrigin[0]==0);
  boltMissing=false;

  // A level turn about an off-center contact cannot expand below the floor.
  const vec3_t yawPivot={-32,0,0},yawGoal={-32,32,0},yawSeat={-32,42,0};
  checkFloor=true;
  CHECK(nativeMoveVehicle(hull,yawGoal,facing,level.time,yawPivot));
  CHECK(Distance(hull.currentOrigin,yawGoal)<.001f&&Distance(pilot.currentOrigin,yawSeat)<.001f);
  checkFloor=false;

  // The pilot's pitch arc hits a blocker that the start-to-finish chord misses.
  const vec3_t pitchPivot={-20,0,0},pitchStart={-5.857864f,0,14.142136f},pitchEnd={-5.857864f,0,-14.142136f};
  const vec3_t noseUp={-45,0,0},noseDown={45,0,0};
  CHECK(nativeMoveVehicle(hull,pitchStart,noseUp,level.time));
  vec3_t originalPilot,originalDroid;
  VectorCopy(pilot.currentOrigin,originalPilot);VectorCopy(droid.currentOrigin,originalDroid);
  blockSeatArc=true;
  CHECK(!nativeMoveVehicle(hull,pitchEnd,noseDown,level.time,pitchPivot));
  CHECK(VectorCompare(hull.currentOrigin,pitchStart)&&VectorCompare(vehicle.m_vOrientation,noseUp));
  CHECK(VectorCompare(pilot.currentOrigin,originalPilot)&&VectorCompare(droid.currentOrigin,originalDroid));
  CHECK(hull.linked&&pilot.linked&&!droid.linked);
  blockSeatArc=false;
  CHECK(nativeMoveVehicle(hull,pitchEnd,noseDown,level.time,pitchPivot));
  CHECK(VectorCompare(hull.currentOrigin,pitchEnd)&&VectorCompare(vehicle.m_vOrientation,noseDown));

  CHECK(nativeMoveVehicle(hull,goal,facing,level.time));
  CHECK(Distance(hull.currentOrigin,goal)<.001f&&Distance(pilot.currentOrigin,expectedPilot)<.001f);
  CHECK(Distance(droid.currentOrigin,expectedDroid)<.001f&&droid.owner==&hull&&droid.s.m_iVehicleNum==20);
  CHECK(hull.linked&&pilot.linked&&!droid.linked&&VectorCompare(vehicle.m_vOrientation,facing));
  CHECK(VectorCompare(vehicle.m_vPrevOrientation,facing)&&VectorCompare(hullClient.ps.viewangles,facing));
  nativeFrame(.05f);CHECK(nativeControls[20].velocity[0]>0);
  const float speed=nativeControls[20].velocity[0];nativeRelease(20,true);
  CHECK(hullClient.ps.gravity==800&&hullClient.ps.velocity[0]==speed&&vehicle.jarPushDrift[0]==speed);
  CHECK(vehicle.jarPushDrift[2]==0&&hull.health==100&&pilot.health==100&&droid.health==100);
  CHECK(!nativeControlledRider(pilot)&&!nativeControlledRider(droid));
  held=ENTITYNUM_NONE;nativeBegin(hull,true);
  CHECK(nativeControlledRider(pilot)&&nativeControlledRider(droid)&&!nativeControls.count(21)&&!nativeControls.count(22));
  pilot.owner=nullptr;CHECK(!nativeControlledRider(pilot));pilot.owner=&hull;
  const vec3_t parked={85,20,30};CHECK(nativeMoveVehicle(hull,parked,facing,level.time));
  vec3_t savedSeat;VectorCopy(pilot.currentOrigin,savedSeat);
  ChunkSave frozenSave;ojk::SavedGameHelper frozenWriter(&frozenSave);nativeSave(false,frozenWriter);
  MemorySave vehicleSave;ojk::SavedGameHelper vehicleIO(&vehicleSave);
  vehicle.sg_export(vehicleIO);vehicleSave.reset_buffer_offset();
  VectorClear(vehicle.m_vOrientation);VectorClear(vehicle.m_vPrevOrientation);
  vehicle.sg_import(vehicleIO);vehicleIO.ensure_all_data_read();
  // The full engine save resolves the vehicle's pointer fields through its entity table.
  vehicle.m_pParentEntity=&hull;vehicle.m_pVehicleInfo=&info;vehicle.m_pPilot=&pilot;vehicle.m_pDroidUnit=&droid;
  nativeControls.clear();nativeRead(false,frozenWriter);
  CHECK(nativeControls[20].frozen&&nativeControlledRider(pilot)&&nativeControlledRider(droid));
  CHECK(VectorCompare(vehicle.m_vOrientation,facing)&&VectorCompare(vehicle.m_vPrevOrientation,facing));
  nativeFrame(.05f);CHECK(VectorCompare(hull.currentOrigin,parked)&&VectorCompare(pilot.currentOrigin,savedSeat));
  vehicle.m_pPilot=nullptr;CHECK(!nativeControlledRider(pilot)&&nativeControlledRider(droid));
  vehicle.m_iBoarding=1;nativeFrame(.05f);
  CHECK(!nativeControls.count(20)&&!nativeControlledRider(droid)&&hullClient.ps.gravity==800);
  gi.trace=oldTrace;
  hull.inuse=pilot.inuse=droid.inuse=qfalse;hull.m_pVehicle=nullptr;
 }
 // Movers keep their controller and translated/rotated path and paused timing.
 auto&mover=g_entities[8];mover.inuse=qtrue;mover.s.number=8;mover.s.eType=ET_MOVER;mover.s.pos.trType=TR_LINEAR;
 mover.s.pos.trDelta[0]=10;mover.s.pos.trTime=100;mover.s.apos.trBase[1]=20;mover.currentAngles[1]=20;mover.nextthink=200;
 nativeBegin(mover,false);mover.currentOrigin[0]=40;mover.currentAngles[1]=50;level.time=600;nativeRelease(8,true);
 CHECK(mover.s.pos.trType==TR_LINEAR&&mover.s.pos.trBase[0]==40&&mover.s.pos.trDelta[0]==10&&mover.s.pos.trTime==600);
 CHECK(mover.nextthink==700&&mover.s.apos.trBase[1]==50);
 // Physgun translation rebases authored train waypoints, including trains
 // that have never received a Thruster. Grabbing cancels external momentum.
 mover.classname=const_cast<char*>("func_train");nativeBegin(mover,false);mover.currentOrigin[0]+=25;nativeRelease(8,false);
 JarMoverMotion motion;CHECK(G_JAR_MoverMotionRead(mover,motion)&&motion.offset[0]==25);
 motion.velocity[0]=80;motion.angular[2]=2;CHECK(G_JAR_MoverMotionRestore(mover,motion));nativeBegin(mover,false);
 CHECK(G_JAR_MoverMotionRead(mover,motion)&&motion.velocity[0]==0&&motion.angular[2]==0);mover.currentOrigin[0]-=5;nativeRelease(8,false);
 CHECK(G_JAR_MoverMotionRead(mover,motion)&&motion.offset[0]==20);
 // ROFF pos1 is an origin accumulator; pos2 contains angles, not an endpoint.
 mover.next_roff_time=1000;VectorSet(mover.pos1,1,2,3);VectorSet(mover.pos2,10,20,30);
 nativeBegin(mover,false);mover.currentOrigin[0]+=7;mover.currentAngles[1]+=15;level.time+=100;nativeRelease(8,false);
 CHECK(mover.pos1[0]==8&&mover.pos1[1]==2&&mover.pos2[0]==10&&mover.pos2[1]==35&&mover.pos2[2]==30&&mover.next_roff_time==1100);
 // Generic objects acquire the native falling controller; scripts are retained.
 auto&object=g_entities[9];object.inuse=qtrue;object.s.number=9;nativeBegin(object,false);nativeControls[9].velocity[1]=50;nativeRelease(9,true);
 CHECK(object.e_ThinkFunc==thinkF_G_RunObject&&object.s.pos.trType==TR_GRAVITY&&object.s.pos.trDelta[1]==50);
 object.e_ThinkFunc=thinkF_G_FreeEntity;nativeBegin(object,true);nativeRelease(9,true);CHECK(object.e_ThinkFunc==thinkF_G_FreeEntity);
 // Empty/autosave records consume both chunks. Corrupt floats reject atomically.
 ChunkSave empty;ojk::SavedGameHelper eh(&empty);nativeSave(true,eh);nativeRead(true,eh);CHECK(nativeControls.empty());
 nativeBegin(npc,true);nativeControls[7].original[0]=std::numeric_limits<float>::quiet_NaN();ChunkSave bad;ojk::SavedGameHelper bh(&bad);nativeSave(false,bh);nativeControls.clear();
 bool rejected=false;try{nativeRead(false,bh);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected&&nativeControls.empty());
 // Frozen sine and world-axis controllers retain complete serialized paths.
 for(auto type:{TR_SINE,TR_AXIS_LINEAR,TR_AXIS_SINE,TR_AXIS_STATIONARY}){
  mover.next_roff_time=0;mover.s.apos={};mover.s.apos.trType=type;mover.s.apos.trTime=100;mover.s.apos.trDuration=2400;VectorSet(mover.s.apos.trBase,30,20,10);VectorSet(mover.s.apos.trDelta,15,-30,40);mover.s.pos.trType=TR_SINE;mover.s.pos.trDuration=1200;
  level.time=900;if(!jar::angular::Evaluate(mover.s.apos,level.time,mover.currentAngles))VectorCopy(mover.s.apos.trBase,mover.currentAngles);vec3_t before;VectorCopy(mover.currentAngles,before);const auto path=mover.s.apos;
  nativeBegin(mover,true);CHECK(mover.s.apos.trType==(jar::angular::Path(type)?TR_AXIS_INTERPOLATE:TR_INTERPOLATE));
  ChunkSave frozen;ojk::SavedGameHelper frozenHelper(&frozen);nativeSave(false,frozenHelper);nativeControls.clear();nativeRead(false,frozenHelper);CHECK(nativeControls.at(8).rotation.trType==type&&nativeControls.at(8).position.trType==TR_SINE);
  const vec3_t desired={-40,60,27};G_SetAngles(&mover,desired);level.time=1900;nativeRelease(8,false);CHECK(mover.s.apos.trType==type&&mover.s.apos.trTime==1100);
  if(jar::angular::Path(type)){vec3_t original,expected,actual;jar::angular::Evaluate(path,1400,original);jar::angular::RebaseAngles(original,before,desired,expected);jar::angular::Evaluate(mover.s.apos,2400,actual);vec3_t ea[3],aa[3];AnglesToAxis(expected,ea);AnglesToAxis(actual,aa);for(int a=0;a<3;++a)CHECK(Distance(ea[a],aa[a])<.0001f);}
 }
 using namespace jar::physics;ActorContact c;
 float old[3]={-100,0,30},now[3]={100,0,30},mins[3]={-10,-10,-10},maxs[3]={10,10,10},amin[3]={-16,-16,0},amax[3]={16,16,64};
 CHECK(SweepActor(old,now,mins,maxs,amin,amax,c)&&c.normal[0]==1&&c.depth>125); // tunneling
 now[1]=old[1]=100;CHECK(!SweepActor(old,now,mins,maxs,amin,amax,c));
 now[1]=old[1]=0;now[0]=old[0]=-20;CHECK(SweepActor(old,now,mins,maxs,amin,amax,c)&&c.depth==6&&c.normal[0]==1);
 // Momentum conservation with different masses, and separating contacts.
 for(float mass:{10.f,80.f,800.f}){float impulse=ContactImpulse(mass,80,100);float pv=100-impulse/mass,av=impulse/80;
  CHECK(std::fabs(pv-av)<.001f&&std::fabs(mass*pv+80*av-mass*100)<.01f);}
 CHECK(ContactImpulse(80,80,-1)==0);
 });}
