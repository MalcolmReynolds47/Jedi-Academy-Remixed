// Hull sweeps use the native BSP traversal and surface clipping margin.
// Included by cm_trace.cpp. GPL-2.0-only.
#include "../../shared/qcommon/jar_sweep_hull.h"
#include <algorithm>
#include <cmath>

float CM_WorkshopHullMin(const traceWork_t& trace,const vec3_t normal){
 const auto& hull=*trace.workshopHull;
 float low=DotProduct(hull.points[0],normal);
 for(int i=1;i<hull.pointCount;++i)low=std::min(low,DotProduct(hull.points[i],normal));
 return low-DotProduct(trace.workshopCenter,normal)-JAR_SweepPadding(hull,normal);
}

static bool CM_TraceWorkshopAxialBrush(traceWork_t& work,const cbrush_t& brush){
 if(!work.workshopHull||brush.numsides!=6)return false;
 const cbrushside_t* sides[6]{};
 for(int i=0;i<6;++i){
  const auto& normal=brush.sides[i].plane->normal;
  int axis=0;
  while(axis<3&&normal[axis]==0)++axis;
  if(axis==3||std::fabs(normal[axis])!=1)return false;
  for(int j=axis+1;j<3;++j)if(normal[j]!=0)return false;
  const int slot=2*axis+(normal[axis]<0);
  const float dist=normal[axis]<0?-brush.bounds[0][axis]:brush.bounds[1][axis];
  if(sides[slot]||brush.sides[i].plane->dist!=dist)return false;
  sides[slot]=brush.sides+i;
 }

 const auto& hull=*work.workshopHull;
 float enter=0,leave=1,speedAtEntry=0,planeDist=0;
 vec3_t contact{};
 bool insideStart=true,insideEnd=true;
 const auto clipAxis=[&](const vec3_t raw){
  const float length=VectorLength(raw);
  if(length<1e-6f)return true;
  vec3_t normal,opposite;
  VectorScale(raw,1/length,normal);VectorNegate(normal,opposite);
  float fixedLow=0,fixedHigh=0;
  for(int axis=0;axis<3;++axis){
   const int side=normal[axis]<0;
   fixedLow+=normal[axis]*brush.bounds[side][axis];
   fixedHigh+=normal[axis]*brush.bounds[!side][axis];
  }
  const float low=fixedLow+CM_WorkshopHullMin(work,opposite);
  const float high=fixedHigh-CM_WorkshopHullMin(work,normal);
  const float first=DotProduct(work.start,normal),last=DotProduct(work.end,normal);
  if((first<low&&last<low)||(first>high&&last>high))return false;
  insideStart&=first>=low&&first<=high;
  insideEnd&=last>=low&&last<=high;
  const float speed=last-first;
  if(speed==0)return true;
  const bool increasing=speed>0;
  const float entry=((increasing?low:high)-first)/speed;
  if(entry>enter){
   enter=entry;speedAtEntry=std::fabs(speed);
   VectorCopy(increasing?opposite:normal,contact);
   planeDist=increasing?-fixedLow:fixedHigh;
  }
  leave=std::min(leave,((increasing?high:low)-first)/speed);
  return enter<=leave;
 };
 // The box and hull face axes plus crossed edge directions fully describe
 // the separating planes for a fixed-orientation hull against this brush.
 const vec3_t boxAxes[]={{1,0,0},{0,1,0},{0,0,1}};
 for(const auto& axis:boxAxes)if(!clipAxis(axis))return true;
 for(int i=0;i<hull.faceCount;++i)if(!clipAxis(hull.faces[i]))return true;
 for(int i=0;i<hull.edgeCount;++i)for(const auto& axis:boxAxes){
  vec3_t bevel;
  CrossProduct(hull.edges[i],axis,bevel);
  if(!clipAxis(bevel))return true;
 }
 if(insideStart){
  work.trace.startsolid=qtrue;work.trace.contents|=brush.contents;
  if(insideEnd){work.trace.allsolid=qtrue;work.trace.fraction=0;}
  return true;
 }
 if(speedAtEntry==0)return true;
 const float fraction=std::max(0.f,enter-float(SURFACE_CLIP_EPSILON)/speedAtEntry);
 if(fraction>=work.trace.fraction)return true;
 work.trace.fraction=fraction;work.trace.contents=brush.contents;
 VectorCopy(contact,work.trace.plane.normal);work.trace.plane.dist=planeDist;
 work.trace.plane.type=PlaneTypeForNormal(work.trace.plane.normal);
 SetPlaneSignbits(&work.trace.plane);
 // For an edge hit, choose the brush face most closely aligned with the
 // contact normal for surface flags.
 int major=0;
 for(int axis=1;axis<3;++axis)if(std::fabs(contact[axis])>std::fabs(contact[major]))major=axis;
 const auto& map=work.workshopMap?*work.workshopMap:cmg;
 work.trace.surfaceFlags=map.shaders[sides[2*major+(contact[major]<0)]->shaderNum].surfaceFlags;
 return true;
}
