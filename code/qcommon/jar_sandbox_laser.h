// Single-player Sandbox Laser Emitter settings. GPL-2.0-only.
#pragma once
#include <cstring>

namespace jar { namespace sandboxLaser {
constexpr const char* model="@jar_laser_emitter";
constexpr int maxDamage=1000, interval=100;
constexpr float range=2048;
struct Settings {int damage=1;bool firing=true,enabled=true;};
inline bool Class(const char* name){return name&&!std::strcmp(name,"target_laser");}
inline bool Valid(const Settings& value){return value.damage>=0&&value.damage<=maxDamage;}
inline bool Same(const Settings& a,const Settings& b){
 return a.damage==b.damage&&a.firing==b.firing&&a.enabled==b.enabled;
}
} }
