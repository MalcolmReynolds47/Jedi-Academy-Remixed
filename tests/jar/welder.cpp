// Production portable welder against native collision, FX and damage boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include "memory_save.h"
#include <deque>
#include <functional>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/weapons2/demp2/demp2.md3"));return 7;}
int G_EffectIndex(const char*s){CHECK(!strcmp(s,"sparks/spark_nosnd"));return 8;}
int G_SoundIndex(const char*s){CHECK(!strcmp(s,"sound/effects/electric_beam_lp.wav"));return 9;}
static int effects=0,damageCalls=0,traces=0;static float damageValue=0,damageRadius=0;
static vec3_t emitted{},emissionDirection{};static std::function<void()> onDamage;
void G_PlayEffect(int id,const vec3_t origin,const vec3_t dir){CHECK(id==8);++effects;VectorCopy(origin,emitted);VectorCopy(dir,emissionDirection);}
void G_RadiusDamage(const vec3_t origin,gentity_t*attacker,float damage,float radius,gentity_t*ignore,int mod){
 CHECK(attacker==ignore&&mod==MOD_UNKNOWN&&VectorCompare(origin,emitted));++damageCalls;damageValue=damage;damageRadius=radius;
 auto action=onDamage;onDamage=nullptr;if(action)action();
}
#include "game/g_welder.inc"
int main(){return RunTests("welder pulses, collision, cooldown, lifetime and saved state",[]{
 globals.num_entities=16;level.time=1000;gi.linkentity=[](gentity_t*){};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static bool solid=false;static float fraction=1;
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){
  ++traces;CHECK(!mins&&!maxs&&pass>=2&&mask==MASK_SHOT);*t={};t->fraction=fraction;t->startsolid=solid?qtrue:qfalse;VectorSet(t->plane.normal,-1,0,0);
  for(int a=0;a<3;++a)t->endpos[a]=start[a]+fraction*(end[a]-start[a]);
 };
 auto make=[](int n)->gentity_t&{auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString("misc_model_welder");e.model=G_NewString("@jar_welder");e.svFlags=0;VectorClear(e.currentOrigin);VectorClear(e.currentAngles);CHECK(G_WelderSpawn(&e));return e;};
 auto&e=make(2);CHECK(e.e_UseFunc==useF_welder_use&&e.e_ThinkFunc==thinkF_NULL&&!e.nextthink&&e.contents==CONTENTS_ITEM&&e.s.modelindex==7&&(e.svFlags&SVF_PLAYER_USABLE));
 G_WelderFrame();CHECK(effects==1&&damageCalls==1&&damageValue==10&&damageRadius==45&&e.s.loopSound==9&&e.attackDebounceTime==1200);CHECK(emitted[0]==15&&std::fabs(emitted[1]+.7046813f)<.001f&&std::fabs(emitted[2]+3.21862f)<.001f&&emissionDirection[0]==-1);
 for(int i=0;i<10;++i){G_WelderMoved(e);G_WelderSet(e,jar::welder::Settings{});G_WelderUse(&e);CHECK(!e.s.loopSound);G_WelderUse(&e);G_WelderFrame();}CHECK(traces==1&&damageCalls==1);
 level.time=1199;G_WelderFrame();CHECK(damageCalls==1);level.time=1200;G_WelderFrame();CHECK(damageCalls==2);
 // No catch-up damage burst after a slow frame.
 level.time=5000;G_WelderFrame();CHECK(damageCalls==3&&e.attackDebounceTime==5200);
 jar::welder::Settings s;CHECK(G_WelderRead(e,s));s.reach=64;s.radius=32.5f;s.interval=.5f;s.damage=0;CHECK(G_WelderSet(e,s));
 VectorSet(e.currentAngles,0,90,0);VectorSet(e.currentOrigin,100,20,30);G_WelderMoved(e);level.time=5200;G_WelderFrame();CHECK(effects==4&&damageCalls==3&&std::fabs(emitted[0]-100.7046813f)<.001f&&std::fabs(emitted[1]-84)<.001f&&std::fabs(emitted[2]-26.78138f)<.001f&&e.attackDebounceTime==5700);
 s.damage=25;G_WelderSet(e,s);VectorClear(e.currentAngles);VectorClear(e.currentOrigin);fraction=.5f;level.time=5700;G_WelderFrame();CHECK(damageCalls==4&&damageRadius==32.5f&&damageValue==25&&emitted[0]==31.5f&&emissionDirection[0]==-1);
 solid=true;level.time=6200;G_WelderFrame();CHECK(effects==5&&damageCalls==4);solid=false;fraction=1;
 e.svFlags|=SVF_INACTIVE;level.time=7200;G_WelderFrame();CHECK(!e.s.loopSound&&effects==5);e.svFlags&=~SVF_INACTIVE;G_WelderUse(&e);G_WelderFrame();CHECK(!e.s.loopSound&&effects==5);G_WelderUse(&e);G_WelderFrame();CHECK(e.s.loopSound==9&&effects==6);
 auto bad=s;bad.radius=std::numeric_limits<float>::quiet_NaN();CHECK(!G_WelderSet(e,bad)&&e.random==32.5f);bad=s;bad.interval=.099f;CHECK(!G_WelderSet(e,bad));bad=s;bad.damage=1001;CHECK(!G_WelderSet(e,bad));
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},restored{};
 stored.radius=e.radius;stored.random=e.random;stored.wait=e.wait;stored.damage=e.damage;stored.spawnflags=e.spawnflags;stored.attackDebounceTime=e.attackDebounceTime;stored.e_UseFunc=e.e_UseFunc;stored.e_ThinkFunc=e.e_ThinkFunc;
 stored.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.radius==64&&restored.random==32.5f&&restored.wait==.5f&&restored.damage==25&&restored.attackDebounceTime==e.attackDebounceTime&&restored.e_UseFunc==useF_welder_use);
 const int beforeRestore=effects;G_WelderRestore();CHECK(!e.s.loopSound);G_WelderFrame();CHECK(effects==beforeRestore&&e.s.loopSound==9);e.wait=0;bool rejected=false;try{G_WelderRestore();}catch(...){rejected=true;}CHECK(rejected);e.wait=.5f;G_WelderRestore();
 // A damage callback removes and immediately reuses a future registry slot.
 G_WelderReset();CHECK(!e.s.loopSound);auto&first=make(2);auto&future=make(4);level.time=8000;
 onDamage=[&]{G_WelderForget(&future);future.inuse=qfalse;make(4);};const int beforeReuse=effects;G_WelderFrame();CHECK(effects==beforeReuse+1);G_WelderFrame();CHECK(effects==beforeReuse+2);
 // Self-deletion performs no subsequent entity reads; removal clears the loop.
 G_WelderForget(&future);future.inuse=qfalse;level.time=9000;onDamage=[&]{G_WelderForget(&first);first.inuse=qfalse;};G_WelderFrame();CHECK(!first.inuse&&!first.s.loopSound&&portableWelders.empty());
 auto&last=make(5);G_WelderFrame();CHECK(last.s.loopSound);G_WelderReset();CHECK(!last.s.loopSound&&portableWelders.empty());
 });}
