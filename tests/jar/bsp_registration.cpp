// Production shared client registration; collision/renderer imports are fixtures.
#include "qcommon/q_shared.h"
#include "test_support.h"
#include <map>
#include <string>
#include <vector>
struct {int numInlineModels=3;qhandle_t inlineDrawModel[512]{};vec3_t inlineModelMidpoints[512]{};} cgs;
static std::map<int,std::string> configs;
struct Range{int first,count;};static std::map<std::string,Range> maps;
static std::vector<std::string> registrations,loads;static int messages=0,failedChild=-1;
const char*CG_ConfigString(int n){return configs[n].c_str();}
void CG_Printf(const char*,...){++messages;}
qboolean Fixture_CM_BSPModelInfo(const char*name,int*first,int*count){auto p=maps.find(name);if(p==maps.end())return qfalse;*first=p->second.first;*count=p->second.count;return qtrue;}
void Fixture_CM_LoadMap(const char*name,qboolean sub){CHECK(sub);loads.emplace_back(name);if(!strcmp(name,"#new"))maps[name]={40,3};}
qhandle_t Fixture_R_RegisterModel(const char*name){registrations.emplace_back(name);const char*colon=strchr(name,':');CHECK(colon);int child=atoi(colon+1);if(child==failedChild)return 0;return 100+child;}
void Fixture_R_ModelBounds(qhandle_t n,vec3_t low,vec3_t high){VectorSet(low,float(n),-8,-4);VectorSet(high,float(n+2),8,4);}
#define JAR_BSP_CALL(name) Fixture_##name
#include "../../shared/qcommon/jar_bsp_client.inc"
#undef JAR_BSP_CALL
int main(){return RunTests("BSP names, authoritative ranges, late registration, retries and reset",[]{
 char path[MAX_QPATH];int child=-1;
 CHECK(JAR_BSPModelName("#mp/duel1",path,&child)&&!strcmp(path,"maps/mp/duel1.bsp")&&child==0);
 CHECK(JAR_BSPModelName("#mp/duel1:12",path,&child)&&child==12&&!strcmp(path,"maps/mp/duel1.bsp"));
 for(const char*bad:{"","map","#","#/map","#../map","#x/../map","#x//map","#x/","#x\\map","#x:512","#x:-1","#x:","#x:1:2","#x:999999999999999","#x\nmap"})CHECK(!JAR_BSPModelName(bad,path,&child));
 std::string maxName="#"+std::string(MAX_QPATH-10,'x');CHECK(JAR_BSPModelName(maxName.c_str(),path,&child)&&strlen(path)==MAX_QPATH-1);
 maxName+='x';CHECK(!JAR_BSPModelName(maxName.c_str(),path,&child));
 configs[CS_BSP_MODELS+1]="#second";configs[CS_BSP_MODELS+3]="#first";maps["#first"]={3,2};maps["#second"]={11,1};
 CG_ResetBSPModels();CG_RegisterBSPModels();CHECK(loads.empty()&&registrations.size()==3&&messages==0);
 CHECK(cgs.inlineDrawModel[3]==100&&cgs.inlineDrawModel[4]==101&&cgs.inlineDrawModel[11]==100&&!cgs.inlineDrawModel[5]);
 CHECK(cgs.inlineModelMidpoints[4][0]==102&&cgs.numInlineModels==12);
 CG_RegisterBSPModels();CHECK(registrations.size()==3);configs[CS_BSP_MODELS+1]="#SECOND";CG_RegisterBSPModels();CHECK(registrations.size()==3);
 // A later server configstring registers all new draw handles at CM's indices.
 configs[CS_BSP_MODELS+7]="#new";CG_RegisterBSPModels();CHECK(loads.size()==1&&registrations.size()==6&&cgs.inlineDrawModel[42]==102&&cgs.numInlineModels==43);
 configs[CS_BSP_MODELS+8]="#retry";maps["#retry"]={50,3};failedChild=1;CG_RegisterBSPModels();CHECK(!cgs.inlineDrawModel[51]&&messages==1);
 failedChild=-1;CG_RegisterBSPModels();CHECK(cgs.inlineDrawModel[51]==101&&cgs.numInlineModels==53);const auto before=registrations.size();CG_RegisterBSPModels();CHECK(registrations.size()==before);
 // Bad ranges cannot overwrite adjacent arrays. A changed slot resolves a new range.
 configs[CS_BSP_MODELS+8]="#overflow";maps["#overflow"]={511,2};CG_RegisterBSPModels();CHECK(registrations.size()==before&&!cgs.inlineDrawModel[511]);
 configs[CS_BSP_MODELS+8]="#empty";maps["#empty"]={0,0};CG_RegisterBSPModels();CHECK(registrations.size()==before);
 configs[CS_BSP_MODELS+8]="#../../invalid";const auto loadCount=loads.size();CG_RegisterBSPModels();CHECK(loads.size()==loadCount);
 configs[CS_BSP_MODELS+8]="#new:0";CG_RegisterBSPModels();CHECK(loads.size()==loadCount);
 configs.erase(CS_BSP_MODELS+8);configs.erase(CS_BSP_MODELS+7);configs[CS_BSP_MODELS+1]="#second";
 CG_ResetBSPModels();CG_RegisterBSPModels();CHECK(registrations.size()==before+3);
 });}
