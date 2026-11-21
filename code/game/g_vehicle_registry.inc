// Production vehicle-name registry serialization. GPL-2.0-only.
static int jarRestoredVehicleCount = 0;
static int jarRestoredVehicles[MAX_VEHICLES]{};
static bool jarRestoringVehicles = false;

static bool JarVehicleNameValid(const char* name)
{
    const char* end = static_cast<const char*>(std::memchr(name, 0, MAX_TOKEN_CHARS));
    if (!end || end == name) return false;
    for (const char* p = name; p != end; ++p)
        if (static_cast<unsigned char>(*p) < 32 || *p == 127) return false;
    return true;
}

static void JarWriteVehicleRegistry(int version)
{
    if (!jar::IsWorkshopSaveVersion(version) || version < jar::workshopSaveV2) return;
    ojk::SavedGameHelper h(gi.saved_game);
    const uint32_t count = numVehicles > 0 ? numVehicles - 1 : 0;
    if (count >= MAX_VEHICLES) gi.Error(ERR_DROP, "Invalid JA:R vehicle registry size");
    char names[MAX_VEHICLES][MAX_TOKEN_CHARS]{};
    for (uint32_t i = 0; i < count; ++i) {
        const char* name = g_vehicleInfo[i + 1].name;
        if (!name || std::strlen(name) >= MAX_TOKEN_CHARS) gi.Error(ERR_DROP, "Invalid JA:R vehicle registry name");
        Q_strncpyz(names[i], name, MAX_TOKEN_CHARS);
        if (!JarVehicleNameValid(names[i])) gi.Error(ERR_DROP, "Invalid JA:R vehicle registry name");
    }
    h.write_chunk<uint32_t>(INT_ID('J','V','C','T'), count);
    if (count) h.write_chunk<char>(INT_ID('J','V','N','M'), &names[0][0], count * MAX_TOKEN_CHARS);
}

static void JarReadVehicleRegistry(int version)
{
    if (!jar::IsWorkshopSaveVersion(version) || version < jar::workshopSaveV2) return;
    ojk::SavedGameHelper h(gi.saved_game);
    uint32_t count = 0;
    h.read_chunk<uint32_t>(INT_ID('J','V','C','T'), count);
    if (count >= MAX_VEHICLES) gi.Error(ERR_DROP, "Invalid JA:R saved vehicle registry size");
    char names[MAX_VEHICLES][MAX_TOKEN_CHARS]{};
    if (count) h.read_chunk<char>(INT_ID('J','V','N','M'), &names[0][0], count * MAX_TOKEN_CHARS);
    for (uint32_t i = 0; i < count; ++i) {
        if (!JarVehicleNameValid(names[i])) gi.Error(ERR_DROP, "Invalid JA:R saved vehicle registry name");
        for (uint32_t j = 0; j < i; ++j)
            if (!Q_stricmp(names[i], names[j])) gi.Error(ERR_DROP, "Duplicate JA:R saved vehicle registry name");
        const int index = BG_VehicleGetIndex(names[i]);
        if (index <= 0 || index >= numVehicles) gi.Error(ERR_DROP, "JA:R saved vehicle definition is unavailable");
        jarRestoredVehicles[i + 1] = index;
    }
    jarRestoredVehicleCount = count;
    jarRestoringVehicles = true;
}

