// Portable stock-art welder settings. GPL-2.0-only.
#ifndef JAR_WELDER_H
#define JAR_WELDER_H
#include <cmath>
#include <cstring>
namespace jar { namespace welder {
constexpr const char* model="models/weapons2/demp2/demp2.md3";
constexpr const char* effect="sparks/spark_nosnd";
constexpr const char* sound="sound/effects/electric_beam_lp.wav";
constexpr int maxCooldownMs=10000;
struct Settings {float reach=15,radius=45,interval=.2f;int damage=10;bool enabled=true;};
inline bool Class(const char*n){return n&&!std::strcmp(n,"misc_model_welder");}
inline bool Valid(const Settings&s){return std::isfinite(s.reach)&&s.reach>=1&&s.reach<=256&&std::isfinite(s.radius)&&s.radius>=1&&s.radius<=256&&std::isfinite(s.interval)&&s.interval>=.1f&&s.interval<=10&&s.damage>=0&&s.damage<=1000;}
}}
#endif
