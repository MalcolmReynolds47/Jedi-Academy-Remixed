// Scene-local light-grid ownership. No retained entity pointers. GPL-2.0-only.
#include <cmath>
#include <vector>
struct bspGridInstance_t
{
    world_t *world;
    vec3_t origin, axis[3], inverse[3];
    double volume;
};
static std::vector<bspGridInstance_t> bspGridInstances;

void R_ClearBSPGridInstances() { bspGridInstances.clear(); }

static bool R_ValidBSPGrid(const world_t *world)
{
    return world && world->lightGridData && world->lightGridArray &&
        world->numGridDataElements > 0 && world->numGridArrayElements > 0;
}

void R_BuildBSPGridInstances(const trRefdef_t *scene)
{
    R_ClearBSPGridInstances();
    if (!scene || !scene->entities || (scene->rdflags & RDF_NOWORLDMODEL)) return;
    for (int i = 0; i < scene->num_entities; ++i)
    {
        const refEntity_t &entity = scene->entities[i].e;
        if (entity.reType != RT_MODEL) continue;
        const model_t *model = R_GetModelByHandle(entity.hModel);
        if (!model || model->type != MOD_BRUSH) continue;
        world_t *world = nullptr;
#ifdef JAR_GRID_REND2
        const bmodel_t *brush = model->data.bmodel;
        if (brush && brush->worldIndex >= 0 && brush->worldIndex < tr.numBspModels)
            world = tr.bspModels[brush->worldIndex];
#else
        const bmodel_t *brush = model->bmodel;
        if (brush && brush->bspIndex > 0 && brush->bspIndex <= tr.numBSPModels)
            world = &tr.bspModels[brush->bspIndex - 1];
#endif
        // Only a whole map section defines a lighting volume; its moving doors
        // must not create independent copies of the same map's lighting space.
        if (!R_ValidBSPGrid(world) || brush != world->bmodels) continue;
        bspGridInstance_t instance{};
        instance.world = world;
        instance.volume = 1;
        bool valid = true;
        for (int axis = 0; axis < 3; ++axis)
        {
            const float lengthSquared = DotProduct(entity.axis[axis], entity.axis[axis]);
            if (!std::isfinite(lengthSquared) || lengthSquared <= 0.000001f || !std::isfinite(entity.origin[axis]))
            { valid = false; break; }
            VectorCopy(entity.axis[axis], instance.axis[axis]);
            VectorScale(entity.axis[axis], 1.0f / lengthSquared, instance.inverse[axis]);
            instance.origin[axis] = entity.origin[axis];
            instance.volume *= (brush->bounds[1][axis] - brush->bounds[0][axis]) * sqrt(double(lengthSquared));
            for (int other = 0; other < axis; ++other)
                if (fabs(DotProduct(entity.axis[axis], entity.axis[other])) >
                    0.001f * sqrt(lengthSquared * DotProduct(entity.axis[other], entity.axis[other]))) valid = false;
        }
        if (valid && std::isfinite(instance.volume) && instance.volume > 0) bspGridInstances.push_back(instance);
    }
}

static const bspGridInstance_t *R_BSPGridAtPoint(const vec3_t point, vec3_t local)
{
    const bspGridInstance_t *selected = nullptr;
    VectorCopy(point, local);
    for (const auto &instance : bspGridInstances)
    {
        vec3_t delta, candidate;
        VectorSubtract(point, instance.origin, delta);
        bool inside = true;
        for (int axis = 0; axis < 3; ++axis)
        {
            candidate[axis] = DotProduct(delta, instance.inverse[axis]);
            if (!std::isfinite(candidate[axis]) || candidate[axis] < instance.world->bmodels[0].bounds[0][axis] ||
                candidate[axis] > instance.world->bmodels[0].bounds[1][axis]) inside = false;
        }
        // Most specific containing volume wins; equal volumes retain scene order.
        if (inside && (!selected || instance.volume < selected->volume))
        { selected = &instance; VectorCopy(candidate, local); }
    }
    return selected;
}

// Clamp coordinates before choosing both corners, preventing row/slice wrapping
// at edges and supporting one-cell dimensions without out-of-range pointers.
static bool R_BSPGridCorners(const world_t *world, const vec3_t point, int indices[8], float factors[8])
{
    if (!R_ValidBSPGrid(world)) return false;
    int low[3], high[3]; float fraction[3];
    for (int axis = 0; axis < 3; ++axis)
    {
        const double coordinate = (double(point[axis]) - world->lightGridOrigin[axis]) * world->lightGridInverseSize[axis];
        if (!std::isfinite(coordinate) || world->lightGridBounds[axis] < 1) return false;
        const double clamped = coordinate < 0 ? 0 : (coordinate > world->lightGridBounds[axis] - 1 ? world->lightGridBounds[axis] - 1 : coordinate);
        low[axis] = int(floor(clamped)); high[axis] = low[axis] + (low[axis] + 1 < world->lightGridBounds[axis] ? 1 : 0);
        fraction[axis] = float(clamped - low[axis]);
    }
    for (int corner = 0; corner < 8; ++corner)
    {
        int position[3]; float factor = 1;
        for (int axis = 0; axis < 3; ++axis)
        {
            const bool upper = (corner & (1 << axis)) != 0;
            position[axis] = upper ? high[axis] : low[axis];
            factor *= upper ? fraction[axis] : 1 - fraction[axis];
        }
        const int index = position[0] + world->lightGridBounds[0] * (position[1] + world->lightGridBounds[1] * position[2]);
        if (index < 0 || index >= world->numGridArrayElements || world->lightGridArray[index] >= world->numGridDataElements) return false;
        indices[corner] = index; factors[corner] = factor;
    }
    return true;
}
