// Write the same launch velocity to native and optional rigid prop motion.
#ifndef G_PUSH_PROP_MOTION_H
#define G_PUSH_PROP_MOTION_H
#include "jar_physics.h"
inline void G_PushPropMotion(jar::physics::World& world,gentity_t& prop,const vec3_t velocity,bool rigid){
 if(rigid&&!world.Correct(prop.s.number,
   {prop.currentOrigin[0],prop.currentOrigin[1],prop.currentOrigin[2]},
   {velocity[0],velocity[1],velocity[2]}))return;
 VectorCopy(velocity,prop.s.pos.trDelta);
}
#endif
