// Full per-shot catalog. The held weapon controls cadence/ammo; each event draws
// from the same complete emitter+mode pool, independently of that held weapon.
#include "qcommon/jar_audio_catalog.h"
#include <algorithm>
namespace {
enum JarEmitter {JarNative,JarBolt,JarMarkBow,JarNoghri,JarTurbo,JarVehicle};
struct JarShot {std::string name;int weapon;bool alt=false;int charge=0;JarEmitter emitter=JarNative;int speed=0,damage=0,vehicle=0;bool acquireTarget=false;};
std::vector<JarShot> jarShots;
const gentity_t *jarInstantActor=nullptr;int jarInstantTarget=ENTITYNUM_NONE;
int JarShotVisual(const JarShot& s){if(s.emitter==JarVehicle)return WP_BLASTER;if(s.weapon==WP_BOT_LASER)return WP_BRYAR_PISTOL;if(s.weapon==WP_TIE_FIGHTER&&s.emitter!=JarTurbo)return WP_EMPLACED_GUN;if(s.weapon==WP_RAPID_FIRE_CONC)return s.alt?WP_REPEATER:WP_EMPLACED_GUN;if(s.weapon==WP_SCEPTER)return WP_DISRUPTOR;return s.weapon;}
bool JarRegisterShot(const JarShot& s){const int weapon=JarShotVisual(s);
    if(!weaponData[weapon].weaponMdl[0]||gi.FS_ReadFile(weaponData[weapon].weaponMdl,nullptr)<=0)return false;
    for(int i=1;i<bg_numItems&&bg_itemlist[i].classname;++i)if(bg_itemlist[i].giType==IT_WEAPON&&bg_itemlist[i].giTag==weapon){RegisterItem(&bg_itemlist[i]);return true;}
    return false;
}
void JarAddShot(const JarShot& s){if(JarRegisterShot(s)){jarShots.push_back(s);if(gi.JAR_Log)gi.JAR_Log(2,"randomizer","projectile_catalog_entry","name=%s weapon=%d alt=%d charge_ms=%d emitter=%d status=available",s.name.c_str(),s.weapon,s.alt,s.charge,int(s.emitter));}
    else if(gi.JAR_Log)gi.JAR_Log(1,"randomizer","projectile_catalog_entry","name=%s status=unsupported reason=missing-item-or-render-model",s.name.c_str());}
bool JarShotSource(const gentity_t *e,qboolean alt){
    if(!e||!e->client||e->s.number!=0||e->NPC||e->s.m_iVehicleNum||e->client->NPC_class==CLASS_VEHICLE||(e->client->ps.eFlags&(EF_LOCKED_TO_WEAPON|EF_IN_ATST)))return false;
    switch(e->s.weapon){case WP_SABER:case WP_NONE:case WP_MELEE:case WP_STUN_BATON:case WP_TUSKEN_STAFF:case WP_EMPLACED_GUN:case WP_ATST_MAIN:case WP_ATST_SIDE:return false;
    case WP_DET_PACK:return !alt;case WP_TUSKEN_RIFLE:return alt!=qfalse;case WP_NOGHRI_STICK:return !alt;default:return true;}
}
const char *JarShotSourceName(int weapon) {
    // Textual identities survive enum/catalog reordering. Input charge duration
    // does not reroll a held weapon's primary/alternate replacement.
    switch(weapon) {
#define JAR_SHOT_KEY(w) case w: return #w;
    JAR_SHOT_KEY(WP_BLASTER_PISTOL) JAR_SHOT_KEY(WP_BRYAR_PISTOL) JAR_SHOT_KEY(WP_BLASTER)
    JAR_SHOT_KEY(WP_BOWCASTER) JAR_SHOT_KEY(WP_REPEATER) JAR_SHOT_KEY(WP_DEMP2)
    JAR_SHOT_KEY(WP_FLECHETTE) JAR_SHOT_KEY(WP_ROCKET_LAUNCHER) JAR_SHOT_KEY(WP_CONCUSSION)
    JAR_SHOT_KEY(WP_DISRUPTOR) JAR_SHOT_KEY(WP_THERMAL) JAR_SHOT_KEY(WP_TRIP_MINE)
    JAR_SHOT_KEY(WP_DET_PACK) JAR_SHOT_KEY(WP_BOT_LASER) JAR_SHOT_KEY(WP_JAWA)
    JAR_SHOT_KEY(WP_TUSKEN_RIFLE) JAR_SHOT_KEY(WP_NOGHRI_STICK) JAR_SHOT_KEY(WP_SCEPTER)
    JAR_SHOT_KEY(WP_TIE_FIGHTER) JAR_SHOT_KEY(WP_RAPID_FIRE_CONC)
#undef JAR_SHOT_KEY
    default:return nullptr;
    }
}
void JarPayload(gentity_t *ent,const JarShot& s){
    vec3_t start,dir;VectorCopy(muzzle,start);VectorCopy(forwardVec,dir);WP_TraceSetStart(ent,start,vec3_origin,vec3_origin);
    auto missile=CreateMissile(start,dir,s.speed,10000,ent,qfalse);if(!missile)return;
    missile->classname="jar_npc_projectile";missile->s.weapon=s.weapon;missile->damage=s.damage;missile->dflags=DAMAGE_DEATH_KNOCKBACK;missile->methodOfDeath=MOD_ENERGY;missile->clipmask=MASK_SHOT|CONTENTS_LIGHTSABER;
    if(s.emitter==JarNoghri){missile->classname="noghri_proj";missile->dflags=DAMAGE_NO_KNOCKBACK;missile->methodOfDeath=MOD_BLASTER;missile->splashRadius=100;missile->splashMethodOfDeath=MOD_GAS;}
    if(s.emitter==JarMarkBow){VectorSet(missile->maxs,BOWCASTER_SIZE,BOWCASTER_SIZE,BOWCASTER_SIZE);VectorScale(missile->maxs,-1,missile->mins);missile->splashDamage=BOWCASTER_SPLASH_DAMAGE;missile->splashRadius=BOWCASTER_SPLASH_RADIUS;missile->bounceCount=0;}
    if(s.emitter==JarTurbo){missile->classname="turbo_proj";missile->methodOfDeath=missile->splashMethodOfDeath=MOD_EMPLACED;missile->clipmask=MASK_SHOT;missile->bounceCount=8;}
}
struct JarShotScope {
    const gentity_t *instantActor;int instantTarget;gentity_t *e;int weapon,psWeapon,charge,fx,lock,lockTime,last,target,globalLock,globalTime,globalSlack;qboolean alt;
    JarShotScope(gentity_t *actor,const JarShot& s):instantActor(jarInstantActor),instantTarget(jarInstantTarget),e(actor),weapon(e->s.weapon),psWeapon(e->client->ps.weapon),charge(e->client->ps.weaponChargeTime),fx(e->fxID),lock(e->client->rocketLockIndex),lockTime(e->client->rocketLockTime),last(e->client->rocketLastValidTime),target(e->client->rocketTargetTime),globalLock(g_rocketLockEntNum),globalTime(g_rocketLockTime),globalSlack(g_rocketSlackTime),alt(e->alt_fire){
        jarInstantActor=nullptr;jarInstantTarget=ENTITYNUM_NONE;
        e->s.weapon=e->client->ps.weapon=s.weapon;e->client->ps.weaponChargeTime=level.time-s.charge;e->alt_fire=s.alt?qtrue:qfalse;
        // Never consume unrelated held-gun lock state. Guided variants acquire a
        // validated crosshair target below, and this scope restores all state.
        e->client->rocketLockIndex=ENTITYNUM_NONE;e->client->rocketLockTime=e->client->rocketLastValidTime=e->client->rocketTargetTime=0;
        g_rocketLockEntNum=ENTITYNUM_NONE;g_rocketLockTime=g_rocketSlackTime=0;
    }
    ~JarShotScope(){jarInstantActor=instantActor;jarInstantTarget=instantTarget;e->s.weapon=weapon;e->client->ps.weapon=psWeapon;e->client->ps.weaponChargeTime=charge;e->fxID=fx;e->alt_fire=alt;e->client->rocketLockIndex=lock;e->client->rocketLockTime=lockTime;e->client->rocketLastValidTime=last;e->client->rocketTargetTime=target;g_rocketLockEntNum=globalLock;g_rocketLockTime=globalTime;g_rocketSlackTime=globalSlack;}
};
bool JarAcquireShotTarget(gentity_t *ent,const JarShot& shot){
    if(!shot.acquireTarget)return false;
    WP_RocketLock(ent,16384);
    const int index=ent->client->rocketLockIndex;
    if(index<=0||index>=ENTITYNUM_WORLD||!g_entities[index].inuse||!g_entities[index].client||g_entities[index].health<=0||g_entities[index].client->playerTeam==TEAM_FREE||g_entities[index].client->playerTeam==TEAM_NEUTRAL||OnSameTeam(ent,&g_entities[index])){
        ent->client->rocketLockIndex=ENTITYNUM_NONE;return false;
    }
    // Random shots have no pre-fire charge phase. Complete acquisition only for
    // the native trace's live hostile target; never borrow a stale lock.
    const int duration=shot.emitter==JarVehicle?std::max(1200,g_vehWeaponInfo[shot.vehicle].iLockOnTime):1200;
    int start=level.time-duration;if(start==-1)--start;
    ent->client->rocketLockTime=ent->client->rocketLastValidTime=start;
    ent->client->rocketTargetTime=level.time+500;
    g_rocketLockEntNum=index;g_rocketLockTime=std::max(1,start);
    jarInstantActor=ent;jarInstantTarget=index;
    return true;
}
}
bool G_JAR_InstantProjectileLock(const gentity_t *actor,int target){return actor==jarInstantActor&&target==jarInstantTarget&&target>0&&target<ENTITYNUM_WORLD;}
void G_JAR_InitShotCatalog(bool enabled){
    jarShots.clear();gi.cvar("jar_projectileSequence","0",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    if(!enabled)return;
    auto add=[](const char *name,int weapon,bool alt=false,int charge=0){JarAddShot({name,weapon,alt,charge});};
    for(int weapon:{WP_BLASTER_PISTOL,WP_BRYAR_PISTOL}){const std::string prefix=weapon==WP_BLASTER_PISTOL?"blaster-pistol":"bryar";add((prefix+"-primary").c_str(),weapon);
        for(int tier=1;tier<=5;++tier)add((prefix+"-charged-"+std::to_string(tier)).c_str(),weapon,true,int(tier*BRYAR_CHARGE_UNIT));}
    add("blaster-primary",WP_BLASTER);add("blaster-alt",WP_BLASTER,true);
    for(int tier:{1,3,5})add(("bowcaster-volley-"+std::to_string(tier)).c_str(),WP_BOWCASTER,false,int(tier*BOWCASTER_CHARGE_UNIT));add("bowcaster-bounce",WP_BOWCASTER,true);
    add("repeater-bolt",WP_REPEATER);add("repeater-grenade",WP_REPEATER,true);
    add("demp-bolt",WP_DEMP2);for(int tier=1;tier<=3;++tier)add(("demp-ion-"+std::to_string(tier)).c_str(),WP_DEMP2,true,int(tier*DEMP2_CHARGE_UNIT));
    add("flechette-scatter",WP_FLECHETTE);add("flechette-bouncing-mines",WP_FLECHETTE,true);
    add("rocket-primary",WP_ROCKET_LAUNCHER);add("rocket-alt-without-stale-lock",WP_ROCKET_LAUNCHER,true);JarAddShot({"rocket-guided-crosshair",WP_ROCKET_LAUNCHER,true,0,JarNative,0,0,0,true});
    add("concussion-projectile",WP_CONCUSSION);add("concussion-beam",WP_CONCUSSION,true);
    add("disruptor-beam",WP_DISRUPTOR);for(int tier=1;tier<=10;++tier)add(("disruptor-charged-beam-"+std::to_string(tier)).c_str(),WP_DISRUPTOR,true,50+int(tier*DISRUPTOR_CHARGE_UNIT));
    for(bool alt:{false,true})for(int charge:{0,450,900})add((std::string(alt?"thermal-impact-":"thermal-timed-")+std::to_string(charge)).c_str(),WP_THERMAL,alt,charge);
    add("tripmine-beam",WP_TRIP_MINE);add("tripmine-proximity",WP_TRIP_MINE,true);add("detpack-throw",WP_DET_PACK);
    add("atst-main",WP_ATST_MAIN);add("atst-side",WP_ATST_SIDE);add("atst-rocket",WP_ATST_SIDE,true);
    add("emplaced-bolt",WP_EMPLACED_GUN);add("bot-laser",WP_BOT_LASER);add("jawa-ion-bolt",WP_JAWA);add("tusken-rifle",WP_TUSKEN_RIFLE,true);add("scepter-beam",WP_SCEPTER);
    JarAddShot({"remote-bolt",WP_BRYAR_PISTOL,false,0,JarBolt,1000,10});JarAddShot({"seeker-bolt",WP_BLASTER,false,0,JarBolt,1000,5});
    JarAddShot({"mark1-mark2-bolt",WP_BRYAR_PISTOL,false,0,JarBolt,1600,1});
    JarAddShot({"probe-bolt",WP_BRYAR_PISTOL,false,0,JarBolt,1600,g_spskill->integer<2?5:10});
    JarAddShot({"sentry-bolt",WP_BRYAR_PISTOL,false,0,JarBolt,1600,g_spskill->integer==0?1:g_spskill->integer==1?3:5});
    JarAddShot({"mark1-explosive-bow",WP_BOWCASTER,false,0,JarMarkBow,BOWCASTER_VELOCITY,50});
    JarAddShot({"noghri-gas-stick",WP_NOGHRI_STICK,false,0,JarNoghri,1200,g_spskill->integer==0?1:g_spskill->integer==1?5:10});
    JarAddShot({"turbolaser-default",WP_TIE_FIGHTER,false,0,JarTurbo,20000,20});
}
void G_JAR_FinishShotCatalog(){
    if(!G_JAR_ProjectilesEnabled())return;
    // Append after native map precaching, retaining legacy vehicle weapon indices.
    std::vector<int> vehicle;G_JAR_GetVehicleShots(vehicle);for(int index:vehicle){const auto& v=g_vehWeaponInfo[index];JarAddShot({std::string("vehicle-")+v.name,WP_BLASTER,false,0,JarVehicle,0,0,index,v.fHoming!=0});}
    if(gi.JAR_Log)gi.JAR_Log(0,"randomizer","projectile_catalog","available=%u sequence=full-cycle-permutation held_weapon_filter=none vehicles=%u hitscan=included controls=preserved aliases=tie-and-rapid-primary-use-emplaced,rapid-alt-use-repeater melee=not-projectiles",unsigned(jarShots.size()),unsigned(vehicle.size()));
}
void G_JAR_RefreshShotResources(){
    if(!G_JAR_ProjectilesEnabled())return;
    for(const auto& s:jarShots)JarRegisterShot(s);
    G_JAR_RefreshVehicleResources();
}
static bool G_JAR_FireRandomProjectile(gentity_t *ent,qboolean alt){
    // World readiness retains precached resources/save semantics. The current
    // toggle can stop future random shots immediately without touching missiles
    // already in flight or enabling an unqualified world's catalog.
    if(!G_JAR_ProjectilesEnabled() || strcmp(gi.cvar("jar_randomProjectiles","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,"1") || !JarShotSource(ent,alt)||jarShots.empty())return false;
    bool fixed; uint32_t seed=1,sequence=0;
    if(!jar::ParseRandomMode(gi.cvar("jar_randomMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,fixed) ||
       !jar::ParsePickupSeed(gi.cvar("jar_randomSeed","1",0)->string,seed))return false;
    const char *held=JarShotSourceName(ent->s.weapon);if(!held)return false;
    const std::string source=std::string(held)+(alt?"/alternate":"/primary");
    jar::ParsePickupSeed(gi.cvar("jar_projectileSequence","0",0)->string,sequence);
    if(!fixed) {
        if(strcmp(gi.cvar("jar_projectileSequenceSeed","",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART)->string,std::to_string(seed).c_str())) {
            gi.cvar_set("jar_projectileSequenceSeed",std::to_string(seed).c_str());sequence=0;
        }
    }
    const unsigned index=fixed?jar::FixedCatalogIndex(seed,"projectiles-v1",source.c_str(),unsigned(jarShots.size()),
        [&](unsigned i){return jarShots[i].name.c_str();}):jar::CatalogIndex(seed,sequence,unsigned(jarShots.size()),"all-projectiles-v2");
    const auto& s=jarShots[index];
    if(!fixed){char next[16];Com_sprintf(next,sizeof(next),"%u",(sequence+1)&2147483647u);gi.cvar_set("jar_projectileSequence",next);}
    if(gi.JAR_Log)gi.JAR_Log(2,"randomizer","projectile","seed=%u sequence=%u held=%d input_alt=%d chosen=%s weapon=%d alt=%d charge_ms=%d candidates=%u mode=%s source=%s",seed,sequence,ent->s.weapon,alt,s.name.c_str(),s.weapon,s.alt,s.charge,unsigned(jarShots.size()),fixed?"fixed":"reroll",source.c_str());
    JarShotScope scope(ent,s);const bool acquired=JarAcquireShotTarget(ent,s);const qboolean mode=s.alt?qtrue:qfalse;
    if(s.acquireTarget&&gi.JAR_Log)gi.JAR_Log(2,"randomizer","projectile_guidance","chosen=%s acquired=%d target=%d adapter=instant-validated-crosshair",s.name.c_str(),acquired,ent->client->rocketLockIndex);
    if(s.emitter==JarVehicle){vec3_t start,dir;VectorCopy(muzzle,start);VectorCopy(forwardVec,dir);WP_FireVehicleWeapon(ent,start,dir,&g_vehWeaponInfo[s.vehicle]);return true;}
    if(s.emitter!=JarNative){JarPayload(ent,s);return true;}
    switch(s.weapon){
    case WP_BLASTER_PISTOL:case WP_BRYAR_PISTOL:case WP_JAWA:WP_FireBryarPistol(ent,mode);break;
    case WP_BLASTER:WP_FireBlaster(ent,mode);break;case WP_BOWCASTER:WP_FireBowcaster(ent,mode);break;
    case WP_REPEATER:WP_FireRepeater(ent,mode);break;case WP_DEMP2:WP_FireDEMP2(ent,mode);break;
    case WP_FLECHETTE:WP_FireFlechette(ent,mode);break;case WP_ROCKET_LAUNCHER:WP_FireRocket(ent,mode);break;
    case WP_CONCUSSION:WP_Concussion(ent,mode);break;case WP_DISRUPTOR:WP_FireDisruptor(ent,mode);break;
    case WP_THERMAL:WP_FireThermalDetonator(ent,mode);break;case WP_TRIP_MINE:WP_PlaceLaserTrap(ent,mode);break;case WP_DET_PACK:WP_FireDetPack(ent,qfalse);break;
    case WP_ATST_MAIN:WP_ATSTMainFire(ent);break;case WP_ATST_SIDE:if(mode)WP_ATSTSideAltFire(ent);else WP_ATSTSideFire(ent);break;
    case WP_EMPLACED_GUN:WP_EmplacedFire(ent);break;case WP_BOT_LASER:WP_BotLaser(ent);break;case WP_TUSKEN_RIFLE:WP_FireTuskenRifle(ent);break;case WP_SCEPTER:WP_FireScepter(ent,mode);break;
    default:break;}
    return true;
}
