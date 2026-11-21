#include "../../shared/qcommon/jar_rope_mesh.h"
#include <cassert>
#include <cmath>
#include <limits>
int main(){
 jarRopeVertex_t mesh[JAR_ROPE_VERTICES];float a[3]={0,0,0},b[3]={100,0,0};
 assert(JAR_RopeMesh(a,b,100,mesh)==JAR_ROPE_VERTICES);
 for(const auto&v:mesh){for(float c:v.xyz)assert(std::isfinite(c));assert(v.color[3]==255);assert(fabsf(v.xyz[2])<2);}
 assert(JAR_RopeMesh(a,b,140,mesh)==JAR_ROPE_VERTICES);assert(mesh[JAR_ROPE_VERTICES/2].xyz[2]<-40);
 // The rendered curve follows the requested length and clips against terrain.
 float delta[3]={100,0,0};
 float low=0,high=140;for(int i=0;i<12;++i){float mid=(low+high)/2;if(JAR_RopeArcLength(delta,mid)>140)high=mid;else low=mid;}
 assert(fabsf(JAR_RopeArcLength(delta,(low+high)/2)-140)<.1f);
 a[2]=b[2]=8;int traces=0;
 auto floor=[](const float*chord,float*point,void*data){++*static_cast<int*>(data);point[2]=fmaxf(1.8f,point[2]);};
 assert(JAR_RopeMeshTrace(a,b,140,mesh,floor,&traces)==JAR_ROPE_VERTICES&&traces==23);
 for(const auto&v:mesh)assert(v.xyz[2]>=0);a[2]=b[2]=0;
 b[0]=0;b[2]=100;assert(JAR_RopeMesh(a,b,100,mesh)==JAR_ROPE_VERTICES);
 for(const auto&v:mesh)for(float c:v.xyz)assert(std::isfinite(c));
 assert(JAR_RopeMesh(a,a,0,mesh)==0);assert(JAR_RopeMesh(a,a,10,mesh)==JAR_ROPE_VERTICES);
 assert(JAR_RopeMesh(a,b,std::numeric_limits<float>::infinity(),mesh)==0);
 b[1]=std::numeric_limits<float>::quiet_NaN();assert(JAR_RopeMesh(a,b,100,mesh)==0);
}
