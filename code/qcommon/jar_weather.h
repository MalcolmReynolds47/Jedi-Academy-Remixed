// Portable weather properties and bounded renderer transport. GPL-2.0-only.
#ifndef JAR_WEATHER_H
#define JAR_WEATHER_H
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
namespace jar { namespace weather {
enum Kind {Rain,Snow,Puff,Wind,WindZone,Exposure,Invalid};
inline int Modes(Kind k){return k==Exposure?2:k==Snow||k==Wind||k==WindZone?3:4;}
constexpr int offFlag=128,particleLimit=4000;
inline Kind Class(const char*n){if(!n)return Invalid;const char*names[]={"fx_rain","fx_snow","fx_puff","fx_wind","fx_wind_zone","misc_weather_zone"};for(int i=0;i<6;++i)if(!strcmp(n,names[i]))return Kind(i);return Invalid;}
inline bool ZoneClass(const char*n){auto k=Class(n);return k==WindZone||k==Exposure;}
struct Settings {Kind kind=Rain;int mode=1;float density=1,size=1,alpha=.5f,speed=500;bool enabled=true;};
inline bool Valid(const Settings&s){return s.kind>=Rain&&s.kind<=Exposure&&s.mode>=0&&s.mode<Modes(s.kind)&&std::isfinite(s.density)&&s.density>=.1f&&s.density<=4&&std::isfinite(s.size)&&s.size>=.1f&&s.size<=8&&std::isfinite(s.alpha)&&s.alpha>=0&&s.alpha<=1&&std::isfinite(s.speed)&&s.speed>=0&&s.speed<=5000;}
inline const char*Mode(Kind kind,int mode){static const char*names[6][4]={{"Light","Normal","Heavy","Acid"},{"Light","Normal","Heavy",""},{"Fog","Sand","Smoke","Snowstorm"},{"Steady","Gusting","Swirling",""},{"Steady","Gusting","Swirling",""},{"Shelter","Exposed","",""}};return kind>=Rain&&kind<=Exposure&&mode>=0&&mode<Modes(kind)?names[kind][mode]:"Unknown";}
struct Layer {int count=0,mode=0;float size=1,alpha=.5f;};
struct Frame {Layer layers[3];float wind[3]{};};
inline bool Valid(const Frame&f){for(int i=0;i<3;++i){const auto&l=f.layers[i];if(l.count<0||l.count>particleLimit||l.mode<0||l.mode>(i==1?2:3)||!std::isfinite(l.size)||l.size<.1f||l.size>8||!std::isfinite(l.alpha)||l.alpha<0||l.alpha>1)return false;}for(float w:f.wind)if(!std::isfinite(w)||std::fabs(w)>5000)return false;return true;}
inline bool Same(const Layer&a,const Layer&b){return a.count==b.count&&a.mode==b.mode&&a.size==b.size&&a.alpha==b.alpha;}
inline bool HasWind(const Frame&f){return f.wind[0]!=0||f.wind[1]!=0||f.wind[2]!=0;}
// Classic's authored direction query mixes its normalized global direction with
// local zones. Preserve that behavior exactly until a portable wind is present.
inline void ClassicWindBase(const Frame&f,const float*direction,const float*velocity,float*out,const float*local=nullptr){const bool active=HasWind(f)||(local&&(local[0]!=0||local[1]!=0||local[2]!=0));for(int a=0;a<3;++a)out[a]=active?velocity[a]+f.wind[a]+(local?local[a]:0):direction[a];}
inline bool Active(const Frame&f){for(const auto&l:f.layers)if(l.count)return true;return false;}
inline std::string Encode(const Frame&f){char b[384];const auto&a=f.layers[0],&c=f.layers[1],&d=f.layers[2];std::snprintf(b,sizeof(b),"jar_weather %d %d %.3f %.3f %d %d %.3f %.3f %d %d %.3f %.3f %.3f %.3f %.3f",a.count,a.mode,a.size,a.alpha,c.count,c.mode,c.size,c.alpha,d.count,d.mode,d.size,d.alpha,f.wind[0],f.wind[1],f.wind[2]);return b;}
// Recognized-but-invalid commands leave the previous frame intact.
inline bool Decode(const char*text,Frame&out){if(!text||std::strlen(text)>383)return false;std::istringstream stream(text);std::string prefix,extra;Frame f;if(!(stream>>prefix)||prefix!="jar_weather")return false;for(auto&l:f.layers)if(!(stream>>l.count>>l.mode>>l.size>>l.alpha))return false;for(float&w:f.wind)if(!(stream>>w))return false;if(stream>>extra||!Valid(f))return false;out=f;return true;}
// Concrete native material/configuration used by both renderer adapters.
struct Render {const char*texture;float width,height,gravity,color[4];bool additive,water,oriented;};
inline Render Describe(int kind,const Layer&l){
 Render r{"gfx/effects/alpha_smoke2b.tga",70,70,0,{.6f,.6f,.6f,l.alpha},true,false,false};
 if(kind==Rain){r.texture="gfx/world/rain.jpg";r.width=1.2f;r.height=80;r.gravity=l.mode==2?2800:2000;r.water=r.oriented=true;if(l.mode==3){r.color[0]=.5f;r.color[1]=.8f;r.color[2]=.3f;}}
 else if(kind==Snow||(kind==Puff&&l.mode==3)){r.texture="gfx/effects/snowflake1.tga";r.width=r.height=1.5f;r.gravity=kind==Puff?600:300;r.water=true;r.color[0]=r.color[1]=r.color[2]=.75f;}
 else if(l.mode==1){r.color[0]=.9f;r.color[1]=.6f;r.color[2]=0;r.additive=false;}
 else if(l.mode==2){r.color[0]=r.color[1]=r.color[2]=.3f;r.additive=false;r.width=r.height=100;}
 else {r.width=r.height=100;r.color[0]=r.color[1]=r.color[2]=.19f;}
 r.width*=l.size;r.height*=l.size;return r;
}
} }
#endif
