// Production grid loader, interpolation and scene ownership, native structures.
#ifdef JAR_GRID_REND2
#include "../../shared/rd-rend2/tr_local.h"
#elif defined(JAR_CLASSIC_SP)
#include "../../code/rd-vanilla/tr_local.h"
#else
#include "../../codemp/rd-vanilla/tr_local.h"
#endif
#include "test_support.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>
#if !defined(JAR_CLASSIC_SP) && !defined(JAR_GRID_REND2)
#include "../../codemp/qcommon/q_shared.c"
void QDECL Com_Error(int,const char *format,...){char message[2048];va_list args;va_start(args,format);vsnprintf(message,sizeof(message),format,args);va_end(args);throw std::runtime_error(message);}
void QDECL Com_Printf(const char*,...){}
#endif
trGlobals_t tr;
refimport_t ri{};
color4ub_t styleColors[MAX_LIGHT_STYLES];
static byte *fileBase;
static cvar_t fullbright{},ambient{},directed{},debug{},hdr{};
cvar_t *r_fullbright=&fullbright,*r_ambientScale=&ambient,*r_directedScale=&directed,*r_debugLight=&debug,*r_hdr=&hdr;
static model_t models[4]{};
model_t *R_GetModelByHandle(qhandle_t h){return h>=0&&h<4?&models[h]:nullptr;}
struct Allocation{byte*data;int bytes;};
static std::vector<Allocation> allocations;
static void *allocate(int size){CHECK(size>0);byte*p=(byte*)malloc(size+32);memset(p,0xcd,size+32);allocations.push_back({p,size});return p+16;}
#ifdef JAR_CLASSIC_SP
void *R_Hunk_Alloc(int size,qboolean){return allocate(size);}
void R_ColorShiftLightingBytes(byte*){}
void RE_AddRefEntityToScene(const refEntity_t*){}
qhandle_t RE_RegisterShader(const char*){return 1;}
#else
void *Hunk_Alloc(int size,ha_pref){return allocate(size);}
void R_ColorShiftLightingBytes(byte *in,byte*out){if(in!=out)memcpy(out,in,3);}
#endif
#ifdef JAR_GRID_REND2
static std::string requested;static std::vector<float> hdrFile;static int hdrFrees=0;
static long readHDR(const char*name,void **data){requested=name;*data=hdrFile.empty()?nullptr:hdrFile.data();return int(hdrFile.size()*sizeof(float));}
static void freeHDR(void*){++hdrFrees;}
#endif
#include "../../code/rd-common/tr_bsp_grid_load.inc"
#include "../../code/rd-common/tr_bsp_grid_sample.inc"
static bool closeEnough(float a,float b){return fabs(a-b)<0.001f;}
template<class F>static void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}CHECK(caught);}
static void bindModel(int handle,world_t *world,int index){models[handle].type=MOD_BRUSH;
#ifdef JAR_GRID_REND2
 models[handle].data.bmodel=world->bmodels;world->bmodels[0].worldIndex=index;
#else
 models[handle].bmodel=world->bmodels;world->bmodels[0].bspIndex=index;
#endif
}
struct Fixture{
 std::vector<byte> bytes; lump_t samples{},array{},entities{};bmodel_t model[2]{};world_t world{};
 Fixture(int x=2,int y=2,int z=2):bytes(x*y*z*(sizeof(mgrid_t)+2),0){
  samples={0,int(x*y*z*sizeof(mgrid_t))};array={samples.filelen,x*y*z*2};
  strcpy(world.name,"maps/mp/grid.bsp");world.bmodels=model;VectorSet(world.lightGridSize,10,10,10);VectorSet(model[0].bounds[1],(x-1)*10,(y-1)*10,(z-1)*10);
  for(int i=0;i<x*y*z;++i){auto &cell=((mgrid_t*)bytes.data())[i];memset(cell.styles,255,MAXLIGHTMAPS);cell.styles[0]=0;cell.ambientLight[0][0]=10+i*10;cell.directLight[0][1]=20+i*10;cell.latLong[0]=64;unsigned short index=i;memcpy(bytes.data()+array.fileofs+i*2,&index,2);}
 }
 void load(){fileBase=bytes.data();R_LoadBSPGrid(&world,&samples,&array);}
};
int main(int argc,char**argv){return RunTests("BSP grids: native loading, edges, styled/HDR interpolation and transformed instances",[&]{
 ambient.value=directed.value=1;memset(styleColors,255,sizeof(styleColors));
 for(int i=0;i<FUNCTABLE_SIZE;++i)tr.sinTable[i]=sinf(i*2*float(M_PI)/FUNCTABLE_SIZE);
#ifdef JAR_GRID_REND2
 ri.FS_ReadFile=readHDR;ri.FS_FreeFile=freeHDR;
#endif
 Fixture root;root.load();tr.world=&root.world;CHECK(root.world.numGridArrayElements==8&&root.world.numGridDataElements==8);
 trRefEntity_t sample{};VectorSet(sample.e.origin,5,5,5);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],45)&&closeEnough(sample.directedLight[1],55)&&closeEnough(sample.lightDir[0],1));
 // High X must remain on that row rather than blending into the next row.
 VectorSet(sample.e.origin,12,0,0);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],20));
 VectorSet(sample.e.origin,-2,0,0);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],10));
 VectorSet(sample.e.origin,1e30f,1e30f,1e30f);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],80));
 sample.e.origin[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!R_SetupEntityLightingGrid(&sample));
 Fixture thin(1,2,1);thin.load();VectorSet(sample.e.origin,100,5,100);CHECK(R_SetupEntityLightingGrid(&sample,&thin.world));CHECK(closeEnough(sample.ambientLight[0],15));
 VectorSet(sample.e.origin,5,5,5);root.world.lightGridData[0].styles[0]=LS_LSNONE;CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],50));root.world.lightGridData[0].styles[0]=0;
 root.world.lightGridData[0].styles[1]=1;root.world.lightGridData[0].ambientLight[1][0]=20;styleColors[1][0]=127;
 VectorClear(sample.e.origin);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],10+20*127/255.0f));root.world.lightGridData[0].styles[1]=255;
 Fixture child;child.load();for(int i=0;i<8;++i)child.world.lightGridData[i].ambientLight[0][0]+=100;
