// Actual native setup/firing with deterministic renderer services.
#include "game/g_local.h"
#include "game/b_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include <map>
#include <set>
game_import_t gi{};level_locals_t level{};
#include "game/g_npc_classes.inc"
struct TestPool:IGhoul2InfoArray{
 std::map<int,std::vector<CGhoul2Info>> entries;int next=1;
 int New()override{entries[next]={};return next++;}void Delete(int h)override{entries.erase(h);}
 bool IsValid(int h)const override{return entries.count(h)!=0;}
 std::vector<CGhoul2Info>&Get(int h)override{return entries.at(h);}
 const std::vector<CGhoul2Info>&Get(int h)const override{return entries.at(h);}
}pool;
IGhoul2InfoArray&_TheGhoul2InfoArray(){return pool;}
static bool original=false,badModel=false,badBone=false,badBolt=false,badMatrix=false;
static std::string loaded,attachment;static int shots=0,matrixModel=-1,matrixBolt=-1;
static std::set<std::string> models,sounds,effects;
static vec3_t shotOrigin,shotDirection;
int G_ModelIndex(const char*p){models.insert(p);return 1;}
int G_SoundIndex(const char*p){sounds.insert(p);return 1;}
int G_EffectIndex(const char*p){effects.insert(p);return 1;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);}
qboolean G_SpawnFloat(const char*,const char*d,float*out){*out=float(atof(d));return qfalse;}
gitem_t*FindItemForWeapon(weapon_t){static gitem_t item{};return &item;}
void RegisterItem(gitem_t*){}
void G_FreeEntity(gentity_t*e){e->ghoul2.clear();e->inuse=qfalse;e->linked=qfalse;}
void TurboLaser_SetBoneAnim(gentity_t*,int,int){throw std::runtime_error("Unexpected turbo setup");}
void turret_fire(gentity_t*,vec3_t start,vec3_t direction){++shots;VectorCopy(start,shotOrigin);VectorCopy(direction,shotDirection);}
#define SPF_TURRETG2_TURBO 4
#define START_DIS 15
#include "game/g_turret_muzzle.inc"
#include "game/g_turret_spawn.inc"
#include "game/g_turret_head.inc"
int main(){return RunTests("NS turret native resource selection, setup, muzzle firing, failure cleanup and saved state",[]{
 gi.FS_ReadFile=[](const char*p,void**)->long{return original&&!strcmp(p,"models/map_objects/nar_shaddar/turret/turret.glm")?100:-1;};
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&g,const char*p,int,qhandle_t,qhandle_t,int,int){loaded=p;if(badModel)return -1;g.resize(2);return 1;};
 gi.G2API_SetBoneAngles=[](CGhoul2Info*,const char*p,const vec3_t,int,Eorientations,Eorientations,Eorientations,qhandle_t*,int,int){CHECK(!strcmp(p,"Bone_body"));return badBone?qfalse:qtrue;};
 gi.G2API_AddBolt=[](CGhoul2Info*,const char*p){attachment=p;return badBolt?-1:!strcmp(p,"*flash02")?6:!strcmp(p,"*flash03")?7:!strcmp(p,"*muzzle1")?8:9;};
 gi.G2API_GetBoltMatrix=[](CGhoul2Info_v&,int model,int bolt,mdxaBone_t*,const vec3_t,const vec3_t,int,qhandle_t*,const vec3_t){matrixModel=model;matrixBolt=bolt;return badMatrix?qfalse:qtrue;};
 gi.G2API_GiveMeVectorFromMatrix=[](mdxaBone_t&,Eorientations flag,vec3_t&out){if(flag==ORIGIN)VectorSet(out,10,20,30);else VectorSet(out,1,0,0);};
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};level.time=1000;
 for(bool authored:{false,true}){original=authored;gentity_t e{},enemy{};e.inuse=qtrue;e.classname=const_cast<char*>("misc_ns_turret");e.wait=200;
  SP_misc_ns_turret(&e);CHECK(e.inuse&&e.linked&&e.playerModel==1&&e.torsoBolt==(authored?6:7));
  CHECK(loaded==(authored?"models/map_objects/nar_shaddar/turret/turret.glm":"models/map_objects/imp_mine/turret_canon.glm"));
  CHECK(e.health==100&&e.max_health==100&&e.radius==512&&e.wait==200&&e.damage==5&&e.s.weapon==WP_TURRET);
  CHECK(e.mins[2]==-30&&e.maxs[2]==0&&e.noDamageTeam==TEAM_ENEMY&&e.takedamage);
  CHECK(e.e_ThinkFunc==thinkF_turret_base_think&&e.e_UseFunc==useF_turret_base_use&&e.e_DieFunc==dieF_turret_die&&e.e_PainFunc==painF_TurretPain);
  CHECK(e.nextthink==level.time+FRAMETIME*5&&(e.contents&CONTENTS_SHOTCLIP));
  CHECK(bool(e.s.eFlags&EF_SHADER_ANIM)==!authored);
  e.enemy=&enemy;const int before=shots;turret_head_think(&e);CHECK(shots==before+1&&matrixModel==1&&matrixBolt==(authored?6:7));
  CHECK(shotOrigin[0]==25&&shotOrigin[1]==20&&shotDirection[0]==1&&e.pushDebounceTime==1200&&e.fly_sound_debounce_time==1000);
  turret_head_think(&e);CHECK(shots==before+1);e.pushDebounceTime=0;badMatrix=true;turret_head_think(&e);CHECK(shots==before+1);badMatrix=false;
  e.pushDebounceTime=0;e.torsoBolt=-1;turret_head_think(&e);CHECK(shots==before+1);e.torsoBolt=authored?6:7;
  // Ghoul2 instances have a separate native save stream; exercise entity fields
  // without importing a second owner of this fixture's live renderer handle.
  e.ghoul2.clear();MemorySave saved;ojk::SavedGameHelper helper(&saved);e.sg_export(helper);saved.reset_buffer_offset();gentity_t restored{};restored.sg_import(helper);saved.ensure_all_data_read();
  CHECK(restored.playerModel==1&&restored.torsoBolt==e.torsoBolt&&restored.e_ThinkFunc==e.e_ThinkFunc&&restored.wait==200&&restored.damage==5);
  G_FreeEntity(&e);
 }
 original=false;
 for(int failure=0;failure<3;++failure){gentity_t e{};e.inuse=qtrue;e.classname=const_cast<char*>("misc_ns_turret");badModel=failure==0;badBone=failure==1;badBolt=failure==2;
  SP_misc_ns_turret(&e);CHECK(!e.inuse&&!e.linked&&e.ghoul2.size()==0);badModel=badBone=badBolt=false;
 }
 gentity_t off{};off.inuse=qtrue;off.spawnflags=1;off.team=const_cast<char*>("player");off.health=250;off.radius=1024;off.damage=9;off.wait=75;SP_misc_ns_turret(&off);
 CHECK(off.s.frame==1&&off.noDamageTeam==TEAM_PLAYER&&off.health==250&&off.radius==1024&&off.wait==75&&off.damage==9);G_FreeEntity(&off);
 // Existing ordinary/turbo turret muzzle choices retain their native convention.
 gentity_t regular{};regular.ghoul2.resize(1);regular.classname=const_cast<char*>("misc_turret");mdxaBone_t matrix{};
 CHECK(G_TurretMuzzleMatrix(&regular,vec3_origin,matrix)&&matrixModel==0&&matrixBolt==7);
 regular.spawnflags=4;CHECK(G_TurretMuzzleMatrix(&regular,vec3_origin,matrix)&&matrixBolt==8);regular.alt_fire=qtrue;CHECK(G_TurretMuzzleMatrix(&regular,vec3_origin,matrix)&&matrixBolt==9);
 badBolt=true;CHECK(!G_TurretMuzzleMatrix(&regular,vec3_origin,matrix));badBolt=false;regular.ghoul2.clear();CHECK(!G_TurretMuzzleMatrix(&regular,vec3_origin,matrix));
 CHECK(models.count("models/map_objects/imp_mine/turret_damage.md3")&&sounds.count("sound/chars/turret/move.wav")&&effects.count("turret/explode"));
 });}
