#include "../../shared/qcommon/jar_thruster.h"
// Original JA:R bridge; Box3D is separately licensed MIT in lib/box3d/LICENSE.
#include "jar_physics.h"
#include "box3d/box3d.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>
namespace jar { namespace physics {
namespace {
constexpr float scale=1.f/32.f,step=1.f/120.f,rad=.01745329251994329577f;
constexpr uint64_t ordinaryCategory=1,mountedCategory=2,mountProxyCategory=4;
b3Vec3 V(Vec v,float s=1){return {v.x*s,v.y*s,v.z*s};}
Vec V(b3Vec3 v,float s=1){return {v.x*s,v.y*s,v.z*s};}
b3Pos P(Vec v){return {v.x*scale,v.y*scale,v.z*scale};}
// Use the native trigonometric boundary here. The solver's approximate trig is
// useful internally, but repeated Euler conversions must not drift frozen props.
b3Quat Axis(b3Vec3 axis,float angle){const float s=std::sin(angle*.5f);return {{axis.x*s,axis.y*s,axis.z*s},std::cos(angle*.5f)};}
b3Quat Q(Vec a){return b3NormalizeQuat(b3MulQuat(b3MulQuat(Axis({0,0,1},a.y*rad),Axis({0,1,0},a.x*rad)),Axis({1,0,0},a.z*rad)));}
Vec Angles(b3Quat q){const auto x=b3RotateVector(q,{1,0,0}),y=b3RotateVector(q,{0,1,0}),z=b3RotateVector(q,{0,0,1});
 const float horizontal=std::sqrt(x.x*x.x+x.y*x.y);
 if(horizontal<.00001f)return {std::atan2(-x.z,horizontal)/rad,std::atan2(-y.x,y.y)/rad,0};
 return {std::atan2(-x.z,horizontal)/rad,std::atan2(x.y,x.x)/rad,std::atan2(y.z,z.z)/rad};}
bool finite(Vec a,float limit){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z)&&std::fabs(a.x)<limit&&std::fabs(a.y)<limit&&std::fabs(a.z)<limit;}
bool valid(const Pose&p){return finite(p.position,65536)&&finite(p.angles,100000)&&finite(p.linear,5000)&&finite(p.angular,1000);}
bool same(Vec a,Vec b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
b3Vec3 RotationServo(b3Quat current,b3Quat goal){
 current.v=b3Neg(current.v);
 auto error=b3NormalizeQuat(b3MulQuat(goal,current));
 if(error.s<0){error.s=-error.s;error.v=b3Neg(error.v);}
 const float length=b3Length(error.v);
 if(length<=.00001f)return {};
 return b3MulSV(std::min(8.f,24.f*std::atan2(length,error.s))/length,error.v);
}
b3MeshData* meshOf(const std::vector<Vec>&triangles){
 if(triangles.empty()||triangles.size()%3||triangles.size()>triangleLimit*3)return nullptr;
 std::vector<b3Vec3> vertices;std::vector<int32_t> indices;vertices.reserve(triangles.size());indices.reserve(triangles.size());
 for(const auto&v:triangles){if(!finite(v,65536))return nullptr;vertices.push_back(V(v,scale));indices.push_back(int32_t(indices.size()));}
 b3MeshDef md{};md.vertices=vertices.data();md.indices=indices.data();md.vertexCount=int(vertices.size());md.triangleCount=int(vertices.size()/3);
 md.weldVertices=true;md.weldTolerance=.001f;md.identifyEdges=true;
 return b3CreateMesh(&md,nullptr,0);
}

// The signed axes and diagonals are shared by every prop. Precompute which
// triples can meet at a corner and their inverse matrices once.
struct PropSupportPlanes {
 struct Intersection {int a,b,c; b3Vec3 u,v,w; double invDet;};
 std::array<b3Vec3,27> directions{};
 std::vector<Intersection> corners;
 PropSupportPlanes(){
  for(int z=-1;z<=1;++z)for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)
   directions[13+x+3*y+9*z]={float(x),float(y),float(z)};
  for(int a=0;a<27;++a)for(int b=a+1;b<27;++b)for(int c=b+1;c<27;++c){
   const auto u=b3Cross(directions[b],directions[c]);
   const auto v=b3Cross(directions[c],directions[a]);
   const auto w=b3Cross(directions[a],directions[b]);
   const float determinant=b3Dot(directions[a],u);
   if(determinant!=0)corners.push_back({a,b,c,u,v,w,1.0/determinant});
  }
 }
};

// Box3D can weld small features during construction. Check its returned
// planes against the inputs before adopting the hull.
b3HullData* BuildCheckedPropHull(const b3Vec3* points,int count,float coverageTolerance){
 auto* hull=b3CreateHull(points,count,count);
 if(!hull)return nullptr;
 const auto* faces=b3GetHullPlanes(hull);
 for(int i=0;i<hull->faceCount;++i)for(int j=0;j<count;++j){
  if(b3Dot(faces[i].normal,points[j])-faces[i].offset<=coverageTolerance)continue;
  b3DestroyHull(hull);
  return nullptr;
 }
 return hull;
}

b3HullData* BuildPropSupportHull(const std::array<float,27>& supports,float pad){
 static const PropSupportPlanes planes;
 std::array<double,27> offsets;
 for(size_t i=0;i<offsets.size();++i)offsets[i]=supports[i]+double(pad)*b3Length(planes.directions[i]);
 std::array<b3Vec3,JarSweepHull::modelPoints> points;
 int count=0;
 for(const auto& corner:planes.corners){
  const double da=offsets[corner.a],db=offsets[corner.b],dc=offsets[corner.c];
  const double x=(da*corner.u.x+db*corner.v.x+dc*corner.w.x)*corner.invDet;
  const double y=(da*corner.u.y+db*corner.v.y+dc*corner.w.y)*corner.invDet;
  const double z=(da*corner.u.z+db*corner.v.z+dc*corner.w.z)*corner.invDet;
  bool inside=true;
  for(size_t i=0;i<offsets.size();++i){
   const auto& n=planes.directions[i];
   if(x*n.x+y*n.y+z*n.z>offsets[i]+1e-7){inside=false;break;}
  }
  if(!inside)continue;
  const b3Vec3 point={float(x*scale),float(y*scale),float(z*scale)};
  if(std::any_of(points.begin(),points.begin()+count,[&](b3Vec3 existing){
   const auto delta=b3Sub(existing,point);
   return b3Dot(delta,delta)<1e-12f;
  }))continue;
  if(count==int(points.size()))return nullptr;
  points[count++]=point;
 }
 return BuildCheckedPropHull(points.data(),count,.0625f*scale);
}
}
bool PhysgunGrip::Begin(Vec origin,Vec angles,Vec point,float viewYaw){
 active=false;
 if(!finite(origin,65536)||!finite(point,65536)||!finite(angles,100000)||!std::isfinite(viewYaw))return false;
 localPoint=V(b3InvRotateVector(Q(angles),b3Sub(V(point),V(origin))));
 relativeAngles=angles;
 relativeAngles.y=std::remainder(angles.y-viewYaw,360.f);
 return active=finite(localPoint,65536);
}
Vec PhysgunGrip::Point(Vec origin,Vec angles) const{
 return V(b3Add(V(origin),b3RotateVector(Q(angles),V(localPoint))));
}
Vec PhysgunGrip::Origin(Vec point,Vec angles) const{
 return V(b3Sub(V(point),b3RotateVector(Q(angles),V(localPoint))));
}
Vec PhysgunGrip::Facing(float viewYaw) const{
 auto facing=relativeAngles;
 facing.y=std::remainder(facing.y+viewYaw,360.f);
 return facing;
}
void PhysgunGrip::Turn(float yaw,float pitch,float viewPitch,float viewRoll){
 if(!active||!finite({yaw,pitch,0},181)||!finite({viewPitch,viewRoll,0},100000))return;
 // USE rotates around the camera's horizontal and vertical axes even
 // when the picked-up side of the prop is turned away from its model axes.
 // The grip already stores angles relative to camera yaw.
 const auto tilt=Q({viewPitch,0,viewRoll});
 const auto up=b3RotateVector(tilt,{0,0,1});
 const auto left=b3RotateVector(tilt,{0,1,0});
 const auto turn=b3MulQuat(Axis(up,yaw*rad),Axis(left,pitch*rad));
 relativeAngles=Angles(b3NormalizeQuat(b3MulQuat(turn,Q(relativeAngles))));
}
void PropGeometry::Add(Vec point,SupportPoints* witnesses){
 if(points.size()<=JarSweepHull::modelPoints&&
    std::none_of(points.begin(),points.end(),[&](Vec existing){return same(existing,point);}))
  points.push_back(point);
 for(int z=-1;z<=1;++z)for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){
  if(!x&&!y&&!z)continue;
  const int index=13+x+3*y+9*z;
  const float projection=x*point.x+y*point.y+z*point.z;
  if(!sampled||projection>supports[index]){
   supports[index]=projection;
   if(witnesses)(*witnesses)[index]=point;
  }
 }
 sampled=true;
}
float PropGeometry::Support(Vec direction) const{
 const float components[]={direction.x,direction.y,direction.z};
 const float magnitude[]={std::fabs(direction.x),std::fabs(direction.y),std::fabs(direction.z)};
 constexpr int strides[]={1,3,9};
 int order[]={0,1,2};
 std::sort(order,order+3,[&](int a,int b){return magnitude[a]>magnitude[b];});
 // Decompose the direction into nested signed axis/diagonal vectors.
 // The weighted sum of their maxima encloses all original vertices.
 float bound=0;
 int index=13;
 for(int i=0;i<3;++i){
  const int axis=order[i];
  index+=components[axis]<0?-strides[axis]:strides[axis];
  const float next=i==2?0:magnitude[order[i+1]];
  bound+=(magnitude[axis]-next)*supports[index];
 }
 return bound;
}
void PropGeometry::Bounds(const Vec (&localAxes)[3],Vec& low,Vec& high) const{
 float minimum[3],maximum[3];
 for(int i=0;i<3;++i){
  const auto axis=localAxes[i];
  minimum[i]=-Support({-axis.x,-axis.y,-axis.z});
  maximum[i]=Support(axis);
  const float padding=std::max(.001f,(.5f-(maximum[i]-minimum[i]))*.5f);
  minimum[i]-=padding;maximum[i]+=padding;
 }
 low={minimum[0],minimum[1],minimum[2]};
 high={maximum[0],maximum[1],maximum[2]};
}
const b3HullData* PropGeometry::RigidHull() const{
 if(!rigidReady){
  rigidReady=true;
  bool volumetric=sampled;
  for(int i=0;i<13;++i)volumetric&=supports[i]+supports[26-i]>.001f;
  if(volumetric){
   b3HullData* hull=nullptr;
   if(points.size()>=4&&points.size()<=JarSweepHull::modelPoints){
    std::array<b3Vec3,JarSweepHull::modelPoints> modelPoints;
    std::transform(points.begin(),points.end(),modelPoints.begin(),[](Vec point){return V(point,scale);});
    hull=BuildCheckedPropHull(modelPoints.data(),int(points.size()),.03125f*scale);
   }
   if(!hull)hull=BuildPropSupportHull(supports,0);
   // Tiny or flat geometry can need space for a stable solver hull.
   if(!hull)hull=BuildPropSupportHull(supports,.04f);
   if(hull)rigidHull={hull,b3DestroyHull};
  }
 }
 return rigidHull.get();
}
bool BuildPropSweep(const PropGeometry& geometry,Vec angles,JarSweepHull& sweep){
 sweep.pointCount=sweep.faceCount=sweep.edgeCount=0;
 sweep.arcRadius=0;sweep.arcAxis[0]=sweep.arcAxis[1]=sweep.arcAxis[2]=0;
 const auto* hull=geometry.RigidHull();
 if(!hull||hull->vertexCount>JarSweepHull::modelPoints||hull->faceCount>JarSweepHull::modelFaces||
    hull->edgeCount/2>JarSweepHull::modelEdges)return false;
 const auto rotation=Q(angles);
 const auto* points=b3GetHullPoints(hull);
 const auto* edges=b3GetHullEdges(hull);
 const auto* planes=b3GetHullPlanes(hull);
 const auto store=[](float* dest,b3Vec3 v){dest[0]=v.x;dest[1]=v.y;dest[2]=v.z;};
 for(int i=0;i<hull->vertexCount;++i)
  store(sweep.points[sweep.pointCount++],b3RotateVector(rotation,b3MulSV(1/scale,points[i])));
 for(int i=0;i<hull->faceCount;++i)
  store(sweep.faces[sweep.faceCount++],b3RotateVector(rotation,planes[i].normal));
 const auto point=[&](int i){const auto* p=sweep.points[i];return b3Vec3{p[0],p[1],p[2]};};
 for(int i=0;i<hull->edgeCount;++i){
  const auto& edge=edges[i];
  if(edge.twin<i)continue;
  const auto direction=b3Normalize(b3Sub(point(edges[edge.twin].origin),point(edge.origin)));
  store(sweep.edges[sweep.edgeCount++],direction);
 }
 return true;
}
bool TracePropRotation(const PropGeometry& geometry,Vec before,Vec after,void* context,
 bool (*clear)(void*,const JarSweepHull&),Vec localPivot,Vec translation){
 const auto build=[&](Vec pose,float fraction,JarSweepHull& hull){
  if(!BuildPropSweep(geometry,pose,hull))return false;
  const auto offset=b3Sub(b3RotateVector(Q(pose),V(localPivot)),b3MulSV(fraction,V(translation)));
  for(int i=0;i<hull.pointCount;++i){
   hull.points[i][0]-=offset.x;
   hull.points[i][1]-=offset.y;
   hull.points[i][2]-=offset.z;
  }
  return true;
 };
 JarSweepHull first{},last{},arc{};
 if(!build(before,0,first))return false;
 const auto angular=AngularVelocity(before,after,1);
 const float angle=b3Length(V(angular));
 const bool turns=angle>=1e-5f;
 const auto axis=turns?b3MulSV(1/angle,V(angular)):b3Vec3{0,0,0};
 float radiusSquared=0;
 for(int i=0;i<first.pointCount;++i){
  const auto& p=first.points[i];
  const auto radial=b3Cross(axis,{p[0],p[1],p[2]});
  radiusSquared=std::max(radiusSquared,b3Dot(radial,radial));
 }
 const int steps=std::max(1,int(std::ceil(angle/(5*rad))));
 const float halfSag=std::sin(angle/(4*steps));
 const float padding=2*halfSag*halfSag*std::sqrt(radiusSquared);
 for(int i=1;i<=steps;++i){
  const float fraction=float(i)/steps;
  const auto pose=i==steps?after:AdvanceAngles(before,angular,fraction);
  if(!build(pose,fraction,last))return false;
  // Each vertex follows a circular arc. Its chord stays within the convex
  // envelope of the two poses, and the sag stays inside the padding disc.
  arc=first;
  for(int j=0;j<last.pointCount;++j)std::copy_n(last.points[j],3,arc.points[arc.pointCount++]);
  for(int j=0;j<last.faceCount;++j)std::copy_n(last.faces[j],3,arc.faces[arc.faceCount++]);
  for(int j=0;j<last.edgeCount;++j)std::copy_n(last.edges[j],3,arc.edges[arc.edgeCount++]);
  arc.arcRadius=padding;arc.arcAxis[0]=axis.x;arc.arcAxis[1]=axis.y;arc.arcAxis[2]=axis.z;
  if(!clear(context,arc))return false;
  first=last;
 }
 return true;
}
bool NativePropHull::Build(const PropGeometry& geometry,Vec angles){
 axes.clear();
 if(!BuildPropSweep(geometry,angles,sweep))return false;
 const auto vector=[](const float* p){return b3Vec3{p[0],p[1],p[2]};};
 axes.reserve(3+sweep.faceCount+3*sweep.edgeCount);
 const auto addAxis=[&](b3Vec3 candidate){
  const float lengthSquared=b3Dot(candidate,candidate);
  if(lengthSquared<1e-12f)return;
  const auto normal=b3MulSV(1/std::sqrt(lengthSquared),candidate);
  float low=b3Dot(normal,vector(sweep.points[0])),high=low;
  for(int i=1;i<sweep.pointCount;++i){
   const float projected=b3Dot(normal,vector(sweep.points[i]));
   low=std::min(low,projected);high=std::max(high,projected);
  }
  axes.push_back({V(normal),low,high});
 };
 const b3Vec3 boxAxes[]={{1,0,0},{0,1,0},{0,0,1}};
 // Box faces, hull faces and edge cross products are the complete set of
 // separating axes. Face-only expansion leaves false hits at bevels.
 for(auto axis:boxAxes)addAxis(axis);
 for(int i=0;i<sweep.faceCount;++i)addAxis(vector(sweep.faces[i]));
 for(int i=0;i<sweep.edgeCount;++i)for(auto axis:boxAxes)
  addAxis(b3Cross(vector(sweep.edges[i]),axis));
 return true;
}
namespace {
class PropSweepClip {
 const b3Vec3 start,end,direction;
 float enter=0,leave=1,enterSpeed=0;
 PropTrace result,overlap;
 bool startInside=true,endInside=true;
public:
 PropSweepClip(Vec from,Vec to):start(V(from)),end(V(to)),direction(b3Sub(end,start)){
  overlap.depth=std::numeric_limits<float>::infinity();
 }
 bool Clip(b3Vec3 normal,float fixedLow,float fixedHigh,float movingLow,float movingHigh){
  const float low=fixedLow-movingHigh,high=fixedHigh-movingLow;
  const float first=b3Dot(normal,start),last=b3Dot(normal,end);
  if((first<low&&last<low)||(first>high&&last>high))return false;
  const bool startsInside=first>=low&&first<=high,endsInside=last>=low&&last<=high;
  startInside&=startsInside;endInside&=endsInside;
  if(startInside&&endInside){
   const bool nearLow=last-low<high-last;
   const float depth=nearLow?last-low:high-last;
   if(depth<overlap.depth){
    overlap.depth=depth;
    overlap.normal=V(nearLow?b3Neg(normal):normal);
    overlap.planeDistance=nearLow?-fixedLow:fixedHigh;
   }
  }
  const float speed=b3Dot(normal,direction);
  if(speed==0)return true;
  const bool positive=speed>0;
  const float entry=((positive?low:high)-first)/speed,exit=((positive?high:low)-first)/speed;
  if(entry>enter){
   enter=entry;enterSpeed=std::fabs(speed);
   result.normal=V(positive?b3Neg(normal):normal);
   result.planeDistance=positive?-fixedLow:fixedHigh;
  }
  leave=std::min(leave,exit);
  return enter<=leave;
 }
 PropTrace Finish(){
  if(startInside){
   if(endInside)result=overlap;
   result.startSolid=true;result.allSolid=endInside;
   result.fraction=endInside?0.f:1.f;
  }else if(enterSpeed>0){
   // Keep the same separation margin as native BSP traces.
   constexpr float clipEpsilon=.125f;
   result.fraction=std::max(0.f,enter-clipEpsilon/enterSpeed);
   result.depth=(1-enter)*enterSpeed;
  }
  return result;
 }
};

b3Vec3 SweepVector(const float* point){return {point[0],point[1],point[2]};}

void BoxProjection(Vec mins,Vec maxs,b3Vec3 normal,float& low,float& high){
 const b3Vec3 lowPoint={normal.x<0?maxs.x:mins.x,normal.y<0?maxs.y:mins.y,normal.z<0?maxs.z:mins.z};
 const b3Vec3 highPoint={normal.x<0?mins.x:maxs.x,normal.y<0?mins.y:maxs.y,normal.z<0?mins.z:maxs.z};
 low=b3Dot(normal,lowPoint);
 high=b3Dot(normal,highPoint);
}

void SweepProjection(const JarSweepHull& hull,b3Vec3 normal,float& low,float& high){
 low=high=b3Dot(normal,SweepVector(hull.points[0]));
 for(int i=1;i<hull.pointCount;++i){
  const float distance=b3Dot(normal,SweepVector(hull.points[i]));
  low=std::min(low,distance);
  high=std::max(high,distance);
 }
 const float components[]={normal.x,normal.y,normal.z};
 const float padding=JAR_SweepPadding(hull,components);
 low-=padding;
 high+=padding;
}
}

