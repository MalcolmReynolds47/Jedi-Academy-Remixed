// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_func_bobbing (gentity_t *ent) {
	float		height;
	float		phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	G_JAR_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// set the axis of bobbing
	if ( ent->spawnflags & 1 ) {
		ent->s.pos.trDelta[0] = height;
	} else if ( ent->spawnflags & 2 ) {
		ent->s.pos.trDelta[1] = height;
	} else {
		ent->s.pos.trDelta[2] = height;
	}

	jar::subBSPMover::Vector(*ent, ent->s.pos.trDelta, ent->s.pos.trDelta);

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;

	if ( ent->spawnflags & 4 ) // start_off
	{
		ent->s.pos.trType = TR_INTERPOLATE;

		// Now use the phase to calculate where it should be at the start.
		ent->radius = phase;
		phase = (float)sin( phase * M_PI * 2 );
		VectorMA( ent->s.pos.trBase, phase, ent->s.pos.trDelta, ent->s.pos.trBase );

		if ( ent->targetname )
		{
			ent->e_UseFunc = useF_func_bobbing_use;
		}
	}
	else
	{
		ent->s.pos.trType = TR_SINE;
	}
	if (jar::subBSPMover::Root(*ent))
	{
		EvaluateTrajectory(&ent->s.pos, level.time, ent->currentOrigin);
		gi.linkentity(ent);
	}

}
