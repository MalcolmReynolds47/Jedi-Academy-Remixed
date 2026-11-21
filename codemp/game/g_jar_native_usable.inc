// Native func_usable callbacks from g_mover.c, unchanged. GPL-2.0-only.
extern void func_wait_return_solid(gentity_t*);
void func_usable_think( gentity_t *self )
{
	if ( self->spawnflags & 8 )
	{
		self->r.svFlags |= SVF_PLAYER_USABLE;	//Replace the usable flag
		self->use = func_usable_use;
		self->think = 0;
	}
}

void func_usable_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{//Toggle on and off
	G_ActivateBehavior( self, BSET_USE );
	if ( self->s.eFlags & EF_SHADER_ANIM )
	{//animate shader when used
		self->s.frame++;//inc frame
		if ( self->s.frame > self->genericValue5 )
		{//wrap around
			self->s.frame = 0;
		}
		if ( self->target && self->target[0] )
		{
			G_UseTargets( self, activator );
		}
	}
	else if ( self->spawnflags & 8 )
	{//ALWAYS_ON
		//Remove the ability to use the entity directly
		self->r.svFlags &= ~SVF_PLAYER_USABLE;
		//also remove ability to call any use func at all!
		self->use = 0;

		if(self->target && self->target[0])
		{
			G_UseTargets(self, activator);
		}

		if ( self->wait )
		{
			self->think = func_usable_think;
			self->nextthink = JAR_NumberDeadline(level.time, (double)self->wait * 1000);
		}

		return;
	}
	else if ( !self->count )
	{//become solid again
		self->count = 1;
		func_wait_return_solid( self );
	}
	else
	{
		self->s.solid = 0;
		self->r.contents = 0;
		self->clipmask = 0;
		self->r.svFlags |= SVF_NOCLIENT;
		self->s.eFlags |= EF_NODRAW;
		self->count = 0;

		if(self->target && self->target[0])
		{
			G_UseTargets(self, activator);
		}
		self->think = 0;
		self->nextthink = -1;
	}
}

void func_usable_pain(gentity_t *self, gentity_t *attacker, int damage)
{
	GlobalUse(self, attacker, attacker);
}

void func_usable_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	self->takedamage = qfalse;
	GlobalUse(self, inflictor, attacker);
}
