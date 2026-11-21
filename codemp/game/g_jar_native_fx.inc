// Native Raven/OpenJK FX callbacks, unchanged from g_misc.c. GPL-2.0-only.
void fx_runner_think( gentity_t *ent )
{
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );

	// call the effect with the desired position and orientation
	if (ent->s.isPortalEnt)
	{
//		G_AddEvent( ent, EV_PLAY_PORTAL_EFFECT_ID, ent->genericValue5 );
	}
	else
	{
//		G_AddEvent( ent, EV_PLAY_EFFECT_ID, ent->genericValue5 );
	}

	// start the fx on the client (continuous)
	ent->s.modelindex2 = FX_STATE_CONTINUOUS;

	VectorCopy(ent->r.currentAngles, ent->s.angles);
	VectorCopy(ent->r.currentOrigin, ent->s.origin);

	ent->nextthink = JAR_NumberDeadline(level.time, ent->delay + (double)Q_flrand(0.0f, 1.0f) * ent->random);

	if ( ent->spawnflags & 4 ) // damage
	{
		G_RadiusDamage( ent->r.currentOrigin, ent, ent->splashDamage, ent->splashRadius, ent, ent, MOD_UNKNOWN );
	}

	if ( ent->target2 && ent->target2[0] )
	{
		// let our target know that we have spawned an effect
		G_UseTargets2( ent, ent, ent->target2 );
	}

	if ( !(ent->spawnflags & 2 ) && !ent->s.loopSound ) // NOT ONESHOT...this is an assy thing to do
	{
		if ( ent->soundSet && ent->soundSet[0] )
		{
			ent->s.soundSetIndex = G_SoundSetIndex(ent->soundSet);
			ent->s.loopIsSoundset = qtrue;
			ent->s.loopSound = BMS_MID;
		}
	}

}

//----------------------------------------------------------
void fx_runner_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if (self->s.isPortalEnt)
	{ //rww - mark it as broadcast upon first use if it's within the area of a skyportal
		self->r.svFlags |= SVF_BROADCAST;
	}

	if ( self->spawnflags & 2 ) // ONESHOT
	{
		// call the effect with the desired position and orientation, as a safety thing,
		//	make sure we aren't thinking at all.
		int		saveState = self->s.modelindex2 + 1;

		fx_runner_think( self );
		self->nextthink = -1;
		// one shot indicator
		self->s.modelindex2 = saveState;
		if (self->s.modelindex2 > FX_STATE_ONE_SHOT_LIMIT)
		{
			self->s.modelindex2 = FX_STATE_ONE_SHOT;
		}

		if ( self->target2 )
		{
			// let our target know that we have spawned an effect
			G_UseTargets2( self, self, self->target2 );
		}

		if ( self->soundSet && self->soundSet[0] )
		{
			self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
			G_AddEvent( self, EV_BMODEL_SOUND, BMS_START);
		}
	}
	else
	{
		// ensure we are working with the right think function
		self->think = fx_runner_think;

		// toggle our state
		if ( self->nextthink == -1 )
		{
			// NOTE: we fire the effect immediately on use, the fx_runner_think func will set
			//	up the nextthink time.
			fx_runner_think( self );

			if ( self->soundSet && self->soundSet[0] )
			{
				self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
				G_AddEvent( self, EV_BMODEL_SOUND, BMS_START);
				self->s.loopSound = BMS_MID;
				self->s.loopIsSoundset = qtrue;
			}
		}
		else
		{
			// turn off for now
			self->nextthink = -1;

			// turn off fx on client
			self->s.modelindex2 = FX_STATE_OFF;

			if ( self->soundSet && self->soundSet[0] )
			{
				self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
				G_AddEvent( self, EV_BMODEL_SOUND, BMS_END );
				self->s.loopSound = 0;
				self->s.loopIsSoundset = qfalse;
			}
		}
	}
}

//----------------------------------------------------------
