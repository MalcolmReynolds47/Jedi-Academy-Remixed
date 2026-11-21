// Include visible instances of secondary BSPs in the view's depth range.
// GPL-2.0-only.
#include "../../code/rd-common/tr_bsp_far.h"

static float R_Rend2BSPFarDepth(const viewParms_t &view, const trRefdef_t &scene, float depth)
{
	if (!r_drawentities->integer)
		return depth;

	for (int entity = 0; entity < scene.num_entities; ++entity)
	{
		const refEntity_t &instance = scene.entities[entity].e;
		if (instance.reType != RT_MODEL || ((view.flags & VPF_NOVIEWMODEL) && (instance.renderfx & RF_FIRST_PERSON)))
			continue;
		const model_t *model = R_GetModelByHandle(instance.hModel);
		if (!model || model->type != MOD_BRUSH || model->data.bmodel->worldIndex < 0)
			continue;
		const float instanceDepth = R_BSPFarDepth(model->data.bmodel->bounds, instance.origin, instance.axis,
			view.ori.origin, view.ori.axis[0], r_nocull->integer ? nullptr : view.frustum);
		if (instanceDepth > depth)
			depth = instanceDepth;
	}
	return depth;
}
