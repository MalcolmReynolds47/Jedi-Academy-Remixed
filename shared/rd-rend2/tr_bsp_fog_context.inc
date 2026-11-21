// BSP-owned fog, GPU constants and light-grid coordinate transforms. GPL-2.0-only.
world_t *R_BSPFogWorld(const trRefEntity_t *entity)
{
#ifdef REND2_SP
    if (entity && entity->riftFogRoom) entity = entity->riftFogRoom;
    else if (entity == &tr.worldEntity && backEnd.riftFogCapRoom) entity = backEnd.riftFogCapRoom;
#endif
    if (!entity || entity == &tr.worldEntity || entity->e.reType != RT_MODEL)
        return tr.world;
    const model_t *model = R_GetModelByHandle(entity->e.hModel);
    if (model && model->type == MOD_BRUSH && model->data.bmodel)
    {
        const int index = model->data.bmodel->worldIndex;
        if (index >= 0 && index < tr.numBspModels) return tr.bspModels[index];
    }
    return tr.world;
}

#ifdef REND2_SP
const fog_t *R_RiftCameraFog(const trRefEntity_t *room)
{
    if (!room) return nullptr;
    const auto *world = R_BSPFogWorld(room);
    return world && world != tr.world ? world->globalFog : nullptr;
}
#endif

static const world_t *R_BSPFogGridWorld(const trRefEntity_t *entity, bool global)
{
    const world_t *world = R_BSPFogWorld(entity);
    if (global && world && world->inheritedFog > 0) return tr.world;
    return world;
}

int R_BSPGlobalFogIndex(const trRefEntity_t *entity)
{
    const world_t *world = R_BSPFogWorld(entity);
    return world ? world->globalFogIndex : -1;
}

fog_t R_BSPFog(const trRefEntity_t *entity, int index)
{
    const world_t *world = R_BSPFogWorld(entity);
    if (!world || !world->fogs || index <= 0 || index >= world->numfogs) return fog_t{};
    if (index == world->inheritedFog && tr.world && tr.world->globalFog)
        return *tr.world->globalFog;
    fog_t fog = world->fogs[index];
    if (world == tr.world || !fog.hasSurface) return fog;
#ifdef REND2_SP
    if (entity->riftFogRoom) entity = entity->riftFogRoom;
    else if (entity == &tr.worldEntity && backEnd.riftFogCapRoom) entity = backEnd.riftFogCapRoom;
#endif
    vec3_t normal = {};
    for (int i = 0; i < 3; ++i)
    {
        const float lengthSquared = DotProduct(entity->e.axis[i], entity->e.axis[i]);
        if (lengthSquared <= 0.000001f) return fog_t{};
        VectorMA(normal, fog.surface[i] / lengthSquared, entity->e.axis[i], normal);
    }
    const float length = VectorLength(normal);
    if (length <= 0.000001f) return fog_t{};
    fog.surface[3] = (fog.surface[3] + DotProduct(normal, entity->e.origin)) / length;
    VectorScale(normal, 1.0f / length, fog.surface);
    return fog;
}

static void R_BSPFogGridMatrix(const trRefEntity_t *entity, bool global, matrix_t matrix)
{
    memset(matrix, 0, sizeof(matrix_t));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    const world_t *world = R_BSPFogGridWorld(entity, global);
    if (!world || !world->lightGridData || !world->volumetricLightMaps[0]) return;
#ifdef REND2_SP
    if (entity && entity->riftFogRoom) entity = entity->riftFogRoom;
    else if (entity == &tr.worldEntity && backEnd.riftFogCapRoom) entity = backEnd.riftFogCapRoom;
#endif
    // Column-major world-to-grid-cell matrix. Texture dimensions are applied
    // by the shader, so the same transform supports every loaded grid size.
    for (int row = 0; row < 3; ++row)
    {
        vec3_t inverseAxis = {};
        float translation = 0.0f;
        if (world != tr.world)
        {
            const float lengthSquared = DotProduct(entity->e.axis[row], entity->e.axis[row]);
            if (lengthSquared <= 0.000001f) continue;
            VectorScale(entity->e.axis[row], 1.0f / lengthSquared, inverseAxis);
            translation = -DotProduct(inverseAxis, entity->e.origin);
        }
        else inverseAxis[row] = 1.0f;
        for (int column = 0; column < 3; ++column)
            matrix[column * 4 + row] = inverseAxis[column] * world->lightGridInverseSize[row];
        matrix[12 + row] = (translation - world->lightGridOrigin[row] +
            0.5f * world->lightGridSize[row]) * world->lightGridInverseSize[row];
    }
}

