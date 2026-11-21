// Portable emplaced guns use their stock MP models and animation rigs.
// GPL-2.0-only.
#include "rd-common/mdx_format.h"
extern int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint);
extern void CG_G2ServerBoneAngles(centity_t *cent);

qboolean CG_JAR_EmplacedSlotChanged(const entityState_t *current, const entityState_t *next)
{
	const qboolean wasEmplaced = JAR_MP_EmplacedState(current);
	const qboolean isEmplaced = JAR_MP_EmplacedState(next);
	return (wasEmplaced || isEmplaced) && (wasEmplaced != isEmplaced || current->bolt2 != next->bolt2 ||
		current->modelindex != next->modelindex || current->time2 != next->time2);
}

void CG_JAR_EmplacedTransition(centity_t *chair, const entityState_t *next)
{
	if (!CG_JAR_EmplacedSlotChanged(&chair->currentState, next))
		return;
	if (chair->ghoul2)
		trap->G2API_CleanGhoul2Models(&chair->ghoul2);
	chair->ghoul2 = NULL;
	chair->bolt1 = chair->bolt2 = 0;
	chair->jarEmplacedAimed = qfalse;
	chair->jarEWebWrecked = qfalse;
	chair->interpolate = qfalse;
	chair->previousEvent = 0;
}

static qboolean CG_JAR_EmplacedModel(centity_t *chair)
{
	if (chair->ghoul2)
		return qtrue;
	const entityState_t *s = &chair->currentState;
	if (s->modelGhoul2 != 1 || s->modelindex <= 0 || s->modelindex >= MAX_MODELS)
		return qfalse;
	const char *model = CG_ConfigString(CS_MODELS + s->modelindex);
	if (!model[0] || trap->G2API_InitGhoul2Model(&chair->ghoul2, model, 0, 0, 0, 0, 0) < 0 || !chair->ghoul2)
		return qfalse;
	chair->bolt1 = chair->bolt2 = 0;
	chair->jarEmplacedAimed = qfalse;
	chair->jarEWebWrecked = qfalse;
	if (JAR_MP_EWebState(s)) {
		chair->pe.torso.animationNumber = -1;
		chair->pe.legs.animationNumber = -1;
	}
	return qtrue;
}

static void CG_GeneralGhoul2Pose(centity_t *cent)
{
	const entityState_t *s = &cent->currentState;
	if (JAR_MP_EWebState(s) && !(s->eFlags & EF_NODRAW)) {
		if (!CG_JAR_EmplacedModel(cent))
			return;
		const qboolean wrecked = s->time == -1;
		if (wrecked != cent->jarEWebWrecked && trap->G2API_SetSurfaceOnOff(cent->ghoul2, "eweb_damage",
			wrecked ? G2SURFACEFLAG_NODESCENDANTS : 0))
			cent->jarEWebWrecked = wrecked;
	}
	if (!cent->ghoul2)
		return;

	if (s->boneOrient)
		CG_G2ServerBoneAngles(cent);
	if ((s->eFlags & EF_G2ANIMATING) && (s->torsoAnim != cent->pe.torso.animationNumber ||
		s->legsAnim != cent->pe.legs.animationNumber || s->torsoFlip != cent->pe.torso.lastFlip)) {
		// For general entities the torso and legs values bound a root animation.
		trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", s->torsoAnim, s->legsAnim,
			BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, 1.0f, cg.time, -1, 100);
		cent->pe.torso.animationNumber = s->torsoAnim;
		cent->pe.legs.animationNumber = s->legsAnim;
		cent->pe.torso.lastFlip = s->torsoFlip;
	}
}

qboolean CG_JAR_EWebOperator(const centity_t *gun, const playerState_t *player)
{
	const entityState_t *s = &gun->currentState;
	return gun->currentValid && JAR_MP_EWebState(s) && s->weapon == WP_NONE &&
		s->activeForcePass == NUM_FORCE_POWERS + 1 && s->emplacedOwner == player->clientNum &&
		player->weapon == WP_EMPLACED_GUN && player->emplacedIndex == s->number;
}

qboolean CG_JAR_EWebMuzzle(centity_t *gun, vec3_t point, vec3_t direction)
{
	mdxaBone_t matrix;
	vec3_t axis, muzzle;
	if (!gun->ghoul2 || !JAR_MP_EWebState(&gun->currentState))
		return qfalse;
	if (!gun->bolt1)
		gun->bolt1 = trap->G2API_AddBolt(gun->ghoul2, 0, "*cannonflash") + 1;
	if (!gun->bolt1 || !trap->G2API_GetBoltMatrix(gun->ghoul2, 0, gun->bolt1 - 1, &matrix,
		gun->lerpAngles, gun->lerpOrigin, cg.time, cgs.gameModels, gun->modelScale))
		return qfalse;

	BG_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Y, axis);
	const float length = VectorNormalize(axis);
	if (length <= 0 || !isfinite(length))
		return qfalse;
	BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, muzzle);
	VectorMA(muzzle, -16.0f, axis, point);
	VectorCopy(axis, direction);
	return qtrue;
}

