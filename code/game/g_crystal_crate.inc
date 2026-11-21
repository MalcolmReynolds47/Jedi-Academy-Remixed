// Portable branch of Raven's crystal crate. Native map-authored crates retain
// their original model, callbacks and effects. GPL-2.0-only.
#include "g_crystal_crate.h"
#include <map>
extern gentity_t*G_JAR_SpawnSetupChild(gentity_t*);
namespace {
std::map<int,uint64_t> portableCrystalCrates;
uint64_t crystalSerial=0;
void crystalRegister(gentity_t&e){portableCrystalCrates[e.s.number]=++crystalSerial;}
bool crystalAlive(int id,uint64_t serial){auto it=portableCrystalCrates.find(id);return it!=portableCrystalCrates.end()&&it->second==serial&&G_CrystalCratePortable(g_entities[id]);}
void crystalPoint(const gentity_t&e,float up,vec3_t p){vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);VectorMA(e.currentOrigin,up,axis[2],p);}
}
bool G_CrystalCrateMoved(gentity_t&e){
 if(!G_CrystalCratePortable(e))return false;
 VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);VectorCopy(e.currentAngles,e.s.apos.trBase);e.s.apos.trType=TR_INTERPOLATE;
 vec3_t axis[3];AnglesToAxis(e.currentAngles,axis);
 // Stock crate bounds fit +/-17, +/-11, 0..22. The unscaled crystal at Z25
 // reaches 37.5. Rotate the enclosing box about the original model origin.
 for(int i=0;i<3;++i){const float center=axis[2][i]*19,extent=std::fabs(axis[0][i])*17+std::fabs(axis[1][i])*11+std::fabs(axis[2][i])*19;e.mins[i]=center-extent;e.maxs[i]=center+extent;}
 if(G_CrystalCratePart(e)){auto*p=e.lastEnemy;vec3_t pos;crystalPoint(e,25,pos);G_SetOrigin(p,pos);G_SetAngles(p,e.currentAngles);VectorCopy(pos,p->s.origin);VectorCopy(e.currentAngles,p->s.angles);p->s.pos.trType=p->s.apos.trType=TR_INTERPOLATE;gi.linkentity(p);}
 gi.linkentity(&e);return true;
}
bool G_CrystalCrateSpawn(gentity_t*e){
 if(!e||!G_CrystalCratePortable(*e))return false;
 auto*p=G_JAR_SpawnSetupChild(e);if(!p)return false;
 p->classname=G_NewString("info_notnull");p->model=G_NewString(jar::crystalCrate::crystal);p->s.modelindex=G_ModelIndex(p->model);p->owner=e;p->contents=CONTENTS_ITEM;p->takedamage=qfalse;
 VectorSet(p->mins,-7,-7,-13);VectorSet(p->maxs,7,7,13);e->lastEnemy=p;
 e->s.modelindex=G_ModelIndex(jar::crystalCrate::model);e->s.modelindex2=-1;e->s.eType=ET_GENERAL;e->svFlags|=SVF_PLAYER_USABLE;e->takedamage=qtrue;e->health=e->max_health=80;e->mass=40;e->material=MAT_CRATE2;
 e->e_PainFunc=painF_CrystalCratePain;e->e_DieFunc=dieF_misc_model_breakable_die;e->e_UseFunc=useF_misc_use;e->e_ThinkFunc=thinkF_NULL;e->nextthink=0;e->painDebounceTime=0;
 G_EffectIndex(jar::crystalCrate::hitEffect);G_EffectIndex(jar::crystalCrate::explosion);G_EffectIndex(jar::crystalCrate::debris);G_SoundIndex(jar::crystalCrate::sound);
 crystalRegister(*e);return G_CrystalCrateSet(*e,jar::crystalCrate::Settings{});
}
bool G_CrystalCratePain(gentity_t*e){
 if(!e||!G_CrystalCratePortable(*e))return false;
 jar::crystalCrate::Settings s;
 if(!e->takedamage||!G_CrystalCrateRead(*e,s)||e->painDebounceTime>level.time)return true;
 // Prevent neighboring crates from recursively pulsing one another in the
 // same frame. Direct weapon damage still applies on every hit.
 e->painDebounceTime=level.time+100;vec3_t p;crystalPoint(*e,38,p);G_PlayEffect(jar::crystalCrate::hitEffect,p);
 if(s.hitDamage)G_RadiusDamage(p,e,float(s.hitDamage),s.hitRadius,e,MOD_UNKNOWN);
 return true; // Damage callbacks may have removed or reused this entity.
}
bool G_CrystalCrateDie(gentity_t*e,gentity_t*attacker){
 if(!e||!G_CrystalCratePortable(*e))return false;
 if(!e->takedamage||e->e_DieFunc==dieF_NULL)return true;
 auto it=portableCrystalCrates.find(e->s.number);if(it==portableCrystalCrates.end())return true;
 const int id=e->s.number;const uint64_t serial=it->second;
 const int damage=e->splashDamage,radius=e->splashRadius;vec3_t p;crystalPoint(*e,19,p);
 e->takedamage=qfalse;e->health=0;e->e_DieFunc=dieF_NULL;e->e_PainFunc=painF_NULL;e->e_UseFunc=useF_NULL;e->contents=0;e->s.solid=0;gi.linkentity(e);
 G_PlayEffect(jar::crystalCrate::explosion,p);G_PlayEffect(jar::crystalCrate::debris,p);G_Sound(e,G_SoundIndex(jar::crystalCrate::sound));
 // Native breakables fire their target before blast damage. This also avoids
 // using the attacker after a damage callback could delete or reuse it.
 G_UseTargets(e,attacker);
 if(!crystalAlive(id,serial))return true;
 // Use the native damage path for falloff, LOS and chain reactions. Generation
 // checks stop callbacks acting on a replacement in the same slot.
 if(damage>0&&radius>0)G_RadiusDamage(p,e,float(damage),float(radius),e,MOD_UNKNOWN);
 if(crystalAlive(id,serial))G_FreeEntity(e);
 return true;
}
void G_CrystalCrateForget(gentity_t*e){
 if(!e)return;
 if(G_CrystalCratePortable(*e)){
  portableCrystalCrates.erase(e->s.number);auto*p=G_CrystalCratePart(*e)?e->lastEnemy:nullptr;e->lastEnemy=nullptr;
  if(p){p->owner=nullptr;G_FreeEntity(p);}
 }else if(auto*r=G_CrystalCrateRoot(e)){r->lastEnemy=nullptr;e->owner=nullptr;}
}
void G_CrystalCrateReset(){portableCrystalCrates.clear();}
void G_CrystalCrateRestore(){
 G_CrystalCrateReset();
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_CrystalCratePortable(e))continue;
  jar::crystalCrate::Settings s;if(!G_CrystalCrateRead(e,s)||e.health<=0||e.e_DieFunc!=dieF_misc_model_breakable_die||e.e_PainFunc!=painF_CrystalCratePain||e.e_UseFunc!=useF_misc_use||e.s.eType!=ET_GENERAL)gi.Error(ERR_DROP,"Invalid saved portable crystal crate");
  crystalRegister(e);e.painDebounceTime=0;G_CrystalCrateSet(e,s);G_EffectIndex(jar::crystalCrate::hitEffect);G_EffectIndex(jar::crystalCrate::explosion);G_EffectIndex(jar::crystalCrate::debris);G_SoundIndex(jar::crystalCrate::sound);
 }
}
