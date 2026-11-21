// Native Raven/OpenJK multiplayer mover push and rollback. GPL-2.0-only.
typedef struct pushed_s {
	gentity_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	vec3_t	currentOrigin;
	vec3_t	currentAngles;
	vec3_t	playerOrigin;
	int	groundEntityNum;
	int	deltayaw;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;

/*
============
G_TestEntityPosition

============
*/
gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	int		mask;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		vec3_t vMax;
		VectorCopy(ent->r.maxs, vMax);
		if (vMax[2] < 1)
		{
			vMax[2] = 1;
		}
		trap->Trace( &tr, ent->client->ps.origin, ent->r.mins, vMax, ent->client->ps.origin, ent->s.number, mask, qfalse, 0, 0 );
	} else {
		trap->Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->s.number, mask, qfalse, 0, 0 );
	}

	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];

	return NULL;
}

/*
================
G_CreateRotationMatrix
================
*/
void G_CreateRotationMatrix(vec3_t angles, matrix3_t matrix) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
G_TransposeMatrix
================
*/
void G_TransposeMatrix(matrix3_t matrix, matrix3_t transpose) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
G_RotatePoint
================
*/
void G_RotatePoint(vec3_t point, matrix3_t matrix) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

static void G_RestorePushedEntity( const pushed_t *saved ) {
	gentity_t *ent = saved->ent;
	VectorCopy( saved->origin, ent->s.pos.trBase );
	VectorCopy( saved->angles, ent->s.apos.trBase );
	VectorCopy( saved->currentOrigin, ent->r.currentOrigin );
	VectorCopy( saved->currentAngles, ent->r.currentAngles );
	ent->s.groundEntityNum = saved->groundEntityNum;
	if ( ent->client ) {
		ent->client->ps.delta_angles[YAW] = saved->deltayaw;
		VectorCopy( saved->playerOrigin, ent->client->ps.origin );
	}
}

/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
static qboolean G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove, qboolean protected ) {
	matrix3_t	matrix, transpose;
	vec3_t		org, org2, move2;
	gentity_t	*block;

	//This was only serverside not to mention it was never set.
	/*
	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
		check->s.groundEntityNum != pusher->s.number ) {
		return qfalse;
	}
	*/
	if ( !protected && pusher->s.apos.trType != TR_STATIONARY//rotating
		&& (pusher->spawnflags&16) //IMPACT
		&& Q_stricmp( "func_rotating", pusher->classname ) == 0 )
	{//just blow the fuck out of them
		G_Damage( check, pusher, pusher, NULL, NULL, pusher->damage, DAMAGE_NO_KNOCKBACK, MOD_CRUSH );
		return qtrue;
	}

	// save off the old position
	if ( pushed_p == &pushed[MAX_GENTITIES] ) {
		trap->Error( ERR_DROP, "Native mover push stack exhausted" );
	}
	pushed_p->ent = check;
	VectorCopy( check->s.pos.trBase, pushed_p->origin );
	VectorCopy( check->s.apos.trBase, pushed_p->angles );
	VectorCopy( check->r.currentOrigin, pushed_p->currentOrigin );
	VectorCopy( check->r.currentAngles, pushed_p->currentAngles );
	pushed_p->groundEntityNum = check->s.groundEntityNum;
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy( check->client->ps.origin, pushed_p->playerOrigin );
	}
	pushed_p++;

	// try moving the contacted entity
	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	G_TransposeMatrix( transpose, matrix );
	if ( check->client ) {
		VectorSubtract (check->client->ps.origin, pusher->r.currentOrigin, org);
	}
	else {
		VectorSubtract (check->s.pos.trBase, pusher->r.currentOrigin, org);
	}
	VectorCopy( org, org2 );
	G_RotatePoint( org2, matrix );
	VectorSubtract (org2, org, move2);
	// add movement
	VectorAdd (check->s.pos.trBase, move, check->s.pos.trBase);
	VectorAdd (check->s.pos.trBase, move2, check->s.pos.trBase);
	if ( check->client ) {
		VectorAdd (check->client->ps.origin, move, check->client->ps.origin);
		VectorAdd (check->client->ps.origin, move2, check->client->ps.origin);
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	block = G_TestEntityPosition( check );
	if (!block) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->r.currentOrigin );
		} else {
			VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
		}
		trap->LinkEntity ((sharedEntity_t *)check);
		return qtrue;
	}

	if ( !protected && check->takedamage && !check->client && check->s.weapon &&
		check->r.ownerNum < MAX_CLIENTS && check->health > 0 && check->health < 500 ) {
		G_Damage( check, pusher, pusher, vec3_origin, check->r.currentOrigin, 999, 0, MOD_UNKNOWN );
		if ( !check->inuse ) {
			--pushed_p;
			return qtrue;
		}
	}
	// A sliding platform can leave an actor at its old pose.
	G_RestorePushedEntity( pushed_p - 1 );
	block = G_TestEntityPosition (check);
	if ( !block ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}


