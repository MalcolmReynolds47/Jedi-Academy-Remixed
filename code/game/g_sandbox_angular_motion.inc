// Step native portable angular movers in bounded increments. GPL-2.0-only.
#include "g_sandbox_angular.h"
#include "g_mover_motion.h"

void G_SandboxAngularFrame(gentity_t& e,int elapsed){
 using namespace jar::sandboxAngular;
 Settings settings;
 if(!G_SandboxAngularRead(e,settings)||!settings.running||!settings.enabled)return;
 elapsed=std::max(0,std::min(elapsed,100));
 const int maxStep=JAR_AngularMaxStep(settings.kind==Pendulum,settings.rate,settings.period);
 while(elapsed>0){
  const int step=std::min(elapsed,maxStep);
  int phase=settings.phase;
  const float turn=JAR_AngularStep(settings.kind==Pendulum,settings.rate,settings.period,step,&phase);
  trajectory_t rotation{};
  rotation.trType=TR_AXIS_LINEAR;
  VectorCopy(e.currentAngles,rotation.trBase);
  vec3_t target;
  G_SandboxAngularAxis(e,settings.axis,rotation.trDelta);
  VectorScale(rotation.trDelta,turn,rotation.trDelta);
  jar::angular::Evaluate(rotation,1000,target);
  if(turn&&!G_JAR_MoverRotate(e,target)){
   e.s.eFlags|=EF_BLOCKED_MOVER;
   return;
  }
  e.s.eFlags&=~EF_BLOCKED_MOVER;
  e.count=settings.phase=phase;
  elapsed-=step;
 }
}
