// Native MP model hull cache with a two-frame MD3 and Ghoul2 renderer fixture.
// GPL-2.0-only.
#include "qcommon/jar_mp_model_hull.h"
#include "rd-common/tr_public.h"
#include "ghoul2/g2_local.h"
#include "qcommon/qfiles.h"
#include "test_support.h"
#include <algorithm>
#include <cmath>
#include <vector>

using jar::physics::PropGeometry;
using jar::physics::Vec;

namespace {
struct TestModel {
    md3Header_t header{};
    md3Surface_t surface{};
    md3Triangle_t faces[4]{};
    md3XyzNormal_t vertices[10]{};
};
TestModel model;
int opens, reads, closes, poses;
bool malformed, failPose;
refexport_t renderer{};
alignas(CGhoul2Info_v) unsigned char ghoulStorage[sizeof(CGhoul2Info_v)]{};

void MakeModel() {
    model.header.ident = MD3_IDENT;
    model.header.version = MD3_VERSION;
    model.header.numFrames = 2;
    model.header.numSurfaces = 1;
    model.header.ofsSurfaces = offsetof(TestModel, surface);
    model.header.ofsEnd = sizeof(model);
    model.surface.ident = MD3_IDENT;
    model.surface.numFrames = 2;
    model.surface.numVerts = 5;
    model.surface.numTriangles = 4;
    model.surface.ofsEnd = sizeof(model) - model.header.ofsSurfaces;
    model.surface.ofsXyzNormals = offsetof(TestModel, vertices) - model.header.ofsSurfaces;
    model.surface.ofsTriangles = offsetof(TestModel, faces) - model.header.ofsSurfaces;
    const int faceIndices[][3] = {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3}};
    const short points[5][3] = {{0, 0, 0}, {640, 0, 0}, {640, 320, 0}, {640, 0, 640},
                                {32000, 32000, 32000}};
    for (int triangle = 0; triangle < 4; ++triangle)
        std::copy_n(faceIndices[triangle], 3, model.faces[triangle].indexes);
    for (int frame = 0; frame < 2; ++frame) {
        for (int vertex = 0; vertex < 5; ++vertex) {
            auto& target = model.vertices[5 * frame + vertex];
            std::copy_n(points[vertex], 3, target.xyz);
            if (frame && vertex < 4) target.xyz[0] += 640;
        }
    }
}

void CountPoints(void* data, const float* point) {
    static_cast<std::vector<Vec>*>(data)->push_back({point[0], point[1], point[2]});
}
}

refexport_t* re = nullptr;

long FS_FOpenFileRead(const char* name, fileHandle_t* handle, qboolean unique) {
    CHECK(!strcmp(name, "models/hull.md3") && unique);
    ++opens;
    *handle = 1;
    return sizeof(model);
}
int FS_Read(void* out, int length, fileHandle_t handle) {
    CHECK(handle == 1 && length == sizeof(model));
    ++reads;
    memcpy(out, &model, sizeof(model));
    if (malformed) static_cast<TestModel*>(out)->faces[0].indexes[0] = 99;
    return length;
}
void FS_FCloseFile(fileHandle_t handle) {
    CHECK(handle == 1);
    ++closes;
}
void QDECL Com_Printf(const char*, ...) {}
void QDECL Com_Error(int, const char*, ...) { throw std::runtime_error("unexpected fatal error"); }