PropTrace NativePropHull::Trace(Vec start,Vec end,Vec mins,Vec maxs) const{
 if(axes.empty())return {};
 PropSweepClip clip(start,end);
 for(const auto& axis:axes){
  const auto normal=V(axis.normal);
  float low,high;
  BoxProjection(mins,maxs,normal,low,high);
  if(!clip.Clip(normal,axis.low,axis.high,low,high))return {};
 }
 return clip.Finish();
}

bool NativePropHull::BoxContactFromBelow(Vec origin,Vec mins,Vec maxs,float& height) const{
 float enter=-std::numeric_limits<float>::infinity();
 float leave=std::numeric_limits<float>::infinity();
 for(const auto& axis:axes){
  const auto normal=V(axis.normal);
  float low,high;
  BoxProjection(mins,maxs,normal,low,high);
  const float position=b3Dot(normal,V(origin));
  const float lower=axis.low-high-position,upper=axis.high-low-position;
  if(normal.z==0){
   if(lower>0||upper<0)return false;
   continue;
  }
  const float first=lower/normal.z,last=upper/normal.z;
  enter=std::max(enter,std::min(first,last));
  leave=std::min(leave,std::max(first,last));
  if(enter>leave)return false;
 }
 if(!std::isfinite(enter))return false;
 height=enter;
 return true;
}