void G_ExplodeMissile( gentity_t *ent );
void NPC_RemoveBody( gentity_t *self );

static qboolean G_MoverClearBlocker( gentity_t *pusher, gentity_t *check ) {
	if ( pusher->damage && check->client && (pusher->spawnflags & 32) ) {
		G_Damage( check, pusher, pusher, NULL, NULL, pusher->damage, 0, MOD_CRUSH );
		return qtrue;
	}
	if ( check->s.eType == ET_BODY || (check->s.eType == ET_PLAYER && check->health < 1) ) {
		G_Damage( check, pusher, pusher, NULL, NULL, 999, 0, MOD_CRUSH );
		return qtrue;
	}
	if ( ((check->r.contents & CONTENTS_TRIGGER) && check->s.weapon == G2_MODEL_PART) ||
		(check->s.eFlags & EF_DROPPEDWEAPON) ) {
		G_FreeEntity( check );
		return qtrue;
	}
	if ( check->s.eType == ET_NPC && check->health <= 0 && !(check->flags & FL_NOTARGET) ) {
		NPC_RemoveBody( check );
		return qtrue;
	}
	// Native sinusoidal movers can crush a blocker and continue.
	if ( pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE ) {
		G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
		return qtrue;
	}
	return qfalse;
}

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	int			i;
	vec3_t		mins, maxs;
	int			entityList[MAX_GENTITIES];
	int			listedEntities;
	vec3_t		totalMins, totalMaxs;
	const qboolean sectionMover = JAR_MP_SectionMoverProtected( pusher, NULL );

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
		|| amove[0] || amove[1] || amove[2] ) {
		float		radius;

		radius = RadiusFromBounds( pusher->r.mins, pusher->r.maxs );
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - (sectionMover ? fmaxf( move[i], 0 ) : move[i]);
			totalMaxs[i] = maxs[i] - (sectionMover ? fminf( move[i], 0 ) : move[i]);
		}
	} else {
		for (i=0 ; i<3 ; i++) {
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );
		for (i=0 ; i<3 ; i++) {
			if ( move[i] > 0 ) {
				totalMaxs[i] += move[i];
			} else {
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap->UnlinkEntity( (sharedEntity_t *)pusher );

	listedEntities = trap->EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap->LinkEntity( (sharedEntity_t *)pusher );

	// A protected obstruction must be checked before native crush or free.
	// Authored movers use their original one-pass traversal.
	for ( int pass = 0; pass < (sectionMover ? 2 : 1); ++pass ) {
		for ( int e = 0; e < listedEntities; ++e ) {
			gentity_t *check = &g_entities[entityList[e]];
			if ( !check->inuse || (check->s.eType != ET_PLAYER &&
				check->s.eType != ET_NPC && !check->physicsObject) ) continue;
			const qboolean protected = sectionMover && JAR_MP_SectionMoverProtected( pusher, check );
			if ( sectionMover && protected != (pass == 0) ) continue;

			if ( check->s.groundEntityNum != pusher->s.number ) {
				if ( check->r.absmin[0] >= maxs[0] || check->r.absmin[1] >= maxs[1] ||
					check->r.absmin[2] >= maxs[2] || check->r.absmax[0] <= mins[0] ||
					check->r.absmax[1] <= mins[1] || check->r.absmax[2] <= mins[2] ) continue;
				if ( !G_TestEntityPosition( check ) ) continue;
			}
			if ( G_TryPushingEntity( check, pusher, move, amove, protected ) ||
				(!protected && G_MoverClearBlocker( pusher, check )) ) continue;

			*obstacle = check;
			// Reverse order also restores entities encountered by multiple team parts.
			while ( pushed_p != pushed ) {
				const pushed_t *saved = --pushed_p;
				if ( !saved->ent->inuse ) continue;
				G_RestorePushedEntity( saved );
				trap->LinkEntity( (sharedEntity_t *)saved->ent );
			}
			return qfalse;
		}
	}

	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( gentity_t *ent ) {
	vec3_t		move, amove;
	gentity_t	*part, *obstacle;
	vec3_t		origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain) {
		// get current position
		BG_EvaluateTrajectory( &part->s.pos, level.time, origin );
		BG_EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->r.currentOrigin, move );
		VectorSubtract( angles, part->r.currentAngles, amove );
		if ( !VectorCompare( move, vec3_origin )
			|| !VectorCompare( amove, vec3_origin ) )
		{//actually moved
			if ( !G_MoverPush( part, move, amove, &obstacle ) ) {
				break;	// move was blocked
			}
		}
	}

	if (part) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s.pos, level.time, part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s.apos, level.time, part->r.currentAngles );
			trap->LinkEntity( (sharedEntity_t *)part );
		}

		// if the pusher has a "blocked" function, call it
		if (ent->blocked) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == TR_LINEAR_STOP ||
			part->s.pos.trType == TR_NONLINEAR_STOP) {
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

	// check think function
	G_RunThink( ent );
}
