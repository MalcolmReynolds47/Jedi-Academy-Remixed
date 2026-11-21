#include "game/g_local.h"
#include "game/g_randomizer.h"
#include "test_support.h"
#include "memory_save.h"
#include "game/b_local.h"
#include "game/g_npc_seeker_init.inc"
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
#include <algorithm>
#include <set>
#include <cctype>
game_import_t gi={};static const char *NPCParms;
weaponData_t weaponData[WP_NUM_WEAPONS]={};gitem_t bg_itemlist[64]={};extern const int bg_numItems=64;
int NPC_WeaponsForTeam(team_t team,int,const char *name){return team==TEAM_PLAYER&&!Q_stricmp(name,"prisoner")?0:1u<<WP_BLASTER;}
#include "game/g_npc_classes.inc"
#include "game/g_npc_force_powers.inc"
stringID_table_t WPTable[]={{"WP_NONE",WP_NONE},{"WP_SABER",WP_SABER},{"WP_BLASTER",WP_BLASTER},{"WP_BOT_LASER",WP_BOT_LASER},{"",-1}};
static std::set<std::string> files;
#include "../../code/game/g_randomizer_npc_catalog.inc"
int main(){return RunTests("NPC catalog: native tokenizer, first definition, missing assets, compound skins, class and weapon semantics",[]{
 gi.FS_ReadFile=[](const char *n,void **)->long{return files.count(n)?100:-1;};
 files={"models/players/trooper/model.glm","models/players/trooper/model_default.skin","models/players/trooper/head.skin","models/players/trooper/torso.skin","models/players/trooper/lower.skin"};
 files.insert("fixture-weapon.md3");for(int weapon=1;weapon<WP_NUM_WEAPONS;++weapon){bg_itemlist[weapon].classname=const_cast<char*>("fixture");bg_itemlist[weapon].giType=IT_WEAPON;bg_itemlist[weapon].giTag=weapon;Q_strncpyz(weaponData[weapon].weaponMdl,"fixture-weapon.md3",MAX_QPATH);}
 std::string source;
 auto add=[&](const char *name,const char *extra){source+=std::string(name)+"\n{\nplayerModel trooper\nclass CLASS_STORMTROOPER\nplayerTeam enemy\nenemyTeam player\n"+extra+"\n}\n";};
 add("FIRST","weapon WP_SABER\nweapon WP_BLASTER\nweapon unknown\nwidth 16\nheight 56");add("first","playerModel missing");add("prisoner","playerTeam player\nweapon WP_NONE");
 add("multipart","customSkin head|torso|lower");add("missing","customSkin missing");add("part_missing","customSkin head|torso|absent");add("bad_parts","customSkin head|torso");add("vehicle","class CLASS_VEHICLE");add("bad_class","class absent");
 CHECK(GetIDForString(ClassTable,"CLASS_MORGAN")==CLASS_MORGANKATARN);CHECK(GetIDForString(ClassTable,"CLASS_MORGANKATARN")==CLASS_MORGANKATARN);
 CHECK(GetIDForString(ClassTable,"CLASS_MONMOTHMA")==CLASS_MONMOTHA);CHECK(GetIDForString(ClassTable,"CLASS_MONMOTHA")==CLASS_MONMOTHA);
 NPCParms=source.c_str();std::vector<JarNpcTemplate> out;G_JAR_ReadNpcCatalog(out);CHECK(out.size()==8);
 auto get=[&](const char *name)->const JarNpcTemplate&{for(const auto& e:out)if(e.name==name)return e;throw std::runtime_error("missing fixture");};
 CHECK(get("first").reason.empty()&&get("first").weapon==WP_BLASTER&&get("first").team==TEAM_ENEMY&&get("first").enemyTeam==TEAM_PLAYER);
 CHECK(get("multipart").reason.empty());CHECK(get("missing").reason=="missing-required-skin");CHECK(get("part_missing").reason=="missing-required-part-skin");CHECK(get("bad_parts").reason=="invalid-three-part-skin");CHECK(!get("vehicle").reason.empty()&&!get("bad_class").reason.empty());
 const std::string compact =
     "compact { playerModel trooper fullName \"}\" health 100 FP_PUSH 3 saberColor2 blue "
     "customRGBA 255 240 100 255 class CLASS_STORMTROOPER width 19 height 62 }\n"
     "COMPACT { playerModel absent class CLASS_STORMTROOPER width 24 }\n"
     "following { playerModel trooper customRGBA jedi_zf class CLASS_STORMTROOPER "
     "width 30 height 65 unused class invalid }\n"
     "missing_value { playerModel trooper class CLASS_STORMTROOPER fullName }\n"
     "after_missing { playerModel trooper class CLASS_STORMTROOPER width 12 }\n";
 NPCParms=compact.c_str();G_JAR_ReadNpcCatalog(out);
 CHECK(out.size()==4);
 CHECK(get("compact").reason.empty()&&get("compact").width==19&&get("compact").height==62);
 CHECK(get("following").reason.empty()&&get("following").width==30&&get("following").height==65);
 CHECK(get("after_missing").reason.empty()&&get("after_missing").width==12);
 COM_BeginParseSession();
 const char *body = JarNpcFindDefinition(compact.c_str(), "cOmPaCt");
 CHECK(body && !strncmp(body, " playerModel trooper", strlen(" playerModel trooper")));
 CHECK(JarNpcFindDefinition(compact.c_str(), "following"));
 CHECK(JarNpcFindDefinition(compact.c_str(), "after_missing"));
 CHECK(!JarNpcFindDefinition(compact.c_str(), "absent"));
 const char *quoted =
     "earlier { fullName \"}\" /* } */ snd \"\" fullName \"{\" health 100 }\n"
     "actor { fullName \"}\" width 62 }";
 body = JarNpcFindDefinition(quoted, "actor");
 unsigned fingerprint;
 CHECK(body && JarNpcBodyFingerprint(body, fingerprint) && fingerprint == 0x63fcf97cu);
 CHECK(!JarNpcFindDefinition("broken \"{\" fullName x }", "broken"));
 CHECK(!JarNpcFindDefinition("broken { fullName \"unterminated", "following"));
 CHECK(!JarNpcBodyFingerprint(" fullName \"unterminated", fingerprint));
 body = JarNpcFindDefinition(
     "model { fullName \"}\" width \n height 62 customRGBA 1 2 3 4 "
     "fullName \"\" unknown \"}\" ignored } next { }", "model");
 CHECK(body && !strcmp(JarNpcReadToken(body, qtrue), "fullName"));
 const char *name;
 CHECK(JarNpcParseValue(body, name) && !strcmp(name, "}"));
 CHECK(!strcmp(JarNpcReadToken(body, qtrue), "width"));
 int value;
 CHECK(!JarNpcParseValue(body, value));
 CHECK(!strcmp(JarNpcReadToken(body, qtrue), "height"));
 CHECK(JarNpcParseValue(body, value) && value == 62);
 const char *key = JarNpcReadToken(body, qtrue);
 CHECK(!strcmp(key, "customRGBA"));
 JarNpcSkipField(key, body);
 CHECK(!strcmp(JarNpcReadToken(body, qtrue), "fullName"));
 CHECK(JarNpcParseValue(body, name) && !*name);
 key = JarNpcReadToken(body, qtrue);
 CHECK(!strcmp(key, "unknown"));
 JarNpcSkipField(key, body);
 CHECK(!JarNpcParseValue(body, value));
 const char *close = JarNpcReadToken(body, qtrue);
 CHECK(JarNpcIsBrace(close, body, '}'));
 CHECK(!strcmp(JarNpcReadToken(body, qtrue), "next"));
 COM_EndParseSession();
 NPCParms=source.c_str();G_JAR_ReadNpcCatalog(out);
 Q_strncpyz(weaponData[WP_SABER].weaponMdl,"missing-weapon.md3",MAX_QPATH);G_JAR_ReadNpcCatalog(out);CHECK(get("first").reason=="missing-NPC-weapon-item-or-render-model"); // earlier explicit saber still precaches despite final blaster selection
 Q_strncpyz(weaponData[WP_SABER].weaponMdl,"fixture-weapon.md3",MAX_QPATH);
 Q_strncpyz(weaponData[WP_BLASTER].weaponMdl,"missing-weapon.md3",MAX_QPATH);G_JAR_ReadNpcCatalog(out);CHECK(get("multipart").reason=="missing-NPC-weapon-item-or-render-model");CHECK(get("prisoner").reason=="missing-NPC-weapon-item-or-render-model"); // original and restored-enemy fallback
 Q_strncpyz(weaponData[WP_BLASTER].weaponMdl,"fixture-weapon.md3",MAX_QPATH);
 files.erase("models/players/trooper/model_default.skin");G_JAR_ReadNpcCatalog(out);CHECK(get("first").reason.empty()); // native embedded-default materials, as in stock sentry
 files.erase("models/players/trooper/model.glm");G_JAR_ReadNpcCatalog(out);CHECK(!get("first").reason.empty());
 // Legacy actors use their own lower/upper/head resources, never a GLM fallback.
 source="mouse\n{\nheadmodel none\ntorsomodel none\nlegsmodel mouse\nclass CLASS_MOUSE\nwidth 8\nheight 16\n}\n";
 NPCParms=source.c_str();files.insert("models/players/mouse/lower.md3");files.insert("models/players/mouse/animation.cfg");
 G_JAR_ReadNpcCatalog(out);CHECK(get("mouse").reason=="no-Ghoul2-playerModel"); // campaign pool unchanged
 G_JAR_ReadNpcCatalog(out,true);CHECK(get("mouse").reason.empty()&&get("mouse").legacyModels.size()==1&&get("mouse").legacySkins.empty()&&get("mouse").legacyAnimation=="mouse");
 files.erase("models/players/mouse/animation.cfg");G_JAR_ReadNpcCatalog(out,true);CHECK(get("mouse").reason=="missing-legacy-animation-config");files.insert("models/players/mouse/animation.cfg");
 files.erase("models/players/mouse/lower.md3");G_JAR_ReadNpcCatalog(out,true);CHECK(get("mouse").reason=="missing-legacy-model");files.insert("models/players/mouse/lower.md3");
 auto legacy=[&](const char* extra){source=std::string("parts\n{\nlegsmodel mouse\nclass CLASS_MOUSE\n")+extra+"\n}\n";NPCParms=source.c_str();G_JAR_ReadNpcCatalog(out,true);};
 legacy("headmodel absent");CHECK(get("parts").reason=="missing-legacy-model");
 files.insert("models/players/fixture/head.md3");legacy("headmodel fixture/blue");CHECK(get("parts").reason=="missing-required-legacy-skin");
 files.insert("models/players/fixture/head_blue.skin");files.insert("models/players/fixture/upper.md3");files.insert("models/players/fixture/upper_default.skin");
 legacy("headmodel fixture/blue\ntorsomodel fixture");CHECK(get("parts").reason.empty()&&get("parts").legacyModels.size()==3&&get("parts").legacySkins.size()==2);
 files.insert("models/players/mouse/lower.mdr");legacy("");CHECK(get("parts").legacyModels.front()=="models/players/mouse/lower.mdr");
 char directory[MAX_QPATH];
 const char* skin=JAR_NpcSplitLegacyModel("mouse/blue",directory);
 CHECK(!strcmp(directory,"mouse")&&!strcmp(skin,"blue"));
 skin=JAR_NpcSplitLegacyModel("mouse",directory);
 CHECK(!strcmp(directory,"mouse")&&!strcmp(skin,"default"));
 legacy("legsmodel mouse/blue");CHECK(get("parts").reason=="missing-required-legacy-skin");
 files.insert("models/players/mouse/lower_blue.skin");
 legacy("legsmodel mouse/blue");
 CHECK(get("parts").reason.empty()&&get("parts").legacyAnimation==directory);
 CHECK(get("parts").legacyModels.size()==1&&get("parts").legacyModels[0]=="models/players/mouse/lower.mdr");
 CHECK(get("parts").legacySkins.size()==1&&get("parts").legacySkins[0]=="models/players/mouse/lower_blue.skin");
 files.erase("models/players/mouse/animation.cfg");
 legacy("legsmodel mouse/blue");CHECK(get("parts").reason=="missing-legacy-animation-config");
 files.insert("models/players/mouse/animation.cfg");
 legacy("legsmodel mouse/default");CHECK(get("parts").reason.empty()&&get("parts").legacySkins.empty());
 legacy("legsmodel mouse/blue/extra");CHECK(get("parts").reason=="invalid-legacy-model-or-skin-name");
 legacy(("legsmodel "+std::string(35,'a')+"/x").c_str());CHECK(get("parts").reason=="legacy-resource-path-too-long");
 legacy("headmodel ../fixture");CHECK(get("parts").reason=="invalid-legacy-model-or-skin-name");
 legacy("headmodel fixture/blue/extra");CHECK(get("parts").reason=="invalid-legacy-model-or-skin-name");
 legacy("legsmodel none");CHECK(get("parts").reason=="missing-legacy-legs-model");
 // Seeker's missing retail body is resolved identically before native model and
 // animation registration and during Sandbox admission. AI/team/weapon stay native.
 source="seeker\n{\nheadmodel none\ntorsomodel none\nlegsmodel remote\nclass CLASS_SEEKER\nplayerTeam TEAM_PLAYER\nenemyTeam TEAM_ENEMY\nweapon WP_BOT_LASER\nwidth 8\nheight 32\n}\n";
 NPCParms=source.c_str();files.insert("models/players/remote_sp/lower.md3");files.insert("models/players/remote_sp/animation.cfg");
 CHECK(!strcmp(JAR_NpcLegacyLegs("SEEKER","REMOTE"),"remote_sp"));
 G_JAR_ReadNpcCatalog(out,true);const auto& seeker=get("seeker");
 CHECK(seeker.reason.empty()&&seeker.legacyAnimation=="remote_sp"&&seeker.legacyModels.front()=="models/players/remote_sp/lower.md3");
 CHECK(seeker.npcClass==CLASS_SEEKER&&seeker.team==TEAM_PLAYER&&seeker.enemyTeam==TEAM_ENEMY&&seeker.weapon==WP_BOT_LASER&&seeker.width==8&&seeker.height==32);
 G_JAR_ReadNpcCatalog(out);CHECK(get("seeker").reason=="no-Ghoul2-playerModel"); // campaign randomizer eligibility is unchanged
 for(const char* path:{"models/players/remote/lower.md3","models/players/remote/lower.mdr"}){
  files.insert(path);CHECK(!strcmp(JAR_NpcLegacyLegs("seeker","remote"),"remote"));
  G_JAR_ReadNpcCatalog(out,true);CHECK(get("seeker").reason=="missing-legacy-animation-config");
  files.insert("models/players/remote/animation.cfg");G_JAR_ReadNpcCatalog(out,true);CHECK(get("seeker").reason.empty()&&get("seeker").legacyModels.front()==path);
  files.erase("models/players/remote/animation.cfg");files.erase(path);
 }
 for(const char* path:{"models/players/remote_sp/lower.md3","models/players/remote_sp/animation.cfg"}){
  files.erase(path);CHECK(!strcmp(JAR_NpcLegacyLegs("seeker","remote"),"remote"));
  G_JAR_ReadNpcCatalog(out,true);CHECK(!get("seeker").reason.empty());files.insert(path);
 }
 CHECK(!strcmp(JAR_NpcLegacyLegs("remote","remote"),"remote"));
 CHECK(!strcmp(JAR_NpcLegacyLegs("seeker","custom"),"custom"));
 CHECK(!strcmp(JAR_NpcLegacyLegs("seeker","remote/blue"),"remote/blue"));
 CHECK(JAR_NpcLegacyLegs("seeker",nullptr)==nullptr);
 // Remote requests a nonexistent GLM, but can use the stock segmented droid.
 source="remote\n{\nplayerModel remote\nclass CLASS_SEEKER\nplayerTeam TEAM_FREE\nenemyTeam TEAM_FREE\nweapon WP_BOT_LASER\nwidth 8\nheight 32\n}\n";
 NPCParms=source.c_str();G_JAR_ReadNpcCatalog(out,true);
 CHECK(get("remote").reason.empty()&&get("remote").model.empty()&&get("remote").legacyAnimation=="remote_sp");
 CHECK(get("remote").legacyModels.front()=="models/players/remote_sp/lower.md3");
 CHECK(get("remote").npcClass==CLASS_SEEKER&&get("remote").team==TEAM_FREE&&get("remote").enemyTeam==TEAM_FREE&&get("remote").weapon==WP_BOT_LASER);
 G_JAR_ReadNpcCatalog(out);CHECK(get("remote").reason=="missing-Ghoul2-model");
 CHECK(!strcmp(JAR_NpcLegacyBody("REMOTE","REMOTE","DEFAULT"),"remote_sp"));
 renderInfo_t render{};strcpy(render.headModelName,"old");strcpy(render.torsoModelName,"old");render.headYawRangeLeft=80;render.torsoPitchRangeUp=30;
 CHECK(JAR_NpcApplyLegacyBody("remote","remote","default",render));
 CHECK(!render.headModelName[0]&&!render.torsoModelName[0]&&!strcmp(render.legsModelName,"remote_sp")&&!render.headYawRangeLeft&&!render.torsoPitchRangeUp);
 auto unchanged=render;CHECK(!JAR_NpcApplyLegacyBody("remote","custom","default",render)&&!memcmp(&unchanged,&render,sizeof(render)));
 for(const char*part:{"lower.md3","animation.cfg"}){
  const auto path=std::string("models/players/remote_sp/")+part;files.erase(path);
  CHECK(!JAR_NpcLegacyBody("remote","remote","default"));G_JAR_ReadNpcCatalog(out,true);CHECK(!get("remote").reason.empty());files.insert(path);
 }
 files.insert("models/players/remote/model.glm");CHECK(!JAR_NpcLegacyBody("remote","remote","default"));
 G_JAR_ReadNpcCatalog(out,true);CHECK(get("remote").reason.empty()&&get("remote").model=="remote"&&get("remote").legacyModels.empty());
 files.erase("models/players/remote/model.glm");
 CHECK(!JAR_NpcLegacyBody("remote","remote","blue")&&!JAR_NpcLegacyBody("custom","remote","default")&&!JAR_NpcLegacyBody("remote","remote/blue","default"));
 CHECK(!JAR_NpcLegacyBody(nullptr,"remote","default")&&!JAR_NpcLegacyBody("remote",nullptr,"default")&&!JAR_NpcLegacyBody("remote","remote",nullptr));
 // Seeker ammo/gravity must be initialized for neutral Remote as well as the
 // accepted friendly Seeker. No team/weapon/behavior-state mutation or G2 access.
 gentity_t ent{},restored{};gclient_t client{};gNPC_t npc{};ent.client=&client;ent.NPC=&npc;client.NPC_class=CLASS_SEEKER;client.ps.weapon=WP_BOT_LASER;
 for(int team=TEAM_FREE;team<=TEAM_NEUTRAL;++team){
  client.playerTeam=team_t(team);client.enemyTeam=TEAM_FREE;client.ps.gravity=800;ent.count=0;ent.svFlags=SVF_NOCLIENT;npc.behaviorState=BS_CINEMATIC;
  CHECK(JAR_NpcInitSeeker(&ent)&&ent.count==30&&client.ps.gravity==0&&client.moveType==MT_FLYSWIM);
  CHECK((ent.svFlags&(SVF_CUSTOM_GRAVITY|SVF_NOCLIENT))==(SVF_CUSTOM_GRAVITY|SVF_NOCLIENT)&&npc.defaultBehavior==BS_DEFAULT&&npc.behaviorState==BS_CINEMATIC);
  CHECK(client.playerTeam==team&&client.enemyTeam==TEAM_FREE&&client.ps.weapon==WP_BOT_LASER);
 }
 client.NPC_class=CLASS_REMOTE;ent.count=9;client.ps.gravity=37;CHECK(!JAR_NpcInitSeeker(&ent)&&ent.count==9&&client.ps.gravity==37);
 ent.NPC=nullptr;CHECK(!JAR_NpcInitSeeker(&ent));ent.client=nullptr;CHECK(!JAR_NpcInitSeeker(&ent)&&!JAR_NpcInitSeeker(nullptr));
 // The native entity save stream retains remaining ammo and custom gravity flags.
 ent.count=17;MemorySave saved;ojk::SavedGameHelper save(&saved);ent.sg_export(save);saved.reset_buffer_offset();restored.sg_import(save);saved.ensure_all_data_read();CHECK(restored.count==17&&(restored.svFlags&SVF_CUSTOM_GRAVITY));

 });}
