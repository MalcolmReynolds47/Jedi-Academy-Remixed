// Oriented portable weather volumes and bounded GPU transport. GPL-2.0-only.
#ifndef JAR_WEATHER_ZONES_H
#define JAR_WEATHER_ZONES_H
#include "jar_weather.h"
#include <iomanip>
namespace jar { namespace weather {
constexpr int zoneLimit=32,zoneVectors=5;
// center/type (0 wind, 1 shelter, 2 exposed), three axis/half-extent vectors,
// then wind velocity/reserved. Exactly five vec4s in the renderer uniform array.
struct Zone{float v[20]{};};
static_assert(sizeof(Zone)==zoneVectors*4*sizeof(float),"GPU weather layout");
struct Zones{int count=0;Zone values[zoneLimit];};
inline bool Valid(const Zone&z){
 for(float x:z.v)if(!std::isfinite(x))return false;
 if(z.v[3]!=0&&z.v[3]!=1&&z.v[3]!=2)return false;
 for(int a=0;a<3;++a){if(std::fabs(z.v[a])>1000000||std::fabs(z.v[16+a])>5000)return false;const float*axis=z.v+4+4*a;if(axis[3]<4||axis[3]>4096)return false;float length=0;for(int b=0;b<3;++b)length+=axis[b]*axis[b];if(std::fabs(length-1)>.002f)return false;for(int b=0;b<a;++b){float dot=0;for(int c=0;c<3;++c)dot+=axis[c]*z.v[4+4*b+c];if(std::fabs(dot)>.002f)return false;}}
 return z.v[19]==0;
}
inline bool Contains(const Zone&z,const float*p){if(!p)return false;for(int a=0;a<3;++a){const float*axis=z.v+4+4*a;float d=0;for(int b=0;b<3;++b)d+=(p[b]-z.v[b])*axis[b];if(!std::isfinite(d)||std::fabs(d)>axis[3]+.001f)return false;}return true;}
inline int ExposureAt(const Zones&zones,const float*p){for(int i=zones.count-1;i>=0;--i)if(zones.values[i].v[3]>0&&Contains(zones.values[i],p))return int(zones.values[i].v[3]);return 0;}
inline void LocalWind(const Zones&zones,const float*p,float*out){for(int a=0;a<3;++a)out[a]=0;for(int i=0;i<zones.count;++i){const auto&z=zones.values[i];if(z.v[3]==0&&Contains(z,p))for(int a=0;a<3;++a)out[a]+=z.v[16+a];}}
inline std::string Encode(const Zones&z){std::ostringstream s;s<<"jar_weather_zones "<<z.count<<std::fixed<<std::setprecision(5);for(int i=0;i<z.count;++i)for(float x:z.values[i].v)s<<' '<<x;return s.str();}
inline bool Decode(const char*text,Zones&out){if(!text||std::strlen(text)>32768)return false;std::istringstream s(text);std::string word;Zones z;if(!(s>>word>>z.count)||word!="jar_weather_zones"||z.count<0||z.count>zoneLimit)return false;for(int i=0;i<z.count;++i){for(float&x:z.values[i].v)if(!(s>>x))return false;if(!Valid(z.values[i]))return false;}if(s>>word)return false;out=z;return true;}
} }
#endif