PropTrace NativePropHull::TraceHull(Vec start,Vec end,const JarSweepHull& moving) const{
 if(axes.empty())return {};
 PropSweepClip clip(start,end);
 // Cached stationary axes give a cheap rejection before testing the edge pairs.
 for(const auto& axis:axes){
  const auto normal=V(axis.normal);
  float low,high;
  SweepProjection(moving,normal,low,high);
  if(!clip.Clip(normal,axis.low,axis.high,low,high))return {};
 }
 const auto clipAxis=[&](b3Vec3 candidate){
  const float lengthSquared=b3Dot(candidate,candidate);
  if(lengthSquared<1e-12f)return true;
  const auto normal=b3MulSV(1/std::sqrt(lengthSquared),candidate);
  float fixedLow,fixedHigh,movingLow,movingHigh;
  SweepProjection(sweep,normal,fixedLow,fixedHigh);
  SweepProjection(moving,normal,movingLow,movingHigh);
  return clip.Clip(normal,fixedLow,fixedHigh,movingLow,movingHigh);
 };
 // Face axes and edge cross products cover convex prop pairs. A rotation
 // envelope supplies endpoint axes and keeps its conservative arc padding.
 for(int i=0;i<moving.faceCount;++i)
  if(!clipAxis(SweepVector(moving.faces[i])))return {};
 for(int i=0;i<sweep.edgeCount;++i)for(int j=0;j<moving.edgeCount;++j)
  if(!clipAxis(b3Cross(SweepVector(sweep.edges[i]),SweepVector(moving.edges[j]))))return {};
 return clip.Finish();
}
bool LocalFrame(const Pose&p,Vec anchor,Vec axis,constraint::Frame&out){
 if(!valid(p)||!finite(anchor,65536)||!finite(axis,2))return false;
 const float length=b3Length(V(axis));if(length<.5f||length>1.5f)return false;
 const auto rotation=Q(p.angles);const auto local=b3InvRotateVector(rotation,{anchor.x-p.position.x,anchor.y-p.position.y,anchor.z-p.position.z});
 const auto q=b3NormalizeQuat(b3InvMulQuat(rotation,b3ComputeQuatBetweenUnitVectors({0,0,1},b3MulSV(1/length,V(axis)))));
 constraint::Frame f;f.position[0]=local.x;f.position[1]=local.y;f.position[2]=local.z;
 f.rotation[0]=q.v.x;f.rotation[1]=q.v.y;f.rotation[2]=q.v.z;f.rotation[3]=q.s;
 if(!constraint::Valid(f))return false;out=f;return true;
}
bool FrameTransform(const Pose&p,const constraint::Frame&f,Vec&point,Vec&axis){
 if(!valid(p)||!constraint::Valid(f))return false;
 const auto rotation=Q(p.angles);const auto offset=b3RotateVector(rotation,{f.position[0],f.position[1],f.position[2]});
 const auto q=b3NormalizeQuat({{f.rotation[0],f.rotation[1],f.rotation[2]},f.rotation[3]});
 point={p.position.x+offset.x,p.position.y+offset.y,p.position.z+offset.z};axis=V(b3RotateVector(rotation,b3RotateVector(q,{0,0,1})));
 return finite(point,65536);
}
struct World::Impl {
 b3WorldId world{}; b3MeshData* mesh=nullptr;
 b3BodyId worldAnchorBody{};
 struct MountedShape {b3ShapeId id;MountedHull hull;bool seen;};
 struct Body {
  b3BodyId id{};uint32_t identity=0;unsigned meshKey=0;Vec half,center;
  bool frozen=false,seen=false,rotating=false,mountOnly=false,customMass=false;b3Quat rotationGoal{};Pose target;
  std::vector<MountedShape> mounts;
  std::shared_ptr<const PropGeometry> geometry;
  bool shaped=false;
 };
 std::map<uint32_t,Body> props,proxies;
 struct GripTarget { Vec point,localPoint,facing; };
 std::map<uint32_t,GripTarget> grips;
 unsigned mountProxyCount=0;
 decltype(proxies)::iterator forgetProxy(decltype(proxies)::iterator found){
  if(found->second.mountOnly)--mountProxyCount;
  b3DestroyBody(found->second.id);
  return proxies.erase(found);
 }
 struct ProxyMesh {b3MeshData* data=nullptr;unsigned triangles=0;bool used=false;};
 std::map<unsigned,ProxyMesh> proxyMeshes;
 struct Link {b3JointId id{};uint32_t first=0,second=0;constraint::Joint record;bool seen=false;};
 std::map<uint32_t,Link> joints;
 Stats stats;float accumulator=0,lastGravity=800;std::vector<Contact> contacts;std::vector<Impact> impacts;
 unsigned workerBudget=1;
 void adjustWorkers(){
  if(workerBudget==1)return;
  const int awake=b3World_GetAwakeBodyCount(world);
  // Leave a margin before dropping a tier so contact islands near a
  // threshold don't rebuild worker scratch buffers every step.
  const auto current=stats.workers;
  const unsigned desired=awake>=(current>4?384:512)?8:
                         awake>=(current>2?96:128)?4:
                         awake>=(current>1?48:64)?2:1;
  const unsigned workers=std::min(workerBudget,desired);
  if(workers==current)return;
  b3World_SetWorkerCount(world,workers);
  stats.workers=workers;
 }
 Body make(uint32_t identity,Vec half,Vec center,const Pose&p,b3BodyType type,bool mountOnly=false,
  const std::shared_ptr<const PropGeometry>& geometry={}){
  Body out;out.identity=identity;out.half=half;out.center=center;out.target=p;out.mountOnly=mountOnly;
  out.geometry=geometry;
  auto bd=b3DefaultBodyDef();bd.type=type;bd.position=P(p.position);bd.rotation=Q(p.angles);
  if(type==b3_dynamicBody){bd.linearVelocity=V(p.linear,scale);bd.angularVelocity=V(p.angular);bd.angularDamping=.15f;bd.safetyFactor=.1f;bd.isBullet=true;bd.isAwake=p.awake;}
  out.id=b3CreateBody(world,&bd);auto sd=b3DefaultShapeDef();sd.baseMaterial.friction=.6f;sd.baseMaterial.restitution=.05f;sd.density=120;sd.enableContactEvents=true;sd.enableHitEvents=true;
  sd.filter.categoryBits=mountOnly?mountProxyCategory:ordinaryCategory;
  if(mountOnly)sd.filter.maskBits=mountedCategory;
  if(geometry)if(const auto* hull=geometry->RigidHull())
   out.shaped=b3Shape_IsValid(b3CreateHullShape(out.id,&sd,hull));
  if(!out.shaped){
   const auto box=b3MakeOffsetBoxHull(half.x*scale,half.y*scale,half.z*scale,V(center,scale));
   b3CreateHullShape(out.id,&sd,&box.base);
  }
  return out;
 }
 bool syncMounts(Body& body,const std::vector<MountedHull>* mounts){
  if(body.mounts.empty()&&(!mounts||mounts->empty()))return true;
  for(auto& entry:body.mounts)entry.seen=false;
  bool changed=false;
  if(mounts)for(const auto& hull:*mounts){
   if(!hull.lifetime||!finite(hull.half,1024)||!finite(hull.center,65536)||
      hull.half.x<.25f||hull.half.y<.25f||hull.half.z<.25f)return false;
   const auto found=std::find_if(body.mounts.begin(),body.mounts.end(),
    [&](const MountedShape& entry){return entry.hull.entity==hull.entity;});
   if(found!=body.mounts.end()){
    const auto& old=found->hull;
    if(old.lifetime==hull.lifetime&&same(old.half,hull.half)&&same(old.center,hull.center)){
     found->seen=true;
     continue;
    }
   }
   auto def=b3DefaultShapeDef();
   def.density=0;def.updateBodyMass=false;
   def.filter.categoryBits=mountedCategory;
   def.baseMaterial.restitution=.05f;def.enableContactEvents=true;def.enableHitEvents=true;
   const auto box=b3MakeOffsetBoxHull(hull.half.x*scale,hull.half.y*scale,hull.half.z*scale,V(hull.center,scale));
   const auto shape=b3CreateHullShape(body.id,&def,&box.base);
   if(!b3Shape_IsValid(shape))return false;
   if(found!=body.mounts.end()){
    b3DestroyShape(found->id,false);
    *found={shape,hull,true};
   }else body.mounts.push_back({shape,hull,true});
   changed=true;
  }
  for(auto it=body.mounts.begin();it!=body.mounts.end();){
   if(it->seen){++it;continue;}
   b3DestroyShape(it->id,false);it=body.mounts.erase(it);changed=true;
  }
  if(changed&&!body.frozen)b3Body_SetAwake(body.id,true);
  return true;
 }
};
World::World(unsigned capacity,bool rigid):impl(new Impl),proxyCapacity(capacity),rigidConstraints(rigid){}
World::~World(){Stop();}
bool World::Active()const{return b3World_IsValid(impl->world);}
void World::Stop(){if(Active())b3DestroyWorld(impl->world);if(impl->mesh)b3DestroyMesh(impl->mesh);
 for(auto&mesh:impl->proxyMeshes)b3DestroyMesh(mesh.second.data);impl.reset(new Impl);}
