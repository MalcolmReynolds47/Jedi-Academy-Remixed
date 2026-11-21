// Break, detach and leave a repairable controller at the same location. GPL-2.0-only.
#include "g_sandbox_breakable.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"

static bool sandboxBreakableDetonating=false;
struct SandboxBreakableBlastScope {
 const bool previous=sandboxBreakableDetonating;
 SandboxBreakableBlastScope(){sandboxBreakableDetonating=true;}
 ~SandboxBreakableBlastScope(){sandboxBreakableDetonating=previous;}
};

static void sandboxBreakableDetonate(gentity_t& source){
 using namespace jar::sandboxBreakable;
 Settings settings;
 if(!G_SandboxBreakableRead(source,settings)||settings.phase!=Pending)return;
 SandboxBreakableBlastScope blastScope;
 const auto life=G_EntityLifetime(&source);
 auto* attacker=source.enemy;
 const auto attackerLife=G_EntityLifetime(attacker);
 vec3_t mins,maxs,center,extent,dir={0,0,1};
 VectorCopy(source.absmin,mins);VectorCopy(source.absmax,maxs);
 VectorAdd(mins,maxs,center);VectorScale(center,.5f,center);
 VectorSubtract(maxs,mins,extent);
 if(attackerLife&&attacker->client){
  VectorSubtract(center,attacker->currentOrigin,dir);
  if(VectorNormalize(dir)==0)VectorSet(dir,0,0,1);
 }
 G_JAR_WorkshopBreakableDetach(&source);
 settings.phase=Broken;settings.remaining=0;
 if(!G_SandboxBreakableSet(source,settings))return;

 // Publish dormancy before stuck charges, splash damage or linked Use can reenter.
 for(int i=0;i<globals.num_entities;++i){
  auto& charge=g_entities[i];
  if(charge.inuse&&charge.s.groundEntityNum==source.s.number&&(charge.s.eFlags&EF_MISSILE_STICK)){
   G_Damage(&charge,&source,&source,nullptr,nullptr,99999,0,MOD_CRUSH);
   if(G_EntityLifetime(&source)!=life||!G_SandboxBreakableBroken(source))return;
  }
 }
 const auto material=static_cast<material_t>(settings.material);
 CacheChunkEffects(material);
 const float volume=std::max(0.f,extent[0]*extent[1]*extent[2]);
 const float size=std::sqrt(std::sqrt(volume))*1.75f;
 if(settings.explosion)CG_MiscModelExplosion(mins,maxs,size>48?2:size>24?1:0,material);
 if(settings.chunks){
  const int base=Q_irand(18,24);
  CG_Chunks(source.s.number,center,dir,mins,maxs,300,(base*settings.chunks+50)/100,
   material,0,size/base,source.noise_index);
 }
 auto* receiver=attackerLife&&G_EntityLifetime(attacker)==attackerLife?attacker:&source;
 if(settings.splash&&settings.radius){
  AddSightEvent(receiver,center,256,AEL_DISCOVERED,100);
  AddSoundEvent(receiver,center,128,AEL_DISCOVERED,qfalse,qtrue);
  G_Sound(&source,G_SoundIndex("sound/weapons/explosions/cargoexplode.wav"));
  G_RadiusDamage(center,&source,settings.splash,settings.radius,&source,MOD_UNKNOWN);
 }else{
  AddSightEvent(receiver,center,128,AEL_DISCOVERED);
  AddSoundEvent(receiver,center,64,AEL_SUSPICIOUS,qfalse,qtrue);
 }
 if(G_EntityLifetime(&source)!=life||!G_SandboxBreakableBroken(source)||!source.target)return;
 if(receiver!=&source&&G_EntityLifetime(receiver)!=attackerLife)receiver=&source;
 G_UseTargets(&source,receiver);
}

static void sandboxBreakableBegin(gentity_t& source,gentity_t* attacker){
 using namespace jar::sandboxBreakable;
 if(source.count!=Intact||(source.svFlags&SVF_INACTIVE))return;
 if(source.e_ThinkFunc==thinkF_G_RunObject){
  vec3_t origin,angles;VectorCopy(source.currentOrigin,origin);VectorCopy(source.currentAngles,angles);
  G_SetOrigin(&source,origin);G_SetAngles(&source,angles);
 }
 source.count=Pending;source.health=0;
 source.enemy=G_EntityLifetime(attacker)?attacker:nullptr;
 source.e_ThinkFunc=thinkF_funcBBrushDieGo;
 source.nextthink=level.time+std::max(1,source.delay);
 G_SandboxBreakableRefresh(source);
 // A chain reaction may arm many objects, so nested blasts run on a later game tick.
 if(!source.delay&&!sandboxBreakableDetonating)sandboxBreakableDetonate(source);
}
