#include "g_target_markers.h"
#include "g_functions.h"
#define MAX_INTEREST_DIST	( 256 * 256 )
/*
-------------------------
NPC_FindLocalInterestPoint
-------------------------
*/

int G_FindLocalInterestPoint( gentity_t *self, lookMode_t& mode )
{
	mode=LM_INTEREST;
	int		i, bestPoint = ENTITYNUM_NONE;
	float	dist, bestDist = Q3_INFINITE;
	vec3_t	diffVec, eyes;

	CalcEntitySpot( self, SPOT_HEAD_LEAN, eyes );
	for ( i = 0; i < level.numInterestPoints; i++ )
	{
		//Don't ignore portals?  If through a portal, need to look at portal!
		if ( gi.inPVS( level.interestPoints[i].origin, eyes ) )
		{
			VectorSubtract( level.interestPoints[i].origin, eyes, diffVec );
			if ( (fabs(diffVec[0]) + fabs(diffVec[1])) / 2 < 48 &&
				fabs(diffVec[2]) > (fabs(diffVec[0]) + fabs(diffVec[1])) / 2 )
			{//Too close to look so far up or down
				continue;
			}
			dist = VectorLengthSquared( diffVec );
			//Some priority to more interesting points
			//dist -= ((int)level.interestPoints[i].lookMode * 5) * ((int)level.interestPoints[i].lookMode * 5);
			if ( dist < MAX_INTEREST_DIST && dist < bestDist )
			{
				if ( G_ClearLineOfSight( eyes, level.interestPoints[i].origin, self->s.number, MASK_OPAQUE ) )
				{
					bestDist = dist;
					bestPoint = i;
				}
			}
		}
	}
	const int portable=G_ChoosePortableInterest(self,eyes,bestDist);
	if(portable!=ENTITYNUM_NONE){if(!G_FirePortableInterest(self,portable))return ENTITYNUM_NONE;mode=LM_ENT;return portable;}
	if ( bestPoint != ENTITYNUM_NONE && level.interestPoints[bestPoint].target )
	{
		G_UseTargets2( self, self, level.interestPoints[bestPoint].target );
	}
	return bestPoint;
}

/*QUAKED target_interest (1 0.8 0.5) (-4 -4 -4) (4 4 4)
A point that a squadmate will look at if standing still

target - thing to fire when someone looks at this thing
*/

void SP_target_interest( gentity_t *self )
{//FIXME: rename point_interest
	if(G_PortableInterest(*self)){self->e_UseFunc=useF_marker_use;G_RegisterInterest(self);return;}
	if(level.numInterestPoints >= MAX_INTEREST_POINTS)
	{
		gi.Printf("ERROR:  Too many interest points, limit is %d\n", MAX_INTEREST_POINTS);
		G_FreeEntity(self);
		return;
	}

	VectorCopy(self->currentOrigin, level.interestPoints[level.numInterestPoints].origin);

	if(self->target && self->target[0])
	{
		level.interestPoints[level.numInterestPoints].target = G_NewString( self->target );
	}

	level.numInterestPoints++;

	G_FreeEntity(self);
}
