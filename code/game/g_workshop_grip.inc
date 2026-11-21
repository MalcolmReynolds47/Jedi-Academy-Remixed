// The temporary grab belongs to one active Physgun hold. GPL-2.0-only.
#pragma once
#include "jar_physics.h"

jar::physics::PhysgunGrip physgunGrip;
jar::physics::Vec physgunViewOrigin,physgunViewDirection;
float physgunViewYaw=0;

jar::physics::Vec physgunAimPoint(float distance){
 return {physgunViewOrigin.x+distance*physgunViewDirection.x,
         physgunViewOrigin.y+distance*physgunViewDirection.y,
         physgunViewOrigin.z+distance*physgunViewDirection.z};
}

void physgunOriginGoal(const vec3_t angles,float distance,vec3_t target){
 const auto point=physgunGrip.Origin(physgunAimPoint(distance),{angles[0],angles[1],angles[2]});
 VectorSet(target,point.x,point.y,point.z);
}
