// Native laser trace shared with portable Sandbox beams. GPL-2.0-only.
void JAR_MP_NativeLaserTrace(gentity_t*self,int pass,trace_t*tr){
	vec3_t end,point;
	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA (point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	trap->Trace( tr, self->s.origin, NULL, NULL, end, pass, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE, qfalse, 0, 0);

}

void target_laser_think (gentity_t *self) {
	trace_t tr;
	JAR_MP_NativeLaserTrace(self,self->s.number,&tr);
	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir,
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy (tr.endpos, self->s.origin2);

	trap->LinkEntity( (sharedEntity_t *)self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_off (gentity_t *self)
{
	trap->UnlinkEntity( (sharedEntity_t *)self );
	self->nextthink = 0;
}

