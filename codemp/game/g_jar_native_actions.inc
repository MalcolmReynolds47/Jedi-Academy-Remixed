// Native target callbacks; owned Sandbox actions use identity-based adapters. GPL-2.0-only.
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if(JAR_MP_GiveTarget(ent,activator))return;
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap->UnlinkEntity( (sharedEntity_t *)t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}

void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if(JAR_MP_ActionTarget(ent,activator))return;
	AddScore( activator, ent->r.currentOrigin, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}

void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if(JAR_MP_ActionTarget(self,activator))return;
	G_ActivateBehavior(self,BSET_USE);
	G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}