#ifdef JAR_GRID_REND2
 tr.bspModels[0]=&child.world;tr.numBspModels=1;world_t *placed=&child.world;bindModel(1,placed,0);
#else
 tr.bspModels[0]=child.world;tr.numBSPModels=1;world_t *placed=&tr.bspModels[0];bindModel(1,placed,1);
#endif
 trRefEntity_t instances[2]{};for(auto &e:instances){e.e.reType=RT_MODEL;e.e.hModel=1;VectorSet(e.e.origin,100,200,300);e.e.axis[0][1]=2;e.e.axis[1][0]=-1;e.e.axis[2][2]=1;}
 instances[1].e.origin[0]=500;trRefdef_t scene{};scene.entities=instances;scene.num_entities=2;R_BuildBSPGridInstances(&scene);CHECK(bspGridInstances.size()==2);
 VectorSet(sample.e.origin,95,210,305);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],145)&&closeEnough(sample.lightDir[1],1));
 VectorSet(sample.e.origin,495,210,305);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],145));
 sample.e.renderfx=RF_LIGHTING_ORIGIN;VectorSet(sample.e.lightingOrigin,0,0,0);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],10));sample.e.renderfx=0;
 // Registry owns transforms; a caller changing the original struct cannot stale it.
 instances[0].e.origin[0]=0;VectorSet(sample.e.origin,95,210,305);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],145));
 R_BuildBSPGridInstances(&scene);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],80));instances[0].e.origin[0]=100;
 // Smallest containing section wins; a degenerate or sheared transform is ignored.
 instances[1]=instances[0];instances[1].e.axis[0][1]=1;R_BuildBSPGridInstances(&scene);vec3_t local{},point{95,205,305};CHECK(R_BSPGridAtPoint(point,local)==&bspGridInstances[1]);
 instances[1].e.axis[0][1]=0;R_BuildBSPGridInstances(&scene);CHECK(bspGridInstances.size()==1);
 instances[1].e.axis[0][0]=1;instances[1].e.axis[0][1]=1;R_BuildBSPGridInstances(&scene);CHECK(bspGridInstances.size()==1);
 instances[1]=instances[0];instances[1].e.origin[0]=500;instances[1].e.axis[0][1]=-2;R_BuildBSPGridInstances(&scene);
 VectorSet(sample.e.origin,495,190,305);CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],145)&&closeEnough(sample.lightDir[1],-1));
 // Moving inline brushes must not claim a whole section's grid.
 models[2]=models[1];
#ifdef JAR_GRID_REND2
 child.model[1].worldIndex=0;models[2].data.bmodel=&child.model[1];
#else
 child.model[1].bspIndex=1;models[2].bmodel=&child.model[1];
#endif
 instances[1].e.hModel=2;R_BuildBSPGridInstances(&scene);CHECK(bspGridInstances.size()==1);
 VectorSet(sample.e.origin,95,210,305);
 for(int i=0;i<8;++i)placed->lightGridData[i].styles[0]=255;CHECK(R_SetupEntityLightingGrid(&sample));CHECK(closeEnough(sample.ambientLight[0],80));
 R_ClearBSPGridInstances();CHECK(bspGridInstances.empty());scene.rdflags=RDF_NOWORLDMODEL;R_BuildBSPGridInstances(&scene);CHECK(bspGridInstances.empty());scene.rdflags=0;
 fullbright.integer=1;CHECK(R_SetupEntityLightingGrid(&sample));CHECK(sample.ambientLight[0]==255);fullbright.integer=0;
