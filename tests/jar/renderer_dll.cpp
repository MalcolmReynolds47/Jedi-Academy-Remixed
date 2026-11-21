// Load the built renderer without creating a window or using game assets.
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "rd-common/tr_public.h"
#include "test_support.h"
#include <windows.h>
#include <vector>
#include <set>

static void QDECL Print(int, const char *, ...) {}
static unsigned int diagnosticEvents=0;
static void Log(int,const char *,const char *,const char *,...) { ++diagnosticEvents; }
int main(int argc, char **argv) {
    return RunTests("native renderer API and actual Ghoul2 slot exhaustion/recovery", [&] {
        CHECK(argc == 2);
        HMODULE module = LoadLibraryA(argv[1]);
        CHECK(module != nullptr);
        auto getAPI = reinterpret_cast<GetRefAPI_t>(GetProcAddress(module, "GetRefAPI"));
        CHECK(getAPI);
        refimport_t imports = {};
        imports.Printf = Print;
        imports.Error = Com_Error;
        imports.Z_Malloc = Z_Malloc;
        imports.Z_Free = Z_Free;
        imports.JAR_Log = Log;
        CHECK(getAPI(1020, &imports) == nullptr);
        CHECK(getAPI(1022, &imports) == nullptr);
        const refexport_t *api = getAPI(REF_API_VERSION, &imports);
        CHECK(api && api->TheGhoul2InfoArray && api->AddPolyToScene && api->AddPortalToScene && api->SetReflectionProbes && api->inPVS && api->G2API_JAR_ModelBounds && api->G2API_JAR_ModelGeometry);
        const int probeStatus = api->SetReflectionProbes(nullptr,0);
        CHECK(probeStatus == jar::reflection::NoWorld || probeStatus == jar::reflection::Unsupported);
        auto& pool = api->TheGhoul2InfoArray();
        std::vector<int> handles;
        std::set<int> unique;
        for (int i = 0; i < (1<<JAR_GHOUL2_BITS); ++i) {
            int handle = pool.New();
            CHECK(handle > 0 && pool.IsValid(handle));
            CHECK(unique.insert(handle).second);
            handles.push_back(handle);
        }
        bool exhausted = false;
        try { pool.New(); } catch (const std::runtime_error& error) {
            exhausted = std::strstr(error.what(), "capacity exhausted") != nullptr;
        }
        CHECK(exhausted);
        // Real model-vector storage, including growth and nested allocations,
        // exercises the pool's diagnostic callbacks without commercial assets.
        auto& models=pool.Get(handles.front());
        models.resize(1);
        models[0].mBltlist.resize(3);
        models.resize(2);
        for (int handle : handles) { pool.Delete(handle); CHECK(!pool.IsValid(handle)); }
        for (int i = 0; i < 65536; ++i) { int handle = pool.New(); CHECK(pool.IsValid(handle)); pool.Delete(handle); }
        CHECK(diagnosticEvents>handles.size());
        CHECK(FreeLibrary(module));
    });
}
