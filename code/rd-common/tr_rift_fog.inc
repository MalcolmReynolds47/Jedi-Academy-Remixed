// Scene-local fog for qualified portable BSPs.
// The renderer uses its own BSP leaf tree and its rendered transform.
#include "../game/g_bsp_section_rift.h"
#include "../game/g_bsp_section_korriban.h"
#include "../game/g_bsp_section_coruscant.h"
#include "../game/g_bsp_section_wasteland.h"
#include <cmath>
#include <vector>

namespace {
constexpr int allVisibleAreas=-2;
struct RiftFogRoom {
    const trRefEntity_t *entity;
    const bmodel_t *brush;
    const world_t *world;
    int fogIndex;
    int area;
};
std::vector<RiftFogRoom> riftFogRooms;

const RiftFogRoom *R_FindRiftFogRoom(const vec3_t point)
{
    for (const auto &room : riftFogRooms)
    {
        vec3_t offset, local;
        VectorSubtract(point, room.entity->e.origin, offset);
        bool inside = true;
        for (int i = 0; i < 3; ++i)
        {
            local[i] = DotProduct(offset, room.entity->e.axis[i]);
            if (!(local[i] >= room.brush->bounds[0][i] && local[i] <= room.brush->bounds[1][i]))
                inside = false;
        }
        if (!inside) continue;
        const mnode_t *node = room.world->nodes;
        for (int depth = 0; node && depth <= room.world->numDecisionNodes; ++depth)
        {
            if (node->contents != CONTENTS_NODE)
            {
                if (node->cluster >= 0 && (room.area == allVisibleAreas || node->area == room.area)) return &room;
                break;
            }
            if (!node->plane) break;
            node = node->children[DotProduct(local, node->plane->normal) < node->plane->dist];
        }
    }
    return nullptr;
}
}

void R_AssignRiftFog(trRefdef_t& scene, int firstEntity)
{
    riftFogRooms.clear();
    for (int i = 0; i < scene.num_entities; ++i)
    {
        auto& entity = scene.entities[i];
        entity.riftFogRoom = nullptr;
        entity.riftFogIndex = 0;
        if (i < firstEntity || (scene.rdflags & (RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_NOFOG)) ||
            entity.e.reType != RT_MODEL || !(entity.e.renderfx & RF_JAR_SECTION_FOG)) continue;
        const model_t *model = R_GetModelByHandle(entity.e.hModel);
        if (!model || model->type != MOD_BRUSH) continue;
#if defined(REND2_SP) || defined(JAR_REND2_MP)
        const auto *brush = model->data.bmodel;
        const int index = brush ? brush->worldIndex : -1;
        const world_t *world = index >= 0 && index < tr.numBspModels ? tr.bspModels[index] : nullptr;
        const int fog = world ? world->globalFogIndex : 0;
#else
        const auto *brush = model->bmodel;
        const int index = brush ? brush->bspIndex : 0;
        const world_t *world = index > 0 && index <= tr.numBSPModels ? &tr.bspModels[index - 1] : nullptr;
        const int fog = world ? world->globalFog : 0;
#endif
        if (!world || world == tr.world || !world->nodes || brush != world->bmodels || !world->fogs ||
            fog <= 0 || fog >= world->numfogs) continue;
#ifndef JAR_REND2_MP
        if (fog == world->inheritedFog) continue;
#endif
        const int area = !Q_stricmp(world->name,jar::rift::path)?jar::rift::arenaArea:
                         !Q_stricmp(world->name,jar::korriban::path)?jar::korriban::arenaArea:
                         (!Q_stricmp(world->name,jar::coruscant::path)||
                          !Q_stricmp(world->name,jar::wasteland::path))?allVisibleAreas:-1;
        if (area == -1) continue;
        bool validAxes = true;
        for (int axis = 0; axis < 3; ++axis)
        {
            const float length = DotProduct(entity.e.axis[axis], entity.e.axis[axis]);
            if (!(std::fabs(length - 1.0f) < 0.001f)) validAxes = false;
            for (int previous = 0; previous < axis; ++previous)
                if (!(std::fabs(DotProduct(entity.e.axis[axis], entity.e.axis[previous])) < 0.001f))
                    validAxes = false;
        }
        if (!validAxes) continue;
        riftFogRooms.push_back({&entity, brush, world, fog, area});
    }
    if (riftFogRooms.empty()) return;
    for (int i = firstEntity; i < scene.num_entities; ++i)
    {
        auto& entity = scene.entities[i];
        if (entity.e.reType == RT_MODEL && entity.e.hModel > 0)
        {
            const auto *model = R_GetModelByHandle(entity.e.hModel);
            if (model && model->type == MOD_BRUSH) continue;
        }
        const float *point = entity.e.renderfx & RF_LIGHTING_ORIGIN ? entity.e.lightingOrigin : entity.e.origin;
        if (const auto *room = R_FindRiftFogRoom(point))
        {
            entity.riftFogRoom = room->entity;
            entity.riftFogIndex = room->fogIndex;
        }
    }
}

const trRefEntity_t *R_RiftFogRoomAtPoint(const vec3_t point)
{
    const auto *room = R_FindRiftFogRoom(point);
    return room ? room->entity : nullptr;
}

