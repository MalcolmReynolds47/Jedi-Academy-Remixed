// Oriented box geometry shared by native volume tools and clipboard validation.
// GPL-2.0-only.
#ifndef JAR_VOLUME_H
#define JAR_VOLUME_H
#include "qcommon/q_shared.h"
#include "../../shared/qcommon/jar_box_contact.h"
#include <cmath>
#include <limits>
namespace jar { namespace volume {
constexpr float minimum=8,maximum=8192;
inline bool Valid(const float*lo,const float*hi){
 for(int a=0;a<3;++a)if(!std::isfinite(lo[a])||!std::isfinite(hi[a])||std::fabs(lo[a])>maximum||std::fabs(hi[a])>maximum||hi[a]-lo[a]<minimum||hi[a]-lo[a]>maximum)return false;
 return true;
}
inline void Hull(const float*lo,const float*hi,const float*angles,float*mins,float*maxs){
 vec3_t axis[3],center,half;AnglesToAxis(angles,axis);
 for(int a=0;a<3;++a){center[a]=(lo[a]+hi[a])*.5f;half[a]=(hi[a]-lo[a])*.5f;}
 for(int a=0;a<3;++a){float middle=0,radius=0;for(int k=0;k<3;++k){middle+=axis[k][a]*center[k];radius+=std::fabs(axis[k][a])*half[k];}mins[a]=middle-radius;maxs[a]=middle+radius;}
}
inline bool Contains(const float*lo,const float*hi,const float*origin,const float*angles,const float*point){
 if(!Valid(lo,hi))return false;
 vec3_t delta,axis[3];VectorSubtract(point,origin,delta);AnglesToAxis(angles,axis);
 for(int a=0;a<3;++a){const float local=DotProduct(delta,axis[a]);if(!std::isfinite(local)||local<lo[a]-.001f||local>hi[a]+.001f)return false;}
 return true;
}
inline bool Contact(const vec3_t lo, const vec3_t hi, const vec3_t origin, const vec3_t angles,
    const vec3_t subjectOrigin, const vec3_t subjectMins, const vec3_t subjectMaxs)
{
 if (!Valid(lo, hi)) return false;
 vec3_t axes[3], center, half;
 AnglesToAxis(angles, axes);
 VectorCopy(origin, center);
 for (int i = 0; i < 3; ++i) {
  VectorMA(center, .5f * (lo[i] + hi[i]), axes[i], center);
  half[i] = .5f * (hi[i] - lo[i]);
 }
 return JAR_BoxAxesContact(center, axes, half, subjectOrigin, subjectMins, subjectMaxs) == qtrue;
}
// Intersect time windows on the 15 separating axes for a translating native
// AABB against a fixed oriented volume.
inline bool Sweep(const vec3_t lo,const vec3_t hi,const vec3_t origin,const vec3_t angles,
 const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,float& fraction){
 if(!Valid(lo,hi))return false;
 vec3_t axes[3],center,half,offset,travel,subjectHalf;
 AnglesToAxis(angles,axes);VectorCopy(origin,center);VectorSubtract(end,start,travel);
 for(int i=0;i<3;++i){
  VectorMA(center,.5f*(lo[i]+hi[i]),axes[i],center);half[i]=.5f*(hi[i]-lo[i]);
  if(!std::isfinite(start[i])||!std::isfinite(end[i])||!std::isfinite(mins[i])||
     !std::isfinite(maxs[i])||mins[i]>maxs[i])return false;
  subjectHalf[i]=.5f*(maxs[i]-mins[i]);offset[i]=start[i]+.5f*(mins[i]+maxs[i]);
 }
 VectorSubtract(offset,center,offset);
 float first=0,last=1;
 const auto overlap=[&](const vec3_t axis){
  if(VectorLengthSquared(axis)<1e-10f)return true;
  float radius=.001f;
  for(int i=0;i<3;++i)radius+=half[i]*std::fabs(DotProduct(axis,axes[i]))+subjectHalf[i]*std::fabs(axis[i]);
  const float distance=DotProduct(offset,axis),speed=DotProduct(travel,axis);
  if(!std::isfinite(radius)||!std::isfinite(distance)||!std::isfinite(speed))return false;
  if(std::fabs(speed)<1e-6f)return std::fabs(distance)<=radius;
  const float enter=(-radius-distance)/speed,exit=(radius-distance)/speed;
  first=std::fmax(first,std::fmin(enter,exit));last=std::fmin(last,std::fmax(enter,exit));
  return first<=last;
 };
 for(int i=0;i<3;++i){
  vec3_t world={0,0,0};world[i]=1;
  if(!overlap(world)||!overlap(axes[i]))return false;
  for(const auto& local:axes){vec3_t cross;CrossProduct(world,local,cross);if(!overlap(cross))return false;}
 }
 fraction=first;
 return true;
}
inline bool Ray(const float*lo,const float*hi,const float*origin,const float*angles,const float*start,const float*direction,float limit,float&hit){
 if(!Valid(lo,hi)||!std::isfinite(limit)||limit<0)return false;
 for(int a=0;a<3;++a)if(!std::isfinite(origin[a])||!std::isfinite(angles[a])||!std::isfinite(start[a])||!std::isfinite(direction[a]))return false;
 vec3_t delta,axis[3];VectorSubtract(start,origin,delta);AnglesToAxis(angles,axis);float near=0,far=std::numeric_limits<float>::max();bool inside=true;
 for(int a=0;a<3&&near<=far;++a){const float s=DotProduct(delta,axis[a]),d=DotProduct(direction,axis[a]);
  inside&=s>=lo[a]&&s<=hi[a];
  if(std::fabs(d)<.00001f){if(s<lo[a]||s>hi[a])return false;continue;}
  float first=(lo[a]-s)/d,last=(hi[a]-s)/d;if(first>last){const float swap=first;first=last;last=swap;}
  near=std::fmax(near,first);far=std::fmin(far,last);
 }if(near>far)return false;const float distance=inside?far:near;if(distance>limit)return false;hit=distance;return true;
}
} }
#endif
