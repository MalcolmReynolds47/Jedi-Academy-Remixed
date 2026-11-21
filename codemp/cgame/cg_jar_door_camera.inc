// Match the stock telescoping shaft to the interpolated camera head.
// GPL-2.0-only.
#include "game/jar_mp_door_camera.h"

static qboolean CG_JAR_DoorCameraShaftValid(const entityState_t *state)
{
	return JAR_MP_DoorCameraPartState(state) == JAR_MP_DOOR_CAMERA_SHAFT && !state->modelGhoul2 &&
		state->modelindex > 0 && state->modelindex < MAX_MODELS &&
		!(state->eFlags & EF_NODRAW) && isfinite(state->speed) &&
		state->speed >= JAR_MP_DOOR_CAMERA_MIN_SCALE && state->speed <= JAR_MP_DOOR_CAMERA_MAX_SCALE;
}

static qboolean CG_JAR_DoorCameraScale(const centity_t *cent, refEntity_t *ent)
{
	const entityState_t *current = &cent->currentState;
	if (!CG_JAR_DoorCameraShaftValid(current))
		return qfalse;

	float scale = current->speed;
	const entityState_t *next = &cent->nextState;
	const float fraction = cg.frameInterpolation;
	if (cent->interpolate && isfinite(fraction) && fraction > 0.0f &&
		CG_JAR_DoorCameraShaftValid(next) && next->number == current->number &&
		next->modelindex == current->modelindex && next->otherEntityNum == current->otherEntityNum &&
		next->time2 == current->time2)
		scale += (next->speed - scale) * fminf(fraction, 1.0f);

	VectorSet(ent->modelScale, 1, 1, scale);
	return qtrue;
}
