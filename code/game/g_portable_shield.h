// Portable retail shield wall. GPL-2.0-only.
#ifndef G_PORTABLE_SHIELD_H
#define G_PORTABLE_SHIELD_H
struct gentity_s;
namespace jar { namespace shieldCopy { struct State; struct Properties; } }
bool G_PortableShield(const gentity_s&);
bool G_PortableShieldCopyState(const gentity_s&,jar::shieldCopy::State&);
gentity_s* G_PortableShieldSpawnCopy(const jar::shieldCopy::State&,const float* origin,const float* angles);
bool G_PortableShieldClear(const gentity_s&);
bool G_PortableShieldProperties(const gentity_s&,jar::shieldCopy::Properties&);
bool G_PortableShieldSetProperties(gentity_s&,const jar::shieldCopy::Properties&);
bool G_PortableShieldMoved(gentity_s&);
bool G_PortableShieldRestore();
void G_PortableShieldCleanup();
bool G_UsePortableShield(gentity_s*);
void portable_shield_think(gentity_s*);
void portable_shield_touch(gentity_s*,gentity_s*,trace_t*);
void portable_shield_pain(gentity_s*,gentity_s*,gentity_s*,const vec3_t,int,int,int);
void portable_shield_die(gentity_s*,gentity_s*,gentity_s*,int,int,int,int);
#endif
