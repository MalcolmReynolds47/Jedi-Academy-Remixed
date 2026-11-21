#pragma once
#include "../../shared/qcommon/jar_movement_limits.h"

namespace jar { namespace movement {
enum Kind { None, Actor, Gravity, Friction };
constexpr int maxGravity=JAR_MOVEMENT_MAX_GRAVITY, maxFriction=JAR_MOVEMENT_MAX_FRICTION;
constexpr int defaultGravity=JAR_MOVEMENT_DEFAULT_GRAVITY, defaultFriction=JAR_MOVEMENT_DEFAULT_FRICTION;
constexpr unsigned gravityId=95709066, frictionId=92295820;

struct Marker {
 Kind kind=None;
 int value=0;
 bool enabled=false;
};
struct ActorState {
 int gravity=0,friction=0;
 bool customGravity=false;
};
inline bool Valid(const Marker& state){
 return state.value>=0&&((state.kind==Gravity&&state.value<=maxGravity)||
                         (state.kind==Friction&&state.value<=maxFriction));
}
inline bool Same(const Marker& a,const Marker& b){
 return a.kind==b.kind&&a.value==b.value&&a.enabled==b.enabled;
}
inline bool Same(const ActorState& a,const ActorState& b){
 return a.customGravity==b.customGravity&&(!a.customGravity||a.gravity==b.gravity)&&a.friction==b.friction;
}
}}