bool World::Start(const std::vector<Vec>& triangles,unsigned maxWorkers){
 Stop();impl->mesh=meshOf(triangles);if(!impl->mesh)return false;
 auto wd=b3DefaultWorldDef();wd.gravity={0,0,-25};wd.workerCount=std::min(8u,std::max(1u,maxWorkers));
 impl->world=b3CreateWorld(&wd);if(!Active()){Stop();return false;}
 impl->workerBudget=b3World_GetWorkerCount(impl->world);
 b3World_SetWorkerCount(impl->world,1);impl->stats.workers=1;
 auto bd=b3DefaultBodyDef();auto body=b3CreateBody(impl->world,&bd);auto sd=b3DefaultShapeDef();sd.baseMaterial.friction=.6f;
 sd.filter.categoryBits=ordinaryCategory;
 b3CreateMeshShape(body,&sd,impl->mesh,{1,1,1});impl->stats.triangles=unsigned(triangles.size()/3);return true;
}
bool World::Prop(uint32_t entity,uint32_t identity,Vec half,Vec center,const Pose&p,bool frozen,
 const std::vector<MountedHull>* mounts,const std::shared_ptr<const PropGeometry>& geometry){
 if(!Active()||!identity||!valid(p)||!finite(half,1024)||!finite(center,1024)||half.x<.25f||half.y<.25f||half.z<.25f)return false;
 auto found=impl->props.find(entity);
 if(found!=impl->props.end() && (found->second.identity!=identity||!same(found->second.half,half)||
    !same(found->second.center,center)||found->second.geometry!=geometry)){Forget(entity);found=impl->props.end();}
 if(found==impl->props.end()){
  if(impl->props.size()>=propLimit)return false;
  auto body=impl->make(identity,half,center,p,frozen?b3_staticBody:b3_dynamicBody,false,geometry);body.frozen=frozen;
  b3Body_SetUserData(body.id,reinterpret_cast<void*>(uintptr_t(entity+1)|uintptr_t(0x80000000u)));
  found=impl->props.emplace(entity,std::move(body)).first;
 }
 auto&b=found->second;
 if(b.frozen!=frozen){if(frozen)impl->grips.erase(entity);b3Body_SetType(b.id,frozen?b3_staticBody:b3_dynamicBody);b.frozen=frozen;
  b3Body_SetTransform(b.id,P(p.position),Q(p.angles));if(!frozen){b3Body_SetLinearVelocity(b.id,V(p.linear,scale));b3Body_SetAngularVelocity(b.id,V(p.angular));b3Body_SetAwake(b.id,true);}}
 if(frozen && (!same(b.target.position,p.position)||!same(b.target.angles,p.angles)))b3Body_SetTransform(b.id,P(p.position),Q(p.angles));
 b.target=p;return impl->syncMounts(b,mounts);
}
bool World::Drag(uint32_t entity,Vec target,bool constrained){
 auto f=impl->props.find(entity);if(!Active()||f==impl->props.end()||f->second.frozen||!finite(target,65536))return false;
 impl->grips.erase(entity);
 const auto body=f->second.id;const auto origin=b3Body_GetPosition(body),goal=P(target);
 b3Vec3 velocity={float(goal.x-origin.x)*12,float(goal.y-origin.y)*12,float(goal.z-origin.z)*12};
 const float speed=b3Length(velocity);if(speed>32)velocity=b3MulSV(32/speed,velocity);
 bool linked=constrained;for(const auto&j:impl->joints)if(j.second.first==entity||j.second.second==entity){linked=true;break;}
 b3Body_SetGravityScale(body,0);b3MotionLocks locks{};locks.angularX=locks.angularY=locks.angularZ=!linked&&!f->second.rotating;b3Body_SetMotionLocks(body,locks);
 if(f->second.rotating)b3Body_SetAngularVelocity(body,RotationServo(b3Body_GetRotation(body),f->second.rotationGoal));
 else if(!linked)b3Body_SetAngularVelocity(body,{0,0,0});
 b3Body_SetLinearVelocity(body,velocity);b3Body_SetAwake(body,true);return true;
}
namespace {
void ApplyGripTarget(b3BodyId body,Vec point,Vec localPoint,Vec facing){
 const auto anchor=b3Body_GetWorldPoint(body,V(localPoint,scale));
 auto velocity=b3MulSV(24,b3SubPos(P(point),anchor));
 const float speed=b3Length(velocity);
 if(speed>32)velocity=b3MulSV(32/speed,velocity);
 auto spin=RotationServo(b3Body_GetRotation(body),Q(facing));
 const auto lever=b3SubPos(anchor,b3Body_GetWorldCenter(body));
 auto angularVelocityAtGrip=b3Cross(spin,lever);
 const float gripSpeed=b3Length(angularVelocityAtGrip);
 if(gripSpeed>32){
  spin=b3MulSV(32/gripSpeed,spin);
  angularVelocityAtGrip=b3MulSV(32/gripSpeed,angularVelocityAtGrip);
 }
 // Box3D sets COM velocity. Cancel the velocity from spin at the clicked
 // point so rotation does not drag the point away from the player's aim.
 b3Body_SetGravityScale(body,0);
 b3Body_SetMotionLocks(body,{});
 b3Body_SetAngularVelocity(body,spin);
 b3Body_SetLinearVelocity(body,b3Sub(velocity,angularVelocityAtGrip));
 b3Body_SetAwake(body,true);
}
}
bool World::DragAt(uint32_t entity,Vec point,Vec localPoint,Vec facing){
 const auto found=impl->props.find(entity);
 if(!Active()||found==impl->props.end()||found->second.frozen||
    !finite(point,65536)||!finite(localPoint,65536)||!finite(facing,100000))return false;
 impl->grips[entity]={point,localPoint,facing};
 ApplyGripTarget(found->second.id,point,localPoint,facing);
 return true;
}
bool World::Rotate(uint32_t entity,float yaw,float pitch){
 auto f=impl->props.find(entity);if(!Active()||f==impl->props.end()||f->second.frozen||!finite({yaw,pitch,0},181))return false;
 auto&b=f->second;if(!b.rotating)b.rotationGoal=b3Body_GetRotation(b.id);
 b.rotationGoal=b3NormalizeQuat(b3MulQuat(b3MulQuat(Axis({0,0,1},yaw*rad),b.rotationGoal),Axis({0,1,0},pitch*rad)));
 b.rotating=true;return true;
}
void World::EndRotate(uint32_t entity){auto f=impl->props.find(entity);if(f!=impl->props.end())f->second.rotating=false;}
void World::Release(uint32_t entity){auto f=impl->props.find(entity);if(!Active()||f==impl->props.end())return;
 impl->grips.erase(entity);
 f->second.rotating=false;
 b3Body_SetGravityScale(f->second.id,1);b3MotionLocks locks{};b3Body_SetMotionLocks(f->second.id,locks);
}
float World::Mass(uint32_t entity)const{auto f=impl->props.find(entity);return Active()&&f!=impl->props.end()?b3Body_GetMass(f->second.id):0;}
// A zero mass override restores the shape's computed mass and inertia.
bool World::Configure(uint32_t entity,float mass,float gravity,bool unrestrictedMass){
 auto f=impl->props.find(entity);if(!Active()||f==impl->props.end()||!std::isfinite(mass)||mass<0||!std::isfinite(gravity)||gravity<0||gravity>10)return false;
 if(unrestrictedMass?(mass>0&&!std::isfinite(1.f/mass)):((mass>0&&mass<1)||mass>100000))return false;
 auto&b=f->second;if(b.frozen)return true;
 if(mass>0){
  auto data=b3Body_GetMassData(b.id);
  if(data.mass<=0)return false;
  if(unrestrictedMass?data.mass!=mass:std::fabs(data.mass-mass)>.001f){
   const float ratio=mass/data.mass;
   data.mass=mass;
   data.inertia.cx=b3MulSV(ratio,data.inertia.cx);data.inertia.cy=b3MulSV(ratio,data.inertia.cy);data.inertia.cz=b3MulSV(ratio,data.inertia.cz);
   if(unrestrictedMass){
    const float determinant=b3Det(data.inertia);
    if(!b3IsValidMatrix3(data.inertia)||!std::isfinite(determinant)||determinant<0||
       (determinant>0&&(!std::isfinite(1.f/determinant)||!b3IsValidMatrix3(b3InvertT(data.inertia)))))return false;
   }
   b3Body_SetMassData(b.id,data);b3Body_SetAwake(b.id,true);
  }
  b.customMass=true;
 }else if(b.customMass){
  b3Body_ApplyMassFromShapes(b.id);b3Body_SetAwake(b.id,true);b.customMass=false;
 }
 if(b3Body_GetGravityScale(b.id)!=gravity){b3Body_SetGravityScale(b.id,gravity);b3Body_SetAwake(b.id,true);}return true;
}
bool World::Correct(uint32_t entity,Vec position,Vec linear){auto f=impl->props.find(entity);
 if(!Active()||f==impl->props.end()||f->second.frozen||!finite(position,65536)||!finite(linear,5000))return false;
 b3Body_SetTransform(f->second.id,P(position),b3Body_GetRotation(f->second.id));
 const auto body=f->second.id;
 const auto center=b3RotateVector(b3Body_GetRotation(body),b3Body_GetLocalCenter(body));
 b3Body_SetLinearVelocity(body,b3Add(V(linear,scale),b3Cross(b3Body_GetAngularVelocity(body),center)));
 b3Body_SetAwake(body,true);return true;
}
bool World::Impulse(uint32_t entity,Vec impulse,Vec point){auto f=impl->props.find(entity);
 if(!Active()||f==impl->props.end()||f->second.frozen||!finite(impulse,1000000)||!finite(point,65536))return false;
 b3Body_ApplyLinearImpulse(f->second.id,V(impulse,scale),P(point),true);return true;
}
float World::ImpulseResponse(uint32_t entity,Vec direction,Vec torque,Vec origin) const{
 auto found=impl->props.find(entity);
 if(!Active()||found==impl->props.end()||found->second.frozen)return 0;
 const auto body=found->second.id;
 const auto linear=V(direction);
 const auto lever=b3SubPos(P(origin),b3Body_GetWorldCenter(body));
 const auto angular=b3Add(V(torque,scale),b3Cross(lever,linear));
 return b3Dot(linear,linear)*b3Body_GetInverseMass(body)+
        b3Dot(angular,b3MulMV(b3Body_GetWorldInverseRotationalInertia(body),angular));
}
// Clamp the resulting velocity before reading it back, including torque from
// a large off-centre impulse. Limits do not replace model mass or inertia.
bool World::LimitedImpulse(uint32_t entity,Vec impulse,Vec point,float speed,float spin){
 return LimitedWrench(entity,impulse,{},point,speed,spin);
}

