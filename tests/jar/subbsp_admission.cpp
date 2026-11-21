// Executes portable content admission, not collision/renderer lump decoding.
#include "game/g_local.h"
#include "qcommon/qfiles.h"
#include "game/g_bsp_section.h"
#include "game/g_subbsp_admission.h"
#include "test_support.h"
#include <fstream>
#include <iterator>
using namespace jar::bspAdmission;
static std::vector<byte> Map(const std::string&text,int models=2){
    dheader_t h{};h.ident=BSP_IDENT;h.version=BSP_VERSION;
    h.lumps[LUMP_MODELS]={sizeof(h),models*int(sizeof(dmodel_t))};
    h.lumps[LUMP_ENTITIES]={int(sizeof(h)+models*sizeof(dmodel_t)),int(text.size()+1)};
    std::vector<byte>b(h.lumps[LUMP_ENTITIES].fileofs+text.size()+1);
    memcpy(b.data(),&h,sizeof(h));memcpy(b.data()+h.lumps[LUMP_ENTITIES].fileofs,text.c_str(),text.size()+1);
    for(int n=0;n<models;++n){dmodel_t m{};VectorSet(m.mins,-32,-24,-16);VectorSet(m.maxs,32,24,16);memcpy(b.data()+sizeof(h)+n*sizeof(m),&m,sizeof(m));}
    return b;
}
static bool InspectBytes(const std::vector<byte>&b,Report&r){return Inspect(b.data(),b.size(),r);}
static void Reject(const std::string&text,const char*why){Report r;CHECK(!InspectBytes(Map(text),r));CHECK(r.reason.find(why)!=std::string::npos);}
static std::string Quote(const std::string&s){std::string out="\"";for(char c:s){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else out+=c;}return out+'"';}
int main(int argc,char**argv){
    if(argc==3||argc==4){
        auto mode=jar::bspSection::Mode::Generic;
        if(!strcmp(argv[1],"--section")){
            const auto*profile=argc==4?jar::bspSection::Resource(argv[3]):&jar::bspSection::profiles[0];
            if(!profile)return 2;mode=profile->mode;
        }else if(!strcmp(argv[1],"--siege-korriban")&&argc==3){
            mode=jar::bspSection::Mode::SiegeKorriban;
        }else if(strcmp(argv[1],"--inspect")||argc!=3)return 2;
        std::ifstream in(argv[2],std::ios::binary);if(!in)return 2;
        std::vector<byte>b((std::istreambuf_iterator<char>(in)),{});Report r;const bool allowed=Inspect(b.data(),b.size(),r,mode);
        printf("{\"ContentAdmitted\":%s,\"Models\":%d,\"Entities\":%d,\"AllocationBudget\":%d,\"RejectedEntity\":%d,\"Reason\":%s}\n",
            allowed?"true":"false",r.models,r.entities,r.allocationBudget,r.rejectedEntity,Quote(r.reason).c_str());return 0;
    }
    return RunTests("portable BSP content admission and non-mutating rejection",[]{
        const std::string world="{ \"classname\" \"worldspawn\" }\n";Report r;
        const std::string section=world+"{ classname info_player_duel1 origin \"2 4 8\" }\n{ classname target_speaker noise sound/local.wav spawnflags 1 }\n";
        auto portable=Map(section,1);CHECK(Inspect(portable.data(),portable.size(),r,jar::bspSection::Mode::Stationary)&&r.entryPoints==1&&r.resources.size()==1);
        CHECK(!InspectBytes(portable,r)); // Local-point adaptations require the explicit section profile.
        portable=Map(section+"{ classname func_door model *1 }\n",2);CHECK(!Inspect(portable.data(),portable.size(),r,jar::bspSection::Mode::Stationary));
        portable=Map(world+"{ classname info_player_duel1 }\n{ classname target_speaker noise sound/local.wav spawnflags 4 }\n",1);
        CHECK(!Inspect(portable.data(),portable.size(),r,jar::bspSection::Mode::Stationary));
        CHECK(InspectBytes(Map(world),r)&&r.entities==1&&r.models==2&&r.allocationBudget==1&&r.mins[2]==-16);
        for(const char*cls:{"func_door","func_button","func_plat","func_bobbing","func_rotating","func_pendulum"}){
            const std::string text=world+"{ classname "+cls+" model *1 origin \"10 20 30\" angles \"89 5 4\" targetname mover model2 models/map_objects/test.md3 }\n";
            CHECK(InspectBytes(Map(text),r)&&r.resources.size()==1&&r.entities==2);
            CHECK(r.allocationBudget==((!strcmp(cls,"func_door")||!strcmp(cls,"func_plat"))?3:2));
        }
        CHECK(InspectBytes(Map(world+"// comment\n{ classname info_notnull origin \"1 2 3\" } /* end */\n"),r));
        Reject(world+"{ classname target_scriptrunner usescript mission/end }","Script isolation");
        Reject(world+"{ classname func_door model *1 useSCRIPT mission/end }","Script isolation");
        for(const char*cls:{"NPC_Kyle","ref_tag","misc_bsp","terrain","info_player_start","target_level_change","trigger_once","target_speaker"})
            Reject(world+"{ classname "+cls+" }","adapter is pending");
        Reject(world+"{ classname func_door model *1 target outside }","field adapter");
        Reject(world+"{ classname func_door model *2 }","inline brush");
        Reject(world+"{ classname func_door model *1.0 }","inline brush");
        Reject(world+"{ classname func_door model #other }","inline brush");
        for(const char*v:{"nan 0 0","1e99 0 0","1 2","1 2 3 extra"})Reject(world+"{ classname info_null origin \""+v+"\" }","vector");
        Reject(world+"{ classname func_bobbing model *1 speed 0 }","parameter");
        Reject(world+"{ classname func_bobbing model *1 phase 5 }","parameter");
        Reject(world+"{ classname func_door model *1 model2 ../bad.md3 }","MD3");
        Reject(world+"{ classname func_door model *1 model2 models/npc.glm }","MD3");
        Reject(world+"{ classname info_null Spawnflags -1 }","spawnflags");
        Reject(world+"{ classname info_null CLASSNAME func_door }","Duplicate");
        Reject("{ classname info_notnull }","worldspawn");
        Reject(world+"{ classname }","no value");Reject(world+"{ classname \"unfinished", "Unterminated");
        Reject(world+"{ classname info_null", "Unclosed");Reject(world+"/* open", "comment");
        Reject(world+"{classname info_null }", "opening brace");
        Reject(world+"{ classname info_null key \""+std::string(MAX_TOKEN_CHARS,'x')+"\" }","token");
        std::string many="{ classname worldspawn ";for(int i=0;i<MAX_SPAWN_VARS;++i)many+="k"+std::to_string(i)+" v ";Reject(many+"}","capacity");
        std::string full=world;for(int i=0;i<MAX_GENTITIES;++i)full+="{ classname info_null }\n";Reject(full,"Too many");
        auto b=Map(world);CHECK(!Inspect(nullptr,b.size(),r));CHECK(!Inspect(b.data(),sizeof(dheader_t)-1,r));
        auto word=[&](size_t at,int value){b=Map(world);memcpy(b.data()+at,&value,4);CHECK(!InspectBytes(b,r));};
        word(0,0);word(4,99);word(8,-1);word(12,INT_MAX);word(8,int(b.size()));
        word(8+LUMP_MODELS*8+4,41);word(8+LUMP_MODELS*8+4,0);
        word(8+LUMP_ENTITIES*8,int(sizeof(dheader_t)));CHECK(r.reason.find("overlap")!=std::string::npos);
        b=Map(world);float nan=std::nanf("");memcpy(b.data()+sizeof(dheader_t),&nan,4);CHECK(!InspectBytes(b,r));
        b=Map(world);b[b.size()-4]=0;CHECK(!InspectBytes(b,r)&&r.reason.find("terminator")!=std::string::npos);
        // A missing final NUL is legal: the native collision loader appends one.
        b=Map(world);b.pop_back();int len=int(world.size());memcpy(b.data()+12,&len,4);CHECK(InspectBytes(b,r));
    });
}
