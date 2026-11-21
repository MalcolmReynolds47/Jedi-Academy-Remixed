#ifndef G_JAR_SPOTLIGHT_H
#define G_JAR_SPOTLIGHT_H
#include "qcommon/jar_spotlight.h"
inline bool G_SpotlightPortable(const gentity_t&e){return e.inuse&&jar::spotlight::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_spotlight");}
inline bool G_SpotlightRead(const gentity_t&e,jar::spotlight::Settings&s){if(!G_SpotlightPortable(e))return false;s.range=e.wait;s.cone=e.speed;s.light=e.random;VectorCopy(e.pos1,s.rgb);s.tracking=e.count==0;s.enabled=!(e.spawnflags&1);return e.count>=0&&e.count<=1&&jar::spotlight::Valid(s);}
bool G_SpotlightMoved(gentity_t&);
inline bool G_SpotlightSet(gentity_t&e,const jar::spotlight::Settings&s){if(!G_SpotlightPortable(e)||!jar::spotlight::Valid(s))return false;e.wait=s.range;e.speed=s.cone;e.random=s.light;VectorCopy(s.rgb,e.pos1);e.count=s.tracking?0:1;e.spawnflags=s.enabled?e.spawnflags&~1:e.spawnflags|1;if(!s.enabled)e.alt_fire=qfalse;e.radius=0;e.attackDebounceTime=0;return G_SpotlightMoved(e);}
bool G_SpotlightSpawn(gentity_t*);
void G_SpotlightUse(gentity_t*);
void G_SpotlightFrame();
void G_SpotlightForget(gentity_t*);
void G_SpotlightReset();
void G_SpotlightRestore();
#endif
