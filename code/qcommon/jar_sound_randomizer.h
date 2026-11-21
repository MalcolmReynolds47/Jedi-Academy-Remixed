// Deterministic audio substitutions. GPL-2.0-only.
#ifndef JAR_SOUND_RANDOMIZER_H
#define JAR_SOUND_RANDOMIZER_H
#include "jar_randomizer.h"
#include <cstring>
namespace jar {
static const char *const soundCvars[] = {"jar_randomEffects","jar_randomUI","jar_randomLoops","jar_randomMusic","jar_randomVoices"};
inline bool CanCaptureAudio(bool started, bool muted, bool openAL, bool bufferReady, int rate) {
    return started && !muted && !openAL && bufferReady && rate>=8000 && rate<=96000;
}
enum class SoundRole { Effects, Interface, Loop };
inline SoundRole AudioRole(const char *name) {
    return name && (std::strstr(name,"sound/interface/") || std::strstr(name,"sound/menu/")) ? SoundRole::Interface : SoundRole::Effects;
}
}
#endif
