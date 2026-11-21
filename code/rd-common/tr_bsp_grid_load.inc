// BSP-owned light grids shared by classic SP/MP and Rend2. GPL-2.0-only.
#include <cmath>
#include <string>

static void R_ReadBSPGridSize(world_t *world, const lump_t *entities)
{
    VectorSet(world->lightGridSize, 64, 64, 128);
    if (entities->fileofs < 0 || entities->filelen < 0)
        Com_Error(ERR_DROP, "JA:R: invalid BSP entity lump");
    const std::string text(reinterpret_cast<const char *>(fileBase + entities->fileofs), entities->filelen);
    const char *cursor = text.c_str();
#if defined(JAR_CLASSIC_SP) || defined(REND2_SP)
    COM_BeginParseSession();
#else
    COM_BeginParseSession("BSP light-grid size");
#endif
    const char *token = COM_ParseExt(&cursor, qtrue);
    if (token[0] == '{')
    {
        for (;;)
        {
            token = COM_ParseExt(&cursor, qtrue);
            if (!token[0] || token[0] == '}') break;
            const bool grid = !Q_stricmp(token, "gridsize");
            token = COM_ParseExt(&cursor, qtrue);
            if (!token[0] || token[0] == '}') break;
            if (grid && sscanf(token, "%f %f %f", &world->lightGridSize[0],
                &world->lightGridSize[1], &world->lightGridSize[2]) != 3)
                VectorClear(world->lightGridSize); // Rejected by the grid loader.
        }
    }
#if defined(JAR_CLASSIC_SP) || defined(REND2_SP)
    COM_EndParseSession();
#endif
}

static void R_LoadBSPGrid(world_t *world, const lump_t *samples, const lump_t *array)
{
    world->lightGridData = nullptr;
    world->lightGridArray = nullptr;
    world->numGridDataElements = world->numGridArrayElements = 0;
#ifdef JAR_GRID_REND2
    world->hdrLightGrid = nullptr;
#endif
    if (samples->fileofs < 0 || samples->filelen < 0 || array->fileofs < 0 || array->filelen < 0 ||
        samples->filelen % sizeof(mgrid_t) || array->filelen % sizeof(unsigned short))
        Com_Error(ERR_DROP, "JA:R: malformed BSP light-grid lumps in %s", world->name);
    if (!samples->filelen && !array->filelen) return; // Valid unlit map.
    const int count = samples->filelen / sizeof(mgrid_t);
    if (!world->bmodels || count < 1 || count > MAX_MAP_LIGHTGRID)
        Com_Error(ERR_DROP, "JA:R: invalid BSP light-grid sample count in %s", world->name);
    int cells = 1;
    for (int axis = 0; axis < 3; ++axis)
    {
        const double size = world->lightGridSize[axis];
        const double low = world->bmodels[0].bounds[0][axis], high = world->bmodels[0].bounds[1][axis];
        if (!std::isfinite(size) || size <= 0 || !std::isfinite(low) || !std::isfinite(high) || low > high)
            Com_Error(ERR_DROP, "JA:R: invalid BSP light-grid dimensions in %s", world->name);
        const double first = ceil(low / size), last = floor(high / size);
        const double length = last - first + 1;
        if (!std::isfinite(length) || length < 1 || length > MAX_MAP_LIGHTGRID_ARRAY / cells ||
            !std::isfinite(float(first * size)) || !std::isfinite(float(1.0 / size)))
            Com_Error(ERR_DROP, "JA:R: BSP light-grid capacity exceeded in %s", world->name);
        world->lightGridBounds[axis] = int(length);
        world->lightGridOrigin[axis] = float(first * size);
        world->lightGridInverseSize[axis] = float(1.0 / size);
        cells *= int(length);
    }
    if (array->filelen != cells * int(sizeof(unsigned short)))
        Com_Error(ERR_DROP, "JA:R: BSP light-grid array size mismatch in %s", world->name);
    for (int i = 0; i < count; ++i)
    {
        mgrid_t sample;
        memcpy(&sample, fileBase + samples->fileofs + i * sizeof(sample), sizeof(sample));
        for (int style = 0; style < MAXLIGHTMAPS && sample.styles[style] != LS_LSNONE; ++style)
            if (sample.styles[style] >= MAX_LIGHT_STYLES)
                Com_Error(ERR_DROP, "JA:R: BSP light-grid style out of range in %s", world->name);
    }
    // Validate every compressed lookup before exposing it to CPU or GPU consumers.
    for (int i = 0; i < cells; ++i)
    {
        unsigned short index;
        memcpy(&index, fileBase + array->fileofs + i * sizeof(index), sizeof(index));
        if (static_cast<unsigned short>(LittleShort(index)) >= count)
            Com_Error(ERR_DROP, "JA:R: BSP light-grid sample index out of range in %s", world->name);
    }
#ifdef JAR_CLASSIC_SP
    world->lightGridData = (mgrid_t *)R_Hunk_Alloc(samples->filelen, qfalse);
    world->lightGridArray = (unsigned short *)R_Hunk_Alloc(array->filelen, qfalse);
#else
    world->lightGridData = (mgrid_t *)Hunk_Alloc(samples->filelen, h_low);
    world->lightGridArray = (unsigned short *)Hunk_Alloc(array->filelen, h_low);
#endif
    memcpy(world->lightGridData, fileBase + samples->fileofs, samples->filelen);
    memcpy(world->lightGridArray, fileBase + array->fileofs, array->filelen);
    world->numGridDataElements = count;
    world->numGridArrayElements = cells;
    for (int i = 0; i < cells; ++i) world->lightGridArray[i] = LittleShort(world->lightGridArray[i]);
    for (int i = 0; i < count; ++i)
        for (int style = 0; style < MAXLIGHTMAPS; ++style)
        {
#ifdef JAR_CLASSIC_SP
            R_ColorShiftLightingBytes(world->lightGridData[i].ambientLight[style]);
            R_ColorShiftLightingBytes(world->lightGridData[i].directLight[style]);
#else
            R_ColorShiftLightingBytes(world->lightGridData[i].ambientLight[style], world->lightGridData[i].ambientLight[style]);
            R_ColorShiftLightingBytes(world->lightGridData[i].directLight[style], world->lightGridData[i].directLight[style]);
#endif
        }
#ifdef JAR_GRID_REND2
    if (r_hdr->integer)
    {
        char directory[MAX_QPATH], filename[MAX_QPATH];
        COM_StripExtension(world->name, directory, sizeof(directory));
        Com_sprintf(filename, sizeof(filename), "%s/lightgrid.raw", directory);
        float *raw = nullptr;
        const int bytes = ri.FS_ReadFile(filename, (void **)&raw);
        if (raw)
        {
            const int expected = cells * 6 * int(sizeof(float));
            bool valid = bytes == expected;
            for (int i = 0; valid && i < cells * 6; ++i)
                valid = std::isfinite(LittleFloat(raw[i])) && LittleFloat(raw[i]) >= 0;
            if (!valid)
            {
                ri.FS_FreeFile(raw);
                Com_Error(ERR_DROP, "JA:R: invalid HDR light grid %s", filename);
            }
            world->hdrLightGrid = (float *)Hunk_Alloc(bytes, h_low);
            for (int i = 0; i < cells * 6; ++i) world->hdrLightGrid[i] = LittleFloat(raw[i]);
            ri.FS_FreeFile(raw);
        }
    }
#endif
}
