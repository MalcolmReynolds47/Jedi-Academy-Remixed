// Exercise the production scheduler and native serialized fields without a game.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include <deque>
#include <set>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static std::set<int> controlled;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
gentity_t*G_Find(gentity_t*,int,const char*name){for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(e.inuse&&e.targetname&&!strcmp(e.targetname,name))return &e;}return nullptr;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;}
int G_ModelIndex(const char*){return 5;}
bool G_JAR_WorkshopControls(const gentity_t*e){return e&&controlled.count(e->s.number);}
static vec3_t externalOffset{};
void G_JAR_MoverPathOffset(const gentity_t&,vec3_t out){VectorCopy(externalOffset,out);}
#include "game/g_rail_portable.inc"
static int pushed[1],*pushed_p,pushCalls=0;
void EvaluateTrajectory(const trajectory_t*t,int now,vec3_t out){VectorCopy(t->trBase,out);if(t->trType==TR_LINEAR_STOP){const float dt=std::max(0,std::min(now-t->trTime,t->trDuration))*.001f;VectorMA(out,dt,t->trDelta,out);}}
qboolean G_MoverPush(gentity_t*e,vec3_t move,vec3_t angles,gentity_t**,bool,const float (*)[3],bool=false){++pushCalls;VectorAdd(e->currentOrigin,move,e->currentOrigin);VectorAdd(e->currentAngles,angles,e->currentAngles);return qtrue;}
void GEntity_ReachedFunc(gentity_t*){}
void GEntity_BlockedFunc(gentity_t*,gentity_t*){}
#include "game/g_mover_team.inc"

