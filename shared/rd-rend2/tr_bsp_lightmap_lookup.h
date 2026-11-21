// Shader lookup uses the owning lightmap, not the most recently loaded map.
#pragma once
static image_t *R_BSPDeluxemap(int index) {
	return index >= 0 && index < tr.numLightmaps && tr.deluxemaps ? tr.deluxemaps[index] : nullptr;
}
static qboolean R_BSPLightmapHDR(int index) {
	if (index == LIGHTMAP_2D || index == LIGHTMAP_WHITEIMAGE) return qfalse;
	if (index >= 0) return index < tr.numLightmaps && tr.lightmapHDR ? tr.lightmapHDR[index] : qfalse;
	return tr.loadingLightmaps ? tr.loadingLightmaps->hdrLighting : tr.hdrLighting;
}
