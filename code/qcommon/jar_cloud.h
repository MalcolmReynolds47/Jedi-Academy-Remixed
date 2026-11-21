// Native cloud-layer settings shared by the editor, clipboard and renderers.
// GPL-2.0-only.
#ifndef JAR_CLOUD_H
#define JAR_CLOUD_H
#include <cmath>
#include <cstring>
namespace jar { namespace cloud {
constexpr int tubeFlag=1,alternateFlag=2,offFlag=4;
struct Settings {float radius=512,inner=128,curve=0;bool tube=false,alternate=false,enabled=true;};
inline bool Class(const char*name){return name&&!std::strcmp(name,"fx_cloudlayer");}
inline bool Valid(const Settings&s){return std::isfinite(s.radius)&&s.radius>=16&&s.radius<=32768&&std::isfinite(s.inner)&&s.inner>=0&&s.inner<=s.radius-8&&std::isfinite(s.curve)&&s.curve>=-20&&s.curve<=20;}
} }
#endif
