// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
void ReturnToPos1( gentity_t *ent ) {
	ent->e_ThinkFunc = thinkF_NULL;
	ent->nextthink = 0;
	ent->s.time = level.time;

	MatchTeam( ent, MOVER_2TO1, level.time );

	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??
}


/*
================
Reached_BinaryMover
================
*/

void Reached_BinaryMover( gentity_t *ent )
{
	// stop the looping sound
	ent->s.loopSound = 0;

	if ( ent->moverState == MOVER_1TO2 )
	{//reached open
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}

		// play sound
		G_PlayDoorSound( ent, BMS_END );

		if ( ent->wait < 0 )
		{//Done for good
			ent->e_ThinkFunc = thinkF_NULL;
			ent->nextthink = -1;
			ent->e_UseFunc = useF_NULL;
		}
		else
		{
			// return to pos1 after a delay
			ent->e_ThinkFunc = thinkF_ReturnToPos1;
			if(ent->spawnflags & 8)
			{//Toggle, keep think, wait for next use?
				ent->nextthink = -1;
			}
			else
			{
				ent->nextthink = level.time + ent->wait;
			}
		}

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}
		G_UseTargets2( ent, ent->activator, ent->opentarget );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{//closed
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}
		// play sound
		G_PlayDoorSound( ent, BMS_END );

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster )
		{
			G_AdjustMoverAreaPortalState( ent, qfalse );
		}
		G_UseTargets2( ent, ent->activator, ent->closetarget );
	}
	else
	{
		G_Error( "Reached_BinaryMover: bad moverState" );
	}
}


/*
================
Use_BinaryMover_Go
================
*/
void Use_BinaryMover_Go( gentity_t *ent )
{
	int		total;
	int		partial;
//	gentity_t	*other = ent->enemy;
	gentity_t	*activator = ent->activator;

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 )
	{
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}

		// starting sound
		G_PlayDoorLoopSound( ent );
		G_PlayDoorSound( ent, BMS_START );
		ent->s.time = level.time;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			G_AdjustMoverAreaPortalState( ent, qtrue );
		}
		G_UseTargets( ent, ent->activator );
		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2 ) {
		//have to do this because the delay sets our think to Use_BinaryMover_Go
		ent->e_ThinkFunc = thinkF_ReturnToPos1;
		if ( ent->spawnflags & 8 )
		{//TOGGLE doors don't use wait!
			ent->nextthink = level.time + FRAMETIME;
		}
		else
		{
			ent->nextthink = level.time + ent->wait;
		}
		G_UseTargets2( ent, ent->activator, ent->target2 );
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 )
	{
		total = ent->s.pos.trDuration-50;
		if ( ent->s.pos.trType == TR_NONLINEAR_STOP )
		{
			vec3_t curDelta;
			VectorSubtract( ent->currentOrigin, ent->pos1, curDelta );
			float fPartial = VectorLength( curDelta )/VectorLength( ent->s.pos.trDelta );
			VectorScale( ent->s.pos.trDelta, fPartial, curDelta );
			fPartial /= ent->s.pos.trDuration;
			fPartial /= 0.001f;
			fPartial = acos( fPartial );
			fPartial = RAD2DEG( fPartial );
			fPartial = (90.0f - fPartial)/90.0f*ent->s.pos.trDuration;
			partial = total - floor( fPartial );
		}
		else
		{
			partial = level.time - ent->s.pos.trTime;//ent->s.time;
		}

		if ( partial > total ) {
			partial = total;
		}
		ent->s.pos.trTime = level.time - ( total - partial );//ent->s.time;

		MatchTeam( ent, MOVER_1TO2, ent->s.pos.trTime );

		G_PlayDoorSound( ent, BMS_START );

		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 )
	{
		total = ent->s.pos.trDuration-50;
		if ( ent->s.pos.trType == TR_NONLINEAR_STOP )
		{
			vec3_t curDelta;
			VectorSubtract( ent->currentOrigin, ent->pos2, curDelta );
			float fPartial = VectorLength( curDelta )/VectorLength( ent->s.pos.trDelta );
			VectorScale( ent->s.pos.trDelta, fPartial, curDelta );
			fPartial /= ent->s.pos.trDuration;
			fPartial /= 0.001f;
			fPartial = acos( fPartial );
			fPartial = RAD2DEG( fPartial );
			fPartial = (90.0f - fPartial)/90.0f*ent->s.pos.trDuration;
			partial = total - floor( fPartial );
		}
		else
		{
			partial = level.time - ent->s.pos.trTime;//ent->s.time;
		}
		if ( partial > total ) {
			partial = total;
		}

		ent->s.pos.trTime = level.time - ( total - partial );//ent->s.time;
		MatchTeam( ent, MOVER_2TO1, ent->s.pos.trTime );

		G_PlayDoorSound( ent, BMS_START );

		return;
	}
}

void UnLockDoors(gentity_t *const ent)
{
	//noise?
	//go through and unlock the door and all the slaves
	gentity_t	*slave = ent;
	do
	{	// want to allow locked toggle doors, so keep the targetname
		if( !(slave->spawnflags & MOVER_TOGGLE) )
		{
			slave->targetname = NULL;//not usable ever again
		}
		slave->spawnflags &= ~MOVER_LOCKED;
		slave->s.frame = 1;//second stage of anim
		slave = slave->teamchain;
	} while  ( slave );
}
void LockDoors(gentity_t *const ent)
{
	//noise?
	//go through and lock the door and all the slaves
	gentity_t	*slave = ent;
	do
	{
		slave->spawnflags |= MOVER_LOCKED;
		slave->s.frame = 0;//first stage of anim
		slave = slave->teamchain;
	} while  ( slave );
}
/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	int	key;
	const char *text;

	if ( ent->e_UseFunc == useF_NULL )
	{//I cannot be used anymore, must be a door with a wait of -1 that's opened.
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE )
	{
		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	if ( ent->svFlags & SVF_INACTIVE )
	{
		return;
	}

	if ( ent->spawnflags & MOVER_LOCKED )
	{//a locked door, unlock it
		UnLockDoors(ent);
		return;
	}

	if ( ent->spawnflags & MOVER_GOODIE )
	{
		if ( ent->fly_sound_debounce_time > level.time )
		{
			return;
		}
		else
		{
			key = INV_GoodieKeyCheck( activator );
			if (key)
			{//activator has a goodie key, remove it
				activator->client->ps.inventory[key]--;
				G_Sound( activator, G_SoundIndex( "sound/movers/goodie_pass.wav" ) );
				// once the goodie mover has been used, it no longer requires a goodie key
				ent->spawnflags &= ~MOVER_GOODIE;
			}
			else
			{	//don't have a goodie key
				G_Sound( activator, G_SoundIndex( "sound/movers/goodie_fail.wav" ) );
				ent->fly_sound_debounce_time = level.time + 5000;
				text = "cp @SP_INGAME_NEED_KEY_TO_OPEN";
				//FIXME: temp message, only on certain difficulties?, graphic instead of text?
				gi.SendServerCommand( 0, text );
				return;
			}
		}
	}

	G_ActivateBehavior(ent,BSET_USE);

	G_SetEnemy( ent, other );
	ent->activator = activator;
	if(ent->delay)
	{
		ent->e_ThinkFunc = thinkF_Use_BinaryMover_Go;
		ent->nextthink = level.time + ent->delay;
	}
	else
	{
		Use_BinaryMover_Go(ent);
	}
}
