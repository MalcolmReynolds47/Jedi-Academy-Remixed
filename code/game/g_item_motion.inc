// Native single-player item physics (Raven Software and OpenJK contributors). GPL-2.0-only.
void G_RunItem( gentity_t *ent ) {
	if (G_BSPSectionPickupRoot(*ent) || G_JAR_WorkshopRackPickupSuspended(*ent)) {
		G_RunThink(ent);
		return;
	}
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE )
	{
		if ( ent->s.pos.trType != TR_GRAVITY )
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY )
	{
		// check think function
		G_RunThink( ent );
		if ( !g_gravity->value )
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDelta[0] += Q_flrand(-1.0f, 1.0f) * 40.0f; // I dunno, just do this??
			ent->s.pos.trDelta[1] += Q_flrand(-1.0f, 1.0f) * 40.0f;
			ent->s.pos.trDelta[2] += Q_flrand(0.0f, 1.0f) * 20.0f;
		}
		else if ( (ent->flags&FL_DROPPED_ITEM)
			&& ent->item
			&& ent->item->giType == IT_WEAPON
			&& ent->item->giTag == WP_SABER )
		{//a dropped saber item, check below, just in case
			int ignore = ENTITYNUM_NONE;
			if ( ent->clipmask )
			{
				mask = ent->clipmask;
			}
			else
			{
				mask = MASK_SOLID|CONTENTS_PLAYERCLIP;//shouldn't be able to get anywhere player can't
			}
			if ( ent->owner )
			{
				ignore = ent->owner->s.number;
			}
			else if ( ent->activator )
			{
				ignore = ent->activator->s.number;
			}
			VectorSet( origin, ent->currentOrigin[0], ent->currentOrigin[1], ent->currentOrigin[2]-1 );
			gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, origin, ignore, mask, (EG2_Collision)0, 0 );
			if ( !tr.allsolid
				&& !tr.startsolid
				&& tr.fraction > 0.001f )
			{//wha?  fall....
				ent->s.pos.trType = TR_GRAVITY;
				ent->s.pos.trTime = level.time;
			}
		}
		return;
	}

	// get current position
	EvaluateTrajectory( &ent->s.pos, level.time, origin );
	if ( ent->s.apos.trType != TR_STATIONARY )
	{
		EvaluateTrajectory( &ent->s.apos, level.time, ent->currentAngles );
		G_SetAngles( ent, ent->currentAngles );
	}

	// trace a line from the previous position to the current position
	if ( ent->clipmask )
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID|CONTENTS_PLAYERCLIP;//shouldn't be able to get anywhere player can't
	}

	int ignore = ENTITYNUM_NONE;
	if ( ent->owner )
	{
		ignore = ent->owner->s.number;
	}
	else if ( ent->activator )
	{
		ignore = ent->activator->s.number;
	}
	gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, origin, ignore, mask, (EG2_Collision)0, 0 );

	VectorCopy( tr.endpos, ent->currentOrigin );

	if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	gi.linkentity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 )
	{
		if ( g_gravity->value <= 0 )
		{
			if ( ent->s.apos.trType != TR_LINEAR )
			{
				VectorCopy( ent->currentAngles, ent->s.apos.trBase );
				ent->s.apos.trType = TR_LINEAR;
				ent->s.apos.trDelta[1] = Q_flrand( -300, 300 );
				ent->s.apos.trDelta[0] = Q_flrand( -10, 10 );
				ent->s.apos.trDelta[2] = Q_flrand( -10, 10 );
				ent->s.apos.trTime = level.time;
			}
		}
		//friction in zero-G
		if ( !g_gravity->value )
		{
			float friction = 0.975f;
			/*friction -= ent->mass/1000.0f;
			if ( friction < 0.1 )
			{
				friction = 0.1f;
			}
			*/
			VectorScale( ent->s.pos.trDelta, friction, ent->s.pos.trDelta );
			VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
		}
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = gi.pointcontents( ent->currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP )
	{
		G_FreeEntity( ent );
		return;
	}

	if ( !tr.startsolid )
	{
		G_BounceItem( ent, &tr );
	}
}
