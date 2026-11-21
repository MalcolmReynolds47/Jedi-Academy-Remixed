// The renderer keeps stable global indices; each BSP owns its packing state.
static constexpr int JAR_BSP_LIGHTMAP_LIMIT = 4096;
static void R_PublishBSPLightmaps(world_t *worldData) {
	auto &local = worldData->lighting;
	const bool main = worldData == &s_worldData;
	const int base = main ? 0 : tr.numLightmaps;
	if (local.numLightmaps < 0 || base < 0 || local.numLightmaps > JAR_BSP_LIGHTMAP_LIMIT - base)
		ri.Error(ERR_DROP, "BSP lightmap registry capacity exceeded");
	const int count = base + local.numLightmaps;
	auto **maps = count ? (image_t **)Hunk_Alloc(count * sizeof(image_t *), h_low) : nullptr;
	auto **deluxe = count ? (image_t **)Hunk_Alloc(count * sizeof(image_t *), h_low) : nullptr;
	auto *hdr = count ? (qboolean *)Hunk_Alloc(count * sizeof(qboolean), h_low) : nullptr;
	for (int i=0; i<base; ++i) {
		maps[i]=tr.lightmaps[i];deluxe[i]=tr.deluxemaps ? tr.deluxemaps[i] : nullptr;
		hdr[i]=tr.lightmapHDR ? tr.lightmapHDR[i] : qfalse;
	}
	for (int i=0; i<local.numLightmaps; ++i) {
		maps[base+i]=local.lightmaps[i];deluxe[base+i]=local.deluxemaps ? local.deluxemaps[i] : nullptr;
		hdr[base+i]=local.hdrLighting;
	}
	local.firstLightmap=base;
	tr.lightmaps=maps;tr.deluxemaps=deluxe;tr.lightmapHDR=hdr;tr.numLightmaps=count;
	if (main) {
		tr.hdrLighting=local.hdrLighting;
		tr.worldDeluxeMapping=local.worldDeluxeMapping;
		tr.worldInternalDeluxeMapping=local.worldInternalDeluxeMapping;
		tr.worldInternalLightmapping=local.worldInternalLightmapping;
	}
}

class JAR_BSPLightingScope {
	bool sub;
	const jarBspLightmaps_t *previous;
	vec3_t sunLight, sunDirection, tone;
	vec2_t exposure;
	float mapScale, shadowScale, fogScale, distance;
	qboolean sunShadows;
public:
	JAR_BSPLightingScope(world_t *world, bool isSub) : sub(isSub),
		previous(isSub ? tr.loadingLightmaps : nullptr), mapScale(tr.mapLightScale),
		shadowScale(tr.sunShadowScale), fogScale(tr.volumetricFogScale), distance(tr.distanceCull), sunShadows(tr.sunShadows) {
		VectorCopy(tr.sunLight,sunLight);VectorCopy(tr.sunDirection,sunDirection);
		VectorCopy(tr.toneMinAvgMaxLevel,tone);memcpy(exposure,tr.autoExposureMinMax,sizeof(exposure));
		tr.loadingLightmaps=&world->lighting;
	}
	~JAR_BSPLightingScope() {
		tr.loadingLightmaps=previous;
		if (!sub) return;
		VectorCopy(sunLight,tr.sunLight);VectorCopy(sunDirection,tr.sunDirection);
		VectorCopy(tone,tr.toneMinAvgMaxLevel);memcpy(tr.autoExposureMinMax,exposure,sizeof(exposure));
		tr.mapLightScale=mapScale;tr.sunShadowScale=shadowScale;tr.volumetricFogScale=fogScale;
		tr.distanceCull=distance;tr.sunShadows=sunShadows;
	}
};
