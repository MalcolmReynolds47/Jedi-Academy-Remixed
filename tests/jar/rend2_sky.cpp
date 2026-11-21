// Exercise the production sky iterator with the GPU upload and draw queue stubbed.
#include "tr_local.h"
#include "test_support.h"
#include <memory>

trGlobals_t tr{};
backEndState_t backEnd{};
shaderCommands_t tess{};
refimport_t ri{};
backEndData_t* backEndData;
vec4_t colorWhite = {1, 1, 1, 1};
void* R_Malloc(int size, unsigned int, qboolean) { return std::calloc(1, size); }
static cvar_t fastSky{};
cvar_t* r_fastsky = &fastSky;
static int uploads, draws, cloudPasses;

void RB_UpdateVBOs(unsigned int) {
    CHECK(tess.numVertexes == 81 && tess.numIndexes == 384);
    CHECK(tess.firstIndex == 0 && tess.useInternalVBO);
    CHECK(tess.indexes[0] == 0 && tess.indexes[383] == 80);
    for (int i = 0; i < tess.numIndexes; ++i) {
        CHECK(tess.indexes[i] < static_cast<glIndex_t>(tess.numVertexes));
    }
    ++uploads;
}
void RB_FillDrawCommand(DrawCommand& draw, GLenum primitive, int instances, const shaderCommands_t* input) {
    CHECK(primitive == GL_TRIANGLES && instances == 1 && input == &tess);
    draw.params.indexed.numIndices = input->numIndexes;
}
void RB_AddDrawItem(Pass*, uint32_t, const DrawItem& item) {
    CHECK(item.draw.params.indexed.numIndices == 384);
    ++draws;
}
void RB_StageIteratorGeneric() {
    CHECK(tess.firstIndex == 0 && tess.useInternalVBO && tess.externalIBO == nullptr);
    CHECK(tess.numVertexes == 405 && tess.numIndexes == 1920);
    ++cloudPasses;
}

void RB_CommitInternalBufferData() {}
void CalculateVertexArraysProperties(uint32_t, VertexArraysProperties* properties) { *properties = {}; }
void GL_VertexArraysToAttribs(vertexAttribute_t*, size_t, const VertexArraysProperties*) {}
DepthRange RB_GetDepthRange(const trRefEntity_t*, const shader_t*) { return {}; }
void DrawItemSetVertexAttributes(DrawItem&, const vertexAttribute_t*, uint32_t, Allocator&) {}
void DrawItemSetUniformBlockBindings(DrawItem&, const UniformBlockBinding*, uint32_t, Allocator&) {}
#ifdef REND2_SP
uint32_t RB_CreateSkySortKey(const DrawItem&, int, int, int) { return 0; }
#else
uint32_t RB_CreateSortKey(const DrawItem&, int, int) { return 0; }
void* Z_Malloc(int size, unsigned int, qboolean, int) { return std::calloc(1, size); }
void Z_Free(void* memory) { std::free(memory); }
void Matrix16Multiply(const float*, const float*, float*) {}
void Matrix16Translation(const float*, float*) {}
void GL_SetModelviewMatrix(float*) {}
void GL_DepthRange(float, float) {}
#endif
UniformDataWriter::UniformDataWriter() : scratch(scratchBuffer, sizeof(scratchBuffer), 1) {}
void UniformDataWriter::Start(shaderProgram_t*) {}
UniformDataWriter& UniformDataWriter::SetUniformVec4(uniform_t, float, float, float, float) { return *this; }
UniformDataWriter& UniformDataWriter::SetUniformInt(uniform_t, int) { return *this; }
UniformData* UniformDataWriter::Finish(Allocator&) { return nullptr; }
SamplerBindingsWriter::SamplerBindingsWriter() = default;
SamplerBindingsWriter& SamplerBindingsWriter::AddStaticImage(image_t*, int) { return *this; }
SamplerBinding* SamplerBindingsWriter::Finish(Allocator&, uint32_t* count) { *count = 0; return nullptr; }
void RB_BeginSurface(shader_t*, int, int) {}
void RB_AddQuadStamp(vec3_t, vec3_t, vec3_t, float*) {}
void RB_EndSurface() {}

int main() {
    return RunTests("Rend2 skybox uses independent face uploads at the tessellation limit", [] {
        auto data = std::make_unique<backEndData_t>();
        auto world = std::make_unique<world_t>();
        auto frame = std::make_unique<gpuFrame_t>();
        backEndData = data.get();
        tr.world = world.get();
        backEndData->currentFrame = frame.get();
        alignas(16) char memory[128]{};
        Allocator arena(memory, sizeof(memory));
        backEndData->perFrameMemory = &arena;
        backEnd.viewParms.zFar = 4096;
        ri.Error = [](int, const char* message, ...) { throw std::runtime_error(message); };

        shader_t shader{};
        image_t skyImage{};
        shaderStage_t cloud{};
        shaderStage_t* stages[MAX_SHADER_STAGES]{&cloud};
        shader.isSky = qtrue;
        shader.sky.cloudHeight = 128;
        for (auto& side : shader.sky.outerbox) {
            side = &skyImage;
        }
        tess.shader = &shader;
        tess.xstages = stages;
        R_InitSkyTexCoords(shader.sky.cloudHeight);

        // Fill the sky triangle batch almost to its limit, as the imported
        // Construct map can do. The triangles cover all six sides of a cube.
        const glIndex_t cube[] = {0,1,2, 1,3,2, 4,6,5, 5,6,7, 0,4,1, 1,4,5,
                                  2,3,6, 3,7,6, 0,2,4, 2,6,4, 1,5,3, 3,5,7};
        for (int i = 0; i < 8; ++i) {
            VectorSet(tess.xyz[i], (i & 1) ? 64 : -64,
                      (i & 2) ? 64 : -64, (i & 4) ? 64 : -64);
        }
        tess.numVertexes = SHADER_MAX_VERTEXES - 1;
        tess.numIndexes = SHADER_MAX_INDEXES - 6;
        for (int i = 0; i < tess.numIndexes; ++i) {
            tess.indexes[i] = cube[i % ARRAY_LEN(cube)];
        }
        RB_StageIteratorSky();
        CHECK(uploads == 6 && draws == 6 && cloudPasses == 1);
        CHECK(backEnd.skyRenderedThisView);
#ifdef REND2_SP
        CHECK(backEnd.skyNumber == 1);
#endif
    });
}
