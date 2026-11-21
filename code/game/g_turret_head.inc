// Native Raven turret firing controller. GPL-2.0-only.
void turret_head_think( gentity_t *self )
//-----------------------------------------------------
{
	// if it's time to fire and we have an enemy, then gun 'em down!  pushDebounce time controls next fire time
	if ( self->enemy && self->pushDebounceTime < level.time && self->attackDebounceTime < level.time )
	{
		// set up our next fire time
		self->pushDebounceTime = level.time + self->wait;

		vec3_t		fwd, org;
		mdxaBone_t	boltMatrix;

		// Getting the flash bolt here
		if(!G_TurretMuzzleMatrix(self,self->currentOrigin,boltMatrix))return;
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->alt_fire = (qboolean)!self->alt_fire;
		}

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );

		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, fwd );
		}
		else
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, fwd );
		}

		VectorMA( org, START_DIS, fwd, org );

		turret_fire( self, org, fwd );
		self->fly_sound_debounce_time = level.time;//used as lastShotTime
	}
}
