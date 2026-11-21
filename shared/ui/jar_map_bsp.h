// Shared BSP checks and thumbnail loading for the SP and MP map browsers.
#pragma once
#include "jar_map_thumbnail.h"
#include <iterator>

namespace jar { namespace mapPicker {
inline bool ValidHeader(const dheader_t& header, int size) {
    if (size<int(sizeof(header)) || header.ident!=BSP_IDENT || header.version!=BSP_VERSION) return false;
    for (const auto& lump:header.lumps) {
        if (lump.filelen<0 || (lump.filelen && (lump.fileofs<int(sizeof(header)) ||
            lump.fileofs>size || lump.filelen>size-lump.fileofs))) return false;
    }
    return true;
}

template<class Reader>
bool RasterizeBsp(Reader& bsp, const Point& focus, Image& image) {
    std::vector<dmodel_t> models;
    std::vector<dshader_t> shaders;
    std::vector<drawVert_t> vertices;
    std::vector<int> indices;
    std::vector<dsurface_t> surfaces;
    int lumps[]={LUMP_SHADERS,LUMP_MODELS,LUMP_DRAWVERTS,LUMP_DRAWINDEXES,LUMP_SURFACES};
    std::sort(std::begin(lumps),std::end(lumps),[&](int a,int b) { return bsp.Offset(a)<bsp.Offset(b); });
    for (const int index:lumps) {
        bool read=false;
        switch (index) {
        case LUMP_SHADERS: read=bsp.Read(index,MAX_MAP_SHADERS*sizeof(dshader_t),shaders); break;
        case LUMP_MODELS: read=bsp.Read(index,MAX_MAP_MODELS*sizeof(dmodel_t),models); break;
        case LUMP_DRAWVERTS: read=bsp.Read(index,MAX_MAP_DRAW_VERTS*sizeof(drawVert_t),vertices); break;
        case LUMP_DRAWINDEXES: read=bsp.Read(index,MAX_MAP_DRAW_INDEXES*sizeof(int),indices); break;
        case LUMP_SURFACES: read=bsp.Read(index,MAX_MAP_DRAW_SURFS*sizeof(dsurface_t),surfaces); break;
        }
        if (!read) return false;
    }
    return !models.empty() && Rasterize(models.front(),shaders,vertices,indices,surfaces,focus,image);
}

inline void ThumbnailPath(const Image& image, char (&path)[MAX_QPATH]) {
    uint64_t hash=14695981039346656037ull;
    for (const uint8_t byte:image) hash=(hash^byte)*1099511628211ull;
    std::snprintf(path,sizeof(path),"jar/mapshots/%08x%08x.tga",unsigned(hash>>32),unsigned(hash));
}
}} // namespace jar::mapPicker
