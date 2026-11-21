// Persistent menu markers alongside unchanged authored interest tables.
// GPL-2.0-only. Include after g_local.h for native types.
#ifndef G_TARGET_MARKERS_H
#define G_TARGET_MARKERS_H
qboolean G_ClearLineOfSight(const vec3_t,const vec3_t,int,int);
bool G_PortableInterest(const gentity_t&);
void G_RegisterInterest(gentity_t*);
int G_ChoosePortableInterest(gentity_t*,const vec3_t,float&);
bool G_FirePortableInterest(gentity_t*,int);
void G_TargetMarkersReset();
void G_TargetMarkersRestore();
void G_TargetMarkersForget(gentity_t*);
bool G_TargetMarkersPresent();
char* G_NearestLocation(gentity_t*);
char* G_GetLocationForEnt(gentity_t*);
#endif
