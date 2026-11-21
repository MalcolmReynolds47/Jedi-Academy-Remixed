// Exercise the production portable crate with native damage/target services.
#include "cgame/cg_headers.h"
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
int G_ModelIndex(const char*){return 7;}int G_SoundIndex(const char*){return 9;}int G_EffectIndex(const char*){return 11;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);}
static int nextChild=20,damageCalls=0,targets=0,sounds=0;
static vec3_t damagePoint;static float lastDamage=0,lastRadius=0;
static std::vector<std::string> effects;
static std::function<void(gentity_t*)> onDamage,onTarget;
gentity_t*G_JAR_SpawnSetupChild(gentity_t*){auto&e=g_entities[nextChild];e.inuse=qtrue;e.s.number=nextChild++;globals.num_entities=nextChild;return &e;}
void G_PlayEffect(const char*s,const vec3_t){effects.push_back(s);}
void G_Sound(gentity_t*,int){++sounds;}
void G_UseTargets(gentity_t*e,gentity_t*){++targets;if(onTarget)onTarget(e);}
void G_RadiusDamage(const vec3_t p,gentity_t*e,float damage,float radius,gentity_t*ignore,int){CHECK(e==ignore);++damageCalls;VectorCopy(p,damagePoint);lastDamage=damage;lastRadius=radius;if(onDamage)onDamage(e);}
#include "game/g_crystal_crate.inc"
void G_FreeEntity(gentity_t*e){G_CrystalCrateForget(e);e->inuse=qfalse;}
static gentity_t&spawn(int id){auto&e=g_entities[id];e.inuse=qtrue;e.s.number=id;e.classname=G_NewString("misc_crystal_crate");e.model=G_NewString("@jar_crystal_crate");e.spawnflags=0;CHECK(G_CrystalCrateSpawn(&e));return e;}
int main(){return RunTests("crystal crate damage, assembly, reentrancy, lifetime and saved fields",[]{
 gi.linkentity=[](gentity_t*){};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 static bool blocked=false;gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int mask,EG2_Collision,int){CHECK(mask==(MASK_SOLID|CONTENTS_BODY));*t={};t->startsolid=blocked?qtrue:qfalse;};
 level.time=1000;auto&e=spawn(2);auto*part=e.lastEnemy;
 CHECK(G_CrystalCrateRoot(part)==&e&&e.health==80&&e.mass==40&&e.s.modelindex2==-1&&e.e_PainFunc==painF_CrystalCratePain&&e.e_DieFunc==dieF_misc_model_breakable_die&&e.e_UseFunc==useF_misc_use);
 CHECK(e.mins[0]==-17&&e.maxs[0]==17&&e.maxs[2]==38&&part->currentOrigin[2]==25);
 VectorSet(e.currentOrigin,100,200,300);VectorSet(e.currentAngles,30,50,70);CHECK(G_CrystalCrateMoved(e));vec3_t axis[3],tip;AnglesToAxis(e.currentAngles,axis);VectorMA(e.currentOrigin,25,axis[2],tip);CHECK(Distance(tip,part->currentOrigin)<.001f&&part->s.pos.trType==TR_INTERPOLATE);
 for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){vec3_t corner;VectorScale(axis[0],17.f*x,corner);VectorMA(corner,11.f*y,axis[1],corner);VectorMA(corner,19.f*(z+1),axis[2],corner);for(int a=0;a<3;++a)CHECK(corner[a]>=e.mins[a]-.001f&&corner[a]<=e.maxs[a]+.001f);}
 onDamage=[](gentity_t*c){CHECK(G_CrystalCratePain(c));};CHECK(G_CrystalCratePain(&e));CHECK(damageCalls==1&&lastDamage==16&&lastRadius==32&&effects.back()==jar::crystalCrate::hitEffect);VectorMA(e.currentOrigin,38,axis[2],tip);CHECK(Distance(tip,damagePoint)<.001f);
 level.time+=99;G_CrystalCratePain(&e);CHECK(damageCalls==1);++level.time;G_CrystalCratePain(&e);CHECK(damageCalls==2);onDamage={};
 jar::crystalCrate::Settings s;s.hitDamage=0;s.blastDamage=0;s.hitRadius=64.5f;s.blastRadius=256;s.solid=false;CHECK(G_CrystalCrateSet(e,s)&&e.contents==CONTENTS_SHOTCLIP);level.time+=100;G_CrystalCratePain(&e);CHECK(damageCalls==2);
 auto bad=s;bad.hitRadius=std::numeric_limits<float>::infinity();CHECK(!G_CrystalCrateSet(e,bad));bad=s;bad.blastRadius=4.5f;CHECK(!G_CrystalCrateSet(e,bad));bad=s;bad.hitDamage=1001;CHECK(!G_CrystalCrateSet(e,bad));
 s.solid=true;blocked=true;CHECK(!G_CrystalCrateSet(e,s)&&e.contents==CONTENTS_SHOTCLIP);blocked=false;CHECK(G_CrystalCrateSet(e,s)&&(e.contents&CONTENTS_BODY));
 s.hitDamage=12;s.blastDamage=90;CHECK(G_CrystalCrateSet(e,s));MemorySave save;ojk::SavedGameHelper helper(&save);gentity_t stored{},loaded{};stored.damage=e.damage;stored.radius=e.radius;stored.splashDamage=e.splashDamage;stored.splashRadius=e.splashRadius;stored.spawnflags=e.spawnflags;stored.e_DieFunc=e.e_DieFunc;stored.e_PainFunc=e.e_PainFunc;stored.e_UseFunc=e.e_UseFunc;stored.sg_export(helper);save.reset_buffer_offset();loaded.sg_import(helper);helper.ensure_all_data_read();CHECK(loaded.damage==12&&loaded.radius==64.5f&&loaded.splashDamage==90&&loaded.splashRadius==256&&loaded.e_DieFunc==dieF_misc_model_breakable_die);
 G_CrystalCrateRestore();CHECK(!e.painDebounceTime&&G_CrystalCratePart(e));e.radius=-1;bool rejected=false;try{G_CrystalCrateRestore();}catch(...){rejected=true;}CHECK(rejected);e.radius=64.5f;G_CrystalCrateRestore();
 onTarget=[](gentity_t*c){CHECK(G_CrystalCrateDie(c,c));};CHECK(G_CrystalCrateDie(&e,&e));CHECK(!e.inuse&&!part->inuse&&targets==1&&sounds==1&&lastDamage==90&&lastRadius==256&&effects[effects.size()-2]==jar::crystalCrate::explosion);onTarget={};
 // A target deleting/reusing the root must not have its new occupant freed.
 auto&targetCrate=spawn(3);onTarget=[](gentity_t*c){G_FreeEntity(c);spawn(c->s.number);};G_CrystalCrateDie(&targetCrate,&targetCrate);CHECK(targetCrate.inuse&&targetCrate.health==80&&G_CrystalCratePart(targetCrate));onTarget={};G_FreeEntity(&targetCrate);
 // Damage can remove/reuse the exploding root after the native target action.
 auto&damageCrate=spawn(4);const int oldTargets=targets;onDamage=[](gentity_t*c){G_FreeEntity(c);spawn(c->s.number);};G_CrystalCrateDie(&damageCrate,&damageCrate);CHECK(damageCrate.inuse&&damageCrate.health==80&&targets==oldTargets+1);onDamage={};
 part=damageCrate.lastEnemy;G_FreeEntity(part);CHECK(!damageCrate.lastEnemy);part->inuse=qtrue;part->classname=G_NewString("unrelated");CHECK(!G_CrystalCrateRoot(part));G_CrystalCrateMoved(damageCrate);G_CrystalCrateRestore();G_FreeEntity(&damageCrate);CHECK(part->inuse);G_CrystalCrateReset();CHECK(portableCrystalCrates.empty());
 auto&a=spawn(5);auto&b=spawn(6);const int beforeTargets=targets,beforeSounds=sounds;
 onDamage=[&](gentity_t*c){G_CrystalCrateDie(c==&a?&b:&a,c);};G_CrystalCrateDie(&a,&a);onDamage={};CHECK(!a.inuse&&!b.inuse&&targets==beforeTargets+2&&sounds==beforeSounds+2);
 gentity_t native{};native.inuse=qtrue;native.classname=G_NewString("misc_crystal_crate");CHECK(!G_CrystalCratePain(&native)&&!G_CrystalCrateDie(&native,nullptr));
 });}
