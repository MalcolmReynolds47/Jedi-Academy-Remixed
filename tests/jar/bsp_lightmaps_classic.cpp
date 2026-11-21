// Actual shared SP/MP loader; image creation and filesystem are fixture boundaries.
#include "qcommon/q_shared.h"
#include "qcommon/qfiles.h"
#include "qcommon/qcommon.h"
#include "test_support.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <deque>
#include <vector>
#include <set>
constexpr int MAX_LIGHTMAPS=256, GL_RGBA=1, GL_CLAMP=2;
constexpr int LIGHTMAP_NONE=-1, LIGHTMAP_BY_VERTEX=-3;
struct image_t {std::string name;std::vector<byte> pixels;};
struct world_t {int startLightMapIndex=0,numLightmaps=0;};
static world_t s_worldData;
static byte *fileBase;
struct {int numLightmaps=0,overbrightBits=0,sunSurfaceLight=0;image_t*lightmaps[MAX_LIGHTMAPS]{};vec3_t sunLight{},sunDirection{};} tr;
static cvar_t vertex{},debug{},compressed{},overbright{};
cvar_t *r_vertexLight=&vertex,*r_lightmap=&debug,*r_ext_compressed_lightmaps=&compressed,*r_mapOverBrightBits=&overbright;
static std::deque<image_t> images;
static std::set<std::string> external;
static std::vector<std::string> reads;
static int flushes=0;
static void QDECL Print(int,const char*,...){}
struct {void(QDECL *Printf)(int,const char*,...)=Print;} ri;
void R_IssuePendingRenderCommands(){++flushes;}
image_t*R_CreateImage(const char*name,const byte*data,int w,int h,int,qboolean,qboolean,qboolean,int){
 CHECK(w==128&&h==128&&data);images.push_back({name,{data,data+w*h*4}});return &images.back();
}
image_t*R_FindImageFile(const char*name,qboolean,qboolean,qboolean,int){reads.emplace_back(name);if(!external.count(name))return nullptr;images.push_back({name,{}});return &images.back();}
#include "../../code/rd-common/tr_bsp_lightmaps.inc"
struct Lump {
 std::vector<byte> bytes;
 lump_t lighting{},surfaces{};
 Lump(int tiles,std::initializer_list<int> indices):bytes(sizeof(dsurface_t)+tiles*128*128*3,60){
  auto *s=(dsurface_t*)bytes.data();memset(s,0,sizeof(*s));for(int&i:s->lightmapNum)i=LIGHTMAP_NONE;
  int k=0;for(int i:indices)s->lightmapNum[k++]=i;
  surfaces={0,sizeof(dsurface_t)};lighting={sizeof(dsurface_t),tiles*128*128*3};
 }
 void load(world_t &w,const char*name){fileBase=bytes.data();R_LoadLightmaps(&lighting,&surfaces,name,w);}
};
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}CHECK(failed);}
int main(int argc,char**argv){return RunTests("classic BSP lightmap bytes, append, external ownership, bounds and reset",[&]{
 Lump one(1,{0});one.load(s_worldData,"maps/main.bsp");CHECK(tr.numLightmaps==1&&images.size()==1&&flushes==1);
 auto *main=tr.lightmaps[0];CHECK(main->pixels.size()==128*128*4&&main->pixels.back()==255&&main->pixels[0]==60);
 world_t child;Lump two(2,{0,1});two.load(child,"maps/mp/child.bsp");CHECK(tr.numLightmaps==3&&child.startLightMapIndex==1&&child.numLightmaps==2&&tr.lightmaps[0]==main);
 CHECK(R_BSPLightmapIndex(1,child)==2&&R_BSPLightmapIndex(2,child)==LIGHTMAP_BY_VERTEX&&R_BSPLightmapIndex(LIGHTMAP_NONE,child)==LIGHTMAP_NONE);
 world_t ext;external.insert("maps/mp/external/lm_0001.tga");Lump outside(0,{0,1});outside.load(ext,"maps/mp/external.bsp");
 CHECK(ext.startLightMapIndex==3&&tr.numLightmaps==5&&!tr.lightmaps[3]&&tr.lightmaps[4]);CHECK(reads[1]=="maps/mp/external/lm_0001.tga"&&tr.lightmaps[0]==main);
 world_t empty;Lump noMaps(0,{});noMaps.load(empty,"maps/empty.bsp");CHECK(!empty.numLightmaps&&tr.numLightmaps==5);
 vertex.integer=1;world_t low;two.load(low,"maps/vertex.bsp");CHECK(tr.numLightmaps==7&&!tr.lightmaps[5]&&!tr.lightmaps[6]);vertex.integer=0;
 world_t invalid;one.lighting.filelen-=1;rejects([&]{one.load(invalid,"maps/bad.bsp");});CHECK(tr.numLightmaps==7);one.lighting.filelen+=1;
 tr.numLightmaps=MAX_LIGHTMAPS;rejects([&]{one.load(invalid,"maps/full.bsp");});CHECK(tr.numLightmaps==MAX_LIGHTMAPS);
 one.load(s_worldData,"maps/restart.bsp");CHECK(tr.numLightmaps==1&&s_worldData.startLightMapIndex==0&&tr.lightmaps[0]!=main);
 tr.sunLight[0]=3;tr.sunDirection[1]=4;tr.sunSurfaceLight=5;{JAR_ClassicBSPLightingScope scope(true);tr.sunLight[0]=9;tr.sunDirection[1]=8;tr.sunSurfaceLight=7;}
 CHECK(tr.sunLight[0]==3&&tr.sunDirection[1]==4&&tr.sunSurfaceLight==5);
 if(argc>1){
  tr={};images.clear();world_t retailChild;image_t *mainMap=nullptr;std::vector<byte> mainBytes;
  for(int n=1;n<argc;++n){
   std::ifstream input(argv[n],std::ios::binary);CHECK(input.good());std::vector<byte> data((std::istreambuf_iterator<char>(input)),{});
   CHECK(data.size()>=sizeof(dheader_t));auto *head=(dheader_t*)data.data();CHECK(head->version==BSP_VERSION);
   for(const auto &l:head->lumps)CHECK(l.fileofs>=0&&l.filelen>=0&&size_t(l.fileofs)+l.filelen<=data.size());
   fileBase=data.data();world_t &w=n==1?s_worldData:retailChild;
   R_LoadLightmaps(&head->lumps[LUMP_LIGHTMAPS],&head->lumps[LUMP_SURFACES],n==1?"maps/retail_main.bsp":"maps/retail_child.bsp",w);
   if(n==1){CHECK(tr.numLightmaps>0&&tr.lightmaps[0]);mainMap=tr.lightmaps[0];mainBytes=mainMap->pixels;}
   else CHECK(tr.lightmaps[0]==mainMap&&mainMap->pixels==mainBytes&&w.startLightMapIndex>0);
   printf("Retail classic BSP %d: %d owned lightmaps, first %d, aggregate %d\n",n,w.numLightmaps,w.startLightMapIndex,tr.numLightmaps);
  }
 }

 });}
