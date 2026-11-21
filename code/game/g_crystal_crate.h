#ifndef G_JAR_CRYSTAL_CRATE_H
#define G_JAR_CRYSTAL_CRATE_H
#include "qcommon/jar_crystal_crate.h"
inline bool G_CrystalCratePortable(const gentity_t&e){return e.inuse&&jar::crystalCrate::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_crystal_crate");}
inline bool G_CrystalCratePart(const gentity_t&e){auto*p=e.lastEnemy;return p&&p->inuse&&p->owner==&e&&p->classname&&!strcmp(p->classname,"info_notnull")&&p->model&&!strcmp(p->model,jar::crystalCrate::crystal);}
inline gentity_t*G_CrystalCrateRoot(gentity_t*e){if(!e||!e->inuse)return nullptr;if(G_CrystalCratePortable(*e))return e;auto*r=e->owner;return r&&G_CrystalCratePortable(*r)&&G_CrystalCratePart(*r)&&r->lastEnemy==e?r:nullptr;}
inline bool G_CrystalCrateRead(const gentity_t&e,jar::crystalCrate::Settings&s){if(!G_CrystalCratePortable(e))return false;s.hitDamage=e.damage;s.hitRadius=e.radius;s.blastDamage=e.splashDamage;s.blastRadius=float(e.splashRadius);s.solid=!(e.spawnflags&1);return jar::crystalCrate::Valid(s);}
bool G_CrystalCrateMoved(gentity_t&);
inline bool G_CrystalCrateSet(gentity_t&e,const jar::crystalCrate::Settings&s){
 if(!G_CrystalCratePortable(e)||!jar::crystalCrate::Valid(s)||s.blastRadius!=std::floor(s.blastRadius))return false;
 if(s.solid&&(e.spawnflags&1)){trace_t hit{};gi.trace(&hit,e.currentOrigin,e.mins,e.maxs,e.currentOrigin,e.s.number,MASK_SOLID|CONTENTS_BODY,(EG2_Collision)0,0);if(hit.startsolid||hit.allsolid)return false;}
 e.damage=s.hitDamage;e.radius=s.hitRadius;e.splashDamage=s.blastDamage;e.splashRadius=int(s.blastRadius);e.spawnflags=s.solid?e.spawnflags&~1:e.spawnflags|1;
 e.contents=s.solid?CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP:CONTENTS_SHOTCLIP;
 return G_CrystalCrateMoved(e);
}
bool G_CrystalCrateSpawn(gentity_t*);
bool G_CrystalCratePain(gentity_t*);
bool G_CrystalCrateDie(gentity_t*,gentity_t*);
void G_CrystalCrateForget(gentity_t*);
void G_CrystalCrateReset();
void G_CrystalCrateRestore();
#endif
