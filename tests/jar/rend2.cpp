#include "tr_local.h"
#include "tr_allocator.h"
#include "test_support.h"
#include <vector>
#include <memory>
#include <limits>

trGlobals_t tr;
backEndData_t *backEndData;
int r_firstSceneDrawSurf,r_numdlights,r_firstSceneDlight,r_numentities,r_firstSceneEntity,r_numpolys,r_firstScenePoly,r_numpolyverts;
#include "tr_frame.inc"
refimport_t ri = {};
glRefConfig_t glRefConfig = {};
void *R_Malloc(int size, unsigned int, qboolean) { return std::calloc(1, size); }

int main() {
    return RunTests("Rend2 high IDs, full-width sorting, bounded surfaces and aligned buffers", [] {
        auto scene = std::make_unique<backEndData_t>(); backEndData = scene.get();
        auto world = std::make_unique<world_t>(); tr.world = world.get();
        tr.world->skyboxportal = 1;
        R_InitNextFrame(); CHECK(tr.world->skyboxportal == 0);
        tr.world = nullptr; R_InitNextFrame(); // UI with no loaded world
        shader_t shader = {}; shader.sortedIndex = MAX_SHADERS-1;
        tr.sortedShaders[shader.sortedIndex] = &shader;
        std::vector<drawSurf_t> surfaces(MAX_DRAWSURFS);
        tr.refdef.drawSurfs = surfaces.data();
        surfaceType_t mesh = SF_ENTITY;
        for (int ent : {0, 1023, 16383, MAX_REFENTITIES-1, REFENTITYNUM_WORLD}) {
            for (int post : {0, 1}) {
                int index = tr.refdef.numDrawSurfs;
                R_AddDrawSurf(&mesh, ent, &shader, 7, 3, post, 63);
                int actual, cube, actualPost; shader_t *actualShader;
                R_DecomposeSort(surfaces[index].sort, &actual, &actualShader, &cube, &actualPost);
                CHECK(actual == ent && actualShader == &shader && cube == 63 && actualPost == post);
            }
        }
        tr.refdef.numDrawSurfs = MAX_DRAWSURFS;
        const auto first = surfaces[0].sort;
        R_AddDrawSurf(&mesh, 2, &shader, 0, 0, 0, 0);
        CHECK(tr.refdef.numDrawSurfs == MAX_DRAWSURFS && surfaces[0].sort == first);
        for (size_t i = 0; i < 4096; ++i)
            surfaces[i].sort = (uint64_t(i%2)<<63) | R_CreateSortKey(4095-i, (i*31)%MAX_SHADERS, i%64, i%2);
        R_RadixSort(surfaces.data(), 4096);
        for (int i = 1; i < 4096; ++i) CHECK(surfaces[i-1].sort <= surfaces[i].sort);

        ResetGhoul2RenderableSurfaceHeap();
        std::vector<mdxmSurface_t> meshes(JAR_GHOUL2_RENDER_SURFACES);
        std::vector<CRenderableSurface*> submitted;
        for (auto& data : meshes) {
            auto *surface = AllocGhoul2RenderableSurface();
            CHECK(surface && !surface->surfaceData && !surface->boneCache);
            surface->surfaceData = &data;
            submitted.push_back(surface);
        }
        for (int i = 0; i < 1024; ++i) CHECK(AllocGhoul2RenderableSurface() == nullptr);
        for (size_t i = 0; i < meshes.size(); ++i) CHECK(submitted[i]->surfaceData == &meshes[i]);
        ResetGhoul2RenderableSurfaceHeap();
        CHECK(AllocGhoul2RenderableSurface() == submitted[0] && !submitted[0]->surfaceData);

        alignas(16) unsigned char memory[32] = {};
        Allocator allocator(memory, 31, 16);
        CHECK(allocator.Alloc(15) == memory);
        bool exhausted = false;
        try { allocator.Alloc(15); } catch (const std::runtime_error&) { exhausted = true; }
        CHECK(exhausted && allocator.Mark() == memory+16);
        allocator.Reset();
        exhausted = false;
        try { allocator.Alloc(std::numeric_limits<size_t>::max()); } catch (const std::runtime_error&) { exhausted = true; }
        CHECK(exhausted && allocator.Mark() == memory);

        auto frame = std::make_unique<gpuFrame_t>();
        glRefConfig.uniformBufferOffsetAlignment = 16;
        frame->uboMemory[0] = memory;
        frame->uboSize[0] = sizeof(memory);
        const unsigned char data[15] = {42};
        CHECK(RB_AppendConstantsData(frame.get(), data, sizeof(data)) == 0);
        CHECK(RB_AppendConstantsData(frame.get(), data, sizeof(data)) == 16);
        CHECK(memory[0] == 42 && memory[16] == 42);
        exhausted = false;
        try { RB_AppendConstantsData(frame.get(), data, 1); } catch (const std::runtime_error&) { exhausted = true; }
        CHECK(exhausted && frame->uboWriteOffset[0] == 32);
        frame->currentScene = MAX_SCENES;
        exhausted = false;
        try { RB_AppendConstantsData(frame.get(), data, 1); } catch (const std::runtime_error&) { exhausted = true; }
        CHECK(exhausted);
    });
}
