// Portable reflection controller state uses native serialized fields.
// GPL-2.0-only.
#ifndef G_REFLECTION_PROBE_H
#define G_REFLECTION_PROBE_H
#include "qcommon/jar_reflection_probe.h"
#include <climits>
inline bool G_ReflectionPortable(const gentity_t& e) {
    return e.inuse && jar::reflection::Class(e.classname) && e.model && !strcmp(e.model,"@jar_reflection");
}
inline bool G_ReflectionRead(const gentity_t& e, jar::reflection::Settings& s) {
    if (!G_ReflectionPortable(e) || e.count<1) return false;
    s.radius=e.radius; s.enabled=(e.spawnflags&1)==0;
    return jar::reflection::Valid(s);
}
inline bool G_ReflectionSet(gentity_t& e, const jar::reflection::Settings& s) {
    if (!G_ReflectionPortable(e) || !jar::reflection::Valid(s)) return false;
    e.radius=s.radius; e.spawnflags=(e.spawnflags&~1)|(s.enabled?0:1);
    return true;
}
inline bool G_ReflectionRefresh(gentity_t& e) {
    jar::reflection::Settings s;
    if (!G_ReflectionRead(e,s)) return false;
    e.count=e.count==INT_MAX?1:e.count+1;
    return true;
}
inline bool G_ReflectionMoved(gentity_t& e) {
    if (!G_ReflectionPortable(e)) return false;
    VectorCopy(e.currentOrigin,e.s.origin); VectorCopy(e.currentAngles,e.s.angles);
    return true;
}
int G_ReflectionCount(const gentity_t* skip=nullptr);
bool G_ReflectionSpawn(gentity_t&);
bool G_ReflectionRestore();
unsigned G_ReflectionEpoch();
#endif
