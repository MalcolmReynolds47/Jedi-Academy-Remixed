// Portable native combat-point properties. GPL-2.0-only.
#ifndef JAR_COMBAT_POINT_H
#define JAR_COMBAT_POINT_H
namespace jar { namespace combatPoint {
struct Settings {int flags=0;bool enabled=true;};
inline bool Valid(const Settings&s){return s.flags>=0&&s.flags<=63;}
}}
#endif
