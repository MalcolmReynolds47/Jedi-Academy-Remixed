#pragma once
#include "qcommon/jar_waypoint.h"
bool G_WaypointPortable(const gentity_t&);
bool G_WaypointRead(const gentity_t&,jar::waypoint::Settings&);
bool G_WaypointSet(gentity_t&,const jar::waypoint::Settings&);
bool G_WaypointSpawn(gentity_t&);
bool G_WaypointRestore();
void G_WaypointFrame();
void G_WaypointReset();
const char*G_WaypointStatus(const gentity_t&);
