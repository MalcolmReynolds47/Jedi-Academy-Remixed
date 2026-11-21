// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s;
	int		i;

	if ( VALIDSTRING( ent->soundSet ) )
	{
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		gi.linkentity (ent);
		return;
	}

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if(!ent->sounds)
	{
		if ( !G_SpawnString( "noise", "*NOSOUND*", &s ) ) {
			G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
		}

		Q_strncpyz( buffer, s, sizeof(buffer) );
		for(char* slash=buffer;*slash;++slash)if(*slash=='\\')*slash='/';
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		ent->noise_index = G_SoundIndex(buffer);
	}
	else
	{//Precache all possible sounds
		for( i = 0; i < ent->sounds; i++ )
		{
			ent->noise_index = G_SoundIndex( va( ent->paintarget, i+1 ) );
		}
	}

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;

	ent->wait *= 1000;

	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->e_UseFunc = useF_Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (ent);
}
