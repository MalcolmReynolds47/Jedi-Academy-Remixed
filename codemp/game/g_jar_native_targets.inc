// Native Raven/OpenJK target callbacks shared by authored maps and Sandbox.
// Moved unchanged from g_target.c for direct production-code fixtures. GPL-2.0-only.
extern void G_DebugPrint(int level,const char*format,...);

void Think_Target_Delay( gentity_t *ent ) {
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if (ent->nextthink > level.time && (ent->spawnflags & 1))
	{ //Leave me alone, I am thinking.
		return;
	}
	G_ActivateBehavior(ent,BSET_USE);
	ent->nextthink = JAR_NumberDeadline(level.time, (ent->wait + (double)ent->random * Q_flrand(-1.0f, 1.0f)) * 1000);
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}

void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (!ent || !ent->inuse)
	{
		Com_Printf("ERROR: Bad ent in Use_Target_Print");
		return;
	}

	if (ent->wait)
	{
		if (ent->genericValue14 >= level.time)
		{
			return;
		}
		ent->genericValue14 = level.time + ent->wait;
	}

#ifndef FINAL_BUILD
	if (!ent || !ent->inuse)
	{
	//	Com_Error(ERR_DROP, "Bad ent in Use_Target_Print");
		return;
	}
	else if (!activator || !activator->inuse)
	{
	//	Com_Error(ERR_DROP, "Bad activator in Use_Target_Print");
		return;
	}

	if (ent->genericValue15 > level.time)
	{
		Com_Printf("TARGET PRINT ERRORS:\n");
		if (activator && activator->classname && activator->classname[0])
		{
			Com_Printf("activator classname: %s\n", activator->classname);
		}
		if (activator && activator->target && activator->target[0])
		{
			Com_Printf("activator target: %s\n", activator->target);
		}
		if (activator && activator->targetname && activator->targetname[0])
		{
			Com_Printf("activator targetname: %s\n", activator->targetname);
		}
		if (ent->targetname && ent->targetname[0])
		{
			Com_Printf("print targetname: %s\n", ent->targetname);
		}
		Com_Error(ERR_DROP, "target_print used in quick succession, fix it! See the console for details.");
	}
	ent->genericValue15 = level.time + 5000;
#endif

	G_ActivateBehavior(ent,BSET_USE);
	if ( ( ent->spawnflags & 4 ) )
	{//private, to one client only
		if (!activator || !activator->inuse)
		{
			Com_Printf("ERROR: Bad activator in Use_Target_Print");
		}
		if ( activator && activator->client )
		{//make sure there's a valid client ent to send it to
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				trap->SendServerCommand( activator-g_entities, va("cps \"%s\"", ent->message ));
			}
			else
			{
				trap->SendServerCommand( activator-g_entities, va("cp \"%s\"", ent->message ));
			}
		}
		//NOTE: change in functionality - if there *is* no valid client ent, it won't send it to anyone at all
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				G_TeamCommand( TEAM_RED, va("cps \"%s\"", ent->message) );
			}
			else
			{
				G_TeamCommand( TEAM_RED, va("cp \"%s\"", ent->message) );
			}
		}
		if ( ent->spawnflags & 2 ) {
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				G_TeamCommand( TEAM_BLUE, va("cps \"%s\"", ent->message) );
			}
			else
			{
				G_TeamCommand( TEAM_BLUE, va("cp \"%s\"", ent->message) );
			}
		}
		return;
	}

	if (ent->message[0] == '@' && ent->message[1] != '@')
	{
		trap->SendServerCommand( -1, va("cps \"%s\"", ent->message ));
	}
	else
	{
		trap->SendServerCommand( -1, va("cp \"%s\"", ent->message ));
	}
}

void SP_target_print( gentity_t *ent ) {
	ent->use = Use_Target_Print;
}

void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	qboolean ranscript = qfalse;
	if ( ( self->spawnflags & 1 ) && activator->client
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	ranscript = G_ActivateBehavior( self, BSET_USE );
	if ( self->wait == -1 )
	{//never use again
		if ( ranscript )
		{//crap, can't remove!
			self->use = NULL;
		}
		else
		{//remove
			self->think = G_FreeEntity;
			self->nextthink = level.time + FRAMETIME;
		}
	}
	if ( self->spawnflags & 4 ) {
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			GlobalUse( ent, self, activator );
		}
		return;
	}
	G_UseTargets (self, activator);
}

void SP_target_relay (gentity_t *self) {
	self->use = target_relay_use;
	if ( self->spawnflags&128 )
	{
		self->flags |= FL_INACTIVE;
	}
}

void target_counter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count == 0 )
	{
		return;
	}

	//trap->Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
	self->count--;

	if ( activator )
	{
		G_DebugPrint( WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname, (self->genericValue1-self->count), self->genericValue1 );
	}

	if ( self->count )
	{
		if ( self->target2 )
		{
			//trap->Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
			G_UseTargets2( self, activator, self->target2 );
		}
		return;
	}

	G_ActivateBehavior( self,BSET_USE );

	if ( self->spawnflags & 128 )
	{
		self->flags |= FL_INACTIVE;
	}

	self->activator = activator;
	G_UseTargets( self, activator );

	if ( self->count == 0 )
	{
		if ( self->bounceCount == 0 )
		{
			return;
		}
		self->count = self->genericValue1;
		if ( self->bounceCount > 0 )
		{//-1 means bounce back forever
			self->bounceCount--;
		}
	}
}

void SP_target_counter (gentity_t *self)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}
	//if ( self->bounceCount > 0 )//let's always set this anyway
	{//we will reset when we use up our count, remember our initial count
		self->genericValue1 = self->count;
	}

	self->use = target_counter_use;
}