image_t *RB_BSPFogLightGrid(bool global)
{
    const world_t *world = R_BSPFogGridWorld(backEnd.currentEntity, global);
    if (backEnd.refdef.doLAGoggles || !world || !world->lightGridData || !world->volumetricLightMaps[0])
        return tr.whiteImage3D;
    return world->volumetricLightMaps[0];
}

long RB_BSPFogUboOffset()
{
#ifdef REND2_SP
    if (backEnd.riftFogCapRoom) return backEnd.riftFogCapRoom->bspFogUboOffset;
#endif
    if (!backEnd.refdef.doLAGoggles && backEnd.currentEntity &&
        R_BSPFogWorld(backEnd.currentEntity) != tr.world)
        return backEnd.currentEntity->bspFogUboOffset;
    return tr.fogsUboOffset;
}

static void R_BuildBSPFogsBlock(FogsBlock &block, const trRefEntity_t *entity, bool goggles)
{
    block = {};
    const world_t *world = R_BSPFogWorld(entity);
    block.globalFogIndex = world && world->globalFogIndex > 0 ? world->globalFogIndex - 1 : -1;
    R_BSPFogGridMatrix(entity, false, block.worldToLightGrid);
    R_BSPFogGridMatrix(entity, true, block.worldToGlobalLightGrid);
    block.numFogs = world ? world->numfogs - 1 : 0;
    if (block.numFogs < 0 || block.numFogs + (goggles ? 1 : 0) > MAX_GPU_FOGS)
        Com_Error(ERR_DROP, "JA:R Rend2: BSP fog constant capacity exhausted");
    for (int i = 0; i < block.numFogs; ++i)
    {
        const fog_t fog = R_BSPFog(entity, i + 1);
        FogsBlock::Fog &value = block.fogs[i];
        VectorCopy4(fog.surface, value.plane);
        VectorCopy4(fog.color, value.color);
        const float depth = fog.parms.depthForOpaque < 1.0f ? 1.0f : fog.parms.depthForOpaque;
        const world_t *fogWorld = i + 1 == world->inheritedFog ? tr.world : world;
        value.depthToOpaque = r_volumetricFog->integer ?
            (-logf(1.5f / 255.0f)) / depth * fogWorld->volumetricFogScale * r_volumetricFogScale->value :
            sqrtf(-logf(1.0f / 255.0f)) / depth;
        value.hasPlane = fog.hasSurface;
    }
    if (goggles)
    {
        FogsBlock::Fog &value = block.fogs[block.numFogs++];
        VectorSet4(value.color, 0.75f, 0.42f + Q_flrand(0.0f, 1.0f) * 0.025f, 0.07f, 1.0f);
        value.depthToOpaque = r_volumetricFog->integer ?
            -logf(1.0f / 255.0f) / 700.0f : sqrtf(-logf(1.0f / 255.0f)) / 700.0f;
    }
}

static void RB_UpdateFogsConstants(gpuFrame_t *frame, const trRefdef_t *refdef)
{
    FogsBlock block;
    R_BuildBSPFogsBlock(block, &tr.worldEntity, refdef->doLAGoggles != 0);
    if (refdef->doLAGoggles && tr.world)
        VectorCopy4(block.fogs[block.numFogs - 1].color, tr.world->fogs[tr.world->numfogs].color);
    tr.fogsUboOffset = RB_AppendConstantsData(frame, &block, sizeof(block));
    for (int i = 0; i < refdef->num_entities; ++i)
    {
        trRefEntity_t *entity = &refdef->entities[i];
        entity->bspFogUboOffset = tr.fogsUboOffset;
        if (refdef->doLAGoggles || R_BSPFogWorld(entity) == tr.world) continue;
#ifdef REND2_SP
        if (entity->riftFogRoom) continue;
#endif
        R_BuildBSPFogsBlock(block, entity, false);
        entity->bspFogUboOffset = RB_AppendConstantsData(frame, &block, sizeof(block));
    }
#ifdef REND2_SP
    if (!refdef->doLAGoggles)
        for (int i = 0; i < refdef->num_entities; ++i)
            if (const auto *room = refdef->entities[i].riftFogRoom)
                refdef->entities[i].bspFogUboOffset = room->bspFogUboOffset;
#endif
}
