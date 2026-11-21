// Portable extend/retract door-camera settings. GPL-2.0-only.
#ifndef JAR_DOOR_CAMERA_H
#define JAR_DOOR_CAMERA_H
#include <cmath>
#include <cstring>
namespace jar { namespace doorCamera {
constexpr const char* base="models/map_objects/kejim/sec_panel.md3";
constexpr const char* shaft="models/map_objects/kejim/impcam_base.md3";
constexpr const char* head="models/map_objects/kejim/impcam.md3";
constexpr float closed=20,shaftLength=17.546875f;
struct Settings {float travel=48,speed=120;bool extended=false;};
inline bool Class(const char*n){return n&&!std::strcmp(n,"misc_model_jabba_cam");}
inline bool Valid(const Settings&s){return std::isfinite(s.travel)&&s.travel>=8&&s.travel<=256&&std::isfinite(s.speed)&&s.speed>=1&&s.speed<=512;}
}}
#endif
