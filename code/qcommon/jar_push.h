// Single-player Sandbox push controller settings. GPL-2.0-only.
#ifndef JAR_PUSH_H
#define JAR_PUSH_H
#include <cstdint>
#include <cstring>
namespace jar { namespace push {
enum Kind { None, Volume, Marker };
enum Subject { Actors, Player, NPCs, Props, All, SubjectCount };
enum Aim { Forward, Linked, Toward, AimCount };
constexpr uint32_t volumeId=94734448u, markerId=86503085u;
constexpr int maxSpeed=2000, minInterval=100, maxInterval=60000;
constexpr const char* volumeModel="@jar_push_volume";
constexpr const char* markerModel="@jar_push_marker";
inline Kind Class(const char* name){
 if(name&&!std::strcmp(name,"trigger_push"))return Volume;
 if(name&&!std::strcmp(name,"target_push"))return Marker;
 return None;
}
struct Settings {
 Kind kind=None;
 int speed=0,interval=0,subject=Actors,aim=Forward;
 bool additive=false,enabled=true,vehicles=false;
};
inline bool Valid(const Settings& s){
 return (s.kind==Volume||s.kind==Marker)&&s.speed>=1&&s.speed<=maxSpeed&&
  s.interval>=minInterval&&s.interval<=maxInterval&&s.subject>=Actors&&
  s.subject<(s.kind==Volume?SubjectCount:Props)&&s.aim>=Forward&&s.aim<AimCount;
}
inline bool Same(const Settings& a,const Settings& b){
 return a.kind==b.kind&&a.speed==b.speed&&a.interval==b.interval&&a.subject==b.subject&&
  a.aim==b.aim&&a.additive==b.additive&&a.enabled==b.enabled&&a.vehicles==b.vehicles;
}
} }
#endif
