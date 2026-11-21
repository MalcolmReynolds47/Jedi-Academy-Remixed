// Original JA:R native presentation constants. GPL-2.0-only.
#pragma once
#include <cmath>
#include <algorithm>
#include "../../shared/qcommon/jar_ui_palette.h"
#include "../../shared/qcommon/jar_ui_cursor.h"
namespace jar { namespace ui {
constexpr float background[4]=JAR_UI_BACKGROUND;
constexpr float panel[4]=JAR_UI_PANEL;
constexpr float edge[4]=JAR_UI_EDGE;
constexpr float text[4]=JAR_UI_TEXT;
constexpr float muted[4]=JAR_UI_MUTED;
constexpr float accent[4]=JAR_UI_ACCENT;
struct CursorBox {float x,y,w,h;};
inline CursorBox MenuCursor(float x,float y,float xscale,float yscale){
    const auto box=JAR_MenuCursor(x,y,xscale,yscale);return {box.x,box.y,box.w,box.h};
}
// Native LCARS stages are coarse milestones, not a measured byte percentage.
inline int LoadingStage(int stage) { return stage<0 ? 0 : stage>9 ? 9 : stage; }
inline float LoadingWidth(int stage) { return 576.0f*LoadingStage(stage)/9.0f; }
// Describes the actual CG_RegisterSounds / CG_RegisterGraphics phases.
inline const char* LoadingExplanation(int stage){
    static const char* const phases[]={
        "Preparing collision, music and ambient sound",
        "Preparing pickup and equipment sounds",
        "Loading sounds requested by this mission",
        "Building the level's visible geometry",
        "Preparing surface materials and visual effects",
        "Loading shared game models",
        "Preparing the HUD, items and moving map geometry",
        "Loading objects used by this mission",
        "Preparing character models, skins and animations",
        "Finishing terrain and additional map sections"
    };
    return phases[LoadingStage(stage)];
}
} }
