// Additive Sandbox force for native movers and non-actor script controllers.
#ifndef G_JAR_MOVER_MOTION_H
#define G_JAR_MOVER_MOTION_H
#include "g_functions.h"
#include "g_sandbox_angular.h"
#include "g_sliding_door.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "qcommon/jar_angular_trajectory.h"
inline bool G_JAR_PortableDeck(const gentity_t& e){return G_LiftPortable(e)||G_BobbingPortable(e);}
inline bool G_JAR_PushableMover(const gentity_t& e){return e.bmodel||G_SandboxAngular(e)||G_JAR_PortableDeck(e);}
inline void G_JAR_ProjectedDeckHull(const gentity_t& e,const vec3_t angles,vec3_t mins,vec3_t maxs){
 if(G_LiftPortable(e))G_SandboxSlideHull(angles,mins,maxs,true);
 else G_BobbingHull(angles,mins,maxs);
}
inline bool G_JAR_MoverMotionEligible(const gentity_t&e){
 if(!e.inuse||e.client||(e.s.number<0||e.s.number>=ENTITYNUM_WORLD))return false;
 if(e.classname&&!strcmp(e.classname,"jar_sandbox_prop"))return false;
 return e.bmodel||e.s.eType==ET_MOVER||e.next_roff_time>0||(e.s.eType!=ET_ITEM&&e.s.eType!=ET_MISSILE&&e.e_ThinkFunc!=thinkF_NULL&&e.e_ThinkFunc!=thinkF_G_RunObject);
}
bool G_JAR_MoverTranslate(gentity_t&,const vec3_t requested,vec3_t actual);
bool G_JAR_MoverRotate(gentity_t&,const vec3_t angles);
bool G_JAR_MoverRotateAt(gentity_t&,const vec3_t angles,const vec3_t origin,const vec3_t pivot);
// External angular velocity is world-space radians/second.
struct JarMoverMotion {vec3_t velocity{},offset{},angular{};};
bool G_JAR_MoverMotionValid(const JarMoverMotion&);
bool G_JAR_MoverMotionRead(const gentity_t&,JarMoverMotion&);
bool G_JAR_MoverMotionRestore(gentity_t&,const JarMoverMotion&);
void G_JAR_MoverMotionStop(gentity_t&);
void G_JAR_MoverMotionRebase(gentity_t&,const vec3_t offset);
bool G_JAR_MoverMotionActive(const gentity_t&);
bool G_JAR_MoverTranslationActive(const gentity_t&);
void G_JAR_MoverForce(gentity_t&,const vec3_t acceleration);
void G_JAR_MoverTorque(gentity_t&,const vec3_t angularAcceleration);
void G_JAR_MoverSpin(gentity_t&,const vec3_t angular);
void G_JAR_MoverMotionFrame(float dt,bool(*allowed)(const gentity_t&),void(*moved)(gentity_t&)=nullptr);
bool G_JAR_MoverSetOffset(gentity_t&,const vec3_t offset);
void G_JAR_MoverPathOffset(const gentity_t&,vec3_t offset);
void G_JAR_MoverMotionForget(gentity_t*);
void G_JAR_MoverMotionReset();
bool G_JAR_MoverMotionPresent();
void G_JAR_MoverMotionExport(bool autosave,ojk::SavedGameHelper&);
bool G_JAR_MoverMotionImport(bool autosave,ojk::SavedGameHelper&,bool angular=true);
#endif
