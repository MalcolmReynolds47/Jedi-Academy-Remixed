// Portable reflection-probe submission contract. GPL-2.0-only.
#ifndef JAR_REFLECTION_PROBE_H
#define JAR_REFLECTION_PROBE_H
#include <cmath>
#include <cstring>
#include "../../shared/qcommon/jar_probe.h"
namespace jar { namespace reflection {
struct Settings {float radius=JAR_PROBE_DEFAULT_RADIUS;bool enabled=true;};
inline bool Class(const char* name){return name&&!std::strcmp(name,"misc_cubemap");}
inline bool Valid(const Settings& s){return JAR_ProbeRadiusValid(s.radius)!=0;}
}}
#endif
