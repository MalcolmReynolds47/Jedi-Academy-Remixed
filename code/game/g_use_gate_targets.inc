// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
void G_SetActiveState(char *targetstring, qboolean actState)
{
	gentity_t	*target = NULL;
	while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
	{
		target->svFlags = actState ? (target->svFlags&~SVF_INACTIVE) : (target->svFlags|SVF_INACTIVE);
	}
}

void target_activate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (G_JAR_WorkshopUseGate(*self, other, activator)) return;
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, qtrue);
}

void target_deactivate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (G_JAR_WorkshopUseGate(*self, other, activator)) return;
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, qfalse);
}

//FIXME: make these apply to doors, etc too?
/*QUAKED target_activate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be usable/triggerable
*/
void SP_target_activate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_activate_use;
}

/*QUAKED target_deactivate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be non-usable/triggerable
*/
void SP_target_deactivate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_deactivate_use;
}


