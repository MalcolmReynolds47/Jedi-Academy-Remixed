// Exercise the production map pickup policy with headless game imports.
#include "game/g_local.h"
#include "game/g_randomizer.h"
#include "game/g_savegame_fields.h"
#include "game/g_npc_behavior_state.h"
#include "qcommon/jar_randomizer.h"
#include "test_support.h"
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <limits>

static bool sandboxOwned=false;
bool G_JAR_WorkshopOwns(const gentity_t*){return sandboxOwned;}
game_import_t gi = {};
gentity_t g_entities[MAX_GENTITIES];
game_export_t globals={};
void NPC_SetAnim(gentity_t *actor,int,int,int,int) {
    CHECK(actor->taskID[TID_ANIM_UPPER]==-1&&actor->taskID[TID_ANIM_LOWER]==-1&&actor->taskID[TID_ANIM_BOTH]==-1);
}
level_locals_t level = {};
SavedGameJustLoaded_e g_eSavedGameJustLoaded = eNO;
gitem_t bg_itemlist[64] = {};
extern const int bg_numItems = 64;
IGhoul2InfoArray &_TheGhoul2InfoArray() { throw std::runtime_error("unexpected model allocation"); }
struct Variable { std::string value; cvar_t cvar = {}; };
static std::map<std::string, Variable> vars;
static std::vector<std::unique_ptr<char[]>> strings;
static std::vector<std::pair<std::string, std::string>> fields;
static int allocations, warnings;
static bool logEnabled;
weaponData_t weaponData[WP_NUM_WEAPONS] = {};
ammoData_t ammoData[AMMO_MAX] = {};
static bool definitionsValid = true;
static std::set<int> registered;
void RegisterItem(gitem_t *item) { CHECK(item); registered.insert(item->giTag); }
void G_JAR_InitShotCatalog(bool) {}
int NPC_WeaponsForTeam(team_t,int,const char*){return 1u<<WP_SABER;}
void G_JAR_ReadNpcCatalog(std::vector<JarNpcTemplate>& out,bool) {
 out.clear();if(!definitionsValid)return;
 for(int i=0;i<24;++i){JarNpcTemplate e;e.name=i?"fixture"+std::to_string(i):"stormtrooper";e.model="stormtrooper";e.npcClass=CLASS_STORMTROOPER;e.team=TEAM_ENEMY;e.enemyTeam=TEAM_PLAYER;out.push_back(e);}
 JarNpcTemplate v;v.name="vehicle";v.npcClass=CLASS_VEHICLE;v.reason="vehicle-requires-dedicated-spawner";out.push_back(v);
 JarNpcTemplate giant=out.front();giant.name="giant";giant.width=96;giant.height=220;out.push_back(giant);
}


char *G_NewString(const char *text) {
    ++allocations;
    strings.emplace_back(new char[std::strlen(text) + 1]);
    std::strcpy(strings.back().get(), text);
    return strings.back().get();
}
qboolean G_SpawnField(unsigned i, char **key, char **value) {
    if (i >= fields.size()) return qfalse;
    *key = const_cast<char *>(fields[i].first.c_str());
    *value = const_cast<char *>(fields[i].second.c_str());
    return qtrue;
}
static cvar_t *Get(const char *name, const char *initial, int flags) {
    auto it = vars.emplace(name, Variable{});
    auto& var = it.first->second;
    if (it.second) var.value = initial;
    var.cvar.string = const_cast<char *>(var.value.c_str());
    var.cvar.flags |= flags;
    return &var.cvar;
}
static void Set(const char *name, const char *value) { vars[name].value = value; }
static void Init(const char *seed = "73", const char *weapons = "1", const char *ammo = "1", const char *supplies = "1", const char *mode = "1") {
    Set("jar_randomMode",mode);
    Set(jar::pickupCvars[0], seed); Set(jar::pickupCvars[1], weapons);
    Set(jar::pickupCvars[2], ammo); Set(jar::pickupCvars[3], supplies);
    G_JAR_InitPickupRandomizer();
}
static gitem_t& Add(int index, const char *name, itemType_t type, int tag, float bottom = 0) {
    auto& item = bg_itemlist[index];
    item.classname = const_cast<char *>(name);
    item.giType = type; item.giTag = tag; item.quantity = 25;
    item.world_model = const_cast<char *>("models/items/test.md3");
    item.mins[0] = item.mins[1] = -8; item.mins[2] = bottom;
    item.maxs[0] = item.maxs[1] = 8; item.maxs[2] = 16;
    return item;
}
static std::string Choose(const char *name, int ordinal, int entityId = 4) {
    gentity_t ent = {};
    ent.classname = const_cast<char *>(name); ent.s.number = entityId;
    ent.s.origin[0] = 1024; ent.s.origin[1] = 300; ent.s.origin[2] = 32;
    G_JAR_RandomizeMapPickup(&ent, ordinal);
    CHECK(ent.s.number == entityId && ent.s.origin[0] == 1024 && ent.s.origin[2] == 32);
    CHECK(!ent.item && !ent.nextthink); // normal spawn still owns initialization
    return ent.classname;
}

