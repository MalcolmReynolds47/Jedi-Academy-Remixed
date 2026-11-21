// Shared MP renderer pose contract. Bounds reuse the qualified SP traversal.
// GPL-2.0-only.
static void* G2_FindSurface(const model_s*model,int index,int lod){return G2_FindSurface(static_cast<void*>(const_cast<model_s*>(model)),index,lod);}
#include "../../code/rd-common/jar_ghoul_bounds.inc"

qboolean G2API_JAR_PoseGeometry(CGhoul2Info_v& ghoul2,int frame,int*frames,vec3_t low,vec3_t high,
 void* context,void (*emit)(void*,const float*)){
 if(frames)*frames=0;
 if(frame<0||frame>65535||ghoul2.size()!=1||!G2_SetupModelPointers(ghoul2))return qfalse;
 auto& g=ghoul2[0];const auto*h=g.aHeader;
 if(!h||h->ident!=MDXA_IDENT||h->numFrames<1||h->numFrames>65536||h->numBones<1||h->numBones>512||h->ofsEnd<int(sizeof(*h))+h->numBones*4)return qfalse;
 const auto*base=reinterpret_cast<const byte*>(h)+sizeof(*h);const auto*offsets=reinterpret_cast<const int*>(base);
 const char*roots[512];int count=0;
 for(int i=0;i<h->numBones;++i){const int offset=offsets[i];
  if(offset<h->numBones*4||offset>h->ofsEnd-int(sizeof(*h))-int(offsetof(mdxaSkel_t,children)))return qfalse;
  const auto*bone=reinterpret_cast<const mdxaSkel_t*>(base+offset);
  if(bone->parent< -1||bone->parent>=h->numBones||!memchr(bone->name,0,sizeof(bone->name))||!bone->name[0])return qfalse;
  if(bone->parent==-1)roots[count++]=bone->name;
 }
 if(!count)return qfalse;if(frames)*frames=h->numFrames;if(frame>=h->numFrames)return qfalse;
 const auto previous=g.mBlist;
 for(int i=0;i<count;++i){
  // A one-frame positive-speed freeze keeps both native timing frames identical.
  // MP has no NO_LERP flag and its zero-speed branch uses reverse-animation math.
  if(!G2API_SetBoneAnim(ghoul2,0,roots[i],frame,frame+1,BONE_ANIM_OVERRIDE_FREEZE,1,0,-1,0)){
   g.mBlist=previous;g.mSkelFrameNum=-1;return qfalse;
  }
 }
 // Discard render smoothing history so a discrete pose edit cannot display
 // yesterday's vertices while the server already uses the new collision hull.
 if(g.mBoneCache){RemoveBoneCache(g.mBoneCache);g.mBoneCache=nullptr;}
 g.mSkelFrameNum=-1;
 if(G2API_JAR_ModelGeometry(ghoul2,0,low,high,context,emit))return qtrue;
 g.mBlist=previous;g.mSkelFrameNum=-1;return qfalse;
}

qboolean G2API_JAR_PoseBounds(CGhoul2Info_v& ghoul2,int frame,int*frames,vec3_t low,vec3_t high){
 return G2API_JAR_PoseGeometry(ghoul2,frame,frames,low,high,nullptr,nullptr);
}
