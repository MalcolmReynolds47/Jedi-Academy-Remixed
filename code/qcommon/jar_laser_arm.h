// Portable laser-arm settings. GPL-2.0-only.
#ifndef JAR_LASER_ARM_H
#define JAR_LASER_ARM_H
#include <cmath>
#include <cstring>
namespace jar { namespace laserArm {
constexpr const char* baseModel="models/map_objects/kejim/impcam_base.md3";
constexpr const char* armModel="models/map_objects/kejim/impcam.md3";
constexpr const char* headModel="models/weapons2/laser_trap/laser_trap.md3";
constexpr int maxBurstMs=30000;
struct Settings {int action=0,damage=5;float yaw=0,pitch=0,speed=30,duration=3,range=4096;};
inline bool Class(const char*n){return n&&!std::strcmp(n,"misc_laser_arm");}
inline const char* Action(int action){static const char*names[]={"Fire","Turn left","Turn right","Aim up","Aim down"};return action>=0&&action<5?names[action]:"Invalid";}
inline bool Valid(const Settings&s){return s.action>=0&&s.action<5&&s.damage>=0&&s.damage<=1000&&std::isfinite(s.yaw)&&s.yaw>=-180&&s.yaw<=180&&std::isfinite(s.pitch)&&s.pitch>=-45&&s.pitch<=90&&std::isfinite(s.speed)&&s.speed>=1&&s.speed<=360&&std::isfinite(s.duration)&&s.duration>=.1f&&s.duration<=30&&std::isfinite(s.range)&&s.range>=64&&s.range<=8192;}
}}
#endif
