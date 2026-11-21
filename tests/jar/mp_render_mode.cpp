// Exercise native image lookup with the client renderer imports, without GL.
#include "rd-vanilla/tr_local.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>

refimport_t ri{};
glconfig_t glConfig{};
static cvar_t actualMode{};
cvar_t* com_dedicated = &actualMode;
static int legacyMode, loads, creates, frees;
static bool cached, missing;
static image_t testImage{};

int Cvar_VariableIntegerValue(const char* name) {
    if (!Q_stricmp(name, "dedicated")) return legacyMode;
    assert(!strcmp(name, "r_picmip"));
    return 3;
}
#include "../../codemp/client/cl_jar_renderer_mode.inc"

static image_t* R_FindImageFile_NoLoad(const char*, qboolean, qboolean, qboolean, int wrap) {
    assert(wrap == GL_CLAMP_TO_EDGE);
    return cached ? &testImage : nullptr;
}
void R_LoadImage(const char*, byte** pic, int* width, int* height) {
    ++loads;
    *width = *height = 4;
    *pic = missing ? nullptr : static_cast<byte*>(calloc(4 * 4, 4));
}
image_t* R_CreateImage(const char*, const byte* pic, int width, int height,
                      GLenum format, qboolean, qboolean, qboolean, int wrap, bool rectangle) {
    assert(pic && width == 4 && height == 4 && format == GL_RGBA);
    assert(wrap == GL_CLAMP_TO_EDGE && !rectangle);
    ++creates;
    return &testImage;
}
void Z_Free(void* p) { assert(p); ++frees; free(p); }
#include "../../codemp/rd-vanilla/tr_image_lookup.inc"

static image_t* Find(const char* name) {
    return R_FindImageFile(name, qtrue, qtrue, qtrue, GL_CLAMP);
}
int main() {
    glConfig.clampToEdgeAvailable = qtrue;
    // Reproduce the reported LAN failure through the original import binding.
    legacyMode = 1;
    ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
    assert(!Find("textures/map/wall") && !loads && !creates);

    // The graphical client remains graphical regardless of a stale UI/config flag.
    ri.Cvar_VariableIntegerValue = CL_RendererCvarInteger;
    for (legacyMode = 0; legacyMode <= 2; ++legacyMode) {
        assert(CL_RendererCvarInteger("DeDiCaTeD") == 0);
        assert(CL_RendererCvarInteger("r_picmip") == 3);
        for (const char* name : {"textures/map/wall", "gfx/hud/health", "fonts/arial"}) {
            const int before = loads;
            assert(Find(name) == &testImage && loads == before + 1);
            assert(creates == loads && frees == loads);
        }
    }
    const int before = loads;
    cached = true;
    assert(Find("textures/map/wall") == &testImage && loads == before);
    cached = false;
    missing = true;
    assert(!Find("missing/image") && loads == before + 1 && creates == before);
    assert(!Find(nullptr) && loads == before + 1);
    missing = false;

    // Preserve genuine dedicated mode behavior and the server's direct import.
    for (actualMode.integer = 1; actualMode.integer <= 2; ++actualMode.integer) {
        assert(!Find("textures/map/wall") && loads == before + 1);
    }
    ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
    for (legacyMode = 1; legacyMode <= 2; ++legacyMode) {
        assert(!Find("textures/map/wall") && loads == before + 1);
    }
    puts("PASS: reproduced LAN image suppression; graphical mode loads world/HUD/font images with stale dedicated 0/1/2; cache, missing files and dedicated behavior preserved.");
}
