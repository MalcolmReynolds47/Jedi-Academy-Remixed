// Classic SP/MP fog ownership and instance transforms. GPL-2.0-only.
// Read the entity being drawn, never replace the renderer's main world pointer.
static world_t *R_ResolveBSPFogWorld(const trRefEntity_t *ent)
{
    if (ent && ent->riftFogRoom) ent = ent->riftFogRoom;
    if (!ent || ent == &tr.worldEntity || ent->e.reType != RT_MODEL)
        return tr.world;
    const model_t *model = R_GetModelByHandle(ent->e.hModel);
    if (model && model->type == MOD_BRUSH && model->bmodel)
    {
        const int index = model->bmodel->bspIndex;
        if (index > 0 && index <= tr.numBSPModels)
            return &tr.bspModels[index - 1];
    }
    return tr.world;
}

static world_t *RB_BSPFogWorld()
{
#ifdef JAR_CLASSIC_SP
    if (backEnd.refdef.doLAGoggles) return tr.world;
#endif
    return R_ResolveBSPFogWorld(backEnd.currentEntity);
}

const fog_t *R_RiftCameraFog(const trRefEntity_t *room)
{
    if (!room) return nullptr;
    const auto *world = R_ResolveBSPFogWorld(room);
    return world && world != tr.world && world->globalFog > 0 && world->globalFog < world->numfogs ?
        &world->fogs[world->globalFog] : nullptr;
}

bool RB_BSPFogIsGlobal(int index, bool includeSpecial)
{
    const world_t *world = RB_BSPFogWorld();
    return world && index > 0 &&
        (index == world->globalFog || (includeSpecial && index == world->numfogs));
}

fog_t RB_BSPFog(int index)
{
    const world_t *world = RB_BSPFogWorld();
    // Both worlds reserve numfogs for the special fog; no arbitrary index reads.
    if (!world || !world->fogs || index < 0 || index > world->numfogs)
        return fog_t{};
    if (world != tr.world && index == world->inheritedFog && tr.world &&
        tr.world->globalFog > 0 && tr.world->globalFog < tr.world->numfogs)
        return tr.world->fogs[tr.world->globalFog]; // Live main-world transition.
    fog_t fog = world->fogs[index];
    if (world == tr.world || index == 0) return fog;

    const auto *room = backEnd.currentEntity->riftFogRoom;
    const refEntity_t &ent = room ? room->e : backEnd.currentEntity->e;
    if (fog.hasSurface)
    {
        // Inverse transpose handles the orthogonal scaled axes of refEntity_t.
        vec3_t normal = {};
        for (int i = 0; i < 3; ++i)
        {
            const float lengthSquared = DotProduct(ent.axis[i], ent.axis[i]);
            if (lengthSquared <= 0.000001f) return fog_t{};
            VectorMA(normal, fog.surface[i] / lengthSquared, ent.axis[i], normal);
        }
        const float length = VectorLength(normal);
        if (length <= 0.000001f) return fog_t{};
        fog.surface[3] = (fog.surface[3] + DotProduct(normal, ent.origin)) / length;
        VectorScale(normal, 1.0f / length, fog.surface);
    }
    vec3_t bounds[2];
    ClearBounds(bounds[0], bounds[1]);
    for (int corner = 0; corner < 8; ++corner)
    {
        vec3_t point;
        VectorCopy(ent.origin, point);
        for (int i = 0; i < 3; ++i)
            VectorMA(point, fog.bounds[(corner >> i) & 1][i], ent.axis[i], point);
        AddPointToBounds(point, bounds[0], bounds[1]);
    }
    VectorCopy(bounds[0], fog.bounds[0]);
    VectorCopy(bounds[1], fog.bounds[1]);
    return fog;
}
