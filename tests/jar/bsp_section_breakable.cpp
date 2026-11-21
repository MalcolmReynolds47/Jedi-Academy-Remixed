// Native SP portable brush damage callbacks with captured effects and links.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "game/g_bsp_section.h"
#include "game/g_sandbox_breakable.h"
#include "cgame/cg_breakable_effects.h"
#include "memory_save.h"

game_import_t gi{};
game_export_t globals{};
level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES]{};
IGhoul2InfoArray& _TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}

namespace {
gentity_t* portableBrush = nullptr;
gentity_t* sandboxBody = nullptr;
gentity_t* expectedReceiver = nullptr;
gentity_t* chainTarget = nullptr;
int detached=0,outputs=0,blasts=0,explosionSounds=0;
int chunks = 0, explosions = 0, sights = 0, sounds = 0, portals = 0, links = 0, unlinks = 0;
int customSound = 0, fragmentCount = 0;
vec3_t chunkOrigin{}, chunkDirection{}, chunkMins{}, chunkMaxs{}, sightOrigin{}, soundOrigin{};
BreakableDebrisFrame chunkFrame{};
bool oriented = false;

void SetUpBrush(gentity_t& brush)
{
    brush.inuse = brush.linked = brush.bmodel = brush.takedamage = qtrue;
    brush.classname = "func_breakable";
    brush.s.number = 12;
    brush.s.modelindex = 24;
    brush.s.solid = SOLID_BMODEL;
    brush.s.eType = ET_MOVER;
    brush.e_UseFunc = useF_funcBBrushUse;
    brush.e_PainFunc = painF_funcBBrushPain;
    brush.e_DieFunc = dieF_funcBBrushDie;
    brush.contents = brush.clipmask = CONTENTS_SOLID;
    brush.health = 3;
    brush.noise_index = 19;
    brush.material = MAT_LT_STONE;
    brush.spawnflags = 2048;
    VectorSet(brush.currentOrigin, 40, 80, 120);
    VectorSet(brush.currentAngles, 0, 90, 0);
    VectorSet(brush.mins, 20, 100, 2);
    VectorSet(brush.maxs, 22, 108, 6);
    VectorSet(brush.absmin, -120, -80, -40);
    VectorSet(brush.absmax, 200, 240, 280);
}
}

bool G_BSPSectionBreakable(const gentity_t& brush){return &brush == portableBrush;}
gentity_t* G_SubBSPOwner(const gentity_t*){return nullptr;}
bool G_SiegeKorribanRoom(const gentity_t*){return false;}
void G_SetEnemy(gentity_t* ent, gentity_t* enemy){ent->enemy = enemy;}
void G_SetOrigin(gentity_t* ent,const vec3_t origin){
    VectorCopy(origin,ent->s.pos.trBase);ent->s.pos.trType=TR_STATIONARY;
}
void G_SetAngles(gentity_t* ent,const vec3_t angles){
    VectorCopy(angles,ent->s.apos.trBase);ent->s.apos.trType=TR_STATIONARY;
}
uint64_t G_EntityLifetime(const gentity_t* ent){return ent&&ent->inuse?1:0;}
void G_JAR_WorkshopBreakableDetach(gentity_t* ent){
    CHECK(ent==sandboxBody && ent->count==jar::sandboxBreakable::Pending);
    ++detached;
}
static void CacheChunkEffects(material_t type){CHECK(sandboxBody && type==MAT_GLASS);}
qboolean G_ActivateBehavior(gentity_t*, int slot){CHECK(slot == (sandboxBody?BSET_USE:BSET_PAIN)); return qfalse;}
void G_Damage(gentity_t*, gentity_t*, gentity_t*, const vec3_t, const vec3_t, int, int, int, int){CHECK(false);}
void G_UseTargets(gentity_t* source, gentity_t* receiver){
    CHECK(source==sandboxBody && G_SandboxBreakableBroken(*source) && receiver==expectedReceiver);
    ++outputs;
    funcBBrushUse(source,receiver,receiver); // A reentrant link must not arm a second fuse.
}
void G_UseTargets2(gentity_t*, gentity_t*, const char*){CHECK(false);}
void G_RadiusDamage(const vec3_t, gentity_t* source, float damage, float radius, gentity_t* ignored, int){
    CHECK(source==sandboxBody && ignored==source && G_SandboxBreakableBroken(*source) && damage==10 && radius==128);
    ++blasts;
    if(chainTarget){
        funcBBrushDie(chainTarget,source,source,1,MOD_UNKNOWN);
        CHECK(chainTarget->count==jar::sandboxBreakable::Pending&&chainTarget->nextthink==level.time+1);
    }
}
gentity_t* G_TempEntity(const vec3_t, int){CHECK(false); return nullptr;}
int G_SoundIndex(const char*){CHECK(sandboxBody); return 19;}
void G_Sound(gentity_t* source,int sound){CHECK(source==sandboxBody&&sound==19);++explosionSounds;}

