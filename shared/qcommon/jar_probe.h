// Shared renderer input and Sandbox reflection settings. GPL-2.0-only.
#ifndef JAR_PROBE_H
#define JAR_PROBE_H
#include <math.h>

enum { JAR_PROBE_LIMIT=64, JAR_PROBE_MIN_RADIUS=16, JAR_PROBE_MAX_RADIUS=32768,
       JAR_PROBE_DEFAULT_RADIUS=512, JAR_MP_PROBE_TAG=14 };
enum { JAR_PROBE_UNSUPPORTED=-1, JAR_PROBE_DISABLED=-2, JAR_PROBE_NO_WORLD=-3,
       JAR_PROBE_INVALID=-4, JAR_PROBE_CAPACITY=-5 };
typedef struct { unsigned owner, revision; float origin[3], radius; } jarProbe_t;

static inline int JAR_ProbeRadiusValid(float radius){
#ifdef JAR_MP
 return isfinite(radius)&&radius>0&&radius<=2*(64*1024);
#else
 return isfinite(radius)&&radius>=JAR_PROBE_MIN_RADIUS&&radius<=JAR_PROBE_MAX_RADIUS;
#endif
}
static inline int JAR_ProbeValid(const jarProbe_t* probe){
 if(!probe->owner||!JAR_ProbeRadiusValid(probe->radius))return 0;
 for(int k=0;k<3;++k)if(!isfinite(probe->origin[k])||fabsf(probe->origin[k])>65535)return 0;
 return 1;
}
#ifdef __cplusplus
namespace jar { namespace reflection {
using Probe=jarProbe_t;
constexpr int limit=JAR_PROBE_LIMIT;
constexpr unsigned settleMilliseconds=150;
enum Result { Unsupported=JAR_PROBE_UNSUPPORTED, Disabled=JAR_PROBE_DISABLED, NoWorld=JAR_PROBE_NO_WORLD,
              Invalid=JAR_PROBE_INVALID, Capacity=JAR_PROBE_CAPACITY };
inline bool Valid(const Probe& p){return JAR_ProbeValid(&p)!=0;}
inline bool SameCapture(const Probe& a,const Probe& b){
 if(a.owner!=b.owner||a.revision!=b.revision)return false;
 for(int k=0;k<3;++k)if(a.origin[k]!=b.origin[k])return false;
 return true;
}
}}
#endif
#endif
