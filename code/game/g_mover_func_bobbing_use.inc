// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void func_bobbing_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// Toggle our move state
	if ( self->s.pos.trType == TR_SINE )
	{
		self->s.pos.trType = TR_INTERPOLATE;

		// Save off roughly where we were
		VectorCopy( self->currentOrigin, self->s.pos.trBase );
		// Store the current phase value so we know how to start up where we left off.
		self->radius = ( level.time - self->s.pos.trTime ) / (float)self->s.pos.trDuration;
	}
	else
	{
		self->s.pos.trType = TR_SINE;

		// Set the time based on the saved phase value
		self->s.pos.trTime = level.time - self->s.pos.trDuration * self->radius;
		// Always make sure we are starting with a fresh base
		VectorCopy( self->s.origin, self->s.pos.trBase );
	}
}
