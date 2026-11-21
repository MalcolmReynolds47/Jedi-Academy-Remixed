// Presentation for the stock portable MD3 bomb and beacon.
// GPL-2.0-only.
#include "game/jar_mp_animated.h"

static void CG_JAR_AnimatedProp(const centity_t *cent, refEntity_t *ent)
{
	const entityState_t *current = &cent->currentState;
	const int tag = JAR_MP_AnimatedTag(current);
	if (!tag)
		return;

	// Both renderers subtract this offset from the view's shader clock.
	if (tag == JAR_MP_BOMB_TAG && (current->time == JAR_MP_BOMB_DISARMING || current->time == JAR_MP_BOMB_DISARMED))
		ent->shaderTime = cg.time * 0.001f;

	const float fraction = cg.frameInterpolation;
	if (!cent->interpolate || !(fraction > 0.0f && fraction < 1.0f))
		return;

	const entityState_t *next = &cent->nextState;
	if (JAR_MP_AnimatedTag(next) != tag ||
		next->number != current->number || next->modelindex != current->modelindex ||
		!current->time2 || next->time2 != current->time2 ||
		(tag != JAR_MP_BUTTON_TAG && next->time != current->time) ||
		((current->eFlags | next->eFlags) & (EF_NODRAW | EF_SHADER_ANIM)))
		return;

	if (!JAR_MP_AnimatedFrameValid(tag, current->time, current->frame) ||
		!JAR_MP_AnimatedFrameValid(tag, next->time, next->frame))
		return;

	const qboolean wrap = tag == JAR_MP_BEACON_TAG && current->time == JAR_MP_BEACON_SIGNALLING &&
		current->frame == JAR_MP_BEACON_LOOP_LAST && next->frame == JAR_MP_BEACON_LOOP_FIRST;
	if (tag == JAR_MP_BUTTON_TAG ?
		!JAR_ButtonFrameStep(current->time,current->frame,next->time,next->frame) :
		(next->frame != current->frame + 1 && !wrap))
		return;

	ent->frame = next->frame;
	ent->oldframe = current->frame;
	ent->backlerp = 1.0f - fraction;
}
