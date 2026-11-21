namespace {
std::vector<JarNpcTemplate> jarNpcCatalog;
int jarNpcOrdinal=0;
gentity_t *jarAuthoredDeath=nullptr;
constexpr unsigned jarNpcState=0x00800000u,jarNpcStateMask=0x3f800000u;
const JarNpcTemplate *JarNpcSource(const char *name){if(name)for(const auto& e:jarNpcCatalog)if(!Q_stricmp(name,e.name.c_str()))return &e;return nullptr;}
bool JarNpcSafety(){return strcmp(gi.cvar("jar_npcSafety","1",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,"0")!=0;}
const char *JarNpcMissionReason(const gentity_t *ent){
    if(ent->NPC_targetname||ent->script_targetname||ent->cameraGroup||ent->message||ent->target||ent->target2||ent->target3||ent->target4||ent->NPC_target||ent->closetarget||ent->paintarget||ent->opentarget||ent->parms||ent->team)return "mission-metadata";
    for(int i=0;i<NUM_BSETS;++i)if(ent->behaviorSet[i])return "script";
    if(unsigned(ent->flags)&~jarNpcStateMask)return "actor-flags";
    if(unsigned(ent->spawnflags)&~(15u|16u|256u|512u|1024u|2048u|4096u))return "actor-spawnflags";
    char *key,*value;for(unsigned i=0;G_SpawnField(i,&key,&value);++i){bool ordinary=false;
        for(const char *allowed:{"classname","origin","angle","angles","notsingle","spawnflags","targetname","count","delay","wait","NPC_type"})ordinary|=!Q_stricmp(key,allowed);
        if(!ordinary)return "map-metadata";}
    return nullptr;
}
}
extern int NPC_WeaponsForTeam(team_t team,int spawnflags,const char *name);
int G_JAR_SourceEnemyTeam(const JarNpcTemplate& source,int spawnflags){
    if(source.team==TEAM_PLAYER&&(source.npcClass==CLASS_JEDI||source.npcClass==CLASS_KYLE||source.npcClass==CLASS_LUKE))return TEAM_ENEMY;
    if(source.team==TEAM_ENEMY){
        bool saber=source.weapon==WP_SABER;
        if(source.npcClass==CLASS_REBORN&&source.weapon==WP_NONE)saber=(NPC_WeaponsForTeam(team_t(source.team),spawnflags,source.name.c_str())&(1u<<WP_SABER))!=0;
        if(source.npcClass==CLASS_TAVION||source.npcClass==CLASS_ALORA||source.npcClass==CLASS_DESANN||source.npcClass==CLASS_SHADOWTROOPER||(source.npcClass==CLASS_REBORN&&saber))return TEAM_PLAYER;
    }
    return source.enemyTeam;
}
void G_JAR_ApplyNpcAllegiance(gentity_t *actor){
    if(!actor||!actor->client||!(unsigned(actor->flags)&jarNpcState))return;
    actor->client->playerTeam=team_t((unsigned(actor->flags)>>24)&7u);
    actor->client->enemyTeam=team_t((unsigned(actor->flags)>>27)&7u);
    actor->client->ps.persistant[PERS_TEAM]=actor->client->playerTeam;
}
void G_JAR_InitNpcCatalog(){
    jarNpcCatalog.clear();jarNpcOrdinal=0;jarAuthoredDeath=nullptr;G_JAR_InitActorRecovery();
    gi.cvar("jar_npcSafety","1",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    gi.cvar("jar_actorContinuity","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    if(enemies)G_JAR_ReadNpcCatalog(jarNpcCatalog);
    unsigned usable=0;for(const auto& e:jarNpcCatalog){usable+=e.reason.empty();if(gi.JAR_Log)gi.JAR_Log(2,"randomizer","npc_catalog_entry","name=%s class=%d team=%d enemyTeam=%d width=%d height=%d reason=%s",e.name.c_str(),e.npcClass,e.team,e.enemyTeam,e.width,e.height,e.reason.empty()?"available":e.reason.c_str());}
    if(enemies&&gi.JAR_Log)gi.JAR_Log(0,"randomizer","npc_catalog","definitions=%u available=%u safety=%d",unsigned(jarNpcCatalog.size()),usable,JarNpcSafety());
}
void G_JAR_RandomizeMapNPC(gentity_t *ent,int ordinal){
    if(!enemies||!ent||!ent->NPC_type||ordinal<=0||(unsigned(ent->flags)&jarNpcState))return;
    const auto source=JarNpcSource(ent->NPC_type);const bool safety=JarNpcSafety();
    const char *reason=!source||source->team<TEAM_FREE||source->team>TEAM_NEUTRAL||source->enemyTeam<TEAM_FREE||source->enemyTeam>TEAM_NEUTRAL?"unknown-source-template-or-allegiance":source->npcClass==CLASS_VEHICLE?"vehicle-spawner-preserved":nullptr;
    if(!reason&&safety)reason=JarNpcMissionReason(ent);
    if(!reason&&safety&&source->team!=TEAM_ENEMY)reason="mission-safety-preserves-allies-and-neutrals";
    std::vector<const JarNpcTemplate*> pool;
    if(!reason)for(const auto& e:jarNpcCatalog)if(e.reason.empty()&&(!safety||(e.width<=source->width&&e.height<=source->height)))pool.push_back(&e);
    if(!reason&&pool.empty())reason="no-compatible-template";
    const auto chosen=reason?nullptr:pool[fixedMapping ? jar::FixedCatalogIndex(seed,"npc-types-v1",source->name.c_str(),unsigned(pool.size()),
        [&](unsigned i){return pool[i]->name.c_str();}) : jar::PickupDecision(seed,level.mapname,ordinal,"npc-spawns-v2","")%pool.size()];
    if(gi.JAR_Log)gi.JAR_Log(2,"randomizer","enemy_decision","version=2 map=%s seed=%u ordinal=%d original=%s chosen=%s candidates=%u applied=%d safety=%d reason=%s actor=%s spawner=%s",level.mapname,seed,ordinal,ent->NPC_type,chosen?chosen->name.c_str():ent->NPC_type,unsigned(pool.size()),chosen!=nullptr,safety,reason?reason:"selected",ent->NPC_targetname?ent->NPC_targetname:"",ent->targetname?ent->targetname:"");
    if(!chosen)return;
    // Serialized existing flags hold allegiance; no save-structure/ABI changes.
    ent->flags=(unsigned(ent->flags)&~jarNpcStateMask)|jarNpcState|(unsigned(source->team)&7u)<<24|(unsigned(G_JAR_SourceEnemyTeam(*source,ent->spawnflags))&7u)<<27;
    ent->NPC_type=G_NewString(chosen->name.c_str());
    ent->spawnflags&=~15; // wrapper-specific loadout/variant bits, never trigger/mission bits
    enemyDigest=jar::PickupHashText(jar::PickupHashWord(enemyDigest,ordinal),chosen->name.c_str());++enemyChanges;G_JAR_RandomizerStatus();
}
void G_JAR_RandomizeNpcSpawner(gentity_t *ent){G_JAR_RandomizeMapNPC(ent,++jarNpcOrdinal);}
void G_JAR_ApplyNpcIdentity(const gentity_t *spawner,gentity_t *actor){
    if(!spawner||!actor||!actor->client||!(unsigned(spawner->flags)&jarNpcState))return;
    actor->flags=(unsigned(actor->flags)&~jarNpcStateMask)|(unsigned(spawner->flags)&jarNpcStateMask);
    G_JAR_ApplyNpcAllegiance(actor);
    // Native NPC_targetname already supplied both actor identifiers. A distinct
    // script_targetname on the spawner still belongs to that spawner; never
    // overwrite the actor's native ICARUS identity with it.
    if(spawner->NPC_targetname)actor->NPC_targetname=G_NewString(spawner->NPC_targetname);
    if(spawner->cameraGroup)actor->cameraGroup=G_NewString(spawner->cameraGroup);
    const char *name=actor->targetname?actor->targetname:actor->script_targetname;
    if(name)Q_strncpyz(actor->client->clientInfo.name,name,sizeof(actor->client->clientInfo.name));
    if(gi.JAR_Log)gi.JAR_Log(2,"randomizer","npc_identity","entity=%d type=%s targetname=%s script_targetname=%s team=%d enemyTeam=%d",actor->s.number,spawner->NPC_type,actor->targetname?actor->targetname:"",actor->script_targetname?actor->script_targetname:"",actor->client->playerTeam,actor->client->enemyTeam);
}
gentity_t *G_JAR_AuthorizeActorDeath(gentity_t *actor){auto previous=jarAuthoredDeath;jarAuthoredDeath=actor;return previous;}
bool G_JAR_PreserveActor(gentity_t *actor){
    if(actor==jarAuthoredDeath || G_JAR_WorkshopOwns(actor))return false;
    if(!actor||!actor->NPC||!actor->client||!actor->inuse||actor->client->ps.pm_type==PM_DEAD||actor->client->playerTeam!=TEAM_PLAYER||(!actor->targetname&&!actor->script_targetname)||actor->client->NPC_class==CLASS_VEHICLE)return false;
    if((actor->svFlags&SVF_KILLED_SELF)||actor->target||actor->target2||actor->target4||actor->behaviorSet[BSET_DEATH]||actor->behaviorSet[BSET_FFDEATH]||actor->e_ThinkFunc==thinkF_G_FreeEntity)return false;
    if(strcmp(gi.cvar("jar_actorContinuity","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,"1"))return false;
    actor->health=actor->client->ps.stats[STAT_HEALTH]=1;
    if(gi.JAR_Log)gi.JAR_Log(1,"randomizer","actor_continuity","entity=%d name=%s action=prevent-unplanned-death task_state=retained",actor->s.number,actor->targetname?actor->targetname:actor->script_targetname);
    return true;
}
