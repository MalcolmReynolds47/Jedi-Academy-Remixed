// Native collision registration, save chunks, restored linking and client lookup.
// BSP file decoding, collision linking and renderer uploads are service mocks.
#include "game/g_local.h"
#include "qcommon/cm_local.h"
#include "qcommon/jar_savegame.h"
#include "chunk_save.h"
#include <map>
#include <functional>
game_import_t gi{};clipMap_t cmg{},SubBSP[MAX_SUB_BSP]{};int NumSubBSP=0,TotalSubModels=0;
struct EmptyGhoulPool:IGhoul2InfoArray {
 int New()override{throw std::runtime_error("Unexpected Ghoul2 allocation");}
 void Delete(int)override{throw std::runtime_error("Unexpected Ghoul2 deletion");}
 bool IsValid(int)const override{return false;}
 std::vector<CGhoul2Info>&Get(int)override{throw std::runtime_error("Unexpected Ghoul2 read");}
 const std::vector<CGhoul2Info>&Get(int)const override{throw std::runtime_error("Unexpected Ghoul2 read");}
};
IGhoul2InfoArray&_TheGhoul2InfoArray(){static EmptyGhoulPool pool;return pool;}
struct DiskMap{int models;unsigned checksum;};
static std::map<std::string,DiskMap> disk;
static int loads=0,links=0,linkedModel=-1;
long FS_ReadFile(const char*name,void**bytes){CHECK(!bytes);return disk.count(name)?100:0;}
static void CM_LoadMap_Actual(const char*name,qboolean,int*checksum,clipMap_t&map){
 const auto d=disk.at(name);map={};map.numSubModels=d.models;map.checksum=d.checksum;*checksum=d.checksum;TotalSubModels+=d.models;++loads;
}
#include "qcommon/cm_subbsp_load.inc"
#include "qcommon/cm_jar_bsp.inc"
#include "qcommon/cm_jar_bsp_registry.inc"
#include "game/g_bsp_registry.inc"
struct {int numInlineModels=0;int inlineDrawModel[MAX_SUBMODELS]{};vec3_t inlineModelMidpoints[MAX_SUBMODELS]{};}cgs;
static std::map<int,std::string> configs;static std::vector<std::string> drawNames{std::string()};
const char*CG_ConfigString(int index){return configs[index].c_str();}
void CG_Printf(const char*,...){}
qboolean Fixture_CM_BSPModelInfo(const char*n,int*f,int*c){return CM_BSPModelInfo(n,f,c);}
void Fixture_CM_LoadMap(const char*,qboolean){CHECK(false);}
qhandle_t Fixture_R_RegisterModel(const char*name){drawNames.push_back(name);return int(drawNames.size()-1);}
void Fixture_R_ModelBounds(qhandle_t,vec3_t low,vec3_t high){VectorClear(low);VectorSet(high,1,1,1);}
#define JAR_BSP_CALL(name) Fixture_##name
#include "../../shared/qcommon/jar_bsp_client.inc"
#undef JAR_BSP_CALL
static void Reset(){
 cmg={};cmg.numSubModels=3;cmg.checksum=111;NumSubBSP=0;TotalSubModels=3;for(auto&m:SubBSP)m={};loads=links=0;linkedModel=-1;
 disk={{"maps/one.bsp",{2,211}},{"maps/two.bsp",{3,311}},{"maps/extra.bsp",{1,411}}};
 JarResetBSPRestore();configs.clear();drawNames={std::string()};cgs={};CG_ResetBSPModels();
}
static void Load(const char*name){int first,count;unsigned checksum;CHECK(CM_RegisterBSPModel(name,&first,&count,&checksum));}
static void Drop(const std::function<void()>&fn){bool rejected=false;try{fn();}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);}
int main(){return RunTests("BSP save identity, registration order, collision remap and client model lookup",[]{
 gi.Error=Com_Error;gi.BSPModelAt=CM_BSPModelAt;gi.RegisterBSPModel=CM_RegisterBSPModel;
 gi.linkentity=[](gentity_t*e){CHECK(!e->linked);e->linked=qtrue;++links;linkedModel=e->s.modelindex;};
 Reset();Load("#one");Load("#two");CHECK(loads==2&&NumSubBSP==2);Load("#ONE");CHECK(loads==2);
 char name[MAX_QPATH];int first=-1,count=-1;unsigned checksum=0;
 CHECK(CM_BSPModelAt(-1,name,sizeof(name),&first,&count,&checksum)==1&&!name[0]&&first==0&&count==3&&checksum==111);
 CHECK(CM_BSPModelAt(0,name,sizeof(name),&first,&count,&checksum)==1&&!strcmp(name,"#one")&&first==3&&count==2&&checksum==211);
 CHECK(CM_BSPModelAt(1,name,sizeof(name),&first,&count,&checksum)==1&&!strcmp(name,"#two")&&first==5&&count==3&&checksum==311);
 CHECK(CM_BSPModelAt(2,name,sizeof(name),&first,&count,&checksum)==0);
 CHECK(CM_BSPModelAt(-2,name,sizeof(name),&first,&count,&checksum)==-1);
 CHECK(CM_BSPModelAt(0,name,8,&first,&count,&checksum)==-1);
 for(const char*bad:{"#missing","#one:0","#one:1","#../one","one"})CHECK(!CM_RegisterBSPModel(bad,&first,&count,&checksum));CHECK(loads==2);
 ChunkSave saved;gi.saved_game=&saved;JarWriteBSPRegistry(false,jar::workshopSaveCurrent);
 CHECK(saved.chunks.size()==1&&saved.chunks[0].first==INT_ID('J','B','S','P')&&saved.chunks[0].second.size()==12+2*(MAX_QPATH+12));
 // A different registration order, including an unrelated BSP, changes every
 // sub-map's collision indices. Each saved root/inline child follows its name.
 Reset();Load("#two");Load("#extra");ChunkSave reordered;reordered.chunks=saved.chunks;gi.saved_game=&reordered;
 JarReadBSPRegistry(false,jar::workshopSaveCurrent);CHECK(jarRestoringBsp&&loads==3&&NumSubBSP==3);
 gentity_t brush{};brush.bmodel=qtrue;brush.model="*1";brush.s.modelindex=4;brush.s.modelindex2=99;brush.linked=qtrue;
 JarLinkRestoredEntity(&brush);CHECK(brush.linked&&linkedModel==8&&brush.s.modelindex==8&&brush.s.modelindex2==99&&!strcmp(brush.model,"*1"));
 brush.s.modelindex=3;brush.linked=qfalse;JarLinkRestoredEntity(&brush);CHECK(!brush.linked&&brush.s.modelindex==7&&links==1);
 brush.s.modelindex=7;brush.linked=qtrue;JarLinkRestoredEntity(&brush);CHECK(linkedModel==5);
 brush.s.modelindex=2;JarLinkRestoredEntity(&brush);CHECK(linkedModel==2);
 brush.bmodel=qfalse;brush.s.modelindex=4;JarLinkRestoredEntity(&brush);CHECK(linkedModel==4);
 // Saved configstring order differs from collision load order. Execute the
 // production client bridge and check which canonical draw model is selected.
 configs[CS_BSP_MODELS+1]="#one";configs[CS_BSP_MODELS+2]="#two";CG_RegisterBSPModels();
 CHECK(drawNames[cgs.inlineDrawModel[8]]=="#one:1"&&drawNames[cgs.inlineDrawModel[7]]=="#one:0"&&drawNames[cgs.inlineDrawModel[5]]=="#two:2");
 brush.bmodel=qtrue;brush.linked=qtrue;brush.s.modelindex=8;const int before=links;Drop([&]{JarLinkRestoredEntity(&brush);});CHECK(links==before);
 // Malformed records are all validated before any collision resource loads.
 auto bad=[&](const std::function<void(std::vector<byte>&)>&mutate){Reset();ChunkSave b;b.chunks=saved.chunks;mutate(b.chunks[0].second);gi.saved_game=&b;Drop([&]{JarReadBSPRegistry(false,jar::workshopSaveCurrent);});CHECK(!jarRestoringBsp&&loads==0);};
 auto word=[](std::vector<byte>&v,size_t at,uint32_t x){memcpy(v.data()+at,&x,4);};
 bad([&](auto&v){word(v,0,MAX_SUB_BSP+1);});bad([&](auto&v){word(v,4,0);});bad([&](auto&v){word(v,4,MAX_SUBMODELS+1);});
 bad([&](auto&v){word(v,8,999);}); // main source identity changed
 bad([&](auto&v){word(v,12+MAX_QPATH,4);}); // gap before first model
 bad([&](auto&v){word(v,12+MAX_QPATH+4,MAX_SUBMODELS);});
 bad([&](auto&v){memset(v.data()+12,'x',MAX_QPATH);});
 bad([&](auto&v){memset(v.data()+12,0,MAX_QPATH);memcpy(v.data()+12,"#../bad",8);});
 bad([&](auto&v){memset(v.data()+12,0,MAX_QPATH);memcpy(v.data()+12,"#one:0",7);});
 bad([&](auto&v){memset(v.data()+12+MAX_QPATH+12,0,MAX_QPATH);memcpy(v.data()+12+MAX_QPATH+12,"#ONE",5);});
 bad([&](auto&v){word(v,12+MAX_QPATH+12+MAX_QPATH,4);}); // overlapping range
 bad([](auto&v){v.pop_back();});bad([](auto&v){v.push_back(0);});
 // Missing/modified resources cannot publish a partial remap.
 for(int failure=0;failure<3;++failure){Reset();if(!failure)disk.erase("maps/one.bsp");else if(failure==1)disk["maps/one.bsp"].checksum++;else disk["maps/one.bsp"].models++;
  ChunkSave b;b.chunks=saved.chunks;gi.saved_game=&b;Drop([&]{JarReadBSPRegistry(false,jar::workshopSaveCurrent);});CHECK(!jarRestoringBsp);}
 // Autosaves carry no runtime world and must neither inspect nor register BSPs.
 Reset();Load("#two");ChunkSave automatic;gi.saved_game=&automatic;JarWriteBSPRegistry(true,jar::workshopSaveCurrent);CHECK(automatic.chunks[0].second==std::vector<byte>(12,0));
 JarReadBSPRegistry(true,jar::workshopSaveCurrent);CHECK(!jarRestoringBsp&&NumSubBSP==1&&loads==1);
 ChunkSave invalidAuto;invalidAuto.chunks=saved.chunks;gi.saved_game=&invalidAuto;Drop([&]{JarReadBSPRegistry(true,jar::workshopSaveCurrent);});CHECK(!jarRestoringBsp&&loads==1);
 // Old saves omit JBSP and preserve their native model interpretation.
 for(int version:{1,jar::actorRecoverySaveV1,jar::workshopSaveV39,jar::workshopSaveV40}){
  ChunkSave old;gi.saved_game=&old;JarWriteBSPRegistry(false,version);CHECK(old.chunks.empty());jarRestoringBsp=true;JarReadBSPRegistry(false,version);CHECK(!jarRestoringBsp);
  brush.bmodel=qtrue;brush.s.modelindex=4;brush.linked=qtrue;JarLinkRestoredEntity(&brush);CHECK(linkedModel==4);
 }
 // Registry errors are distinct from end-of-list. Do not silently write a
 // truncated registry when a loaded map cannot be represented or has no models.
 Reset();Load("#one");SubBSP[0].numSubModels=0;ChunkSave malformed;gi.saved_game=&malformed;Drop([&]{JarWriteBSPRegistry(false,jar::workshopSaveCurrent);});CHECK(malformed.chunks.empty());
 Reset();cmg.numSubModels=MAX_SUBMODELS-1;disk["maps/one.bsp"].models=1;Load("#one");CHECK(CM_BSPModelAt(0,name,sizeof(name),&first,&count,&checksum)==1&&first==MAX_SUBMODELS-1&&count==1);
 ChunkSave edge;gi.saved_game=&edge;JarWriteBSPRegistry(false,jar::workshopSaveCurrent);JarReadBSPRegistry(false,jar::workshopSaveCurrent);
 brush.s.modelindex=MAX_SUBMODELS-1;brush.linked=qtrue;JarLinkRestoredEntity(&brush);CHECK(linkedModel==MAX_SUBMODELS-1);
 CHECK(!CM_RegisterBSPModel("#two",&first,&count,&checksum));JarResetBSPRestore();gi.saved_game=nullptr;
 });}
