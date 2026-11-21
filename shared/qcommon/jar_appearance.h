// JA:R display validation and namespaced asset overlays. GPL-2.0-only.
#ifndef JAR_APPEARANCE_H
#define JAR_APPEARANCE_H
#include <cstring>
#include <cstdio>
#include <cstddef>
#include "jar_display.h"

namespace jar {
inline bool DecimalInRange(const char *text,int minimum,int maximum){return JAR_DecimalInRange(text,minimum,maximum)!=0;}
inline bool ResolutionValid(int width,int height){return JAR_ResolutionValid(width,height)!=0;}
inline bool Prefix(const char *text, const char *prefix) {
    return !std::strncmp(text, prefix, std::strlen(prefix));
}
inline bool SafePackId(const char *id) {
    if (!id || !*id || std::strlen(id) > 16) return false;
    for (const char *p=id; *p; ++p)
        if (!(*p >= 'a' && *p <= 'z') && !(*p >= '0' && *p <= '9') && *p != '_') return false;
    return true;
}
// Only visual/audio asset paths are eligible. Settings, saves, models, scripts,
// collision and executable code never enter this overlay path.
inline const char *AssetCategory(const char *name) {
    if (Prefix(name,"gfx/2d/dlight")) return nullptr; // world lighting texture
    if (Prefix(name,"fonts/") || Prefix(name,"gfx/2d/charsgrid")) return "font";
    if (Prefix(name,"gfx/effects/sabers/") || Prefix(name,"sfx_sabers/")) return "saber";
    if (Prefix(name,"sound/weapons/saber/")) return "sound";
    if (Prefix(name,"effects/")) return "fx";
    const char *ext = std::strrchr(name,'.');
    if (!ext || (std::strcmp(ext,".tga") && std::strcmp(ext,".jpg") && std::strcmp(ext,".png"))) return nullptr;
    if (Prefix(name,"gfx/menus/") || Prefix(name,"gfx/hud/") || Prefix(name,"gfx/2d/") || Prefix(name,"gfx/interface/") ||
        Prefix(name,"menu/art/") || Prefix(name,"ui/assets/")) return "ui";
    return nullptr;
}
inline bool AssetOverlay(char *out, size_t capacity, const char *filename, const char *category, const char *pack) {
    if (!SafePackId(pack) || !std::strcmp(pack,"stock") || !filename || !category) return false;
    const int n = std::snprintf(out,capacity,"jar/%s/%s/%s",category,pack,filename);
    return n > 0 && size_t(n) < capacity;
}
// Prefer either selected encoding before falling back to a retail WAV. Preserve
// the selected extension because the sound loader dispatches on WAV vs MP3.
template<class Exists>
inline bool SoundOverlay(char *out, size_t capacity, const char *name, const char *pack, Exists exists) {
    if (!Prefix(name,"sound/weapons/saber/") || !AssetOverlay(out,capacity,name,"sound",pack)) return false;
    char *dot=std::strrchr(out,'.');
    if (!dot || (std::strcmp(dot,".wav") && std::strcmp(dot,".mp3"))) return false;
    if (exists(out)) return true;
    std::strcpy(dot,!std::strcmp(dot,".wav") ? ".mp3" : ".wav");
    return exists(out);
}
}
#endif
