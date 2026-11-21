// Context and clipboard share native boundary properties. GPL-2.0-only.
#include "g_shipboundary.h"
#include <cmath>
bool G_ShipBoundaryClass(const gentity_t&e){return e.inuse&&e.classname&&!strcmp(e.classname,"trigger_shipboundary");}
bool G_ShipBoundarySet(gentity_t&e,float seconds,bool preserveEmpty){
 if(!G_ShipBoundaryClass(e)||!std::isfinite(seconds)||seconds<.1f||seconds>60)return false;
 G_ShipBoundaryForget(&e);e.count=int(std::lround(seconds*1000));e.spawnflags=preserveEmpty?e.spawnflags|1:e.spawnflags&~1;return true;
}
