// Native FX runner spawn and callbacks; included by g_fx.cpp and its fixture. GPL-2.0-only.
//----------------------------------------------------------

/*QUAKED fx_runner (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF ONESHOT DAMAGE
Runs the specified effect, can also be targeted at an info_notnull to orient the effect

	STARTOFF - effect starts off, toggles on/off when used
	ONESHOT - effect fires only when used
	DAMAGE - does radius damage around effect every "delay" milliseonds

	"fxFile" - name of the effect file to play
	"target" - direction to aim the effect in, otherwise defaults to up
	"target2" - uses its target2 when the fx gets triggered
	"delay"  - how often to call the effect, don't over-do this ( default 200 )
	"random" - random amount of time to add to delay, ( default 0, 200 = 0ms to 200ms )
	"splashRadius" - only works when damage is checked ( default 16 )
	"splashDamage" - only works when damage is checked ( default 5 )
	"soundset"	- bmodel set to use, plays start sound when toggled on, loop sound while on ( doesn't play on a oneshot), and a stop sound when turned off
*/
#define FX_RUNNER_RESERVED 0x800000

#include "g_bsp_section_fx.inc"

static void FXRunnerRefreshLoop(gentity_t* ent)
{
	if (const int sound=BSPSectionFXSound(*ent))
	{
		ent->s.loopSound = sound;
	}
	else if (VALIDSTRING(ent->soundSet))
	{
		ent->s.loopSound = CAS_GetBModelSound(ent->soundSet, BMS_MID);
		if (ent->s.loopSound < 0) ent->s.loopSound = 0;
	}
}

//----------------------------------------------------------
void fx_runner_think( gentity_t *ent )
{
	vec3_t temp;

	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
	EvaluateTrajectory( &ent->s.apos, level.time, ent->currentAngles );

	// call the effect with the desired position and orientation
	G_AddEvent( ent, EV_PLAY_EFFECT, ent->fxID );

	// Assume angles, we'll do a cross product on the other end to finish up
	AngleVectors( ent->currentAngles, ent->pos3, NULL, NULL );
	MakeNormalVectors( ent->pos3, ent->pos4, temp ); // there IS a reason this is done...it's so that it doesn't break every effect in the game...

	ent->nextthink = level.time + ent->delay + Q_flrand(0.0f, 1.0f) * ent->random;

	if ( ent->spawnflags & 4 ) // damage
	{
		G_RadiusDamage( ent->currentOrigin, ent, ent->splashDamage, ent->splashRadius, ent, MOD_UNKNOWN );
	}

	if ( ent->target2 )
	{
		// let our target know that we have spawned an effect
		G_UseTargets2( ent, ent, ent->target2 );
	}

	if ( !(ent->spawnflags & 2 ) && !ent->s.loopSound ) // NOT ONESHOT...this is an assy thing to do
	{
		FXRunnerRefreshLoop(ent);
	}

}

//----------------------------------------------------------
void fx_runner_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if (self->s.isPortalEnt)
	{ //rww - mark it as broadcast upon first use if it's within the area of a skyportal
		self->svFlags |= SVF_BROADCAST;
	}

	if ( self->spawnflags & 2 ) // ONESHOT
	{
		// call the effect with the desired position and orientation, as a safety thing,
		//	make sure we aren't thinking at all.
		fx_runner_think( self );
		self->nextthink = -1;

		if ( self->target2 )
		{
			// let our target know that we have spawned an effect
			G_UseTargets2( self, self, self->target2 );
		}

		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
		}
	}
	else
	{
		// ensure we are working with the right think function
		self->e_ThinkFunc = thinkF_fx_runner_think;

		// toggle our state
		if ( self->nextthink == -1 )
		{
			// NOTE: we fire the effect immediately on use, the fx_runner_think func will set
			//	up the nextthink time.
			fx_runner_think( self );

			if ( VALIDSTRING( self->soundSet ) == true )
			{
				G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
				self->s.loopSound = CAS_GetBModelSound( self->soundSet, BMS_MID );

				if ( self->s.loopSound < 0 )
				{
					self->s.loopSound = 0;
				}
			}
		}
		else
		{
			// turn off for now
			self->nextthink = -1;
			if (BSPSectionFXSound(*self)) self->s.loopSound = 0;

			if ( VALIDSTRING( self->soundSet ) == true )
			{
				G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_END ));
				self->s.loopSound = 0;
			}
		}
	}
}

//----------------------------------------------------------
void fx_runner_link( gentity_t *ent )
{
	vec3_t	dir;

	if ( ent->target && !BSPSectionFXSound(*ent) )
	{
		// try to use the target to override the orientation
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target );

		if ( !target )
		{
			// Bah, no good, dump a warning, but continue on and use the UP vector
			Com_Printf( "fx_runner_link: target specified but not found: %s\n", ent->target );
			Com_Printf( "  -assuming UP orientation.\n" );
		}
		else
		{
			// Our target is valid so let's override the default UP vector
			VectorSubtract( target->s.origin, ent->s.origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ent->s.angles );
		}
	}

	// don't really do anything with this right now other than do a check to warn the designers if the target2 is bogus
	if ( ent->target2 )
	{
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target2 );

		if ( !target )
		{
			// Target2 is bogus, but we can still continue
			Com_Printf( "fx_runner_link: target2 was specified but is not valid: %s\n", ent->target2 );
		}
	}

	G_SetAngles( ent, ent->s.angles );

	if ( ent->spawnflags & 1 || ent->spawnflags & 2 ) // STARTOFF || ONESHOT
	{
		// We won't even consider thinking until we are used
		ent->nextthink = -1;
	}
	else
	{
		FXRunnerRefreshLoop(ent);

		// Let's get to work right now!
		ent->e_ThinkFunc = thinkF_fx_runner_think;
		ent->nextthink = level.time + 200; // wait a small bit, then start working
	}

	// make us useable if we can be targeted
	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_fx_runner_use;
	}
}

//----------------------------------------------------------
void SP_fx_runner( gentity_t *ent )
{
	// Get our defaults
	G_SpawnInt( "delay", "200", &ent->delay );
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnInt( "splashRadius", "16", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "5", &ent->splashDamage );

	if ( !G_SpawnAngleHack( "angle", "0", ent->s.angles ))
	{
		// didn't have angles, so give us the default of up
		VectorSet( ent->s.angles, -90, 0, 0 );
	}

	if ( !ent->fxFile )
	{
		gi.Printf( S_COLOR_RED"ERROR: fx_runner %s at %s has no fxFile specified\n", ent->targetname, vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}

	// Try and associate an effect file, unfortunately we won't know if this worked or not
	//	until the CGAME trys to register it...
	ent->fxID = G_EffectIndex( ent->fxFile );

	ent->s.eType = ET_MOVER;

	// Give us a bit of time to spawn in the other entities, since we may have to target one of 'em
	ent->e_ThinkFunc = thinkF_fx_runner_link;
	ent->nextthink = level.time + 400;

	// Save our position and link us up!
	G_SetOrigin( ent, ent->s.origin );

	VectorSet( ent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( ent->maxs, -1, ent->mins );

	gi.linkentity( ent );
}


