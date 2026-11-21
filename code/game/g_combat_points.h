// Include after g_local.h. GPL-2.0-only.
#ifndef G_COMBAT_POINTS_H
#define G_COMBAT_POINTS_H
#include "qcommon/jar_combat_point.h"
bool G_CombatPointPortable(const gentity_t&);
bool G_CombatPointRead(const gentity_t&,jar::combatPoint::Settings&);
bool G_CombatPointSet(gentity_t&,const jar::combatPoint::Settings&);
bool G_CombatPointSpawn(gentity_t&);
bool G_CombatPointAvailable(int);
void G_CombatPointForget(gentity_t*);
void G_CombatPointReset();
bool G_CombatPointRestore();
void G_CombatPointFrame();
const char*G_CombatPointStatus(const gentity_t&);
#endif