void AddSightEvent(gentity_t*, vec3_t origin, float radius, alertEventLevel_e alert, float light)
{
    CHECK(alert==AEL_DISCOVERED && (sandboxBody?(radius==256 && light==100):(radius==128 && light==0)));
    ++sights;
    VectorCopy(origin, sightOrigin);
}

void AddSoundEvent(gentity_t*, vec3_t origin, float radius, alertEventLevel_e alert, qboolean needLOS, qboolean onGround)
{
    CHECK((sandboxBody?(radius==128 && alert==AEL_DISCOVERED):(radius==64 && alert==AEL_SUSPICIOUS)) && !needLOS && onGround);
    ++sounds;
    VectorCopy(origin, soundOrigin);
}

void CG_MiscModelExplosion(const vec3_t, const vec3_t, int, material_t, const BreakableDebrisFrame*)
{
    ++explosions;
}

void CG_Chunks(int owner, const vec3_t origin, const vec3_t direction, const vec3_t mins, const vec3_t maxs,
               float speed, int count, material_t material, int model, float scale,
               int custom, const BreakableDebrisFrame* frame)
{
    CHECK(owner == 12 && speed == 300 && material == (sandboxBody?MAT_GLASS:MAT_LT_STONE) && model == 0 && scale > 0);
    ++chunks;
    customSound = custom;
    fragmentCount = count;
    oriented = frame != nullptr;
    if (frame) chunkFrame = *frame;
    VectorCopy(origin, chunkOrigin);
    VectorCopy(direction, chunkDirection);
    VectorCopy(mins, chunkMins);
    VectorCopy(maxs, chunkMaxs);
}

#include "game/g_breakable_brush.inc"

