// Portable section content admission. No registration or entity side effects.
// GPL-2.0-only. Include after the native q_shared/qfiles and spawn capacities.
#ifndef G_SUBBSP_ADMISSION_H
#define G_SUBBSP_ADMISSION_H
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>
#include "../../shared/qcommon/jar_bsp_section_policy.h"
#include "g_bsp_section_fx.h"
#include "g_bsp_section_shaft.h"
#include "g_bsp_section_pickup_specs.h"
#include "g_bsp_section_wasteland.h"
#include "g_siege_korriban.h"
#include "g_bsp_section_box.h"
namespace jar { namespace bspAdmission {
constexpr size_t maxFileBytes=64*1024*1024;
constexpr int maxInlineModels=512; // Native inline model handles have nine bits.
using bspSection::cityBreakables;
using bspSection::yavinBreakables;
struct Field {std::string key,value;};
using Entity=std::vector<Field>;
struct Report {
    int models=0,entities=0,allocationBudget=1,rejectedEntity=0,entryPoints=0,ownedHelpers=0,redundantSoundKeys=0;
    vec3_t mins{},maxs{},hurtMins{},hurtMaxs{};
    vec3_t padMins[2]{},padMaxs[2]{};
    std::string reason;
    std::vector<std::string> resources;
    std::vector<std::string> pickupClasses;
};
inline bool Fail(Report&r,const std::string&why){r.reason=why;return false;}
inline std::string Lower(std::string s){for(char&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline const char* Value(const Entity&e,const char*key){for(const auto&f:e)if(f.key==key)return f.value.c_str();return "";}
inline bool OneOf(const std::string&s,std::initializer_list<const char*> names){for(auto n:names)if(s==n)return true;return false;}
// Accept native whitespace/comments/quotes, but reject truncated strings/tokens
// instead of letting COM_Parse silently truncate them. Braces must be tokens,
// just as they are in the native whitespace-delimited parser.
class Reader {
    const char *at,*end;
public:
    Reader(const char*p,size_t n):at(p),end(p+n){}
    bool Next(std::string&token,bool&eof,Report&r){
        token.clear();eof=false;
        for(;;){
            while(at<end&&static_cast<unsigned char>(*at)<=32)++at;
            if(at==end){eof=true;return true;}
            if(end-at>=2&&at[0]=='/'&&at[1]=='/'){
                at+=2;while(at<end&&*at!='\n')++at;continue;
            }
            if(end-at>=2&&at[0]=='/'&&at[1]=='*'){
                at+=2;while(end-at>=2&&(at[0]!='*'||at[1]!='/'))++at;
                if(end-at<2)return Fail(r,"Unterminated entity comment");
                at+=2;continue;
            }
            break;
        }
        if(*at=='"'){
            const char*start=++at;while(at<end&&*at!='"')++at;
            if(at==end)return Fail(r,"Unterminated entity string");
            token.assign(start,at++);
        }else{
            const char*start=at;while(at<end&&static_cast<unsigned char>(*at)>32)++at;
            token.assign(start,at);
        }
        if(token.size()>=MAX_TOKEN_CHARS)return Fail(r,"Entity token exceeds native parser capacity");
        return true;
    }
};
inline bool Number(const char*s,double lo,double hi){
    if(!*s)return false;char*end;const double n=std::strtod(s,&end);if(end==s)return false;
    while(*end&&static_cast<unsigned char>(*end)<=32)++end;
    return !*end&&std::isfinite(n)&&n>=lo&&n<=hi;
}
inline bool Vector(const char*s){
    for(int i=0;i<3;++i){char*end;const double n=std::strtod(s,&end);
        if(end==s||!std::isfinite(n)||std::fabs(n)>65536)return false;s=end;}
    while(*s&&static_cast<unsigned char>(*s)<=32)++s;return !*s;
}
inline bool Resource(const std::string&s){
    if(s.empty()||s.size()>=MAX_QPATH||s.front()=='/'||s.back()=='/'||s.find("..")!=s.npos||s.find("//")!=s.npos)return false;
    for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='/'||c=='.'))return false;
    return true;
}
inline bool YavinContent(const std::vector<Entity>& entities,Report& r){
    using namespace bspSectionFX;
    if(r.models!=yavinBreakables+1||entities.size()!=324||strcmp(Value(entities[0],"classname"),"worldspawn"))
        return Fail(r,"Yavin needs all stock entities and inline models");
    unsigned brushMask=0;
    int scenery=0,fires=0,dustEmitters=0,timers=0,randoms=0,sky=0;
    std::vector<std::string> aimPoints,aimLinks;
    for(size_t i=1;i<entities.size();++i){
        r.rejectedEntity=int(i)+1;
        const auto& e=entities[i];
        const std::string cls=Value(e,"classname"),name=Value(e,"targetname"),target=Value(e,"target");
        const char* origin=Value(e,"origin");
        if(cls=="func_breakable"){
            const char* model=Value(e,"model");
            if(e.size()!=4||strcmp(Value(e,"spawnflags"),"2064")||!OneOf(Value(e,"health"),{"2","3","4","5","6","8","10"})||
               *model!='*'||!Number(model+1,1,yavinBreakables)||strspn(model+1,"0123456789")!=strlen(model+1))
                return Fail(r,"Yavin needs its stock saber-only breakable brushes");
            const unsigned bit=1u<<(atoi(model+1)-1);
            if(brushMask&bit)return Fail(r,"Yavin repeats a breakable inline model");
            brushMask|=bit;
            continue;
        }
        if(!Vector(origin))return Fail(r,cls=="misc_skyportal"?"Yavin sky camera requires only a local origin":"Yavin entity requires a local origin");
        if(cls=="fx_runner"){
            const bool fire=!strcmp(Value(e,"fxfile"),effect);
            if(target.empty()||(fire?(e.size()!=6||!name.empty()||strcmp(Value(e,"spawnflags"),"4")||strcmp(Value(e,"soundset"),soundSet)):
                 (e.size()!=6||name!="t562"||strcmp(Value(e,"spawnflags"),"2")||strcmp(Value(e,"fxfile"),dust))))
                return Fail(r,"Yavin effect requires its stock local aim and fire or dust settings");
            aimLinks.push_back(target);
            if(fire)++fires;else ++dustEmitters;
        }else if(cls=="func_timer"){
            if(e.size()!=6||target!="t533"||strcmp(Value(e,"spawnflags"),"1")||strcmp(Value(e,"wait"),"5")||strcmp(Value(e,"random"),"3"))
                return Fail(r,"Yavin timer requires its stock delay and local random output");
            ++timers;
        }else if(cls=="target_random"){
            if(e.size()!=4||name!="t533"||target!="t562")return Fail(r,"Yavin random output requires its local dust emitters");
            ++randoms;
        }else if(cls=="misc_model_static"){
            const std::string model=Value(e,"model");
            if(!OneOf(model,{"models/map_objects/yavin/fern3_b.md3","models/map_objects/yavin/tree09_b.md3",
                             "models/map_objects/yavin/grass_tall_b.md3","models/map_objects/desert/rock.md3",
                             "models/map_objects/yavin/tree02_b.md3","models/map_objects/yavin/tree05_b.md3"}))
                return Fail(r,"Yavin requires its stock scenery models");
            for(const auto& f:e){
                if(OneOf(f.key,{"classname","model","origin","_cs","_rs"}))continue;
                if(f.key=="angles"&&Vector(f.value.c_str()))continue;
                if(f.key=="angle"&&Number(f.value.c_str(),-360,360))continue;
                if(f.key=="zoffset"&&Number(f.value.c_str(),0,32))continue;
                if(f.key=="modelscale"&&Number(f.value.c_str(),.5,2))continue;
                return Fail(r,"Yavin scenery placement is unsupported");
            }
            r.resources.push_back(model);
            ++scenery;
        }else if(bspSection::Position(cls.c_str())){
            const bool intermission=cls=="info_player_intermission";
            if(e.size()!=(intermission?4u:3u)||(intermission&&target!="v1")||!Number(Value(e,"angle"),-360,360))
                return Fail(r,"Yavin entry needs its stock placement");
            if(bspSection::Start(cls.c_str()))++r.entryPoints;
        }else if(cls=="misc_skyportal"){
            if(e.size()!=2)return Fail(r,"Yavin sky camera requires only a local origin");
            ++sky;
        }else if(cls=="info_notnull"||cls=="info_null"){
            if(e.size()!=3||name.empty()||name=="t533"||name=="t562")return Fail(r,"Yavin marker requires a distinct local name");
            if(cls=="info_notnull")aimPoints.push_back(Lower(name));
        }else return Fail(r,"Portable Yavin adapter is pending for "+cls);
    }
    for(const auto& aim:aimLinks)
        if(std::count(aimPoints.begin(),aimPoints.end(),Lower(aim))!=1)return Fail(r,"Yavin emitter aim must resolve to one local marker");
    if(brushMask!=(1u<<yavinBreakables)-1||scenery!=51||fires!=16||dustEmitters!=74||timers!=1||randoms!=1||sky!=1||r.entryPoints!=16)
        return Fail(r,"Yavin requires all stock brushes, scenery, emitters, logic and starts");
    for(const char* resource:{effectFile,loop,dustFile,"sound/weapons/explosions/cargoexplode.wav","effects/chunks/metalexplode.efx"})
        r.resources.push_back(resource);
    r.allocationBudget=int(entities.size());
    r.rejectedEntity=0;
    return true;
}
#include "g_subbsp_taspir_admission.inc"
#include "g_subbsp_factory_admission.inc"
#include "g_subbsp_ctf_admission.inc"
#include "g_subbsp_coruscant_admission.inc"
#include "g_subbsp_hilltops_admission.inc"
#include "g_subbsp_dropzone_admission.inc"
#include "g_subbsp_wasteland_admission.inc"
#include "g_subbsp_siege_korriban_admission.inc"
inline bool Content(const std::vector<Entity>&entities,Report&r,bspSection::Mode mode){
    if(mode==bspSection::Mode::Taspir)return TaspirContent(entities,r);
    if(mode==bspSection::Mode::Factory)return FactoryContent(entities,r);
    if(mode==bspSection::Mode::Coruscant)return CoruscantContent(entities,r);
    if(mode==bspSection::Mode::Hilltops)return HilltopsContent(entities,r);
    if(mode==bspSection::Mode::DropZone)return DropZoneContent(entities,r);
    if(mode==bspSection::Mode::Wasteland)return WastelandContent(entities,r);
    if(mode==bspSection::Mode::SiegeKorriban)return SiegeKorribanContent(entities,r);
    if(mode==bspSection::Mode::Yavin)return YavinContent(entities,r);
    const bool sectionProfile=mode!=bspSection::Mode::Generic;
    const bool korribanRoom=mode==bspSection::Mode::Korriban;
    const bool sentinelRoom=mode==bspSection::Mode::Sentinel;
    const bool tatooineRoom=mode==bspSection::Mode::Tatooine;
    const auto* lifts=sentinelRoom?sentinel::lifts:tatooine::lifts;
    const auto* liftEnd=lifts+(sentinelRoom?ARRAY_LEN(sentinel::lifts):ARRAY_LEN(tatooine::lifts));
    const auto* pads=sentinelRoom?sentinel::pads:tatooine::pads;
    const auto* padEnd=pads+(sentinelRoom?ARRAY_LEN(sentinel::pads):ARRAY_LEN(tatooine::pads));
    size_t pickupKinds;
    const auto* pickupSpecs=bspSection::PickupSpecs(mode,pickupKinds);
    const bool stockPickups=pickupSpecs!=nullptr;
    int bobs=0,fires=0,hurts=0,citySpeakers=0;
    bool cityModels[cityBreakables+1]{};
    int cityHealth[4]{};
    unsigned shaftLifts=0,shaftPads=0;
    int shaftScenery[3]{};
    std::vector<int> pickupCounts(pickupKinds);
    unsigned fallBrushes=0;
    unsigned liftMask=0,padMask=0;
    int sentinelSpeakers=0;
    for(size_t i=0;i<entities.size();++i){
        const auto&e=entities[i];r.rejectedEntity=int(i)+1;
        const std::string cls=Value(e,"classname");
        for(const auto&f:e)if(f.key.size()>=6&&f.key.compare(f.key.size()-6,6,"script")==0&&!f.value.empty())
            return Fail(r,"Script isolation is pending: "+f.key);
        if(i==0){if(cls!="worldspawn")return Fail(r,"First entity must be worldspawn");continue;}
        if(stockPickups){
            if(cls=="trigger_hurt"&&!tatooineRoom){
                const std::string model=Value(e,"model");
                const int count=korribanRoom?int(ARRAY_LEN(korriban::fallBoxes)):2;
                const bool validModel=sentinelRoom?model=="*5":model.size()==2&&model[0]=='*'&&model[1]>='1'&&model[1]<='0'+count;
                if(e.size()!=3||strcmp(Value(e,"dmg"),"-1")||!validModel)
                    return Fail(r,"Section requires its stock fall triggers");
                const unsigned bit=1u<<(model[1]-'1');
                if(fallBrushes&bit)return Fail(r,"Section repeats a fall trigger");
                fallBrushes|=bit;
                ++r.allocationBudget;
                continue;
            }
            const auto* pickup=std::find_if(pickupSpecs,pickupSpecs+pickupKinds,
                [&](const rift::Pickup& spec){return cls==spec.name;});
            if(pickup!=pickupSpecs+pickupKinds){
                const char* origin=Value(e,"origin");
                const char* angle=Value(e,"angle");
                if(!*origin||!Vector(origin)||e.size()!=size_t(*angle?3:2)||
                   (*angle&&!Number(angle,-360,360)))
                    return Fail(r,"Section pickup has an unsupported placement");
                ++pickupCounts[pickup-pickupSpecs];
                ++r.allocationBudget;
                continue;
            }
        }
        if(tatooineRoom&&cls=="func_plat"){
            if(r.ownedHelpers||e.size()!=6||strcmp(Value(e,"model"),"*1")||strcmp(Value(e,"height"),"296")||
               strcmp(Value(e,"lip"),"8")||strcmp(Value(e,"speed"),"250")||strcmp(Value(e,"dmg"),"5000"))
                return Fail(r,"Tatooine requires the stock automatic platform and travel");
            ++r.ownedHelpers;r.allocationBudget+=2;continue;
        }
        if((sentinelRoom||tatooineRoom)&&cls=="func_door"){
            const std::string model=Value(e,"model");
            const auto* spec=std::find_if(lifts,liftEnd,
                [&](const sentinel::Lift& lift){return model=="*"+std::to_string(lift.model);});
            if(spec==liftEnd||strcmp(Value(e,"targetname"),pads[spec->group].target)||
               strcmp(Value(e,"angle"),"-2")||strcmp(Value(e,"linear"),"1")||strcmp(Value(e,"speed"),"500")||
               strcmp(Value(e,"wait"),"3")||strcmp(Value(e,"delay"),"0.2")||atoi(Value(e,"spawnflags"))!=spec->flags||
               strcmp(Value(e,"soundset"),spec->sound)||strcmp(Value(e,"lip"),spec->lip)||
               strcmp(Value(e,"dmg"),spec->flags==5?"5000":"")||
               e.size()!=9u+bool(*spec->lip)+bool(*spec->sound)+(spec->flags==5))
                return Fail(r,"Section lift requires its stock travel, timing and local group");
            const unsigned bit=1u<<(spec-lifts);
            if(liftMask&bit)return Fail(r,"Section repeats a lift model");
            liftMask|=bit;++r.allocationBudget;continue;
        }
        if((sentinelRoom||tatooineRoom)&&cls=="trigger_multiple"){
            const std::string model=Value(e,"model");
            const auto* spec=std::find_if(pads,padEnd,
                [&](const sentinel::Pad& pad){return model=="*"+std::to_string(pad.model);});
            if(spec==padEnd||e.size()!=5||strcmp(Value(e,"target"),spec->target)||
               strcmp(Value(e,"delay"),"0.2")||Value(e,"wait")!=std::to_string(spec->wait))
                return Fail(r,"Section floor trigger requires its stock timing and local lift group");
            const unsigned bit=1u<<(spec-pads);
            if(padMask&bit)return Fail(r,"Section repeats a floor trigger model");
            padMask|=bit;++r.allocationBudget;continue;
        }
        if(mode==bspSection::Mode::Shaft && cls=="misc_model_static"){
            const std::string model=Value(e,"model");
            const char* origin=Value(e,"origin");
            int index=-1;
            for(int n=0;n<3;++n)if(model==shaft::sceneryModels[n])index=n;
            if(index<0||e.size()!=5||strcmp(Value(e,"_cs"),"1")||!*origin||!Vector(origin))
                return Fail(r,"Imperial Shaft needs its stock scenery models");
            if((index==0&&!OneOf(Value(e,"angles"),{"0 90 0","0 180 0","0 270 0","0 360 0"}))||
               (index==1&&strcmp(Value(e,"modelscale"),"0.76"))||
               (index==2&&strcmp(Value(e,"modelscale_vec"),"1 1 0.5")))
                return Fail(r,"Imperial Shaft scenery pose is unsupported");
            ++shaftScenery[index];
            r.resources.push_back(model);
            ++r.allocationBudget;
            continue;
        }
        if(mode==bspSection::Mode::Shaft && (cls=="func_door"||cls=="trigger_multiple")){
            const bool lift=cls=="func_door";
            const std::string model=Value(e,"model");
            const int index=model==(lift?"*2":"*1")?0:model==(lift?"*3":"*4")?1:-1;
            if(index<0||strcmp(Value(e,lift?"targetname":"target"),index==0?"t29":"dplat1")||
               strcmp(Value(e,"delay"),"0.2")||strcmp(Value(e,"wait"),lift?"3":"4"))
                return Fail(r,"Imperial Shaft lift and floor trigger must use their matching local target");
            if(lift?(e.size()!=10||strcmp(Value(e,"spawnflags"),"1")||strcmp(Value(e,"linear"),"1")||
                      strcmp(Value(e,"speed"),"500")||strcmp(Value(e,"angle"),"-2")||
                      strcmp(Value(e,"soundset"),"medium_platform")):e.size()!=5)
                return Fail(r,"Imperial Shaft requires the stock lift and floor trigger timing");
            auto& mask=lift?shaftLifts:shaftPads;
            if(mask&(1u<<index))return Fail(r,"Imperial Shaft repeats an inline lift or trigger");
            mask|=1u<<index;
            ++r.allocationBudget;
            continue;
        }
        if(mode==bspSection::Mode::Hurt&&cls=="misc_skyportal"){
            // Hoth Canyon retains its authored camera location as a passive
            // point. Do not admit sky settings that would change the host view.
            const char* origin=Value(e,"origin");
            if(e.size()!=2||!*origin||!Vector(origin))
                return Fail(r,"Section sky camera requires only a local origin");
            ++r.allocationBudget;
            continue;
        }
        if(mode==bspSection::Mode::Breakable&&cls=="func_breakable"){
            const char* model=Value(e,"model");
            const char* health=Value(e,"health");
            if(e.size()!=6||strcmp(Value(e,"spawnflags"),"2048")||strcmp(Value(e,"material"),"5")||
               strcmp(Value(e,"soundset"),"stoneblocks")||!OneOf(health,{"1","2","3"}))
                return Fail(r,"Abandoned City block requires its stock health, flags, material and sound set");
            if(*model!='*'||model[1]<'1'||model[1]>'9'||!Number(model+1,1,cityBreakables)||
               strspn(model+1,"0123456789")!=strlen(model+1))
                return Fail(r,"Abandoned City block requires a stock local inline model");
            const int id=atoi(model+1);
            if(cityModels[id])return Fail(r,"Abandoned City block model is assigned more than once");
            cityModels[id]=true;
            ++cityHealth[health[0]-'0'];
            ++r.allocationBudget;
            continue;
        }
        const bool mover=OneOf(cls,{"func_door","func_button","func_plat","func_bobbing","func_rotating","func_pendulum"});
        const bool point=sectionProfile&&bspSection::Position(cls.c_str());
        const bool speaker=sectionProfile&&cls=="target_speaker";
        const bool fire=(mode==bspSection::Mode::Fire||korribanRoom)&&cls=="fx_runner";
        const bool hurt=(mode==bspSection::Mode::Hurt||mode==bspSection::Mode::Shaft)&&cls=="trigger_hurt";
        if(mover&&sectionProfile&&(mode!=bspSection::Mode::Bobbing||cls!="func_bobbing"))return Fail(r,"Section mover adapter is pending: "+cls);
        if(!mover&&!point&&!speaker&&!fire&&!hurt&&!OneOf(cls,{"info_null","info_notnull","target_position"}))
            return Fail(r,"Portable adapter is pending for "+(cls.empty()?std::string("an unnamed entity"):cls));
        if(point&&bspSection::Start(cls.c_str()))++r.entryPoints;
        size_t chars=0;
        for(const auto&f:e){
            chars+=f.key.size()+f.value.size()+2;
            if(OneOf(f.key,{"classname","targetname"}))continue;
            if(point&&cls=="info_player_intermission"&&f.key=="target")continue;
            if(OneOf(f.key,{"origin","angles"})||(mover&&f.key=="modelangles")){
                if(!Vector(f.value.c_str()))return Fail(r,"Invalid section vector: "+f.key);continue;
            }
            if(f.key=="angle"){
                if(!Number(f.value.c_str(),-65536,65536))return Fail(r,"Invalid section angle");continue;
            }
            if(f.key=="spawnflags"){
                if(!Number(f.value.c_str(),0,65535)||f.value.find_first_not_of("0123456789")!=f.value.npos)return Fail(r,"Invalid section spawnflags");continue;
            }
            if(fire&&OneOf(f.key,{"fxfile","soundset"}))continue;
            if(hurt&&OneOf(f.key,{"model","dmg","wait"}))continue;
            if(speaker&&f.key=="noise"){
                if(!Resource(f.value)||f.value.compare(0,6,"sound/")||f.value.size()<4||
                    !OneOf(Lower(f.value.substr(f.value.size()-4)),{".wav",".mp3"}))return Fail(r,"Speaker requires a local sound resource");
                r.resources.push_back(f.value);continue;
            }
            if(speaker&&OneOf(f.key,{"wait","random"})){
                if(!Number(f.value.c_str(),0,3600))return Fail(r,"Invalid local speaker interval");continue;
            }
            if(mover&&f.key=="model")continue;
            if(mover&&f.key=="model2"){
                if(!Resource(f.value)||f.value.size()<4||Lower(f.value.substr(f.value.size()-4))!=".md3")return Fail(r,"Secondary model needs a portable MD3 resource");
                r.resources.push_back(f.value);continue;
            }
            if(mover&&OneOf(f.key,{"speed","height","lip","phase","dmg","health","wait","delay"})){
                double lo=-10000,hi=10000;
                if(f.key=="phase"){lo=-1;hi=1;}
                if(f.key=="speed"&&cls=="func_bobbing")lo=.001;
                if(!Number(f.value.c_str(),lo,hi))return Fail(r,"Invalid section mover parameter: "+f.key);continue;
            }
            return Fail(r,"Portable field adapter is pending: "+cls+"."+f.key);
        }
        // Leave room for canonical origin/angles and generated local-angle keys.
        if(e.size()+3>MAX_SPAWN_VARS||chars+384>MAX_SPAWN_VARS_CHARS)return Fail(r,"Transformed entity keys exceed the reserved parser budget");
        if(mover){
            const char*model=Value(e,"model");
            if(*model!='*'||!Number(model+1,1,r.models-1)||strspn(model+1,"0123456789")!=strlen(model+1))return Fail(r,"Mover requires a valid local inline brush");
            if(cls=="func_door"||cls=="func_plat")++r.allocationBudget;
        }
        if(speaker&&(!*Value(e,"noise")||atoi(Value(e,"spawnflags"))>3))return Fail(r,"Speaker requires local playback flags and a sound");
        if(speaker&&sentinelRoom){
            if(e.size()!=4||strcmp(Value(e,"spawnflags"),"1")||!*Value(e,"origin")||
               !OneOf(Value(e,"noise"),{"sound/ambience/cairn/cairn_reactor.wav","sound/ambience/cairn/cairn_duct.wav",
                                       "sound/ambience/cairn/cairn_circuits.wav","sound/ambience/kejim/kejim_interior.wav"}))
                return Fail(r,"Vjun Sentinel requires its local stock ambient loops");
            ++sentinelSpeakers;
        }
        if(speaker&&(mode==bspSection::Mode::Breakable||korribanRoom)){
            if(e.size()!=4||strcmp(Value(e,"noise"),"sound/ambience/prototype/windmix1.wav")||
               strcmp(Value(e,"spawnflags"),"1")||!*Value(e,"origin"))
                return Fail(r,"Section needs its stock local wind emitters");
            ++citySpeakers;
        }
        if(fire){
            if(strcmp(Value(e,"fxfile"),bspSectionFX::effect)||strcmp(Value(e,"soundset"),bspSectionFX::soundSet)||strcmp(Value(e,"spawnflags"),"4")||!*Value(e,"origin"))
                return Fail(r,"Fire section requires the stock torch effect, sound set and damage flags");
            if(!fires++){
                r.resources.push_back(bspSectionFX::effectFile);
                r.resources.push_back(bspSectionFX::loop);
            }
        }
        if(hurt){
            // Retail MP uses -1 for a fatal fall. Other negative values, scripted
            // targets, arbitrary origins and non-stock policies need an adapter.
            const auto *dmg=Value(e,"dmg"),*wait=Value(e,"wait"),*flags=Value(e,"spawnflags");
            const bool fall=!strcmp(dmg,"-1")&&!*wait&&(!*flags||!strcmp(flags,"8"));
            const bool timed=!strcmp(dmg,"50")&&!strcmp(wait,"1")&&!*flags;
            const bool shaft=mode==bspSection::Mode::Shaft;
            if(strcmp(Value(e,"model"),shaft?"*5":"*1")||(!fall&&(shaft||!timed))||*Value(e,"origin")||*Value(e,"angle")||
               *Value(e,"angles")||*Value(e,"targetname"))return Fail(r,"Section hazard requires its stock inline model and damage policy");
            ++hurts;
        }
        if(mode==bspSection::Mode::Bobbing&&mover){
            if(strcmp(Value(e,"model"),"*1")||strcmp(Value(e,"model2"),"models/map_objects/bespin/twinpodcc.md3")||strcmp(Value(e,"speed"),"3")||strcmp(Value(e,"height"),"20")||
               strcmp(Value(e,"origin"),"720 -384 120")||*Value(e,"angles")||*Value(e,"angle")||*Value(e,"modelangles")||*Value(e,"spawnflags")||*Value(e,"phase"))
                return Fail(r,"Bobbing section requires the stock Bespin cloud car path");
            ++bobs;
        }
        ++r.allocationBudget;
    }
    const int expectedModels=tatooineRoom?tatooine::modelCount:sentinelRoom?sentinel::modelCount:mode==bspSection::Mode::Breakable?cityBreakables+1:mode==bspSection::Mode::Shaft?6:mode==bspSection::Mode::Rift?3:korribanRoom?9:
        (mode==bspSection::Mode::Bobbing||mode==bspSection::Mode::Hurt)?2:1;
    if(sectionProfile&&(!r.entryPoints||r.models!=expectedModels))return Fail(r,"Section needs its expected brush-model layout and local entry points");
    if(mode==bspSection::Mode::Bobbing&&bobs!=1)return Fail(r,"Bespin section requires one native bobbing mover");
    if(mode==bspSection::Mode::Fire&&fires!=6)return Fail(r,"Rancor Pit section requires all six stock fire emitters");
    if(mode==bspSection::Mode::Hurt&&hurts!=1)return Fail(r,"Hazard section requires one stock hurt brush");
    if(mode==bspSection::Mode::Shaft){
        if(shaftLifts!=3||shaftPads!=3||shaftScenery[0]!=4||shaftScenery[1]!=1||shaftScenery[2]!=1||hurts!=1)
            return Fail(r,"Imperial Shaft requires two lift pairs, six scenery models and one fall volume");
        for(const char* sound:shaft::liftSounds)r.resources.push_back(sound);
    }
    if(stockPickups){
        if(fallBrushes!=(tatooineRoom?0u:sentinelRoom?1u<<(sentinel::fallModel-1):korribanRoom?255u:3u)||
           r.entryPoints!=(tatooineRoom?tatooine::entryCount:sentinelRoom?sentinel::entryCount:korribanRoom?korriban::entryCount:16)||
           r.entities!=(tatooineRoom?tatooine::entityCount:sentinelRoom?sentinel::entityCount:korribanRoom?korriban::entityCount:123))
            return Fail(r,"Section needs all its fall triggers and local entry points");
        if(korribanRoom&&(citySpeakers!=korriban::windCount||fires!=korriban::fireCount))
            return Fail(r,"Korriban FFA needs all its stock wind and torch emitters");
        for(size_t n=0;n<pickupKinds;++n){
            if(pickupCounts[n]!=pickupSpecs[n].count)
                return Fail(r,"Section pickup counts differ from the retail BSP");
            r.pickupClasses.push_back(pickupSpecs[n].name);
        }
    }
    if(sentinelRoom||tatooineRoom){
        if(liftMask!=(1u<<(liftEnd-lifts))-1||padMask!=(1u<<(padEnd-pads))-1||
           (sentinelRoom&&sentinelSpeakers!=12)||(tatooineRoom&&r.ownedHelpers!=1))
            return Fail(r,"Section needs all its stock lifts, floor triggers and helpers");
        if(sentinelRoom)for(const char* sound:shaft::liftSounds)r.resources.push_back(sound);
        for(const char* sound:sentinel::pistonSounds)r.resources.push_back(sound);
    }
    if(mode==bspSection::Mode::Breakable){
        if(r.entities!=107||r.entryPoints!=13||citySpeakers!=2||cityHealth[1]!=10||cityHealth[2]!=26||cityHealth[3]!=33)
            return Fail(r,"Abandoned City requires all 69 stock blocks, local entry points and speakers");
        for(const char* resource:{"sound/weapons/explosions/cargoexplode.wav","sound/effects/wall_smash.mp3",
                "effects/chunks/rockbreaklg.efx","effects/chunks/rockbreakmed.efx",
                "models/chunks/rock/rock2_1.md3","models/chunks/rock/rock2_2.md3",
                "models/chunks/rock/rock2_3.md3","models/chunks/rock/rock2_4.md3"})
            r.resources.push_back(resource);
    }
    r.rejectedEntity=0;return true;
}
// Convert a brush to an oriented box only when the actual brush is exactly
// that box. Verify the six plane equations, not just its enclosing model AABB.
template<class T> inline bool LumpRecord(const byte*bytes,const lump_t&l,int index,T&record){
    if(index<0||size_t(l.filelen)%sizeof(T)||size_t(index)>=size_t(l.filelen)/sizeof(T))return false;
    memcpy(&record,bytes+l.fileofs+size_t(index)*sizeof(T),sizeof(T));return true;
}
template<class T> inline bool LumpRange(const lump_t&l,int first,int count){
    if(l.filelen%sizeof(T)||first<0||count<1)return false;
    const int total=l.filelen/int(sizeof(T));
    return first<=total&&count<=total-first;
}
inline bool BreakableBrush(const byte*bytes,const dheader_t&h,const dmodel_t&model,const dbrush_t&brush,Report&r,bool yavin){
    const int first=LittleLong(brush.firstSide),count=LittleLong(brush.numSides);
    dshader_t material;
    if(count<6||count>(yavin?23:19)||!LumpRange<dbrushside_t>(h.lumps[LUMP_BRUSHSIDES],first,count)||
       !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(brush.shaderNum),material)||
       LittleLong(material.contentFlags)!=(CONTENTS_SOLID|CONTENTS_OPAQUE))
        return Fail(r,"Abandoned City block brush range or material is invalid");
    vec3_t low{},high{};
    for(int side=0;side<count;++side){
        dbrushside_t face;
        dplane_t plane;
        dsurface_t surface;
        if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHSIDES],first+side,face)||
           !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(face.shaderNum),material)||
           !LumpRecord(bytes,h.lumps[LUMP_PLANES],LittleLong(face.planeNum),plane)||
           !LumpRecord(bytes,h.lumps[LUMP_SURFACES],LittleLong(face.drawSurfNum),surface))
            return Fail(r,"Abandoned City block has an invalid brush side reference");
        const float distance=LittleFloat(plane.dist);
        float lengthSquared=0;
        for(int axis=0;axis<3;++axis){
            const float value=LittleFloat(plane.normal[axis]);
            if(!std::isfinite(value))return Fail(r,"Abandoned City block has a non-finite plane");
            lengthSquared+=value*value;
            // CM_BoundBrush reads these six distances in fixed axis order.
            if(side<6&&value!=(axis==side/2?(side%2?1.f:-1.f):0.f))
                return Fail(r,"Abandoned City block is missing its ordered axial planes");
        }
        if(!std::isfinite(distance)||std::fabs(distance)>65536||std::fabs(lengthSquared-1.f)>.001f)
            return Fail(r,"Abandoned City block has an invalid plane");
        if(side<6){
            if(side%2)high[side/2]=distance;
            else low[side/2]=-distance;
        }
    }
    // Yavin's compiled axial bevels round up to 0.00464 units outside
    // its draw-model bounds. The native inline hull already pads by one unit.
    const float tolerance=yavin?1.f/128:0;
    for(int axis=0;axis<3;++axis)
        if(low[axis]>=high[axis]||low[axis]<LittleFloat(model.mins[axis])-tolerance||high[axis]>LittleFloat(model.maxs[axis])+tolerance)
            return Fail(r,"Abandoned City block planes exceed its inline model or enclose no volume");
    return true;
}
inline bool BreakableSurface(const byte*bytes,const dheader_t&h,const dmodel_t&model,int index,Report&r){
    dsurface_t surface;
    dshader_t shader;
    if(!LumpRecord(bytes,h.lumps[LUMP_SURFACES],index,surface)||
       !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(surface.shaderNum),shader))
        return Fail(r,"Abandoned City block has an invalid draw surface or shader reference");
    const int type=LittleLong(surface.surfaceType);
    const int firstVertex=LittleLong(surface.firstVert),vertices=LittleLong(surface.numVerts);
    const int firstIndex=LittleLong(surface.firstIndex),indexes=LittleLong(surface.numIndexes);
    if((type!=MST_PLANAR&&type!=MST_TRIANGLE_SOUP)||LittleLong(surface.fogNum)!=-1||
       indexes%3||!LumpRange<drawVert_t>(h.lumps[LUMP_DRAWVERTS],firstVertex,vertices)||
       !LumpRange<int>(h.lumps[LUMP_DRAWINDEXES],firstIndex,indexes))
        return Fail(r,"Abandoned City block has an invalid draw surface range or type");
    for(int i=0;i<vertices;++i){
        drawVert_t vertex;
        if(!LumpRecord(bytes,h.lumps[LUMP_DRAWVERTS],firstVertex+i,vertex))
            return Fail(r,"Abandoned City block has a missing draw vertex");
        for(int axis=0;axis<3;++axis){
            const float point=LittleFloat(vertex.xyz[axis]);
            if(!std::isfinite(point)||point<LittleFloat(model.mins[axis])||point>LittleFloat(model.maxs[axis]))
                return Fail(r,"Abandoned City block draw vertices exceed its inline model");
        }
    }
    for(int i=0;i<indexes;++i){
        int vertex;
        if(!LumpRecord(bytes,h.lumps[LUMP_DRAWINDEXES],firstIndex+i,vertex)||LittleLong(vertex)<0||LittleLong(vertex)>=vertices)
            return Fail(r,"Abandoned City block has an invalid draw vertex reference");
    }
    return true;
}
inline bool BreakableModels(const byte*bytes,const dheader_t&h,Report&r,bool yavin){
    struct Span {int first,count;};
    Span brushRanges[cityBreakables+1]{},surfaceRanges[cityBreakables+1]{};
    const auto overlaps=[](Span a,Span b){return a.first<b.first+b.count&&b.first<a.first+a.count;};
    for(int index=0;index<r.models;++index){
        dmodel_t model;
        if(!LumpRecord(bytes,h.lumps[LUMP_MODELS],index,model))
            return Fail(r,"Abandoned City inline model is missing");
        const Span brushes{LittleLong(model.firstBrush),LittleLong(model.numBrushes)};
        const Span surfaces{LittleLong(model.firstSurface),LittleLong(model.numSurfaces)};
        if(!LumpRange<dbrush_t>(h.lumps[LUMP_BRUSHES],brushes.first,brushes.count)||
           !LumpRange<dsurface_t>(h.lumps[LUMP_SURFACES],surfaces.first,surfaces.count))
            return Fail(r,"Abandoned City inline model brush or surface range is invalid");
        for(int previous=0;previous<index;++previous)
            if(overlaps(brushes,brushRanges[previous])||overlaps(surfaces,surfaceRanges[previous]))
                return Fail(r,"Abandoned City inline model brush or surface ranges overlap");
        brushRanges[index]=brushes;
        surfaceRanges[index]=surfaces;
        if(index==0)continue;
        if(brushes.count>(yavin?6:3)||surfaces.count>12)
            return Fail(r,"Abandoned City block exceeds the stock inline model size");
        for(int axis=0;axis<3;++axis)
            if(LittleFloat(model.mins[axis])<r.mins[axis]||LittleFloat(model.maxs[axis])>r.maxs[axis])
                return Fail(r,"Abandoned City inline model exceeds the section hull");
        for(int i=0;i<brushes.count;++i){
            dbrush_t brush;
            if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHES],brushes.first+i,brush))
                return Fail(r,"Abandoned City inline model brush is missing");
            if(!BreakableBrush(bytes,h,model,brush,r,yavin))return false;
        }
        for(int i=0;i<surfaces.count;++i)
            if(!BreakableSurface(bytes,h,model,surfaces.first+i,r))return false;
    }
    return true;
}
inline bool AxialSectionBrush(const byte*bytes,const dheader_t&h,Report&r,int index,vec3_t low,vec3_t high,
    const char* weatherMaterial=nullptr){
    dbrush_t brush;dshader_t material;
    if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHES],index,brush)||LittleLong(brush.numSides)!=6||
       !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(brush.shaderNum),material)||
       (LittleLong(material.contentFlags)&CONTENTS_SOLID)||
       (weatherMaterial?strncmp(material.shader,weatherMaterial,sizeof(material.shader))!=0:
        !(LittleLong(material.contentFlags)&CONTENTS_TRIGGER)))
        return Fail(r,weatherMaterial?"Section weather volume does not use the stock brush material":
            "Section hazard is not a single non-solid trigger brush");
    const int first=LittleLong(brush.firstSide);
    if(first<0||first>h.lumps[LUMP_BRUSHSIDES].filelen/int(sizeof(dbrushside_t))-6)
        return Fail(r,"Section hazard brush side range is invalid");
    unsigned seen=0;
    for(int side=0;side<6;++side){
        dbrushside_t face;dplane_t plane;
        if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHSIDES],first+side,face)||
           !LumpRecord(bytes,h.lumps[LUMP_PLANES],LittleLong(face.planeNum),plane))return Fail(r,"Section hazard plane reference is invalid");
        int axis=-1;float sign=0;
        for(int i=0;i<3;++i){const float n=LittleFloat(plane.normal[i]);if(n==0)continue;
            if(axis>=0||(n!=1&&n!=-1))return Fail(r,"Section hazard must have six axial faces");axis=i;sign=n;}
        if(axis<0)return Fail(r,"Section hazard plane is degenerate");
        const unsigned bit=1u<<(2*axis+(sign>0));
        const float limit=LittleFloat(plane.dist);
        if((seen&bit)||!std::isfinite(limit))return Fail(r,"Section hazard has duplicate or invalid planes");
        seen|=bit;
        if(sign>0)high[axis]=limit;else low[axis]=-limit;
    }
    for(int i=0;!weatherMaterial&&i<3;++i)
        if(low[i]<r.mins[i]||high[i]>r.maxs[i])
            return Fail(r,"Section hazard exceeds the section hull");
    return BSPSectionBoxValid(low,high)||Fail(r,"Section hazard bounds exceed the portable volume limits");
}
inline bool TriggerBrush(const byte*bytes,const dheader_t&h,Report&r,int index,vec3_t low,vec3_t high){
    return AxialSectionBrush(bytes,h,r,index,low,high);
}
inline bool TriggerModel(const byte*bytes,const dheader_t&h,Report&r,int index,int count,dmodel_t&model){
    if(!LumpRecord(bytes,h.lumps[LUMP_MODELS],index,model)||LittleLong(model.numSurfaces)!=0||
       LittleLong(model.numBrushes)!=count||!LumpRange<dbrush_t>(h.lumps[LUMP_BRUSHES],LittleLong(model.firstBrush),count))
        return Fail(r,"Section hazard does not contain its expected trigger brushes");
    return true;
}
inline bool TriggerBounds(Report&r,const dmodel_t&model,const vec3_t low,const vec3_t high){
    for(int i=0;i<3;++i)
        if(low[i]!=LittleFloat(model.mins[i])||high[i]!=LittleFloat(model.maxs[i]))
            return Fail(r,"Section hazard planes disagree with the inline model");
    return true;
}
inline bool TriggerBox(const byte*bytes,const dheader_t&h,Report&r,int index,vec3_t low,vec3_t high){
    dmodel_t model;
    return TriggerModel(bytes,h,r,index,1,model)&&
        TriggerBrush(bytes,h,r,LittleLong(model.firstBrush),low,high)&&TriggerBounds(r,model,low,high);
}
inline bool RiftTriggerBoxes(const byte*bytes,const dheader_t&h,Report&r){
    dmodel_t compound;
    vec3_t mins,maxs;
    if(!TriggerModel(bytes,h,r,1,2,compound)||!TriggerBox(bytes,h,r,2,mins,maxs))return false;
    if(!VectorCompare(mins,rift::fallBoxes[2].mins)||!VectorCompare(maxs,rift::fallBoxes[2].maxs))
        return Fail(r,"Rift Sanctuary fall volume differs from the stock brush");
    const int first=LittleLong(compound.firstBrush);
    ClearBounds(mins,maxs);
    for(int n=0;n<2;++n){
        vec3_t low,high;
        if(!TriggerBrush(bytes,h,r,first+n,low,high))return false;
        if(!VectorCompare(low,rift::fallBoxes[n].mins)||!VectorCompare(high,rift::fallBoxes[n].maxs))
            return Fail(r,"Rift Sanctuary fall volume differs from the stock brush");
        AddPointToBounds(low,mins,maxs);
        AddPointToBounds(high,mins,maxs);
    }
    return TriggerBounds(r,compound,mins,maxs);
}
inline bool FactoryGlassModels(const byte* bytes,const dheader_t& h,Report& r){
    for(const auto& window:factory::windows){
        dmodel_t model;
        dsurface_t face;
        dshader_t shader;
        if(!LumpRecord(bytes,h.lumps[LUMP_MODELS],window.model,model)||LittleLong(model.numSurfaces)!=1||
           LittleLong(model.numBrushes)!=1||!LumpRecord(bytes,h.lumps[LUMP_SURFACES],LittleLong(model.firstSurface),face)||
           LittleLong(face.surfaceType)!=MST_PLANAR||LittleLong(face.numVerts)!=4||
           !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(face.shaderNum),shader)||
           strncmp(shader.shader,window.shader,sizeof(shader.shader)))return Fail(r,"Factory needs its stock planar glass faces");
        unsigned points=0;
        const int firstVert=LittleLong(face.firstVert);
        if(!LumpRange<drawVert_t>(h.lumps[LUMP_DRAWVERTS],firstVert,4))return Fail(r,"Factory glass has invalid vertex indices");
        for(int i=0;i<4;++i){
            drawVert_t v;
            if(!LumpRecord(bytes,h.lumps[LUMP_DRAWVERTS],firstVert+i,v))return Fail(r,"Factory glass is missing a vertex");
            const auto match=std::find_if(std::begin(window.points),std::end(window.points),[&](const auto& point){
                return LittleFloat(v.xyz[0])==point[0]&&LittleFloat(v.xyz[1])==point[1]&&LittleFloat(v.xyz[2])==point[2];
            });
            if(match==std::end(window.points))return Fail(r,"Factory glass geometry differs from the stock pane");
            points|=1u<<(match-window.points);
        }
        if(points!=15)return Fail(r,"Factory glass needs four distinct corners");
    }
    return true;
}
inline bool CtfTriggerBrush(const byte* bytes,const dheader_t& h,Report& r,int number){
        dbrush_t brush;dshader_t shader;
        if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHES],number,brush)||
           !LumpRange<dbrushside_t>(h.lumps[LUMP_BRUSHSIDES],LittleLong(brush.firstSide),LittleLong(brush.numSides))||
           LittleLong(brush.numSides)<6||LittleLong(brush.numSides)>24||
           !LumpRecord(bytes,h.lumps[LUMP_SHADERS],LittleLong(brush.shaderNum),shader)||
           !(LittleLong(shader.contentFlags)&CONTENTS_TRIGGER)||(LittleLong(shader.contentFlags)&CONTENTS_SOLID))
            return Fail(r,"CTF section requires its stock non-solid trigger brushes");
        for(int j=0;j<LittleLong(brush.numSides);++j){
            dbrushside_t side;dplane_t plane;
            if(!LumpRecord(bytes,h.lumps[LUMP_BRUSHSIDES],LittleLong(brush.firstSide)+j,side)||
               !LumpRecord(bytes,h.lumps[LUMP_PLANES],LittleLong(side.planeNum),plane))
                return Fail(r,"CTF trigger has an invalid plane reference");
            const vec3_t n={LittleFloat(plane.normal[0]),LittleFloat(plane.normal[1]),LittleFloat(plane.normal[2])};
            if(!(std::fabs(DotProduct(n,n)-1.f)<.001f)||!std::isfinite(LittleFloat(plane.dist)))
                return Fail(r,"CTF trigger has an invalid collision plane");
        }
        return true;
}
inline bool CtfTriggerModel(const byte* bytes,const dheader_t& h,Report& r,int index,int count){
    dmodel_t model;
    if(!TriggerModel(bytes,h,r,index,count,model))return false;
    for(int j=0;j<count;++j)if(!CtfTriggerBrush(bytes,h,r,LittleLong(model.firstBrush)+j))return false;
    return true;
}
template<size_t N>
inline bool CtfTriggers(const byte* bytes,const dheader_t& h,Report& r,const sectionCTF::Lift (&lifts)[N],bool multiBrush){
    for(int index=1;index<r.models;++index){
        if(std::any_of(std::begin(lifts),std::end(lifts),[&](const sectionCTF::Lift& row){return row.model==index;}))continue;
        const int count=multiBrush&&(index==1||index==7)?3:1;
        if(!CtfTriggerModel(bytes,h,r,index,count))return false;
    }
    return true;
}
inline bool CoruscantTriggers(const byte* bytes,const dheader_t& h,Report& r){
    dfog_t fog;
    if(h.lumps[LUMP_FOGS].filelen!=sizeof(fog)||!LumpRecord(bytes,h.lumps[LUMP_FOGS],0,fog)||
       strncmp(fog.shader,coruscant::fog,sizeof(fog.shader))||LittleLong(fog.brushNum)!=-1||
       LittleLong(fog.visibleSide)!=-1)return Fail(r,"Coruscant requires its stock global fog record");
    return CtfTriggers(bytes,h,r,coruscant::lifts,false);
}
inline bool WastelandGeometry(const byte* bytes,const dheader_t& h,Report& r){
    dfog_t fog;
    if(h.lumps[LUMP_FOGS].filelen!=sizeof(fog)||!LumpRecord(bytes,h.lumps[LUMP_FOGS],0,fog)||
       strncmp(fog.shader,wasteland::fog,sizeof(fog.shader))||LittleLong(fog.brushNum)!=-1||
       LittleLong(fog.visibleSide)!=-1)return Fail(r,"Hoth Wasteland requires its stock global fog record");
    for(const auto& volume:wasteland::weatherVolumes){
        dmodel_t model;
        vec3_t mins,maxs;
        if(!TriggerModel(bytes,h,r,volume.model,1,model)||
           !AxialSectionBrush(bytes,h,r,LittleLong(model.firstBrush),mins,maxs,wasteland::weatherMaterial)||
           !TriggerBounds(r,model,mins,maxs))return false;
        if(!VectorCompare(mins,volume.mins)||!VectorCompare(maxs,volume.maxs))
            return Fail(r,"Hoth Wasteland weather planes differ from the stock volume");
    }
    for(const auto& pad:wasteland::liftPads)if(!CtfTriggerModel(bytes,h,r,pad.model,1))return false;
    for(const auto& fall:wasteland::falls)if(!CtfTriggerModel(bytes,h,r,fall.model,fall.brushes))return false;
    return true;
}
inline bool SiegeKorribanGeometry(const byte* bytes,const dheader_t& h,Report& r){
    for(const auto& pad:siegeKorriban::pressurePads)if(!CtfTriggerModel(bytes,h,r,pad.model,1))return false;
    for(int model:siegeKorriban::hazardModels)if(!CtfTriggerModel(bytes,h,r,model,1))return false;
    for(int index:siegeKorriban::lavaModels){
        dmodel_t model;
        vec3_t mins,maxs;
        if(!TriggerModel(bytes,h,r,index,1,model)||
           !AxialSectionBrush(bytes,h,r,LittleLong(model.firstBrush),mins,maxs,"textures/system/nodrop")||
           !TriggerBounds(r,model,mins,maxs))return false;
    }
    for(const auto& delivery:siegeKorriban::deliveries)
        if(!CtfTriggerModel(bytes,h,r,delivery.model,1))return false;
    return true;
}
inline bool Inspect(const void*data,size_t size,Report&r,bspSection::Mode mode=bspSection::Mode::Generic,
                    std::vector<Entity>*admitted=nullptr){
    r=Report{};
    if(!data||size<sizeof(dheader_t)||size>maxFileBytes)return Fail(r,"BSP file size is outside the portable limit");
    const auto*bytes=static_cast<const byte*>(data);dheader_t h;memcpy(&h,bytes,sizeof(h));
    if(LittleLong(h.ident)!=BSP_IDENT||LittleLong(h.version)!=BSP_VERSION)return Fail(r,"BSP format is not Jedi Academy RBSP");
    for(int i=0;i<HEADER_LUMPS;++i){auto&l=h.lumps[i];l.fileofs=LittleLong(l.fileofs);l.filelen=LittleLong(l.filelen);
        if(l.fileofs<0||l.filelen<0||size_t(l.fileofs)>size||size_t(l.filelen)>size-size_t(l.fileofs)||(l.filelen&&size_t(l.fileofs)<sizeof(h)))return Fail(r,"BSP lump range is invalid");
        for(int j=0;j<i;++j){const auto&p=h.lumps[j];
            if(l.filelen&&p.filelen&&l.fileofs<p.fileofs+p.filelen&&p.fileofs<l.fileofs+l.filelen)return Fail(r,"BSP lumps overlap");}
    }
    const auto&m=h.lumps[LUMP_MODELS];
    if(!m.filelen||m.filelen%sizeof(dmodel_t)||size_t(m.filelen)/sizeof(dmodel_t)>maxInlineModels)return Fail(r,"BSP model table is invalid");
    r.models=int(m.filelen/sizeof(dmodel_t));
    for(int n=0;n<r.models;++n){dmodel_t model;memcpy(&model,bytes+m.fileofs+n*sizeof(model),sizeof(model));
        for(int i=0;i<3;++i){const float lo=LittleFloat(model.mins[i]),hi=LittleFloat(model.maxs[i]);
            if(!std::isfinite(lo)||!std::isfinite(hi)||lo>hi||lo< -65536||hi>65536)return Fail(r,"BSP model bounds are invalid");
            if(!n){r.mins[i]=lo;r.maxs[i]=hi;}}
    }
    const auto&l=h.lumps[LUMP_ENTITIES];
    if(!l.filelen||l.filelen>16*MAX_MAP_ENTSTRING)return Fail(r,"BSP entity text is outside the portable limit");
    const char*text=reinterpret_cast<const char*>(bytes+l.fileofs);size_t length=l.filelen;
    if(text[length-1]==0)--length;
    if(memchr(text,0,length))return Fail(r,"BSP entity text contains an embedded terminator");
    Reader reader(text,length);std::vector<Entity> entities;std::string token;bool eof;
    for(;;){
        if(!reader.Next(token,eof,r))return false;if(eof)break;
        r.rejectedEntity=int(entities.size())+1;
        if(token!="{")return Fail(r,"Expected an entity opening brace");
        Entity e;size_t chars=0;
        for(;;){
            if(!reader.Next(token,eof,r))return false;
            if(eof)return Fail(r,"Unclosed entity record");
            if(token=="}")break;
            if(token.empty()||token=="{")return Fail(r,"Invalid entity key");
            const std::string key=Lower(token);
            if(!reader.Next(token,eof,r))return false;
            if(eof||token=="}"||token=="{")return Fail(r,"Entity key has no value");
            chars+=key.size()+token.size()+2;
            if(e.size()>=MAX_SPAWN_VARS||chars>MAX_SPAWN_VARS_CHARS)return Fail(r,"Entity keys exceed native parser capacity");
            const auto duplicate=std::find_if(e.begin(),e.end(),[&](const Field&f){return f.key==key;});
            if(duplicate!=e.end()){
                if(mode==bspSection::Mode::Taspir&&key=="soundset"&&token==taspir::sounds[2].stock&&
                   token==duplicate->value&&!strcmp(Value(e,"classname"),"func_train")&&
                   !strcmp(Value(e,"model"),"*2")&&!r.redundantSoundKeys){
                    ++r.redundantSoundKeys;
                    continue;
                }
                return Fail(r,"Duplicate entity key: "+key);
            }
            e.push_back({key,token});
        }
        entities.push_back(std::move(e));
        r.entities=int(entities.size());
        if(entities.size()>=MAX_GENTITIES)return Fail(r,"Too many portable section entities");
    }
    r.entities=int(entities.size());
    if(entities.empty())return Fail(r,"BSP has no worldspawn");
    if(!Content(entities,r,mode))return false;
    if(mode==bspSection::Mode::Hurt&&!TriggerBox(bytes,h,r,1,r.hurtMins,r.hurtMaxs))return false;
    if(mode==bspSection::Mode::Shaft&&
       (!TriggerBox(bytes,h,r,5,r.hurtMins,r.hurtMaxs)||
        !TriggerBox(bytes,h,r,1,r.padMins[0],r.padMaxs[0])||
        !TriggerBox(bytes,h,r,4,r.padMins[1],r.padMaxs[1])))return false;
    if(mode==bspSection::Mode::Rift&&!RiftTriggerBoxes(bytes,h,r))return false;
    if(mode==bspSection::Mode::Sentinel&&!TriggerBox(bytes,h,r,sentinel::fallModel,r.hurtMins,r.hurtMaxs))return false;
    if(mode==bspSection::Mode::Taspir&&!TriggerBox(bytes,h,r,taspir::fallModel,r.hurtMins,r.hurtMaxs))return false;
    if(mode==bspSection::Mode::Factory&&(!TriggerBox(bytes,h,r,factory::hurtModel,r.hurtMins,r.hurtMaxs)||!FactoryGlassModels(bytes,h,r)))return false;
    if(mode==bspSection::Mode::Coruscant&&!CoruscantTriggers(bytes,h,r))return false;
    if(mode==bspSection::Mode::Hilltops&&!CtfTriggers(bytes,h,r,hilltops::lifts,true))return false;
    if(mode==bspSection::Mode::DropZone){
        for(const auto& pad:dropzone::pads)if(!CtfTriggerModel(bytes,h,r,pad.model,1))return false;
        for(int fall:dropzone::falls)if(!CtfTriggerModel(bytes,h,r,fall,1))return false;
    }
    if(mode==bspSection::Mode::Wasteland&&!WastelandGeometry(bytes,h,r))return false;
    if(mode==bspSection::Mode::SiegeKorriban&&!SiegeKorribanGeometry(bytes,h,r))return false;
    if(mode==bspSection::Mode::Korriban){
        for(size_t i=0;i<ARRAY_LEN(korriban::fallBoxes);++i){
            vec3_t mins,maxs;
            if(!TriggerBox(bytes,h,r,int(i)+1,mins,maxs))return false;
            if(!VectorCompare(mins,korriban::fallBoxes[i].mins)||!VectorCompare(maxs,korriban::fallBoxes[i].maxs))
                return Fail(r,"Korriban FFA fall volume differs from the stock brush");
        }
    }
    if(bspSection::Breakable(mode)&&!BreakableModels(bytes,h,r,mode==bspSection::Mode::Yavin))return false;
    if(admitted)*admitted=std::move(entities);
    return true;
}
} }
#endif
