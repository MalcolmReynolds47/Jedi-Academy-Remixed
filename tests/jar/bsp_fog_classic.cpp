// Production classic fog loader, draw lookup and texcoords. No GL calls.
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "qcommon/qfiles.h"
#include "test_support.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>

struct fogParms_t { vec3_t color; float depthForOpaque; };
struct fog_t {
 int originalBrushNumber; vec3_t bounds[2]; unsigned colorInt; float tcScale;
 fogParms_t parms; qboolean hasSurface; float surface[4];
};
struct shader_t { fogParms_t *fogParms; };
struct bmodel_t {int bspIndex;vec3_t bounds[2];};
struct mnode_t {int contents,cluster,area;cplane_t *plane;mnode_t *children[2];};
struct world_t {
 char name[MAX_QPATH]{}; int numplanes=0; cplane_t *planes=nullptr;
 int numfogs=0; fog_t *fogs=nullptr; int globalFog=-1,inheritedFog=-1;
 bmodel_t *bmodels=nullptr;mnode_t *nodes=nullptr;int numDecisionNodes=0;
};
enum {RT_MODEL,RT_SPRITE,MOD_BAD,MOD_BRUSH,MOD_MESH};
struct refEntity_t {int reType=RT_MODEL,hModel=0,renderfx=0;vec3_t origin{},lightingOrigin{};vec3_t axis[3]{};};
struct trRefEntity_t {refEntity_t e;const trRefEntity_t *riftFogRoom=nullptr;int riftFogIndex=0;};
struct trRefdef_t {trRefEntity_t *entities;int num_entities,rdflags;};
enum {CONTENTS_NODE=-1,RDF_NOWORLDMODEL=1,RDF_SKYBOXPORTAL=8,RDF_NOFOG=64,RF_LIGHTING_ORIGIN=128,RF_JAR_SECTION_FOG=0x20000000};
struct model_t {int type; bmodel_t *bmodel;};
struct {world_t *world=nullptr; world_t bspModels[3];int numBSPModels=0;trRefEntity_t worldEntity;float identityLight=0.5f;} tr;
struct Orientation {vec3_t origin{},axis[3]{},viewOrigin{};float modelMatrix[16]{};};
struct {trRefEntity_t *currentEntity=nullptr;struct {bool doLAGoggles=false;}refdef;Orientation ori;struct{Orientation ori;}viewParms;}backEnd;
struct {int fogNum=0,numVertexes=0;float xyz[4][4]{};}tess;
static model_t models[3]{};
model_t *R_GetModelByHandle(int handle){return handle>=0&&handle<3?&models[handle]:nullptr;}
static byte *fileBase;
static const int LIGHTMAP_NONE=-1;
static const byte stylesDefault[MAXLIGHTMAPS]{};
static fogParms_t shaderFog{{0.2f,0.4f,0.8f},256.0f};
shader_t *R_FindShader(const char*,const int*,const byte*,qboolean){static shader_t s{&shaderFog};return &s;}
struct Allocation {byte *data;int size;};
static std::vector<Allocation> allocations;
static void *Allocate(int size){
 byte*p=(byte*)malloc(size+32);memset(p,0xcd,size+32);allocations.push_back({p,size});return p+16;
}
static void *R_Hunk_Alloc(int size,qboolean){return Allocate(size);}
enum ha_pref {h_low};
static void *Hunk_Alloc(int size,ha_pref){return Allocate(size);}
bool RB_BSPFogIsGlobal(int index,bool includeSpecial=false);
#include "../../code/rd-common/tr_bsp_fogs.inc"
#include "../../code/rd-common/tr_bsp_fog_context.inc"
#include "../../code/rd-common/tr_bsp_fog_coords.inc"
#ifdef JAR_CLASSIC_SP
#include "../../code/rd-common/tr_rift_fog.inc"
#endif
static bool near(float a,float b){return fabs(a-b)<0.0001f;}
template<class F>static void rejects(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}CHECK(failed);}
static void checkAllocations(){for(const auto &a:allocations)for(int j=0;j<16;++j)CHECK(a.data[j]==0xcd&&a.data[16+a.size+j]==0xcd);}
struct Fixture {
 std::vector<byte> bytes; lump_t fog{},brush{},side{};cplane_t planes[6]{};
 Fixture(int count,bool global):bytes(count*sizeof(dfog_t)+sizeof(dbrush_t)+6*sizeof(dbrushside_t),0){
  fog={0,int(count*sizeof(dfog_t))};brush={fog.filelen,sizeof(dbrush_t)};side={brush.fileofs+brush.filelen,6*sizeof(dbrushside_t)};
  auto*b=(dbrush_t*)(bytes.data()+brush.fileofs);b->firstSide=0;b->numSides=6;
  for(int i=0;i<6;++i){((dbrushside_t*)(bytes.data()+side.fileofs))[i].planeNum=i;planes[i].normal[i/2]=(i&1)?1.0f:-1.0f;planes[i].dist=(i&1)?10.0f:0.0f;}
  for(int i=0;i<count;++i){auto&f=((dfog_t*)bytes.data())[i];strcpy(f.shader,"fixture/fog");f.brushNum=global?-1:0;f.visibleSide=global?-1:1;}
 }
 void load(world_t &world,int index){world.planes=planes;world.numplanes=6;strcpy(world.name,"maps/fog-fixture.bsp");fileBase=bytes.data();R_LoadFogs(&fog,&brush,&side,world,index);}
};
int main(int argc,char**argv){return RunTests("classic SP/MP fog loading, ownership, transforms, coordinates and bounds",[&]{
 world_t main;tr.world=&main;tr.numBSPModels=2;
 Fixture root(1,true);root.load(main,0);CHECK(main.numfogs==2&&main.globalFog==1&&main.inheritedFog==-1&&!main.fogs[1].hasSurface);
 CHECK(main.fogs[2].originalBrushNumber==-1&&main.fogs[2].colorInt==0&&main.fogs[2].bounds[1][0]==MAX_WORLD_COORD);
#ifdef JAR_CLASSIC_SP
 CHECK((main.fogs[1].colorInt&255)==51);
#else
 CHECK((main.fogs[1].colorInt&255)==25);
#endif
 Fixture child(1,false);child.load(tr.bspModels[0],1);auto &world=tr.bspModels[0];
 CHECK(world.numfogs==3&&world.inheritedFog==2&&world.globalFog==2&&world.fogs[1].hasSurface);
 CHECK(R_BSPFogIndex(-1,world,1)==2&&R_BSPFogIndex(0,world,1)==1&&R_BSPFogIndex(-1,main,0)==0);
 rejects([&]{R_BSPFogIndex(1,world,1);});rejects([&]{R_BSPFogIndex(-2,world,1);});
 bmodel_t bmodels[3]{{0},{1},{2}};for(int i=0;i<3;++i)models[i]={MOD_BRUSH,&bmodels[i]};
 trRefEntity_t entity;entity.e.hModel=1;entity.e.origin[0]=100;entity.e.origin[1]=200;entity.e.origin[2]=300;
 // Local X points along world Y at twice the scale. Local Y points -X.
 entity.e.axis[0][1]=2;entity.e.axis[1][0]=-1;entity.e.axis[2][2]=1;backEnd.currentEntity=&entity;
 auto resolved=RB_BSPFog(1);CHECK(near(resolved.surface[0],0)&&near(resolved.surface[1],-1)&&near(resolved.surface[3],-220));
 CHECK(near(resolved.bounds[0][0],90)&&near(resolved.bounds[1][0],100)&&near(resolved.bounds[0][1],200)&&near(resolved.bounds[1][1],220));
 CHECK(near(world.fogs[1].surface[0],-1)&&near(world.fogs[1].surface[3],-10)); // Source not mutated.
 CHECK(!RB_BSPFogIsGlobal(1)&&RB_BSPFogIsGlobal(2)&&RB_BSPFogIsGlobal(3,true));
 main.fogs[1].parms.depthForOpaque=777;CHECK(RB_BSPFog(2).parms.depthForOpaque==777&&RB_BSPFog(2).bounds[0][0]==MIN_WORLD_COORD);
 CHECK(RB_BSPFog(99).colorInt==0&&RB_BSPFog(-1).colorInt==0);
 // Production clipped-fog texcoords agree with an equivalent world-space fog.
 tess.fogNum=1;tess.numVertexes=2;tess.xyz[0][0]=5;tess.xyz[1][0]=15;
 backEnd.ori.origin[0]=100;backEnd.ori.origin[1]=200;backEnd.ori.origin[2]=300;
 memcpy(backEnd.ori.axis,entity.e.axis,sizeof(entity.e.axis));backEnd.ori.modelMatrix[2]=-1;
 backEnd.viewParms.ori.axis[0][0]=1;backEnd.ori.viewOrigin[0]=20;
 float localST[4],worldST[4];RB_CalcFogTexCoords(localST);
 const fog_t original=main.fogs[1];main.fogs[1]=resolved;entity.e.hModel=0;RB_CalcFogTexCoords(worldST);
 for(int i=0;i<4;++i)CHECK(near(localST[i],worldST[i]));main.fogs[1]=original;entity.e.hModel=1;
 // A different instance of the same BSP must use its own transform.
 entity.e.origin[1]=500;CHECK(near(RB_BSPFog(1).surface[3],-520));
 Fixture ownGlobal(1,true);ownGlobal.load(tr.bspModels[1],2);entity.e.hModel=2;
 CHECK(tr.bspModels[1].inheritedFog==-1&&RB_BSPFogIsGlobal(1)&&RB_BSPFog(1).parms.depthForOpaque==256);
#ifdef JAR_CLASSIC_SP
 {
  auto& rift=tr.bspModels[1];Q_strncpyz(rift.name,jar::rift::path,sizeof(rift.name));
  rift.bmodels=&bmodels[2];VectorSet(bmodels[2].bounds[0],-64,-64,-64);VectorSet(bmodels[2].bounds[1],64,64,64);
  cplane_t split{};split.normal[0]=1;
  mnode_t leaf[2]{{0,1,jar::rift::arenaArea},{0,1,5}};
  mnode_t node{CONTENTS_NODE,0,0,&split,{&leaf[0],&leaf[1]}};
  rift.nodes=&node;rift.numDecisionNodes=1;
  trRefEntity_t sceneEntities[5]{};
  for(int n=0;n<2;++n){
   auto& room=sceneEntities[n];room.e.hModel=2;room.e.renderfx=RF_JAR_SECTION_FOG;
   room.e.origin[0]=200*n;vec3_t angles={0,90.f*n,0};AnglesToAxis(angles,room.e.axis);
  }
  VectorSet(sceneEntities[2].e.origin,32,0,0);
  VectorSet(sceneEntities[3].e.origin,200,32,0);
  VectorSet(sceneEntities[4].e.origin,200,-32,0);
  trRefdef_t scene{sceneEntities,5,0};
  R_AssignRiftFog(scene,0);
  CHECK(sceneEntities[2].riftFogRoom==sceneEntities&&sceneEntities[3].riftFogRoom==sceneEntities+1);
  CHECK(!sceneEntities[4].riftFogRoom&&sceneEntities[2].riftFogIndex==1);
  CHECK(R_RiftCameraFog(R_RiftFogRoomAtPoint(sceneEntities[3].e.origin))==&rift.fogs[1]);
  CHECK(!R_RiftCameraFog(R_RiftFogRoomAtPoint(sceneEntities[4].e.origin)));
  backEnd.currentEntity=&sceneEntities[2];CHECK(RB_BSPFog(1).parms.depthForOpaque==256);
  backEnd.currentEntity=&sceneEntities[4];CHECK(RB_BSPFog(1).parms.depthForOpaque==777);
  Q_strncpyz(rift.name,jar::korriban::path,sizeof(rift.name));
  leaf[0].area=jar::korriban::arenaArea;
  R_AssignRiftFog(scene,0);
  CHECK(sceneEntities[2].riftFogRoom==sceneEntities&&sceneEntities[3].riftFogRoom==sceneEntities+1&&!sceneEntities[4].riftFogRoom);
  CHECK(R_RiftCameraFog(R_RiftFogRoomAtPoint(sceneEntities[3].e.origin))==&rift.fogs[1]);
  Q_strncpyz(rift.name,jar::coruscant::path,sizeof(rift.name));
  R_AssignRiftFog(scene,0);CHECK(sceneEntities[4].riftFogRoom==sceneEntities+1);
  Q_strncpyz(rift.name,jar::wasteland::path,sizeof(rift.name));
  R_AssignRiftFog(scene,0);CHECK(sceneEntities[2].riftFogRoom==sceneEntities&&sceneEntities[4].riftFogRoom==sceneEntities+1);
  leaf[1].cluster=-1;R_AssignRiftFog(scene,0);CHECK(!sceneEntities[4].riftFogRoom);
  scene.rdflags=RDF_NOWORLDMODEL;R_AssignRiftFog(scene,0);CHECK(!sceneEntities[2].riftFogRoom);
  backEnd.currentEntity=&entity;
 }
#endif
 entity.e.hModel=1;entity.e.axis[0][1]=0;CHECK(!RB_BSPFog(1).hasSurface);entity.e.axis[0][1]=2;
#ifdef JAR_CLASSIC_SP
 backEnd.refdef.doLAGoggles=true;main.fogs[main.numfogs].colorInt=0x1234;CHECK(RB_BSPFog(main.numfogs).colorInt==0x1234);CHECK(RB_BSPFogIsGlobal(main.numfogs,true));backEnd.refdef.doLAGoggles=false;
#endif
 backEnd.currentEntity=nullptr;CHECK(RB_BSPFog(1).parms.depthForOpaque==777);backEnd.currentEntity=&entity;
 entity.e.reType=RT_SPRITE;CHECK(RB_BSPFog(1).parms.depthForOpaque==777);entity.e.reType=RT_MODEL;
 entity.e.hModel=99;CHECK(RB_BSPFog(1).parms.depthForOpaque==777);entity.e.hModel=1;
 // No-fog children still reserve their inherited and special slots.
 Fixture empty(0,false);world_t noFog;empty.load(noFog,1);CHECK(noFog.numfogs==2&&noFog.inheritedFog==1);checkAllocations();
 main.globalFog=-1;empty.load(noFog,1);CHECK(noFog.numfogs==1&&noFog.inheritedFog==-1);main.globalFog=1;
 Fixture maxFogs(29,false);maxFogs.load(noFog,1);CHECK(noFog.numfogs==31);checkAllocations();
 Fixture over(30,false);rejects([&]{over.load(noFog,1);});over.load(noFog,0);CHECK(noFog.numfogs==31);checkAllocations();
 Fixture duplicate(2,true);rejects([&]{duplicate.load(noFog,0);});
 auto badChild=[&](auto modify){Fixture bad(1,false);modify(bad);rejects([&]{bad.load(noFog,0);});checkAllocations();};
 badChild([](Fixture&f){--f.fog.filelen;});badChild([](Fixture&f){((dfog_t*)f.bytes.data())->brushNum=99;});
 badChild([](Fixture&f){((dbrush_t*)(f.bytes.data()+f.brush.fileofs))->firstSide=-1;});
 badChild([](Fixture&f){((dbrush_t*)(f.bytes.data()+f.brush.fileofs))->numSides=5;});
 badChild([](Fixture&f){((dbrushside_t*)(f.bytes.data()+f.side.fileofs))[0].planeNum=999;});
 badChild([](Fixture&f){((dfog_t*)f.bytes.data())->visibleSide=6;});
 badChild([](Fixture&f){memset(((dfog_t*)f.bytes.data())->shader,'x',sizeof(dfog_t::shader));});
 if(argc>1){
  for(int n=1;n<argc;++n){
   std::ifstream input(argv[n],std::ios::binary);CHECK(input.good());std::vector<byte>data((std::istreambuf_iterator<char>(input)),{});
   CHECK(data.size()>=sizeof(dheader_t));auto*header=(dheader_t*)data.data();CHECK(header->version==BSP_VERSION);
   for(auto &l:header->lumps)CHECK(l.fileofs>=0&&l.filelen>=0&&size_t(l.fileofs)+l.filelen<=data.size());
   auto &planeLump=header->lumps[LUMP_PLANES];CHECK(planeLump.filelen%sizeof(dplane_t)==0);std::vector<cplane_t>planes(planeLump.filelen/sizeof(dplane_t));
   auto*diskPlanes=(dplane_t*)(data.data()+planeLump.fileofs);for(size_t i=0;i<planes.size();++i){VectorCopy(diskPlanes[i].normal,planes[i].normal);planes[i].dist=diskPlanes[i].dist;}
   world_t &target=n==1?main:tr.bspModels[0];target.planes=planes.data();target.numplanes=int(planes.size());fileBase=data.data();
   R_LoadFogs(&header->lumps[LUMP_FOGS],&header->lumps[LUMP_BRUSHES],&header->lumps[LUMP_BRUSHSIDES],target,n==1?0:1);
   auto &surfaceLump=header->lumps[LUMP_SURFACES];CHECK(surfaceLump.filelen%sizeof(dsurface_t)==0);auto*surfaces=(dsurface_t*)(data.data()+surfaceLump.fileofs);
   for(int i=0;i<surfaceLump.filelen/sizeof(dsurface_t);++i)R_BSPFogIndex(surfaces[i].fogNum,target,n==1?0:1);
   printf("Retail classic fog BSP %d: %d entries, global %d, inherited %d, surface references valid\n",n,target.numfogs,target.globalFog,target.inheritedFog);checkAllocations();
  }
 }
 checkAllocations();for(auto&a:allocations)free(a.data);
 });}
