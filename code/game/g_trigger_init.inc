// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
void InitTrigger( gentity_t *self ) {
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	G_JAR_SetBrushModel( self, self->model );
	self->contents = CONTENTS_TRIGGER;		// replaces the -1 from gi.SetBrushModel
	self->svFlags = SVF_NOCLIENT;

	if(self->spawnflags & 128)
	{
		self->svFlags |= SVF_INACTIVE;
	}
}
