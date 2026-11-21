// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
// the wait time has passed, so set back up for another activation
void multi_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger_run( gentity_t *ent )
{
 if(G_ContactKind(*ent)){ContactFire(*ent);return;}
	ent->e_ThinkFunc = thinkF_NULL;

	G_ActivateBehavior( ent, BSET_USE );

	if ( ent->soundSet && ent->soundSet[0] )
	{
		gi.SetConfigstring( CS_AMBIENT_SET, ent->soundSet );
	}

	G_UseTargets (ent, ent->activator);
	if ( ent->noise_index )
	{
		G_Sound( ent->activator, ent->noise_index );
	}

	if ( ent->target2 && ent->target2[0] && ent->wait >= 0 )
	{
		ent->e_ThinkFunc = thinkF_trigger_cleared_fire;
		ent->nextthink = level.time + ent->speed;
	}
	else if ( ent->wait > 0 )
	{
		if ( ent->painDebounceTime != level.time )
		{//first ent to touch it this frame
			//ent->e_ThinkFunc = thinkF_multi_wait;
			ent->nextthink = level.time + ( ent->wait + ent->random * Q_flrand(-1.0f, 1.0f) ) * 1000;
			ent->painDebounceTime = level.time;
		}
	}
	else if ( ent->wait < 0 )
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->contents &= ~CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		ent->e_TouchFunc = touchF_NULL;
		ent->e_UseFunc = useF_NULL;
		//Don't remove, Icarus may barf?
		//ent->nextthink = level.time + FRAMETIME;
		//ent->think = G_FreeEntity;
	}
	if( ent->activator && ent->activator->s.number == 0 )
	{	// mark the trigger as being touched by the player
		ent->aimDebounceTime = level.time;
	}
}


