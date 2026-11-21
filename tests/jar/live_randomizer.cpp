#include "qcommon/qcommon.h"
#include "qcommon/jar_live_randomizer.h"
#include "qcommon/jar_sound_randomizer.h"
#include "test_support.h"
#include <map>
#include <string>
#include <set>
#include <vector>
#include <cstdlib>

struct Variable { std::string value; cvar_t cvar = {}; };
static std::map<std::string,Variable> vars;
static std::map<std::string,xcommand_t> commands;
cvar_t *Cvar_FindVar(const char *name) { auto it=vars.find(name);return it==vars.end()?nullptr:&it->second.cvar; }
cvar_t *Cvar_Get(const char *name,const char *initial,int flags) {
    auto result=vars.emplace(name,Variable{});auto& v=result.first->second;
    if (result.second) v.value=initial;
    v.cvar.string=const_cast<char *>(v.value.c_str());v.cvar.flags|=flags;return &v.cvar;
}
void Cvar_Set(const char *name,const char *value) {
    const std::string copy=value;auto *cvar=Cvar_Get(name,copy.c_str(),0);auto& v=vars[name];v.value=copy;cvar->string=const_cast<char *>(v.value.c_str());
}
char *Cvar_VariableString(const char *name) { auto *v=Cvar_FindVar(name);return v?v->string:const_cast<char *>(""); }
void Cmd_AddCommand(const char *name,xcommand_t command) { commands[name]=command; }
static std::string Value(const char *name) { return Cvar_VariableString(name); }
static std::map<std::string,std::string> Save() {
    JAR_RandomizerBeforeSave();std::map<std::string,std::string> saved;
    for (const auto& pair:vars) if(pair.second.cvar.flags&CVAR_SAVEGAME)saved[pair.first]=pair.second.value;
    return saved;
}
static void Load(const std::map<std::string,std::string>& saved) {
    JAR_RandomizerBeforeLoad();
    for (const auto& pair:saved)Cvar_Set(pair.first.c_str(),pair.second.c_str());
    JAR_RandomizerReadComplete();
    // Native normal-map initialization resets speed (helpUsObi=0).
    Cvar_Set("g_speed","250");
    JAR_RandomizerResumeAfterLoad();
}
int main() {
 return RunTests("live cvars/audio: seed stability, bounded values, ownership, save/config isolation, safe restoration",[] {
    float color[4]={};CHECK(!jar::SceneBrightnessOverlay(1,color));
    CHECK(jar::SceneBrightnessOverlay(0.85f,color) && color[0]==0 && color[3]>0.149f && color[3]<0.151f);
    CHECK(jar::SceneBrightnessOverlay(1.25f,color) && color[0]==1 && color[3]==0.25f);
    CHECK(jar::SceneBrightnessOverlay(900,color) && color[3]==0.5f);
    JAR_LiveInit();CHECK(Value("jar_randomMode")=="0");
    // A fresh process can immediately resave a legacy-mode save before firing.
    Load({{"jar_randomMode","1"},{"jar_projectileSequenceSeed","73"}});
    auto immediate=Save();CHECK(immediate.at("jar_projectileSequenceSeed")=="73");
    vars.clear();JAR_LiveInit();Load(immediate);CHECK(Save().at("jar_projectileSequenceSeed")=="73");
    Load({});Cvar_Set("jar_randomSeed","73");Cvar_Set("mapname","t1_surprise");
    for (const auto& s:jar::liveSettings)CHECK(!(Cvar_FindVar(s.target)->flags&CVAR_SAVEGAME));
    CHECK(commands.size()==3 && !commands.count("vid_restart") && !commands.count("snd_restart"));
    Cvar_FindVar("jar_sceneBrightness")->flags|=CVAR_ARCHIVE;Cvar_FindVar("cg_fov")->flags|=CVAR_ARCHIVE;
    auto ordinary=Save();Cvar_Set("jar_sceneBrightness","1.3");Cvar_Set("cg_fov","100");Load(ordinary);
    CHECK(Value("jar_sceneBrightness")=="1.3" && Value("cg_fov")=="100");
    for(const auto& s:jar::liveSettings)Cvar_Set(s.toggle,"1");
    JAR_LiveApply();CHECK(Value("g_gravity")=="800"); // no mission: no mutation
    Cvar_Set("sv_running","1");JAR_LiveApply();
    std::vector<std::string> first;
    for(const auto& s:jar::liveSettings)first.push_back(Value(s.target));
    JAR_LiveApply();
    for(unsigned i=0;i<jar::liveSettingCount;++i)CHECK(first[i]==Value(jar::liveSettings[i].target));
    Cvar_Set("mapname","different_map");JAR_LiveApply();
    for(unsigned i=0;i<jar::liveSettingCount;++i)CHECK(first[i]==Value(jar::liveSettings[i].target));
    CHECK(Value("jar_liveSequence")=="0");
    CHECK(std::string(JAR_LiveConfigValue("jar_sceneBrightness",Cvar_VariableString("jar_sceneBrightness")))=="1.3");
    CHECK(std::string(JAR_LiveConfigValue("cg_fov",Cvar_VariableString("cg_fov")))=="100");
    auto active=Save();CHECK(!active.count("jar_sceneBrightness") && active.count("jar_live2_last"));
    JAR_LiveRestore();CHECK(Value("g_gravity")=="800" && Value("g_speed")=="250" && Value("jar_sceneBrightness")=="1.3");
    Cvar_Set("jar_sceneBrightness","1.4");Load(active);
    CHECK(Value("g_speed")==first[1] && Value("jar_sceneBrightness")==first[2]);
    CHECK(std::string(JAR_LiveConfigValue("jar_sceneBrightness",Cvar_VariableString("jar_sceneBrightness")))=="1.4");
    Cvar_Set("g_speed","275");JAR_RandomizerResumeAfterLoad();CHECK(Value("g_speed")=="275"); // consume once
    Cvar_Set("jar_sceneBrightness","1.5");auto released=Save();CHECK(released["jar_live2_base"].empty());
    JAR_LiveRestore();CHECK(Value("jar_sceneBrightness")=="1.5" && Value("g_speed")=="275");
    Load(released);CHECK(Value("jar_sceneBrightness")=="1.5" && Value("g_speed")=="250");
    Cvar_Set("jar_audioSequence","39");Cvar_Set("jar_audioSequenceSeed","73");Cvar_Set("jar_projectileSequence","89");Cvar_Set("jar_npcSafety","0");Cvar_Set("jar_actorContinuity","1");
    Load({});CHECK(Value("jar_audioSequence")=="0"&&Value("jar_audioSequenceSeed").empty()&&Value("jar_projectileSequence")=="0"&&Value("jar_npcSafety")=="1"&&Value("jar_actorContinuity")=="0");
    CHECK(Value("jar_randomMode")=="0"&&Value("jar_worldMode")=="1");
    Cvar_Set("jar_audioSequence","93");Cvar_Set("jar_audioSequenceSeed","73");Load({});CHECK(Value("jar_audioSequence")=="0");
    for(const auto& s:jar::liveSettings)CHECK(Value(s.toggle)=="0");
    CHECK(Value("jar_worldObjects")=="0" && Value("jar_randomMusic")=="0");
    Cvar_Set("g_speed","250");Cvar_Set("g_gravity","600");Cvar_Set("jar_sceneBrightness","1.5");
    std::set<std::string> speeds,gammas;
    for(int seed=0;seed<100;++seed) {
        Cvar_Set("jar_randomSeed",std::to_string(seed).c_str());
        for(const auto& s:jar::liveSettings)Cvar_Set(s.toggle,"1");
        JAR_LiveApply();
        CHECK(std::atof(Cvar_VariableString("g_gravity"))>=450 && std::atof(Cvar_VariableString("g_gravity"))<=600);
        CHECK(std::atof(Cvar_VariableString("g_speed"))>=250 && std::atof(Cvar_VariableString("g_speed"))<=313);
        CHECK(std::atof(Cvar_VariableString("jar_sceneBrightness"))>=0.85 && std::atof(Cvar_VariableString("jar_sceneBrightness"))<=1.25);
        speeds.insert(Value("g_speed"));gammas.insert(Value("jar_sceneBrightness"));JAR_LiveRestore();
    }
    CHECK(speeds.size()>30 && gammas.size()>40);
    Cvar_Set("jar_randomMode","1");Cvar_Set("jar_randomSeed","73");Cvar_Set("jar_randomFov","1");
    std::set<std::string> rerolled;for(int i=0;i<30;++i){JAR_LiveApply();rerolled.insert(Value("cg_fov"));}CHECK(rerolled.size()>8);
    const auto rerollSave=Save();const auto currentFov=Value("cg_fov");JAR_LiveApply();const auto nextFov=Value("cg_fov");
    Load(rerollSave);CHECK(Value("jar_randomMode")=="1"&&Value("cg_fov")==currentFov);JAR_LiveApply();CHECK(Value("cg_fov")==nextFov);
    Cvar_Set("jar_randomMode","0");JAR_LiveApply();const auto fixedFov=Value("cg_fov");JAR_LiveApply();CHECK(Value("cg_fov")==fixedFov);
    Cvar_Set("jar_randomMode","bad");JAR_LiveApply();CHECK(Value("cg_fov")==fixedFov);Cvar_Set("jar_randomMode","0");JAR_LiveRestore();
    Cvar_Set("jar_randomBrightness","1");Cvar_Set("jar_randomSeed","invalid");JAR_LiveApply();CHECK(Value("jar_sceneBrightness")=="1.5");
    Cvar_Set("jar_randomSeed","73");Cvar_Set("jar_randomSky","2");JAR_LiveApply();CHECK(Value("jar_sceneBrightness")=="1.5");
    Cvar_Set("jar_randomSky","0");Cvar_FindVar("jar_sceneBrightness")->flags|=CVAR_LATCH;JAR_LiveApply();CHECK(Value("jar_sceneBrightness")=="1.5");
    Cvar_FindVar("jar_sceneBrightness")->flags&=~CVAR_LATCH;Cvar_Set("jar_sceneBrightness","nan");JAR_LiveApply();CHECK(Value("jar_sceneBrightness")=="nan");

 });
}
