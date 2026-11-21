// JA:R build presets. GPL-2.0-only.
#ifndef JAR_PRESETS_H
#define JAR_PRESETS_H
#include "jar_graphics.h"
namespace jar {
enum class BuildPreset { Intended, Vanilla, Chaos };
inline bool RandomizerCategory(const GraphicsSetting& setting) {
    return setting.page >= 8 && Prefix(setting.name,"jar_random") &&
        std::strcmp(setting.name,"jar_randomSeed") && std::strcmp(setting.name,"jar_randomMode");
}
// Null means preserve the user's value, including seeds, display geometry,
// camera, appearance and independently controlled sandbox preferences.
inline const char *BuildPresetValue(const GraphicsSetting& setting, BuildPreset preset) {
    const bool chaos = preset == BuildPreset::Chaos;
    if (RandomizerCategory(setting)) return chaos ? "1" : "0";
    if (!std::strcmp(setting.name,"jar_npcSafety")) return "1";
    if (!std::strcmp(setting.name,"jar_actorContinuity") || !std::strcmp(setting.name,"jar_actorRecovery")) return chaos ? "1" : "0";
    return preset == BuildPreset::Intended ? setting.preset[0] : nullptr;
}
// Run after config/command-line loading, before subsystem defaults register.
// Never replaces a value supplied by an existing profile, mod or command line.
template<class Exists, class Create> void InitializeIntendedDefaults(bool newProfile, Exists exists, Create create) {
    if (!newProfile) return;
    for (const auto& setting : graphicsSettings)
        if (const char *value=BuildPresetValue(setting,BuildPreset::Intended))
            if (!exists(setting.name)) create(setting.name,value);
}
}
#endif
