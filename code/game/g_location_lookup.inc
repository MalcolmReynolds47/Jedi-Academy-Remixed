#include "g_target_markers.h"
#include "g_volume_geometry.h"
char *G_GetLocationForEnt( gentity_t *ent )
{
	vec3_t		mins, maxs;
	gentity_t	*found = NULL;

	VectorAdd( ent->currentOrigin, ent->mins, mins );
	VectorAdd( ent->currentOrigin, ent->maxs, maxs );

	while( (found = G_Find(found, FOFS(classname), "trigger_location")) != NULL )
	{
		const bool contact = jar::volumeEntity::Portable(*found)
			? !(found->svFlags & SVF_INACTIVE) && jar::volume::Contact(found->pos1, found->pos2,
				found->currentOrigin, found->currentAngles, ent->currentOrigin, ent->mins, ent->maxs)
			: gi.EntityContact(mins, maxs, found) == qtrue;
		if (contact)
		{
			return found->message;
		}
	}

	return G_NearestLocation(ent);
}

