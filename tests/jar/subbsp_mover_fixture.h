// Native parser, mover spawn/state/trajectory/touch callbacks and ownership.
// Collision registration/linking, activation and graphics are service boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_mover_motion.h"
#include "test_support.h"
#include "memory_save.h"
#include <cmath>
#include <string>

game_import_t gi{}; level_locals_t level{}; gentity_t g_entities[MAX_GENTITIES];
weaponData_t weaponData[WP_NUM_WEAPONS]{}; ammoData_t ammoData[AMMO_MAX]{};
qboolean spawning=qfalse, NPCsPrecached=qtrue;
int numSpawnVars=0,numSpawnVarChars=0;
char *spawnVars[MAX_SPAWN_VARS][2],spawnVarChars[MAX_SPAWN_VARS_CHARS];
static cvar_t gravity{}; cvar_t *g_gravity=&gravity;
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected model allocation");}
void G_Error(const char*,...){throw std::runtime_error("Native spawn error");}
bool G_JAR_SuspendActorRecovery(bool){return false;}
#include "game/g_subbsp_ownership.inc"
#include "game/g_spawn_parse.inc"
#include "game/g_subbsp_transform.inc"
#include "game/g_subbsp_movers.h"
static int nextSlot,uses,matched; static gentity_t *lastSpawn;
static vec3_t brushLo={-40,-4,-12},brushHi={40,4,12};
void G_FreeEntity(gentity_t *e){if(G_SubBSPFreeInProgress(e))return;G_SubBSPFreeScope scope(e);e->inuse=qfalse;}
gentity_t *G_Spawn(){CHECK(nextSlot<MAX_GENTITIES);auto *e=&g_entities[nextSlot++];*e=gentity_t{};e->inuse=qtrue;e->s.number=int(e-g_entities);G_SubBSPAllocated(e);lastSpawn=e;return e;}
static const char *field(const char *key){for(int i=0;i<numSpawnVars;++i)if(!Q_stricmp(spawnVars[i][0],key))return spawnVars[i][1];return nullptr;}
qboolean G_SpawnString(const char *key,const char *fallback,char **out){const auto*v=field(key);*out=const_cast<char*>(v?v:fallback);return v?qtrue:qfalse;}
qboolean G_SpawnFloat(const char *key,const char *fallback,float *out){char*v;auto found=G_SpawnString(key,fallback,&v);*out=float(atof(v));return found;}
qboolean G_SpawnInt(const char *key,const char *fallback,int *out){char*v;auto found=G_SpawnString(key,fallback,&v);*out=atoi(v);return found;}
qboolean G_SpawnVector(const char *key,const char *fallback,float *out){char*v;auto found=G_SpawnString(key,fallback,&v);CHECK(sscanf(v,"%f %f %f",out,out+1,out+2)==3);return found;}
int G_ModelIndex(const char*){return 7;} int G_SoundIndex(const char*){return 8;}
void G_JAR_SetBrushModel(gentity_t *e,const char *model){e->bmodel=strcmp(model,"@jar_box")?qtrue:qfalse;VectorCopy(brushLo,e->mins);VectorCopy(brushHi,e->maxs);e->contents=CONTENTS_SOLID;}
void G_SetMovedir(vec3_t angles,vec3_t out){if(angles[0]==0&&angles[2]==0&&(angles[1]==-1||angles[1]==-2))VectorSet(out,0,0,angles[1]==-1?1:-1);else AngleVectors(angles,out,nullptr,nullptr);VectorClear(angles);}
void Use_BinaryMover(gentity_t*,gentity_t*,gentity_t*){++uses;}
void MatchTeam(gentity_t*,moverState_t,int){++matched;}
void G_JAR_MoverPathOffset(const gentity_t&,vec3_t out){VectorClear(out);}
void G_PlayDoorLoopSound(gentity_t*){} void G_UseTargets(gentity_t*,gentity_t*){}
void G_PlayDoorSound(gentity_t*,int){}
#include "game/g_mover_initmovertrdata.inc"
#include "game/g_mover_init.inc"
#include "game/g_mover_train.inc"
#include "game/g_mover_spawnplattrigger.inc"
#include "game/g_mover_sp_func_door.inc"
#include "game/g_mover_sp_func_button.inc"
#include "game/g_mover_sp_func_plat.inc"
#include "game/g_mover_sp_func_bobbing.inc"
#include "game/g_mover_func_bobbing_use.inc"
#include "game/g_mover_touch_doortrigger.inc"
#include "game/g_mover_touch_platcentertrigger.inc"
#include "game/g_door_trigger_spawn.inc"

