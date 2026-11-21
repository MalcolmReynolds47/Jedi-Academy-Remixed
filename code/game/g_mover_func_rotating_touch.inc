// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void func_rotating_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
//	vec3_t spot;
//	gentity_t	*tent;
	if( !other->client ) // don't want to disintegrate items or weapons, etc...
	{
		return;
	}
	// yeah, this is a bit hacky...
	if(	!jar::angular::Stationary(self->s.apos.trType) && !jar::angular::Interpolates(self->s.apos.trType) && !(other->flags & FL_DISINTEGRATED) ) // only damage if it's moving
	{
//		VectorCopy( self->currentOrigin, spot );
//		tent = G_TempEntity( spot, EV_DISINTEGRATION );
//		tent->s.eventParm = PW_DISRUPTION;
//		tent->owner = self;
		// let G_Damage call the fx instead, beside, this way you can disintegrate a corpse this way
		G_Sound( other, G_SoundIndex( "sound/effects/energy_crackle.wav" ) );
		G_Damage( other, self, self, NULL, NULL, 10000, DAMAGE_NO_KNOCKBACK, MOD_SNIPER );
	}
}