#include "g_siege_item.h"
void multi_trigger( gentity_t *ent, gentity_t *activator )
{
 if(G_ContactKind(*ent)){ContactUse(*ent,activator);return;}
	if ( ent->e_ThinkFunc == thinkF_multi_trigger_run )
	{//already triggered, just waiting to run
		return;
	}

	if ( ent->nextthink > level.time )
	{
		if( ent->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in a single frame
		{
			if ( ent->painDebounceTime && ent->painDebounceTime != level.time )
			{//this should still allow subsequent ents to fire this trigger in the current frame
				return;		// can't retrigger until the wait is over
			}
		}
		else
		{
			return;
		}

	}
	if ( ent->spawnflags & 32)
	{
		ent->nextthink = level.time + ent->delay;

	//	trace_t	viewTrace;
	//	gi.trace(&viewTrace, ent->currentOrigin, 0, 0, activator->currentOrigin, ent->s.number, MASK_SHOT);
	//	if ((viewTrace.allsolid) || (viewTrace.startsolid) || 	(viewTrace.entityNum!=activator->s.number))
	//	{
	//		return;
	//	}
	}


	// if the player has already activated this trigger this frame
	if( activator && !activator->s.number && ent->aimDebounceTime == level.time )
	{
		return;
	}

	if ( ent->svFlags & SVF_INACTIVE )
	{//Not active at this time
		return;
	}

	if(ent->spawnflags&jar::siege::requiresItem){
		const auto lifetime=G_EntityLifetime(ent);
		if(!G_SiegeTryDeliver(*ent,activator)||!lifetime||G_EntityLifetime(ent)!=lifetime)return;
	}
	ent->activator = activator;

	if(ent->delay && ent->painDebounceTime < (level.time + ent->delay) )
	{//delay before firing trigger
		ent->e_ThinkFunc = thinkF_multi_trigger_run;
		ent->nextthink = level.time + ent->delay;
		ent->painDebounceTime = level.time;

	}
	else
	{
		multi_trigger_run (ent);
	}
}

void Use_Multi( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	multi_trigger( ent, activator );
}

extern	int	Pilot_ActivePilotCount(void);

void Touch_Multi( gentity_t *self, gentity_t *other, trace_t *trace )
{
 if(G_ContactKind(*self)){if(other)G_ContactTouchAt(*self,other,other->currentOrigin);return;}
	if( !other->client )
	{
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if( self->noDamageTeam )
	{
		if ( other->client->playerTeam != self->noDamageTeam )
		{
			return;
		}
	}

// moved to just above multi_trigger because up here it just checks if the trigger is not being touched
// we want it to check any conditions set on the trigger, if one of those isn't met, the trigger is considered to be "cleared"
//	if ( self->e_ThinkFunc == thinkF_trigger_cleared_fire )
//	{//We're waiting to fire our target2 first
//		self->nextthink = level.time + self->speed;
//		return;
//	}

	if ( self->spawnflags & 1 )
	{
		if ( other->s.number != 0 )
		{
			return;
		}
	}
	else
	{
		if ( self->spawnflags & 16 )
		{//NPCONLY
			if ( other->NPC == NULL )
			{
				return;
			}
		}

		if ( self->NPC_targetname && self->NPC_targetname[0] )
		{
			if ( other->script_targetname && other->script_targetname[0] )
			{
				if ( Q_stricmp( self->NPC_targetname, other->script_targetname ) != 0 )
				{//not the right guy to fire me off
					return;
				}
			}
			else
			{
				return;
			}
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if ( !other->client )
		{
			return;
		}

		if( !( other->client->usercmd.buttons & BUTTON_USE ) )
		{//not pressing use button
			return;
		}
	}

	if ( self->spawnflags & 2 )
	{//FACING
		vec3_t	forward;

		if ( other->client )
		{
			AngleVectors( other->client->ps.viewangles, forward, NULL, NULL );
		}
		else
		{
			AngleVectors( other->currentAngles, forward, NULL, NULL );
		}

		if ( DotProduct( self->movedir, forward ) < 0.5 )
		{//Not Within 45 degrees
			return;
		}
	}

	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		if ( !other->client )
		{
			return;
		}

		if( !( other->client->ps.eFlags & EF_FIRING /*usercmd.buttons & BUTTON_ATTACK*/ ) &&
			!( other->client->ps.eFlags & EF_ALT_FIRING/*usercmd.buttons & BUTTON_ALT_ATTACK*/ ) )
		{//not pressing fire button or altfire button
			return;
		}

		//FIXME: do we care about the sniper rifle or not?

		if( other->s.number == 0 && ( other->client->ps.weapon > MAX_PLAYER_WEAPONS || other->client->ps.weapon <= WP_NONE ) )
		{//don't care about non-player weapons if this is the player
			return;
		}
	}

	if ( other->client && self->radius )
	{
		vec3_t	eyeSpot;

		//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		VectorCopy(other->currentOrigin, eyeSpot);
		eyeSpot[2] += other->client->ps.viewheight;

		if ( G_PointInBounds( eyeSpot, self->absmin, self->absmax ) )
		{
			if( !( other->client->ps.eFlags & EF_FIRING ) &&
				!( other->client->ps.eFlags & EF_ALT_FIRING ) )
			{//not attacking, so hiding bonus
				//FIXME:  should really have sound events clear the hiddenDist
				other->client->hiddenDist = self->radius;
				//NOTE: movedir HAS to be normalized!
				if ( VectorLength( self->movedir ) )
				{//They can only be hidden from enemies looking in this direction
					VectorCopy( self->movedir, other->client->hiddenDir );
				}
				else
				{
					VectorClear( other->client->hiddenDir );
				}
			}
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		NPC_SetAnim( other, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		/*
		if ( !VectorLengthSquared( other->client->ps.velocity ) && !PM_CrouchAnim( other->client->ps.legsAnim ) )
		{
			NPC_SetAnim( other, SETANIM_LEGS, BOTH_BUTTON_HOLD, SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
		}
		*/
		//other->client->ps.weaponTime = other->client->ps.torsoAnimTimer;
	}

	if ( self->e_ThinkFunc == thinkF_trigger_cleared_fire )
	{//We're waiting to fire our target2 first
		self->nextthink = level.time + self->speed;
		return;
	}

	if ( self->spawnflags & 32)
	{
		if (Pilot_ActivePilotCount()>=self->lastInAirTime)
		{
			return;
		}
	}

	multi_trigger( self, other );
}

void trigger_cleared_fire (gentity_t *self)
{
	G_UseTargets2( self, self->activator, self->target2 );
	self->e_ThinkFunc = thinkF_NULL;
	// should start the wait timer now, because the trigger's just been cleared, so we must "wait" from this point
	if ( self->wait > 0 )
	{
		self->nextthink = level.time + ( self->wait + self->random * Q_flrand(-1.0f, 1.0f) ) * 1000;
	}
}
