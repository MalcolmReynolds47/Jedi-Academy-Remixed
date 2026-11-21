// Explicit portable portal geometry/camera contract. GPL-2.0-only.
#ifndef JAR_PORTAL_H
#define JAR_PORTAL_H
#include <cmath>
#include <cstring>
#include <array>
#include "../../shared/qcommon/jar_portal_limits.h"
namespace jar { namespace portal {
constexpr unsigned frameLimit=JAR_PORTAL_SURFACE_LIMIT;
constexpr std::array<const char*, JAR_PORTAL_CAMERA_FEEDS> monitorMaterials = {
 JAR_PORTAL_CAMERA_SHADER, "gfx/jar/portal_monitor1", "gfx/jar/portal_monitor2", "gfx/jar/portal_monitor3"
};
inline int MonitorImageSlot(const char* name){
 constexpr char prefix[]="$jarCamera";
 if(Q_stricmpn(name,prefix,sizeof(prefix)-1))return -1;
 const char* suffix=name+sizeof(prefix)-1;
 if(!*suffix)return 0;
 return suffix[0]>='1'&&suffix[0]<'0'+JAR_PORTAL_CAMERA_FEEDS&&!suffix[1]?suffix[0]-'0':-1;
}
struct Settings {float width=JAR_PORTAL_WIDTH,height=JAR_PORTAL_HEIGHT;bool mirror=false,enabled=true;};
inline bool Same(const Settings&a,const Settings&b){
 return a.width==b.width&&a.height==b.height&&a.mirror==b.mirror&&a.enabled==b.enabled;
}
using View=jarPortablePortalView_t;
inline bool SurfaceClass(const char*n){return n&&!std::strcmp(n,"misc_portal_surface");}
inline bool CameraClass(const char*n){return n&&!std::strcmp(n,"misc_portal_camera");}
inline bool Valid(const Settings&s){return std::isfinite(s.width)&&s.width>=JAR_PORTAL_MIN_SIZE&&s.width<=JAR_PORTAL_MAX_SIZE&&std::isfinite(s.height)&&s.height>=JAR_PORTAL_MIN_SIZE&&s.height<=JAR_PORTAL_MAX_SIZE;}
inline bool Basis(const float a[3][3]){
 for(int i=0;i<3;++i)for(int j=i;j<3;++j){float d=0;for(int k=0;k<3;++k){if(!std::isfinite(a[i][k])||!std::isfinite(a[j][k]))return false;d+=a[i][k]*a[j][k];}if(std::fabs(d-(i==j?1.f:0.f))>.002f)return false;}
 const float cross[3]={a[0][1]*a[1][2]-a[0][2]*a[1][1],a[0][2]*a[1][0]-a[0][0]*a[1][2],a[0][0]*a[1][1]-a[0][1]*a[1][0]};
 return cross[0]*a[2][0]+cross[1]*a[2][1]+cross[2]*a[2][2]>.998f;
}
inline bool Valid(const View&v){
 Settings s;s.width=v.width;s.height=v.height;if(!Valid(s)||v.mirror<0||v.mirror>1||!Basis(v.axis)||!Basis(v.cameraAxis))return false;
 for(int i=0;i<3;++i)if(!std::isfinite(v.origin[i])||!std::isfinite(v.cameraOrigin[i])||std::fabs(v.origin[i])>65535||std::fabs(v.cameraOrigin[i])>65535)return false;return true;
}
// Clockwise as seen from behind, matching PlaneFromPoints and native poly normals.
inline void Corners(const View&v,float p[4][3]){
 static const int horizontal[4]={-1,-1,1,1},vertical[4]={-1,1,1,-1};
 for(int i=0;i<4;++i)for(int a=0;a<3;++a)p[i][a]=v.origin[a]+horizontal[i]*v.width*.5f*v.axis[1][a]+vertical[i]*v.height*.5f*v.axis[2][a];
}
inline bool PanelFacesEye(const View& view, const float* eye){
 float front=0;
 for(int a=0;a<3;++a)front+=(eye[a]-view.origin[a])*view.axis[0][a];
 return front>0;
}
// Check the panel in world space without staging triangles in backend tess.
template<class Transform> inline bool PanelVisible(const View& view, const float* eye, Transform transform){
 if(!PanelFacesEye(view,eye))return false;

 float corners[4][3];Corners(view,corners);
 unsigned outsideAll=0x3f;
 for(const auto& point:corners){
  float clip[4];transform(point,clip);
  unsigned outside=0;
  for(int a=0;a<3;++a){
   if(clip[a]>clip[3])outside|=1u<<(2*a);
   if(clip[a]<-clip[3])outside|=2u<<(2*a);
  }
  outsideAll &= outside;
 }
 return outsideAll==0;
}
// Clip the convex quad before dividing by w so near-plane crossings and
// screens extending outside the viewport get their actual visible area.
template<class Transform> inline float PanelCoverage(const View& view, const float* eye, Transform transform){
 if(!PanelFacesEye(view,eye))return 0;
 using ClipPoint=std::array<float,4>;
 // A convex polygon gains at most one vertex per clip plane.
 std::array<ClipPoint,10> polygon,clipped;
 float corners[4][3];Corners(view,corners);
 int count=4;
 for(int i=0;i<count;++i)transform(corners[i],polygon[i].data());
 for(int plane=0;plane<6&&count>=3;++plane){
  const int axis=plane/2;
  const float sign=plane%2?-1.f:1.f;
  const auto distance=[=](const ClipPoint& point){return point[3]+sign*point[axis];};
  int used=0;
  for(int i=0;i<count;++i){
   const auto& a=polygon[i];
   const auto& b=polygon[(i+1)%count];
   const float da=distance(a),db=distance(b);
   if(da>=0){
    if(used==int(clipped.size()))return 0;
    clipped[used++]=a;
   }
   if((da<0&&db>0)||(da>0&&db<0)){
    if(used==int(clipped.size()))return 0;
    auto& intersection=clipped[used++];
    const float fraction=da/(da-db);
    for(int component=0;component<4;++component)
     intersection[component]=a[component]+fraction*(b[component]-a[component]);
   }
  }
  count=used;
  for(int i=0;i<count;++i)polygon[i]=clipped[i];
 }
 if(count<3)return 0;
 float twiceArea=0;
 for(int i=0;i<count;++i){
  const auto& a=polygon[i];
  const auto& b=polygon[(i+1)%count];
  if(!(a[3]>0&&b[3]>0))return 0;
  twiceArea+=(a[0]/a[3])*(b[1]/b[3])-(a[1]/a[3])*(b[0]/b[3]);
 }
 return std::isfinite(twiceArea)?std::fmin(1.f,std::fabs(twiceArea)*.125f):0;
}
// The feed uses the camera's pose. Match the field of view to the screen
// proportions so the square render target doesn't stretch world objects.
template<class Orientation> inline void MonitorView(const View& view, Orientation& pose,
 float* pvs, float& fovX, float& fovY){
 for(int i=0;i<3;++i){
  pose.origin[i]=pvs[i]=view.cameraOrigin[i];
  for(int j=0;j<3;++j)pose.axis[i][j]=view.cameraAxis[i][j];
 }
 constexpr float halfTangent=.767326988f; // tan(75 degrees / 2)
 constexpr float degreesFromHalfRadians=114.591559f;
 const float longest=view.width>view.height?view.width:view.height;
 fovX=std::atan(halfTangent*(view.width/longest))*degreesFromHalfRadians;
 fovY=std::atan(halfTangent*(view.height/longest))*degreesFromHalfRadians;
}
template<class Orientation> inline void Orientations(const View&v,Orientation&surface,Orientation&camera,float*pvs){
 for(int i=0;i<3;++i){surface.origin[i]=v.origin[i];camera.origin[i]=v.mirror?v.origin[i]:v.cameraOrigin[i];pvs[i]=camera.origin[i];
  for(int j=0;j<3;++j){surface.axis[i][j]=v.axis[i][j];camera.axis[i][j]=v.mirror?(i==0?-v.axis[i][j]:v.axis[i][j]):(i<2?-v.cameraAxis[i][j]:v.cameraAxis[i][j]);}}
}
}}
#endif
