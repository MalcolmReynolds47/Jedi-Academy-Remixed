// Native shared jump-pad movement, unchanged. GPL-2.0-only.
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL && ps->pm_type != PM_JETPACK && ps->pm_type != PM_FLOAT ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	/*
	if ( ps->jumppad_ent != jumppad->number ) {
		vec3_t angles;
		float p;

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		effectNum =  (p<45) ? 0 : 1;
	}
	*/

	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
	// fix: no more force draining after bouncing the jumppad
	ps->fd.forcePowersActive &= ~(1<<FP_LEVITATION);
}
