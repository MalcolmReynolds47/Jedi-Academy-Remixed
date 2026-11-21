// The optional Rend2 launch is separate from the archived renderer setting.
static constexpr char JAR_MPClassic[] = "rdjar-mp-vanilla";
static constexpr char JAR_MPRend2[] = "rdjar-mp-rend2";

static refexport_t* CL_JAR_TryRenderer(const char* renderer, refimport_t* imports, void** library) {
    char dllName[MAX_OSPATH];
    Com_sprintf(dllName, sizeof(dllName), "%s_" ARCH_STRING DLL_EXT, renderer);
    void* candidate = Sys_LoadDll(dllName, qfalse);
    if (!candidate) return nullptr;
    auto getAPI = reinterpret_cast<GetRefAPI_t>(Sys_LoadFunction(candidate, "GetRefAPI"));
    refexport_t* api = getAPI ? getAPI(REF_API_VERSION, imports) : nullptr;
    if (!api) {
        Sys_UnloadDll(candidate);
        return nullptr;
    }
    *library = candidate;
    return api;
}

static refexport_t* CL_JAR_LoadRenderer(const char* configured, bool rend2, refimport_t* imports,
                                        void** library, bool* fallback) {
    const char* renderer = rend2 ? JAR_MPRend2 : configured;
    *fallback = false;
    if (auto* api = CL_JAR_TryRenderer(renderer, imports, library)) return api;
    if (!strcmp(renderer, JAR_MPClassic)) return nullptr;
    *fallback = true;
    Com_Printf("Renderer %s unavailable or incompatible; trying Classic.\n", renderer);
    return CL_JAR_TryRenderer(JAR_MPClassic, imports, library);
}
