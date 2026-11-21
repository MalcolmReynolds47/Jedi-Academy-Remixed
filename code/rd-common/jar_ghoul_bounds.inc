// Shared by both SP renderers. Measure the same posed LOD-0 vertices used by
// Ghoul2 collision, without allocating from or invalidating its trace heap.
// GPL-2.0-only.
#include <cmath>
#include <limits>
const mdxaBone_t& EvalBoneCache(int,CBoneCache*);

static bool JAR_GhoulSurfaceBounds(CGhoul2Info& g, vec3_t low, vec3_t high,
 void* context=nullptr, void (*emit)(void*,const float*)=nullptr) {
 if (!g.mValid || !g.currentModel || !g.aHeader || !g.mBoneCache) return false;
#ifdef REND2_SP
 const auto* h=g.currentModel->data.glm?g.currentModel->data.glm->header:nullptr;
#else
 const auto* h=g.currentModel->mdxm;
#endif
 if(!h)return false;
 if(h->numSurfaces<1 || h->numSurfaces>512 || g.mSurfaceRoot<0 || g.mSurfaceRoot>=h->numSurfaces) return false;
 const auto* base=reinterpret_cast<const byte*>(h);
 auto range=[&](int offset, size_t count, size_t size){return offset>=0 && offset<=h->ofsEnd && count<=size_t(h->ofsEnd-offset)/size;};
 if(!range(sizeof(*h),h->numSurfaces,sizeof(int)))return false;
 const auto* offsets=reinterpret_cast<const mdxmHierarchyOffsets_t*>(base+sizeof(*h));
 bool visited[512]{};int pending[512],count=1;pending[0]=g.mSurfaceRoot;unsigned vertices=0;
 vec3_t lo,hi;for(int a=0;a<3;++a){lo[a]=std::numeric_limits<float>::max();hi[a]=-lo[a];}
 G2_FindOverrideSurface(-1,g.mSlist);
 while(count){
  const int index=pending[--count];if(index<0||index>=h->numSurfaces||visited[index])return false;visited[index]=true;
  const int offset=offsets->offsets[index];
  if(offset<0 || offset>h->ofsEnd-int(sizeof(*h)) || !range(int(sizeof(*h))+offset,1,offsetof(mdxmSurfHierarchy_t,childIndexes)))return false;
  const auto* info=reinterpret_cast<const mdxmSurfHierarchy_t*>(reinterpret_cast<const byte*>(offsets)+offset);
  const auto* overrideSurface=G2_FindOverrideSurface(index,g.mSlist);
  const unsigned flags=overrideSurface?overrideSurface->offFlags:info->flags;
  if(!flags){
   const auto* surface=static_cast<const mdxmSurface_t*>(G2_FindSurface(g.currentModel,index,0));
   if(!surface)return false;
   const auto relative=reinterpret_cast<const byte*>(surface)-base;
   if(relative<0 || relative>h->ofsEnd-int(sizeof(*surface)))return false;
   auto part=[&](int offset,size_t count,size_t size){return offset>=int(sizeof(*surface)) && offset<=h->ofsEnd-int(relative) && range(int(relative)+offset,count,size);};
   if(surface->numVerts<0 || surface->numVerts>250000-int(vertices) || surface->numBoneReferences<1 || surface->numBoneReferences>32 ||
      !part(surface->ofsVerts,surface->numVerts,sizeof(mdxmVertex_t)) || !part(surface->ofsBoneReferences,surface->numBoneReferences,sizeof(int)))return false;
   const auto* refs=reinterpret_cast<const int*>(reinterpret_cast<const byte*>(surface)+surface->ofsBoneReferences);
   const auto* verts=reinterpret_cast<const mdxmVertex_t*>(reinterpret_cast<const byte*>(surface)+surface->ofsVerts);
   for(int j=0;j<surface->numVerts;++j){const auto&v=verts[j];vec3_t point{};float total=0;const int weights=G2_GetVertWeights(&v);
    for(int k=0;k<weights;++k){const int ref=G2_GetVertBoneIndex(&v,k);if(ref>=surface->numBoneReferences||refs[ref]<0||refs[ref]>=g.aHeader->numBones)return false;
     const float weight=G2_GetVertBoneWeight(&v,k,total,weights);if(weight<0||weight>1)return false;
     const auto&bone=EvalBoneCache(refs[ref],g.mBoneCache);
     for(int a=0;a<3;++a)point[a]+=weight*(DotProduct(bone.matrix[a],v.vertCoords)+bone.matrix[a][3]);
    }
    for(int a=0;a<3;++a){if(!std::isfinite(point[a])||std::fabs(point[a])>8192)return false;lo[a]=std::min(lo[a],point[a]);hi[a]=std::max(hi[a],point[a]);}
    if(emit)emit(context,point);
   }vertices+=surface->numVerts;
  }
  if(flags&G2SURFACEFLAG_NODESCENDANTS)continue;
  if(info->numChildren<0 || info->numChildren>512-count || !range(int(sizeof(*h))+offset+offsetof(mdxmSurfHierarchy_t,childIndexes),info->numChildren,sizeof(int)))return false;
  for(int j=0;j<info->numChildren;++j)pending[count++]=info->childIndexes[j];
 }
 if(!vertices)return false;
 VectorCopy(lo,low);VectorCopy(hi,high);return true;
}

qboolean G2API_JAR_ModelGeometry(CGhoul2Info_v& ghoul2, int time, vec3_t low, vec3_t high,
 void* context, void (*emit)(void*,const float*)) {
 if(ghoul2.size()!=1 || !G2_SetupModelPointers(ghoul2))return qfalse;
 const vec3_t scale={1,1,1};G2_ConstructGhoulSkeleton(ghoul2,G2API_GetTime(time),true,scale);
 return JAR_GhoulSurfaceBounds(ghoul2[0],low,high,context,emit)?qtrue:qfalse;
}

qboolean G2API_JAR_ModelBounds(CGhoul2Info_v& ghoul2, int time, vec3_t low, vec3_t high) {
 return G2API_JAR_ModelGeometry(ghoul2,time,low,high,nullptr,nullptr);
}
