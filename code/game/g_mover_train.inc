// Raven/Activision/OpenJK native mover state and train arrivals. GPL-2.0-only.
#include "g_bsp_section.h"

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
		ent->s.eFlags &= ~EF_BLOCKED_MOVER;
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
		ent->s.eFlags &= ~EF_BLOCKED_MOVER;
		break;
	}
	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
	gi.linkentity( ent );
}

void G_PlayDoorSound(gentity_t*,int);
extern int BMS_START,BMS_END;

void Think_BeginMoving(gentity_t* ent){
	if(jar::bspSection::SectionTrainRoot(*ent)){
		G_PlayDoorSound(ent,BMS_START);
		G_PlayDoorLoopSound(ent);
	}
	// The Doom Detention shooting gallery hides TIEs during the wait.
	if(ent->spawnflags&2048)ent->s.eFlags&=~EF_NODRAW;
	ent->s.pos.trTime=level.time;
	ent->s.pos.trType=ent->alt_fire?TR_LINEAR_STOP:TR_NONLINEAR_STOP;
}

void Reached_Train( gentity_t *ent ) {
	gentity_t		*next;
	float			speed;
	vec3_t			move;
	float			length;

	// copy the apropriate values
	next = ent->nextTrain;
	const auto* section=jar::bspSection::SectionTrainRoot(*ent);
	if(section&&(!next||!jar::bspSection::SectionTrainCorner(*next,*section)||!next->nextTrain||
	             !jar::bspSection::SectionTrainCorner(*next->nextTrain,*section))){
		VectorCopy(ent->currentOrigin,ent->s.pos.trBase);
		ent->s.pos.trType=TR_STATIONARY;
		ent->s.loopSound=0;
		return;
	}
	if(!next||!next->nextTrain)return;

	// fire all other targets
	G_UseTargets( next, ent );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
	VectorCopy( next->s.origin, ent->pos1 );
	VectorCopy( next->nextTrain->s.origin, ent->pos2 );
	vec3_t forceOffset;G_JAR_MoverPathOffset(*ent,forceOffset);VectorAdd(ent->pos1,forceOffset,ent->pos1);VectorAdd(ent->pos2,forceOffset,ent->pos2);

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
	} else {
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if ( speed < 1 ) {
		speed = 1;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->s.pos.trDuration = length * 1000 / speed;

	// looping sound
/*
	if ( VALIDSTRING( next->soundSet ) )
	{
		ent->s.loopSound = CAS_GetBModelSound( next->soundSet, BMS_MID );//ent->soundLoop;
	}
*/
	if(!section||!next->wait)G_PlayDoorLoopSound( ent );
	else ent->s.loopSound=0;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );
	if(section)G_PlayDoorSound(ent,BMS_END);

	if (( next->spawnflags & 1 ))
	{
		vec3_t angs;

		vectoangles( move, angs );
		AnglesSubtract( angs, ent->currentAngles, angs );

		for ( int i = 0; i < 3; i++ )
		{
			AngleNormalize360( angs[i] );
		}
		VectorCopy( ent->currentAngles, ent->s.apos.trBase );
		VectorScale( angs, 0.5f, ent->s.apos.trDelta );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 2000;
		if ( ent->alt_fire )
		{
			ent->s.apos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.apos.trType = TR_NONLINEAR_STOP;
		}
	}
	else
	{
		if (( next->spawnflags & 4 ))
		{//yaw
			vec3_t angs;

			vectoangles( move, angs );
			AnglesSubtract( angs, ent->currentAngles, angs );

			for ( int i = 0; i < 3; i++ )
			{
				AngleNormalize360( angs[i] );
			}
			VectorCopy( ent->currentAngles, ent->s.apos.trBase );
			ent->s.apos.trDelta[YAW] = angs[YAW] * 0.5f;
			if (( next->spawnflags & 8 ))
			{//roll, too
				ent->s.apos.trDelta[ROLL] = angs[YAW] * -0.1f;
			}
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 2000;
			if ( ent->alt_fire )
			{
				ent->s.apos.trType = TR_LINEAR_STOP;
			}
			else
			{
				ent->s.apos.trType = TR_NONLINEAR_STOP;
			}
		}
	}

	// This is for the tie fighter shooting gallery on doom detention, you could see them waiting under the bay, but the architecture couldn't easily be changed..
	if (( next->spawnflags & 2 ))
	{
		ent->s.eFlags |= EF_NODRAW; // make us invisible until we start moving again
	}

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait )
	{
		ent->nextthink = level.time + next->wait * 1000;
		ent->e_ThinkFunc = thinkF_Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
	else if (!( next->spawnflags & 2 ))
	{
		// we aren't waiting to start, so let us draw right away
		ent->s.eFlags &= ~EF_NODRAW;
	}
}
