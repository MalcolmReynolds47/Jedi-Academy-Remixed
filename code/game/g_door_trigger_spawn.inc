// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_subbsp_movers.h"

void Think_SpawnNewDoorTrigger( gentity_t *ent )
{
	gentity_t		*other;
	vec3_t		mins, maxs;
	int			i, best;

	// set all of the slaves as shootable
	if ( ent->takedamage )
	{
		for ( other = ent ; other ; other = other->teamchain )
		{
			other->takedamage = qtrue;
		}
	}

	// find the bounds of everything on the team
	const bool section = jar::subBSPMover::DoorBounds(*ent, mins, maxs);
	if (!section)
	{
		VectorCopy (ent->absmin, mins);
		VectorCopy (ent->absmax, maxs);
	
		for (other = ent->teamchain ; other ; other=other->teamchain) {
			AddPointToBounds (other->absmin, mins, maxs);
			AddPointToBounds (other->absmax, mins, maxs);
		}
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn ();
	if (section) jar::subBSPMover::Trigger(*other, *ent, mins, maxs);
	else
	{
		VectorCopy(mins, other->mins);
		VectorCopy(maxs, other->maxs);
	}
	other->owner = ent;
	other->contents = CONTENTS_TRIGGER;
	other->e_TouchFunc = touchF_Touch_DoorTrigger;
	gi.linkentity (other);
	other->classname = "trigger_door";

	MatchTeam( ent, ent->moverState, level.time );
}

