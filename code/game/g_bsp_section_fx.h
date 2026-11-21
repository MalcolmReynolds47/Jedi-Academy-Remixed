// Stock Rancor Pit flame adapter. Include after g_local.h. GPL-2.0-only.
#ifndef G_BSP_SECTION_FX_H
#define G_BSP_SECTION_FX_H
namespace jar { namespace bspSectionFX {
constexpr const char* effect="env/small_fire";
constexpr const char* effectFile="effects/env/small_fire.efx";
constexpr const char* soundSet="yavintorch_lp";
constexpr const char* loop="sound/effects/torch_burning_lp.wav";
constexpr const char* loopIndex="sound/effects/torch_burning_lp";
constexpr int damageFlag=4;
constexpr const char* dust="chunks/dustFall.efx";
constexpr const char* dustFile="effects/chunks/dustfall.efx";
constexpr int dustFlag=2;
} }
bool G_BSPSectionInitializeFX(gentity_t&,const vec3_t,const gentity_t* aim=nullptr);
#endif
