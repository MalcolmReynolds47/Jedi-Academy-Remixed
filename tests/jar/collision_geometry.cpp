// Actual collision exporter and winding code; the loaded clip maps are fixtures.
#include "qcommon/cm_local.h"
#include "qcommon/cm_patch.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <stdexcept>
clipMap_t cmg,SubBSP[MAX_SUB_BSP];int NumSubBSP=0;
#ifdef JAR_CM_MP
void QDECL Com_Error(int,const char*,...){throw std::runtime_error("unexpected engine error");}
void QDECL Com_Printf(const char*,...){}
void QDECL Com_DPrintf(const char*,...){}
void*Z_Malloc(int size,memtag_t,qboolean,int){return calloc(1,size);}
void Z_Free(void*p){free(p);}
#else
int testContents=0;int CM_TotalMapContents(){return testContents;}
#endif
#include "../../code/qcommon/cm_jar_geometry.inc"
#include "../../code/qcommon/cm_jar_bsp.inc"
#ifdef JAR_CM_MP
#include "../../codemp/qcommon/cm_jar_bsp_registry.inc"
int CM_FindSubBSP(int model){return model>=2&&model<6 ? (model-2)/2 : -1;}
cmodel_t* CM_ClipHandleToModel(clipHandle_t handle, clipMap_t **map){
 const int slot=CM_FindSubBSP(handle);
 assert(slot>=0);
 *map=&SubBSP[slot];
 return &(*map)->cmodels[(handle-2)%2];
}
int CM_PointLeafnum_r(const vec3_t point,int node,clipMap_t *map){
 assert(node==0&&map==&SubBSP[1]);
 return point[0]>=0?0:1;
}
#include "../../codemp/qcommon/cm_jar_hoth_sky.inc"
#endif
extern int c_active_windings;
struct Map {
 cmodel_t models[2]{};cLeaf_t leaves[2]{};int brushIndices[2]{0,1},surfaceIndices[1]{0};
 cplane_t planes[12]{};cbrushside_t sides[12]{};cbrush_t brushes[2]{};
 patchPlane_t patchPlanes[5]{};facet_t facet{};patchCollide_t collision{};cPatch_t patch{};cPatch_t*surfaces[1]{&patch};
 void install(clipMap_t&cm,float x){
  cm={};cm.numSubModels=2;cm.cmodels=models;cm.numLeafs=2;cm.leafs=leaves;
  cm.numBrushes=2;cm.brushes=brushes;cm.leafbrushes=brushIndices;cm.numLeafBrushes=2;
  cm.numSurfaces=1;cm.surfaces=surfaces;cm.leafsurfaces=surfaceIndices;cm.numLeafSurfaces=1;
  for(int b=0;b<2;++b){
   brushes[b].contents=CONTENTS_SOLID;brushes[b].numsides=6;brushes[b].sides=sides+b*6;
   for(int s=0;s<6;++s){auto&p=planes[b*6+s];const int axis=s/2;const float sign=s%2?-1.f:1.f;
    p.normal[axis]=sign;p.dist=8+(axis==0?sign*(x+b*100):0);sides[b*6+s].plane=&p;}
   models[b].leaf.firstLeafBrush=b;models[b].leaf.numLeafBrushes=1;
   leaves[b]=models[0].leaf; // repeated main-world references must be deduplicated
  }
  patch.contents=CONTENTS_BODY;patch.pc=&collision;collision.numPlanes=5;collision.planes=patchPlanes;collision.numFacets=1;collision.facets=&facet;
  patchPlanes[0].plane[2]=1;patchPlanes[0].plane[3]=20;facet.surfacePlane=0;facet.numBorders=4;
  for(int b=0;b<4;++b){auto&p=patchPlanes[b+1].plane;const int axis=b/2;const float sign=b%2?-1.f:1.f;p[axis]=sign;p[3]=-8+(axis==0?sign*x:0);facet.borderPlanes[b]=b+1;facet.borderInward[b]=1;}
  models[0].leaf.numLeafSurfaces=1;leaves[0].numLeafSurfaces=leaves[1].numLeafSurfaces=1;
 }
};
struct Result{int calls=0;float low=1e9f,high=-1e9f;bool reject=false;};
qboolean emit(void*ctx,int n,const float*p,const float*normal){auto&r=*static_cast<Result*>(ctx);++r.calls;
 assert(n>=3&&n<=MAX_POINTS_ON_WINDING);assert(std::fabs(VectorLength(normal)-1)<.001f);
 for(int i=0;i<n;++i){r.low=std::min(r.low,p[i*3]);r.high=std::max(r.high,p[i*3]);}return r.reject?qfalse:qtrue;}
