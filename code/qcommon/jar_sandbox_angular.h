// Settings for the portable spinner and pendulum cards. GPL-2.0-only.
#pragma once
#include <cstring>
#include "../../shared/qcommon/jar_angular_mover.h"

namespace jar { namespace sandboxAngular {
enum Kind {None, Spinner, Pendulum};
enum Axis {Up=JAR_ANGULAR_UP, Forward=JAR_ANGULAR_FORWARD, Side=JAR_ANGULAR_SIDE, AxisCount=JAR_ANGULAR_AXES};
constexpr char model[]="@jar_angular_crate";
constexpr int maxSpeed=JAR_SPINNER_MAX,maxSwing=JAR_PENDULUM_MAX,minPeriod=JAR_ANGULAR_MIN_PERIOD,maxPeriod=JAR_ANGULAR_MAX_PERIOD;

struct Settings {
 Kind kind=None;
 int axis=Up,rate=JAR_SPINNER_RATE,period=JAR_ANGULAR_PERIOD,phase=0;
 bool running=false,enabled=true;
};

inline Kind Class(const char* name){
 if(!name)return None;
 if(!std::strcmp(name,"func_rotating"))return Spinner;
 return !std::strcmp(name,"func_pendulum")?Pendulum:None;
}
inline Settings Defaults(Kind kind){
 return kind==Pendulum?Settings{Pendulum,Forward,JAR_PENDULUM_SWING,JAR_ANGULAR_PERIOD,0,true,true}:Settings{Spinner};
}
inline bool Valid(const Settings& s){
 return (s.kind==Spinner||s.kind==Pendulum)&&JAR_AngularSettingsValid(s.kind==Pendulum,s.axis,s.rate,s.period,s.phase);
}
inline bool SameControls(const Settings& a,const Settings& b){
 return a.kind==b.kind&&a.axis==b.axis&&a.rate==b.rate&&a.period==b.period&&a.running==b.running&&a.enabled==b.enabled;
}
inline const char* AxisLabel(int axis){
 return JAR_AngularAxisLabel(axis);
}
}}
