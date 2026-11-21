// Use the normal crouched hull when a ladder ends in a low opening.
// GPL-2.0-only.
#pragma once

static inline void JAR_LadderTrace(pmove_t *move, trace_t *hit, const vec3_t maxs, const vec3_t end) {
#ifdef JAR_MP
	move->trace(hit, move->ps->origin, move->mins, maxs, end, move->ps->clientNum, move->tracemask);
#else
	move->trace(hit, move->ps->origin, move->mins, maxs, end, move->ps->clientNum, move->tracemask, (EG2_Collision)0, 0);
#endif
}

static inline void JAR_LadderCrouch(pmove_t *move, const vec3_t forward, const vec3_t right, float crouchHeight) {
	if ((move->ps->pm_flags & PMF_DUCKED) || crouchHeight >= move->maxs[2]) {
		return;
	}

	vec3_t direction;
	VectorScale(forward, move->cmd.forwardmove, direction);
	VectorMA(direction, move->cmd.rightmove, right, direction);
	direction[2] = fmaxf(0.f, direction[2] + move->cmd.upmove);
	if (!VectorNormalize(direction)) {
		return;
	}

	vec3_t end, crouchedMaxs;
	VectorMA(move->ps->origin, 4, direction, end);
	trace_t standing;
	JAR_LadderTrace(move, &standing, move->maxs, end);
	if (standing.startsolid || standing.allsolid || standing.fraction == 1) {
		return;
	}

	VectorCopy(move->maxs, crouchedMaxs);
	crouchedMaxs[2] = crouchHeight;
	trace_t crouched;
	JAR_LadderTrace(move, &crouched, crouchedMaxs, end);
	if (!crouched.startsolid && !crouched.allsolid && crouched.fraction == 1) {
		move->ps->pm_flags |= PMF_DUCKED;
		move->maxs[2] = crouchHeight;
		move->ps->viewheight = crouchHeight + STANDARD_VIEWHEIGHT_OFFSET;
	}
}