void check(int model,int mask,int expected,float x){Result r;assert(CM_JARCollisionPolygons(model,mask,&r,emit)==expected);assert(r.calls==expected);
 if(expected)assert(std::fabs(r.low-(x-8))<.01f&&std::fabs(r.high-(x+8))<.01f);assert(c_active_windings==0);}
#ifdef JAR_CM_MP
static void CheckRegistry(){
 char name[MAX_QPATH];int first=-1,count=-1;unsigned checksum=0;
 cmg.checksum=314;SubBSP[0].checksum=271;SubBSP[1].checksum=103;
 assert(CM_BSPModelAt(-1,name,sizeof(name),&first,&count,&checksum)==1&&!name[0]&&first==0&&count==2&&checksum==314);
 assert(CM_BSPModelAt(0,name,sizeof(name),&first,&count,&checksum)==1&&!strcmp(name,"#first")&&first==2&&count==2&&checksum==271);
 assert(CM_BSPModelAt(1,name,sizeof(name),&first,&count,&checksum)==1&&!strcmp(name,"#mp/second")&&first==4&&count==2&&checksum==103);
 assert(CM_BSPModelAt(2,name,sizeof(name),&first,&count,&checksum)==0);
 assert(CM_BSPModelAt(-2,name,sizeof(name),&first,&count,&checksum)==-1);
 assert(CM_BSPModelAt(0,name,sizeof(name)-1,&first,&count,&checksum)==-1);
 assert(CM_BSPModelAt(0,name,sizeof(name),&first,&count,nullptr)==-1);

 Q_strncpyz(SubBSP[1].name,"maps/mp/../second.bsp",MAX_QPATH);
 assert(CM_BSPModelAt(1,name,sizeof(name),&first,&count,&checksum)==-1);
 Q_strncpyz(SubBSP[1].name,"maps/mp/second.bsp",MAX_QPATH);
 assert(!strcmp(name,"#mp/second")&&first==4&&count==2&&checksum==103);
 SubBSP[1].numSubModels=CAPSULE_MODEL_HANDLE-4;
 assert(CM_BSPModelAt(1,name,sizeof(name),&first,&count,&checksum)==1&&first+count==CAPSULE_MODEL_HANDLE);
 ++SubBSP[1].numSubModels;
 assert(CM_BSPModelAt(1,name,sizeof(name),&first,&count,&checksum)==-1);
 SubBSP[1].numSubModels=2;
}

