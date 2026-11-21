// Native object callbacks against deterministic collision, damage and FX services.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "test_support.h"
#include <string>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected model allocation");}
static std::string model,effect;
static int effects=0,triggers=0,blasts=0,frees=0,detaches=0,behaviors=0;
static gentity_t*expectedActivator=nullptr,*detachedDoor=nullptr,*doorTrigger=nullptr,*exploding=nullptr;
static trace_t traceResult{};
int G_ModelIndex(const char*s){model=s;return 7;}
int G_EffectIndex(const char*s){effect=s;return 8;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);}
void G_PlayEffect(int id,const vec3_t,const vec3_t){CHECK(id==8);++effects;}
void G_UseTargets(gentity_t*,gentity_t*activator){CHECK(activator==expectedActivator);++triggers;}
void G_RadiusDamage(const vec3_t,gentity_t*,float damage,float radius,gentity_t*,int){
 CHECK(exploding&&!exploding->takedamage);CHECK(damage==100&&radius==200);++blasts;
}
qboolean G_ActivateBehavior(gentity_t*,int){++behaviors;return qtrue;}
gentity_t*G_FindDoorTrigger(gentity_t*){return doorTrigger;}
char*vtos(const vec3_t){return const_cast<char*>("test origin");}
void G_JAR_WorkshopDetachChild(gentity_t*,gentity_t*child){detachedDoor=child;++detaches;}
#include "game/g_maglock.inc"
void G_FreeEntity(gentity_t*e){G_MaglockForget(e);e->inuse=qfalse;++frees;}
#include "game/g_cargo_barrel.inc"
#include "game/g_object_explosion.inc"
static gentity_t&Make(int slot,const char*name){auto&e=g_entities[slot];e.inuse=qtrue;e.s.number=slot;e.classname=const_cast<char*>(name);return e;}
int main(){return RunTests("native locks, slot reuse and explosive barrel callbacks",[]{
 globals.num_entities=32;level.time=1000;
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.Printf=Com_Printf;
 gi.trace=[](trace_t*t,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int,EG2_Collision,int){*t=traceResult;};
 auto&door=Make(1,"func_door");auto&first=Make(2,"misc_maglock");auto&second=Make(3,"misc_maglock");auto&attacker=Make(4,"player");
 first.model=const_cast<char*>("models/map_objects/kejim/sec_panel.md3");SP_misc_maglock(&first);
 CHECK(model==first.model&&effect=="thermal/explosion"&&first.nextthink>level.time&&first.e_ThinkFunc==thinkF_maglock_link);
 CHECK(!G_MaglockAttach(&first,nullptr)&&!G_MaglockAttach(&first,&first));
 CHECK(G_MaglockAttach(&first,&door));CHECK(first.activator==&door&&door.lockCount==1&&(door.svFlags&SVF_INACTIVE));
 CHECK(first.health==10&&first.takedamage&&(first.flags&FL_SHIELDED)&&first.contents==CONTENTS_CORPSE&&first.maxs[0]==12);
 CHECK(first.e_ThinkFunc==thinkF_NULL&&!first.nextthink&&first.e_DieFunc==dieF_maglock_die);
 CHECK(G_MaglockAttach(&first,&door)&&door.lockCount==1);CHECK(G_MaglockAttach(&second,&door)&&door.lockCount==2&&second.maxs[0]==8);
 G_FreeEntity(&second);CHECK(door.lockCount==1&&(door.svFlags&SVF_INACTIVE)&&!second.activator&&!detaches);
 // Production WP_Explode must receive the attacker and dispatch targets once.
 first.target=const_cast<char*>("unlock_feedback");expectedActivator=&attacker;
 maglock_die(&first,&attacker,&attacker,10,MOD_SABER,0,0);
 CHECK(!door.lockCount&&!(door.svFlags&SVF_INACTIVE)&&door.inuse&&!first.activator);
 CHECK(detaches==1&&detachedDoor==&door&&first.owner==&attacker&&triggers==1&&effects==1&&!blasts);
 CHECK(!first.takedamage&&first.e_DieFunc==dieF_NULL&&first.e_ThinkFunc==thinkF_G_FreeEntity&&first.nextthink==1050);
 G_FreeEntity(&first);CHECK(door.inuse&&detaches==1);
 // Removing the door clears every lock reference before this slot is reused.
 first.inuse=second.inuse=qtrue;CHECK(G_MaglockAttach(&first,&door)&&G_MaglockAttach(&second,&door));
 G_FreeEntity(&door);CHECK(!first.activator&&!second.activator&&!door.lockCount);
 door.inuse=qtrue;door.svFlags=0;door.lockCount=6;G_FreeEntity(&first);CHECK(door.lockCount==6&&!door.svFlags);door.lockCount=0;
 // Authored attachment tolerates missing/non-door/invalid traces, then binds
 // the native trigger where present. A bad map lock cannot drop the game.
 auto&authored=Make(5,"misc_maglock");traceResult.fraction=1;maglock_link(&authored);CHECK(authored.nextthink==1100&&!authored.activator);
 traceResult.fraction=.5f;for(int id:{-1,ENTITYNUM_WORLD,ENTITYNUM_NONE,6}){traceResult.entityNum=id;maglock_link(&authored);CHECK(!authored.activator);}
 auto&nonDoor=Make(6,"misc_model_static");(void)nonDoor;maglock_link(&authored);CHECK(!authored.activator);
 auto&trigger=Make(7,"door_trigger");doorTrigger=&trigger;traceResult.entityNum=door.s.number;traceResult.plane.normal[1]=1;traceResult.endpos[0]=42;
 maglock_link(&authored);CHECK(authored.activator==&trigger&&trigger.lockCount==1&&authored.currentOrigin[0]==42&&authored.maxs[0]==8);
 traceResult.startsolid=qtrue;maglock_link(&authored);CHECK(!authored.inuse&&!trigger.lockCount&&!(trigger.svFlags&SVF_INACTIVE));
 doorTrigger=nullptr;traceResult.startsolid=qfalse;authored.inuse=qtrue;maglock_link(&authored);CHECK(authored.activator==&door);G_FreeEntity(&authored);
 // The portable barrel keeps measured model bounds and executes native
 // delayed death/radius damage/target/free callbacks with recursion disarmed.
 auto&barrel=Make(8,"object_cargo_barrel1");barrel.model=const_cast<char*>("models/map_objects/imperial/barrels.md3");barrel.targetname=const_cast<char*>("barrel");barrel.target=const_cast<char*>("feedback");
 VectorSet(barrel.mins,-39,-39,-1);VectorSet(barrel.maxs,39,39,48);SP_object_cargo_barrel1(&barrel);
 CHECK(model==barrel.model&&effect=="thermal/explosion"&&barrel.mins[2]==-1&&barrel.maxs[0]==39&&barrel.maxs[2]==48);
 CHECK(barrel.health==20&&barrel.splashDamage==100&&barrel.splashRadius==200&&barrel.takedamage&&barrel.e_DieFunc==dieF_ExplodeDeath_Wait&&barrel.e_UseFunc==useF_GoExplodeDeath);
 CHECK(barrel.contents==(CONTENTS_SOLID|CONTENTS_OPAQUE));ExplodeDeath_Wait(&barrel,&attacker,&attacker,40,MOD_BLASTER,0,0);
 CHECK(barrel.e_DieFunc==dieF_NULL&&barrel.e_ThinkFunc==thinkF_ExplodeDeath&&barrel.nextthink>=1100&&barrel.nextthink<=1500&&!blasts);
 exploding=expectedActivator=&barrel;ExplodeDeath(&barrel);CHECK(!barrel.inuse&&blasts==1&&triggers==2&&effects==2);
 auto&used=Make(9,"object_cargo_barrel1");used.model=barrel.model;used.targetname=const_cast<char*>("use_barrel");used.target=barrel.target;used.health=123;SP_object_cargo_barrel1(&used);CHECK(used.health==123);
 exploding=expectedActivator=&used;GoExplodeDeath(&used,&attacker,&attacker);CHECK(!used.inuse&&!used.targetname&&blasts==2&&triggers==3&&effects==3&&behaviors==1);
 });}