bool World::LimitedWrench(uint32_t entity,Vec impulse,Vec torque,Vec origin,float speed,float spin){
 const auto body=impl->props.find(entity);
 if(!Active()||body==impl->props.end()||body->second.frozen||!finite(impulse,1000000)||
    !finite(origin,65536)||!finite(torque,2e11f)||
    !std::isfinite(speed)||speed<1||speed>4000||!std::isfinite(spin)||spin<.01f||spin>100)return false;
 const auto id=body->second.id;
 const auto linearImpulse=V(impulse,scale);
 const auto lever=b3SubPos(P(origin),b3Body_GetWorldCenter(id));
 const auto angularImpulse=b3Add(V(torque,scale*scale),b3Cross(lever,linearImpulse));
 b3Body_ApplyLinearImpulseToCenter(id,linearImpulse,true);
 b3Body_ApplyAngularImpulse(id,angularImpulse,true);
 auto angular=b3Body_GetAngularVelocity(id);const float rotation=b3Length(angular);if(rotation>spin)b3Body_SetAngularVelocity(id,b3MulSV(spin/rotation,angular));
 auto linear=b3Body_GetLocalPointVelocity(id,{0,0,0});const float velocity=b3Length(linear),limit=speed*scale;
 if(velocity>limit){linear=b3MulSV(limit/velocity,linear);const auto center=b3RotateVector(b3Body_GetRotation(id),b3Body_GetLocalCenter(id));b3Body_SetLinearVelocity(id,b3Add(linear,b3Cross(b3Body_GetAngularVelocity(id),center)));}
 return true;
}
static void CorrectBodyPose(b3BodyId body,const Pose& pose){
 const auto rotation=Q(pose.angles);
 b3Body_SetTransform(body,P(pose.position),rotation);b3Body_SetAngularVelocity(body,V(pose.angular));
 const auto center=b3RotateVector(rotation,b3Body_GetLocalCenter(body));
 b3Body_SetLinearVelocity(body,b3Add(V(pose.linear,scale),b3Cross(V(pose.angular),center)));b3Body_SetAwake(body,true);
}
bool World::CorrectPose(uint32_t entity,const Pose& pose){
 const auto body=impl->props.find(entity);
 if(!Active()||body==impl->props.end()||body->second.frozen||!valid(pose))return false;
 CorrectBodyPose(body->second.id,pose);return true;
}
bool World::CorrectPoses(const std::vector<PoseUpdate>& updates){
 if(updates.empty())return true;
 if(!Active())return false;
 for(const auto& update:updates){
  const auto body=impl->props.find(update.entity);
  if(body==impl->props.end()||body->second.frozen!=update.frozen||!valid(update.pose))return false;
 }
 for(const auto& update:updates){
  auto& body=impl->props.at(update.entity);
  if(body.frozen){
   b3Body_SetTransform(body.id,P(update.pose.position),Q(update.pose.angles));
   body.target=update.pose;
  }else CorrectBodyPose(body.id,update.pose);
 }
 return true;
}
bool World::JointPoints(const constraint::Joint&j,uint32_t first,uint32_t second,Vec&a,Vec&b)const{
 auto x=impl->props.find(first),y=impl->props.find(second);if(!Active()||x==impl->props.end()||y==impl->props.end())return false;
 const auto p=b3Body_GetWorldPoint(x->second.id,{j.a.position[0]*scale,j.a.position[1]*scale,j.a.position[2]*scale});
 const auto q=b3Body_GetWorldPoint(y->second.id,{j.b.position[0]*scale,j.b.position[1]*scale,j.b.position[2]*scale});
 a={float(p.x/scale),float(p.y/scale),float(p.z/scale)};b={float(q.x/scale),float(q.y/scale),float(q.z/scale)};return true;
}
void World::ForgetJoint(uint32_t id){auto f=impl->joints.find(id);if(f==impl->joints.end())return;
 if(Active()&&b3Joint_IsValid(f->second.id))b3DestroyJoint(f->second.id,true);impl->joints.erase(f);
}
void World::Forget(uint32_t entity){auto f=impl->props.find(entity);if(f==impl->props.end())return;
 impl->grips.erase(entity);
 for(auto j=impl->joints.begin();j!=impl->joints.end();)if(j->second.first==entity||j->second.second==entity){const auto id=j->first;++j;ForgetJoint(id);}else ++j;
 if(Active())b3DestroyBody(f->second.id);impl->props.erase(f);
}
bool World::JointFrames(uint32_t first,uint32_t second,Vec anchor,Vec axis,constraint::Frame&a,constraint::Frame&b)const{
 auto x=impl->props.find(first),y=impl->props.find(second);
 if(!Active()||x==impl->props.end()||y==impl->props.end()||!finite(anchor,65536)||!finite(axis,2))return false;
 const float length=b3Length(V(axis));if(length<.5f||length>1.5f)return false;
 // Revolute joints rotate about the common frame's Z axis.
 const auto rotation=b3ComputeQuatBetweenUnitVectors({0,0,1},b3MulSV(1/length,V(axis)));
 auto frame=[&](b3BodyId body){constraint::Frame f;const auto p=b3Body_GetLocalPoint(body,P(anchor));
  const auto q=b3NormalizeQuat(b3InvMulQuat(b3Body_GetRotation(body),rotation));
  f.position[0]=p.x/scale;f.position[1]=p.y/scale;f.position[2]=p.z/scale;
  f.rotation[0]=q.v.x;f.rotation[1]=q.v.y;f.rotation[2]=q.v.z;f.rotation[3]=q.s;return f;
 };
 const auto fa=frame(x->second.id),fb=frame(y->second.id);if(!constraint::Valid(fa)||!constraint::Valid(fb))return false;a=fa;b=fb;return true;
}
void World::BeginJoints(){for(auto&j:impl->joints)j.second.seen=false;}
bool World::Joint(const constraint::Joint&record,uint32_t first,uint32_t second){
 if(!Active()||!constraint::Valid(record)||record.path.count||(first==second&&record.kind!=constraint::Thruster)||
    ((first==worldAnchor||second==worldAnchor)&&record.kind==constraint::Thruster))return false;
 const auto body=[&](uint32_t entity,uint32_t identity){
  if(entity==worldAnchor){
   if(!constraint::Native(identity))return b3BodyId{};
   if(!b3Body_IsValid(impl->worldAnchorBody)){
    const auto def=b3DefaultBodyDef();
    impl->worldAnchorBody=b3CreateBody(impl->world,&def);
   }
   return impl->worldAnchorBody;
  }
  const auto found=impl->props.find(entity);
  return found!=impl->props.end()&&found->second.identity==identity?found->second.id:b3BodyId{};
 };
 const auto a=body(first,record.first),b=body(second,record.second);
 if(!b3Body_IsValid(a)||!b3Body_IsValid(b))return false;
 auto sameFrame=[](const constraint::Frame&x,const constraint::Frame&y){for(int i=0;i<3;++i)if(x.position[i]!=y.position[i])return false;for(int i=0;i<4;++i)if(x.rotation[i]!=y.rotation[i])return false;return true;};
 auto found=impl->joints.find(record.id);
 if(found!=impl->joints.end()){auto&j=found->second;
  if(j.first==first&&j.second==second&&j.record.kind==record.kind&&j.record.first==record.first&&j.record.second==record.second&&sameFrame(j.record.a,record.a)&&sameFrame(j.record.b,record.b)&&j.record.length==record.length&&j.record.force==record.force&&j.record.enabled==record.enabled&&(record.kind==constraint::Thruster||b3Joint_IsValid(j.id))){j.seen=true;return true;}
  ForgetJoint(record.id);
 }
 if(impl->joints.size()>=constraint::limit)return false;
 auto frame=[](const constraint::Frame&f){
  // Save validation permits float roundoff; the solver requires a tighter unit quaternion.
  const auto q=b3NormalizeQuat({{f.rotation[0],f.rotation[1],f.rotation[2]},f.rotation[3]});
  return b3Transform{{f.position[0]*scale,f.position[1]*scale,f.position[2]*scale},q};};
 auto base=[&](b3JointDef&def){
  def.bodyIdA=a;def.bodyIdB=b;def.localFrameA=frame(record.a);def.localFrameB=frame(record.b);def.collideConnected=false;
  if(rigidConstraints)def.constraintHertz=0;
 };
 Impl::Link link;link.first=first;link.second=second;link.record=record;link.seen=true;
 if(record.kind==constraint::Weld){auto def=b3DefaultWeldJointDef();base(def.base);link.id=b3CreateWeldJoint(impl->world,&def);}
 else if(record.kind==constraint::Hinge){auto def=b3DefaultRevoluteJointDef();base(def.base);link.id=b3CreateRevoluteJoint(impl->world,&def);}
 else if(record.kind==constraint::Rope){auto def=b3DefaultDistanceJointDef();base(def.base);def.base.collideConnected=true;def.length=std::max(.01f,record.length*scale);
  // Box3D uses its unilateral limits in this mode. A zero spring frequency
  // and force leave only the upper limit on multiplayer cables.
  def.enableSpring=true;def.hertz=0;def.lowerSpringForce=def.upperSpringForce=0;def.enableLimit=true;
  def.minLength=rigidConstraints?0:.005f;def.maxLength=rigidConstraints?record.length*scale:def.length;
  link.id=b3CreateDistanceJoint(impl->world,&def);}
 else {impl->joints.emplace(record.id,link);return true;}
 if(!b3Joint_IsValid(link.id))return false;impl->joints.emplace(record.id,link);return true;
}
void World::EndJoints(){for(auto j=impl->joints.begin();j!=impl->joints.end();)if(!j->second.seen){const auto id=j->first;++j;ForgetJoint(id);}else ++j;}