static gentity_t&Make(int n,const char*name,const char*target){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.s.groundEntityNum=ENTITYNUM_NONE;e.classname=G_NewString(name);e.targetname=G_NewString(target);e.model=G_NewString("@jar_rail");CHECK(G_RailSpawn(e));return e;}
int main(){return RunTests("portable rail scheduling, routes, collision admission, riders, pause and save fields",[]{
 globals.num_entities=20;level.time=1000;level.previousTime=900;
 gi.linkentity=[](gentity_t*){};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static bool solid=false;static int traces=0;static float travelFraction=1;
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t lo,const vec3_t hi,const vec3_t end,int pass,int mask,EG2_Collision,int){CHECK(pass>=3&&(mask==(MASK_SOLID|CONTENTS_BODY)||mask==MASK_SOLID)&&lo[0]<hi[0]&&(mask==MASK_SOLID||VectorCompare(start,end)));++traces;*t={};t->fraction=mask==MASK_SOLID?travelFraction:1;t->startsolid=solid?qtrue:qfalse;};
 auto&track=Make(1,"rail_track","track");auto&lane=Make(2,"rail_lane","lane");auto&mover=Make(3,"rail_mover","mover");lane.target=track.targetname;mover.target=lane.targetname;VectorSet(lane.currentOrigin,0,64,0);
 CHECK(track.s.eType==ET_GENERAL&&lane.contents==CONTENTS_ITEM&&mover.s.eType==ET_MOVER&&mover.contents==CONTENTS_SOLID&&mover.mass==150);
 jar::rail::Settings t,l,m;CHECK(G_RailRead(track,t)&&G_RailRead(lane,l)&&G_RailRead(mover,m));m.center=true;G_RailSet(mover,m);
 solid=true;G_RailPortableFrame();CHECK(!mover.count&&traces==1&&!railLookup);solid=false;level.time+=500;G_RailPortableFrame();CHECK(mover.count==1&&mover.s.pos.trType==TR_LINEAR_STOP&&mover.currentOrigin[0]==0&&mover.currentOrigin[1]==64&&mover.currentOrigin[2]==32&&mover.pos1[0]==512&&mover.s.pos.trDuration==4000);
 vec3_t corners[4];CHECK(G_RailPreview(mover,corners)&&corners[0][1]==0&&corners[2][1]==128);
 // Full swept hull is required for portable rails, while authored movers retain behavior.
 vec3_t ahead={16,64,32};CHECK(G_RailCanMove(mover,ahead));travelFraction=.5f;CHECK(!G_RailCanMove(mover,ahead));travelFraction=1;solid=true;CHECK(!G_RailCanMove(mover,ahead));solid=false;CHECK(G_RailCanMove(track,ahead));mover.model=G_NewString("authored_rail");travelFraction=0;CHECK(G_RailCanMove(mover,ahead));travelFraction=1;mover.model=G_NewString("@jar_rail");
 // Execute the native team transaction: a blocked wall pauses its clock, then resumes.
 const int tripTime=mover.s.pos.trTime;level.previousTime=level.time;level.time+=100;travelFraction=.5f;G_MoverTeam(&mover);CHECK(!pushCalls&&mover.currentOrigin[0]==0&&mover.s.pos.trTime==tripTime+100&&(mover.s.eFlags&EF_BLOCKED_MOVER));level.previousTime=level.time;level.time+=100;travelFraction=1;G_MoverTeam(&mover);CHECK(pushCalls==1&&std::fabs(mover.currentOrigin[0]-12.8f)<.001f&&!(mover.s.eFlags&EF_BLOCKED_MOVER));
 // Native pusher owns actual motion. Disabling stops at the last committed position.
 mover.currentOrigin[0]=100;t.enabled=false;G_RailSet(track,t);G_RailPortableFrame();CHECK(mover.count==2&&mover.s.pos.trType==TR_STATIONARY&&mover.currentOrigin[0]==100);
 t.enabled=true;t.speed=256;G_RailSet(track,t);G_RailPortableFrame();CHECK(mover.count==1&&mover.s.pos.trDuration==1610&&mover.s.pos.trDelta[0]==256);
 controlled.insert(2);G_RailPortableFrame();CHECK(mover.count==2);controlled.clear();controlled.insert(3);G_RailPortableFrame();CHECK(mover.count==2);controlled.clear();G_RailPortableFrame();CHECK(mover.count==1);
 lane.inuse=qfalse;G_RailPortableFrame();CHECK(mover.count==2);lane.inuse=qtrue;lane.targetname=G_NewString("reused_slot");G_RailPortableFrame();CHECK(mover.count==2);lane.targetname=mover.target;G_RailPortableFrame();CHECK(mover.count==1);
 // Do not stop before the native pusher has committed the final frame.
 const int end=mover.s.pos.trTime+mover.s.pos.trDuration;level.time=end+1;level.previousTime=end-1;G_RailPortableFrame();CHECK(mover.count==1);
 mover.currentOrigin[0]=512;auto&rider=g_entities[4];rider.inuse=qtrue;rider.s.number=4;rider.s.groundEntityNum=3;level.previousTime=end;G_RailPortableFrame();CHECK(!mover.count&&mover.currentOrigin[0]==512);
 const int noRecycle=traces;level.time+=1000;G_RailPortableFrame();CHECK(!mover.count&&traces==noRecycle);rider.inuse=qfalse;level.time+=1000;G_RailPortableFrame();CHECK(mover.count==1&&mover.currentOrigin[0]==0);
 // Spacing permits at most the requested count per row; delay postpones trips.
 auto&second=Make(5,"rail_mover","second");auto&third=Make(6,"rail_mover","third");second.target=third.target=track.targetname;t.count=1;t.delay=1;G_RailSet(track,t);G_RailPortableFrame();CHECK(!second.count&&!third.count&&mover.count==2);level.time+=1000;G_RailPortableFrame();CHECK((second.count==1)+(third.count==1)==1);G_RailPortableFrame();CHECK((second.count==1)+(third.count==1)==1);level.time+=1000;G_RailPortableFrame();CHECK(second.count&&third.count);
 // Rotated track/lane preview and offset; admission accounts for asymmetric hulls.
 VectorSet(track.currentAngles,30,90,20);vec3_t axis[3];AnglesToAxis(track.currentAngles,axis);VectorMA(track.currentOrigin,64,axis[1],lane.currentOrigin);t.delay=0;t.count=8;G_RailSet(track,t);CHECK(G_RailPreview(lane,corners));vec3_t d;VectorSubtract(corners[1],corners[0],d);CHECK(std::fabs(VectorLength(d)-512)<.001f);
 railStop(mover,0);VectorSet(externalOffset,15,25,35);CHECK(railLaunch(mover,track,&lane));VectorSubtract(mover.pos1,mover.pos2,d);CHECK(std::fabs(VectorLength(d)-512)<.001f);VectorClear(externalOffset);
 auto bad=t;bad.cell=33.5f;CHECK(!G_RailSet(track,bad));bad=t;bad.width=63;CHECK(!G_RailSet(track,bad));bad=t;bad.count=9;CHECK(!G_RailSet(track,bad));bad=t;bad.speed=std::numeric_limits<float>::quiet_NaN();CHECK(!G_RailSet(track,bad));
 portable_rail_use(&track,nullptr,nullptr);CHECK((track.spawnflags&2)&&!(track.svFlags&SVF_INACTIVE));portable_rail_use(&track,nullptr,nullptr);CHECK(!(track.spawnflags&2)&&!(track.svFlags&SVF_INACTIVE));
 // Round-trip the actual entity record, including endpoints, trajectory and callback.
 MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t loaded{};mover.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.count==mover.count&&loaded.e_UseFunc==useF_portable_rail_use&&loaded.s.pos.trType==TR_LINEAR_STOP&&loaded.s.pos.trDuration==mover.s.pos.trDuration&&VectorCompare(loaded.pos1,mover.pos1)&&VectorCompare(loaded.pos2,mover.pos2));
 CHECK(G_RailRestore());mover.count=3;CHECK(!G_RailRestore());mover.count=1;mover.e_UseFunc=useF_NULL;CHECK(!G_RailRestore());mover.e_UseFunc=useF_portable_rail_use;mover.pos1[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!G_RailRestore());
 mover.model=G_NewString("authored_rail");CHECK(!G_RailPortable(mover)&&!G_RailMoved(mover)&&G_RailRestore());
 });}
