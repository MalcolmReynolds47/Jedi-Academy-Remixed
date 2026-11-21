#include "game/g_local.h"
#include "game/g_randomizer.h"
#include "game/g_vehicles.h"
#include "game/w_local.h"
#include "test_support.h"
#include <map>
#include <set>
game_import_t gi={};level_locals_t level={};gentity_t g_entities[MAX_GENTITIES];
gitem_t bg_itemlist[64]={};extern const int bg_numItems=64;
weaponData_t weaponData[WP_NUM_WEAPONS]={};vehWeaponInfo_t g_vehWeaponInfo[MAX_VEH_WEAPONS]={};
static cvar_t skill={};cvar_t *g_spskill=&skill;
vec3_t forwardVec={1,0,0},muzzle={0,0,64};
int g_rocketLockEntNum=42,g_rocketLockTime=43,g_rocketSlackTime=44;
static int traceTarget=ENTITYNUM_NONE;static bool sameTeam=false;
void WP_RocketLock(gentity_t *e,float){e->client->rocketLockIndex=traceTarget;e->client->rocketLockTime=level.time;}
qboolean OnSameTeam(gentity_t *,gentity_t *){return sameTeam?qtrue:qfalse;}
static std::string lastChoice;
static bool enabled=true;static int emitted;static std::set<int> resources;
IGhoul2InfoArray &_TheGhoul2InfoArray(){throw std::runtime_error("unexpected Ghoul2 allocation");}
struct V{std::string value;cvar_t cvar={};};static std::map<std::string,V> vars;
static cvar_t *Get(const char *n,const char *v,int flags){auto r=vars.emplace(n,V{});auto& out=r.first->second;if(r.second)out.value=v;out.cvar.string=const_cast<char*>(out.value.c_str());out.cvar.flags|=flags;return &out.cvar;}
static void Set(const char *n,const char *v){vars[n].value=v;}
void RegisterItem(gitem_t *item){resources.insert(item->giTag);}
bool G_JAR_ProjectilesEnabled(){return enabled;}
void G_JAR_GetVehicleShots(std::vector<int>& out){out={1};g_vehWeaponInfo[1].name=const_cast<char*>("fixture-laser");}
void G_JAR_RefreshVehicleResources(){}
static void Fired(gentity_t *e){++emitted;CHECK(e->client->ps.weapon==e->s.weapon);CHECK(e->client->ps.weaponChargeTime<=level.time);CHECK(level.time-e->client->ps.weaponChargeTime<=1550);e->fxID=999;e->client->rocketLockTime=999;}
#define TWO(n) void n(gentity_t *e,qboolean){Fired(e);}
#define ONE(n) void n(gentity_t *e){Fired(e);}
TWO(WP_FireBryarPistol) TWO(WP_FireBlaster) TWO(WP_FireBowcaster) TWO(WP_FireRepeater) TWO(WP_FireDEMP2) TWO(WP_FireFlechette) TWO(WP_FireRocket) TWO(WP_Concussion) TWO(WP_FireDisruptor) TWO(WP_PlaceLaserTrap) TWO(WP_FireDetPack) TWO(WP_FireScepter)
ONE(WP_ATSTMainFire) ONE(WP_ATSTSideAltFire) ONE(WP_ATSTSideFire) ONE(WP_EmplacedFire) ONE(WP_BotLaser) ONE(WP_FireTuskenRifle)
gentity_t *WP_FireThermalDetonator(gentity_t *e,qboolean){Fired(e);return nullptr;}
void WP_FireVehicleWeapon(gentity_t *e,vec3_t,vec3_t,vehWeaponInfo_t *v){CHECK(v==&g_vehWeaponInfo[1]);Fired(e);}
void WP_TraceSetStart(const gentity_t *,vec3_t,const vec3_t,const vec3_t){}
static gentity_t missile;
gentity_t *CreateMissile(vec3_t,vec3_t,float vel,int life,gentity_t *e,qboolean){CHECK(vel>0&&life==10000);Fired(e);return &missile;}
#include "../../code/game/g_randomizer_projectiles.inc"
int main(){return RunTests("all shot emitters: full cycles, scope restoration, identical pools across held guns, native control exclusions",[]{
    gi.JAR_Log=[](int,const char *,const char *event,const char *fmt,...) {
        if(strcmp(event,"projectile"))return;
        char log[1024];va_list args;va_start(args,fmt);vsnprintf(log,sizeof(log),fmt,args);va_end(args);
        const char *chosen=strstr(log,"chosen=");CHECK(chosen);chosen+=7;lastChoice=std::string(chosen,strchr(chosen,' '));
    };
    Set("jar_randomProjectiles","1");Set("jar_randomMode","1");Set("jar_randomSeed","73");
    gi.cvar=Get;gi.cvar_set=Set;gi.FS_ReadFile=[](const char *,void **)->long{return 100;};level.time=10000;
    for(int weapon=1;weapon<WP_NUM_WEAPONS;++weapon){auto& i=bg_itemlist[weapon];i.classname=const_cast<char*>("weapon-fixture");i.giType=IT_WEAPON;i.giTag=weapon;Q_strncpyz(weaponData[weapon].weaponMdl,"fixture.md3",MAX_QPATH);}
    G_JAR_InitShotCatalog(true);G_JAR_FinishShotCatalog();CHECK(jarShots.size()>60);CHECK(resources.count(WP_JAWA));
    std::set<std::string> names;for(const auto& shot:jarShots)CHECK(names.insert(shot.name).second);
    gentity_t player={};gclient_t client={};player.client=&client;client.ps.ammo[AMMO_BLASTER]=100;client.ps.weaponChargeTime=987;client.rocketLockIndex=13;client.rocketLockTime=14;client.rocketLastValidTime=15;client.rocketTargetTime=16;player.fxID=17;
    for(int held:{WP_BLASTER,WP_REPEATER,WP_DEMP2,WP_CONCUSSION,WP_DISRUPTOR}){
        Set("jar_worldSeed","73");Set("jar_projectileSequence","0");player.s.weapon=client.ps.weapon=held;std::set<std::string> reached;
        for(unsigned i=0;i<jarShots.size();++i){auto expected=jar::CatalogIndex(73,i,unsigned(jarShots.size()),"all-projectiles-v2");reached.insert(jarShots[expected].name);const int before=emitted;
            CHECK(G_JAR_FireRandomProjectile(&player,i%2?qtrue:qfalse));CHECK(lastChoice==jarShots[expected].name);CHECK(emitted==before+1);
            CHECK(player.s.weapon==held&&client.ps.weapon==held&&client.ps.weaponChargeTime==987&&player.fxID==17&&client.ps.ammo[AMMO_BLASTER]==100);
            CHECK(client.rocketLockIndex==13&&client.rocketLockTime==14&&client.rocketLastValidTime==15&&client.rocketTargetTime==16&&g_rocketLockEntNum==42&&g_rocketLockTime==43&&g_rocketSlackTime==44);
        }CHECK(reached==names);
    }
    Set("jar_randomMode","0");Set("jar_projectileSequence","999");
    auto fire=[&](int held,bool alt){player.s.weapon=client.ps.weapon=held;CHECK(G_JAR_FireRandomProjectile(&player,alt?qtrue:qfalse));return lastChoice;};
    std::map<std::pair<int,bool>,std::string> fixed;
    for(int held:{WP_BLASTER,WP_REPEATER,WP_DEMP2,WP_CONCUSSION,WP_DISRUPTOR})for(bool alt:{false,true})fixed[{held,alt}]=fire(held,alt);
    std::reverse(jarShots.begin(),jarShots.end());
    for(int repeat=0;repeat<20;++repeat)for(auto it=fixed.rbegin();it!=fixed.rend();++it)CHECK(fire(it->first.first,it->first.second)==it->second);
    CHECK(vars["jar_projectileSequence"].value=="999");
    auto savedVars=vars;Set("jar_randomMode","1");fire(WP_BLASTER,false);vars=savedVars;
    G_JAR_InitShotCatalog(true);G_JAR_FinishShotCatalog(); // full save/map catalog rebuild
    for(const auto& p:fixed)CHECK(fire(p.first.first,p.first.second)==p.second);
    std::set<std::string> seeded;
    for(int seed=0;seed<100;++seed){Set("jar_randomSeed",std::to_string(seed).c_str());seeded.insert(fire(WP_BLASTER,false));}CHECK(seeded.size()>20);
    Set("jar_randomSeed","73");CHECK(fire(WP_BLASTER,false)==fixed.at({WP_BLASTER,false}));
    const auto savedShots=jarShots;const auto chosen=lastChoice;
    jarShots.erase(std::remove_if(jarShots.begin(),jarShots.end(),[&](const JarShot& s){return s.name==chosen;}),jarShots.end());
    CHECK(fire(WP_BLASTER,false)!=chosen);jarShots=savedShots;CHECK(fire(WP_BLASTER,false)==chosen);
    Set("jar_randomMode","bad");CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));Set("jar_randomMode","0");
    player.s.weapon=WP_DET_PACK;CHECK(!G_JAR_FireRandomProjectile(&player,qtrue));CHECK(G_JAR_FireRandomProjectile(&player,qfalse));
    for(int held:{WP_SABER,WP_MELEE,WP_STUN_BATON,WP_TUSKEN_STAFF,WP_ATST_MAIN,WP_EMPLACED_GUN}){player.s.weapon=held;CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));CHECK(!G_JAR_FireRandomProjectile(&player,qtrue));}
    player.s.weapon=WP_BLASTER;client.ps.eFlags|=EF_LOCKED_TO_WEAPON;CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));client.ps.eFlags=0;player.s.m_iVehicleNum=5;CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));player.s.m_iVehicleNum=0;player.s.number=2;CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));player.s.number=0;
    Set("jar_randomMode","1");Set("jar_projectileSequenceSeed","off-seed-sentinel");
    const int beforeDisabled=emitted;const auto beforeSequence=vars["jar_projectileSequence"].value;
    for(const char *disabled:{"0","bad","2"}) {
        Set("jar_randomProjectiles",disabled);CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));
        CHECK(!G_JAR_FireRandomProjectile(&player,qtrue));
        CHECK(vars["jar_projectileSequenceSeed"].value=="off-seed-sentinel");
        CHECK(emitted==beforeDisabled && vars["jar_projectileSequence"].value==beforeSequence);
    }
    Set("jar_randomProjectiles","1");CHECK(G_JAR_FireRandomProjectile(&player,qfalse));
    enabled=false;CHECK(!G_JAR_FireRandomProjectile(&player,qfalse));
    resources.clear();enabled=true;G_JAR_RefreshShotResources();CHECK(resources.count(WP_JAWA)&&resources.count(WP_THERMAL)&&resources.count(WP_ATST_SIDE));
    JarShot guided={"guided-fixture",WP_ROCKET_LAUNCHER,true,0,JarNative,0,0,0,true};gclient_t enemyClient={};
    enemyClient.playerTeam=TEAM_ENEMY;g_entities[9].inuse=qtrue;g_entities[9].client=&enemyClient;g_entities[9].health=100;traceTarget=9;
    {JarShotScope scope(&player,guided);CHECK(JarAcquireShotTarget(&player,guided));CHECK(client.rocketLockIndex==9&&level.time-client.rocketLockTime>=1200);}
    CHECK(client.rocketLockIndex==13&&g_rocketLockEntNum==42);
    for(int invalid=0;invalid<4;++invalid){sameTeam=invalid==0;g_entities[9].health=invalid==1?0:100;g_entities[9].inuse=invalid==2?qfalse:qtrue;traceTarget=invalid==3?ENTITYNUM_WORLD:9;
        JarShotScope scope(&player,guided);CHECK(!JarAcquireShotTarget(&player,guided));CHECK(client.rocketLockIndex==ENTITYNUM_NONE);}
    for(int time:{0,600,1200,10000}){level.time=time;traceTarget=9;sameTeam=false;g_entities[9].inuse=qtrue;g_entities[9].health=100;
        {JarShotScope scope(&player,guided);CHECK(JarAcquireShotTarget(&player,guided));CHECK(G_JAR_InstantProjectileLock(&player,9));CHECK(!G_JAR_InstantProjectileLock(&player,8));}
        CHECK(!G_JAR_InstantProjectileLock(&player,9));}
    sameTeam=false;traceTarget=9;enemyClient.playerTeam=TEAM_NEUTRAL;{JarShotScope scope(&player,guided);CHECK(!JarAcquireShotTarget(&player,guided));}
    enemyClient.playerTeam=TEAM_ENEMY;g_entities[9].inuse=qtrue;guided.emitter=JarVehicle;guided.vehicle=1;g_vehWeaponInfo[1].iLockOnTime=2400;
    {JarShotScope scope(&player,guided);CHECK(JarAcquireShotTarget(&player,guided));CHECK(level.time-client.rocketLockTime>=2400);}

});}
