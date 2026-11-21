// Native mover transaction shared with collision/clock regression. GPL-2.0-only.
#include "qcommon/jar_angular_trajectory.h"
void G_MoverTeam( gentity_t *ent ) {
	vec3_t		move, amove;
	gentity_t	*part, *obstacle;
	vec3_t		origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain)
	{
		// get current position
		part->s.eFlags &= ~EF_BLOCKED_MOVER;
		EvaluateTrajectory( &part->s.pos, level.time, origin );
		EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->currentOrigin, move );
		VectorSubtract( angles, part->currentAngles, amove );
		if (!G_RailCanMove(*part,origin)) {obstacle=&g_entities[ENTITYNUM_WORLD];break;}
		float rotation[3][3];
		const bool angular = jar::angular::Path(part->s.apos.trType);
		if (angular) jar::angular::DeltaMatrix(part->currentAngles,angles,rotation);
		if ( !G_MoverPush( part, move, amove, &obstacle, true, angular ? rotation : nullptr ) )
		{
			break;	// move was blocked
		}
	}

	if (part)
	{
		// if the pusher has a "blocked" function, call it
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain )
		{
			//Push up time so it doesn't wiggle when blocked
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			EvaluateTrajectory( &part->s.pos, level.time, part->currentOrigin );
			EvaluateTrajectory( &part->s.apos, level.time, part->currentAngles );
			gi.linkentity( part );
			part->s.eFlags |= EF_BLOCKED_MOVER;
		}

		if ( ent->e_BlockedFunc != blockedF_NULL )
		{// this check no longer necessary, done internally below, but it's here for reference
			GEntity_BlockedFunc( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain )
	{
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == TR_LINEAR_STOP ||
			part->s.pos.trType == TR_NONLINEAR_STOP )
		{
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration )
			{
				GEntity_ReachedFunc( part );
			}
		}
	}
}
