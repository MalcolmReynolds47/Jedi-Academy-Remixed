// Native actor / rigid prop collision math, in game coordinates. GPL-2.0-only.
#ifndef JAR_ACTOR_CONTACT_H
#define JAR_ACTOR_CONTACT_H
#include <algorithm>
#include <cmath>
namespace jar { namespace physics {
struct ActorContact {float normal[3]{};float depth=0;};
// Swept expanded bounds prevent a fast prop crossing an actor between native
// frames. The normal points from the prop toward the actor.
inline bool SweepActor(const float*oldOrigin,const float*origin,const float*mins,const float*maxs,
 const float*actorMin,const float*actorMax,ActorContact&out){
 float low[3],high[3],entry=0,exit=1;int entryAxis=-1;float sign=0;bool inside=true,overlap=true;
 for(int a=0;a<3;++a){low[a]=actorMin[a]-maxs[a];high[a]=actorMax[a]-mins[a];
  inside&=oldOrigin[a]>low[a]&&oldOrigin[a]<high[a];overlap&=origin[a]>low[a]&&origin[a]<high[a];}
 if(!inside){bool hit=true;
  for(int a=0;a<3;++a){const float delta=origin[a]-oldOrigin[a];
   if(std::fabs(delta)<.0001f){if(oldOrigin[a]<=low[a]||oldOrigin[a]>=high[a]){hit=false;break;}continue;}
   float first=(low[a]-oldOrigin[a])/delta,last=(high[a]-oldOrigin[a])/delta;
   if(first>last)std::swap(first,last);
   if(first>=entry){entry=first;entryAxis=a;sign=delta>0?1.f:-1.f;}exit=std::min(exit,last);
   if(entry>exit){hit=false;break;}
  }
  if(hit&&entryAxis>=0&&entry>=0&&entry<=1&&exit>=entry){out={};out.normal[entryAxis]=sign;
   out.depth=std::max(0.f,(origin[entryAxis]-oldOrigin[entryAxis])*(1-entry)*sign);return out.depth>.001f;}
 }
 if(!overlap)return false;
 out={};out.depth=100000;int axis=0;
 for(int a=0;a<3;++a){const float left=origin[a]-low[a],right=high[a]-origin[a];
  const float depth=std::min(left,right);if(depth<out.depth){out.depth=depth;axis=a;sign=left<right?1.f:-1.f;}}
 out.normal[axis]=sign;return out.depth>.001f;
}
inline float ContactImpulse(float propMass,float actorMass,float approach){
 if(propMass<=0||actorMass<=0||approach<=0)return 0;
 return approach/(1/propMass+1/actorMass);
}
} }
#endif
