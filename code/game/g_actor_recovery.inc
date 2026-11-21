// Opt-in v1 script-facing actor recovery. GPL-2.0-only.
// Keep the native controller in its original slot: ICARUS task groups, names,
// parms and relationships continue to refer to precisely the same owner.
namespace {
constexpr unsigned jarRecoveryEnrolled=0x40000000u,jarRecoveryStandin=0x80000000u;
bool jarRecoverySuspended=false;
bool JarRecoveryEnabled(){return !strcmp(gi.cvar("jar_actorRecovery","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,"1");}
bool JarRecoveryDeathMetadata(const gentity_t *e){
    return e->target||e->target2||e->target4||e->behaviorSet[BSET_DEATH]||e->behaviorSet[BSET_FFDEATH];
}
bool JarRecoveryEligible(const gentity_t *e){
    if(jarRecoverySuspended||!e||!e->inuse||e==jarAuthoredDeath||!e->NPC||!e->client||e->s.number<=0) return false;
    if(!(unsigned(e->flags)&jarRecoveryEnrolled)||(unsigned(e->flags)&jarRecoveryStandin)||!JarRecoveryEnabled()) return false;
    if(!e->script_targetname||!e->script_targetname[0]||!e->targetname||!e->targetname[0]||e->client->playerTeam!=TEAM_PLAYER) return false;
    if(e->client->ps.pm_type==PM_DEAD||e->client->NPC_class==CLASS_VEHICLE||e->m_pVehicle||e->s.m_iVehicleNum||e->owner) return false;
    if(e->m_iIcarusID<=0||!e->NPC->tempGoal||!e->NPC->tempGoal->inuse||JarRecoveryDeathMetadata(e)) return false;
    if((e->svFlags&SVF_KILLED_SELF)||(e->NPC->scriptFlags&SCF_FFDEATH)||e->NPC->charmedTime>level.time||e->NPC->group||e->NPC->troop) return false;
    // v1 qualifies ground humanoid controllers only. Flying templates and
    // custom-gravity actors need their own death/locomotion recovery contract.
    if(e->client->clientInfo.animFileIndex!=0 || (e->client->moveType!=MT_RUNJUMP && e->client->moveType!=MT_WALK) || e->client->noclip || (e->svFlags&SVF_CUSTOM_GRAVITY))return false;
    if(e->NPC->scriptFlags&SCF_SAFE_REMOVE)return false;
    // Player-clone controllers can rearm themselves inside ClientThink. Their
    // special equipment synchronization is outside this ground-ally contract.
    if(e->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)return false;
    if(e->s.eFlags&EF_LOCKED_TO_WEAPON) return false;
    if(e->client->ps.eFlags&(EF_HELD_BY_RANCOR|EF_HELD_BY_WAMPA|EF_HELD_BY_SAND_CREATURE)) return false;
    if(e->taskID[TID_BSTATE]>=0||e->taskID[TID_RESIZE]>=0) return false;
    if(g_entities[0].client&&g_entities[0].client->ps.viewEntity==e->s.number) return false;
    return true;
}
void JarRecoveryConvert(gentity_t *e,const char *cause){
    e->flags=unsigned(e->flags)|jarRecoveryStandin;
    e->NPC->combatPoint=-1;
    e->NPC->aiFlags&=~NPCAI_DIE_ON_IMPACT;
    auto bounded=[](bState_t state){return state==BS_WAIT||state==BS_CINEMATIC||state==BS_DEFAULT?state:BS_CINEMATIC;};
    e->NPC->behaviorState=bounded(e->NPC->behaviorState);
    e->NPC->defaultBehavior=bounded(e->NPC->defaultBehavior);
    if(e->NPC->tempBehavior!=BS_WAIT&&e->NPC->tempBehavior!=BS_CINEMATIC)e->NPC->tempBehavior=BS_DEFAULT;
    e->NPC->timeOfDeath=0;
    e->client->ps.pm_flags&=~(PMF_TIME_KNOCKBACK|PMF_STUCK_TO_WALL);
    e->client->ps.pm_time=0;
    VectorClear(e->client->ps.velocity);
    VectorClear(e->client->ps.moveDir);
    G_JAR_EnforceActorStandin(e);
    e->nextthink=level.time+FRAMETIME/2;
    gi.linkentity(e);
    if(gi.JAR_Log)gi.JAR_Log(1,"randomizer","actor_recovery","version=1 entity=%d name=%s type=%s cause=%s icarus=%d tasks=retained time=%d",e->s.number,e->targetname,e->NPC_type?e->NPC_type:"",cause,e->m_iIcarusID,level.time);
}
}
void G_JAR_InitActorRecovery(){
    jarRecoverySuspended=false;
    gi.cvar("jar_actorRecovery","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    gi.cvar("jar_actorRecoverySaveVersion","0",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    if(g_eSavedGameJustLoaded!=eFULL)gi.cvar_set("jar_actorRecoverySaveVersion","0");
}
void G_JAR_RestoreActorRecovery(){
    bool state=false;
    for(int i=1;i<globals.num_entities;++i)if(g_entities[i].inuse&&(unsigned(g_entities[i].flags)&(jarRecoveryEnrolled|jarRecoveryStandin))){
        state=true;if(G_JAR_IsActorStandin(&g_entities[i]))G_JAR_EnforceActorStandin(&g_entities[i]);
    }
    gi.cvar_set("jar_actorRecoverySaveVersion",state?"1":"0");
}
bool G_JAR_SuspendActorRecovery(bool suspended){bool old=jarRecoverySuspended;jarRecoverySuspended=suspended;return old;}
void G_JAR_EnrollActor(const gentity_t *spawner,gentity_t *actor){
    // Enrollment happens only at native spawn, never by guessing from a corpse
    // or missing-name lookup. Original map death hooks are authoritative.
    if(!JarRecoveryEnabled()||!spawner||!actor||!actor->client||!spawner->NPC_targetname||!spawner->NPC_targetname[0])return;
    if(spawner->NPC_target||spawner->target2||spawner->target4||spawner->closetarget||spawner->behaviorSet[BSET_DEATH]||spawner->behaviorSet[BSET_FFDEATH])return;
    if(actor->client->playerTeam!=TEAM_PLAYER||actor->client->NPC_class==CLASS_VEHICLE)return;
    actor->flags=unsigned(actor->flags)|jarRecoveryEnrolled;
    gi.cvar_set("jar_actorRecoverySaveVersion","1");
}
void G_JAR_ForgetActor(gentity_t *e){if(e)e->flags=unsigned(e->flags)&~(jarRecoveryEnrolled|jarRecoveryStandin);}
bool G_JAR_IsActorStandin(const gentity_t *e){return e&&e->inuse&&e->NPC&&e->client&&(unsigned(e->flags)&jarRecoveryStandin);}
void G_JAR_EnforceActorStandin(gentity_t *e){
    if(!G_JAR_IsActorStandin(e))return;
    e->health=e->client->ps.stats[STAT_HEALTH]=1;
    e->client->ps.pm_type=PM_NORMAL;
    e->flags|=FL_NOTARGET|FL_GODMODE|FL_NO_KNOCKBACK;
    e->s.eFlags|=EF_NODRAW;
    e->client->ps.eFlags|=EF_NODRAW;
    e->s.eType=ET_PLAYER;
    e->contents=0;
    e->clipmask=MASK_NPCSOLID&~CONTENTS_BODY;
    e->takedamage=qfalse;
    e->e_TouchFunc=touchF_NULL;
    e->e_ThinkFunc=thinkF_NPC_Think;
    e->e_DieFunc=dieF_player_die;
    e->s.weapon=e->client->ps.weapon=WP_NONE;
    e->client->ps.forcePowersActive=0;
    e->s.loopSound=0;
}
bool G_JAR_RemoveToStandin(gentity_t *e){
    if(!jarRecoverySuspended&&G_JAR_IsActorStandin(e))return true;
    if(!JarRecoveryEligible(e)||e->health<=0)return false;
    JarRecoveryConvert(e,"unplanned-removal");return true;
}
JarActorRecoveryDeath::JarActorRecoveryDeath(gentity_t *e){
    if(!JarRecoveryEligible(e)||e->health>0)return;
    actor=e;
    behavior=e->NPC->behaviorState;defaultBehavior=e->NPC->defaultBehavior;tempBehavior=e->NPC->tempBehavior;
    VectorCopy(e->mins,mins);VectorCopy(e->maxs,maxs);
    memcpy(tasks,e->taskID,sizeof(tasks));
    torso=e->client->ps.torsoAnim;legs=e->client->ps.legsAnim;
    torsoTime=e->client->ps.torsoAnimTimer;legsTime=e->client->ps.legsAnimTimer;
    yaw=e->NPC->desiredYaw;pitch=e->NPC->desiredPitch;friction=e->client->ps.friction;
    // Native death overrides animations and zeros timers, which would otherwise
    // complete the *old* ICARUS tasks. Leave voice IDs intact during death audio.
    e->taskID[TID_ANIM_UPPER]=e->taskID[TID_ANIM_LOWER]=e->taskID[TID_ANIM_BOTH]=-1;
}
JarActorRecoveryDeath::~JarActorRecoveryDeath(){
    if(!actor)return;
    auto e=actor;
    if(!e->inuse||!e->NPC||!e->client)return;
    NPC_SetAnim(e,SETANIM_TORSO,torso,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD);
    NPC_SetAnim(e,SETANIM_LEGS,legs,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD);
    e->client->ps.torsoAnimTimer=torsoTime;e->client->ps.legsAnimTimer=legsTime;
    e->NPC->desiredYaw=yaw;e->NPC->desiredPitch=pitch;e->client->ps.friction=friction;
    VectorCopy(mins,e->mins);VectorCopy(maxs,e->maxs);
    memcpy(e->taskID,tasks,sizeof(tasks));
    e->NPC->behaviorState=bState_t(behavior);e->NPC->defaultBehavior=bState_t(defaultBehavior);e->NPC->tempBehavior=bState_t(tempBehavior);
    JarRecoveryConvert(e,"unplanned-death");
}
