// JA:R collision export. GPL-2.0-only; reuses OpenJK winding and facet helpers.
// The native collision map remains authoritative and is never modified.
#ifndef JAR_CM_MP
#include "cm_polylib.h"
#endif
#include <vector>
#include <cmath>
int CM_JARCollisionPolygons(int model, int mask, void* context,
 qboolean (*emit)(void*,int,const float*,const float*)) {
 if(!emit || model<0 || model==BOX_MODEL_HANDLE)return -1;
 // Entity model indices span the main map followed by each loaded sub-BSP.
 // Reject unknown handles here instead of invoking the engine's fatal lookup.
 const clipMap_t* selected=&cmg;int local=model;
 if(local>=cmg.numSubModels){
  local-=cmg.numSubModels;selected=nullptr;
  for(int i=0;i<NumSubBSP;++i){
   if(local<SubBSP[i].numSubModels){selected=&SubBSP[i];break;}
   local-=SubBSP[i].numSubModels;
  }
 }
 if(!selected || local<0 || local>=selected->numSubModels)return -1;
 const auto&cm=*selected;
#ifndef JAR_CM_MP
 if(CM_TotalMapContents()&CONTENTS_TERRAIN)return -1;
#endif
 std::vector<unsigned char> brushes(cm.numBrushes,0),patches(cm.numSurfaces,0);
 auto mark=[&](const cLeaf_t& leaf){
  for(int i=0;i<leaf.numLeafBrushes;++i){const int n=cm.leafbrushes[leaf.firstLeafBrush+i];if(n>=0&&n<cm.numBrushes)brushes[n]=1;}
  for(int i=0;i<leaf.numLeafSurfaces;++i){const int n=cm.leafsurfaces[leaf.firstLeafSurface+i];if(n>=0&&n<cm.numSurfaces)patches[n]=1;}
 };
 // A sub-BSP root has a model leaf, just like an ordinary inline brush.
 // Only the main world uses the BSP tree leaves; never merge its child doors.
 if(model==0){for(int i=0;i<cm.numLeafs;++i)mark(cm.leafs[i]);}
 else mark(cm.cmodels[local].leaf);
 int polygons=0;
 auto send=[&](winding_t* w,const float* normal){
  if(!w)return true;
  bool ok=w->numpoints>=3 && w->numpoints<=MAX_POINTS_ON_WINDING;
  for(int i=0;i<w->numpoints&&ok;++i)for(int a=0;a<3;++a)
   if(!std::isfinite(w->p[i][a]) || std::fabs(w->p[i][a])>=MAX_MAP_BOUNDS)ok=false;
  if(ok){ok=polygons<262144 && emit(context,w->numpoints,&w->p[0][0],normal)!=qfalse;++polygons;}
  FreeWinding(w);return ok;
 };
 for(int n=0;n<cm.numBrushes;++n){const auto& b=cm.brushes[n];
  if(!brushes[n] || !(b.contents&mask))continue;
  if(b.numsides>60)return -1;
  for(int side=0;side<b.numsides;++side){const auto* p=b.sides[side].plane;
   winding_t* w=BaseWindingForPlane(const_cast<float*>(p->normal),p->dist);
   for(int j=0;j<b.numsides&&w;++j)if(j!=side){const auto* clip=b.sides[j].plane;vec3_t inward;
    VectorNegate(clip->normal,inward);ChopWindingInPlace(&w,inward,-clip->dist,0.01f);}
   if(!send(w,p->normal))return -1;
  }
 }
 for(int n=0;n<cm.numSurfaces;++n){const auto* p=cm.surfaces[n];
  if(!patches[n] || !p || !(p->contents&mask))continue;
  const auto* pc=p->pc;
  for(int i=0;i<pc->numFacets;++i){const auto& f=pc->facets[i];const auto* surface=pc->planes[f.surfacePlane].plane;
   winding_t* w=BaseWindingForPlane(const_cast<float*>(surface),surface[3]);
   for(int j=0;j<f.numBorders&&w;++j){if(f.borderPlanes[j]==f.surfacePlane)continue;
    const auto* border=pc->planes[f.borderPlanes[j]].plane;vec3_t normal;float distance=border[3];VectorCopy(border,normal);
    if(!f.borderInward[j]){VectorNegate(normal,normal);distance=-distance;}
    ChopWindingInPlace(&w,normal,distance,0.01f);
   }
   if(!send(w,surface))return -1;
  }
 }
 return polygons;
}
