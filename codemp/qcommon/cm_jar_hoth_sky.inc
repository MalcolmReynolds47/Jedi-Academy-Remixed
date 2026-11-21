// Stock section collision identity and playable area. GPL-2.0-only.
#include "../../shared/qcommon/jar_hoth_sky.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include "../../code/game/g_bsp_section_rift.h"
#include "../../code/game/g_bsp_section_korriban.h"

static qboolean CM_StockSectionArea(int modelIndex, const vec3_t localPoint, const char *resource,
                                     unsigned stockChecksum, int area) {
	const int slot = CM_FindSubBSP(modelIndex);
	if (slot < 0)
		return qfalse;
	char name[MAX_QPATH];
	int first, count;
	unsigned checksum;
	if (CM_BSPModelAt(slot, name, sizeof(name), &first, &count, &checksum) != 1 ||
		first != modelIndex || checksum != stockChecksum || strcmp(name, resource))
		return qfalse;
	if (!localPoint)
		return qtrue;

	clipMap_t *map;
	const cmodel_t *model = CM_ClipHandleToModel(modelIndex, &map);
	for (int axis = 0; axis < 3; ++axis)
		if (!(localPoint[axis] >= model->mins[axis] && localPoint[axis] <= model->maxs[axis]))
			return qfalse;
	if (!map->numNodes)
		return qfalse;
	const int leaf = CM_PointLeafnum_r(localPoint, 0, map);
	return leaf >= 0 && leaf < map->numLeafs && map->leafs[leaf].cluster >= 0 &&
		map->leafs[leaf].area == area ? qtrue : qfalse;
}

qboolean CM_HothSectionSky(int modelIndex, const vec3_t localPoint) {
    return CM_StockSectionArea(modelIndex, localPoint, jar::hothSky::resource,
                               jar::hothSky::checksum, jar::hothSky::arenaArea);
}

qboolean CM_RiftSectionAir(int modelIndex, const vec3_t localPoint) {
    return CM_StockSectionArea(modelIndex, localPoint, jar::rift::resource,
                               jar::rift::checksum, jar::rift::arenaArea);
}

qboolean CM_YavinSectionSky(int modelIndex, const vec3_t localPoint) {
    return CM_StockSectionArea(modelIndex, localPoint, jar::yavinSky::resource,
                               jar::yavinSky::checksum, jar::yavinSky::arenaArea);
}

qboolean CM_KorribanSectionFog(int modelIndex) {
    return CM_StockSectionArea(modelIndex, nullptr, jar::korriban::resource,
                               jar::korriban::checksum, jar::korriban::arenaArea);
}
