// Native Workshop transform clearance before publishing a prop's pose.
// Included in the workshop namespace. GPL-2.0-only.
#include "g_sandbox_charges.h"

bool propBodyTransformClear(const gentity_t& entity,const Prop& model,int frame,const vec3_t origin,
 const vec3_t angles,bool sweep,const vec3_t fallbackMins,const vec3_t fallbackMaxs,
 const float* pivot=nullptr,jar::physics::Vec localPivot={}){
 constexpr int mask=MASK_SOLID|CONTENTS_BODY|SANDBOX_MOUNTED_CHARGE_CONTENTS;
 const auto geometry=gi.JAR_TraceMovingHull?propTransformGeometry(model,frame):nullptr;
 if(geometry&&geometry->RigidHull()){
  struct Query {const float* origin;int slot,mask;};
  Query query{pivot?pivot:sweep?entity.currentOrigin:origin,entity.s.number,mask};
  const auto clear=[](void* context,const JarSweepHull& shape){
   const auto& request=*static_cast<Query*>(context);
   trace_t hit{};
   vec3_t low,high;JAR_SweepBounds(shape,low,high);
   gi.JAR_TraceMovingHull(&hit,request.origin,request.origin,low,high,request.slot,request.mask,shape);
   return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
  };
  const jar::physics::Vec after={angles[0],angles[1],angles[2]};
  if(sweep){
   const auto& a=entity.currentAngles;
   vec3_t move{};
   if(!pivot)VectorSubtract(origin,entity.currentOrigin,move);
   // Fixed orientation can use a single linear sweep instead of an envelope.
   if(!pivot&&VectorCompare(a,angles)){
    JarSweepHull shape;
    if(!jar::physics::BuildPropSweep(*geometry,after,shape))return false;
    trace_t hit{};
    vec3_t low,high;JAR_SweepBounds(shape,low,high);
    gi.JAR_TraceMovingHull(&hit,entity.currentOrigin,origin,low,high,
     entity.s.number,mask,shape);
    return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
   }
   return jar::physics::TracePropRotation(*geometry,{a[0],a[1],a[2]},after,&query,clear,
    localPivot,{move[0],move[1],move[2]});
  }
  JarSweepHull shape;
  if(jar::physics::BuildPropSweep(*geometry,after,shape))return clear(&query,shape);
 }
 trace_t hit{};
 if(pivot){
  vec3_t low,high,move,axis;
  VectorSubtract(origin,entity.currentOrigin,move);
  const auto spin=jar::physics::AngularVelocity(
   {entity.currentAngles[0],entity.currentAngles[1],entity.currentAngles[2]},
   {angles[0],angles[1],angles[2]},1);
  VectorSet(axis,spin.x,spin.y,spin.z);
  const float sag=(Distance(pivot,entity.currentOrigin)+std::max(
   RadiusFromBounds(entity.mins,entity.maxs),RadiusFromBounds(fallbackMins,fallbackMaxs)))*
   (1-std::cos(.5f*VectorNormalize(axis)));
  for(int a=0;a<3;++a){
   const float pad=sag*std::sqrt(std::max(0.f,1-axis[a]*axis[a]));
   low[a]=std::min(entity.mins[a],move[a]+fallbackMins[a])-pad;
   high[a]=std::max(entity.maxs[a],move[a]+fallbackMaxs[a])+pad;
  }
  gi.trace(&hit,entity.currentOrigin,low,high,entity.currentOrigin,entity.s.number,mask,G2_NOCOLLIDE,0);
  return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
 }
 gi.trace(&hit,sweep?entity.currentOrigin:origin,fallbackMins,fallbackMaxs,origin,entity.s.number,mask,G2_NOCOLLIDE,0);
 return !hit.startsolid&&!hit.allsolid&&hit.fraction>=1;
}
