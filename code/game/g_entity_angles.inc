#include "qcommon/jar_angular_trajectory.h"
void G_SetAngles( gentity_t *ent, const vec3_t angles )
{
	vec3_t target;VectorCopy(angles,target);
	const bool angular = jar::angular::Rebase(ent->s.apos, ent->currentAngles, target);
	VectorCopy( target, ent->currentAngles );
	VectorCopy( target, ent->s.angles );
	if (!angular) VectorCopy( target, ent->s.apos.trBase );
}
