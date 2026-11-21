// Native binary mover trajectory helpers. GPL-2.0-only.
void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vec3_t			delta;
	float			f;

	ent->moverState = moverState;

	ent->s.pos.trTime = time;

	if ( ent->s.pos.trDuration <= 0 )
	{//Don't allow divide by zero!
		ent->s.pos.trDuration = 1;
	}

	switch( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		if ( ent->alt_fire )
		{
			ent->s.pos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.pos.trType = TR_NONLINEAR_STOP;
		}
		//ent->s.eFlags &= ~EF_BLOCKED_MOVER;
		break;
	case MOVER_2TO1:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		if ( ent->alt_fire )
		{
			ent->s.pos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.pos.trType = TR_NONLINEAR_STOP;
		}
		//ent->s.eFlags &= ~EF_BLOCKED_MOVER;
		break;
	}
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	trap->LinkEntity( (sharedEntity_t *)ent );
}

void InitMoverTrData( gentity_t *ent )
{
	vec3_t		move;
	float		distance;

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( ! ent->speed )
	{
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 )
	{
		ent->s.pos.trDuration = 1;
	}
}
