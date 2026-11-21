// Actual native callbacks and target dispatcher; only unrelated engine services
// (script VM, authored-map search, spawn parser and console) are fixture imports.
char*vtos(const vec3_t p){static char text[96];Com_sprintf(text,sizeof(text),"%g %g %g",p[0],p[1],p[2]);return text;}
qboolean G_ActivateBehavior(gentity_t*e,int behavior){assert(behavior==BSET_USE);return qfalse;}
qboolean G_SpawnFloat(const char*key,const char*value,float*out){
 char *text;const qboolean found=G_SpawnString(key,value,&text);*out=strtof(text,NULL);return found;
}
void G_DebugPrint(int level,const char*fmt,...){}
void G_TeamCommand(team_t team,char*command){assert(0);}
static gentity_t*authoredDestination;
gentity_t*G_PickTarget(char*name){return authoredDestination;}
static gentity_t*activeStateMapTarget;
gentity_t*G_Find(gentity_t*from,int field,const char*name){return !from&&activeStateMapTarget&&field==FOFS(targetname)&&name&&!strcmp(name,activeStateMapTarget->targetname)?activeStateMapTarget:NULL;}
void AddRemap(const char*oldShader,const char*newShader,float timeOffset){assert(0);}
const char*BuildShaderStateConfig(void){return "";}
#define FINAL_BUILD
#include "game/bg_saga.h"
qboolean gSiegeRoundBegun=qtrue;
siegeClass_t bgSiegeClasses[MAX_SIEGE_CLASSES];
int gTrigFallSound;
qboolean G_SpawnInt(const char*key,const char*value,int*out){assert(!level.numSpawnVars&&level.spawning);*out=atoi(value);return qfalse;}
qboolean G_SpawnString(const char*key,const char*value,char**out){
 assert(level.spawning);
 for(int i=0;i<level.numSpawnVars;++i)if(!Q_stricmp(key,level.spawnVars[i][0])){*out=level.spawnVars[i][1];return qtrue;}
 *out=(char*)value;
 return qfalse;
}
void SiegeItemRemoveOwner(gentity_t*e,gentity_t*carrier){assert(0);}
void G_SetMovedir(vec3_t angles,vec3_t dir){AngleVectors(angles,dir,NULL,NULL);VectorClear(angles);}
qboolean G_PointInBounds(vec3_t point,vec3_t mins,vec3_t maxs){for(int i=0;i<3;++i)if(point[i]<mins[i]||point[i]>maxs[i])return qfalse;return qtrue;}
void ClientRespawn(gentity_t*e){assert(0);}
static int sectionFallSounds;
void Jetpack_Off(gentity_t*e){assert(e->client&&e->client->ps.fallingToDeath);}
void G_EntitySound(gentity_t*e,int channel,int sound){
 assert(e->client&&channel==CHAN_VOICE&&sound==1);
 if(e->client->ps.fallingToDeath)++sectionFallSounds;
 else {assert(e->client->inSpaceIndex);++spaceSounds;}
}
#define MAX_AMMO_GIVE 2
#define STATION_RECHARGE_TIME 100
ammoData_t ammoData[AMMO_MAX];
void func_wait_return_solid(gentity_t*e){assert(JAR_MP_UsableReturn(e));}
#include "game/g_jar_native_usable.inc"
#include "game/g_jar_native_stations.inc"
#include "game/g_jar_native_triggers.inc"
#include "game/g_jar_native_hurt.inc"
#include "game/bg_jar_native_jumppad.inc"
#include "game/g_jar_native_push_trigger.inc"
#include "game/g_jar_native_teleport_trigger.inc"
#include "game/g_jar_native_targets.inc"
#include "mp_native_pickup.inc"
#include "game/g_jar_native_actions.inc"
#include "game/g_jar_native_random.inc"
#include "game/g_jar_native_timer.inc"
#include "game/g_jar_native_switches.inc"
#include "game/g_jar_native_laser.inc"
#include "game/g_jar_native_media.inc"
#include "game/g_jar_native_fx.inc"
#include "game/g_jar_native_transport.inc"
#include "game/g_jar_native_teleport.inc"
#include "game/g_jar_native_use.inc"
#undef FINAL_BUILD
