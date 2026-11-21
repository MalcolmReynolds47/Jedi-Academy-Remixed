// Mounted rack pickups use stock media at the published shelf pose.
// GPL-2.0-only.
#include "game/jar_mp_controller.h"

static qboolean CG_JAR_IsRackMedpack(const gitem_t *item)
{
	return item->giType == IT_HEALTH && item->classname && !strcmp(item->classname, "item_medpak_instant");
}

static qboolean CG_JAR_IsRackItem(const entityState_t *state, const gitem_t *item)
{
	if (state->eType != ET_ITEM || state->generic1 != JAR_MP_RACK_ITEM_TAG ||
		state->otherEntityNum < MAX_CLIENTS || state->otherEntityNum >= ENTITYNUM_WORLD)
		return qfalse;
	if (CG_JAR_IsRackMedpack(item))
		return qtrue;
	if (state->modelindex2 != 1)
		return qfalse;
	return (item->giType == IT_WEAPON && (item->giTag == WP_BLASTER || item->giTag == WP_REPEATER ||
		item->giTag == WP_ROCKET_LAUNCHER)) || (item->giType == IT_AMMO &&
		(item->giTag == AMMO_BLASTER || item->giTag == AMMO_METAL_BOLTS || item->giTag == AMMO_POWERCELL));
}

static void CG_JAR_DrawRackItem(const centity_t *cent, const gitem_t *item, const itemInfo_t *media)
{
	refEntity_t ent = {0};
	ent.reType = RT_MODEL;
	ent.hModel = media->models[0];
	ent.ghoul2 = media->g2Models[0];
	ent.radius = media->radius[0];
	const int model = cent->currentState.modelindex2;
	if (CG_JAR_IsRackMedpack(item) && model > 0 && model < MAX_MODELS && cgs.gameModels[model] > 0)
		ent.hModel = cgs.gameModels[model];
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	VectorCopy(cent->lerpAngles, ent.angles);
	AnglesToAxis(ent.angles, ent.axis);
	VectorSet(ent.modelScale, 1, 1, 1);
	memset(ent.shaderRGBA, 255, sizeof(ent.shaderRGBA));
	trap->R_AddRefEntityToScene(&ent);
}
