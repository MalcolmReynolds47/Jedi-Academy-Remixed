// Include after g_local.h. GPL-2.0-only.
#ifndef G_RANDOMIZER_H
#define G_RANDOMIZER_H
#include <string>
#include <vector>
struct JarNpcTemplate {
    std::string name,model,reason,legacyAnimation;
    std::vector<std::string> legacyModels,legacySkins;
    int npcClass=0,team=0,enemyTeam=0,weapon=0,width=16,height=DEFAULT_MAXS_2-DEFAULT_MINS_2;
};
// Legacy segmented actors are opt-in for Sandbox; preserve seeded campaign pools.
void G_JAR_ReadNpcCatalog(std::vector<JarNpcTemplate>& out,bool allowLegacyModels=false);
void G_JAR_InitNpcCatalog();
void G_JAR_RandomizeNpcSpawner(gentity_t *ent);
int G_JAR_SourceEnemyTeam(const JarNpcTemplate& source,int spawnflags);
void G_JAR_ApplyNpcAllegiance(gentity_t *actor);
void G_JAR_ApplyNpcIdentity(const gentity_t *spawner,gentity_t *actor);
bool G_JAR_PreserveActor(gentity_t *actor);
// v1 recovery markers use the two previously unused high entity flag bits.
// They and all native script/controller state use existing save serialization.
void G_JAR_InitActorRecovery();
void G_JAR_RestoreActorRecovery();
void G_JAR_EnrollActor(const gentity_t *spawner,gentity_t *actor);
void G_JAR_ForgetActor(gentity_t *actor);
bool G_JAR_IsActorStandin(const gentity_t *actor);
bool G_JAR_RemoveToStandin(gentity_t *actor);
void G_JAR_EnforceActorStandin(gentity_t *actor);
bool G_JAR_SuspendActorRecovery(bool suspended);
struct JarActorRecoverySuspension {
    bool previous;
    JarActorRecoverySuspension():previous(G_JAR_SuspendActorRecovery(true)){}
    ~JarActorRecoverySuspension(){G_JAR_SuspendActorRecovery(previous);}
};
struct JarActorRecoveryDeath {
    gentity_t *actor=nullptr;
    int tasks[NUM_TIDS],torso,legs,torsoTime,legsTime,friction,behavior,defaultBehavior,tempBehavior;
    float yaw,pitch;
    vec3_t mins,maxs;
    explicit JarActorRecoveryDeath(gentity_t *entity);
    ~JarActorRecoveryDeath();
    bool Active() const {return actor!=nullptr;}
};
gentity_t *G_JAR_AuthorizeActorDeath(gentity_t *actor);
struct JarAuthoredDeathScope {gentity_t *previous; explicit JarAuthoredDeathScope(gentity_t *actor):previous(G_JAR_AuthorizeActorDeath(actor)){} ~JarAuthoredDeathScope(){G_JAR_AuthorizeActorDeath(previous);}};
void G_JAR_InitPickupRandomizer();
void G_JAR_RandomizeMapPickup(gentity_t *ent, int mapOrdinal);
void G_JAR_RandomizeMapNPC(gentity_t *ent, int mapOrdinal);
void G_JAR_RandomizePlayerLoadout(gentity_t *ent, int spawnflags);
void G_JAR_RandomizerStatus();
bool G_JAR_NpcDefinitionMatches(const char *name, unsigned fingerprint);
void G_JAR_ReportRandomizer();
void G_JAR_RandomizeMapObject(gentity_t *ent, int mapOrdinal);
bool G_JAR_ProjectilesEnabled();
bool G_JAR_InstantProjectileLock(const gentity_t *actor,int target);
void G_JAR_InitShotCatalog(bool enabled);
void G_JAR_GetVehicleShots(std::vector<int>& out);
void G_JAR_FinishShotCatalog();
void G_JAR_RefreshShotResources();
void G_JAR_RefreshVehicleResources();
#endif
