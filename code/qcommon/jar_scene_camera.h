// Portable scene cameras; native entity fields own persistent state.
#pragma once
#include <cmath>
#include <cstring>
namespace jar { namespace sceneCamera {
inline bool Focus(const char*n){return n&&!std::strcmp(n,"misc_camera_focus");}
inline bool Track(const char*n){return n&&!std::strcmp(n,"misc_camera_track");}
inline bool Class(const char*n){return Focus(n)||Track(n);}
struct Settings {float speed=64,turn=180,wait=1;bool loop=true;};
inline bool Valid(const Settings&s){return std::isfinite(s.speed)&&s.speed>=0&&s.speed<=1024&&std::isfinite(s.turn)&&s.turn>=1&&s.turn<=720&&std::isfinite(s.wait)&&s.wait>=0&&s.wait<=30;}
}}
