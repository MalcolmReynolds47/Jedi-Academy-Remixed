// Single-player Sandbox teleporter settings. GPL-2.0-only.
#ifndef JAR_TELEPORT_H
#define JAR_TELEPORT_H
#include <cstdint>
#include <cstring>
namespace jar { namespace teleport {
enum Kind { None, Volume, Marker };
enum Subject { Actors, Player, NPCs, NoActors, SubjectCount };
enum Object { Prop=1, Projectile=2, Assembly=4, Mover=8 };
constexpr uint32_t volumeId=97836047u, discId=86167446u, markerId=86345049u, destinationId=94436271u;
constexpr int minInterval=100,maxInterval=60000,defaultInterval=500;
constexpr const char* volumeModel="@jar_teleport_volume";
constexpr const char* markerModel="@jar_teleport_marker";
inline Kind Class(const char* name){
 if(name&&(!std::strcmp(name,"trigger_teleport")||!std::strcmp(name,"misc_teleporter")))return Volume;
 if(name&&!std::strcmp(name,"target_teleporter"))return Marker;
 return None;
}
struct Settings {
 Kind kind=None;
 int interval=0,subject=Actors;
 bool keepView=false,keepVelocity=false,enabled=true;
 bool props=false,projectiles=false,vehicles=false,assemblies=false,frozenProps=false,movers=false,worldAnchors=false;
};
inline bool Valid(const Settings& s){
 return (s.kind==Volume||s.kind==Marker)&&s.interval>=minInterval&&s.interval<=maxInterval&&
  s.subject>=Actors&&s.subject<SubjectCount&&
  (s.kind==Volume||(s.subject!=NoActors&&!s.props&&!s.projectiles&&!s.assemblies&&!s.frozenProps&&!s.movers&&!s.worldAnchors));
}
inline bool Same(const Settings& a,const Settings& b){
 return a.kind==b.kind&&a.interval==b.interval&&a.subject==b.subject&&a.keepView==b.keepView&&
  a.keepVelocity==b.keepVelocity&&a.enabled==b.enabled&&a.props==b.props&&
  a.projectiles==b.projectiles&&a.vehicles==b.vehicles&&a.assemblies==b.assemblies&&
  a.frozenProps==b.frozenProps&&a.movers==b.movers&&a.worldAnchors==b.worldAnchors;
}
} }
#endif
