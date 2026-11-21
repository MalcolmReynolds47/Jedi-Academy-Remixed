// Single-player Sandbox contact-trigger settings. GPL-2.0-only.
#ifndef JAR_CONTACT_TRIGGER_H
#define JAR_CONTACT_TRIGGER_H
#include <cstdint>
#include <cstring>
namespace jar { namespace contactTrigger {
enum Kind { None, Repeat, Once, Hurt };
enum Subject { Actors, Player, NPCs, SubjectCount };
enum DamageMode { Normal, Falling, Electrical, DamageModeCount };
constexpr uint32_t repeatId=92116424u, onceId=90013869u, hurtId=94230341u;
constexpr int minInterval=100, maxTime=60000;
constexpr int maxDamage=10000, defaultDamage=5;
constexpr const char* model="@jar_contact_trigger";

inline Kind Class(const char* name){
 if(name&&!std::strcmp(name,"trigger_multiple"))return Repeat;
 if(name&&!std::strcmp(name,"trigger_once"))return Once;
 if(name&&!std::strcmp(name,"trigger_hurt"))return Hurt;
 return None;
}
struct Settings {
 Kind kind=None;
 int delay=0, interval=0, variation=0;
 int subject=Actors;
 bool useRequired=false, enabled=true;
 int damage=0, mode=Normal;
 bool armed=true;
};
inline bool Valid(const Settings& value){
 if(value.delay<0||value.delay>maxTime||value.subject<Actors||value.subject>=SubjectCount)return false;
 if(value.kind==Hurt)return !value.delay&&!value.variation&&value.interval>=minInterval&&value.interval<=maxTime&&
  value.damage>=1&&value.damage<=maxDamage&&value.mode>=Normal&&value.mode<DamageModeCount;
 if(value.damage||value.mode!=Normal||!value.armed)return false;
 if(value.kind==Once)return !value.interval&&!value.variation;
 return value.kind==Repeat&&value.interval>=minInterval&&value.interval<=maxTime&&
  value.variation>=0&&value.variation<=value.interval-minInterval&&value.variation<=maxTime-value.interval;
}
inline bool Same(const Settings& a,const Settings& b){
 return a.kind==b.kind&&a.delay==b.delay&&a.interval==b.interval&&a.variation==b.variation&&
  a.subject==b.subject&&a.useRequired==b.useRequired&&a.enabled==b.enabled&&
  a.damage==b.damage&&a.mode==b.mode&&a.armed==b.armed;
}
inline uint32_t Catalog(Kind kind){return kind==Repeat?repeatId:kind==Once?onceId:kind==Hurt?hurtId:0;}
} }
#endif
