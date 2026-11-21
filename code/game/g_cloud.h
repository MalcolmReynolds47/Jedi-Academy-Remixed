// Cloud controllers use existing native saved fields. GPL-2.0-only.
#ifndef G_CLOUD_H
#define G_CLOUD_H
#include "qcommon/jar_cloud.h"
inline bool G_CloudClass(const gentity_t&e){return e.inuse&&jar::cloud::Class(e.classname);}
inline bool G_CloudPortable(const gentity_t&e){return G_CloudClass(e)&&e.model&&!strcmp(e.model,"@jar_cloud");}
inline bool G_CloudRead(const gentity_t&e,jar::cloud::Settings&s){
 if(!G_CloudClass(e))return false;s.radius=e.radius;s.inner=e.random;s.curve=e.wait;
 s.tube=(e.spawnflags&jar::cloud::tubeFlag)!=0;s.alternate=(e.spawnflags&jar::cloud::alternateFlag)!=0;s.enabled=(e.spawnflags&jar::cloud::offFlag)==0;return jar::cloud::Valid(s);
}
inline bool G_CloudSet(gentity_t&e,const jar::cloud::Settings&s){
 if(!G_CloudClass(e)||!jar::cloud::Valid(s))return false;
 e.radius=s.radius;e.random=s.inner;e.wait=s.curve;
 e.spawnflags=(e.spawnflags&~7)|(s.tube?1:0)|(s.alternate?2:0)|(s.enabled?0:4);return true;
}
inline bool G_CloudMoved(gentity_t&e){if(!G_CloudClass(e))return false;VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);return true;}
bool G_CloudHasLayers();
void G_CloudRestore();
#endif
