#include "rd-vanilla/tr_local.h"
#include "test_support.h"
#include <memory>
#include <vector>

trGlobals_t tr;
backEndData_t *backEndData;
refimport_t ri = {};
glconfig_t glConfig = {};
cvar_t *r_dynamiclight, *r_norefresh, *r_vertexLight;
int recursivePortalCount;
// Lighting is outside this surface-storage fixture; BSP-grid tests cover it.
void R_ClearBSPGridInstances() {}
void R_BuildBSPGridInstances(const trRefdef_t *) { throw std::runtime_error("unexpected scene lighting"); }
void R_RenderView(viewParms_t *) { throw std::runtime_error("unexpected GPU rendering"); }
void RE_RenderWorldEffects() { throw std::runtime_error("unexpected world FX"); }
void R_AddDrawSurf(const surfaceType_t *, const shader_t *, int, int) { throw std::runtime_error("unexpected draw submission"); }
shader_t *R_GetShaderByHandle(int) { throw std::runtime_error("unexpected shader lookup"); }
model_t *R_GetModelByHandle(int) { throw std::runtime_error("unexpected model lookup"); }
qboolean R_FogParmsMatch(int, int) { throw std::runtime_error("unexpected fog lookup"); }
qhandle_t RE_RegisterShader(const char *name) {
    if (!strcmp(name, "gfx/colors/black")) return 2;
    for (const char* material : jar::portal::monitorMaterials)
        if (!strcmp(name, material)) return 1;
    throw std::runtime_error("unexpected shader registration");
}
extern int r_numpolys,r_numpolyverts;

int main()
{
    return RunTests("skeletal surface lifetime across crowds, views, exhaustion and frame reuse", [] {
        auto frame = std::make_unique<backEndData_t>();
        backEndData = frame.get();
        skyboxportal = drawskyboxportal = 1;
        RE_ClearScene(); CHECK(skyboxportal && drawskyboxportal);
        R_InitNextFrame();
        CHECK(!skyboxportal && !drawskyboxportal);
        // 1,024 actors, 16 parts each, each submitted for a model and a shadow.
        const int capacity = 2048 * JAR_CAPACITY_MULTIPLIER;
        std::vector<mdxmSurface_t> meshes(capacity);
        std::vector<CRenderableSurface*> submitted;
        for (int i = 0; i < capacity; ++i) {
            if (i % 4096 == 0) RE_ClearScene(); // UI, portal and other views share a frame.
            auto *surface = AllocRS();
            CHECK(surface != nullptr);
            CHECK(surface->surfaceData == nullptr && surface->boneCache == nullptr);
            surface->surfaceData = &meshes[i];
            submitted.push_back(surface);
        }
        for (int i = 0; i < capacity; ++i) CHECK(submitted[i]->surfaceData == &meshes[i]);
        for (int i = 0; i < 1024; ++i) CHECK(AllocRS() == nullptr);
        for (int i = 0; i < capacity; ++i) CHECK(submitted[i]->surfaceData == &meshes[i]);
        // Only the next frame can reclaim surfaces retained by the backend.
        R_InitNextFrame();
        auto *reused = AllocRS();
        CHECK(reused == submitted[0]);
        CHECK(reused->surfaceData == nullptr && reused->boneCache == nullptr);
#ifdef _G2_GORE
        CHECK(reused->goreChain == nullptr && reused->alternateTex == nullptr);
#endif
        // Exercise the real scene producer/storage, including multiple scenes
        // in one frame and reused ordinary-poly slots without camera metadata.
        tr.registered=qtrue;ri.Printf=[](int,const char*,...){};
        jar::portal::View portal;AnglesToAxis(vec3_origin,portal.axis);AnglesToAxis(vec3_origin,portal.cameraAxis);portal.cameraOrigin[0]=400;
        RE_AddPortalToScene(&portal);CHECK(r_numpolys==1&&backEndData->numJarPortals==1);
        const auto *first=backEndData->polys[0].jarPortal;CHECK(first&&first!=&portal&&first->cameraOrigin[0]==400);
        RE_ClearScene();portal.cameraOrigin[0]=800;RE_AddPortalToScene(&portal);
        CHECK(r_numpolys==2&&backEndData->numJarPortals==2&&first->cameraOrigin[0]==400&&backEndData->polys[1].jarPortal->cameraOrigin[0]==800);
        r_numpolyverts=MAX_POLYVERTS-4;RE_AddPortalToScene(&portal);CHECK(r_numpolys==2&&backEndData->numJarPortals==2);
        R_InitNextFrame();CHECK(!r_numpolys&&!backEndData->numJarPortals);polyVert_t vertices[4]{};
        RE_AddPolyToScene(2,4,vertices,1);CHECK(r_numpolys==1&&!backEndData->polys[0].jarPortal&&!backEndData->polys[0].jarFallback&&!backEndData->polys[0].jarSelected&&!backEndData->polys[0].jarMonitorShader);
    });
}
