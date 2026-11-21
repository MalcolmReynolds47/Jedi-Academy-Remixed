// Fixed-orientation Sandbox hull, in trace-origin-relative game units.
// This plain data crosses the SP engine/game boundary. GPL-2.0-only.
#ifndef JAR_SWEEP_HULL_H
#define JAR_SWEEP_HULL_H
#include <algorithm>
#include <cmath>
struct JarSweepHull {
 // A bounded convex volume with 26 support planes has at most 48 corners.
 static constexpr int modelPoints=48;
 static constexpr int modelFaces=2*modelPoints-4;
 static constexpr int modelEdges=3*modelPoints-6;
 static constexpr int pointLimit=2*modelPoints;
 static constexpr int faceLimit=2*modelFaces;
 static constexpr int edgeLimit=2*modelEdges;
 int pointCount,faceCount,edgeCount;
 float arcRadius,arcAxis[3];
 float points[pointLimit][3],faces[faceLimit][3],edges[edgeLimit][3];
};

// Arc padding is a disc perpendicular to the world rotation axis.
inline float JAR_SweepPadding(const JarSweepHull& hull,const float* normal){
 if(hull.arcRadius<=0)return 0;
 const auto* axis=hull.arcAxis;
 const float x=axis[1]*normal[2]-axis[2]*normal[1];
 const float y=axis[2]*normal[0]-axis[0]*normal[2];
 const float z=axis[0]*normal[1]-axis[1]*normal[0];
 return hull.arcRadius*std::sqrt(x*x+y*y+z*z);
}
inline void JAR_SweepBounds(const JarSweepHull& hull,float* mins,float* maxs){
 for(int axis=0;axis<3;++axis){
  float low=hull.points[0][axis],high=low;
  for(int i=1;i<hull.pointCount;++i){
   low=std::min(low,hull.points[i][axis]);
   high=std::max(high,hull.points[i][axis]);
  }
  float normal[3]={};normal[axis]=1;
  const float padding=JAR_SweepPadding(hull,normal);
  mins[axis]=low-padding;maxs[axis]=high+padding;
 }
}
#endif
