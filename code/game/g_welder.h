#ifndef G_JAR_WELDER_H
#define G_JAR_WELDER_H
#include "qcommon/jar_welder.h"
inline bool G_WelderPortable(const gentity_t&e){return e.inuse&&jar::welder::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_welder");}
inline bool G_WelderRead(const gentity_t&e,jar::welder::Settings&s){if(!G_WelderPortable(e))return false;s.reach=e.radius;s.radius=e.random;s.interval=e.wait;s.damage=e.damage;s.enabled=!(e.spawnflags&1);return jar::welder::Valid(s);}
bool G_WelderMoved(gentity_t&);
inline bool G_WelderSet(gentity_t&e,const jar::welder::Settings&s){
 if(!G_WelderPortable(e)||!jar::welder::Valid(s))return false;
 e.radius=s.reach;e.random=s.radius;e.wait=s.interval;e.damage=s.damage;e.spawnflags=s.enabled?e.spawnflags&~1:e.spawnflags|1;
 if(!s.enabled)e.s.loopSound=0;
 // Editing, moving or toggling cannot bypass the current pulse cooldown.
 return G_WelderMoved(e);
}
bool G_WelderSpawn(gentity_t*);
void G_WelderUse(gentity_t*);
void G_WelderFrame();
void G_WelderForget(gentity_t*);
void G_WelderReset();
void G_WelderRestore();
#endif
