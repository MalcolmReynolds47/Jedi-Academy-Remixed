// Real script-runner callbacks against ICARUS/filesystem service boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "icarus/IcarusInterface.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
static unsigned executions=0,initializations=0;static int receiverSlot=-1;static std::string executed;
static bool refuseInit=false;static gentity_t*destroy=nullptr;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
void G_SetEnemy(gentity_t*e,gentity_t*other){e->enemy=other;}
gentity_t*G_Find(gentity_t*,int field,const char*name){CHECK(field==FOFS(script_targetname));for(int i=0;i<globals.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].script_targetname&&!strcmp(g_entities[i].script_targetname,name))return &g_entities[i];return nullptr;}
qboolean G_SpawnFloat(const char*,const char*value,float*out){*out=float(atof(value));return qfalse;}
struct ScriptGame {
 bool ValidEntity(gentity_t*e){return e->script_targetname&&*e->script_targetname;}
 bool InitEntity(gentity_t*e){++initializations;if(!refuseInit)e->m_iIcarusID=e->s.number+1;return false;}
 bool RunScript(gentity_t*e,const char*path){++executions;receiverSlot=e->s.number;executed=path;if(destroy){destroy->inuse=qfalse;destroy->count=77;destroy->nextthink=777;destroy->e_ThinkFunc=thinkF_G_FreeEntity;}return true;}
};
ScriptGame*Quake3Game(){static ScriptGame game;return &game;}
qboolean G_ActivateBehavior(gentity_t*e,int set){return Quake3Game()->RunScript(e,e->behaviorSet[set])?qtrue:qfalse;}
#include "game/g_target_scriptrunner.inc"
#include "scriptrunner_files.h"
static gentity_t&Make(int index){auto&e=g_entities[index];e.inuse=qtrue;e.s.number=index;e.classname=G_NewString("target_scriptrunner");e.count=-1;e.wait=1;e.spawnflags=1;e.behaviorSet[BSET_USE]=G_NewString(jar::scriptRunner::defaultScript);SP_target_scriptrunner(&e);return e;}
int main(){return RunTests("script runner paths, initialization, delays, cooldown, lifetimes and native save fields",[]{
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};ScriptFiles();globals.num_entities=32;level.time=1000;
 using namespace jar::scriptRunner;std::string path;
 CHECK(scriptCount>1000);for(const char*s:scripts)CHECK(Canonical(s,path)&&path==s);
 CHECK(Canonical("SCRIPTS/COMMON/DOOROPENEDELSEWHERE.IBI",path)&&path==defaultScript);
 for(const char*bad:{"","/foo","foo/","foo//bar","../foo","foo/../bar","foo;quit","foo bar","foo\\bar","foo\nbar",".ibi","scripts/"})CHECK(!Canonical(bad,path));
 CHECK(!strcmp(Cycle(scripts[0],-1),scripts[scriptCount-1]));CHECK(!strcmp(Cycle(scripts[scriptCount-1],1),scripts[0]));
 auto&runner=Make(10);auto&actor=g_entities[0];actor.inuse=qtrue;actor.s.number=0;
 Settings settings;CHECK(G_ScriptRunnerRead(runner,settings)&&settings.count==-1&&settings.wait==1&&settings.delay==0&&settings.activator);
 auto limits=settings;limits.delay=limits.wait=3600;limits.count=100000;Settings reread;
 CHECK(G_ScriptRunnerSet(runner,limits)&&G_ScriptRunnerRead(runner,reread)&&reread.delay==3600&&reread.wait==3600&&reread.count==100000);
 settings.delay=.25f;settings.wait=.5f;settings.count=2;CHECK(G_ScriptRunnerSet(runner,settings));
 target_scriptrunner_use(&runner,nullptr,&actor);CHECK(runner.nextthink==1250&&runner.e_ThinkFunc==thinkF_scriptrunner_run&&executions==0);
 target_scriptrunner_use(&runner,nullptr,nullptr);CHECK(runner.activator==&actor&&runner.nextthink==1250);
 level.time=1250;scriptrunner_run(&runner);CHECK(executions==1&&receiverSlot==0&&executed==defaultScript&&runner.count==1&&runner.nextthink==1750&&runner.e_ThinkFunc==thinkF_NULL&&initializations==1);
 const auto ownedName=actor.script_targetname;for(int i=0;i<16;++i)va("overwrite transient string %d",i);CHECK(actor.script_targetname==ownedName&&strstr(ownedName,"jar_script_"));
 target_scriptrunner_use(&runner,nullptr,&actor);CHECK(executions==1&&runner.e_ThinkFunc==thinkF_NULL);
 level.time=1750;target_scriptrunner_use(&runner,nullptr,&actor);CHECK(runner.nextthink==2000);level.time=2000;scriptrunner_run(&runner);CHECK(executions==2&&runner.count==0&&runner.e_UseFunc==useF_NULL&&runner.e_ThinkFunc==thinkF_NULL&&initializations==1);
 level.time=3000;target_scriptrunner_use(&runner,nullptr,&actor);CHECK(executions==2);
 // Undo/editor rearming cancels a queue and does not execute anything.
 settings.count=-1;CHECK(G_ScriptRunnerSet(runner,settings));target_scriptrunner_use(&runner,nullptr,&actor);CHECK(runner.e_ThinkFunc==thinkF_scriptrunner_run);
 CHECK(G_ScriptRunnerSet(runner,settings)&&!runner.nextthink&&!runner.activator&&runner.e_ThinkFunc==thinkF_NULL&&executions==2);
 target_scriptrunner_use(&runner,nullptr,&actor);G_ScriptRunnerForget(&actor);actor.inuse=qfalse;actor.inuse=qtrue;CHECK(!runner.nextthink&&!runner.activator&&runner.e_ThinkFunc==thinkF_NULL&&runner.count==-1);
 // Self execution initializes an entity spawned outside normal map loading.
 settings.activator=false;settings.delay=0;CHECK(G_ScriptRunnerSet(runner,settings));target_scriptrunner_use(&runner,nullptr,nullptr);CHECK(executions==3&&receiverSlot==10&&initializations==2&&runner.m_iIcarusID);
 // A self-run queue survives loss of an optional, unused activator.
 settings.delay=.5f;CHECK(G_ScriptRunnerSet(runner,settings));target_scriptrunner_use(&runner,nullptr,&actor);G_ScriptRunnerForget(&actor);CHECK(runner.nextthink==3500&&runner.e_ThinkFunc==thinkF_scriptrunner_run&&!runner.activator);
 // Native save fields retain remaining uses, path handle, pending callback,
 // activator slot and timers. Pointer rebasing belongs to ReadGEntities.
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t saved{},loaded{};
 saved.delay=runner.delay;saved.wait=runner.wait;saved.count=runner.count;saved.spawnflags=runner.spawnflags;saved.nextthink=runner.nextthink;saved.e_ThinkFunc=runner.e_ThinkFunc;saved.e_UseFunc=runner.e_UseFunc;
 saved.activator=reinterpret_cast<gentity_t*>(17);saved.behaviorSet[BSET_USE]=reinterpret_cast<char*>(23);
 saved.sg_export(h);save.reset_buffer_offset();loaded.sg_import(h);h.ensure_all_data_read();
 CHECK(loaded.delay==500&&loaded.wait==500&&loaded.count==-1&&loaded.nextthink==3500&&loaded.e_ThinkFunc==thinkF_scriptrunner_run&&loaded.e_UseFunc==useF_target_scriptrunner_use&&loaded.activator==reinterpret_cast<gentity_t*>(17)&&loaded.behaviorSet[BSET_USE]==reinterpret_cast<char*>(23));
 // Invalid edits and missing files leave every existing queued property intact.
 auto oldPath=runner.behaviorSet[BSET_USE];scriptMissing=true;CHECK(!G_ScriptRunnerSet(runner,settings));scriptMissing=false;scriptBadHeader=true;CHECK(!G_ScriptRunnerSet(runner,settings));scriptBadHeader=false;scriptShortRead=true;CHECK(!G_ScriptRunnerSet(runner,settings));scriptShortRead=false;
 for(float bad:{-1.f,3601.f,std::numeric_limits<float>::quiet_NaN()}){auto badSettings=settings;badSettings.delay=bad;CHECK(!G_ScriptRunnerSet(runner,badSettings));}
 settings.script=std::string(defaultScript)+std::string("\0junk",5);CHECK(!G_ScriptRunnerSet(runner,settings));settings.script=defaultScript;
 CHECK(runner.nextthink==3500&&runner.behaviorSet[BSET_USE]==oldPath&&runner.e_ThinkFunc==thinkF_scriptrunner_run&&scriptOpens==scriptCloses);
 // Failed initialization consumes no use. The return value of InitEntity is
 // not treated as success: the native implementation returns false on success.
 settings.activator=true;settings.delay=0;settings.count=1;CHECK(G_ScriptRunnerSet(runner,settings));actor.m_iIcarusID=0;refuseInit=true;target_scriptrunner_use(&runner,nullptr,&actor);CHECK(executions==3&&runner.count==1&&!runner.nextthink);refuseInit=false;
 // Executing a destructive script cannot write into the freed/reused slot.
 destroy=&runner;target_scriptrunner_use(&runner,nullptr,&actor);CHECK(executions==4&&!runner.inuse&&runner.count==77&&runner.nextthink==777&&runner.e_ThinkFunc==thinkF_G_FreeEntity);destroy=nullptr;
 });}
