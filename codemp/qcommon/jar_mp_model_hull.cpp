// Model/frame collision shared by the MP server, client prediction and Box3D.
// GPL-2.0-only.
#include "jar_mp_model_hull.h"
#include "qcommon.h"
#include "qfiles.h"
#include "../rd-common/tr_public.h"
#include "../../shared/qcommon/jar_md3_surface.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

extern refexport_t* re;

namespace {
using jar::physics::NativePropHull;
using jar::physics::PropGeometry;

constexpr size_t modelFrameLimit = 8192;
constexpr size_t traceCacheLimit = 4096;

struct ModelHull {
    PropGeometry geometry;
    // Keep the original model point giving each support maximum. Sending
    // these to Box3D preserves the same support values without copying a C++
    // allocation across the DLL boundary or refitting a generated hull.
    PropGeometry::SupportPoints witnesses{};
};

struct Collector {
    ModelHull& model;
    bool valid = true;

    static void Emit(void* context, const float* point) {
        auto& collector = *static_cast<Collector*>(context);
        for (int axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(point[axis]) || std::fabs(point[axis]) > 8192.f) {
                collector.valid = false;
                return;
            }
        }
        collector.model.geometry.Add({point[0], point[1], point[2]}, &collector.model.witnesses);
    }
};

bool LoadMD3(const char* model, int frame, Collector& collector) {
    fileHandle_t file = 0;
    const long length = FS_FOpenFileRead(model, &file, qtrue);
    if (!file) return false;
    std::vector<byte> bytes;
    constexpr int maxModelBytes = 64 * 1024 * 1024;
    if (length >= long(sizeof(md3Header_t)) && length <= maxModelBytes) {
        bytes.resize(size_t(length));
        if (FS_Read(bytes.data(), int(length), file) != length) bytes.clear();
    }
    FS_FCloseFile(file);
    if (bytes.empty()) return false;

    const auto& header = *reinterpret_cast<const md3Header_t*>(bytes.data());
    const int frames = LittleLong(header.numFrames);
    if (LittleLong(header.ident) != MD3_IDENT || LittleLong(header.version) != MD3_VERSION ||
        frames < 1 || frames > 65536 || frame >= frames || LittleLong(header.ofsEnd) > length)
        return false;

    return jar::md3::VisitSurfaces(header, frame, frames,
        [&](const md3XyzNormal_t* vertices, const md3Triangle_t* faces, int triangles) {
            std::array<bool, MD3_MAX_VERTS> seen{};
            for (int triangle = 0; triangle < triangles; ++triangle) {
                for (const int rawIndex : faces[triangle].indexes) {
                    const int index = LittleLong(rawIndex);
                    if (seen[index]) continue;
                    seen[index] = true;
                    vec3_t point;
                    jar::md3::Vertex(vertices[index], point);
                    Collector::Emit(&collector, point);
                }
            }
        });
}

bool LoadGhoul(const char* model, int frame, Collector& collector, bool client) {
    if (!re || !re->G2API_JAR_PoseGeometry) return false;
    CGhoul2Info_v* instance = nullptr;
    const bool loaded = re->G2API_InitGhoul2Model(&instance, model, 0, 0, 0, 0, 0) >= 0 && instance;
    if (!loaded) {
        if (instance) re->G2API_CleanGhoul2Models(&instance);
        return false;
    }

    if (re->G2API_SkinlessModel(*instance, 0)) {
        char skin[MAX_QPATH];
        Q_strncpyz(skin, model, sizeof(skin));
        if (char* slash = strrchr(skin, '/')) {
            *++slash = '\0';
            constexpr char skinName[] = "model_default.skin";
            if (size_t(slash - skin) + sizeof(skinName) <= sizeof(skin)) {
                Q_strcat(skin, sizeof(skin), skinName);
                // Prediction and drawing share the client's skin cache. Use
                // the render registration path so collision cannot cache a
                // skin whose materials have no render passes.
                const qhandle_t handle = client ? re->RegisterSkin(skin) : re->RegisterServerSkin(skin);
                if (handle) re->G2API_SetSkin(*instance, 0, handle, handle);
            }
        }
    }
    vec3_t low, high;
    const bool sampled = re->G2API_JAR_PoseGeometry(*instance, frame, nullptr, low, high,
                                                   &collector, Collector::Emit) != qfalse;
    re->G2API_CleanGhoul2Models(&instance);
    return sampled;
}

