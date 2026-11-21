// JA:R inventory and owned Sandbox integration. GPL-2.0-only.
#ifndef G_JAR_WORKSHOP_H
#define G_JAR_WORKSHOP_H
#include "qcommon/jar_workshop_state.h"
enum class WorkshopSpawnFeedback { Individual, Assembly };
void G_JAR_WorkshopInit(bool loading);
void G_JAR_WorkshopFrame();
void G_JAR_WorkshopInput(gentity_t *ent,usercmd_t *cmd);
bool G_JAR_WorkshopControls(const gentity_t*);
bool G_JAR_WorkshopTransportAllowed(const gentity_t&);
bool G_JAR_WorkshopTeleportAssembly(gentity_t& source,gentity_t& actor);
bool G_JAR_WorkshopTeleportProjectile(gentity_t&,const vec3_t previous,const vec3_t end,bool clipped,
 const float* rollingVelocity=nullptr);
bool G_JAR_WorkshopOutsideMapPoint(const vec3_t origin);
bool G_JAR_WorkshopNativeInput(gentity_t*,usercmd_t*);
void G_JAR_WorkshopTouch(gentity_t*,gentity_t*);
struct JarSweepHull;
qboolean G_JAR_WorkshopTraceProp(const gentity_t*,const vec3_t origin,const vec3_t angles,
 const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,int mask,trace_t*,
 const JarSweepHull* movingHull=nullptr);
bool G_JAR_WorkshopPropDepth(const gentity_t& fixed,const vec3_t origin,const vec3_t mins,
 const vec3_t maxs,const JarSweepHull* movingHull,float& depth);
const char *G_JAR_WorkshopToolHint();
void G_JAR_WorkshopViewRay(vec3_t origin,vec3_t direction);
void G_JAR_WorkshopDrawBeam();
void G_JAR_WorkshopDrawRopes();
struct JarWeaponBarEntry {unsigned id,family;const char*name;const char*icon;};
int G_JAR_WorkshopWeaponBar(JarWeaponBarEntry* entries,int capacity,int& selected);
bool G_JAR_WorkshopCycleWeapon(int direction);
void G_JAR_WorkshopSpawned(bool hubReturn);
void G_JAR_WorkshopRead(bool autosave,int version,bool hubReturn);
void G_JAR_WorkshopWrite(bool autosave,int version);
qboolean G_JAR_WorkshopPrepareSave(qboolean autosave);
void G_JAR_WorkshopTransition();
void G_JAR_WorkshopForget(gentity_t *ent,bool freeing=true);
bool G_JAR_WorkshopOwns(const gentity_t *ent);
void G_JAR_WorkshopTrackConsole(gentity_t* ent);
void G_JAR_WorkshopTrackConsoleChild(gentity_t* root,gentity_t* child);
void G_JAR_WorkshopConsoleActor(gentity_t* spawner,gentity_t* actor);
bool G_JAR_WorkshopActorSpawn(const gentity_t& actor);
inline bool G_JAR_CountCampaignActor(const gentity_t& actor){return !G_JAR_WorkshopActorSpawn(actor);}
constexpr int JAR_NPC_OVERLAP_SPAWN=1<<28;
void G_JAR_WorkshopBreakableDetach(gentity_t *ent);
bool G_JAR_WorkshopUseGate(gentity_t& source,gentity_t* other,gentity_t* activator);
bool G_JAR_WorkshopTrackDrop(gentity_t *source,gentity_t *drop);
bool G_JAR_WorkshopTrackChild(gentity_t *source,gentity_t *child);
bool G_JAR_WorkshopRackPickupSuspended(const gentity_t& item);
void G_JAR_WorkshopRackPickupLoose(gentity_t& item);
bool G_JAR_WorkshopMoveRackStock(gentity_t& rack,const vec3_t origin,const vec3_t angles);
void G_JAR_WorkshopDetachChild(gentity_t *source,gentity_t *child);
gentity_t *G_JAR_SpawnSetupChild(gentity_t *parent);
qboolean G_JAR_WorkshopCommand(const char *cmd);
jar::workshop::Weapon G_JAR_WorkshopCurrentWeapon();
void G_JAR_WorkshopAcquired(const jar::workshop::Weapon& previous);
#endif

void G_JAR_WorkshopShutdown();
