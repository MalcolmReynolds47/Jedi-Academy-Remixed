// Native callbacks: push velocity adapter and owned teleporter destination hook.
// Authored-map paths retain the original behavior. GPL-2.0-only.
void JAR_MP_NativePush( gentity_t *self, gentity_t *other, gentity_t *activator, const vec3_t velocity ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL && activator->client->ps.pm_type != PM_FLOAT ) {
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	VectorCopy (velocity, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if ( activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		if (self->noise_index)
		{
			G_Sound( activator, CHAN_AUTO, self->noise_index );
		}
	}
}

void Use_target_push(gentity_t*self,gentity_t*other,gentity_t*activator){
 if(JAR_MP_PushTarget(self,other,activator))return;
 JAR_MP_NativePush(self,other,activator,self->s.origin2);
}

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;
	if (JAR_MP_TeleportTarget(self,activator)) return;

	if (!activator->client)
		return;

	G_ActivateBehavior(self,BSET_USE);

	dest = 	G_PickTarget( self->target );
	if (!dest) {
		trap->Print ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}
