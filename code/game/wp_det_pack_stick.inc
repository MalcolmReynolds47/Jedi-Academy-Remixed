// Native planting callback shared with the Sandbox charge fixture. GPL-2.0-only.
//---------------------------------------------------------
void charge_stick( gentity_t *self, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	self->s.eType = ET_GENERAL;

	// make us so we can take damage
	self->clipmask = MASK_SHOT;
	self->contents = CONTENTS_SHOTCLIP;
	self->takedamage = qtrue;
	self->health = 25;

	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet( self->maxs, 10, 10, 10 );
	VectorScale( self->maxs, -1, self->mins );

	self->activator = self->owner;
	self->owner = NULL;

	self->e_TouchFunc = touchF_NULL;
	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;

	WP_Stick( self, trace, 1.0f );
	if(trace->entityNum==ENTITYNUM_WORLD)self->s.groundEntityNum=ENTITYNUM_WORLD;
	if(other)G_SandboxChargeAttach(*self,*other);
}