static void CG_JAR_EmplacedAim(centity_t *chair, const vec3_t baseAngles, qboolean snapshotPose)
{
	const entityState_t *s = &chair->currentState;
	const int operatorNum = s->emplacedOwner;
	vec3_t aim, base;
	float yaw;
	if (!chair->ghoul2 || !cg.snap)
		return;
	if (s->activeForcePass != NUM_FORCE_POWERS + 1) {
		if (chair->jarEmplacedAimed) {
			trap->G2API_SetBoneAngles(chair->ghoul2, 0, "Bone02", vec3_origin, BONE_ANGLES_REPLACE,
				NEGATIVE_Y, NEGATIVE_X, POSITIVE_Z, NULL, 0, cg.time);
			chair->jarEmplacedAimed = qfalse;
		}
		return;
	}
	if (operatorNum < 0 || operatorNum >= MAX_CLIENTS)
		return;

	if (operatorNum == cg.snap->ps.clientNum) {
		if (cg.snap->ps.emplacedIndex != s->number || cg.snap->ps.weapon != WP_EMPLACED_GUN)
			return;
		if (snapshotPose)
			VectorCopy(cg.snap->ps.viewangles, aim);
		else if (cg.renderingThirdPerson)
			VectorCopy(cg.predictedPlayerState.viewangles, aim);
		else
			VectorCopy(cg.refdef.viewangles, aim);
	} else {
		const centity_t *gunner = &cg_entities[operatorNum];
		if (!gunner->currentValid || gunner->currentState.weapon != WP_EMPLACED_GUN || gunner->currentState.otherEntityNum2 != s->number)
			return;
		if (snapshotPose)
			BG_EvaluateTrajectory(&gunner->currentState.apos, cg.snap->serverTime, aim);
		else
			VectorCopy(gunner->lerpAngles, aim);
	}

	VectorCopy(baseAngles, base);
	if (BG_EmplacedView(aim, base, &yaw, JAR_MP_EmplacedConstraint(s)))
		aim[YAW] = yaw;
	if (aim[PITCH] > 40)
		aim[PITCH] = 40;
	aim[YAW] = AngleSubtract(aim[YAW], base[YAW]);
	if (trap->G2API_SetBoneAngles(chair->ghoul2, 0, "Bone02", aim, BONE_ANGLES_REPLACE,
		NEGATIVE_Y, NEGATIVE_X, POSITIVE_Z, NULL, 0, cg.time))
		chair->jarEmplacedAimed = qtrue;
}

// Snapshot events can arrive before the chair's first rendered frame.
void CG_JAR_EmplacedFireEvent(centity_t *chair, const vec3_t origin)
{
	if (!cg.snap || !CG_JAR_EmplacedModel(chair))
		return;
	const entityState_t *s = &chair->currentState;
	int *muzzleBolt = s->eventParm ? &chair->bolt2 : &chair->bolt1;
	// Bolt zero is valid. Keep a one-based cache for this Ghoul2 instance.
	if (!*muzzleBolt)
		*muzzleBolt = trap->G2API_AddBolt(chair->ghoul2, 0, s->eventParm ? "*flash02" : "*flash01") + 1;
	if (!*muzzleBolt)
		return;

	vec3_t angles, point, direction;
	mdxaBone_t matrix;
	const float factor = s->iModelScale * 0.01f;
	vec3_t scale = {factor, factor, factor};
	BG_EvaluateTrajectory(&s->apos, cg.snap->serverTime, angles);
	trap->G2API_SetBoneAnim(chair->ghoul2, 0, "Bone02", 1, 4,
		BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, 1.0f, cg.time, -1, 300);
	CG_JAR_EmplacedAim(chair, angles, qtrue);
	if (!trap->G2API_GetBoltMatrix(chair->ghoul2, 0, *muzzleBolt - 1, &matrix, angles,
		origin, cg.time, cgs.gameModels, scale))
		return;
	BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, point);
	BG_GiveMeVectorFromMatrix(&matrix, NEGATIVE_X, direction);
	trap->FX_PlayEffectID(cgs.effects.mEmplacedMuzzleFlash, point, direction, -1, -1, qfalse);
}