#ifdef JAR_CLASSIC_SP
 tr.refdef.rdflags=RDF_doLAGoggles;CHECK(R_SetupEntityLightingGrid(&sample)&&sample.ambientLight[0]==255);tr.refdef.rdflags=0;
#elif defined(JAR_GRID_REND2)
 tr.refdef.doLAGoggles=true;CHECK(R_SetupEntityLightingGrid(&sample)&&sample.ambientLight[0]==255);tr.refdef.doLAGoggles=false;
#endif
 // Rejection occurs before any grid allocation is exposed.
 Fixture bad;bad.array.filelen-=2;rejects([&]{bad.load();});CHECK(!bad.world.lightGridData);
 bad=Fixture();bad.world.bmodels=bad.model;bad.bytes[bad.array.fileofs]=99;rejects([&]{bad.load();});
 bad=Fixture();bad.world.bmodels=bad.model;bad.world.lightGridSize[0]=0;rejects([&]{bad.load();});
 bad=Fixture();bad.world.bmodels=bad.model;bad.world.lightGridSize[0]=std::numeric_limits<float>::infinity();rejects([&]{bad.load();});
 bad=Fixture();bad.world.bmodels=bad.model;bad.world.bmodels[0].bounds[1][0]=1e30f;rejects([&]{bad.load();});
 bad=Fixture();bad.world.bmodels=bad.model;bad.samples.filelen-=1;rejects([&]{bad.load();});
 bad=Fixture();bad.world.bmodels=bad.model;((mgrid_t*)bad.bytes.data())->styles[0]=MAX_LIGHT_STYLES;rejects([&]{bad.load();});CHECK(!bad.world.lightGridData);
 world_t unlit{};lump_t empty{};R_LoadBSPGrid(&unlit,&empty,&empty);CHECK(!R_ValidBSPGrid(&unlit));
 const char config[]="{ \"classname\" \"worldspawn\" \"gridsize\" \"16 32 64\" \"ambient\" \"999\" }";
 fileBase=(byte*)config;lump_t ent{0,sizeof(config)-1};R_ReadBSPGridSize(&unlit,&ent);CHECK(unlit.lightGridSize[0]==16&&unlit.lightGridSize[1]==32&&unlit.lightGridSize[2]==64);
#ifdef JAR_GRID_REND2
 hdr.integer=1;hdrFile.assign(8*6,0.5f);Fixture high;high.load();CHECK(requested=="maps/mp/grid/lightgrid.raw"&&hdrFrees==1);VectorSet(sample.e.origin,5,5,5);CHECK(R_SetupEntityLightingGrid(&sample,&high.world));CHECK(closeEnough(sample.ambientLight[0],127.5f));
 hdrFile[0]=std::numeric_limits<float>::quiet_NaN();rejects([&]{high.load();});CHECK(hdrFrees==2);hdrFile.pop_back();rejects([&]{high.load();});CHECK(hdrFrees==3);hdr.integer=0;hdrFile.clear();
#endif
 // Real BSP mode reads native structs and executes the same loader and sampler.
 for(int arg=1;arg<argc;++arg){
  std::ifstream input(argv[arg],std::ios::binary);std::vector<byte> bytes((std::istreambuf_iterator<char>(input)),{});CHECK(bytes.size()>=sizeof(dheader_t));auto*h=(dheader_t*)bytes.data();CHECK(h->version==BSP_VERSION);
  for(auto&l:h->lumps)CHECK(l.fileofs>=0&&l.filelen>=0&&size_t(l.fileofs)+l.filelen<=bytes.size());
  auto&ml=h->lumps[LUMP_MODELS];CHECK(ml.filelen>=sizeof(dmodel_t));const auto&model=*(dmodel_t*)(bytes.data()+ml.fileofs);bmodel_t brush{};world_t map{};map.bmodels=&brush;strcpy(map.name,"maps/retail.bsp");VectorCopy(model.mins,brush.bounds[0]);VectorCopy(model.maxs,brush.bounds[1]);fileBase=bytes.data();
  R_ReadBSPGridSize(&map,&h->lumps[LUMP_ENTITIES]);R_LoadBSPGrid(&map,&h->lumps[LUMP_LIGHTGRID],&h->lumps[LUMP_LIGHTARRAY]);
  if(R_ValidBSPGrid(&map))for(int corner=0;corner<8;++corner){for(int axis=0;axis<3;++axis)sample.e.origin[axis]=brush.bounds[(corner>>axis)&1][axis];R_SetupEntityLightingGrid(&sample,&map);for(int axis=0;axis<3;++axis)CHECK(std::isfinite(sample.ambientLight[axis]));}
  printf("Retail grid: %s, %d samples, %d cells\n",argv[arg],map.numGridDataElements,map.numGridArrayElements);
 }
 for(auto &a:allocations){for(int i=0;i<16;++i)CHECK(a.data[i]==0xcd&&a.data[16+a.bytes+i]==0xcd);free(a.data);}allocations.clear();
 });}
