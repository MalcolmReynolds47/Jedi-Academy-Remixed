// Server-side policy for revisiting a saved single-player hub. GPL-2.0-only.
#pragma once
#include <cstring>
#include <initializer_list>

namespace jar { namespace hubTransition {
inline bool RetainOutgoingCvar(const char* name) {
    for (const char* key : {"playersave", "playerammo", "playerinv", "playerfplvl",
                            "spawntarget", "tiers_complete", "jar_toolMode"}) {
        if (!std::strcmp(name,key)) return true;
    }
    constexpr char transferPrefix[]="jar_inventoryTransfer";
    return !std::strncmp(name,transferPrefix,sizeof(transferPrefix)-1);
}
} }
