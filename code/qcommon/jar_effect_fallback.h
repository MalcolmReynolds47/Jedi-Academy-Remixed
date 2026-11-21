// Missing native projectile media only; never replace an existing definition.
// Original JA:R code, GPL-2.0-only.
#pragma once
#include <cstring>
namespace jar {
inline const char *MissingEffectFallback(const char *key,bool fileExists,bool editorPath=false) {
    if (fileExists || editorPath || !key) return nullptr;
    if (!std::strcmp(key,"ships/bomb")) return "jar/fx/fallback/effects/ships/bomb.efx";
    if (!std::strcmp(key,"ships/ion_blastershot")) return "jar/fx/fallback/effects/ships/ion_blastershot.efx";
    return nullptr;
}
}