static bool near(float a,float b,float tolerance=.004f){return fabs(a-b)<tolerance;}
static std::string scenario;
static void same(const vec3_t a,const vec3_t b,float tolerance=.004f){for(int i=0;i<3;++i)if(!near(a[i],b[i],tolerance))throw std::runtime_error(scenario+" axis "+std::to_string(i)+": "+std::to_string(a[i])+" != "+std::to_string(b[i]));}
static void rotate(const vec3_t angles,const vec3_t local,vec3_t out){vec3_t basis[3];AnglesToAxis(angles,basis);VectorClear(out);for(int i=0;i<3;++i)VectorMA(out,local[i],basis[i],out);}
static void point(const gentity_t &root,const vec3_t local,vec3_t out){rotate(root.currentAngles,local,out);VectorAdd(out,root.currentOrigin,out);}
static void sameAxes(const vec3_t a,const vec3_t b){vec3_t aa[3],bb[3];AnglesToAxis(a,aa);AnglesToAxis(b,bb);for(int i=0;i<3;++i)same(aa[i],bb[i]);}
static gentity_t *root(const vec3_t angles){G_SubBSPOwnershipReset();nextSlot=MAX_CLIENTS;uses=matched=0;level.time=1000;auto*e=G_Spawn();e->classname="misc_bsp";e->s.modelindex=1;VectorSet(e->currentOrigin,100,200,300);VectorCopy(angles,e->currentAngles);CHECK(G_SubBSPBegin(e));return e;}
static gentity_t *child(gentity_t *parent,const char *classname,const char *keys="",int flags=0){
 std::string text="{ \"classname\" \""+std::string(classname)+"\" \"origin\" \"7 11 13\" "+keys+" }";const char *p=text.c_str();CHECK(G_ParseSpawnVars(&p));
 if(parent)G_AdjustSubBSPSpawnVars(parent->currentOrigin,parent->currentAngles);
 G_SubBSPCapture capture(parent);auto*e=G_Spawn();e->classname=const_cast<char*>(classname);e->model="*1";e->spawnflags=flags;
 G_SpawnVector("origin","0 0 0",e->s.origin);G_SpawnVector("angles","0 0 0",e->s.angles);G_SpawnVector("modelAngles","0 0 0",e->modelAngles);
 if(!parent&&field("angle"))e->s.angles[1]=float(atof(field("angle")));
 VectorCopy(e->s.origin,e->currentOrigin);VectorCopy(e->s.angles,e->currentAngles);VectorCopy(e->s.angles,e->s.apos.trBase);return e;
}
static void trajectory(gentity_t &e){
 // Execute the production state transition and native trajectory evaluator.
 for(bool linear:{false,true}){e.alt_fire=linear?qtrue:qfalse;level.time=2000;SetMoverState(&e,MOVER_1TO2,level.time);
  for(float fraction:{0.f,.25f,.5f,1.f,1.5f}){vec3_t at;EvaluateTrajectory(&e.s.pos,2000+int(e.s.pos.trDuration*fraction),at);const float t=std::min(1.f,float(int(e.s.pos.trDuration*fraction))/e.s.pos.trDuration),weight=linear?t:float(sin(t*M_PI*.5));vec3_t expected;for(int i=0;i<3;++i)expected[i]=e.pos1[i]+(e.pos2[i]-e.pos1[i])*weight;same(at,expected,.1f);}
  level.time=4000;SetMoverState(&e,MOVER_2TO1,level.time);vec3_t at;EvaluateTrajectory(&e.s.pos,4000+e.s.pos.trDuration,at);same(at,e.pos1);
 }
}