static void CheckHothSky(){
 auto& map=SubBSP[1];
 Q_strncpyz(map.name,"maps/mp/duel9.bsp",MAX_QPATH);
 const vec3_t arena={1,0,0}, outside={-1,0,0}, beyond={129,0,0};
 assert(!CM_HothSectionSky(4,nullptr));
 map.checksum=jar::hothSky::checksum;
 map.numNodes=1;
 map.leafs[0].area=jar::hothSky::arenaArea;
 map.leafs[0].cluster=0;
 map.leafs[1].area=1;
 map.leafs[1].cluster=0;
 VectorSet(map.cmodels[0].mins,-128,-128,-128);
 VectorSet(map.cmodels[0].maxs,128,128,128);
 assert(CM_HothSectionSky(4,nullptr)&&CM_HothSectionSky(4,arena));
 assert(!CM_HothSectionSky(4,outside)&&!CM_HothSectionSky(4,beyond));
 assert(!CM_HothSectionSky(5,nullptr)&&!CM_HothSectionSky(2,nullptr)&&!CM_HothSectionSky(0,nullptr));
 map.leafs[0].cluster=-1;
 assert(!CM_HothSectionSky(4,arena));
 Q_strncpyz(map.name,jar::rift::path,MAX_QPATH);
 assert(!CM_RiftSectionAir(4,nullptr));
 map.checksum=jar::rift::checksum;
 map.leafs[0].area=jar::rift::arenaArea;
 map.leafs[0].cluster=0;
 map.leafs[1].area=0;
 assert(CM_RiftSectionAir(4,nullptr)&&CM_RiftSectionAir(4,arena));
 assert(!CM_RiftSectionAir(4,outside)&&!CM_RiftSectionAir(4,beyond));
 assert(!CM_RiftSectionAir(5,nullptr)&&!CM_RiftSectionAir(2,nullptr)&&!CM_RiftSectionAir(0,nullptr));
 map.leafs[0].cluster=-1;
 assert(!CM_RiftSectionAir(4,arena));
 Q_strncpyz(map.name,jar::korriban::path,MAX_QPATH);
 assert(!CM_KorribanSectionFog(4));
 map.checksum=jar::korriban::checksum;
 assert(CM_KorribanSectionFog(4)&&!CM_KorribanSectionFog(5)&&!CM_KorribanSectionFog(0));
 assert(!CM_RiftSectionAir(4,nullptr));
 Q_strncpyz(map.name,"maps/mp/duel6.bsp",MAX_QPATH);
 assert(!CM_YavinSectionSky(4,nullptr));
 map.checksum=jar::yavinSky::checksum;
 map.leafs[0].area=jar::yavinSky::arenaArea;
 map.leafs[0].cluster=0;
 map.leafs[1].area=1;
 assert(CM_YavinSectionSky(4,arena)&&!CM_YavinSectionSky(4,outside)&&!CM_YavinSectionSky(5,nullptr));
 Q_strncpyz(map.name,"maps/mp/second.bsp",MAX_QPATH);
}
#endif
int main(){
 Map base,first,second;base.install(cmg,0);first.install(SubBSP[0],1000);second.install(SubBSP[1],2000);
 NumSubBSP=2;
 Q_strncpyz(SubBSP[0].name,"maps/first.bsp",MAX_QPATH);Q_strncpyz(SubBSP[1].name,"maps/mp/second.bsp",MAX_QPATH);
#ifdef JAR_CM_MP
 CheckRegistry();
 CheckHothSky();
#endif
 int firstModel=-1,modelCount=-1;assert(CM_BSPModelInfo("#FIRST",&firstModel,&modelCount)&&firstModel==2&&modelCount==2);
 assert(CM_BSPModelInfo("#mp/second",&firstModel,&modelCount)&&firstModel==4&&modelCount==2);
 assert(!CM_BSPModelInfo("#missing",&firstModel,&modelCount)&&firstModel==4&&modelCount==2);
 assert(!CM_BSPModelInfo("#first:1",&firstModel,&modelCount)&&!CM_BSPModelInfo("#../first",&firstModel,&modelCount));
 assert(!CM_BSPModelInfo("#first:0",&firstModel,&modelCount));
 assert(!CM_BSPModelInfo("#first",nullptr,&modelCount)&&!CM_BSPModelInfo("#first",&firstModel,nullptr));
 check(0,CONTENTS_SOLID,6,0);check(1,CONTENTS_SOLID,6,100);
 check(2,CONTENTS_SOLID,6,1000);check(3,CONTENTS_SOLID,6,1100);check(4,CONTENTS_SOLID,6,2000);check(5,CONTENTS_SOLID,6,2100);
 // Sub-map root selection must use its model leaf, not collect its child door.
 first.leaves[1]=first.models[1].leaf;check(2,CONTENTS_SOLID,6,1000);
 check(0,CONTENTS_BODY,1,0);check(2,CONTENTS_BODY,1,1000);check(4,CONTENTS_SOLID|CONTENTS_BODY,7,2000);
 check(1,CONTENTS_BODY,0,0);check(2,0,0,0);
 for(int bad:{-1,6,1000000,BOX_MODEL_HANDLE}){Result r;assert(CM_JARCollisionPolygons(bad,CONTENTS_SOLID,&r,emit)==-1&&r.calls==0);}
 assert(CM_JARCollisionPolygons(0,CONTENTS_SOLID,nullptr,nullptr)==-1);
 Result rejected;rejected.reject=true;assert(CM_JARCollisionPolygons(2,CONTENTS_SOLID,&rejected,emit)==-1&&rejected.calls==1&&c_active_windings==0);
 NumSubBSP=0;check(0,CONTENTS_SOLID,6,0);Result missing;assert(CM_JARCollisionPolygons(2,CONTENTS_SOLID,&missing,emit)==-1);
#ifndef JAR_CM_MP
 testContents=CONTENTS_TERRAIN;assert(CM_JARCollisionPolygons(0,CONTENTS_SOLID,&missing,emit)==-1);testContents=0;
#endif
 std::puts("PASS: actual SP/MP collision export, world/inline/sub-BSP separation, brush/patch masks, rejection and winding cleanup");
}
