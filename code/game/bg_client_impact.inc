// Native movement contact and impact callback dispatch. GPL-2.0-only.
#include "g_actor_lifetime.h"

qboolean PM_ClientImpact( trace_t *trace, qboolean damageSelf )
{
	gentity_t	*traceEnt;
	int			otherEntityNum = trace->entityNum;

	if ( !pm->gent || otherEntityNum<0 || otherEntityNum>ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	traceEnt = &g_entities[otherEntityNum];

	if ( otherEntityNum == ENTITYNUM_WORLD
		|| (traceEnt->bmodel && traceEnt->s.pos.trType == TR_STATIONARY ) )
	{//hit world or a non-moving brush
		if ( PM_CheckGrabWall( trace ) )
		{//stopped on the wall
			return qtrue;
		}
	}

	if( (VectorLength( pm->ps->velocity )*(pm->gent->mass/10)) >= 100 && ((pm->gent->client && pm->gent->client->NPC_class == CLASS_VEHICLE) || pm->ps->lastOnGround+100<level.time) )//was 300 ||(other->material>=MAT_GLASS&&pm->gent->lastImpact+100<=level.time))
	{
		const G_ActorLifetime mover(*pm->gent);
		const G_ActorLifetime obstacle(*traceEnt);
		DoImpact( pm->gent, traceEnt, damageSelf, trace );
		if (!mover.Current()) {
			pm->moverRemoved=qtrue;
			return qfalse;
		}
		if (otherEntityNum<ENTITYNUM_WORLD&&!obstacle.Current())
			return qtrue;
	}

	if ( otherEntityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	if ( !traceEnt || !(traceEnt->contents&pm->tracemask) )
	{//it's dead or not in my way anymore
		return qtrue;
	}

	return qfalse;
}
