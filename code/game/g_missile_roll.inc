// Native rolling missile movement and per-segment Sandbox portal contact.
// GPL-2.0-only.
/*
==================

G_GroundTrace

==================
*/
static int G_GroundTrace( gentity_t *ent, pml_t *pPml )
{
	vec3_t		point;
	trace_t		trace;

	point[0] = ent->currentOrigin[0];
	point[1] = ent->currentOrigin[1];
	point[2] = ent->currentOrigin[2] - 0.25;

	gi.trace ( &trace, ent->currentOrigin, ent->mins, ent->maxs, point, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
	pPml->groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid )
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 )
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// check if getting thrown off the ground
	if ( ent->s.pos.trDelta[2] > 0 && DotProduct( ent->s.pos.trDelta, trace.plane.normal ) > 10 )
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL )
	{
		pPml->groundPlane = qtrue;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	pPml->groundPlane = qtrue;
	pPml->walking = qtrue;

	/*
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE )
	{
		// just hit the ground
	}
	*/

	return trace.entityNum;
}

static void G_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce )
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ )
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}
/*
==================

G_RollMissile

reworking the rolling object code,
still needs to stop bobbling up & down,
need to get roll angles right,
and need to maybe make the transfer of velocity happen on impacts?
Also need bounce sound for bounces off a floor.
Also need to not bounce as much off of enemies
Also gets stuck inside thrower if looking down when thrown

==================
*/
#define	MAX_CLIP_PLANES	5
#define	BUMPCLIP		1.5f
static bool G_RollMissile( gentity_t *ent )
{
	int			bumpcount, numbumps;
	bool allowPortal = true;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	pml_t		objPML;
	float		bounceAmt = BUMPCLIP;
	gentity_t	*hitEnt = NULL;

	memset( &objPML, 0, sizeof( objPML ) );

	G_GroundTrace( ent, &objPML );

	objPML.frametime = (level.time - level.previousTime)*0.001;

	numbumps = 4;

	VectorCopy ( ent->s.pos.trDelta, primal_velocity );

	VectorCopy( ent->s.pos.trDelta, endVelocity );
	endVelocity[2] -= g_gravity->value * objPML.frametime;
	ent->s.pos.trDelta[2] = ( ent->s.pos.trDelta[2] + endVelocity[2] ) * 0.5;
	primal_velocity[2] = endVelocity[2];
	if ( objPML.groundPlane )
	{//FIXME: never happens!
		// slide along the ground plane
		G_ClipVelocity( ent->s.pos.trDelta, objPML.groundTrace.plane.normal, ent->s.pos.trDelta, BUMPCLIP );
		VectorScale( ent->s.pos.trDelta, 0.9f, ent->s.pos.trDelta );
	}

	time_left = objPML.frametime;

	// never turn against the ground plane
	if ( objPML.groundPlane )
	{
		numplanes = 1;
		VectorCopy( objPML.groundTrace.plane.normal, planes[0] );
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	/*
	VectorNormalize2( ent->s.pos.trDelta, planes[numplanes] );
	numplanes++;
	*/

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		// calculate position we are trying to move to
		VectorMA( ent->currentOrigin, time_left, ent->s.pos.trDelta, end );

		// see if we can make it there
		gi.trace ( &trace, ent->currentOrigin, ent->mins, ent->maxs, end, ent->s.number, ent->clipmask, G2_RETURNONHIT, 10 );

		// Each actual movement segment can enter a portal before its impact.
		// A chord from the original position after all bounces could cross a wall.
		if ( allowPortal && ent->s.weapon == WP_THERMAL && !trace.startsolid && !trace.allsolid &&
			 G_JAR_WorkshopTeleportProjectile( *ent, ent->currentOrigin, trace.endpos, trace.fraction < 1, endVelocity ) )
		{
			return true;
		}
		if ( trace.fraction < 1 && (trace.surfaceFlags & SURF_NOIMPACT) )
			allowPortal = false;

		//TEMP HACK:
		//had to move this up above the trace.allsolid check now that for some reason ghoul2 impacts tell me I'm allsolid?!
		//this needs to be fixed, really
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//hit another ent
			hitEnt = &g_entities[trace.entityNum];
			if ( hitEnt && (hitEnt->takedamage || (hitEnt->contents&CONTENTS_LIGHTSABER) ) )
			{
				G_MissileImpact( ent, &trace );
				if ( !ent->inuse || ent->s.eType != ET_MISSILE )
				{//exploded
					return false;
				}
			}
		}

		if ( trace.allsolid )
		{
			// entity is completely trapped in another solid
			//FIXME: this happens a lot now when we hit a G2 ent... WTF?
			ent->s.pos.trDelta[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return false;// qtrue;
		}

		if ( trace.fraction > 0 )
		{
			// actually covered some distance
			VectorCopy( trace.endpos, ent->currentOrigin );
		}

		if ( trace.fraction == 1 )
		{
			 break;		// moved the entire distance
		}

		//pm->ps->pm_flags |= PMF_BUMPED;

		// save entity for contact
		//PM_AddTouchEnt( trace.entityNum );

		//Hit it
		/*
		if ( PM_ClientImpact( trace.entityNum, qtrue ) )
		{
			continue;
		}
		*/

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES )
		{
			// this shouldn't really happen
			VectorClear( ent->s.pos.trDelta );
			return false;// qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0 ; i < numplanes ; i++ )
		{
			if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 )
			{
				VectorAdd( trace.plane.normal, ent->s.pos.trDelta, ent->s.pos.trDelta );
				break;
			}
		}
		if ( i < numplanes )
		{
			continue;
		}
		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//
		if ( g_entities[trace.entityNum].inuse && g_entities[trace.entityNum].client )
		{//hit a person, bounce off much less
			bounceAmt = OVERCLIP;
		}
		else
		{
			bounceAmt = BUMPCLIP;
		}

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ )
		{
			into = DotProduct( ent->s.pos.trDelta, planes[i] );
			if ( into >= 0.1 )
			{
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed )
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			G_ClipVelocity( ent->s.pos.trDelta, planes[i], clipVelocity, bounceAmt );

			// slide along the plane
			G_ClipVelocity( endVelocity, planes[i], endClipVelocity, bounceAmt );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ )
			{
				if ( j == i )
				{
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 )
				{
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				G_ClipVelocity( clipVelocity, planes[j], clipVelocity, bounceAmt );
				G_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, bounceAmt );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 )
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, ent->s.pos.trDelta );
				VectorScale( dir, d, clipVelocity );

				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ )
				{
					if ( k == i || k == j )
					{
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 )
					{
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear( ent->s.pos.trDelta );
					return false;// qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, ent->s.pos.trDelta );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
		VectorScale( endVelocity, 0.975f, endVelocity );
	}

	VectorCopy( endVelocity, ent->s.pos.trDelta );

	// don't change velocity if in a timer (FIXME: is this correct?)
	/*
	if ( pm->ps->pm_time )
	{
		VectorCopy( primal_velocity, ent->s.pos.trDelta );
	}
	*/

	return false;// ( bumpcount != 0 );
}