bool World::Read(uint32_t entity,Pose&p)const{auto f=impl->props.find(entity);if(!Active()||f==impl->props.end())return false;
 const auto b=f->second.id;const auto pos=b3Body_GetPosition(b);p.position={float(pos.x/scale),float(pos.y/scale),float(pos.z/scale)};
 p.angles=f->second.frozen?f->second.target.angles:Angles(b3Body_GetRotation(b));p.linear=V(b3Body_GetLocalPointVelocity(b,{0,0,0}),1/scale);p.angular=V(b3Body_GetAngularVelocity(b));p.awake=b3Body_IsAwake(b);return valid(p);
}
bool World::Bounds(uint32_t entity,Vec& minimum,Vec& maximum)const{
 auto f=impl->props.find(entity);if(!Active()||f==impl->props.end())return false;const auto&b=f->second;const auto q=b3Body_GetRotation(b.id);
 if(b.shaped){
  const Vec axes[]={V(b3InvRotateVector(q,{1,0,0})),V(b3InvRotateVector(q,{0,1,0})),V(b3InvRotateVector(q,{0,0,1}))};
  b.geometry->Bounds(axes,minimum,maximum);return true;
 }
 const auto center=b3RotateVector(q,V(b.center));const auto x=b3RotateVector(q,{b.half.x,0,0}),y=b3RotateVector(q,{0,b.half.y,0}),z=b3RotateVector(q,{0,0,b.half.z});
 b3Vec3 h={std::fabs(x.x)+std::fabs(y.x)+std::fabs(z.x),std::fabs(x.y)+std::fabs(y.y)+std::fabs(z.y),std::fabs(x.z)+std::fabs(y.z)+std::fabs(z.z)};
 minimum=V(b3Sub(center,h));maximum=V(b3Add(center,h));return true;
}
void World::BeginProxies(){for(auto&b:impl->proxies)b.second.seen=false;}
void World::KeepProxy(uint32_t entity,uint32_t identity,unsigned model,bool mountOnly){
 const auto found=impl->proxies.find(entity);
 if(found!=impl->proxies.end()&&found->second.identity==identity&&found->second.meshKey==model&&found->second.mountOnly==mountOnly)
  found->second.seen=true;
}
bool World::Proxy(uint32_t entity,uint32_t identity,Vec half,Vec center,const Pose&p,bool mountOnly){
 if(!Active()||!valid(p)||!finite(half,8192)||!finite(center,65536)||half.x<.25f||half.y<.25f||half.z<.25f)return false;
 auto found=impl->proxies.find(entity);
 if(found!=impl->proxies.end() && (found->second.identity!=identity || found->second.meshKey ||
    found->second.mountOnly!=mountOnly||!same(found->second.half,half)||!same(found->second.center,center))){
  impl->forgetProxy(found);found=impl->proxies.end();
 }
 if(found==impl->proxies.end()){
  if(mountOnly?impl->mountProxyCount>=proxyLimit:impl->proxies.size()-impl->mountProxyCount>=proxyCapacity)return false;
  found=impl->proxies.emplace(entity,impl->make(identity,half,center,p,b3_kinematicBody,mountOnly)).first;
  if(mountOnly)++impl->mountProxyCount;
 }
 auto&b=found->second;b.target=p;b.seen=true;b3Body_SetUserData(b.id,reinterpret_cast<void*>(uintptr_t(entity+1)));return true;
}
bool World::HasProxyMesh(unsigned model)const{return impl->proxyMeshes.count(model)!=0;}
bool World::AddProxyMesh(unsigned model,const std::vector<Vec>&triangles){
 if(!Active()||!model||HasProxyMesh(model)||impl->proxyMeshes.size()>=proxyLimit||
    triangles.size()>size_t(triangleLimit-impl->stats.proxyTriangles)*3)return false;
 auto*mesh=meshOf(triangles);if(!mesh)return false;
 const unsigned count=unsigned(triangles.size()/3);impl->proxyMeshes.emplace(model,Impl::ProxyMesh{mesh,count,false});impl->stats.proxyTriangles+=count;return true;
}
bool World::MeshProxy(uint32_t entity,uint32_t identity,unsigned model,const Pose&p){
 if(!Active()||!valid(p)||!model)return false;auto mesh=impl->proxyMeshes.find(model);if(mesh==impl->proxyMeshes.end())return false;
 auto found=impl->proxies.find(entity);
 if(found!=impl->proxies.end()&&(found->second.identity!=identity||found->second.meshKey!=model)){
  impl->forgetProxy(found);found=impl->proxies.end();
 }
 if(found==impl->proxies.end()){
  if(impl->proxies.size()-impl->mountProxyCount>=proxyCapacity)return false;
  Impl::Body b;b.identity=identity;b.meshKey=model;
  auto bd=b3DefaultBodyDef();bd.type=b3_kinematicBody;bd.position=P(p.position);bd.rotation=Q(p.angles);
  b.id=b3CreateBody(impl->world,&bd);if(!b3Body_IsValid(b.id))return false;
  auto sd=b3DefaultShapeDef();sd.baseMaterial.friction=.6f;sd.baseMaterial.restitution=.05f;sd.enableContactEvents=true;sd.enableHitEvents=true;
  sd.filter.categoryBits=ordinaryCategory;
  if(!b3Shape_IsValid(b3CreateMeshShape(b.id,&sd,mesh->second.data,{1,1,1}))){b3DestroyBody(b.id);return false;}
  found=impl->proxies.emplace(entity,b).first;
 }
 auto&b=found->second;b.target=p;b.seen=true;b3Body_SetUserData(b.id,reinterpret_cast<void*>(uintptr_t(entity+1)));return true;
}
void World::EndProxies(){
 for(auto i=impl->proxies.begin();i!=impl->proxies.end();)if(!i->second.seen)i=impl->forgetProxy(i);else ++i;
 for(auto&mesh:impl->proxyMeshes)mesh.second.used=false;
 for(const auto&b:impl->proxies)if(b.second.meshKey)impl->proxyMeshes.at(b.second.meshKey).used=true;
 for(auto i=impl->proxyMeshes.begin();i!=impl->proxyMeshes.end();)if(!i->second.used){
  impl->stats.proxyTriangles-=i->second.triangles;b3DestroyMesh(i->second.data);i=impl->proxyMeshes.erase(i);
 }else ++i;
}
void World::Step(float elapsed,float gravity){
 if(!Active()||!std::isfinite(elapsed)||!std::isfinite(gravity))return;
 impl->contacts.clear();impl->impacts.clear();impl->stats.stepMs=0;
 gravity=std::max(0.f,std::min(2000.f,gravity));
 if(gravity!=impl->lastGravity){for(auto&entry:impl->props)if(!entry.second.frozen)b3Body_SetAwake(entry.second.id,true);impl->lastGravity=gravity;}
 b3World_SetGravity(impl->world,{0,0,-gravity*scale});
 impl->accumulator+=std::max(0.f,std::min(.05f,elapsed));
 const int count=std::min(6,int((impl->accumulator+.000001f)/step));
 if(count)impl->adjustWorkers();
 for(int i=0;i<count;++i){
  // The lever arm changes during a turn. Recompute the correction at
  // 120 Hz so the selected point stays fixed during multi-step frames.
  for(const auto& entry:impl->grips){
   const auto found=impl->props.find(entry.first);
   if(found==impl->props.end()||found->second.frozen)continue;
   const auto& grip=entry.second;
   ApplyGripTarget(found->second.id,grip.point,grip.localPoint,grip.facing);
  }
  for(auto&pair:impl->proxies){auto&b=pair.second;auto current=b3Body_GetPosition(b.id);auto target=P(b.target.position);float distance=float(std::fabs(target.x-current.x)+std::fabs(target.y-current.y)+std::fabs(target.z-current.z));
   if(distance>8){b3Body_SetTransform(b.id,target,Q(b.target.angles));b3Body_SetLinearVelocity(b.id,{0,0,0});b3Body_SetAngularVelocity(b.id,{0,0,0});}
   else b3Body_SetTargetTransform(b.id,{target,Q(b.target.angles)},step*(count-i),true);
  }
  for(const auto&entry:impl->joints){const auto&j=entry.second;if(j.record.kind!=constraint::Thruster||!j.record.enabled)continue;
   auto body=impl->props.find(j.first);if(body==impl->props.end()||body->second.frozen)continue;
   const auto&f=j.record.a;const b3Quat q=b3NormalizeQuat({{f.rotation[0],f.rotation[1],f.rotation[2]},f.rotation[3]});
   const auto direction=b3Body_GetWorldVector(body->second.id,b3RotateVector(q,{0,0,1}));
   const auto point=b3Body_GetWorldPoint(body->second.id,{f.position[0]*scale,f.position[1]*scale,f.position[2]*scale});
   b3Body_ApplyForce(body->second.id,b3MulSV(JAR_THRUSTER_SIGN*j.record.force,direction),point,true);
  }
  b3World_Step(impl->world,step,4);const auto profile=b3World_GetProfile(impl->world);impl->stats.stepMs+=profile.step;impl->stats.maxStepMs=std::max(impl->stats.maxStepMs,profile.step);++impl->stats.steps;impl->accumulator-=step;
  const auto events=b3World_GetContactEvents(impl->world);
  for(int n=0;n<events.hitCount && impl->impacts.size()<128;++n){const auto&e=events.hitEvents[n];
   for(const auto shape:{e.shapeIdA,e.shapeIdB}){const auto id=uintptr_t(b3Body_GetUserData(b3Shape_GetBody(shape)));
    if(id&0x80000000u)impl->impacts.push_back({uint32_t((id&0x7fffffffu)-1),e.approachSpeed/scale,{float(e.point.x/scale),float(e.point.y/scale),float(e.point.z/scale)}});}}

  for(int n=0;n<events.beginCount;++n){const auto&e=events.beginEvents[n];
   auto a=uintptr_t(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdA))),b=uintptr_t(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdB)));
   if(!(a&0x80000000u))std::swap(a,b);
   if((a&0x80000000u) && b && !(b&0x80000000u) && impl->contacts.size()<1024)impl->contacts.push_back({uint32_t((a&0x7fffffffu)-1),uint32_t(b-1)});
  }
 }
 impl->accumulator=std::max(0.f,impl->accumulator);
}
const std::vector<Contact>& World::Contacts()const{return impl->contacts;}
const std::vector<Impact>& World::Impacts()const{return impl->impacts;}
Stats World::Statistics()const{auto s=impl->stats;s.props=unsigned(impl->props.size());s.proxies=unsigned(impl->proxies.size());s.proxyMeshes=unsigned(impl->proxyMeshes.size());s.joints=unsigned(impl->joints.size());if(Active())for(const auto&b:impl->props)if(!b.second.frozen&&b3Body_IsAwake(b.second.id))++s.awake;return s;}
#include "jar_constraint_solver.inc"
} }
