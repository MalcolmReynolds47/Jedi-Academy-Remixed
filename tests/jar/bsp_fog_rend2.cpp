// Production Rend2 BSP fog loader and per-entity constant generation; no GL.
#include "tr_local.h"
#include "test_support.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>
trGlobals_t tr;
backEndState_t backEnd;
refimport_t ri{};
static byte *fileBase;
const int lightmapsNone[MAXLIGHTMAPS]={-1,-1,-1,-1};
const byte stylesDefault[MAXLIGHTMAPS]={0,255,255,255};
static cvar_t volume{},scale{};
cvar_t *r_volumetricFog=&volume,*r_volumetricFogScale=&scale;
static shader_t shader{};
shader_t *R_FindShader(const char*,const int*,const byte*,qboolean){return &shader;}
static model_t models[3]{};
model_t *R_GetModelByHandle(qhandle_t handle){return handle>=0&&handle<3?&models[handle]:nullptr;}
struct Allocation{byte *data;int size;};
static std::vector<Allocation> allocations;
void *Hunk_Alloc(int size,ha_pref){byte*p=(byte*)malloc(size+32);memset(p,0xcd,size+32);allocations.push_back({p,size});return p+16;}
static void guards(){for(auto&a:allocations)for(int i=0;i<16;++i)CHECK(a.data[i]==0xcd&&a.data[16+a.size+i]==0xcd);}
static std::vector<FogsBlock> blocks;
size_t RB_AppendConstantsData(gpuFrame_t*,const void *data,size_t size){CHECK(size==sizeof(FogsBlock));blocks.push_back(*(const FogsBlock*)data);return (blocks.size()-1)*2048;}
#include "../../shared/rd-rend2/tr_bsp_fogs.inc"
#include "../../shared/rd-rend2/tr_bsp_fog_context.inc"
#include "../../code/rd-common/tr_rift_fog.inc"
static bool closeEnough(float a,float b){return fabs(a-b)<0.0001f;}
template<class F>static void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}CHECK(caught);}
struct Fixture {
 std::vector<byte>bytes;lump_t fog{},brush{},side{};cplane_t planes[6]{};
 Fixture(int count,bool global):bytes(count*sizeof(dfog_t)+sizeof(dbrush_t)+6*sizeof(dbrushside_t),0){
  fog={0,int(count*sizeof(dfog_t))};brush={fog.filelen,sizeof(dbrush_t)};side={brush.fileofs+brush.filelen,6*sizeof(dbrushside_t)};
  auto*b=(dbrush_t*)(bytes.data()+brush.fileofs);b->numSides=6;
  for(int i=0;i<6;++i){((dbrushside_t*)(bytes.data()+side.fileofs))[i].planeNum=i;planes[i].normal[i/2]=(i&1)?1.0f:-1.0f;planes[i].dist=(i&1)?10.0f:0.0f;}
  for(int i=0;i<count;++i){auto&f=((dfog_t*)bytes.data())[i];strcpy(f.shader,"fixture/fog");f.brushNum=global?-1:0;f.visibleSide=global?-1:1;}
 }
 void load(world_t&w,int index){strcpy(w.name,"maps/fog-fixture.bsp");w.planes=planes;w.numplanes=6;w.lightmapNamespace=index;fileBase=bytes.data();R_LoadFogs(&w,&fog,&brush,&side);}
};
static void gridPoint(const matrix_t matrix,const vec3_t world,vec3_t grid){for(int i=0;i<3;++i)grid[i]=matrix[i]*world[0]+matrix[4+i]*world[1]+matrix[8+i]*world[2]+matrix[12+i];}
int main(int argc,char**argv){return RunTests("Rend2 BSP fog owner, transformed grids, GPU constants, inheritance and limits",[&]{
 static_assert(offsetof(FogsBlock,worldToLightGrid)==16,"std140 grid matrix");
 static_assert(offsetof(FogsBlock,worldToGlobalLightGrid)==80,"std140 global matrix");
 static_assert(offsetof(FogsBlock,fogs)==144&&sizeof(FogsBlock::Fog)==48,"std140 fog records");
 world_t main{},child{},own{};main.volumetricFogScale=2;child.volumetricFogScale=5;own.volumetricFogScale=7;
 tr.world=&main;tr.bspModels[0]=&child;tr.bspModels[1]=&own;tr.numBspModels=2;tr.identityLight=1;scale.value=1;
 VectorSet(shader.fogParms.color,0.2f,0.4f,0.8f);shader.fogParms.depthForOpaque=256;
 Fixture root(1,true);root.load(main,0);Fixture local(1,false);local.load(child,1);Fixture global(1,true);global.load(own,2);
 CHECK(child.numfogs==3&&child.inheritedFog==2&&child.globalFogIndex==2&&own.inheritedFog==-1);
 CHECK(R_BSPFogIndex(-1,child)==2&&R_BSPFogIndex(0,child)==1);rejects([&]{R_BSPFogIndex(1,child);});
 bmodel_t bmodels[3]{};for(int i=0;i<3;++i){models[i].type=MOD_BRUSH;models[i].data.bmodel=&bmodels[i];bmodels[i].worldIndex=i-1;}
 trRefEntity_t entities[3]{};for(auto&e:entities){e.e.reType=RT_MODEL;e.e.hModel=1;VectorSet(e.e.origin,100,200,300);e.e.axis[0][1]=2;e.e.axis[1][0]=-1;e.e.axis[2][2]=1;}
 entities[1].e.origin[1]=500;entities[2].e.hModel=0;
 backEnd.currentEntity=&entities[0];
 const auto fog=R_BSPFog(&entities[0],1);CHECK(closeEnough(fog.surface[1],-1)&&closeEnough(fog.surface[3],-220));
 CHECK(closeEnough(R_BSPFog(&entities[1],1).surface[3],-520)&&closeEnough(child.fogs[1].surface[3],-10));
 image_t mainImage{},childImage{},white{};tr.whiteImage3D=&white;mgrid_t cell{};
 for(auto*w:{&main,&child}){w->lightGridData=&cell;VectorSet(w->lightGridSize,10,10,10);VectorSet(w->lightGridInverseSize,0.1f,0.1f,0.1f);}
 main.volumetricLightMaps[0]=&mainImage;child.volumetricLightMaps[0]=&childImage;
 CHECK(RB_BSPFogLightGrid(false)==&childImage&&RB_BSPFogLightGrid(true)==&mainImage);
 FogsBlock block;volume.integer=1;R_BuildBSPFogsBlock(block,&entities[0],false);
 vec3_t point{90,210,315},grid{};gridPoint(block.worldToLightGrid,point,grid);CHECK(closeEnough(grid[0],1)&&closeEnough(grid[1],1.5f)&&closeEnough(grid[2],2));
 gridPoint(block.worldToGlobalLightGrid,point,grid);CHECK(closeEnough(grid[0],9.5f)&&closeEnough(grid[1],21.5f)&&closeEnough(grid[2],32));
 CHECK(block.globalFogIndex==1&&block.numFogs==2&&block.fogs[0].depthToOpaque>block.fogs[1].depthToOpaque);
 main.fogs[1].color[0]=0.9f;R_BuildBSPFogsBlock(block,&entities[0],false);CHECK(block.fogs[1].color[0]==0.9f&&block.fogs[0].color[0]==0.2f);
 trRefdef_t scene{};scene.entities=entities;scene.num_entities=3;RB_UpdateFogsConstants(nullptr,&scene);
 CHECK(blocks.size()==3&&entities[0].bspFogUboOffset==2048&&entities[1].bspFogUboOffset==4096&&entities[2].bspFogUboOffset==0);
 CHECK(blocks[1].fogs[0].plane[3]==-220&&blocks[2].fogs[0].plane[3]==-520&&blocks[0].fogs[0].color[0]==0.9f);
 CHECK(RB_BSPFogUboOffset()==2048);backEnd.currentEntity=&entities[1];CHECK(RB_BSPFogUboOffset()==4096);
 blocks.clear();scene.doLAGoggles=qtrue;RB_UpdateFogsConstants(nullptr,&scene);CHECK(blocks.size()==1&&blocks[0].numFogs==2&&entities[0].bspFogUboOffset==0);
 backEnd.refdef.doLAGoggles=qtrue;CHECK(RB_BSPFogLightGrid(false)==&white);backEnd.refdef.doLAGoggles=qfalse;
 backEnd.currentEntity=&entities[0];entities[0].e.hModel=2;CHECK(R_BSPGlobalFogIndex(&entities[0])==1);R_BuildBSPFogsBlock(block,&entities[0],false);CHECK(block.numFogs==1&&block.globalFogIndex==0&&block.fogs[0].color[0]==0.2f);
 entities[0].e.hModel=1;child.lightGridData=nullptr;CHECK(RB_BSPFogLightGrid(false)==&white&&RB_BSPFogLightGrid(true)==&mainImage);child.lightGridData=&cell;
 CHECK(R_BSPFog(&entities[0],999).color[0]==0);entities[0].e.hModel=99;CHECK(R_BSPFogWorld(&entities[0])==&main);entities[0].e.hModel=1;
 {
  Q_strncpyz(own.name,jar::rift::path,sizeof(own.name));
  own.bmodels=&bmodels[2];VectorSet(bmodels[2].bounds[0],-64,-64,-64);VectorSet(bmodels[2].bounds[1],64,64,64);
  cplane_t split{};split.normal[0]=1;
  mnode_t node[3]{};
  node[0].contents=CONTENTS_NODE;node[0].plane=&split;node[0].children[0]=&node[1];node[0].children[1]=&node[2];
  node[1].cluster=1;node[1].area=jar::rift::arenaArea;node[2].cluster=-1;
  own.nodes=node;own.numDecisionNodes=1;
  trRefEntity_t actors[4]{};
  auto& room=actors[2];room.e.hModel=2;room.e.renderfx=RF_JAR_SECTION_FOG;
  VectorSet(room.e.origin,200,100,0);const vec3_t angles={0,90,0};AnglesToAxis(angles,room.e.axis);
  VectorSet(actors[0].e.origin,200,140,0);VectorSet(actors[1].e.origin,200,80,0);
  VectorSet(actors[3].e.lightingOrigin,200,130,0);actors[3].e.renderfx=RF_LIGHTING_ORIGIN;
  scene={};scene.entities=actors;scene.num_entities=4;
  R_AssignRiftFog(scene,0);
  CHECK(actors[0].riftFogRoom==&room&&actors[0].riftFogIndex==1);
  CHECK(!actors[1].riftFogRoom&&!room.riftFogRoom&&actors[3].riftFogRoom==&room);
  CHECK(R_BSPFogWorld(&actors[0])==&own&&R_BSPFogWorld(&actors[1])==&main);
  CHECK(R_RiftCameraFog(R_RiftFogRoomAtPoint(actors[0].e.origin))==own.globalFog);
  blocks.clear();RB_UpdateFogsConstants(nullptr,&scene);
  CHECK(blocks.size()==2&&actors[0].bspFogUboOffset==room.bspFogUboOffset&&actors[1].bspFogUboOffset==0);
  backEnd.currentEntity=&tr.worldEntity;backEnd.riftFogCapRoom=&room;
  CHECK(R_BSPFogWorld(&tr.worldEntity)==&own&&RB_BSPFogUboOffset()==room.bspFogUboOffset);
  backEnd.riftFogCapRoom=nullptr;CHECK(R_BSPFogWorld(&tr.worldEntity)==&main);
  Q_strncpyz(own.name,jar::korriban::path,sizeof(own.name));
  node[1].area=jar::korriban::arenaArea;
  R_AssignRiftFog(scene,0);
  CHECK(actors[0].riftFogRoom==&room&&actors[3].riftFogRoom==&room&&!actors[1].riftFogRoom);
  CHECK(R_RiftCameraFog(R_RiftFogRoomAtPoint(actors[0].e.origin))==own.globalFog);
  Q_strncpyz(own.name,jar::coruscant::path,sizeof(own.name));
  node[2].cluster=2;node[2].area=5;R_AssignRiftFog(scene,0);CHECK(actors[1].riftFogRoom==&room);
  Q_strncpyz(own.name,jar::wasteland::path,sizeof(own.name));
  R_AssignRiftFog(scene,0);CHECK(actors[0].riftFogRoom==&room&&actors[1].riftFogRoom==&room);
  node[2].cluster=-1;R_AssignRiftFog(scene,0);CHECK(!actors[1].riftFogRoom);
  R_AssignRiftFog(scene,1);
  CHECK(!actors[0].riftFogRoom&&actors[3].riftFogRoom==&room);
  R_AssignRiftFog(scene,3);
  CHECK(!actors[3].riftFogRoom&&!R_RiftFogRoomAtPoint(actors[3].e.lightingOrigin));
  scene.num_entities=2;R_AssignRiftFog(scene,0);
  CHECK(!actors[0].riftFogRoom&&!R_RiftFogRoomAtPoint(actors[0].e.origin));
  backEnd.currentEntity=&entities[0];
 }
 Fixture max(22,false);world_t limit{};max.load(limit,1);CHECK(limit.numfogs==24);Fixture over(23,false);rejects([&]{over.load(limit,1);});over.load(limit,0);CHECK(limit.numfogs==24);
 Fixture bad(1,false);((dfog_t*)bad.bytes.data())->visibleSide=6;rejects([&]{bad.load(limit,0);});guards();
 if(argc>1)for(int n=1;n<argc;++n){
  std::ifstream input(argv[n],std::ios::binary);CHECK(input.good());std::vector<byte>data((std::istreambuf_iterator<char>(input)),{});CHECK(data.size()>=sizeof(dheader_t));
  auto*h=(dheader_t*)data.data();CHECK(h->version==BSP_VERSION);for(auto&l:h->lumps)CHECK(l.fileofs>=0&&l.filelen>=0&&size_t(l.fileofs)+l.filelen<=data.size());
  auto&pl=h->lumps[LUMP_PLANES];CHECK(pl.filelen%sizeof(dplane_t)==0);std::vector<cplane_t>planes(pl.filelen/sizeof(dplane_t));auto*disk=(dplane_t*)(data.data()+pl.fileofs);
  for(size_t i=0;i<planes.size();++i){VectorCopy(disk[i].normal,planes[i].normal);planes[i].dist=disk[i].dist;}
  world_t &w=n==1?main:child;w.planes=planes.data();w.numplanes=int(planes.size());w.lightmapNamespace=n==1?0:1;fileBase=data.data();R_LoadFogs(&w,&h->lumps[LUMP_FOGS],&h->lumps[LUMP_BRUSHES],&h->lumps[LUMP_BRUSHSIDES]);
  const auto&sl=h->lumps[LUMP_SURFACES];CHECK(sl.filelen%sizeof(dsurface_t)==0);auto*surfs=(dsurface_t*)(data.data()+sl.fileofs);for(int i=0;i<sl.filelen/sizeof(dsurface_t);++i)R_BSPFogIndex(surfs[i].fogNum,w);
  printf("Retail Rend2 fog BSP %d: %d entries, global %d, inherited %d; surface references valid\n",n,w.numfogs,w.globalFogIndex,w.inheritedFog);guards();
 }
 guards();for(auto&a:allocations)free(a.data);
 });}
