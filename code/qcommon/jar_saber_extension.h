// Compatibility wrapper for the common C/C++ ignition policy.
#ifndef JAR_SABER_EXTENSION_H
#define JAR_SABER_EXTENSION_H
#include "qcommon/jar_combat.h"
namespace jar {
inline float ExtendSaber(float length,float maximum,int frameMs,int selected,bool protectedBlade) {
    return JAR_ExtendSaber(length,maximum,frameMs,selected,protectedBlade);
}
}
#endif
