// Run the native MP renderer handshake without creating a game or GL window.
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "qcommon/qfiles.h"
#include <climits>
#include "rd-common/tr_public.h"
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

static const char* paths[2];
static bool unavailable, missingEntry, wrongABI;
static int unloads;
static std::vector<std::string> attempts;
static void QDECL Print(int, const char*, ...) {}
static int reads, frees, bspCase;
static dheader_t badHeader;
static long ReadBSP(const char* name, void** buffer) {
    assert(!strcmp(name, "maps/mp/ffa3.bsp"));
    ++reads;
    if (!bspCase) { *buffer = nullptr; return -1; }
    *buffer = &badHeader;
    return bspCase == 1 ? sizeof(badHeader) - 1 : sizeof(badHeader);
}
static void FreeBSP(void* buffer) { assert(buffer == &badHeader); ++frees; }
static void* UnexpectedHunkAlloc(int, ha_pref) { assert(false); return nullptr; }

static refimport_t ri{};
static char textAllocation[8];
static void* TextAlloc(int length, ha_pref preference) {
    assert(length == 1 || length == 5);
    assert(preference == h_low);
    return textAllocation;
}
#include "../../codemp/rd-rend2/tr_bsp_entity_text.inc"

static void EntityTextCase() {
    // Place the nonterminated lump against an inaccessible page. A C-string
    // copy from the original bytes would read outside the supplied extent.
    SYSTEM_INFO sys{};
    GetSystemInfo(&sys);
    auto* region = static_cast<byte*>(VirtualAlloc(nullptr, 2 * sys.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    assert(region);
    DWORD protection;
    assert(VirtualProtect(region + sys.dwPageSize, sys.dwPageSize, PAGE_NOACCESS, &protection));
    byte* bytes = region + sys.dwPageSize - 4;
    memcpy(bytes, "{}  ", 4);
    ri.Hunk_Alloc = TextAlloc;
    memset(textAllocation, '!', sizeof(textAllocation));
    assert(!strcmp(R_CopyBSPEntityText(bytes, 4), "{}  "));
    assert(textAllocation[5] == '!');
    assert(!R_CopyBSPEntityText(bytes, INT_MAX));
    assert(!R_CopyBSPEntityText(bytes, -1));
    assert(!*R_CopyBSPEntityText(nullptr, 0));
    assert(VirtualFree(region, 0, MEM_RELEASE));
}
void QDECL Com_Printf(const char*, ...) {}

void* QDECL Sys_LoadDll(const char* name, qboolean useSystemLib) {
    assert(!useSystemLib);
    attempts.emplace_back(name);
    const bool preview = !strcmp(name, "rdjar-mp-rend2_" ARCH_STRING DLL_EXT);
    if (preview && unavailable) return nullptr;
    return LoadLibraryA(paths[preview ? 1 : 0]);
}
void Sys_UnloadDll(void* handle) { ++unloads; assert(FreeLibrary(static_cast<HMODULE>(handle))); }

static GetRefAPI_t realGet;
static refexport_t* QDECL WrongABI(int version, refimport_t* imports) { return realGet(version - 1, imports); }
static void* Lookup(void* library, const char* name) {
    if (missingEntry && attempts.back().find("rdjar-mp-rend2_") == 0) return nullptr;
    void* raw = (void*)GetProcAddress(static_cast<HMODULE>(library), name);
    if (wrongABI && raw && attempts.back().find("rdjar-mp-rend2_") == 0) {
        realGet = reinterpret_cast<GetRefAPI_t>(raw);
        return reinterpret_cast<void*>(WrongABI);
    }
    return raw;
}
#undef Sys_LoadFunction
#define Sys_LoadFunction Lookup
#include "../../codemp/client/cl_jar_renderer_loader.inc"

int QDECL Com_sprintf(char* dest, int size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int count = vsnprintf(dest, size, fmt, args);
    va_end(args);
    return count;
}

int main(int argc, char** argv) {
    assert(argc == 3);
    paths[0] = argv[1];
    paths[1] = argv[2];
    refimport_t imports{};
    imports.Printf = Print;
    imports.FS_ReadFile = ReadBSP;
    imports.FS_FreeFile = FreeBSP;
    imports.Hunk_Alloc = UnexpectedHunkAlloc;
    EntityTextCase();
    for (int mode = 0; mode != 5; ++mode) {
        attempts.clear();
        unloads = 0;
        unavailable = mode == 2;
        missingEntry = mode == 3;
        wrongABI = mode == 4;
        void* library = nullptr;
        bool fallback = false;
        auto* api = CL_JAR_LoadRenderer(JAR_MPClassic, mode != 0, &imports, &library, &fallback);
        assert(api && library);
        assert(fallback == (mode >= 2));
        assert(attempts.size() == (fallback ? 2 : 1));
        assert(attempts.front() == (mode ? "rdjar-mp-rend2_" ARCH_STRING DLL_EXT : "rdjar-mp-vanilla_" ARCH_STRING DLL_EXT));
        if (fallback) assert(attempts.back() == "rdjar-mp-vanilla_" ARCH_STRING DLL_EXT);
        assert(unloads == (mode >= 3 ? 1 : 0));
        assert(api->Shutdown && api->BeginRegistration && api->LoadWorld && api->SetWorldVisData);
        assert(api->RegisterModel && api->RegisterServerModel && api->RegisterFont && api->ext.Font_StrLenPixels);
        assert(api->RenderScene && api->AddPolyToScene && api->AddRefEntityToScene);
        assert(api->InitializeWireframeAutomap && api->AutomapElevationAdjustment);
        assert(api->G2API_JAR_PoseBounds && api->G2API_JAR_PoseGeometry && api->G2API_JAR_ApplyFace &&
               api->G2API_InitGhoul2Model && api->G2API_CleanGhoul2Models);
        assert(api->SetReflectionProbes);
        const jarProbe_t probe={41,1,{32,64,128},512};
        assert(api->SetReflectionProbes(&probe,1)==(mode==1?JAR_PROBE_NO_WORLD:JAR_PROBE_UNSUPPORTED));
        if(mode==1){
            const jarProbe_t duplicate[]={probe,probe};
            assert(api->SetReflectionProbes(duplicate,2)==JAR_PROBE_INVALID);
            assert(api->SetReflectionProbes(nullptr,0)==JAR_PROBE_NO_WORLD);
        }
        api->G2API_SetTime(321, 0);
        assert(api->G2API_GetTime(0) == 321);
        if (mode == 1) {
            badHeader.ident = BSP_IDENT;
            badHeader.version = BSP_VERSION;
            badHeader.lumps[LUMP_ENTITIES].fileofs = sizeof(badHeader) - 1;
            badHeader.lumps[LUMP_ENTITIES].filelen = 2;
            for (bspCase = 0; bspCase <= 2; ++bspCase) {
                assert(!api->RegisterModel("#mp/ffa3:1"));
            }
            assert(reads == 3 && frees == 2);
        }
        Sys_UnloadDll(library);
    }
    puts("PASS: native Classic and Rend2 renderer ABI; missing library, missing export and old ABI fall back to Classic. Invalid BSP reads are refused and entity text is bounded.");
}
