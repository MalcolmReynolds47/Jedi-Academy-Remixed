// Native Raven/OpenJK callbacks shared with the headless regression fixture.
// GPL-2.0-only; copyright and license retained in the including source file.
#include "g_distance_trigger.h"
#include "g_workshop.h"

static void G_PortableDistanceUse(gentity_t* trigger, gentity_t* activator)
{
    if (!JAR_DistanceSettingsValid(trigger->count, trigger->spawnflags)) return;
    const gentity_t* reference = trigger;
    if (trigger->ownername && *trigger->ownername) {
        if (!strcmp(trigger->ownername,JAR_DISTANCE_MISSING_REFERENCE_NAME)) return;
        reference = G_Find(nullptr, FOFS(targetname), trigger->ownername);
        if (!reference || reference == trigger || !G_JAR_WorkshopOwns(reference)) return;
    }
    const gentity_t* npc = nullptr;
    if (trigger->spawnflags & JAR_DISTANCE_NPC) {
        if (!trigger->NPC_target || !*trigger->NPC_target) return;
        npc = G_Find(nullptr, FOFS(targetname), trigger->NPC_target);
        if (!npc || !G_JAR_WorkshopOwns(npc) || !npc->client || !npc->NPC) return;
    }

    const float rangeSquared = static_cast<float>(trigger->count * trigger->count);
    const auto& player = g_entities[0];
    const bool playerInside = (trigger->spawnflags & JAR_DISTANCE_PLAYER) &&
        player.inuse && player.client && player.health > 0 &&
        DistanceSquared(reference->currentOrigin, player.currentOrigin) < rangeSquared;
    const bool npcInside = npc && npc->health > 0 &&
        DistanceSquared(reference->currentOrigin, npc->currentOrigin) < rangeSquared;

    trigger->svFlags |= SVF_INACTIVE;
    const char* output = playerInside || npcInside ? trigger->target : trigger->target2;
    if (output && G_JAR_WorkshopOwns(G_Find(nullptr, FOFS(targetname), output)))
        G_UseTargets2(trigger, activator ? activator : &g_entities[0], output);
}

void trigger_entdist_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	vec3_t		diff;
	gentity_t	*found = NULL;
	gentity_t	*owner = NULL;
	qboolean	useflag;
	const char	*token, *holdString;

	if ( self->svFlags & SVF_INACTIVE )	// Don't use INACTIVE
		return;

	if (G_DistancePortable(*self))
	{
		G_PortableDistanceUse(self, activator);
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	if(self->ownername && self->ownername[0])
	{
		owner = G_Find(NULL, FOFS(targetname), self->ownername);
	}

	if(owner == NULL)
	{
		owner = self;
	}

	self->activator = activator;

	useflag = qfalse;

	self->svFlags |= SVF_INACTIVE;	// Make it inactive after one use

	if (self->spawnflags & ENTDIST_PLAYER)	// Look for player???
	{
		found = &g_entities[0];

		if (found)
		{
			VectorSubtract(owner->currentOrigin, found->currentOrigin, diff);
			if(VectorLength(diff) < self->count)
			{
				useflag = qtrue;
			}
		}
	}

	if ((self->spawnflags & ENTDIST_NPC) && (!useflag))
	{
		holdString = self->NPC_target;

		while (holdString)
		{
			token = COM_Parse( &holdString);
			if ( !token ) // Nothing left to look at
			{
				break;
			}

			found = G_Find(found, FOFS(targetname), token);	// Look for the specified NPC
			if (found)	//Found???
			{
				VectorSubtract(owner->currentOrigin, found->currentOrigin, diff);
				if(VectorLength(diff) < self->count)	// Within distance
				{
					useflag = qtrue;
					break;
				}
			}
		}
	}

	if (useflag)
	{
		G_UseTargets2 (self, self->activator, self->target);
	}
	else if (self->target2)
	{
		// This is the negative target
		G_UseTargets2 (self, self->activator, self->target2);
	}


}

void SP_trigger_entdist( gentity_t *self )
{
	if (G_DistancePortable(*self))
	{
		self->count = JAR_DISTANCE_DEFAULT;
		self->spawnflags = JAR_DISTANCE_PLAYER;
	}
	else
	{
		G_SpawnInt( "distance", "0", &self->count);
	}

	self->e_UseFunc = useF_trigger_entdist_use;

}
