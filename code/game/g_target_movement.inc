// Native movement targets and the scoped Sandbox activation path.
#include "g_movement_settings.h"

static bool SandboxMovementUse(gentity_t* source,gentity_t* activator){
 if(!G_MovementMarkerPortable(*source))return false;
 if(!(source->svFlags&SVF_INACTIVE)){
  G_MovementMarkerActivate(*source,activator);
  G_ActivateBehavior(source,BSET_USE);
 }
 return true;
}

void target_gravity_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( SandboxMovementUse(self,activator) ) return;
	G_ActivateBehavior(self,BSET_USE);

	if ( self->spawnflags & 1 )
	{
		gi.cvar_set("g_gravity", va("%f", self->speed));
	}
	else if ( activator && activator->client )
	{
		int	grav = floor(self->speed);
		/*
		if ( activator->client->ps.gravity != grav )
		{
			gi.Printf("%s gravity changed to %d\n", activator->targetname, grav );
		}
		*/
		activator->client->ps.gravity = grav;
		activator->svFlags |= SVF_CUSTOM_GRAVITY;
		//FIXME: need a way to set this back to normal?
	}
}

/*QUAKED target_gravity_change (1 0 0) (-4 -4 -4) (4 4 4) GLOBAL

"gravity" - Normal = 800, Valid range: any

GLOBAL - Apply to entire world, not just the activator
*/
void SP_target_gravity_change( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	G_SpawnFloat( "gravity", "0", &self->speed );
	self->e_UseFunc = useF_target_gravity_change_use;
}

void target_friction_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( SandboxMovementUse(self,activator) ) return;
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{//FIXME - make a global?
		//gi.Cvar_Set("g_friction", va("%d", self->health));
	}
	else if(activator&&activator->client)
	{
		activator->client->ps.friction = self->health;
	}
}

/*QUAKED target_friction_change (1 0 0) (-4 -4 -4) (4 4 4)

"friction" Normal = 6, Valid range 0 - 10

*/
void SP_target_friction_change( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_friction_change_use;
}

