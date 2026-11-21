// Native trajectories keep running; force translates and rotates their frame.
// Included after the native pusher implementation in g_mover.cpp. GPL-2.0-only.
#include "g_mover_motion.h"
#include "jar_physics.h"
#include <map>
#include <algorithm>
#include <cmath>
namespace {
struct MoverMotion {vec3_t velocity{},offset{},acceleration{},angular{},angularAcceleration{};};
std::map<int,MoverMotion> moverMotion;
bool moverMotionEntity(const gentity_t&e){return e.inuse&&!e.client&&e.s.number>=0&&e.s.number<ENTITYNUM_WORLD;}
void moverMotionStop(MoverMotion&m){VectorClear(m.velocity);VectorClear(m.acceleration);VectorClear(m.angular);VectorClear(m.angularAcceleration);}

}
bool G_JAR_MoverMotionRead(const gentity_t&e,JarMoverMotion&out){auto it=moverMotion.find(e.s.number);if(!moverMotionEntity(e)||it==moverMotion.end())return false;VectorCopy(it->second.velocity,out.velocity);VectorCopy(it->second.offset,out.offset);VectorCopy(it->second.angular,out.angular);return true;}
bool G_JAR_MoverMotionValid(const JarMoverMotion&s){
 for(int a=0;a<3;++a)if(!std::isfinite(s.velocity[a])||std::fabs(s.velocity[a])>2000.01f||!std::isfinite(s.offset[a])||std::fabs(s.offset[a])>131072)return false;
 for(float v:s.angular)if(!std::isfinite(v)||std::fabs(v)>8.001f)return false;
 return VectorLength(s.angular)<=8.001f&&VectorLength(s.velocity)<=2000.01f;
}
bool G_JAR_MoverMotionRestore(gentity_t&e,const JarMoverMotion&s){
 if(!moverMotionEntity(e)||!G_JAR_MoverMotionValid(s))return false;
 auto&m=moverMotion[e.s.number];VectorCopy(s.velocity,m.velocity);VectorCopy(s.offset,m.offset);VectorCopy(s.angular,m.angular);VectorClear(m.acceleration);VectorClear(m.angularAcceleration);return true;
}
void G_JAR_MoverMotionStop(gentity_t&e){auto it=moverMotion.find(e.s.number);if(it!=moverMotion.end())moverMotionStop(it->second);}
void G_JAR_MoverMotionRebase(gentity_t&e,const vec3_t offset){
 const bool train=e.e_ReachedFunc==reachedF_Reached_Train||(e.classname&&!strcmp(e.classname,"func_train"));
 JarMoverMotion s;if(!G_JAR_MoverMotionRead(e,s)&&!train)return;VectorAdd(s.offset,offset,s.offset);G_JAR_MoverMotionRestore(e,s);
}
bool G_JAR_MoverMotionActive(const gentity_t&e){return moverMotionEntity(e)&&moverMotion.count(e.s.number)!=0;}
bool G_JAR_MoverTranslationActive(const gentity_t&e){auto it=moverMotion.find(e.s.number);return moverMotionEntity(e)&&it!=moverMotion.end()&&(VectorLengthSquared(it->second.velocity)>0||VectorLengthSquared(it->second.acceleration)>0);}
void G_JAR_MoverTorque(gentity_t&e,const vec3_t acceleration){
 if(!moverMotionEntity(e))return;for(int a=0;a<3;++a)if(!std::isfinite(acceleration[a])||std::fabs(acceleration[a])>1e8f)return;
 if(VectorLengthSquared(acceleration)<1e-12f)return;
 VectorAdd(moverMotion[e.s.number].angularAcceleration,acceleration,moverMotion[e.s.number].angularAcceleration);
}
void G_JAR_MoverSpin(gentity_t&e,const vec3_t angular){
 if(!moverMotionEntity(e))return;for(int a=0;a<3;++a)if(!std::isfinite(angular[a])||std::fabs(angular[a])>8.001f)return;
 if(VectorLength(angular)>8.001f)return;auto it=moverMotion.find(e.s.number);if(it==moverMotion.end()&&VectorLengthSquared(angular)==0)return;
 auto&m=moverMotion[e.s.number];VectorCopy(angular,m.angular);VectorClear(m.angularAcceleration);
}
void G_JAR_MoverForce(gentity_t&e,const vec3_t acceleration){
 if(!moverMotionEntity(e)||(!G_JAR_MoverMotionEligible(e)&&!G_JAR_MoverTranslationActive(e)))return;
 for(int a=0;a<3;++a)if(!std::isfinite(acceleration[a])||std::fabs(acceleration[a])>3200000)return;
 auto&m=moverMotion[e.s.number];VectorAdd(m.acceleration,acceleration,m.acceleration);
}
void G_JAR_MoverMotionFrame(float dt,bool(*allowed)(const gentity_t&),void(*moved)(gentity_t&)){
 if(!std::isfinite(dt)||dt<0||dt>.1f)dt=0;
 for(auto it=moverMotion.begin();it!=moverMotion.end();){auto&e=g_entities[it->first];auto&m=it->second;
  if(!moverMotionEntity(e)){it=moverMotion.erase(it);continue;}++it;
  if(!allowed||!allowed(e)){moverMotionStop(m);continue;}
  if(dt<=0){VectorClear(m.acceleration);VectorClear(m.angularAcceleration);continue;}
  // Cap each sweep at 1/250 second: even at the 2000-unit speed limit,
  // native destination-based rider detection advances at most eight units.
  const int steps=std::max(1,int(std::ceil(dt*250)));const float step=dt/steps;
  const float spinLimit=std::min(8.f,2000.f/std::max(1.f,RadiusFromBounds(e.mins,e.maxs)));
  const float oldRate=VectorLength(m.angular);if(oldRate>spinLimit)VectorScale(m.angular,spinLimit/oldRate,m.angular);
  for(int n=0;n<steps;++n){vec3_t spin,average;
   VectorMA(m.angular,step,m.angularAcceleration,spin);const float rate=VectorLength(spin);if(rate>spinLimit)VectorScale(spin,spinLimit/rate,spin);
   for(int a=0;a<3;++a)average[a]=(m.angular[a]+spin[a])*.5f;
   if(VectorLengthSquared(average)>1e-12f){const auto angles=jar::physics::AdvanceAngles({e.currentAngles[0],e.currentAngles[1],e.currentAngles[2]},{average[0],average[1],average[2]},step);const vec3_t target={angles.x,angles.y,angles.z};
    if(G_JAR_MoverRotate(e,target))VectorCopy(spin,m.angular);else {VectorClear(m.angular);VectorClear(m.angularAcceleration);}
   }else VectorCopy(spin,m.angular);
   vec3_t next,move,actual;
   VectorMA(m.velocity,step,m.acceleration,next);const float speed=VectorLength(next);if(speed>2000)VectorScale(next,2000/speed,next);
   for(int a=0;a<3;++a)move[a]=(m.velocity[a]+next[a])*.5f*step;
   if(VectorLengthSquared(move)<1e-12f){VectorCopy(next,m.velocity);continue;}
   bool bounded=true;for(int a=0;a<3;++a)bounded=bounded&&std::fabs(m.offset[a]+move[a])<=131072;
   if(!bounded){VectorClear(m.velocity);VectorClear(m.acceleration);continue;}
   const bool complete=G_JAR_MoverTranslate(e,move,actual);VectorAdd(m.offset,actual,m.offset);VectorCopy(next,m.velocity);
   if(!complete){VectorClear(m.velocity);VectorClear(m.acceleration);}
  }
  VectorClear(m.acceleration);VectorClear(m.angularAcceleration);if(moved)moved(e);
 }
}
bool G_JAR_MoverSetOffset(gentity_t&e,const vec3_t offset){
 if(!moverMotionEntity(e))return false;for(int a=0;a<3;++a)if(!std::isfinite(offset[a])||std::fabs(offset[a])>131072)return false;
 auto&m=moverMotion[e.s.number];moverMotionStop(m);VectorCopy(offset,m.offset);return true;
}
void G_JAR_MoverPathOffset(const gentity_t&e,vec3_t offset){auto it=moverMotion.find(e.s.number);if(it==moverMotion.end())VectorClear(offset);else VectorCopy(it->second.offset,offset);}
void G_JAR_MoverMotionForget(gentity_t*e){if(e)moverMotion.erase(e->s.number);}
void G_JAR_MoverMotionReset(){moverMotion.clear();}
bool G_JAR_MoverMotionPresent(){return !moverMotion.empty();}
void G_JAR_MoverMotionExport(bool autosave,ojk::SavedGameHelper&h){
 h.write<uint32_t>(autosave?0:uint32_t(moverMotion.size()));if(autosave)return;
 for(const auto&entry:moverMotion){h.write<int32_t>(entry.first);h.write<float>(entry.second.velocity,3);h.write<float>(entry.second.offset,3);h.write<float>(entry.second.angular,3);}
}
bool G_JAR_MoverMotionImport(bool autosave,ojk::SavedGameHelper&h,bool angular){
 uint32_t count;h.read<uint32_t>(count);if(count>=ENTITYNUM_WORLD||(autosave&&count))return false;std::map<int,MoverMotion> candidate;
 for(unsigned i=0;i<count;++i){int id;MoverMotion m;h.read<int32_t>(id);h.read<float>(m.velocity,3);h.read<float>(m.offset,3);if(angular)h.read<float>(m.angular,3);
  if(id<0||id>=ENTITYNUM_WORLD||!moverMotionEntity(g_entities[id])||candidate.count(id))return false;
  for(int a=0;a<3;++a)if(!std::isfinite(m.velocity[a])||std::fabs(m.velocity[a])>2000.01f||!std::isfinite(m.offset[a])||std::fabs(m.offset[a])>131072)return false;
  for(float v:m.angular)if(!std::isfinite(v)||std::fabs(v)>8.001f)return false;
  if(VectorLength(m.angular)>8.001f||VectorLength(m.velocity)>2000.01f)return false;candidate.emplace(id,m);
 }moverMotion=std::move(candidate);return true;
}
