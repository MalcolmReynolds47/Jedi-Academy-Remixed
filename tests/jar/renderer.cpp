#include "../../code/rd-vanilla/tr_sort.cpp"
#include "test_support.h"
#include <vector>
#include <algorithm>

trGlobals_t tr;
refimport_t ri = {};
surfaceType_t entitySurface = SF_ENTITY;

int main()
{
    return RunTests("64-bit draw sorting, high entity IDs, alpha ordering and surface exhaustion", [] {
        std::vector<drawSurf_t> surfaces(MAX_DRAWSURFS);
        tr.refdef.drawSurfs = surfaces.data();
        shader_t shader = {}; shader.sortedIndex = MAX_SHADERS-1;
        tr.sortedShaders[shader.sortedIndex] = &shader;
        for (int ent : {0, 2047, 9999, 16383, MAX_REFENTITIES-1, REFENTITYNUM_WORLD}) {
            tr.shiftedEntityNum = uint64_t(ent) << QSORT_REFENTITYNUM_SHIFT;
            int index = tr.refdef.numDrawSurfs;
            R_AddDrawSurf(&entitySurface, &shader, 31, 3);
            int actual, fog, light; shader_t *actualShader;
            R_DecomposeSort(surfaces[index].sort, &actual, &actualShader, &fog, &light);
            CHECK(actual == ent && actualShader == &shader && fog == 31 && light == 3);
        }
        std::vector<drawSurf_t> shuffled(4096);
        for(size_t i=0; i<shuffled.size(); ++i) {
            shuffled[i].sort = (uint64_t(i % 2) << 63) | (uint64_t(4095-i) << QSORT_SHADERNUM_SHIFT) | (uint64_t(i) << 7);
        }
        R_RadixSort(shuffled.data(), int(shuffled.size()));
        for(size_t i=1; i<shuffled.size(); ++i) CHECK(shuffled[i-1].sort <= shuffled[i].sort);
        tr.refdef.numDrawSurfs = MAX_DRAWSURFS;
        auto first = surfaces[0].sort;
        R_AddDrawSurf(&entitySurface, &shader, 0, 0);
        CHECK(tr.refdef.numDrawSurfs == MAX_DRAWSURFS && surfaces[0].sort == first);
    });
}