using ModelKey = std::pair<std::string, int>;
struct ModelKeyHash {
    size_t operator()(const ModelKey& key) const {
        return std::hash<std::string>{}(key.first) * 65599u + unsigned(key.second);
    }
};

using ModelList = std::list<std::pair<ModelKey, std::shared_ptr<const ModelHull>>>;
ModelList modelOrder;
std::unordered_map<ModelKey, ModelList::iterator, ModelKeyHash> modelIndex;

std::shared_ptr<const ModelHull> LookupModelHull(const char* model, int frame, bool client) {
    if (!model || frame < 0 || frame > 65535) return {};
    size_t length = 0;
    while (length < MAX_QPATH && model[length]) ++length;
    if (!length || length == MAX_QPATH) return {};
    ModelKey key{std::string(model, length), frame};
    for (char& c : key.first) {
        c = c == '\\' ? '/' : char(std::tolower(static_cast<unsigned char>(c)));
    }
    const char* extension = COM_GetExtension(key.first.c_str());
    const bool ghoul = !Q_stricmp(extension, "glm");
    if (!ghoul && Q_stricmp(extension, "md3")) return {};
    // A query before renderer initialization is transient.
    if (ghoul && !re) return {};

    const auto found = modelIndex.find(key);
    if (found != modelIndex.end()) {
        modelOrder.splice(modelOrder.begin(), modelOrder, found->second);
        return found->second->second;
    }

    auto result = std::make_shared<ModelHull>();
    Collector collector{*result};
    const bool loaded = ghoul ? LoadGhoul(key.first.c_str(), frame, collector, client) :
                               LoadMD3(key.first.c_str(), frame, collector);
    if (!loaded || !collector.valid || !result->geometry.RigidHull()) result.reset();

    if (modelOrder.size() == modelFrameLimit) {
        modelIndex.erase(modelOrder.back().first);
        modelOrder.pop_back();
    }
    modelOrder.emplace_front(std::move(key), result);
    modelIndex.emplace(modelOrder.front().first, modelOrder.begin());
    return result;
}

struct TraceSlot {
    std::shared_ptr<const ModelHull> model;
    vec3_t angles{};
    bool valid = false;
    NativePropHull hull;
};

std::unordered_map<unsigned, std::unique_ptr<TraceSlot>> traces[2];
TraceSlot transientTrace;

} // namespace

qboolean JAR_MP_VisitModelHull(const char* model, int frame, void* context,
                              void (*emit)(void*, const float*)) {
    if (!emit) return qfalse;
    const auto data = LookupModelHull(model, frame, false);
    if (!data) return qfalse;
    const auto& points = data->geometry.ModelPoints();
    const auto sendPoint = [&](jar::physics::Vec point) {
        const float coords[] = {point.x, point.y, point.z};
        emit(context, coords);
    };
    for (const auto& point : points) sendPoint(point);
    // A complex model crosses the same point limit in the game module.
    // Its remaining vertices are represented by support maxima.
    if (points.size() > JarSweepHull::modelPoints) {
        for (size_t i = 0; i < data->witnesses.size(); ++i)
            if (i != data->witnesses.size() / 2) sendPoint(data->witnesses[i]);
    }
    return qtrue;
}

const NativePropHull* JAR_MP_TraceHull(const char* model, int frame, const vec3_t angles,
                                      unsigned entity, bool client) {
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(angles[axis]) || std::fabs(angles[axis]) > 65536.f) return nullptr;
    }
    const auto data = LookupModelHull(model, frame, client);
    if (!data) return nullptr;

    auto* slot = &transientTrace;
    if (entity < ENTITYNUM_WORLD) {
        auto& cache = traces[client ? 1 : 0];
        auto found = cache.find(entity);
        if (found == cache.end()) {
            if (cache.size() == traceCacheLimit) cache.erase(cache.begin());
            found = cache.emplace(entity, std::make_unique<TraceSlot>()).first;
        }
        slot = found->second.get();
    }
    if (slot->model != data || !VectorCompare(slot->angles, angles)) {
        slot->model = data;
        VectorCopy(angles, slot->angles);
        slot->valid = slot->hull.Build(data->geometry, {angles[0], angles[1], angles[2]});
    }
    return slot->valid ? &slot->hull : nullptr;
}

void JAR_MP_ClearModelHulls() {
    for (auto& cache : traces) cache.clear();
    transientTrace = {};
    modelIndex.clear();
    modelOrder.clear();
}
