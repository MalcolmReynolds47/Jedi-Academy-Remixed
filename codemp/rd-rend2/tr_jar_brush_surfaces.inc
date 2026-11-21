// Raven Software, OpenJK and JA:R contributors. GPL-2.0-only.
#include "../../shared/qcommon/jar_yavin_sky.h"

void R_AddBrushModelSurfaces(trRefEntity_t *ent, int entityNum) {
	const model_t *model = R_GetModelByHandle(ent->e.hModel);
	bmodel_t *bmodel = model->data.bmodel;
	if (R_CullLocalBox(bmodel->bounds) == CULL_OUT)
		return;

	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW))
		R_DlightBmodel(bmodel, ent);

	world_t *world = R_GetWorld(bmodel->worldIndex);
	const int sectionPass = ent->e.renderfx & (RF_JAR_SECTION_ARENA | RF_JAR_SECTION_BACKDROP);
	// Each entity uses its own transform. The BSP leaf traversal alone
	// deduplicates world surfaces referenced by multiple leaves.
	for (int i = 0; i < bmodel->numSurfaces; ++i) {
		if (sectionPass && !jar::sectionSky::SubmitSurface(bmodel->numSurfaces, i, sectionPass == RF_JAR_SECTION_BACKDROP))
			continue;
		msurface_t *surface = world->surfaces + bmodel->firstSurface + i;
		R_AddWorldSurface(surface, ent, entityNum, ent->needDlights, 0);
	}
}
