// Portable searchlight settings shared by the game and context editor.
#ifndef JAR_SPOTLIGHT_H
#define JAR_SPOTLIGHT_H
#include <cmath>
#include <algorithm>
#include <cstring>
#include "../../shared/qcommon/jar_searchlight.h"
namespace jar { namespace spotlight {
constexpr const char*model=JAR_SEARCHLIGHT_MODEL;
struct Settings{float range=JAR_SEARCHLIGHT_DEFAULT_RANGE,cone=JAR_SEARCHLIGHT_DEFAULT_ANGLE,light=JAR_SEARCHLIGHT_DEFAULT_LIGHT,rgb[3]{1,1,1};bool tracking=true,enabled=true;};
inline bool Class(const char*n){return n&&!std::strcmp(n,"misc_spotlight");}
inline bool Valid(const Settings&s){if(!std::isfinite(s.range)||s.range<64||s.range>8192||!std::isfinite(s.cone)||s.cone<1||s.cone>30||!std::isfinite(s.light)||s.light<0||s.light>1024)return false;for(float c:s.rgb)if(!std::isfinite(c)||c<0||c>1)return false;return true;}
inline float Width(const Settings&s,float length){return length*std::tan(s.cone*.01745329252f);}
}}
#endif
