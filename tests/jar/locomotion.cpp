// Exercise PM_SetAnimFinal with independent torso and leg animation clocks.
#include "cgame/cg_headers.h"
#include "game/bg_local.h"
#include "game/anims.h"
#include "test_support.h"
#include <vector>
#include <cmath>
cg_t cg{};level_locals_t level{};game_import_t gi{};
qboolean player_locked=qfalse,MatrixMode=qfalse;
static cvar_t yes{},no{};
cvar_t *g_timescale=&yes,*g_synchSplitAnims=&yes,*g_noFootSlide=&no,*g_saberAnimSpeed=&yes,*g_AnimWarning=&no;
vmCvar_t cg_debugAnim{},cg_debugAnimTarget{};
qboolean ValidAnimFileIndex(int i){return i==0?qtrue:qfalse;}
qboolean Q3_TaskIDPending(gentity_t*,taskID_t){return qfalse;}
void Q3_TaskIDComplete(gentity_t*,taskID_t){}
struct TestPool : IGhoul2InfoArray {
 std::vector<CGhoul2Info> models;
 int New()override{return 1;}void Delete(int)override{models.clear();}
 bool IsValid(int h)const override{return h==1;}
 std::vector<CGhoul2Info>&Get(int)override{return models;}
 const std::vector<CGhoul2Info>&Get(int)const override{return models;}
};
IGhoul2InfoArray&_TheGhoul2InfoArray(){static TestPool pool;return pool;}
float PM_GetTimeScaleMod(gentity_t*){return 1;}
void PM_SaberStartTransAnim(int,int,float*,gentity_t*){}
qboolean PM_RunningAnim(int anim){return anim==BOTH_RUN1?qtrue:qfalse;}
qboolean PM_WalkingAnim(int anim){return anim==BOTH_WALK1?qtrue:qfalse;}
void PM_SetLegsAnimTimer(gentity_t*,int*timer,int value){*timer=value;}
void PM_SetTorsoAnimTimer(gentity_t*,int*timer,int value){*timer=value;}
#include "game/bg_setanim.inc"
static float legsFrame=106,torsoFrame=101,legsSpeed=1;static int bodyWrites=0;
int main(){return RunTests("running and walking preserve leg phase while torso clocks differ",[]{
 yes.integer=1;yes.value=1;cg.time=1000;
 gi.G2API_HaveWeGhoul2Models=[](CGhoul2Info_v&){return qtrue;};
 gi.G2API_GetBoneAnimIndex=[](CGhoul2Info*,int bone,int,float*frame,int*start,int*end,int*flags,float*speed,qhandle_t*){
  *frame=bone==0?legsFrame:torsoFrame;*start=100;*end=120;*flags=BONE_ANIM_OVERRIDE_LOOP;*speed=bone==0?legsSpeed:1;return qtrue;};
 gi.G2API_SetAnimIndex=[](CGhoul2Info*,int){return qtrue;};
 gi.G2API_SetBoneAnimIndex=[](CGhoul2Info*,int bone,int,int,int,float speed,int,float frame,int){if(bone==0){++bodyWrites;legsFrame=frame;legsSpeed=speed;}return qtrue;};
 gentity_t entity{};gclient_t client{};entity.client=&client;entity.playerModel=0;entity.rootBone=0;entity.lowerLumbarBone=1;entity.motionBone=-1;entity.ghoul2.resize(1);
 for(int anim:{BOTH_RUN1,BOTH_WALK1}){
  auto&a=level.knownAnimFileSets[0].animations[anim];a.firstFrame=100;a.numFrames=20;a.frameLerp=50;a.loopFrames=0;
  client.ps.legsAnim=client.ps.torsoAnim=anim;client.ps.legsAnimTimer=client.ps.torsoAnimTimer=0;
  legsSpeed=1;bodyWrites=0;
  for(int frame=0;frame<80;++frame){cg.time+=50;legsFrame=100+float(frame%20);torsoFrame=100+float((frame+7)%20);
   PM_SetAnimFinal(&client.ps.torsoAnim,&client.ps.legsAnim,SETANIM_LEGS,anim,0,&client.ps.torsoAnimTimer,&client.ps.legsAnimTimer,&entity,150);
   CHECK(legsFrame==100+float(frame%20));}
  CHECK(bodyWrites==0);
  // A speed change updates playback but must retain the advanced leg frame.
  legsFrame=117;torsoFrame=102;legsSpeed=.8f;
  PM_SetAnimFinal(&client.ps.torsoAnim,&client.ps.legsAnim,SETANIM_LEGS,anim,0,&client.ps.torsoAnimTimer,&client.ps.legsAnimTimer,&entity,150);
  CHECK(bodyWrites==1&&legsFrame==117&&legsSpeed==1);
 }
 // With native no-foot-slide enabled, stale server displacement must never
 // pause a moving player's gait. The two clocks deliberately disagree.
 g_noFootSlide=&yes;entity.s.number=0;client.NPC_class=CLASS_PLAYER;
 for(int anim:{BOTH_RUN1,BOTH_WALK1}){
  client.ps.legsAnim=client.ps.torsoAnim=anim;client.ps.velocity[0]=anim==BOTH_WALK1?50:150;
  client.ps.velocity[1]=0;client.ps.velocity[2]=700;legsSpeed=1;bodyWrites=0;legsFrame=100;
  float advanced=0;
  for(int tick=0;tick<240;++tick){cg.time+=16;entity.resultspeed=tick%2?0:500;client.ps.velocity[0]=float(tick%3)*137;torsoFrame=101;
   legsFrame+=legsSpeed*.32f;advanced+=legsSpeed*.32f;if(legsFrame>=120)legsFrame-=20;
   const float before=legsFrame;
   PM_SetAnimFinal(&client.ps.torsoAnim,&client.ps.legsAnim,SETANIM_LEGS,anim,0,&client.ps.torsoAnimTimer,&client.ps.legsAnimTimer,&entity,150);
   CHECK(legsFrame==before && legsSpeed==1);
  }
  CHECK(advanced>76 && bodyWrites==0); // More than three complete cycles.
  entity.s.number=1;entity.resultspeed=25;client.ps.velocity[0]=0;
  PM_SetAnimFinal(&client.ps.torsoAnim,&client.ps.legsAnim,SETANIM_LEGS,anim,0,&client.ps.torsoAnimTimer,&client.ps.legsAnimTimer,&entity,150);
  CHECK(std::fabs(legsSpeed-(anim==BOTH_WALK1?.5f:1.f/6))<.001f);
  entity.s.number=0;
 }
 });}
