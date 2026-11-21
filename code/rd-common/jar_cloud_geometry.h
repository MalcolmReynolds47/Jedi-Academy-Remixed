// Stock cloud strips shared by Classic and Rend2. GPL-2.0-only.
// Generate world vertices without mutating the submitted render entity.
#ifndef JAR_CLOUD_GEOMETRY_H
#define JAR_CLOUD_GEOMETRY_H
#include <cmath>
namespace jar { namespace cloud {
struct Vertex {float point[3],uv[2],shade;};
template<class Emit> bool Geometry(float radius,float inner,float curve,bool tube,const float*origin,const float axis[3][3],Emit emit){
 if(!std::isfinite(radius)||radius<=0||radius>65535||!std::isfinite(inner)||inner<0||inner>=radius||!std::isfinite(curve)||std::fabs(curve)>100)return false;
 for(int i=0;i<3;++i){if(!std::isfinite(origin[i]))return false;for(int a=0;a<3;++a)if(!std::isfinite(axis[i][a]))return false;}
 const float diskR[]={0,.4f,.7f,1},diskA[]={1,1,.4f,0},diskZ[]={0,0,.008f,.02f};
 const float tubeR[]={0,.05f,.1f,.5f,.7f,1},tubeA[]={0,.45f,1,1,.45f,0},tubeZ[]={0,.004f,.006f,.01f,.006f,0};
 const float*r=tube?tubeR:diskR,*a=tube?tubeA:diskA,*z=tube?tubeZ:diskZ;
 const int rings=tube?6:4;const float hole=tube?inner:0,curvature=tube?-curve:curve;
 for(int ring=0;ring<rings-1;++ring)for(int segment=0;segment<12;++segment){
  Vertex vertices[4]{};
  for(int v=0;v<4;++v){const int band=ring+(v&1),edge=segment+(v>=2?1:0);
   const float theta=(edge%12)*(6.2831853071795864769f/12),distance=r[band]*(radius-hole)+hole;
   const float local[3]={distance*std::cos(theta),distance*std::sin(theta),z[band]*radius*curvature};
   for(int dim=0;dim<3;++dim){vertices[v].point[dim]=origin[dim];for(int ax=0;ax<3;++ax)vertices[v].point[dim]+=axis[ax][dim]*local[ax];}
   // Keep stock world-offset scrolling for unrotated layers; rotate with the layer.
   vertices[v].uv[0]=(origin[0]+local[0])*.1f;vertices[v].uv[1]=(origin[1]+local[1])*.1f;vertices[v].shade=a[band];
  }
  emit(vertices);
 }
 return true;
}
} }
#endif