int main(){return RunTests("portable breakable native damage, debris frame and solidity", []{
    gi.linkentity = [](gentity_t* entity){
        ++links;
        entity->linked = qtrue;
        if (entity->bmodel) entity->s.solid = SOLID_BMODEL;
    };
    gi.unlinkentity = [](gentity_t* entity){++unlinks; entity->linked = qfalse;};
    gi.AdjustAreaPortalState = [](gentity_t*, qboolean open){CHECK(open); ++portals;};
    level.time = 1500;

    gentity_t brush{}, attacker{};
    gclient_t client{};
    attacker.client = &client;
    VectorSet(attacker.currentOrigin, -84, 101, 124);
    SetUpBrush(brush);
    portableBrush = &brush;
    const vec3_t firstCenter = {-64, 101, 124};
    funcBBrushPain(&brush, &attacker, &attacker, firstCenter, 1, MOD_SABER);
    CHECK(chunks == 1 && oriented && customSound == 0 && fragmentCount >= 1 && fragmentCount <= 3);
    CHECK(Distance(chunkOrigin, firstCenter) < .001f && chunkDirection[0] < -.99f);
    CHECK(VectorCompare(chunkMins, brush.mins) && VectorCompare(chunkMaxs, brush.maxs));
    vec3_t firstCorner;
    VectorCopy(chunkMins, firstCorner);
    chunkFrame.ToWorld(firstCorner);
    const vec3_t expectedCorner = {-60, 100, 122};
    CHECK(Distance(firstCorner, expectedCorner) < .001f);
    CHECK(brush.linked && brush.s.solid == SOLID_BMODEL && !links && !unlinks && !portals);

    brush.delay = 1;
    funcBBrushDie(&brush, &attacker, &attacker, 1, MOD_SABER);
    CHECK(brush.linked && brush.bmodel && !brush.takedamage && chunks == 1);
    CHECK(brush.nextthink == 2500 && brush.e_ThinkFunc == thinkF_funcBBrushDieGo);
    VectorSet(brush.currentOrigin, 10, 30, 50);
    VectorSet(brush.currentAngles, 0, 180, 0);
    level.time = brush.nextthink;
    funcBBrushDieGo(&brush);
    const vec3_t lastCenter = {-11, -74, 54};
    CHECK(Distance(chunkOrigin, lastCenter) < .001f);
    CHECK(Distance(sightOrigin, lastCenter) < .001f && Distance(soundOrigin, lastCenter) < .001f);
    CHECK(chunks == 2 && sights == 1 && sounds == 1 && oriented && customSound == 19);
    CHECK(!links && unlinks == 1 && !portals && !explosions);
    CHECK(!brush.bmodel && !brush.linked && !brush.contents && !brush.clipmask && !brush.s.solid);
    CHECK(!brush.s.modelindex && !brush.s.modelindex2 && (brush.s.eFlags & EF_NODRAW));
    CHECK(brush.inuse && brush.nextthink == 2550 && brush.e_ThinkFunc == thinkF_G_FreeEntity);

    MemorySave save;
    ojk::SavedGameHelper helper(&save);
    brush.sg_export(helper);
    save.reset_buffer_offset();
    gentity_t restored{};
    restored.sg_import(helper);
    helper.ensure_all_data_read();
    CHECK(restored.inuse && !restored.bmodel && !restored.linked && !restored.s.solid);
    CHECK(restored.nextthink == 2550 && restored.e_ThinkFunc == thinkF_G_FreeEntity);

    gentity_t native{};
    SetUpBrush(native);
    native.spawnflags = 0;
    funcBBrushDie(&native, &attacker, &attacker, 1, MOD_SABER);
    CHECK(portals == 1 && links == 1 && unlinks == 1 && explosions == 1);
    CHECK(native.linked && native.bmodel && native.s.solid == SOLID_BMODEL);
    CHECK(chunks == 3 && !oriented && Distance(chunkOrigin, native.currentOrigin) < .001f);

    using namespace jar::sandboxBreakable;
    gentity_t body{};
    SetUpBrush(body);
    sandboxBody=&body;
    body.bmodel=qfalse;body.classname="func_glass";body.targetname="jar_sb_12";body.target="output";
    body.material=MAT_GLASS;body.spawnflags=0;body.health=40;
    CHECK(G_SandboxBreakableInitialize(body));
    Settings properties;
    CHECK(G_SandboxBreakableRead(body,properties)&&properties.capacity==40&&body.s.eType==ET_GENERAL);
    properties.fuse=250;properties.splash=10;properties.radius=128;properties.chunks=200;properties.mode=Saber;
    CHECK(G_SandboxBreakableSet(body,properties)&&body.takedamage&&(body.flags&FL_DMG_BY_SABER_ONLY));
    attacker.inuse=qtrue;
    funcBBrushUse(&body,&attacker,&attacker);
    const int deadline=level.time+250;
    CHECK(body.count==Pending && body.nextthink==deadline && !body.takedamage && body.contents);
    level.time=deadline-1;
    funcBBrushDie(&body,&attacker,&attacker,1,MOD_SABER); // A second hit cannot extend the fuse.
    G_SandboxBreakableControlledFrame(body);
    CHECK(!detached && body.nextthink==deadline);
    expectedReceiver=&attacker;
    level.time=deadline;G_SandboxBreakableControlledFrame(body);
    CHECK(detached==1&&blasts==1&&outputs==1&&explosionSounds==1&&chunks==4&&fragmentCount>=36&&fragmentCount<=48);
    CHECK(G_SandboxBreakableBroken(body)&&body.inuse&&body.linked&&!body.contents&&!body.s.solid&&!body.nextthink);
    CHECK(body.target&&body.s.modelindex==24&&portals==1);
    static bool occupied=true;
    gi.trace=[](trace_t* hit,const vec3_t origin,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,EG2_Collision,int){
        CHECK(VectorCompare(origin,end)&&VectorCompare(mins,sandboxBody->mins)&&VectorCompare(maxs,sandboxBody->maxs));
        CHECK(pass==12&&mask==(MASK_PLAYERSOLID|CONTENTS_CORPSE));
        *hit={};hit->fraction=1;hit->startsolid=occupied?qtrue:qfalse;
    };
    CHECK(!G_SandboxBreakableSet(body,properties)&&G_SandboxBreakableBroken(body));
    occupied=false;
    CHECK(G_SandboxBreakableSet(body,properties)&&body.takedamage&&body.contents&&!(body.s.eFlags&EF_NODRAW));
    properties.fuse=0;properties.enabled=false;
    CHECK(G_SandboxBreakableSet(body,properties));
    funcBBrushUse(&body,&attacker,&attacker);
    CHECK(body.count==Intact&&!body.takedamage&&outputs==1);
    properties.enabled=true;properties.mode=UseOnly;
    CHECK(G_SandboxBreakableSet(body,properties)&&!body.takedamage);
    gentity_t chain{};
    SetUpBrush(chain);
    chain.bmodel=qfalse;chain.targetname="jar_sb_13";chain.s.number=13;
    CHECK(G_SandboxBreakableInitialize(chain));chainTarget=&chain;
    expectedReceiver=&body;funcBBrushUse(&body,nullptr,nullptr);
    CHECK(G_SandboxBreakableBroken(body)&&outputs==2&&blasts==2);
    CHECK(chain.count==Pending&&chain.nextthink==level.time+1);
});}
