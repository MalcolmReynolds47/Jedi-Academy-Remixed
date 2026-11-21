#include "g_waypoints.h"
bool G_WaypointPortable(const gentity_t&e){return e.inuse&&e.s.number>0&&e.s.number<ENTITYNUM_WORLD&&jar::waypoint::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_waypoint");}
bool G_WaypointRead(const gentity_t&e,jar::waypoint::Settings&out){if(!G_WaypointPortable(e))return false;jar::waypoint::Settings s{e.wait,!(e.svFlags&SVF_INACTIVE)};if(!jar::waypoint::Valid(s))return false;out=s;return true;}
bool G_WaypointSet(gentity_t&e,const jar::waypoint::Settings&s){if(!G_WaypointPortable(e)||!jar::waypoint::Valid(s))return false;e.wait=s.range;e.svFlags=s.enabled?e.svFlags&~SVF_INACTIVE:e.svFlags|SVF_INACTIVE;return true;}
bool G_WaypointSpawn(gentity_t&e){
 if(!e.inuse||!jar::waypoint::Class(e.classname)||!jar::waypoint::Position(e.currentOrigin))return false;
 if(!waypointAdmission())return false;
 if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);e.model=G_NewString("@jar_waypoint");e.wait=512;e.radius=!strcmp(e.classname,"waypoint_small")?2:20;
 e.s.eType=ET_GENERAL;e.contents=CONTENTS_ITEM;e.takedamage=qfalse;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;e.e_UseFunc=useF_marker_use;e.svFlags=(e.svFlags&~SVF_INACTIVE)|SVF_PLAYER_USABLE;
 VectorSet(e.mins,-8,-8,-24);VectorSet(e.maxs,8,8,16);return true;
}
