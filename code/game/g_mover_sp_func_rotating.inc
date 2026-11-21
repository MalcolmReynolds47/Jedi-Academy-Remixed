// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_func_rotating (gentity_t *ent) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}


	ent->s.apos.trType = TR_STATIONARY;
	if( ent->spawnflags & 1 )	// start on
	{
		ent->s.apos.trType = TR_LINEAR;
	}
	// set the axis of rotation
	if ( ent->spawnflags & 4 ) {
		ent->s.apos.trDelta[2] = ent->speed;
	} else if ( ent->spawnflags & 8 ) {
		ent->s.apos.trDelta[0] = ent->speed;
	} else {
		ent->s.apos.trDelta[1] = ent->speed;
	}

	if (!ent->damage) {
		ent->damage = 2;
	}


	G_JAR_SetBrushModel( ent, ent->model );
	InitMover( ent );

	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_func_rotating_use;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->currentAngles );
	if( ent->spawnflags & 2 )
	{
		ent->e_TouchFunc = touchF_func_rotating_touch;
		G_SoundIndex( "sound/effects/energy_crackle.wav" );
	}

	jar::subBSPMover::Angular(*ent);
	gi.linkentity( ent );
}
