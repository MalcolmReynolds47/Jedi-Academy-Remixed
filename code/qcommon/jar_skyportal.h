// Portable sky viewpoint appended to the native sky configstring. GPL-2.0-only.
#ifndef JAR_SKYPORTAL_H
#define JAR_SKYPORTAL_H
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
namespace jar {namespace skyPortal {
struct Settings {bool enabled=false,entities=true;};
struct View {float origin[3]{},angles[3]{};int entity=-1;bool entities=true;};
inline bool Class(const char*n){return n&&!std::strcmp(n,"misc_skyportal");}
inline bool Valid(const View&v,int limit){
 if(v.entity<0||v.entity>=limit)return false;
 for(int i=0;i<3;++i)if(!std::isfinite(v.origin[i])||std::fabs(v.origin[i])>65535||!std::isfinite(v.angles[i])||std::fabs(v.angles[i])>100000)return false;
 return true;
}
inline std::string Encode(const View&v){
 char text[256];std::snprintf(text,sizeof(text),"%.3f %.3f %.3f 0 0 0 0 0 300 jar_sky1 %d %.3f %.3f %.3f %d",v.origin[0],v.origin[1],v.origin[2],v.entity,v.angles[0],v.angles[1],v.angles[2],v.entities?1:0);return text;
}
inline bool Decode(const char*text,View&out,int limit){
 if(!text||std::strlen(text)>=256)return false;
 View v;int fog,nearPlane,farPlane,entities,end=0;float rgb[3];
 if(std::sscanf(text,"%f %f %f %d %f %f %f %d %d jar_sky1 %d %f %f %f %d%n",&v.origin[0],&v.origin[1],&v.origin[2],&fog,&rgb[0],&rgb[1],&rgb[2],&nearPlane,&farPlane,&v.entity,&v.angles[0],&v.angles[1],&v.angles[2],&entities,&end)!=14||!end||text[end]||fog||rgb[0]||rgb[1]||rgb[2]||nearPlane||farPlane!=300||(entities!=0&&entities!=1)||!Valid(v,limit))return false;
 v.entities=entities!=0;out=v;return true;
}
inline bool BaselineValid(const char*text,int limit){
 if(!text)return false;if(!*text)return true;View portable;if(Decode(text,portable,limit))return true;
 float origin[3],rgb[3];int fog,nearPlane,farPlane;char extra;
 if(std::sscanf(text,"%f %f %f %d %f %f %f %d %d %c",&origin[0],&origin[1],&origin[2],&fog,&rgb[0],&rgb[1],&rgb[2],&nearPlane,&farPlane,&extra)!=9||fog<0||fog>3)return false;
 for(int i=0;i<3;++i)if(!std::isfinite(origin[i])||std::fabs(origin[i])>65535||!std::isfinite(rgb[i])||rgb[i]<0||rgb[i]>1)return false;
 return true;
}
// Compose the controller's local rotation with the player's current view.
inline void Rotate(const float offset[3][3],const float view[3][3],float out[3][3]){
 float result[3][3]{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k)result[i][j]+=view[i][k]*offset[k][j];
 std::memcpy(out,result,sizeof(result));
}
}}
#endif
