// Build fresh native charge entities for a pending Duplicate paste. GPL-2.0-only.
#include "g_sandbox_charges.h"
#include <climits>

static bool duplicateChargeChild(const Owned& owner,const gentity_t& charge){
 return owner.catalog==consoleCatalog&&owner.parent&&(charge.s.eFlags&EF_MISSILE_STICK)&&charge.classname&&
  (!strcmp(charge.classname,"detpack")||!strcmp(charge.classname,"tripmine"));
}

static gentity_t* duplicateCreateCharge(const jar::chargeCopy::State& state,gentity_t& player){
 using jar::chargeCopy::Mode;
 const bool pack=jar::chargeCopy::Pack(state.mode);
 const int weapon=pack?WP_DET_PACK:WP_TRIP_MINE;
 const char* model=weaponData[weapon].missileMdl;
 if(!model[0])return nullptr;
 auto* charge=G_TrySpawn();
 if(!charge)return nullptr;
 const int modelIndex=G_ModelIndex(model);
 if(!modelIndex||gi.G2API_InitGhoul2Model(charge->ghoul2,model,modelIndex,NULL_HANDLE,NULL_HANDLE,0,0)<0){
  G_FreeEntity(charge);
  return nullptr;
 }
 charge->classname=pack?"detpack":"tripmine";
 charge->s.weapon=weapon;
 charge->s.eType=ET_GENERAL;
 charge->s.eFlags=EF_MISSILE_STICK;
 charge->s.radius=pack?30:60;
 VectorSet(charge->s.modelScale,1,1,1);
 charge->svFlags=SVF_USE_CURRENT_ORIGIN;
 charge->contents=CONTENTS_SHOTCLIP;
 charge->clipmask=MASK_SHOT;
 const float half=jar::chargeCopy::HalfSize(state.mode);
 VectorSet(charge->mins,-half,-half,-half);
 VectorSet(charge->maxs,half,half,half);
 charge->takedamage=state.shootable?qtrue:qfalse;
 charge->health=state.health;
 charge->damage=state.damage;
 charge->splashDamage=state.splash;
 charge->splashRadius=state.radius;
 charge->methodOfDeath=charge->splashMethodOfDeath=pack?MOD_DETPACK:state.alternate?MOD_LASERTRIP_ALT:MOD_LASERTRIP;
 charge->activator=&player;
 charge->setTime=level.time;
 charge->fxID=G_EffectIndex(pack?"detpack/explosion":"tripMine/explosion");
 charge->e_DieFunc=dieF_WP_ExplosiveDie;
 charge->s.pos.trType=TR_STATIONARY;
 const auto deadline=[](int remaining){return int(std::min(int64_t(INT_MAX),int64_t(level.time)+remaining));};
 charge->nextthink=state.mode==Mode::DetPack?-1:deadline(state.pending);
 switch(state.mode){
 case Mode::DetPack: break;
 case Mode::Tripwire:
  charge->e_ThinkFunc=thinkF_laserTrapThink;
  if(state.armed){
   charge->s.eFlags|=EF_FIRING;
   charge->s.loopSound=G_SoundIndex("sound/weapons/laser_trap/hum_loop.wav");
  }
  break;
 case Mode::Proximity:
  charge->e_ThinkFunc=thinkF_WP_prox_mine_think;
  charge->delay=deadline(state.timeout);
  charge->count=state.armed?0:1;
  if(state.armed)charge->s.eFlags|=EF_PROX_TRIP;
  break;
 case Mode::DetonatingPack:
 case Mode::DetonatingMine:
  charge->e_ThinkFunc=thinkF_WP_Explode;
  if(state.armed)charge->s.eFlags|=EF_PROX_TRIP;
  break;
 }
 return charge;
}

static gentity_t* duplicateMountedCharge(const jar::chargeCopy::State& state,gentity_t& prop,gentity_t& player){
 auto* charge=duplicateCreateCharge(state,player);
 if(charge&&!G_SandboxChargeBindCopy(*charge,prop,state)){
  G_FreeEntity(charge);
  return nullptr;
 }
 return charge;
}

static gentity_t* duplicateStandaloneCharge(const jar::chargeCopy::State& state,gentity_t& player,
 const vec3_t origin,const vec3_t normal){
 auto* charge=duplicateCreateCharge(state,player);
 if(!charge)return nullptr;
 SandboxChargeMove move{};
 move.charge=charge;
 VectorCopy(origin,move.origin);VectorCopy(origin,move.beamStart);
 if(!G_SandboxChargeFits(move)){
  G_FreeEntity(charge);
  return nullptr;
 }
 vec3_t angles;vectoangles(normal,angles);
 G_SetOrigin(charge,origin);G_SetAngles(charge,angles);
 VectorCopy(origin,charge->s.origin);
 charge->s.groundEntityNum=ENTITYNUM_WORLD;
 if(state.mode==jar::chargeCopy::Mode::Tripwire){
  VectorCopy(normal,charge->movedir);
  VectorCopy(origin,charge->s.origin2);
 }
 return charge;
}
