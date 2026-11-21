#pragma once
#include <cmath>
#include <cstring>
namespace jar { namespace waypoint {
inline bool Class(const char*n){return n&&(!std::strcmp(n,"waypoint")||!std::strcmp(n,"waypoint_small"));}
inline bool Position(const float*p){for(int i=0;i<3;++i)if(!std::isfinite(p[i])||std::fabs(p[i])>65535)return false;return true;}
struct Settings {float range=512;bool enabled=true;};
inline bool Valid(const Settings&s){return std::isfinite(s.range)&&s.range>=0&&s.range<=1024;}
}}
