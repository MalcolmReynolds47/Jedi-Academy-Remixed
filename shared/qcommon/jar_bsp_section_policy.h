// Shared section classes for native SP and MP portable BSP admission.
// GPL-2.0-only.
#ifndef JAR_BSP_SECTION_POLICY_H
#define JAR_BSP_SECTION_POLICY_H
#include <cstring>

namespace jar { namespace bspSection {
enum class Mode { Generic, Stationary, Bobbing, Fire, Hurt, Breakable, Shaft, Rift, Korriban, Yavin, Sentinel, Tatooine, Taspir, Factory, Coruscant, Hilltops, DropZone, Wasteland, SiegeKorriban };
inline bool CapturePractice(Mode mode){return mode==Mode::Factory||mode==Mode::Coruscant||mode==Mode::Hilltops||mode==Mode::DropZone||mode==Mode::Wasteland;}

inline bool Breakable(Mode mode){return mode==Mode::Breakable||mode==Mode::Yavin;}
constexpr int cityBreakables=69;
constexpr int yavinBreakables=30;

inline bool Start(const char* name) {
    return name && (!strcmp(name, "info_player_start") || !strcmp(name, "info_player_deathmatch") ||
           !strcmp(name, "info_player_duel") || !strcmp(name, "info_player_duel1") ||
           !strcmp(name, "info_player_duel2"));
}

inline bool Position(const char* name) {
    return Start(name) || (name && !strcmp(name, "info_player_intermission"));
}
} }
#endif
