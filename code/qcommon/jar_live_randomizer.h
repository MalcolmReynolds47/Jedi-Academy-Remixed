// Bounded, explicitly applied live settings. GPL-2.0-only.
#ifndef JAR_LIVE_RANDOMIZER_H
#define JAR_LIVE_RANDOMIZER_H
#include <cmath>
namespace jar {
// A bounded scene fade works in both renderers, including software-gamma
// windowed modes. Positive values lift toward white; negative values dim.
// Native gamma and HUD/menu readability are independent of this optional pass.
inline bool SceneBrightnessOverlay(float brightness, float color[4]) {
    if (!std::isfinite(brightness) || brightness==1.0f) return false;
    if (brightness<0.5f) brightness=0.5f;
    if (brightness>1.5f) brightness=1.5f;
    color[0]=color[1]=color[2]=brightness>1.0f ? 1.0f : 0.0f;
    color[3]=brightness>1.0f ? brightness-1.0f : 1.0f-brightness;
    return true;
}
struct LiveSetting { const char *toggle, *target, *initial; float validMin, validMax; };
static const LiveSetting liveSettings[] = {
    {"jar_randomGravity","g_gravity","800",100,1200},
    {"jar_randomSpeed","g_speed","250",100,425},
    {"jar_randomBrightness","jar_sceneBrightness","1",0.5f,1.5f},
    {"jar_randomFov","cg_fov","80",50,130},
    {"jar_randomSky","r_fastsky","0",0,1},
    {"jar_randomLights","r_dynamiclight","1",0,1}
};
static const unsigned liveSettingCount = sizeof(liveSettings)/sizeof(liveSettings[0]);
}
void JAR_LiveInit();
void JAR_LiveApply();
void JAR_LiveRestore();
void JAR_RandomizerBeforeLoad();
void JAR_RandomizerBeforeSave();
void JAR_RandomizerReadComplete();
void JAR_RandomizerResumeAfterLoad();
// Returns a persistent preference instead of a temporary randomized value.
const char *JAR_LiveConfigValue(const char *name, const char *current);
#endif
