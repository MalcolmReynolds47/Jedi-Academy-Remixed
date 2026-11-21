// Native rotating/pendulum callbacks and client/server angular representation.
#include "subbsp_mover_fixture.h"
static int soundEvents,damageEvents;
int BMS_START=0,BMS_MID=1,BMS_END=2;
int CAS_GetBModelSound(const char*,int){return 7;}
void G_AddEvent(gentity_t*,int,int){++soundEvents;}
void G_Sound(gentity_t*,int){}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){++damageEvents;}
#include "game/g_mover_sp_func_rotating.inc"
#include "game/g_mover_sp_func_pendulum.inc"
#include "game/g_mover_func_rotating_use.inc"
#include "game/g_mover_func_rotating_touch.inc"
#include "game/g_entity_angles.inc"
static void composedAxes(const vec3_t local,const vec3_t frame,vec3_t out[3]){
 vec3_t localAxis[3];AnglesToAxis(local,localAxis);for(int i=0;i<3;++i)rotate(frame,localAxis[i],out[i]);
}
static void sameOrientation(const vec3_t world,const vec3_t local,const vec3_t frame){vec3_t a[3],expected[3];AnglesToAxis(world,a);composedAxes(local,frame,expected);for(int i=0;i<3;++i)same(a[i],expected[i],.0001f);}
int main(){return RunTests("native axis rotation, pendulum paths, toggles, setters and model offsets",[]{
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gravity.value=800;
 const vec3_t frames[]={{0,0,0},{30,45,20},{90,-35,12},{-75,123,-61}};
 const vec3_t bases[]={{0,0,0},{20,40,25},{90,20,40},{-90,-80,17}};
 for(const auto&frame:frames)for(const auto&base:bases)for(int coordinate:{PITCH,YAW,ROLL})for(float rate:{100.f,-80.f}){
  scenario="spinner coordinate "+std::to_string(coordinate)+" frame "+std::to_string(frame[0])+" base "+std::to_string(base[0]);
  auto*r=root(frame);char keys[180];snprintf(keys,sizeof(keys),"\"angles\" \"%g %g %g\" \"modelAngles\" \"10 30 40\"",base[0],base[1],base[2]);
  auto*e=child(r,"func_rotating",keys,1|(coordinate==ROLL?4:coordinate==PITCH?8:0));e->speed=rate;e->targetname="switch";e->soundSet="test";SP_func_rotating(e);CHECK(e->s.apos.trType==TR_AXIS_LINEAR&&e->s.apos.trTime==1000);sameOrientation(e->currentAngles,base,frame);CHECK(near(VectorLength(e->s.apos.trDelta),fabs(rate)));
  const auto original=e->s.apos;
  for(int elapsed:{0,250,1000,2700,6000,100000}){vec3_t at,local;EvaluateTrajectory(&e->s.apos,1000+elapsed,at);VectorCopy(base,local);local[coordinate]+=elapsed*.001f*rate;sameOrientation(at,local,frame);}
  // The visual offset keeps the authored rest pose and follows the same rigid
  // motion, rather than introducing a second world-space Euler rotation.
  vec3_t restModel,restAxes[3],bodyAxes[3],modelAxes[3];VectorAdd(base,vec3_t{10,30,40},restModel);composedAxes(restModel,frame,restAxes);AnglesToAxis(e->s.apos.trBase,bodyAxes);
  for(int elapsed:{0,1700,3600}){vec3_t at;EvaluateTrajectory(&e->s.apos,1000+elapsed,at);jar::angular::ModelAxis(at,e->modelAngles,modelAxes);vec3_t atAxes[3];AnglesToAxis(at,atAxes);
   for(int i=0;i<3;++i){vec3_t expected{};for(int j=0;j<3;++j)VectorMA(expected,DotProduct(restAxes[i],bodyAxes[j]),atAxes[j],expected);same(modelAxes[i],expected,.0001f);}}
  // Native use stops at the current pose and resumes without time catch-up.
  level.time=2400;EvaluateTrajectory(&e->s.apos,level.time,e->currentAngles);vec3_t stopped;VectorCopy(e->currentAngles,stopped);const int sounds=soundEvents;func_rotating_use(e,nullptr,nullptr);CHECK(e->s.apos.trType==TR_AXIS_STATIONARY&&e->s.loopSound==0&&soundEvents==sounds+1);
  vec3_t at;EvaluateTrajectory(&e->s.apos,8000,at);sameAxes(at,stopped);level.time=8400;func_rotating_use(e,nullptr,nullptr);CHECK(e->s.apos.trType==TR_AXIS_LINEAR&&e->s.loopSound==7);EvaluateTrajectory(&e->s.apos,9400,at);vec3_t expected;EvaluateTrajectory(&original,3400,expected);sameAxes(at,expected);
  gentity_t actor;gclient_t client;actor.client=&client;actor.flags=0;damageEvents=0;func_rotating_touch(e,&actor,nullptr);CHECK(damageEvents==1);e->s.apos.trType=TR_AXIS_STATIONARY;func_rotating_touch(e,&actor,nullptr);CHECK(damageEvents==1);e->s.apos.trType=TR_AXIS_INTERPOLATE;func_rotating_touch(e,&actor,nullptr);CHECK(damageEvents==1);
  // A tool rotates the complete future path while preserving its clock/rate.
  e->s.apos=original;level.time=2400;EvaluateTrajectory(&e->s.apos,level.time,e->currentAngles);vec3_t old;VectorCopy(e->currentAngles,old);const vec3_t desired={-35,72,23};G_SetAngles(e,desired);CHECK(e->s.apos.trTime==original.trTime);EvaluateTrajectory(&e->s.apos,level.time,at);sameAxes(at,desired);
  EvaluateTrajectory(&original,level.time+1200,expected);vec3_t transformed;jar::angular::RebaseAngles(expected,old,desired,transformed);EvaluateTrajectory(&e->s.apos,level.time+1200,at);sameAxes(at,transformed);
  MemorySave saved;ojk::SavedGameHelper h(&saved);e->s.apos.sg_export(h);saved.reset_buffer_offset();trajectory_t restored{};restored.sg_import(h);h.ensure_all_data_read();EvaluateTrajectory(&restored,level.time+1200,expected);sameAxes(at,expected);
 }
 for(const auto&frame:frames)for(const auto&base:bases)for(float speed:{30.f,-45.f}){
  scenario="pendulum";auto*r=root(frame);char keys[180];snprintf(keys,sizeof(keys),"\"angles\" \"%g %g %g\" \"speed\" \"%g\" \"phase\" \"0.25\"",base[0],base[1],base[2],speed);auto*e=child(r,"func_pendulum",keys);SP_func_pendulum(e);CHECK(e->s.apos.trType==TR_AXIS_SINE&&e->s.apos.trDuration>0);const float length=12,frequency=1/(2*M_PI)*sqrt(800/(3*length));CHECK(e->s.apos.trDuration==int(1000/frequency));
  for(int now:{0,1000,2500,17000}){vec3_t at,local;EvaluateTrajectory(&e->s.apos,now,at);VectorCopy(base,local);const double phase=(double(now)-e->s.apos.trTime)/e->s.apos.trDuration*M_PI*2;local[ROLL]+=float(sin(phase)*speed);sameOrientation(at,local,frame);vec3_t velocity;EvaluateTrajectoryDelta(&e->s.apos,now,velocity);CHECK(near(VectorLength(velocity),fabs(cos(phase)*M_PI*2000/e->s.apos.trDuration*speed),.01f));}
  vec3_t initial;EvaluateTrajectory(&e->s.apos,level.time,initial);sameAxes(initial,e->currentAngles);
 }
 // Stock and portable-box rotators keep their preexisting trajectory contract.
 root(vec3_origin);auto*e=child(nullptr,"func_rotating","",1);SP_func_rotating(e);CHECK(e->s.apos.trType==TR_LINEAR);e=child(nullptr,"func_pendulum");SP_func_pendulum(e);CHECK(e->s.apos.trType==TR_SINE);
 auto*r=root(frames[1]);e=child(r,"func_rotating");SP_func_rotating(e);CHECK(e->s.apos.trType==TR_AXIS_STATIONARY);const auto original=e->s.apos;level.time=5000;func_rotating_use(e,nullptr,nullptr);vec3_t at;EvaluateTrajectory(&e->s.apos,level.time,at);sameAxes(at,original.trBase);
 // Held angular entities use the short physical arc across a pitch singularity.
 const vec3_t from={89,0,0},to={89,180,180};for(float f:{0.f,.25f,.5f,.75f,1.f}){jar::angular::Interpolate(from,to,f,at);const vec3_t expected={89+2*f,0,0};sameAxes(at,expected);}
 const vec3_t yawFrom={0,170,0},yawTo={0,-170,0};jar::angular::Interpolate(yawFrom,yawTo,.5f,at);sameAxes(at,vec3_t{0,180,0});
 });}
