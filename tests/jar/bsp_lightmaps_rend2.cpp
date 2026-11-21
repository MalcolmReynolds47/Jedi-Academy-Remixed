// Actual Rend2 decoding, atlas packing and registry; disk decoders/GPU are fixtures.
#include "tr_local.h"
#include "test_support.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <vector>
trGlobals_t tr;
refimport_t ri{};
glconfig_t glConfig{};
glRefConfig_t glRefConfig{};
static world_t s_worldData;
static byte *fileBase;
static cvar_t hdr{},deluxe{},debug{},overbright{};
cvar_t *r_hdr=&hdr,*r_deluxeMapping=&deluxe,*r_lightmap=&debug,*r_mapOverBrightBits=&overbright;
static std::vector<void*> hunks;
static std::set<void*> temporary;
void *Hunk_Alloc(int size,ha_pref){void*p=calloc(1,size);hunks.push_back(p);return p;}
void *Hunk_AllocateTempMemory(int size){void*p=malloc(size);temporary.insert(p);return p;}
void Hunk_FreeTempMemory(void*p){CHECK(temporary.erase(p)==1);free(p);}
static void QDECL Print(int,const char*,...){}
void R_IssuePendingRenderCommands(){}
struct Image {std::string name;int width,height,bpp;std::vector<byte> data;};
static std::map<image_t*,Image> images;
static std::vector<std::unique_ptr<image_t>> handles;
image_t *R_CreateImage(const char*name,byte*data,int w,int h,imgType_t,int,int format){
 CHECK(w>0&&h>0&&w<=glConfig.maxTextureSize&&h<=glConfig.maxTextureSize);
 for(const auto&entry:images)CHECK(entry.second.name!=name);
 handles.emplace_back(new image_t{});auto *p=handles.back().get();const int bpp=format==GL_RGBA16F?8:4;
 images[p]={name,w,h,bpp,std::vector<byte>(w*h*bpp,0xfe)};
 if(data)memcpy(images[p].data.data(),data,w*h*bpp);return p;
}
void R_UpdateSubImage(image_t*p,byte*data,int x,int y,int w,int h){
 CHECK(p&&data);auto &im=images.at(p);CHECK(x>=0&&y>=0&&x+w<=im.width&&y+h<=im.height);
 for(int j=0;j<h;++j)memcpy(im.data.data()+((y+j)*im.width+x)*im.bpp,data+j*w*im.bpp,w*im.bpp);
}
struct External {bool hdr;int w,h;byte value;};
static std::map<std::string,External> external;
static std::vector<std::string> reads;
void R_LoadHDRImage(const char*name,byte**data,int*w,int*h){
 reads.emplace_back(name);*data=nullptr;auto at=external.find(name);if(at==external.end()||!at->second.hdr)return;
 *w=at->second.w;*h=at->second.h;float*p=(float*)Hunk_AllocateTempMemory(*w**h*3*sizeof(float));
 for(int j=0;j<*w**h*3;++j)p[j]=float(j+1);*data=(byte*)p;
}
void R_LoadImage(const char*name,byte**data,int*w,int*h){
 reads.emplace_back(name);*data=nullptr;auto at=external.find(name);if(at==external.end()||at->second.hdr)return;
 *w=at->second.w;*h=at->second.h;*data=(byte*)malloc(*w**h*4);memset(*data,at->second.value,*w**h*4);
}
#include "tr_bsp_lighting_registry.inc"
#include "../../shared/rd-rend2/tr_bsp_lightmaps.inc"
#include "tr_bsp_lightmap_lookup.h"
struct Lump {
 std::vector<byte> bytes;lump_t lighting{},surfaces{};
 Lump(int tiles,std::initializer_list<int> indices):bytes(sizeof(dsurface_t)+tiles*128*128*3,60){
  auto*s=(dsurface_t*)bytes.data();memset(s,0,sizeof(*s));for(int&i:s->lightmapNum)i=LIGHTMAP_NONE;int n=0;for(int i:indices)s->lightmapNum[n++]=i;
  surfaces={0,sizeof(dsurface_t)};lighting={sizeof(dsurface_t),tiles*128*128*3};
 }
 void load(world_t&w,const char*name,int slot){
  Q_strncpyz(w.name,name,sizeof(w.name));w.lightmapNamespace=slot;fileBase=bytes.data();JAR_BSPLightingScope scope(&w,&w!=&s_worldData);R_LoadLightmaps(&w,&lighting,&surfaces);
 }
};
template<class F>void rejects(F f,const char*message){bool failed=false;try{f();}catch(const std::runtime_error&e){failed=strstr(e.what(),message)!=nullptr;}CHECK(failed);}
int main(int argc,char**argv){return RunTests("Rend2 BSP owned lightmaps, mixed HDR/deluxe, packing, bytes, bounds and reset",[&]{
 ri.Error=Com_Error;ri.Printf=Print;glConfig.maxTextureSize=4096;
 Lump root(2,{0,1});root.load(s_worldData,"maps/main.bsp",0);CHECK(tr.numLightmaps==1&&s_worldData.lighting.numLightmaps==1);
 auto*main=tr.lightmaps[0];const auto mainPixels=images[main].data;
 CHECK(FatLightmap(&s_worldData,1)==0&&FatLightmap(&s_worldData,2)==LIGHTMAP_BY_VERTEX);
 CHECK(FatPackU(&s_worldData,.5f,1)==.75f&&FatPackV(&s_worldData,.5f,1)==.5f);
 world_t child{};Lump paired(4,{0,2});paired.load(child,"maps/mp/child.bsp",1);
 CHECK(tr.numLightmaps==2&&child.lighting.firstLightmap==1&&FatLightmap(&child,2)==1&&R_BSPDeluxemap(1)&&!R_BSPDeluxemap(0));
 CHECK(child.lighting.worldInternalDeluxeMapping&&!s_worldData.lighting.worldInternalDeluxeMapping);
 CHECK(tr.lightmaps[0]==main&&images[main].data==mainPixels&&FatPackU(&child,.5f,2)==.75f);
 world_t single{};Lump one(1,{0});one.load(single,"maps/one.bsp",2);CHECK(tr.numLightmaps==3&&!single.lighting.worldInternalDeluxeMapping&&!R_BSPDeluxemap(2));
 CHECK(images[tr.lightmaps[2]].data.back()==255&&FatPackU(&single,.5f,0)==.5f);
 // Rectangular external HDR retains every pixel and releases decoder memory correctly.
 hdr.integer=1;glRefConfig.floatLightmap=qtrue;external["maps/mp/hdr/lm_0000.hdr"]={true,2,3,0};
 world_t high{};Lump outside(0,{0});outside.load(high,"maps/mp/hdr.bsp",3);
 CHECK(tr.numLightmaps==4&&R_BSPLightmapHDR(3)&&!tr.hdrLighting&&!R_BSPLightmapHDR(0)&&temporary.empty());
 auto &hi=images[tr.lightmaps[3]];CHECK(hi.width==2&&hi.height==3&&hi.bpp==8);
 auto *half=(const unsigned short*)hi.data.data();for(int p=0;p<6;++p)for(int c=0;c<3;++c)CHECK(half[p*4+c]==FloatToHalf(float(p*3+c+1)));
 CHECK(FatPackU(&high,.7f,0)==.7f&&FatPackV(&high,.3f,0)==.3f);
 // Missing HDR falls back to this map's TGA without inheriting HDR or packing flags.
 external["maps/tga/lm_0000.tga"]={false,2,3,40};world_t low{};outside.load(low,"maps/tga.bsp",4);
 CHECK(tr.numLightmaps==5&&tr.lightmaps[4]&&!R_BSPLightmapHDR(4)&&!low.lighting.worldInternalLightmapping);
 world_t missing{};outside.load(missing,"maps/missing.bsp",5);CHECK(tr.numLightmaps==6&&!tr.lightmaps[5]&&!R_BSPDeluxemap(5));
 // Partial deluxe atlases initialize absent tiles to a neutral direction.
 hdr.integer=0;glRefConfig.floatLightmap=qfalse;deluxe.integer=1;
 external["maps/partial/dm_0001.tga"]={false,128,128,9};world_t partial{};root.load(partial,"maps/partial.bsp",6);
 CHECK(R_BSPDeluxemap(6));auto &dm=images[R_BSPDeluxemap(6)];CHECK(dm.data[0]==127&&dm.data[128*4]==9);
 CHECK(images[main].data==mainPixels&&FatLightmap(&high,0)==3);
 const int before=tr.numLightmaps;world_t invalid{};one.lighting.filelen--;rejects([&]{one.load(invalid,"maps/bad.bsp",7);},"Malformed");one.lighting.filelen++;
 CHECK(tr.numLightmaps==before);glConfig.maxTextureSize=128;rejects([&]{root.load(invalid,"maps/large.bsp",7);},"GPU texture size");glConfig.maxTextureSize=4096;
 invalid.lighting.numLightmaps=JAR_BSP_LIGHTMAP_LIMIT;rejects([&]{R_PublishBSPLightmaps(&invalid);},"capacity");CHECK(tr.numLightmaps==before);
 tr.sunLight[0]=3;tr.sunDirection[1]=4;tr.autoExposureMinMax[0]=5;tr.toneMinAvgMaxLevel[2]=6;tr.distanceCull=7;
 {JAR_BSPLightingScope scope(&high,true);CHECK(R_BSPLightmapHDR(LIGHTMAP_NONE));tr.sunLight[0]=8;tr.sunDirection[1]=9;tr.autoExposureMinMax[0]=10;tr.toneMinAvgMaxLevel[2]=11;tr.distanceCull=12;}
 CHECK(!tr.loadingLightmaps&&tr.sunLight[0]==3&&tr.sunDirection[1]==4&&tr.autoExposureMinMax[0]==5&&tr.toneMinAvgMaxLevel[2]==6&&tr.distanceCull==7);
 CHECK(!R_BSPDeluxemap(-1)&&!R_BSPDeluxemap(9999)&&!R_BSPLightmapHDR(LIGHTMAP_2D));
 // Map restart discards prior registry entries, even when the new map has no maps.
 Lump empty(0,{});empty.load(s_worldData,"maps/empty.bsp",0);CHECK(tr.numLightmaps==0&&!tr.lightmaps&&!tr.deluxemaps&&!tr.hdrLighting);
 if(argc>1){
  tr={};images.clear();handles.clear();external.clear();deluxe.integer=0;hdr.integer=0;glRefConfig.floatLightmap=qfalse;
  world_t retailChild{};image_t *mainMap=nullptr;std::vector<byte> mainBytes;
  for(int n=1;n<argc;++n){
   std::ifstream input(argv[n],std::ios::binary);CHECK(input.good());std::vector<byte> data((std::istreambuf_iterator<char>(input)),{});
   CHECK(data.size()>=sizeof(dheader_t));auto *head=(dheader_t*)data.data();CHECK(head->version==BSP_VERSION);
   for(const auto &l:head->lumps)CHECK(l.fileofs>=0&&l.filelen>=0&&size_t(l.fileofs)+l.filelen<=data.size());
   fileBase=data.data();world_t &w=n==1?s_worldData:retailChild;w.lightmapNamespace=n-1;
   Q_strncpyz(w.name,n==1?"maps/retail_main.bsp":"maps/retail_child.bsp",sizeof(w.name));
   JAR_BSPLightingScope scope(&w,n!=1);R_LoadLightmaps(&w,&head->lumps[LUMP_LIGHTMAPS],&head->lumps[LUMP_SURFACES]);
   if(n==1){CHECK(tr.numLightmaps>0&&tr.lightmaps[0]);mainMap=tr.lightmaps[0];mainBytes=images[mainMap].data;}
   else CHECK(tr.lightmaps[0]==mainMap&&images[mainMap].data==mainBytes&&w.lighting.firstLightmap>0);
   auto *surfaces=(dsurface_t*)(data.data()+head->lumps[LUMP_SURFACES].fileofs);
   for(unsigned k=0;k<head->lumps[LUMP_SURFACES].filelen/sizeof(*surfaces);++k)for(int index:surfaces[k].lightmapNum)
    if(index>=0&&index<w.lighting.rawLightmaps)CHECK(FatLightmap(&w,index)>=w.lighting.firstLightmap&&FatLightmap(&w,index)<tr.numLightmaps);
   printf("Retail Rend2 BSP %d: %d source lightmaps, %d images, first %d, aggregate %d\n",n,w.lighting.rawLightmaps,w.lighting.numLightmaps,w.lighting.firstLightmap,tr.numLightmaps);
  }
 }

 for(void*p:hunks)free(p);hunks.clear();CHECK(temporary.empty());
 });}