#include "../../code/game/g_randomizer.cpp"

int main() {
    return RunTests("seeded map pickups: determinism, independent toggles, metadata/size guards and save handling", [] {
        gi.cvar = Get; gi.cvar_set = Set;
        gi.Printf = [](const char *, ...) { ++warnings; };
        gi.JAR_Log = [](int, const char *category, const char *, const char *, ...) {
            if (logEnabled) CHECK(!std::strcmp(category, "randomizer"));
        };
        Q_strncpyz(level.mapname, "t1_fatal", sizeof(level.mapname));
        const char *weapons[] = {"weapon_blaster_pistol", "weapon_blaster", "weapon_bowcaster",
            "weapon_repeater", "weapon_flechette", "weapon_concussion_rifle"};
        const int tags[] = {WP_BLASTER_PISTOL, WP_BLASTER, WP_BOWCASTER, WP_REPEATER, WP_FLECHETTE, WP_CONCUSSION};
        for (int i = 0; i < 6; ++i) Add(i+1, weapons[i], IT_WEAPON, tags[i], -2);
        Add(7, "ammo_blaster", IT_AMMO, AMMO_BLASTER);
        Add(8, "ammo_metallic_bolts", IT_AMMO, AMMO_METAL_BOLTS);
        Add(9, "item_bacta", IT_HEALTH, ITM_MEDPAK_PICKUP);
        Add(10, "item_medpak_instant", IT_HEALTH, ITM_MEDPAK_PICKUP, -3);
        Add(11, "item_shield_sm_instant", IT_ARMOR, ITM_SHIELD_SM_PICKUP);
        Add(12, "item_shield_lrg_instant", IT_ARMOR, ITM_SHIELD_LRG_PICKUP, -4);

        CHECK(jar::PickupDecision(73, "t1_fatal", 10, "weapons", "weapon_blaster") == 1385700717u);
        uint32_t seed = 999;
        CHECK(jar::ParsePickupSeed("0", seed) && seed == 0);
        CHECK(jar::ParsePickupSeed("2147483647", seed) && seed == 2147483647u);
        for (const char *bad : {"", "-1", "+1", " 1", "1 ", "1.5", "1x", "2147483648", "99999999999999999999"}) {
            seed = 99; CHECK(!jar::ParsePickupSeed(bad, seed) && seed == 99);
        }
        CHECK(!jar::ParsePickupSeed(nullptr, seed));
        CHECK(jar::FixedDecision(73,"audio-v1","Sound/FOOT.wav")==jar::FixedDecision(73,"audio-v1","sound\\foot.wav"));
        bool fixed=false;CHECK(jar::ParseRandomMode("0",fixed)&&fixed);CHECK(jar::ParseRandomMode("1",fixed)&&!fixed);
        for(const char *bad:{"","2","-1","00"," 0"})CHECK(!jar::ParseRandomMode(bad,fixed));
        // Defaults leave gameplay untouched and persist through normal save cvars.
        vars.clear(); G_JAR_InitPickupRandomizer();
        CHECK(vars["jar_randomMode"].value=="0");
        CHECK(Choose("weapon_blaster", 10) == "weapon_blaster" && allocations == 0);
        for (const auto& entry : vars) if (entry.first != "jar_randomStatus") CHECK(entry.second.cvar.flags & CVAR_SAVEGAME);

        Init();
        std::vector<std::string> results;
        std::set<std::string> reached;
        for (int i = 1; i <= 256; ++i) { results.push_back(Choose("weapon_blaster", i)); reached.insert(results.back()); }
        CHECK(reached.size() == 6);
        // Interleaving categories, different entity IDs and logging do not reroll.
        for (int i = 256; i >= 1; --i) {
            Choose("ammo_blaster", i); Choose("item_medpak_instant", i); logEnabled = i % 2;
            CHECK(Choose("weapon_blaster", i, 12000) == results[i-1]);
        }
        Init("73", "1", "0", "0");
        for (int i = 1; i <= 256; ++i) {
            CHECK(Choose("weapon_blaster", i) == results[i-1]);
            CHECK(Choose("ammo_blaster", i) == "ammo_blaster");
            CHECK(Choose("item_medpak_instant", i) == "item_medpak_instant");
        }
        int different = 0;
        Init("74"); for (int i = 1; i <= 256; ++i) different += Choose("weapon_blaster", i) != results[i-1];
        CHECK(different > 128);
        Init(); Q_strncpyz(level.mapname, "t1_sour", sizeof(level.mapname)); different = 0;
        for (int i = 1; i <= 256; ++i) different += Choose("weapon_blaster", i) != results[i-1];
        CHECK(different > 128);
        Q_strncpyz(level.mapname, "t1_fatal", sizeof(level.mapname));
        // Mid-level preference edits apply at the next initialization only.
        Set("jar_randomSeed", "1000"); Set("jar_randomWeapons", "0");
        for (int i = 1; i <= 256; ++i) CHECK(Choose("weapon_blaster", i) == results[i-1]);
        G_JAR_InitPickupRandomizer(); CHECK(Choose("weapon_blaster", 1) == "weapon_blaster");
        Init();
        std::srand(321); const int expectedRandom = std::rand(); std::srand(321);
        Choose("weapon_blaster", 10); CHECK(std::rand() == expectedRandom);
        CHECK(Choose("weapon_blaster", 0) == "weapon_blaster"); // dynamic / console / RMG

        for (const char *special : {"weapon_saber", "weapon_demp2", "weapon_disruptor", "weapon_rocket_launcher",
            "weapon_thermal", "weapon_trip_mine", "weapon_det_pack", "weapon_bryar_pistol", "weapon_turret",
            "ammo_force", "ammo_powercell", "ammo_rockets", "ammo_thermal", "ammo_tripmine", "ammo_detpack",
            "item_security_key", "item_goodie_key", "item_battery", "item_datapad", "holocron_force_push", "NPC_stormtrooper"})
            for (int i = 1; i < 10; ++i) CHECK(Choose(special, i) == special);

        for (const char *key : {"target", "targetname", "target2", "script_targetname", "spawnscript", "usescript",
            "count", "wait", "random", "model", "team", "NPC_type", "spawnflags", "unknown_map_key"}) {
            fields = {{key, "1"}};
            for (int i = 1; i < 40; ++i) CHECK(Choose("weapon_blaster", i) == "weapon_blaster");
        }
        fields = {{"target", ""}}; CHECK(Choose("weapon_blaster", 9) == "weapon_blaster");
        fields = {{"CLASSNAME", "weapon_blaster"}, {"origin", "1024 300 32"}, {"spawnflags", "0"}};
        for (int i = 1; i <= 256; ++i) CHECK(Choose("weapon_blaster", i) == results[i-1]);
        fields.clear();
        for (int i = 0; i < NUM_BSETS; ++i) {
            gentity_t ent = {}; ent.classname = const_cast<char *>("weapon_blaster");
            ent.behaviorSet[i] = const_cast<char *>("test_script");
            G_JAR_RandomizeMapPickup(&ent, 12); CHECK(!std::strcmp(ent.classname, "weapon_blaster"));
        }

        // Candidate footprints never grow into surrounding map geometry.
        for (int i = 1; i < 256; ++i) {
            auto choice = Choose("item_shield_sm_instant", i);
            CHECK(choice == "item_shield_sm_instant" || choice == "item_bacta");
        }
        bg_itemlist[8].maxs[0] = 9;
        for (int i = 1; i < 40; ++i) CHECK(Choose("ammo_blaster", i) == "ammo_blaster");
        bg_itemlist[8].maxs[0] = 8; bg_itemlist[8].world_model = nullptr;
        CHECK(Choose("ammo_blaster", 2) == "ammo_blaster");
        bg_itemlist[8].world_model = const_cast<char *>("models/test.md3");
        bg_itemlist[8].giTag = AMMO_FORCE;
        CHECK(Choose("ammo_blaster", 2) == "ammo_blaster");
        bg_itemlist[8].giTag = AMMO_METAL_BOLTS;
        bg_itemlist[7].mins[0] = std::numeric_limits<float>::quiet_NaN();
        CHECK(Choose("ammo_blaster", 2) == "ammo_blaster"); bg_itemlist[7].mins[0] = -8;
        reached.clear(); for (int i = 1; i < 40; ++i) reached.insert(Choose("ammo_blaster", i)); CHECK(reached.size() == 2);

        // Authored G_CallSpawn stops at the first missing classname.
        // A candidate beyond that sentinel cannot replace a valid earlier item.
        const char *savedClass = bg_itemlist[3].classname;
        bg_itemlist[3].classname = nullptr;
        for (int i = 1; i <= 256; ++i) {
            const auto choice = Choose("weapon_blaster", i);
            CHECK(choice == "weapon_blaster_pistol" || choice == "weapon_blaster");
        }
        bg_itemlist[3].classname = savedClass;

        g_eSavedGameJustLoaded = eFULL; Init();
        for (int i = 1; i < 256; ++i) CHECK(Choose("weapon_blaster", i) == "weapon_blaster");
        g_eSavedGameJustLoaded = eAUTO; Init();
        for (int i = 1; i <= 256; ++i) CHECK(Choose("weapon_blaster", i) == results[i-1]);
        g_eSavedGameJustLoaded = eNO;
        Init("overflow"); CHECK(warnings == 1 && Choose("weapon_blaster", 2) == "weapon_blaster");
        Init("73", "2"); CHECK(warnings == 2 && Choose("ammo_blaster", 2) == "ammo_blaster");
        Init(); CHECK(warnings == 2);
        fields.clear();
        Add(13,"weapon_disruptor",IT_WEAPON,WP_DISRUPTOR);
        Add(14,"weapon_demp2",IT_WEAPON,WP_DEMP2);
        Add(15,"weapon_rocket_launcher",IT_WEAPON,WP_ROCKET_LAUNCHER);
        Add(16,"weapon_thermal",IT_WEAPON,WP_THERMAL);
        Add(17,"weapon_trip_mine",IT_WEAPON,WP_TRIP_MINE);
        Add(18,"weapon_det_pack",IT_WEAPON,WP_DET_PACK);
        Add(19,"ammo_powercell",IT_AMMO,AMMO_POWERCELL);
        Add(20,"ammo_rockets",IT_AMMO,AMMO_ROCKETS);
        Set("jar_randomWide","1"); Init(); reached.clear();
        for (int i=1; i<=512; ++i) {
            std::string picked=Choose("weapon_blaster",i);
            CHECK(picked!="weapon_blaster"); reached.insert(picked);
        }
        CHECK(reached.count("weapon_rocket_launcher") && reached.count("weapon_demp2") && reached.count("ammo_powercell"));
        CHECK(reached.count("item_bacta") && reached.count("weapon_thermal"));
        // Special-purpose sources are still protected even in Chaos.
        for (const char *name : {"weapon_saber","weapon_demp2","ammo_powercell","item_security_key"}) CHECK(Choose(name,3)==name);
        Init("73","0"); CHECK(Choose("weapon_blaster",3)=="weapon_blaster");
        fields={{"targetname","script_pickup"}}; Init(); CHECK(Choose("weapon_blaster",3)=="weapon_blaster"); fields.clear();

        Set("jar_randomEnemies","1"); Init();
        auto npc = [](int ordinal, int flags=0) {
            gentity_t e={}; e.classname=const_cast<char *>("NPC_Stormtrooper");e.NPC_type=const_cast<char *>("stormtrooper");
            e.spawnflags=flags; e.targetname=const_cast<char *>("wave1"); e.count=3; e.wait=2;
            G_JAR_RandomizeMapNPC(&e,ordinal);
            CHECK(!std::strcmp(e.targetname,"wave1") && e.count==3 && e.wait==2);
            return std::string(e.NPC_type ? e.NPC_type : e.classname);
        };
        fields={{"classname","NPC_Stormtrooper"},{"targetname","wave1"},{"count","3"},{"wait","2"}};
        std::vector<std::string> enemies;
        reached.clear();
        for (int i=1;i<=256;++i) { enemies.push_back(npc(i,1|16|256)); reached.insert(enemies.back()); }
        CHECK(reached.size()==24 && reached.count("fixture23") && !reached.count("vehicle") && !reached.count("giant"));
        for (int i=256;i>=1;--i) CHECK(npc(i,1|16|256)==enemies[i-1]);
        Init("74"); different=0; for (int i=1;i<=256;++i) different+=npc(i)!=enemies[i-1]; CHECK(different>128); Init();
        CHECK(npc(0)=="stormtrooper");
        for (int flag : {32,64,128,8192}) CHECK(npc(3,flag)=="stormtrooper");
        for (const char *field : {"npc_targetname","NPC_target","message","spawnscript","deathscript","parm1","script_targetname","team","unknown"}) {
            fields.emplace_back(field,""); CHECK(npc(3)=="stormtrooper"); fields.pop_back();
        }
        fields.clear();
        for (const char *name : {"NPC_Vehicle","NPC_Player","NPC_Kyle","NPC_Monster_Rancor","NPC_Tavion","NPC_Reborn_New","NPC_Merchant"}) {
            gentity_t e={};e.classname=const_cast<char *>(name);G_JAR_RandomizeMapNPC(&e,3);CHECK(!std::strcmp(e.classname,name) && !e.NPC_type);
        }
        definitionsValid=false; Init(); CHECK(npc(3)=="stormtrooper"); definitionsValid=true; Init();
        gentity_t protectedNpc={};protectedNpc.classname=const_cast<char *>("NPC_Tusken");protectedNpc.message=const_cast<char *>("key");
        G_JAR_RandomizeMapNPC(&protectedNpc,3);CHECK(!protectedNpc.NPC_type);
        protectedNpc.message=nullptr;protectedNpc.behaviorSet[BSET_SPAWN]=const_cast<char *>("cinematic");
        G_JAR_RandomizeMapNPC(&protectedNpc,3);CHECK(!protectedNpc.NPC_type);

        Set("jar_randomLoadout","1"); registered.clear(); Init(); CHECK(registered.size()==3);
        for (int i=WP_BLASTER_PISTOL;i<=WP_CONCUSSION;++i) weaponData[i].ammoIndex=AMMO_BLASTER;
        ammoData[AMMO_BLASTER].max=300;
        gclient_t client={}; gentity_t player={}; player.client=&client;
        const unsigned missionTools=(1u<<WP_SABER)|(1u<<WP_DEMP2);
        client.ps.stats[STAT_WEAPONS]=missionTools;
        G_JAR_RandomizePlayerLoadout(&player,0);
        unsigned expected=missionTools;for (int weapon:registered)expected|=1u<<weapon;
        CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==expected && client.ps.ammo[AMMO_BLASTER]==300);
        client.ps.stats[STAT_WEAPONS]=missionTools;client.ps.ammo[AMMO_BLASTER]=5;
        G_JAR_RandomizePlayerLoadout(&player,64);CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==missionTools && client.ps.ammo[AMMO_BLASTER]==5);
        G_JAR_RandomizePlayerLoadout(&player,32);CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==missionTools);
        player.s.number=1;G_JAR_RandomizePlayerLoadout(&player,0);CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==missionTools);player.s.number=0;
        g_eSavedGameJustLoaded=eFULL;Init();CHECK(npc(3)=="stormtrooper" && Choose("weapon_blaster",3)=="weapon_blaster");
        G_JAR_RandomizePlayerLoadout(&player,0);CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==missionTools && client.ps.ammo[AMMO_BLASTER]==5);
        CHECK(vars["jar_randomStatus"].value.find("saved world")!=std::string::npos);
        g_eSavedGameJustLoaded=eAUTO;Init();G_JAR_RandomizePlayerLoadout(&player,0);CHECK(unsigned(client.ps.stats[STAT_WEAPONS])==expected);
        Set("jar_randomEnemies","2");Init();CHECK(npc(3)=="stormtrooper" && Choose("weapon_blaster",3)=="weapon_blaster");
        CHECK(vars["jar_randomStatus"].value.find("invalid")!=std::string::npos);

        // Production fixed mappings: interleaving, map reloads, reordered catalogs,
        // pool changes, save reconstruction and the same mission/identity guards.
        vars.clear();fields.clear();g_eSavedGameJustLoaded=eNO;
        Set("jar_randomEnemies","1");Set("jar_randomLoadout","1");Set("jar_randomObjects","1");
        Init("73","1","1","1","0");
        std::map<std::string,std::string> fixedItems;
        for(const auto& p:catalog)fixedItems[p.name]=Choose(p.name,1);
        const auto fixedNpc=npc(1);const auto fixedGuns=loadoutMask;
        CHECK(fixedGuns);std::reverse(jarNpcCatalog.begin(),jarNpcCatalog.end());
        for(int i=256;i>0;--i) {
            CHECK(npc(i)==fixedNpc);
            for(const auto& p:catalog)CHECK(Choose(p.name,i,12000)==fixedItems[p.name]);
        }
        auto props=[&](int ordinal){
            std::map<std::string,std::string> out;
            for(const auto& p:staticProps) {
                gentity_t e={};e.classname=const_cast<char *>("misc_model_static");e.model=const_cast<char *>(p.name);
                G_JAR_RandomizeMapObject(&e,ordinal);out[p.name]=e.model;
            }return out;
        };
        for(auto& valid:propValid)valid=true; // trusted geometry fixture; native tests fingerprint real PK3s
        const auto fixedProps=props(1);CHECK(props(99)==fixedProps);
        unsigned propChanged=0;for(const auto& p:fixedProps)propChanged+=p.first!=p.second;CHECK(propChanged);
        Q_strncpyz(level.mapname,"another_map",sizeof(level.mapname));
        Init("73","1","1","1","0");for(auto& valid:propValid)valid=true;
        CHECK(npc(77)==fixedNpc&&loadoutMask==fixedGuns&&props(77)==fixedProps);
        for(const auto& p:catalog)CHECK(Choose(p.name,77)==fixedItems[p.name]);
        std::set<std::string> seedChoices;std::set<unsigned> gunChoices;
        for(int i=0;i<64;++i){Init(std::to_string(i).c_str(),"1","1","1","0");seedChoices.insert(npc(42));gunChoices.insert(loadoutMask);}
        CHECK(seedChoices.size()>8&&gunChoices.size()>8);
        Init("73","1","1","1","0");CHECK(npc(88)==fixedNpc&&loadoutMask==fixedGuns);
        // Remove the current eligible winner: select another valid entry, then
        // restoring the catalog restores the original mapping regardless of order.
        auto savedCatalog=jarNpcCatalog;
        for(auto& e:jarNpcCatalog)if(e.name==fixedNpc)e.reason="fixture-unavailable";
        CHECK(npc(1)!=fixedNpc);jarNpcCatalog=savedCatalog;CHECK(npc(1)==fixedNpc);
        fields={{"parm1","story"}};CHECK(npc(1)=="stormtrooper");fields.clear();
        Set("jar_npcSafety","0");gentity_t fixedNamed={};fixedNamed.NPC_type=const_cast<char *>("stormtrooper");
        fixedNamed.NPC_targetname=const_cast<char *>("named_ally");fixedNamed.targetname=const_cast<char *>("spawner");
        G_JAR_RandomizeMapNPC(&fixedNamed,90);gclient_t fixedBody={};gentity_t fixedActor={};fixedActor.client=&fixedBody;
        fixedActor.targetname=fixedActor.script_targetname=const_cast<char *>("named_ally");G_JAR_ApplyNpcIdentity(&fixedNamed,&fixedActor);
        CHECK(!strcmp(fixedNamed.targetname,"spawner")&&!strcmp(fixedActor.script_targetname,"named_ally"));
        CHECK(fixedBody.playerTeam==TEAM_ENEMY&&fixedBody.enemyTeam==TEAM_PLAYER);Set("jar_npcSafety","1");
        Set("jar_randomSeed","999");Set("jar_randomMode","1");g_eSavedGameJustLoaded=eFULL;G_JAR_InitPickupRandomizer();
        for(auto& valid:propValid)valid=true;
        CHECK(props(44)==fixedProps&&vars["jar_worldMode"].value=="0"&&vars["jar_worldSeed"].value=="73");
        CHECK(Choose("weapon_blaster",4)=="weapon_blaster"&&npc(4)=="stormtrooper");
        // A pre-0.10 full save supplies legacy world mode through BeforeLoad's
        // fallback; props reproduce the old ordinal-based choice at reconstruction.
        Set("jar_worldMode","1");Set("jar_randomMode","0");G_JAR_InitPickupRandomizer();for(auto& valid:propValid)valid=true;
        const auto oldProps=props(44);
        for(const auto& source:staticProps) {
            std::vector<const StaticProp*> pool;
            for(const auto& p:staticProps){bool fit=&p!=&source;for(int axis=0;axis<3;++axis)fit&=p.mins[axis]>=source.mins[axis]&&p.maxs[axis]<=source.maxs[axis];if(fit)pool.push_back(&p);}
            CHECK(oldProps.at(source.name)==(pool.empty()?source.name:pool[jar::PickupDecision(73,level.mapname,44,"objects-v1",source.name)%pool.size()]->name));
        }
        // Invalid modes fail closed.
        g_eSavedGameJustLoaded=eNO;Init("73","1","1","1","invalid");CHECK(Choose("weapon_blaster",4)=="weapon_blaster");
        Q_strncpyz(level.mapname,"t1_fatal",sizeof(level.mapname));
        vars.clear();g_eSavedGameJustLoaded=eNO;Set("jar_randomProjectiles","1");Init();
        CHECK(G_JAR_ProjectilesEnabled());CHECK(vars["jar_projectileSequence"].value=="0");Set("jar_projectileSequence","39");
        Set("jar_randomSeed","999");Set("jar_randomProjectiles","0");g_eSavedGameJustLoaded=eFULL;G_JAR_InitPickupRandomizer();
        CHECK(G_JAR_ProjectilesEnabled());CHECK(vars["jar_worldSeed"].value=="73"&&vars["jar_projectileSequence"].value=="39");
        g_eSavedGameJustLoaded=eNO;G_JAR_InitPickupRandomizer();CHECK(!G_JAR_ProjectilesEnabled());CHECK(vars["jar_projectileSequence"].value=="0");
        CHECK(vars["jar_worldSeed"].value=="999");
        Set("jar_randomEnemies","1");Set("jar_npcSafety","0");Init();fields={{"parm1","mission"}};
        reached.clear();for(int i=1;i<1024;++i)reached.insert(npc(i));CHECK(reached.size()==25&&reached.count("giant")&&!reached.count("vehicle"));
        gentity_t named={};named.NPC_type=const_cast<char *>("stormtrooper");named.NPC_targetname=const_cast<char *>("kyle");named.script_targetname=const_cast<char *>("cin_kyle");named.targetname=const_cast<char *>("spawn_kyle");named.cameraGroup=const_cast<char *>("camera1");
        Set("jar_npcSafety","1");G_JAR_RandomizeMapNPC(&named,8);CHECK(!std::strcmp(named.NPC_type,"stormtrooper"));
        Set("jar_npcSafety","0");G_JAR_RandomizeMapNPC(&named,8);gentity_t actor={};gclient_t ac={};actor.client=&ac;actor.targetname=actor.script_targetname=const_cast<char *>("kyle");G_JAR_ApplyNpcIdentity(&named,&actor);
        CHECK(!std::strcmp(named.targetname,"spawn_kyle")&&!std::strcmp(named.NPC_targetname,"kyle"));
        CHECK(!std::strcmp(actor.NPC_targetname,"kyle")&&!std::strcmp(actor.script_targetname,"kyle")&&!std::strcmp(actor.cameraGroup,"camera1"));
        CHECK(ac.playerTeam==TEAM_ENEMY&&ac.enemyTeam==TEAM_PLAYER&&!std::strcmp(ac.clientInfo.name,"kyle"));
        ac.enemyTeam=TEAM_FREE;G_JAR_ApplyNpcAllegiance(&actor);CHECK(ac.enemyTeam==TEAM_PLAYER);
        JarNpcTemplate source;source.team=TEAM_PLAYER;source.npcClass=CLASS_KYLE;source.enemyTeam=TEAM_FREE;CHECK(G_JAR_SourceEnemyTeam(source,0)==TEAM_ENEMY);
        source.team=TEAM_ENEMY;source.npcClass=CLASS_REBORN;source.weapon=WP_BLASTER;CHECK(G_JAR_SourceEnemyTeam(source,0)==TEAM_FREE);source.weapon=WP_NONE;CHECK(G_JAR_SourceEnemyTeam(source,0)==TEAM_PLAYER);
        actor.targetname=const_cast<char *>("kyle");actor.NPC=reinterpret_cast<decltype(actor.NPC)>(1);actor.inuse=qtrue;actor.health=0;ac.playerTeam=TEAM_PLAYER;
        CHECK(!G_JAR_PreserveActor(&actor));Set("jar_actorContinuity","1");CHECK(G_JAR_PreserveActor(&actor)&&actor.health==1&&ac.ps.stats[STAT_HEALTH]==1);
        sandboxOwned=true;CHECK(!G_JAR_PreserveActor(&actor));sandboxOwned=false;
    {JarAuthoredDeathScope authored(&actor);CHECK(!G_JAR_PreserveActor(&actor));}CHECK(G_JAR_PreserveActor(&actor));
        actor.behaviorSet[BSET_FFDEATH]=const_cast<char *>("ffdeath");CHECK(!G_JAR_PreserveActor(&actor));actor.behaviorSet[BSET_FFDEATH]=nullptr;
        actor.target=const_cast<char *>("death_objective");CHECK(!G_JAR_PreserveActor(&actor));actor.target=nullptr;
        actor.svFlags|=SVF_KILLED_SELF;CHECK(!G_JAR_PreserveActor(&actor));actor.svFlags&=~SVF_KILLED_SELF;
        ac.ps.pm_type=PM_DEAD;CHECK(!G_JAR_PreserveActor(&actor));ac.ps.pm_type=PM_NORMAL;ac.playerTeam=TEAM_ENEMY;CHECK(!G_JAR_PreserveActor(&actor));

        // Recovery v1 is enrolled from spawn metadata and keeps exact controller
        // ownership through death. A removed actor becomes noncombat; authored
        // removal and save reconstruction release it normally.
        Set("jar_actorRecovery","1");Set("jar_actorContinuity","0");G_JAR_InitActorRecovery();
        gentity_t original={},recovery={},nav={};gNPC_t brain={};gclient_t body={};
        // Actual native save-field rule on the native x64 entity layout. The old
        // F_NULL pointer write zeroed voice task 6 while ISEQ still waited for it.
        original.m_iIcarusID=20;
        for(int i=0;i<NUM_TIDS;++i)original.taskID[i]=6+i;
        CHECK(G_ClearTransientSaveField(&icarusSaveField,&original));
        CHECK(original.m_iIcarusID==0);
        for(int i=0;i<NUM_TIDS;++i)CHECK(original.taskID[i]==6+i);
        original.NPC_targetname=const_cast<char *>("cin_actor");
        recovery.NPC=&brain;recovery.client=&body;recovery.inuse=qtrue;recovery.s.number=1;
        recovery.targetname=recovery.script_targetname=original.NPC_targetname;
        recovery.m_iIcarusID=17;body.playerTeam=TEAM_PLAYER;body.ps.pm_type=PM_NORMAL;body.moveType=MT_RUNJUMP;
        nav.inuse=qtrue;brain.tempGoal=brain.goalEntity=&nav;
        memset(recovery.taskID,-1,sizeof(recovery.taskID));
        recovery.taskID[TID_ANIM_UPPER]=recovery.taskID[TID_ANIM_LOWER]=recovery.taskID[TID_ANIM_BOTH]=71;
        recovery.taskID[TID_CHAN_VOICE]=82;recovery.taskID[TID_MOVE_NAV]=93;
        body.ps.torsoAnimTimer=3500;body.ps.legsAnimTimer=-1;brain.desiredYaw=90;body.ps.friction=3;brain.behaviorState=BS_WAIT;brain.tempBehavior=BS_WAIT;brain.defaultBehavior=BS_CINEMATIC;
        G_JAR_EnrollActor(&original,&recovery);gi.linkentity=[](gentity_t*){};
        {JarActorRecoveryDeath transition(&recovery);CHECK(transition.Active());
         CHECK(recovery.taskID[TID_ANIM_BOTH]==-1&&recovery.taskID[TID_CHAN_VOICE]==82);
         body.ps.pm_type=PM_DEAD;body.ps.torsoAnimTimer=999;brain.desiredYaw=0;body.ps.friction=0;brain.behaviorState=BS_REMOVE;brain.tempBehavior=BS_DEFAULT;}
        CHECK(G_JAR_IsActorStandin(&recovery)&&recovery.m_iIcarusID==17&&brain.tempGoal==&nav&&brain.goalEntity==&nav);
        CHECK(recovery.taskID[TID_ANIM_BOTH]==71&&recovery.taskID[TID_CHAN_VOICE]==82&&recovery.taskID[TID_MOVE_NAV]==93);
        CHECK(body.ps.torsoAnimTimer==3500&&body.ps.legsAnimTimer==-1&&brain.desiredYaw==90&&body.ps.friction==3);
        CHECK(brain.behaviorState==BS_WAIT&&brain.tempBehavior==BS_WAIT&&brain.defaultBehavior==BS_CINEMATIC);
        CHECK(G_CurrentBState(&brain)==BS_WAIT);
        brain.behaviorState=BS_CINEMATIC;CHECK(G_CurrentBState(&brain)==BS_WAIT);
        brain.tempBehavior=BS_DEFAULT;CHECK(G_CurrentBState(&brain)==BS_CINEMATIC);
        brain.behaviorState=BS_DEFAULT;brain.defaultBehavior=BS_WAIT;CHECK(G_CurrentBState(&brain)==BS_WAIT);
        CHECK(recovery.contents==0&&!recovery.takedamage&&(recovery.s.eFlags&EF_NODRAW)&&body.ps.weapon==WP_NONE);
        CHECK(G_JAR_RemoveToStandin(&recovery)); // retain an existing controller without recursion
        G_JAR_ForgetActor(&recovery);CHECK(!G_JAR_IsActorStandin(&recovery)&&!G_JAR_RemoveToStandin(&recovery));
        G_JAR_EnrollActor(&original,&recovery);
        {JarActorRecoverySuspension loading;CHECK(!G_JAR_RemoveToStandin(&recovery));}
        CHECK(G_JAR_RemoveToStandin(&recovery)&&G_JAR_IsActorStandin(&recovery));
        G_JAR_ForgetActor(&recovery);original.behaviorSet[BSET_DEATH]=const_cast<char *>("story/death");
        G_JAR_EnrollActor(&original,&recovery);CHECK(!G_JAR_RemoveToStandin(&recovery));
        original.behaviorSet[BSET_DEATH]=nullptr;Set("jar_actorRecovery","0");
        G_JAR_EnrollActor(&original,&recovery);Set("jar_actorRecovery","1");CHECK(!G_JAR_RemoveToStandin(&recovery));
        G_JAR_EnrollActor(&original,&recovery);recovery.target=const_cast<char *>("objective");CHECK(!G_JAR_RemoveToStandin(&recovery));
        recovery.target=nullptr;recovery.taskID[TID_BSTATE]=17;CHECK(!G_JAR_RemoveToStandin(&recovery));
        recovery.taskID[TID_BSTATE]=-1;
        {JarAuthoredDeathScope authored(&recovery);CHECK(!G_JAR_RemoveToStandin(&recovery));}
        brain.aiFlags|=NPCAI_MATCHPLAYERWEAPON;CHECK(!G_JAR_RemoveToStandin(&recovery));brain.aiFlags&=~NPCAI_MATCHPLAYERWEAPON;
        body.clientInfo.animFileIndex=1;CHECK(!G_JAR_RemoveToStandin(&recovery));body.clientInfo.animFileIndex=0;
        recovery.svFlags|=SVF_CUSTOM_GRAVITY;CHECK(!G_JAR_RemoveToStandin(&recovery));recovery.svFlags&=~SVF_CUSTOM_GRAVITY;
        brain.scriptFlags|=SCF_SAFE_REMOVE;CHECK(!G_JAR_RemoveToStandin(&recovery));brain.scriptFlags&=~SCF_SAFE_REMOVE;

    });
}
