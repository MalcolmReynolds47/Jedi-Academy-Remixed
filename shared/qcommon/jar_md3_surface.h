// Validated MD3 frame geometry shared by the Workshop and Physgun. GPL-2.0-only.
#pragma once
#include <cmath>
#include <vector>

namespace jar { namespace md3 {
template<class Visitor>
bool VisitSurfaces(const md3Header_t& header,int frame,int frames,Visitor visit){
 const auto* base=reinterpret_cast<const byte*>(&header);
 const int end=LittleLong(header.ofsEnd),count=LittleLong(header.numSurfaces);
 int offset=LittleLong(header.ofsSurfaces);
 if(end<int(sizeof(header))||count<1||count>MD3_MAX_SURFACES||offset<int(sizeof(header)))return false;
 for(int i=0;i<count;++i){
  if(offset>end-int(sizeof(md3Surface_t))||offset%alignof(md3Surface_t))return false;
  const auto& surface=*reinterpret_cast<const md3Surface_t*>(base+offset);
  const int size=LittleLong(surface.ofsEnd),verts=LittleLong(surface.numVerts),triangles=LittleLong(surface.numTriangles);
  const int vertexOffset=LittleLong(surface.ofsXyzNormals),triangleOffset=LittleLong(surface.ofsTriangles);
  const auto range=[&](int start,int elements,size_t stride,size_t alignment){
   return start>=int(sizeof(surface))&&start<=size&&start%alignment==0&&elements<=(size-start)/int(stride);
  };
  if(LittleLong(surface.ident)!=MD3_IDENT||LittleLong(surface.numFrames)!=frames||
     size<int(sizeof(surface))||size>end-offset||verts<0||verts>MD3_MAX_VERTS||
     triangles<0||triangles>MD3_MAX_TRIANGLES||
     !range(vertexOffset,verts*frames,sizeof(md3XyzNormal_t),alignof(md3XyzNormal_t))||
     !range(triangleOffset,triangles,sizeof(md3Triangle_t),alignof(md3Triangle_t)))return false;
  const auto* vertices=reinterpret_cast<const md3XyzNormal_t*>(base+offset+vertexOffset)+frame*verts;
  const auto* faces=reinterpret_cast<const md3Triangle_t*>(base+offset+triangleOffset);
  for(int t=0;t<triangles;++t)for(int index:faces[t].indexes)
   if(LittleLong(index)<0||LittleLong(index)>=verts)return false;
  visit(vertices,faces,triangles);
  offset+=size;
 }
 return true;
}

inline void Vertex(const md3XyzNormal_t& v,vec3_t point){
 for(int a=0;a<3;++a)point[a]=MD3_XYZ_SCALE*LittleShort(v.xyz[a]);
}

// Return whether the mesh was available. A valid mesh may miss, so callers
// must distinguish that case from falling back to collision geometry.
inline bool Pick(const void* bytes,int length,int frame,const vec3_t origin,const vec3_t angles,
                 const vec3_t start,const vec3_t end,float& fraction,vec3_t normal){
 if(!bytes||length<int(sizeof(md3Header_t))||frame<0)return false;
 const auto& header=*static_cast<const md3Header_t*>(bytes);
 const int frames=LittleLong(header.numFrames);
 if(LittleLong(header.ident)!=MD3_IDENT||LittleLong(header.version)!=MD3_VERSION||
    frames>65536||frame>=frames||LittleLong(header.ofsEnd)>length)return false;

 vec3_t axis[3],offset,direction,localStart,localDirection;
 AnglesToAxis(angles,axis);
 VectorSubtract(start,origin,offset);
 VectorSubtract(end,start,direction);
 for(int a=0;a<3;++a){
  localStart[a]=DotProduct(offset,axis[a]);
  localDirection[a]=DotProduct(direction,axis[a]);
 }
 float best=fraction;
 vec3_t bestNormal{};
 bool hasFaces=false;
 if(!VisitSurfaces(header,frame,frames,[&](const auto* vertices,const auto* faces,int count){
  for(int t=0;t<count;++t){
   vec3_t a,b,c,u,v,p,q,relative;
   Vertex(vertices[LittleLong(faces[t].indexes[0])],a);
   Vertex(vertices[LittleLong(faces[t].indexes[1])],b);
   Vertex(vertices[LittleLong(faces[t].indexes[2])],c);
   VectorSubtract(b,a,u);
   VectorSubtract(c,a,v);
   CrossProduct(u,v,p);
   if(DotProduct(p,p)<1e-12f)continue;
   hasFaces=true;
   // The ray is two-sided, including on stock meshes with open surfaces.
   CrossProduct(localDirection,v,p);
   const float det=DotProduct(u,p);
   if(std::fabs(det)<1e-8f)continue;
   VectorSubtract(localStart,a,relative);
   const float x=DotProduct(relative,p)/det;
   if(x<0||x>1)continue;
   CrossProduct(relative,u,q);
   const float y=DotProduct(localDirection,q)/det;
   if(y<0||x+y>1)continue;
   const float hit=DotProduct(v,q)/det;
   if(hit<0||hit>=best)continue;
   best=hit;
   CrossProduct(u,v,bestNormal);
   if(DotProduct(bestNormal,localDirection)>0)VectorNegate(bestNormal,bestNormal);
   VectorNormalize(bestNormal);
  }
 }))return false;
 if(best<fraction){
  fraction=best;
  VectorClear(normal);
  for(int a=0;a<3;++a)VectorMA(normal,bestNormal[a],axis[a],normal);
 }
 return hasFaces;
}

// Read on pickup only. The SP whole-file helper clears active sound buffers.
template<class Open,class Read,class Close>
bool PickFile(const char* model,int frame,const vec3_t origin,const vec3_t angles,
              const vec3_t start,const vec3_t end,float& fraction,vec3_t normal,
              Open open,Read read,Close close){
 fileHandle_t file=0;
 const long length=open(model,&file,qfalse);
 if(!file)return false;
 bool available=false;
 if(length>=long(sizeof(md3Header_t))&&length<=64*1024*1024){
  std::vector<byte> bytes(length);
  if(read(bytes.data(),int(length),file)==length)
   available=Pick(bytes.data(),int(length),frame,origin,angles,start,end,fraction,normal);
 }
 close(file);
 return available;
}
} }
