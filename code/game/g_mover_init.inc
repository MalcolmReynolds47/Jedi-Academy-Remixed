// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void InitMover( gentity_t *ent )
{
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		if ( strstr( ent->model2, ".glm" ))
		{
			ent->s.modelindex2 = G_ModelIndex( ent->model2 );
			ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, ent->model2, ent->s.modelindex2, NULL_HANDLE, NULL_HANDLE, 0, 0 );
			if ( ent->playerModel >= 0 )
			{
				ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
			}

			ent->s.radius = 120;
		}
		else
		{
			ent->s.modelindex2 = G_ModelIndex( ent->model2 );
		}
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->e_UseFunc	   = useF_Use_BinaryMover;
	ent->e_ReachedFunc = reachedF_Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->svFlags = SVF_USE_CURRENT_ORIGIN;
	if ( ent->spawnflags & MOVER_INACTIVE )
	{// Make it inactive
		ent->svFlags |= SVF_INACTIVE;
	}
	if(ent->spawnflags & MOVER_PLAYER_USE)
	{//Can be used by the player's BUTTON_USE
		ent->svFlags |= SVF_PLAYER_USABLE;
	}
	ent->s.eType = ET_MOVER;
	VectorCopy( ent->pos1, ent->currentOrigin );
	gi.linkentity( ent );

	InitMoverTrData( ent );
}
