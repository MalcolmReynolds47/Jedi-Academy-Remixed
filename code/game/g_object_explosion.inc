// Native explosive object callbacks, extracted from g_combat.cpp.
// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void ObjectDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	if(self->target)
		G_UseTargets(self, attacker);

	//remove my script_targetname
	G_FreeEntity( self );
}
/*
==================
ExplodeDeath
==================
*/

//FIXME: all hacked up...

void ExplodeDeath( gentity_t *self )
{
//	gentity_t	*tent;
	vec3_t		forward;

	self->takedamage = qfalse;//stop chain reaction runaway loops

	self->s.loopSound = 0;

	VectorCopy( self->currentOrigin, self->s.pos.trBase );

//	tent = G_TempEntity( self->s.origin, EV_FX_EXPLOSION );
	AngleVectors(self->s.angles, forward, NULL, NULL);  // FIXME: letting effect always shoot up?  Might be ok.

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, forward );
	}
//	else
//	{
//		CG_SurfaceExplosion( self->currentOrigin, forward, 20.0f, 12.0f, ((self->spawnflags&4)==qfalse) );	//FIXME: This needs to be consistent to all exploders!
//		G_Sound(self, self->sounds );
//	}

	if(self->splashDamage > 0 && self->splashRadius > 0)
	{
		gentity_t *attacker = self;
		if ( self->owner )
		{
			attacker = self->owner;
		}
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius,
				attacker, MOD_UNKNOWN );
	}

	ObjectDie( self, self, self, 20, 0 );
}

void ExplodeDeath_Wait( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
{
	self->e_DieFunc = dieF_NULL;
	self->nextthink = level.time + Q_irand(100, 500);
	self->e_ThinkFunc = thinkF_ExplodeDeath;
}

void ExplodeDeath( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
{
	self->currentOrigin[2] += 16; // me bad for hacking this.  should either do it in the effect file or make a custom explode death??
	ExplodeDeath( self );
}

void GoExplodeDeath( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	self->targetname = NULL;	//Make sure this entity cannot be told to explode again (recursive death fix)

	ExplodeDeath( self );
}

