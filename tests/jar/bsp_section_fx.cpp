// Native FX runner spawn/link/emission and portable save fields. Sound, event
// and damage imports are captured; no graphical process is started.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <cstdlib>

game_import_t gi{};
level_locals_t level{};
IGhoul2InfoArray& _TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static int effectCalls=0,soundCalls=0,ambientCalls=0,damageCalls=0,eventCalls=0,freeCalls=0;
static int lastEvent=0,lastEffect=0;
static vec3_t lastDamage{};
static char torchEffect[]="env/small_fire",torchSoundSet[]="yavintorch_lp";
int BMS_START=0,BMS_MID=1,BMS_END=2;
#define FX_ENT_RADIUS 32
qboolean G_SpawnInt(const char*,const char* value,int* out){*out=atoi(value);return qfalse;}
qboolean G_SpawnFloat(const char*,const char* value,float* out){*out=float(atof(value));return qfalse;}
qboolean G_SpawnAngleHack(const char*,const char*,float*){return qfalse;}
int G_EffectIndex(const char* name){CHECK(!strcmp(name,torchEffect));++effectCalls;return 17;}
int G_SoundIndex(const char* name){CHECK(!strcmp(name,"sound/effects/torch_burning_lp.wav"));++soundCalls;return 23;}
sfxHandle_t CAS_GetBModelSound(const char* name,int stage){CHECK(!strcmp(name,torchSoundSet)&&stage==BMS_MID);++ambientCalls;return 117;}
void G_SetOrigin(gentity_t* e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;}
#include "game/g_entity_angles.inc"
void EvaluateTrajectory(const trajectory_t* tr,int,vec3_t out){CHECK(tr->trType==TR_STATIONARY);VectorCopy(tr->trBase,out);}
void G_RadiusDamage(const vec3_t origin,gentity_t* e,float damage,float radius,gentity_t* ignore,int mod){
    CHECK(e==ignore&&damage==5&&radius==16&&mod==MOD_UNKNOWN);++damageCalls;VectorCopy(origin,lastDamage);
}
void G_AddEvent(gentity_t*,int ev,int parm){++eventCalls;lastEvent=ev;lastEffect=parm;}
void G_UseTargets2(gentity_t*,gentity_t*,const char*){throw std::runtime_error("Unexpected effect target");}
gentity_t* G_Find(gentity_t*,int,const char*){throw std::runtime_error("Unexpected effect target lookup");}
char* vtos(const vec3_t){return nullptr;}
void G_FreeEntity(gentity_t*){throw std::runtime_error("Unexpected entity free");}
static void QDECL Print(const char*,...){}
#include "game/g_fx_runner.inc"
static void Setup(gentity_t& e){
    e.inuse=qtrue;e.classname="fx_runner";e.fxFile=torchEffect;e.soundSet=torchSoundSet;e.spawnflags=4;
    VectorSet(e.s.origin,10,20,30);SP_fx_runner(&e);
    CHECK(e.linked&&e.fxID==17&&e.s.eType==ET_MOVER&&e.e_ThinkFunc==thinkF_fx_runner_link);
    CHECK(e.delay==200&&e.splashDamage==5&&e.splashRadius==16&&e.nextthink==level.time+400);
}
int main(){return RunTests("native portable flame, sound, movement and save",[]{
    gi.Printf=Print;
    gi.linkentity=[](gentity_t* e){e->linked=qtrue;};
    gi.bIsFromZone=[](const void* data,memtag_t tag){CHECK(tag==TAG_G_ALLOC&&(data==torchEffect||data==torchSoundSet));return qtrue;};
    gi.Free=[](void* data){CHECK(data==torchEffect||data==torchSoundSet);++freeCalls;return 0;};
    level.time=1200;
    gentity_t native{};Setup(native);
    gentity_t torch{};Setup(torch);
    const vec3_t rootAngles={28,61,-24};vec3_t axis[3];AnglesToAxis(rootAngles,axis);
    torch.spawnflags=3;CHECK(!G_BSPSectionInitializeFX(torch,rootAngles)&&!soundCalls&&!freeCalls&&torch.s.eType==ET_MOVER);
    torch.spawnflags=4;
    CHECK(G_BSPSectionInitializeFX(torch,rootAngles)&&soundCalls==1&&freeCalls==2);
    CHECK(torch.e_ThinkFunc==thinkF_fx_runner_link&&torch.nextthink==1600&&torch.s.eType==ET_GENERAL);
    CHECK(!torch.soundSet&&!torch.fxFile&&!torch.s.modelindex&&torch.fxID==17&&BSPSectionFXSound(torch)==23);
    level.time=1600;fx_runner_link(&native);fx_runner_link(&torch);
    CHECK(native.s.loopSound==117&&ambientCalls==1&&torch.s.loopSound==23);
    CHECK(torch.nextthink==1800&&torch.e_ThinkFunc==thinkF_fx_runner_think&&!torch.e_UseFunc);
    level.time=1800;fx_runner_think(&native);fx_runner_think(&torch);
    CHECK(effectCalls==2&&eventCalls==2&&damageCalls==2&&lastEvent==EV_PLAY_EFFECT&&lastEffect==17);
    CHECK(Distance(axis[2],torch.pos3)<.001f&&Distance(lastDamage,torch.currentOrigin)<.001f&&torch.nextthink==2000);

    // Native field save/load carries both indices (fxID aliases wpIndex) for
    // native events and the loop, without relying on an old ambient handle.
    MemorySave save;ojk::SavedGameHelper h(&save);torch.sg_export(h);save.reset_buffer_offset();
    gentity_t restored{};restored.sg_import(h);h.ensure_all_data_read();restored.classname="fx_runner"; // save string relocation is outside fixture
    CHECK(restored.fxID==17&&restored.s.eType==ET_GENERAL&&restored.s.loopSound==23&&restored.noise_index==23);
    CHECK(restored.nextthink==2000&&restored.e_ThinkFunc==thinkF_fx_runner_think);
    const vec3_t moved={110,40,-80},rotation={-13,89,55};G_SetOrigin(&restored,moved);G_SetAngles(&restored,rotation);
    level.time=restored.nextthink;fx_runner_think(&restored);vec3_t forward;AngleVectors(rotation,forward,nullptr,nullptr);
    CHECK(lastEffect==17&&restored.s.loopSound==23&&ambientCalls==1&&Distance(moved,lastDamage)<.001f);
    CHECK(Distance(forward,restored.pos3)<.001f&&restored.nextthink==2200);
});}
