// Inspect the same loaded definition stream and first-definition precedence used
// by NPC_ParseParms. No temporary actor, Ghoul2 instance, or gameplay RNG.
#include "g_npc_resources.inc"
extern int NPC_WeaponsForTeam(team_t team,int spawnflags,const char *name);
#include "g_npc_fields.inc"

static bool JarNpcWeaponAvailable(int weapon){
    for(int i=1;i<bg_numItems&&bg_itemlist[i].classname;++i)if(bg_itemlist[i].giType==IT_WEAPON&&bg_itemlist[i].giTag==weapon)
        return weaponData[weapon].weaponMdl[0]&&gi.FS_ReadFile(weaponData[weapon].weaponMdl,nullptr)>0;
    return false;
}
static bool JarNpcLegacyPart(JarNpcTemplate& e,const std::string& value,const char* part,bool required){
    if(value.empty()||!Q_stricmp(value.c_str(),"none")){if(required)e.reason="missing-legacy-legs-model";return !required;}
    if(value.size()>=MAX_QPATH){e.reason="legacy-resource-path-too-long";return false;}
    char directory[MAX_QPATH];
    const std::string skin=JAR_NpcSplitLegacyModel(value.c_str(),directory),name=directory;
    auto component=[](const std::string& s){return !s.empty()&&std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';});};
    if(!component(name)||!component(skin)){e.reason="invalid-legacy-model-or-skin-name";return false;}
    const std::string base="models/players/"+name+"/"+part;
    const std::string skinPath=base+"_"+skin+".skin";
    if(skinPath.size()>=MAX_QPATH||(required&&name.size()+sizeof("models/players//animevents.cfg")>MAX_QPATH)){
        e.reason="legacy-resource-path-too-long";return false;
    }
    std::string path=base+".md3";
    if(strcmp(part,"head")&&gi.FS_ReadFile((base+".mdr").c_str(),nullptr)>0)path=base+".mdr";
    if(gi.FS_ReadFile(path.c_str(),nullptr)<=0){e.reason="missing-legacy-model";return false;}
    e.legacyModels.push_back(path);
    // The stock lower default may use embedded materials. Explicit custom
    // lower skins and every selected upper/head skin must exist.
    if(gi.FS_ReadFile(skinPath.c_str(),nullptr)>0)e.legacySkins.push_back(skinPath);
    else if(!required||Q_stricmp(skin.c_str(),"default")){e.reason="missing-required-legacy-skin";return false;}
    if(required){e.legacyAnimation=name;if(gi.FS_ReadFile(("models/players/"+name+"/animation.cfg").c_str(),nullptr)<=0){e.reason="missing-legacy-animation-config";return false;}}
    return true;
}
void G_JAR_ReadNpcCatalog(std::vector<JarNpcTemplate>& out,bool allowLegacyModels) {
    out.clear();std::set<std::string> seen;const char *p=NPCParms;
    COM_BeginParseSession();
    while(p){JarNpcTemplate e;std::string skin="default",head,torso,legs;unsigned explicitWeapons=0;const char *token=JarNpcReadToken(p,qtrue);if(!*token)break;
        e.name=token;std::transform(e.name.begin(),e.name.end(),e.name.begin(),[](unsigned char c){return char(std::tolower(c));});
        token=JarNpcReadToken(p,qtrue);
        if(!JarNpcIsBrace(token,p,'{'))break;
        bool closed=false;
        while(p){std::string key=JarNpcReadToken(p,qtrue);if(key.empty())break;
            if(JarNpcIsBrace(key.c_str(),p,'}')){closed=true;break;}
            if(!JarNpcHasSingleValue(key.c_str())){
                JarNpcSkipField(key.c_str(),p);
                continue;
            }
            const char *value=JarNpcReadValue(p);if(!value)continue;
            if(!Q_stricmp(key.c_str(),"playerModel"))e.model=value;
            else if(!Q_stricmp(key.c_str(),"headmodel"))head=value;
            else if(!Q_stricmp(key.c_str(),"torsomodel"))torso=value;
            else if(!Q_stricmp(key.c_str(),"legsmodel"))legs=value;
            else if(!Q_stricmp(key.c_str(),"customSkin"))skin=value;
            else if(!Q_stricmp(key.c_str(),"class"))e.npcClass=GetIDForString(ClassTable,value);
            else if(!Q_stricmp(key.c_str(),"playerTeam"))e.team=GetIDForString(TeamTable,value);
            else if(!Q_stricmp(key.c_str(),"enemyTeam"))e.enemyTeam=GetIDForString(TeamTable,value);
            else if(!Q_stricmp(key.c_str(),"weapon")){const int weapon=GetIDForString(WPTable,value);if(weapon>=WP_NONE&&weapon<WP_NUM_WEAPONS){e.weapon=weapon;if(weapon>WP_NONE)explicitWeapons|=1u<<weapon;}}
            else if(!Q_stricmp(key.c_str(),"width"))e.width=atoi(value);
            else if(!Q_stricmp(key.c_str(),"height"))e.height=atoi(value);
        }
        if(!closed)break;if(!seen.insert(e.name).second)continue;
        if(allowLegacyModels){if(const char*body=JAR_NpcLegacyBody(e.name.c_str(),e.model.c_str(),skin.c_str())){e.model.clear();legs=body;head.clear();torso.clear();}}
        if(e.npcClass==CLASS_VEHICLE)e.reason="vehicle-requires-dedicated-spawner";
        else if(e.name=="player"||e.name=="test")e.reason="player-template-has-special-spawn-semantics";
        else if(e.npcClass<=CLASS_NONE||e.npcClass>=CLASS_NUM_CLASSES)e.reason="missing-or-invalid-AI-class";
        else if(e.team<TEAM_FREE||e.team>TEAM_NEUTRAL||e.enemyTeam<TEAM_FREE||e.enemyTeam>TEAM_NEUTRAL)e.reason="invalid-allegiance";
        else if(e.width<=0||e.width>512||e.height<=0||e.height>1024)e.reason="invalid-template-bounds";
        else if(e.model.empty()){
            if(!allowLegacyModels)e.reason="no-Ghoul2-playerModel";
            else if(JarNpcLegacyPart(e,JAR_NpcLegacyLegs(e.name.c_str(),legs.c_str()),"lower",true)&&JarNpcLegacyPart(e,torso,"upper",false))JarNpcLegacyPart(e,head,"head",false);
        }
        else {const std::string path="models/players/"+e.model+"/model.glm";
            if(gi.FS_ReadFile(path.c_str(),nullptr)<=0)e.reason="missing-Ghoul2-model";
            // A default skin is optional: native Ghoul2 can use embedded GLM materials
            // (stock sentry has no model_default.skin). Explicit custom skins are required.
            else if(skin!="default"&&skin.find('|')==std::string::npos){if(gi.FS_ReadFile(("models/players/"+e.model+"/model_"+skin+".skin").c_str(),nullptr)<=0)e.reason="missing-required-skin";}
            else if(skin.find('|')!=std::string::npos) {unsigned parts=0;size_t begin=0;for(;;){const size_t end=skin.find('|',begin);const std::string part=skin.substr(begin,end==std::string::npos?end:end-begin);++parts;
                    if(part.empty()||gi.FS_ReadFile(("models/players/"+e.model+"/"+part+".skin").c_str(),nullptr)<=0)e.reason="missing-required-part-skin";
                    if(end==std::string::npos)break;begin=end+1;}
                if(parts!=3)e.reason="invalid-three-part-skin";}
        }
        if(e.reason.empty()){
            // NPC_ParseParms registers every explicit weapon, not only the last
            // selected one. Native fallback loadouts can also register resources.
            unsigned weapons=explicitWeapons|unsigned(NPC_WeaponsForTeam(team_t(e.team),0,e.name.c_str()));
            // Identity restoration precedes NPC_Begin's fallback selection. A
            // template with no selected weapon may therefore inherit any source
            // team's native loadout; validate that whole union before admission.
            if(e.weapon==WP_NONE)for(int team=TEAM_FREE;team<=TEAM_NEUTRAL;++team)weapons|=unsigned(NPC_WeaponsForTeam(team_t(team),0,e.name.c_str()));
            for(int weapon=WP_NONE+1;weapon<WP_NUM_WEAPONS;++weapon)if((weapons&(1u<<weapon))&&!JarNpcWeaponAvailable(weapon)){e.reason="missing-NPC-weapon-item-or-render-model";break;}
        }
        out.push_back(std::move(e));
    }
    COM_EndParseSession();std::sort(out.begin(),out.end(),[](const JarNpcTemplate& a,const JarNpcTemplate& b){return a.name<b.name;});
}
