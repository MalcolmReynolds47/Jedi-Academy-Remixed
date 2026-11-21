// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_func_door (gentity_t *ent)
{
	float	distance;
	float	lip;

	ent->e_BlockedFunc = blockedF_Blocked_Door;

	if ( ent->spawnflags & MOVER_GOODIE )
	{
		G_SoundIndex( "sound/movers/goodie_fail.wav" );
		G_SoundIndex( "sound/movers/goodie_pass.wav" );
	}

	// default speed of 400
	if (!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	ent->delay *= 1000;

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );
	if ( ent->damage < 0 )
	{
		ent->damage = 0;
	}

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	G_JAR_SetBrushModel( ent, ent->model );
	jar::subBSPMover::MoveDirection(*ent);
	distance = jar::subBSPMover::Distance(*ent, ent->movedir, lip);
	jar::subBSPMover::Orient(*ent);
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 )
	{
		vec3_t	temp;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );
	}

	if ( ent->spawnflags & MOVER_LOCKED )
	{//a locked door, set up as locked until used directly
		ent->s.eFlags |= EF_SHADER_ANIM;//use frame-controlled shader anim
		ent->s.frame = 0;//first stage of anim
	}
	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( !(ent->flags&FL_TEAMSLAVE) )
	{
		int health;

		G_SpawnInt( "health", "0", &health );

		if ( health )
		{
			ent->takedamage = qtrue;
		}

		if ( !(ent->spawnflags&MOVER_LOCKED) && (ent->targetname || health || ent->spawnflags & MOVER_PLAYER_USE || ent->spawnflags & MOVER_FORCE_ACTIVATE) )
		{
			// non touch/shoot doors
			ent->e_ThinkFunc = thinkF_Think_MatchTeam;
		}
		else
		{//locked doors still spawn a trigger
			ent->e_ThinkFunc = thinkF_Think_SpawnNewDoorTrigger;
		}
	}
}