int main() {
    return RunTests("MP model collision geometry, cache isolation and renderer sampling", [] {
        MakeModel();
        std::vector<Vec> vertices;
        CHECK(JAR_MP_VisitModelHull("Models\\Hull.MD3", 0, &vertices, CountPoints));
        CHECK(vertices.size() == 4 && opens == 1 && reads == 1 && closes == 1);
        PropGeometry reconstructed;
        for (auto point : vertices) {
            CHECK(point.x <= 10 && point.y <= 10 && point.z <= 10);
            reconstructed.Add(point);
        }
        CHECK(reconstructed.RigidHull());

        const vec3_t angles{0, 0, 0}, turned{0, 90, 0};
        const auto* server = JAR_MP_TraceHull("models/hull.md3", 0, angles, 90);
        CHECK(server && opens == 1);
        // Both test segments are inside the old 10x10x10 AABB in Y and Z.
        CHECK(server->Trace({15, 7, 7}, {0, 7, 7}, {}, {}).fraction == 1);
        CHECK(server->Trace({15, 2, 2}, {0, 2, 2}, {}, {}).fraction < 1);
        CHECK(server->Trace({15, 3, 5.5f}, {0, 3, 5.5f}, {}, {}).fraction == 1);
        const auto* prediction = JAR_MP_TraceHull("models/hull.md3", 0, turned, 90, true);
        CHECK(prediction && prediction != server && opens == 1);
        CHECK(prediction->Trace({-2, 15, 2}, {-2, 0, 2}, {}, {}).fraction < 1);
        CHECK(server->Trace({15, 2, 2}, {0, 2, 2}, {}, {}).fraction < 1);
        const auto* posed = JAR_MP_TraceHull("models/hull.md3", 1, angles, 90);
        CHECK(posed && posed->Trace({25, 2, 2}, {0, 2, 2}, {}, {}).fraction < .3f && opens == 2);

        CHECK(!JAR_MP_TraceHull("models/hull.glm", 0, angles));
        re = &renderer;
        renderer.G2API_InitGhoul2Model = [](CGhoul2Info_v** instance, const char* name,
            int, qhandle_t, qhandle_t, int, int) {
            CHECK(!strcmp(name, "models/hull.glm"));
            *instance = reinterpret_cast<CGhoul2Info_v*>(ghoulStorage);
            return 0;
        };
        renderer.G2API_CleanGhoul2Models = [](CGhoul2Info_v** instance) { *instance = nullptr; };
        renderer.G2API_SkinlessModel = [](CGhoul2Info_v&, int) { return qtrue; };
        static int clientSkins, serverSkins, appliedSkin;
        renderer.RegisterSkin = [](const char* name) {
            CHECK(!strcmp(name, "models/model_default.skin"));
            ++clientSkins;
            return 1;
        };
        renderer.RegisterServerSkin = [](const char* name) {
            CHECK(!strcmp(name, "models/model_default.skin"));
            ++serverSkins;
            return 2;
        };
        renderer.G2API_SetSkin = [](CGhoul2Info_v&, int index, qhandle_t custom, qhandle_t render) {
            CHECK(index == 0 && custom == render);
            appliedSkin = render;
            return qtrue;
        };
        renderer.G2API_JAR_PoseGeometry = [](CGhoul2Info_v&, int frame, int*, vec3_t, vec3_t,
            void* context, void (*emit)(void*, const float*)) {
            ++poses;
            CHECK(frame >= 0 && frame <= 2);
            const float points[][3] = {{0, 0, 0}, {10, 0, 0}, {10, 5, 0}, {10, 0, 10}};
            if (frame == 1) {
                for (int i = 0; i <= JarSweepHull::modelPoints; ++i) {
                    const float interior[] = {5, i / 100.f, 1};
                    emit(context, interior);
                }
            }
            for (const auto& point : points) emit(context, point);
            return failPose ? qfalse : qtrue;
        };
        // An uncached client trace can run before the prop's first draw.
        // It must load visible materials into the shared client skin cache.
        const auto* skeletal = JAR_MP_TraceHull("models/hull.glm", 0, angles, 91, true);
        CHECK(clientSkins == 1 && serverSkins == 0 && appliedSkin == 1);
        CHECK(skeletal && poses == 1 && skeletal->Trace({15, 7, 7}, {0, 7, 7}, {}, {}).fraction == 1);
        CHECK(skeletal->Trace({15, 3, 5.5f}, {0, 3, 5.5f}, {}, {}).fraction == 1);
        const auto* support = JAR_MP_TraceHull("models/hull.glm", 1, angles, 92);
        CHECK(clientSkins == 1 && serverSkins == 1 && appliedSkin == 2);
        CHECK(support && support->Trace({15, 3, 5.5f}, {0, 3, 5.5f}, {}, {}).fraction < 1);
        vertices.clear();
        CHECK(JAR_MP_VisitModelHull("models/hull.glm", 1, &vertices, CountPoints));
        CHECK(vertices.size() == JarSweepHull::modelPoints + 1 + 26);
        PropGeometry solver;
        for (const auto point : vertices) solver.Add(point);
        jar::physics::NativePropHull solverTrace;
        CHECK(solverTrace.Build(solver, {}));
        CHECK(solverTrace.Trace({15, 3, 5.5f}, {0, 3, 5.5f}, {}, {}).fraction ==
              support->Trace({15, 3, 5.5f}, {0, 3, 5.5f}, {}, {}).fraction);
        failPose = true;
        CHECK(!JAR_MP_TraceHull("models/hull.glm", 2, angles));
        CHECK(!JAR_MP_TraceHull("models/hull.glm", 2, angles) && poses == 3);

        JAR_MP_ClearModelHulls();
        malformed = true;
        CHECK(!JAR_MP_TraceHull("models/hull.md3", 0, angles) && opens == 3);
        CHECK(!JAR_MP_TraceHull("models/hull.md3", 0, angles) && opens == 3);
        JAR_MP_ClearModelHulls();
        malformed = false;
        CHECK(JAR_MP_TraceHull("models/hull.md3", 0, angles) && opens == 4 && closes == opens);
        re = nullptr;
    });
}
