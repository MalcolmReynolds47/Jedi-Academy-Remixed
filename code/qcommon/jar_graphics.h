// JA:R graphics settings shared by the menu and regression tests. GPL-2.0-only.
#ifndef JAR_GRAPHICS_H
#define JAR_GRAPHICS_H
#include <cstring>
#include <cstddef>
#include "jar_appearance.h"
#include "jar_randomizer.h"
#include "qcommon/jar_console_style.h"
#include "qcommon/jar_interface_choices.h"
#include "qcommon/jar_classic_graphics.h"
#include "qcommon/jar_effect_choices.h"
#include "qcommon/jar_combat.h"

namespace jar {
struct GraphicsChoice { const char *label; const char *value; };
struct GraphicsSetting {
    const char *name, *label, *description, *initial;
    int page;
    bool restart, modern;
    GraphicsChoice choices[7];
    const char *preset[3]; // Balanced graphics, Quality, Maximum FPS
    int minimum, maximum; // nonzero maximum: editable decimal integer
};
// Defaults match the existing renderer/cgame. Merely opening a menu changes no settings.
static const GraphicsSetting graphicsSettings[] = {
 {"cl_renderer", "Renderer", "Rend2 uses modern OpenGL and GPU model skinning. Apply restarts video.", "rdjar-vanilla", 1, true, false,
  {{"Classic", "rdjar-vanilla"}, {"Rend2 (experimental)", "rdjar-rend2"}}, {"rdjar-rend2", "rdjar-rend2", "rdjar-rend2"}},
 {"r_ext_multisample", "MSAA", "Smooths polygon edges. Higher samples cost GPU time and memory.", "0", 1, true, false,
  JAR_GFX_R_EXT_MULTISAMPLE_CHOICES, {"0","4","0"}},
 {"r_smaa", "SMAA", "Rend2 edge smoothing. Temporal mode also uses previous frames.", "0", 1, true, true,
  {{"Off","0"},{"Spatial","1"},{"Temporal","2"}}, {"1","1","0"}},
 {"r_smaa_quality", "SMAA quality", "Rend2 edge detection quality when SMAA is enabled.", "2", 1, true, true,
  {{"Low","0"},{"Medium","1"},{"High","2"},{"Ultra","3"}}, {"2","3","0"}},
 {"r_swapInterval", "VSync", "Synchronizes presentation to your display; may limit frame rate.", "0", 1, false, false,
  JAR_GFX_R_SWAPINTERVAL_CHOICES, {"0","1","0"}},
 {"com_maxfps", "Frame limit", "A frame cap can reduce power use. Unlimited may increase CPU load.", "125", 1, false, false,
  JAR_GFX_COM_MAXFPS_CHOICES, {"125","125","0"}},
 {"cg_drawFPS", "Show FPS", "Displays the current frame rate for comparing the same scene.", "0", 1, false, false,
  JAR_GFX_CG_DRAWFPS_CHOICES, {"1","1","1"}},
 {"r_dynamicGlow", "Dynamic glow", "Glow around bright materials such as lights and saber blades.", "0", 1, true, false,
  JAR_GFX_R_DYNAMICGLOW_CHOICES, {"0","1","0"}},
 {"r_hdr", "HDR lighting", "Rend2 high dynamic range lighting; this is not HDR monitor output.", "1", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_toneMap", "Tone mapping", "Maps Rend2 HDR lighting to the display range.", "1", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_autoExposure", "Auto exposure", "Adjusts Rend2 exposure to the brightness of the scene.", "1", 2, false, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_ssao", "Ambient occlusion", "Rend2 screen-space contact shading. Can be expensive at high resolution.", "0", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"0","1","0"}},
 {"r_sunShadows", "Sun shadows", "Rend2 shadow maps for sunlight in maps that support it.", "1", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_shadowMapSize", "Shadow resolution", "Rend2 sun-shadow detail. Higher values use more GPU time and memory.", "1024", 2, true, true,
  {{"512","512"},{"1024","1024"},{"2048","2048"},{"4096","4096"}}, {"1024","2048","512"}},
 {"r_volumetricFog", "Volumetric fog", "Rend2 light-grid shading inside fog volumes.", "0", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"0","1","0"}},
 {"r_drawSunRays", "Sun rays", "Rend2 sunlight shafts where the map provides a sun.", "0", 2, true, true,
  {{"Off","0"},{"On","1"}}, {"0","1","0"}},
 {"r_picmip", "Texture detail", "Lower detail saves texture memory; Apply reloads textures.", "0", 3, true, false,
  JAR_GFX_R_PICMIP_CHOICES, {"0","0","1"}},
 {"r_textureMode", "Texture filtering", "Trilinear filtering smooths changes between texture detail levels.", "GL_LINEAR_MIPMAP_NEAREST", 3, false, false,
  JAR_GFX_R_TEXTUREMODE_CHOICES, {"GL_LINEAR_MIPMAP_LINEAR","GL_LINEAR_MIPMAP_LINEAR","GL_LINEAR_MIPMAP_NEAREST"}},
 {"r_ext_texture_filter_anisotropic", "Anisotropic filter", "Improves oblique surfaces. Clamped to your GPU's supported level.", "16", 3, true, false,
  JAR_GFX_R_EXT_TEXTURE_FILTER_ANISOTROPIC_CHOICES, {"4","16","1"}},
 {"r_normalMapping", "Normal maps", "Rend2 surface detail where matching normal maps are available.", "1", 3, true, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_specularMapping", "Specular maps", "Rend2 material highlights; effects depend on the map's materials.", "1", 3, true, true,
  {{"Off","0"},{"On","1"}}, {"1","1","0"}},
 {"r_parallaxMapping", "Parallax mapping", "Rend2 apparent surface depth on materials with height information.", "0", 3, false, true,
  {{"Off","0"},{"On","1"}}, {"0","1","0"}},
 {"r_cubeMapping", "Cubemap reflections", "Rend2 reflections where probes/materials support them. May slow map loading.", "0", 3, true, true,
  {{"Off","0"},{"On","1"}}, {"0","1","0"}},
 {"r_dynamicGlowBloom", "Bloom strength", "Rend2 bloom from bright scene colors. Requires Dynamic glow enabled.", "0", 3, false, true,
  {{"Off","0"},{"Subtle","0.25"},{"Medium","0.5"},{"Strong","1"}}, {"0","0.25","0"}},
 {"r_lodbias", "Model detail", "Lower model detail reduces drawing work in crowded scenes.", "0", 4, false, false,
  JAR_GFX_R_LODBIAS_CHOICES, {"0","0","2"}},
 {"cg_shadows", "Character shadows", "Disable to reduce per-character rendering work in crowds.", "1", 4, false, false,
  JAR_GFX_CG_SHADOWS_CHOICES, {"1","1","0"}},
 {"r_dynamiclight", "Dynamic lights", "Weapon and effect lights. Turning them off can help effect-heavy scenes.", "1", 4, false, false,
  JAR_GFX_R_DYNAMICLIGHT_CHOICES, {"1","1","0"}},
 {"r_surfaceSprites", "Grass and foliage", "Controls decorative surface sprites; collision is unchanged.", "1", 4, false, false,
  JAR_GFX_R_SURFACESPRITES_CHOICES, {"1","1","0"}},
 {"cg_marks", "Impact marks", "Disables new cosmetic impact marks; existing marks fade normally.", "1", 4, false, false,
  JAR_GFX_CG_MARKS_CHOICES, {"1","1","0"}},
 {"r_fastsky", "Simple sky", "Skips detailed sky rendering for a cheaper background.", "0", 4, false, false,
  JAR_GFX_R_FASTSKY_CHOICES, {"0","0","1"}},
 {"jar_npcThinkLOD", "Distant idle NPC AI", "Optional: idle NPCs beyond 1024 units decide less often (up to 400 ms).", "0", 4, false, false,
  {{"Normal (default)","0"},{"Reduced","1"}}, {"0","0","1"}},
 {"r_subdivisions", "Curved surfaces", "Lower curve detail reduces world geometry. Apply reloads video.", "4", 4, true, false,
  JAR_GFX_R_SUBDIVISIONS_CHOICES, {"4","4","20"}},
 {"r_mode", "Resolution", "Desktop is detected automatically. Editing width or height selects Custom.", "4", 0, true, false,
  JAR_RESOLUTION_CHOICES, {nullptr,nullptr,nullptr}},
 {"r_customwidth", "Custom width", "320-16384 pixels; width times height must not exceed 64 megapixels.", "1600", 0, true, false,
  {}, {nullptr,nullptr,nullptr}, 320, 16384},
 {"r_customheight", "Custom height", "200-16384 pixels. Borderless fullscreen always uses the desktop size.", "1024", 0, true, false,
  {}, {nullptr,nullptr,nullptr}, 200, 16384},
 {"r_fullscreen", "Window mode", "Borderless fullscreen uses the desktop without changing the monitor video mode.", "0", 0, true, false,
  JAR_WINDOW_CHOICES, {nullptr,nullptr,nullptr}},
 {"r_noborder", "Window decoration", "Controls borders for ordinary windows; borderless fullscreen has no border.", "0", 0, true, false,
  JAR_BORDER_CHOICES, {nullptr,nullptr,nullptr}},
 {"cg_hudFiles", "HUD layout", "Academy gauges or OpenJK simple numbers. Custom HUD paths remain unchanged unless selected here.", "ui/jahud.txt", 5, true, false,
  JAR_HUD_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_fontPack", "Text appearance", "High-resolution glyphs retain original spacing. Alien and Asian glyphs retain retail artwork.", "sans", 5, true, false,
  JAR_FONT_PACK_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_uiPack", "UI artwork", "Upscales menus, HUD, icons and animation frames. 4x uses more texture memory.", "up2", 5, true, false,
  JAR_UI_PACK_CHOICES, {nullptr,nullptr,nullptr}},
 {"r_fontSharpness", "Font variant detail", "Selects higher-resolution variants when a font pack supplies them.", "1", 5, false, false,
  JAR_FONT_DETAIL_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_uiMotion", "UI motion", "Reduced freezes animated UI materials and removes menu orbit/transition motion.", "1", 5, false, false,
  JAR_UI_MOTION_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_cameraPreset", "Gameplay camera", "Legacy retains custom bob/chase values. Comfort reduces bob and chase delay. Crisp removes run/bob and chase delay. Scripted, zoom and vehicle cameras retain native behavior.", "0", 5, false, false,
  {{"Legacy / custom","0"},{"Comfort","1"},{"Crisp","2"}}, {nullptr,nullptr,nullptr}},
 {"jar_consoleStyle", "Background", "Clean console backgrounds use resolution-independent flat color.", "0", 6, false, false,
  JAR_CONSOLE_STYLE_CHOICES, {nullptr,nullptr,nullptr}},
 {"con_opacity", "Background opacity", "Opacity while the console overlays the game; full console remains opaque.", "1", 6, false, false,
  JAR_CONSOLE_FRACTION_CHOICES, {nullptr,nullptr,nullptr}},
 {"con_scale", "Console text size", "Scale console characters independently of menus and HUD.", "1", 6, false, false,
  JAR_CONSOLE_SCALE_CHOICES, {nullptr,nullptr,nullptr}},
 {"con_height", "Console height", "Percentage of the screen occupied by the open console.", "0.5", 6, false, false,
  JAR_CONSOLE_FRACTION_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_facePreset", "Facial expressions", "Speech-driven jaw, lips and brows. Stock restores native motion; Extreme intentionally stretches faces. Apply live, no restart.", "0", 7, false, false,
  JAR_FACE_PRESET_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_faceSensitivity", "Speech sensitivity", "Controls how strongly quieter dialogue moves the face. Independent of speaker/master volume; requires a facial preset.", "1", 7, false, false,
  JAR_FACE_SENSITIVITY_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_saberExtension", "Saber extension", "Quick extends the player's held energy blades in 250 ms; Legacy takes about 1 second. NPCs, swords, throws and scripted views retain native timing. Maximum reach stays unchanged.", "1", 7, false, false,
  JAR_SABER_EXTENSION_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_continuousFists", "Fist input", "Continuous repeats iknowkungfu primary punches at their animation cadence. Release finishes the current punch. Alt kicks, combined-button grabs and NPC behavior remain native.", "1", 7, false, false,
  JAR_FIST_INPUT_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_saberPack", "Saber blades", "Blade textures and trails; blade color, damage and reach are unchanged.", "stock", 7, true, false,
  JAR_SABER_PACK_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_soundPack", "Saber sounds", "Locally imported MBII sound sets. Missing individual sounds fall back to retail.", "stock", 7, true, false,
  JAR_SOUND_PACK_CHOICES, {nullptr,nullptr,nullptr}},
 {"jar_fxPack", "Combat effects", "Adapted weapon and saber effects. After Apply, reload map/save for all effects.", "stock", 7, true, false,
  JAR_FX_PACK_CHOICES, {nullptr,nullptr,nullptr}},

 {jar::pickupCvars[0], "Randomizer seed", "0-2147483647. Shared seed. Entity changes apply on fresh maps; sounds on playback; live cvars only on Apply live roll.", "1", 8, false, false,
  {}, {nullptr,nullptr,nullptr}, 0, jar::pickupSeedMaximum},
 {"jar_randomMode", "Randomizer mode", "Seed-fixed: each source type keeps its replacement. Reroll: vary events/spawns; active loops stay steady. Fresh maps apply entity changes; future shots/audio use the current seed.", "0", 8, false, false,
  {{"Seed-fixed mapping","0"},{"Per-event reroll","1"}}, {nullptr,nullptr,nullptr}},
 {jar::pickupCvars[1], "Ordinary weapons", "Randomize ordinary map weapons. Sabers and special-purpose weapons stay original.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {jar::pickupCvars[2], "Ordinary ammunition", "Shuffle blaster energy and metallic bolts. Power cells and special ammo stay original.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {jar::pickupCvars[3], "Health / shields", "Shuffle ordinary health and shield pickups. May change difficulty; scripted items stay original.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomWide", "Cross-category pickups", "Ordinary pickups become different guns, explosives, ammo or supplies. Can reduce supplies. Requires pickup toggles.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomEnemies", "NPC spawn types", "Choose from installed NPC templates at existing spawn locations. Mission safety protects named/scripted actors and allies.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomLoadout", "Bonus player arsenal", "Add 3 distinct seeded guns and ammo. Fixed mode keeps the same set across maps. Keeps sabers and mission tools; respects unarmed/training spawns.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomObjects", "Decorative props", "Swap trusted unlinked static props within original bounds. Collision, doors, objectives and scaled models stay original.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomProjectiles", "Projectile / shot variants", "Map each held weapon and fire mode to a shot variant, or reroll per shot. Includes charged, NPC/vehicle and beam attacks. Native ammo/cadence and detonation controls remain.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_npcSafety", "NPC mission safety", "ON protects named/scripted actors, allies and original bounds. OFF is experimental: any feasible nonvehicle template may replace them and can break missions.", "1", 8, false, false,
  {{"Experimental OFF","0"},{"On (recommended)","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_actorContinuity", "Named ally continuity", "Prevent unplanned lethal damage to eligible named allies while retaining their script tasks. Authored deaths/removals remain native. Does not resurrect missing actors.", "0", 8, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_actorRecovery", "Missing ally script recovery", "Experimental. Enable before spawning actors. Unplanned deaths/removals retain hidden noncombat script stand-ins. Ground humanoids only; story deaths stay final. Recovery saves require 0.8+.", "0", 8, false, false,
  {{"Off","0"},{"On (new actors)","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomEffects", "One-shot effects", "Map each source sound to supported installed audio: effects, voices, ambience, music or movie audio. Per-event mode visits a seeded catalog cycle.", "0", 9, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomUI", "Interface sounds", "Interface events draw from the same complete audio catalog. Logical silence/control commands retain their original meaning.", "0", 9, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomLoops", "Ambient / entity loops", "Loop any supported installed audio, including music and dialogue converted to PCM. Loop mappings stay stable while playing.", "0", 9, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomMusic", "Music", "Play a seeded PCM selection from the entire audio catalog as a loop. Honors silence; authored dynamic transition timing is replaced.", "0", 9, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomVoices", "Dialogue / movie audio", "Shuffle voices and embedded movie soundtracks from all audio. Keep original captions, script waits and movie timing. Short replacements finish with silence.", "0", 9, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomGravity", "Gravity", "Roll 75-100% of the original map gravity, bounded to 100-1200. Never increase jump difficulty. Restore keeps later script changes.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomSpeed", "Movement speed", "Roll 100-125% of original movement speed, capped at 425. Cinematic and Force time scaling stay under game control.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomBrightness", "Brightness", "Live scene brightness 0.85-1.25 in both renderers, including windowed mode. HUD/menu and your gamma preference stay readable.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomFov", "Field of view", "Live FOV 80-105. Scripted camera overrides stay under game control. No video restart.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomSky", "Detailed / simple sky", "Seeded detailed or simple sky in either renderer. Changes immediately without rebuilding the renderer.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},
 {"jar_randomLights", "Dynamic lighting", "Seeded weapon/effect lights On or Off in either renderer. HDR, SSAO, resolution and other restart settings are excluded.", "0", 10, false, false,
  {{"Off","0"},{"On","1"}}, {nullptr,nullptr,nullptr}},

};
static const size_t graphicsSettingCount = sizeof(graphicsSettings)/sizeof(graphicsSettings[0]);
inline bool GraphicsChoiceValid(const GraphicsSetting& setting, const char *value) {
    if (setting.maximum) return DecimalInRange(value, setting.minimum, setting.maximum);
    if (std::strstr(setting.name,"Pack") && SafePackId(value)) return true;
    for (const auto& choice : setting.choices)
        if (choice.value && !std::strcmp(choice.value, value)) return true;
    return false;
}
}
#endif
