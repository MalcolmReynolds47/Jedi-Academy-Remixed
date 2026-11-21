// Sandbox placement and out-of-map boundaries. GPL-2.0-only.
#ifndef G_JAR_WORKSHOP_PLACEMENT_H
#define G_JAR_WORKSHOP_PLACEMENT_H
#include "g_local.h"
#include <algorithm>
#include <cmath>
#include "../../shared/game/jar_fall_cleanup.h"
#include "../../shared/game/jar_world_anchor.h"
#include "../../shared/qcommon/jar_sweep_hull.h"

inline void G_JAR_WorkshopAnchorTrace(trace_t* hit,const vec3_t start,const vec3_t end){
 gi.trace(hit,start,nullptr,nullptr,end,ENTITYNUM_NONE,MASK_SOLID,G2_NOCOLLIDE,0);
}

inline void G_JAR_WorkshopSceneryTrace(trace_t& trace,const vec3_t start,const vec3_t end,
 const vec3_t mins,const vec3_t maxs,int pass=ENTITYNUM_NONE,const JarSweepHull* shape=nullptr){
 if(shape&&gi.JAR_TraceSceneryHull){
  vec3_t low,high;
  JAR_SweepBounds(*shape,low,high);
  gi.JAR_TraceSceneryHull(&trace,start,end,low,high,pass,MASK_SOLID,*shape);
 }else gi.trace(&trace,start,mins,maxs,end,pass,MASK_SOLID,G2_BRUSH_ONLY,0);
}

inline float G_JAR_WorkshopSurfaceOffset(const vec3_t mins,const vec3_t maxs,const vec3_t normal,
 const JarSweepHull* shape){
 if(shape){
  float nearest=DotProduct(normal,shape->points[0]);
  for(int i=1;i<shape->pointCount;++i)nearest=std::min(nearest,DotProduct(normal,shape->points[i]));
  return 1-nearest+JAR_SweepPadding(*shape,normal);
 }
 float offset=1;
 for(int axis=0;axis<3;++axis)offset-=normal[axis]*(normal[axis]>=0?mins[axis]:maxs[axis]);
 return offset;
}

inline bool G_JAR_WorkshopSectionInWorld(const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 for(int axis=0;axis<3;++axis){
  const float low=origin[axis]+mins[axis],high=origin[axis]+maxs[axis];
  if(!(low>MIN_WORLD_COORD&&high<MAX_WORLD_COORD&&low<=high))return false;
 }
 return true;
}

inline bool G_JAR_WorkshopPlaceHull(vec3_t origin,const vec3_t mins,const vec3_t maxs,
 const vec3_t normal,bool needsLevelSurface,const JarSweepHull* shape=nullptr){
 trace_t trace{};
 G_JAR_WorkshopSceneryTrace(trace,origin,origin,mins,maxs,ENTITYNUM_NONE,shape);
 if(!trace.startsolid&&!trace.allsolid)return true;
 // A point hit in a dip can leave the hull on the wrong side of nearby
 // world brushes. Find the first full-hull contact from clear space.
 vec3_t clearance;
 VectorMA(origin,64,normal,clearance);
 G_JAR_WorkshopSceneryTrace(trace,clearance,origin,mins,maxs,ENTITYNUM_NONE,shape);
 if(trace.startsolid||trace.allsolid||trace.fraction>=1||
    (needsLevelSurface&&trace.plane.normal[2]<.7f))return false;
 VectorMA(trace.endpos,1,trace.plane.normal,origin);
 G_JAR_WorkshopSceneryTrace(trace,origin,origin,mins,maxs,ENTITYNUM_NONE,shape);
 return !trace.startsolid&&!trace.allsolid;
}

#endif
