// Portable branch of the existing laser-arm callbacks. GPL-2.0-only.
#ifndef G_LASER_ARM_H
#define G_LASER_ARM_H
#include "qcommon/jar_laser_arm.h"
inline bool G_LaserArmPortable(const gentity_t&e){return e.inuse&&jar::laserArm::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_laser_arm");}
inline bool G_LaserArmRead(const gentity_t&e,jar::laserArm::Settings&s){
 if(!G_LaserArmPortable(e))return false;
 s.action=e.count;s.damage=e.damage;s.yaw=e.pos2[1];s.pitch=e.pos2[0];s.speed=e.speed;s.duration=e.wait;s.range=e.radius;return jar::laserArm::Valid(s);
}
bool G_LaserArmMoved(gentity_t&);
inline bool G_LaserArmSet(gentity_t&e,const jar::laserArm::Settings&s){
 if(!G_LaserArmPortable(e)||!jar::laserArm::Valid(s))return false;
 e.count=s.action;e.damage=s.damage;e.pos2[1]=s.yaw;e.pos2[0]=s.pitch;e.speed=s.speed;e.wait=s.duration;e.radius=s.range;
 e.alt_fire=qfalse;e.s.loopSound=0;e.attackDebounceTime=0;G_LaserArmMoved(e);return true;
}
gentity_t*G_LaserArmRoot(gentity_t*);
bool G_LaserArmSpawn(gentity_t*);
bool G_LaserArmResumeBurst(gentity_t&,int remaining,int nextSample);
void G_LaserArmThink(gentity_t*);
void G_LaserArmControlledFrame(gentity_t*);
void G_LaserArmUse(gentity_t*,gentity_t*);
void G_LaserArmForget(gentity_t*);
void G_LaserArmRestore();
bool G_LaserArmBeam(gentity_t&,vec3_t,vec3_t);
#endif
