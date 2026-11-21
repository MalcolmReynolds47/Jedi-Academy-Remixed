// Production actor eligibility with loaded definitions and native resource audit.
#include "game/g_local.h"
#include "game/g_randomizer.h"
#include "game/g_workshop.h"
#include "game/g_workshop_actor_spawn.h"
#include "test_support.h"
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
game_import_t gi{};level_locals_t level{};static const char*NPCParms;
static bool trackedConsole=false;
bool G_JAR_WorkshopActorSpawn(const gentity_t& actor){return (actor.spawnflags&JAR_NPC_OVERLAP_SPAWN)||trackedConsole;}
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
weaponData_t weaponData[WP_NUM_WEAPONS]{};gitem_t bg_itemlist[6]{};extern const int bg_numItems=6;
#include "game/g_npc_classes.inc"
#include "game/g_npc_force_powers.inc"
stringID_table_t WPTable[]={{"WP_REPEATER",WP_REPEATER},{"WP_MELEE",WP_MELEE},{"WP_NONE",WP_NONE},{"WP_SABER",WP_SABER},{"WP_BLASTER",WP_BLASTER},{"WP_BOT_LASER",WP_BOT_LASER},{"",-1}};
// These are the actual native fallback cases exercised by this fixture.
int NPC_WeaponsForTeam(team_t team,int,const char*name){if(team==TEAM_ENEMY)return (!Q_stricmp(name,"desann")||!Q_stricmpn(name,"reborn",6))?1u<<WP_SABER:1u<<WP_BLASTER;if(team==TEAM_PLAYER&&Q_stricmp(name,"monmothma"))return 1u<<WP_BLASTER;return 0;}
static std::set<std::string> files;
static std::string probed;static bool probeOk=true,fingerprintMatches=false,vehicleOk=true;
static bool legacyProbeOk=true,skinProbeOk=true;
qhandle_t cgi_R_RegisterModel(const char*name){probed=name;return legacyProbeOk&&files.count(name)?1:0;}
#include "game/g_randomizer_npc_catalog.inc"
struct Actor {uint32_t id;const char*name,*model;uint32_t npcHash,vehicleHash,alternateHash;float radius,height;bool vehicle;uint32_t npcAlternate=0;};
bool G_JAR_NpcDefinitionMatches(const char*,unsigned){return fingerprintMatches;}
int BG_VehicleGetIndex(const char*){return vehicleOk?1:0;}
bool vehicleDefinition(const Actor&){return vehicleOk;}
#include "game/g_workshop_actor_catalog.inc"
static void Services(){
 gi.FS_ReadFile=[](const char*n,void**)->long{return files.count(n)?100:-1;};
 gi.G2API_InitGhoul2Model=[](CGhoul2Info_v&,const char*name,int,qhandle_t,qhandle_t,int,int){probed=name;return probeOk?0:-1;};
 gi.G2API_CleanGhoul2Models=[](CGhoul2Info_v&){};
 gi.RE_RegisterSkin=[](const char*name){return skinProbeOk&&files.count(name)?1:0;};
 Q_strncpyz(weaponData[WP_REPEATER].weaponMdl,"models/weapons2/heavy_repeater/heavy_repeater.md3",MAX_QPATH);
 for(int i=1;i<=5;++i){bg_itemlist[i].classname="weapon";bg_itemlist[i].giType=IT_WEAPON;bg_itemlist[i].giTag=i==1?WP_SABER:i==2?WP_BLASTER:i==3?WP_BOT_LASER:i==4?WP_MELEE:WP_REPEATER;}
 Q_strncpyz(weaponData[WP_SABER].weaponMdl,"models/weapons2/saber/saber_w.md3",MAX_QPATH);Q_strncpyz(weaponData[WP_BLASTER].weaponMdl,"models/weapons2/blaster_r/blaster.md3",MAX_QPATH);
 Q_strncpyz(weaponData[WP_MELEE].weaponMdl,"models/weapons2/noweap/noweap.md3",MAX_QPATH);
 Q_strncpyz(weaponData[WP_BOT_LASER].weaponMdl,"models/weapons2/noweap/noweap.md3",MAX_QPATH);
}
static void Reset(const std::string&text){NPCParms=text.c_str();npcResources.clear();npcResourcesRead=false;}
static Actor Request(const char*name){return {1,name,"models/players/wrong_cached_model/model.glm",123,0,0,16,64,false};}
int main(int argc,char**argv){return RunTests("loaded NPC admission: mod overrides, retail class alias, placement bounds and precise rejection",[&]{
 Services();std::string model,failure;
 {
  static bool obstructed=false;
  gi.trace=[](trace_t* hit,const vec3_t,const vec3_t mins,const vec3_t maxs,const vec3_t,int skip,int mask,EG2_Collision collision,int){
   CHECK(skip==43&&collision==G2_BRUSH_ONLY&&mask==MASK_SOLID&&mins[2]==-24&&maxs[2]==40);
   *hit={};hit->startsolid=obstructed?qtrue:qfalse;
  };
  gentity_t actor{};gclient_t client{};gNPC_t npc{};
  CHECK(G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64));
  actor.inuse=qtrue;actor.client=&client;actor.NPC=&npc;actor.s.number=43;
  actor.mins[2]=-24;actor.maxs[2]=40;
  CHECK(G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64));
  actor.e_ThinkFunc=thinkF_NPC_Think;
  actor.s.eFlags|=EF_NODRAW;CHECK(G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64));actor.s.eFlags=0;
  actor.currentOrigin[2]=72;
  CHECK(G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64)&&!G_WorkshopActorSpawnFailure(actor,vec3_origin,true,96));
  actor.currentOrigin[2]=0;obstructed=true;
  CHECK(strstr(G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64),"collision hull"));
  obstructed=false;CHECK(!G_WorkshopActorSpawnFailure(actor,vec3_origin,false,64)&&G_JAR_CountCampaignActor(actor));
  actor.spawnflags=JAR_NPC_OVERLAP_SPAWN;CHECK(!G_JAR_CountCampaignActor(actor));
  actor.spawnflags=0;trackedConsole=true;CHECK(!G_JAR_CountCampaignActor(actor));trackedConsole=false;
 }
 if(argc==3){
  std::ifstream input(argv[1],std::ios::binary);CHECK(input.good());const std::string definitions((std::istreambuf_iterator<char>(input)),{});
  std::ifstream list(argv[2]);CHECK(list.good());for(std::string path;std::getline(list,path);)files.insert(path);
  Reset(definitions);for(const char*name:{"desann","monmothma","rebornboss","morgankatarn","mouse","remote_sp","remote","seeker","nulldriver","glider","mark1","mark2","minemonster","galak_mech"}){auto a=Request(name);CHECK(actorValid(a,model,failure));CHECK(model==probed);printf("Eligible installed definition: %s model=%s bounds=%.0f x %.0f\n",name,a.model,a.radius,a.height);}
  for(const char*name:{"glider","mark1","mark2","minemonster","galak_mech"}){const std::string path="models/players/"+std::string(name)+"/model.glm";files.erase(path);Reset(definitions);auto a=Request(name);CHECK(!actorValid(a,model,failure)&&failure=="missing-Ghoul2-model");files.insert(path);Reset(definitions);CHECK(actorValid(a,model,failure));}return;
 }
 files={"models/players/desann/model.glm","models/players/monmothma/model.glm","models/players/reborn/model.glm","models/players/reborn/model_boss.skin","models/weapons2/saber/saber_w.md3","models/weapons2/blaster_r/blaster.md3"};
 const std::string definitions="Desann\n{\nplayerModel desann\nclass CLASS_DESANN\nplayerTeam TEAM_ENEMY\nenemyTeam TEAM_PLAYER\nweapon WP_SABER\nheight 90\nwidth 20\nhealth 900\n}\nMonMothma\n{\nplayerModel monmothma\nclass CLASS_MONMOTHMA\nplayerTeam TEAM_PLAYER\nenemyTeam TEAM_ENEMY\nweapon WP_NONE\n}\nRebornBoss\n{\nplayerModel reborn\ncustomSkin boss\nclass CLASS_REBORN\nplayerTeam TEAM_ENEMY\nweapon WP_SABER\n}\n";
 Reset(definitions);auto desann=Request("desann");CHECK(actorValid(desann,model,failure)&&model=="models/players/desann/model.glm"&&desann.radius==20&&desann.height==90&&!fingerprintMatches);
 auto mon=Request("monmothma");CHECK(actorValid(mon,model,failure)&&mon.height==64&&npcResources.at("monmothma").npcClass==CLASS_MONMOTHA);
 auto boss=Request("rebornboss");CHECK(actorValid(boss,model,failure));
 files.insert("models/players/morgan/model.glm");
 const std::string morganDefinition="MorganKatarn\n{\nplayerModel morgan\nweapon WP_NONE\nclass CLASS_MORGAN\nplayerTeam TEAM_NEUTRAL\nenemyTeam TEAM_NEUTRAL\n}\n";
 Reset(morganDefinition);auto morgan=Request("morgankatarn");CHECK(actorValid(morgan,model,failure)&&model=="models/players/morgan/model.glm"&&npcResources.at("morgankatarn").npcClass==CLASS_MORGANKATARN);
 Reset(definitions);
 auto missing=Request("absent");CHECK(!actorValid(missing,model,failure)&&failure=="NPC definition not loaded");
 files.erase("models/players/reborn/model_boss.skin");Reset(definitions);CHECK(!actorValid(boss,model,failure)&&failure=="missing-required-skin");files.insert("models/players/reborn/model_boss.skin");
 files.erase("models/weapons2/saber/saber_w.md3");Reset(definitions);CHECK(!actorValid(desann,model,failure)&&failure=="missing-NPC-weapon-item-or-render-model");files.insert("models/weapons2/saber/saber_w.md3");
 Reset(definitions);probeOk=false;CHECK(!actorValid(mon,model,failure)&&failure=="Ghoul2 model initialization failed");probeOk=true;
 auto vehicle=Request("swoop");vehicle.vehicle=true;CHECK(!actorValid(vehicle,model,failure)&&failure.find("vehicle/player")!=std::string::npos);fingerprintMatches=true;vehicleOk=false;CHECK(!actorValid(vehicle,model,failure)&&failure=="vehicle definition missing or changed");
 const std::string legacy="mouse\n{\nheadmodel none\ntorsomodel none\nlegsmodel mouse\nclass CLASS_MOUSE\nwidth 8\nheight 16\n}\n";
 files.insert("models/players/mouse/lower.md3");files.insert("models/players/mouse/animation.cfg");Reset(legacy);auto mouse=Request("mouse");probeOk=false;
 CHECK(actorValid(mouse,model,failure)&&model=="models/players/mouse/lower.md3"&&model==probed&&mouse.radius==8&&mouse.height==16); // no Ghoul2 probe
 legacyProbeOk=false;CHECK(!actorValid(mouse,model,failure)&&failure=="legacy model initialization failed");legacyProbeOk=true;
 level.numKnownAnimFileSets=MAX_ANIM_FILES;CHECK(!actorValid(mouse,model,failure)&&failure=="NPC animation cache is full");
 Q_strncpyz(level.knownAnimFileSets[0].filename,"mouse",sizeof(level.knownAnimFileSets[0].filename));CHECK(actorValid(mouse,model,failure));level.numKnownAnimFileSets=0;
 files.insert("models/players/mouse/lower_default.skin");Reset(legacy);skinProbeOk=false;CHECK(!actorValid(mouse,model,failure)&&failure=="legacy skin initialization failed");skinProbeOk=true;CHECK(actorValid(mouse,model,failure));
 const std::string remoteDefinition="remote\n{\nplayerModel remote\nclass CLASS_SEEKER\nplayerTeam TEAM_FREE\nenemyTeam TEAM_FREE\nweapon WP_BOT_LASER\nwidth 8\nheight 32\n}\n";
 files.insert("models/players/remote_sp/lower.md3");files.insert("models/players/remote_sp/animation.cfg");files.insert("models/weapons2/noweap/noweap.md3");Reset(remoteDefinition);auto remote=Request("remote");
 CHECK(actorValid(remote,model,failure)&&model=="models/players/remote_sp/lower.md3"&&model==probed&&remote.radius==8&&remote.height==32);
 legacyProbeOk=false;CHECK(!actorValid(remote,model,failure)&&failure=="legacy model initialization failed");legacyProbeOk=true;
 level.numKnownAnimFileSets=MAX_ANIM_FILES;CHECK(!actorValid(remote,model,failure)&&failure=="NPC animation cache is full");
 Q_strncpyz(level.knownAnimFileSets[0].filename,"remote_sp",sizeof(level.knownAnimFileSets[0].filename));CHECK(actorValid(remote,model,failure));
 });}
