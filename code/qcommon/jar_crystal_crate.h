// Portable crystal crate properties; native art is used without editing.
#ifndef JAR_CRYSTAL_CRATE_H
#define JAR_CRYSTAL_CRATE_H
#include <cmath>
#include <cstring>
namespace jar { namespace crystalCrate {
constexpr const char* model="models/map_objects/imperial/crate1.md3";
constexpr const char* crystal="models/map_objects/mp/crystal_blue.md3";
constexpr const char* hitEffect="sparks/spark_nosnd";
constexpr const char* explosion="thermal/explosion";
constexpr const char* debris="chunks/metalexplode";
constexpr const char* sound="sound/weapons/explosions/cargoexplode.wav";
struct Settings {int hitDamage=16,blastDamage=40;float hitRadius=32,blastRadius=80;bool solid=true;};
inline bool Class(const char*s){return s&&!strcmp(s,"misc_crystal_crate");}
inline bool Valid(const Settings&s){return s.hitDamage>=0&&s.hitDamage<=1000&&s.blastDamage>=0&&s.blastDamage<=1000&&std::isfinite(s.hitRadius)&&s.hitRadius>=1&&s.hitRadius<=1024&&std::isfinite(s.blastRadius)&&s.blastRadius>=1&&s.blastRadius<=1024&&s.blastRadius==std::floor(s.blastRadius);}
}}
#endif
