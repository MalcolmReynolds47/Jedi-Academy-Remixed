// Production door-camera assembly/controller and cgame scale interpolation.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include <deque>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];cg_t cg{};
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static int nextChild=20;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
int G_ModelIndex(const char*){return 7;}int G_SoundIndex(const char*s){CHECK(!strcmp(s,"sound/movers/objects/cameramove_lp2.wav"));return 9;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);}
gentity_t*G_JAR_SpawnSetupChild(gentity_t*){auto&e=g_entities[nextChild];e.inuse=qtrue;e.s.number=nextChild++;globals.num_entities=nextChild;return &e;}
#include "game/g_door_camera.inc"
#include "cgame/cg_door_camera.inc"
int main(){return RunTests("door camera assembly, obstruction, interpolation, lifetime and save fields",[]{
 level.time=1000;gi.linkentity=[](gentity_t*){};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static bool solid=false;static float fraction=1;static int traces=0;
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){++traces;CHECK(mins&&maxs&&mins[0]==-14&&maxs[0]==14&&pass==2&&mask==(MASK_SOLID|CONTENTS_BODY));*t={};t->fraction=fraction;t->startsolid=solid?qtrue:qfalse;for(int a=0;a<3;++a)t->endpos[a]=start[a]+fraction*(end[a]-start[a]);};
 auto&e=g_entities[2];e.inuse=qtrue;e.s.number=2;e.classname=G_NewString("misc_model_jabba_cam");e.model=G_NewString("@jar_door_camera");VectorSet(e.currentOrigin,100,200,300);
 CHECK(G_DoorCameraSpawn(&e));auto*shaft=e.lastEnemy;auto*head=shaft->lastEnemy;CHECK(G_DoorCameraRoot(head)==&e&&G_DoorCameraRoot(shaft)==&e&&G_DoorCameraShaft(shaft)&&!G_DoorCameraShaft(head));
 CHECK(e.e_UseFunc==useF_jabba_cam_use&&e.e_ThinkFunc==thinkF_NULL&&!e.random&&e.maxs[0]==34&&e.mins[0]==-14&&head->currentOrigin[0]==120&&head->currentOrigin[2]==300&&e.mass==20);
 G_DoorCameraFrame();CHECK(!traces&&!e.s.loopSound);G_DoorCameraUse(&e);level.time+=100;G_DoorCameraFrame();CHECK(e.random==12&&head->currentOrigin[0]==132&&e.s.loopSound==9);
 // Input and duplicate calls in one frame cannot speed up the animation.
 G_DoorCameraFrame();CHECK(e.random==12);fraction=.25f;level.time+=100;G_DoorCameraFrame();CHECK(e.random==15&&e.alt_fire&&head->currentOrigin[0]==135);
 solid=true;level.time+=100;G_DoorCameraFrame();CHECK(e.random==15&&!e.s.loopSound);solid=false;fraction=1;level.time+=1000;G_DoorCameraFrame();CHECK(e.random==48&&!e.alt_fire&&!e.s.loopSound&&head->currentOrigin[0]==168);
 G_DoorCameraUse(&e);level.time+=100;G_DoorCameraFrame();CHECK(e.random==36);fraction=0;level.time+=100;G_DoorCameraFrame();CHECK(e.random==36&&e.alt_fire&&!e.s.loopSound);fraction=1;
 e.svFlags|=SVF_INACTIVE;level.time+=10000;G_DoorCameraFrame();CHECK(e.random==36);e.svFlags&=~SVF_INACTIVE;level.time+=100;G_DoorCameraFrame();CHECK(e.random==24);level.time+=200;G_DoorCameraFrame();CHECK(!e.random&&!e.s.loopSound);
 jar::doorCamera::Settings s;s.travel=96;s.speed=240;s.extended=true;CHECK(G_DoorCameraSet(e,s));level.time+=100;G_DoorCameraFrame();CHECK(e.random==24);
 VectorSet(e.currentAngles,15,45,30);G_DoorCameraMoved(e);vec3_t axis[3],poleAxis[3],tip;AnglesToAxis(e.currentAngles,axis);AnglesToAxis(shaft->currentAngles,poleAxis);VectorMA(shaft->currentOrigin,-jar::doorCamera::shaftLength*shaft->s.modelScale[2],poleAxis[2],tip);
 CHECK(Distance(tip,head->currentOrigin)<.001f);for(int a=0;a<3;++a)CHECK(std::fabs(poleAxis[0][a]-axis[2][a])<.0001f&&head->s.apos.trType==TR_INTERPOLATE&&head->s.pos.trType==TR_INTERPOLATE);
 // Current and next model snapshots use the same interpolation as the head.
 centity_t cent{};cent.gent=shaft;cent.interpolate=qtrue;entityState_t next{};cent.nextState=&next;cent.currentState.modelindex=next.modelindex=7;cent.currentState.modelScale[2]=2;next.modelScale[2]=4;cg.frameInterpolation=.25f;refEntity_t render{};render.modelScale[2]=2;CG_DoorCameraScale(&cent,render);CHECK(render.modelScale[2]==2.5f);cent.interpolate=qfalse;render.modelScale[2]=2;CG_DoorCameraScale(&cent,render);CHECK(render.modelScale[2]==2);cent.interpolate=qtrue;next.modelindex=8;CG_DoorCameraScale(&cent,render);CHECK(render.modelScale[2]==2);
 auto bad=s;bad.speed=std::numeric_limits<float>::quiet_NaN();CHECK(!G_DoorCameraSet(e,bad)&&e.speed==240);bad=s;bad.travel=7;CHECK(!G_DoorCameraSet(e,bad));s.travel=8;CHECK(G_DoorCameraSet(e,s)&&e.random==8);s.travel=96;G_DoorCameraSet(e,s);
 MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t stored{},loaded{};stored.radius=e.radius;stored.speed=e.speed;stored.random=e.random;stored.spawnflags=e.spawnflags;stored.e_UseFunc=e.e_UseFunc;stored.e_ThinkFunc=e.e_ThinkFunc;stored.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.radius==96&&loaded.speed==240&&loaded.random==8&&(loaded.spawnflags&1)&&loaded.e_UseFunc==useF_jabba_cam_use);
 level.time+=10000;G_DoorCameraRestore();G_DoorCameraFrame();CHECK(e.random==8&&!e.s.loopSound&&G_DoorCameraParts(e));level.time+=100;G_DoorCameraFrame();CHECK(e.random==32);e.random=-1;bool rejected=false;try{G_DoorCameraRestore();}catch(...){rejected=true;}CHECK(rejected);e.random=32;G_DoorCameraRestore();
 G_DoorCameraForget(head);head->inuse=qfalse;CHECK(!shaft->lastEnemy&&!head->owner&&!e.s.loopSound);head->inuse=qtrue;head->classname=G_NewString("unrelated");CHECK(!G_DoorCameraRoot(head));G_DoorCameraFrame();CHECK(!e.s.loopSound);G_DoorCameraForget(shaft);CHECK(!e.lastEnemy&&!shaft->owner);G_DoorCameraForget(&e);CHECK(portableDoorCameras.empty());
 CHECK(G_DoorCameraSpawn(&e));shaft=e.lastEnemy;head=shaft->lastEnemy;G_DoorCameraUse(&e);level.time+=50;G_DoorCameraFrame();CHECK(e.s.loopSound);G_DoorCameraReset();CHECK(!e.s.loopSound);G_DoorCameraForget(&e);CHECK(!head->owner&&!shaft->owner&&!e.lastEnemy);
 });}
