// Native Raven/OpenJK combat-point selection and reservation. GPL-2.0-only.
// Extracted unchanged except availability and index guards for removable points.
/*QUAKED point_combat (0.7 0 0.7) (-20 -20 -24) (20 20 45) DUCK FLEE INVESTIGATE SQUAD LEAN SNIPE
NPCs in bState BS_COMBAT_POINT will find their closest empty combat_point

DUCK - NPC will duck and fire from this point, NOT IMPLEMENTED?
FLEE - Will choose this point when running
INVESTIGATE - Will look here if a sound is heard near it
SQUAD - NOT IMPLEMENTED
LEAN - Lean-type cover, NOT IMPLEMENTED
SNIPE - Snipers look for these first, NOT IMPLEMENTED
*/

void SP_point_combat( gentity_t *self )
{
	if(level.numCombatPoints >= MAX_COMBAT_POINTS)
	{
#ifndef FINAL_BUILD
		gi.Printf(S_COLOR_RED"ERROR:  Too many combat points, limit is %d\n", MAX_COMBAT_POINTS);
#endif
		G_FreeEntity(self);
		return;
	}

	self->s.origin[2] += 0.125;
	G_SetOrigin(self, self->s.origin);
	gi.linkentity(self);

	if ( G_CheckInSolid( self, qtrue ) )
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"ERROR: combat point at %s in solid!\n", vtos(self->currentOrigin) );
#endif
	}

	VectorCopy( self->currentOrigin, level.combatPoints[level.numCombatPoints].origin );

	level.combatPoints[level.numCombatPoints].flags = self->spawnflags;
	level.combatPoints[level.numCombatPoints].occupied = qfalse;

	level.numCombatPoints++;

	NAV::SpawnedPoint(self, NAV::PT_COMBATNODE);

	G_FreeEntity(self);
};

void CP_FindCombatPointWaypoints( void )
{
	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		if (!G_CombatPointAvailable(i)) continue;
		level.combatPoints[i].waypoint = NAV::GetNearestNode(level.combatPoints[i].origin);
		if ( level.combatPoints[i].waypoint == WAYPOINT_NONE )
		{
			assert(0);
			level.combatPoints[i].waypoint = NAV::GetNearestNode(level.combatPoints[i].origin);
			gi.Printf( S_COLOR_RED"ERROR: Combat Point at %s has no waypoint!\n", vtos(level.combatPoints[i].origin) );
			delayedShutDown = level.time + 100;
		}
	}
}


/*
-------------------------
NPC_CollectCombatPoints
-------------------------
*/

typedef	std::map< float, int >	combatPoint_m;

static int NPC_CollectCombatPoints( const vec3_t origin, const float radius, combatPoint_m &points, const int flags )
{
	float	radiusSqr = (radius*radius);
	float	distance;

	//Collect all nearest
	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		if (!G_CombatPointAvailable(i)) continue;
		//Must be vacant
		if ( level.combatPoints[i].occupied == (int) qtrue )
			continue;

		//If we want a duck space, make sure this is one
		if ( ( flags & CP_DUCK ) && !( level.combatPoints[i].flags & CPF_DUCK ) )
			continue;

		//If we want a flee point, make sure this is one
		if ( ( flags & CP_FLEE ) && !( level.combatPoints[i].flags & CPF_FLEE ) )
			continue;

		//If we want a snipe point, make sure this is one
		if ( ( flags & CP_SNIPE ) && !( level.combatPoints[i].flags & CPF_SNIPE ) )
			continue;

		///Make sure this is an investigate combat point
		if ( ( flags & CP_INVESTIGATE ) && ( level.combatPoints[i].flags & CPF_INVESTIGATE ) )
			continue;

		//Squad points are only valid if we're looking for them
		if ( ( level.combatPoints[i].flags & CPF_SQUAD ) && ( ( flags & CP_SQUAD ) == qfalse ) )
			continue;

		if ( flags&CP_NO_PVS )
		{//must not be within PVS of mu current origin
			if ( gi.inPVS( origin, level.combatPoints[i].origin ) )
			{
				continue;
			}
		}

		if ( flags&CP_HORZ_DIST_COLL )
		{
			distance = 	DistanceHorizontalSquared( origin, level.combatPoints[i].origin );
		}
		else
		{
			distance = 	DistanceSquared( origin, level.combatPoints[i].origin );
		}

		if ( distance < radiusSqr )
		{
			//Using a map will sort nearest automatically
			points[ distance ] = i;
		}
	}

	return points.size();
}

/*
-------------------------
NPC_FindCombatPoint
-------------------------
*/

#define	MIN_AVOID_DOT				0.7f
#define MIN_AVOID_DISTANCE			128
#define MIN_AVOID_DISTANCE_SQUARED	( MIN_AVOID_DISTANCE * MIN_AVOID_DISTANCE )
#define	CP_COLLECT_RADIUS			512.0f

int NPC_FindCombatPoint( const vec3_t position, const vec3_t avoidPosition, vec3_t destPosition, const int flags, float avoidDist, const int ignorePoint )
{
	combatPoint_m			points;
	combatPoint_m::iterator	cpi;

	int						best = -1;//, cost, bestCost = Q3_INFINITE, waypoint = WAYPOINT_NONE, destWaypoint = WAYPOINT_NONE;
	trace_t					tr;
	float					collRad = CP_COLLECT_RADIUS;
	vec3_t					eDir2Me, eDir2CP, weaponOffset;
	vec3_t					enemyPosition;
	float					dotToCp;
	//float					distSqPointToNPC;
	float					distSqPointToEnemy;
	float					distSqPointToEnemyHoriz;
	float					distSqPointToEnemyCheck;
	float					distSqNPCToEnemy;
	float					distSqNPCToEnemyHoriz;
	float					distSqNPCToEnemyCheck;
	float					visRangeSq = (NPCInfo->stats.visrange*NPCInfo->stats.visrange);
	bool					useHorizDist = (NPC->s.weapon==WP_THERMAL) || (flags & CP_HORZ_DIST_COLL);

	if (NPC->enemy)
	{
		VectorCopy(NPC->enemy->currentOrigin, enemyPosition);
	}
	else if (avoidPosition)
	{
		VectorCopy(avoidPosition, enemyPosition);
	}
	else if (destPosition)
	{
		VectorCopy(destPosition, enemyPosition);
	}
	else
	{
		VectorCopy(NPC->currentOrigin, enemyPosition);
	}

	if ( avoidDist <= 0 )
	{
		avoidDist = MIN_AVOID_DISTANCE_SQUARED;
	}
	else
	{
		avoidDist *= avoidDist;
	}


	//Collect our nearest points
	if ( (flags & CP_NO_PVS) || (flags & CP_TRYFAR))
	{//much larger radius since most will be dropped?
		collRad = CP_COLLECT_RADIUS*4;
	}
	NPC_CollectCombatPoints( destPosition, collRad, points, flags );//position

	for ( cpi = points.begin(); cpi != points.end(); ++cpi )
	{
		const int i = (*cpi).second;

		//Must not be one we want to ignore
		if ( i == ignorePoint )
		{
			continue;
		}

		//Get some distances for reasoning
		//distSqPointToNPC		= (*cpi).first;

		distSqPointToEnemy		= DistanceSquared          (level.combatPoints[i].origin, enemyPosition);
		distSqPointToEnemyHoriz	= DistanceHorizontalSquared(level.combatPoints[i].origin, enemyPosition);
		distSqPointToEnemyCheck = (useHorizDist)?(distSqPointToEnemyHoriz):(distSqPointToEnemy);

		distSqNPCToEnemy		= DistanceSquared          (NPC->currentOrigin, enemyPosition);
		distSqNPCToEnemyHoriz	= DistanceHorizontalSquared(NPC->currentOrigin, enemyPosition);
		distSqNPCToEnemyCheck	= (useHorizDist)?(distSqNPCToEnemyHoriz  ):(distSqNPCToEnemy);



		//Ignore points that are farther than currently located
		if ( (flags & CP_APPROACH_ENEMY) && (distSqPointToEnemyCheck > distSqNPCToEnemyCheck))
		{
			continue;
		}

		//Ignore points that are closer than currently located
		if ( (flags & CP_RETREAT) && (distSqPointToEnemyCheck < distSqNPCToEnemyCheck))
		{
			continue;
		}

		//Ignore points that are out of vis range
		if ( (flags & CP_CLEAR) && (distSqPointToEnemyCheck > visRangeSq))
		{
			continue;
		}

		//Avoid this position?
		if ( avoidPosition && !(flags & CP_AVOID_ENEMY) && (flags & CP_AVOID) && (DistanceSquared(level.combatPoints[i].origin, avoidPosition)<avoidDist) )
		{
			continue;
		}

		//We want a point on other side of the enemy from current pos
		if (flags & CP_FLANK )
		{
			VectorSubtract( position, enemyPosition, eDir2Me );
			VectorNormalize( eDir2Me );

			VectorSubtract( level.combatPoints[i].origin, enemyPosition, eDir2CP );
			VectorNormalize( eDir2CP );

			dotToCp = DotProduct( eDir2Me, eDir2CP );

			//Not far enough behind enemy from current pos
			if ( dotToCp >= 0.4 )
			{
				continue;
			}
		}

		//we must have a route to the combat point
		if ( (flags & CP_HAS_ROUTE) && !NAV::InSameRegion(NPC, level.combatPoints[i].origin))
		{
			continue;
		}


		//See if we're trying to avoid our enemy
		if (flags & CP_AVOID_ENEMY)
		{
			//Can't be too close to the enemy
			if (distSqPointToEnemy<avoidDist)
			{
				continue;
			}

			// otherwise, if currently safe and the path is not safe, ignore this point
	  		if (distSqNPCToEnemy>(avoidDist) &&
 				!NAV::SafePathExists(position, level.combatPoints[i].origin, enemyPosition, avoidDist))
			{
				continue;
			}
		}

		//Okay, now make sure it's not blocked
		gi.trace( &tr, level.combatPoints[i].origin, NPC->mins, NPC->maxs, level.combatPoints[i].origin, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
		if ( tr.allsolid || tr.startsolid )
		{
			continue;
		}

		if (NPC->enemy)
		{
			// Ignore Points That Do Not Have A Clear LOS To The Player
			if ( (flags & CP_CLEAR) )
			{
				CalcEntitySpot(NPC, SPOT_WEAPON, weaponOffset);
				VectorSubtract(weaponOffset, NPC->currentOrigin, weaponOffset);
				VectorAdd(weaponOffset, level.combatPoints[i].origin, weaponOffset);

				if (NPC_ClearLOS(weaponOffset, NPC->enemy)==qfalse)
				{
					continue;
				}
			}

			// Ignore points that are not behind cover
			if ( (flags & CP_COVER) && NPC_ClearLOS(level.combatPoints[i].origin, NPC->enemy)==qtrue)
			{
				continue;
			}
		}

		//they are sorted by this distance, so the first one to get this far is the closest
		return i;
	}

	return best;
}

int NPC_FindCombatPointRetry( const vec3_t position,
							 const vec3_t avoidPosition,
							 vec3_t destPosition,
							 int *cpFlags,
							 float avoidDist,
							 const int ignorePoint )
{
	int cp = -1;
	cp = NPC_FindCombatPoint( position,
								avoidPosition,
								destPosition,
								*cpFlags,
								avoidDist,
								ignorePoint );
	while ( cp == -1 && (*cpFlags&~CP_HAS_ROUTE) != CP_ANY )
	{//start "OR"ing out certain flags to see if we can find *any* point
		if ( *cpFlags & CP_INVESTIGATE )
		{//don't need to investigate
			*cpFlags &= ~CP_INVESTIGATE;
		}
		else if ( *cpFlags & CP_SQUAD )
		{//don't need to stick to squads
			*cpFlags &= ~CP_SQUAD;
		}
		else if ( *cpFlags & CP_DUCK )
		{//don't need to duck
			*cpFlags &= ~CP_DUCK;
		}
		else if ( *cpFlags & CP_NEAREST )
		{//don't need closest one to me
			*cpFlags &= ~CP_NEAREST;
		}
		else if ( *cpFlags & CP_FLANK )
		{//don't need to flank enemy
			*cpFlags &= ~CP_FLANK;
		}
		else if ( *cpFlags & CP_SAFE )
		{//don't need one that hasn't been shot at recently
			*cpFlags &= ~CP_SAFE;
		}
		else if ( *cpFlags & CP_CLOSEST )
		{//don't need to get closest to enemy
			*cpFlags &= ~CP_CLOSEST;
			//but let's try to approach at least
			*cpFlags |= CP_APPROACH_ENEMY;
		}
		else if ( *cpFlags & CP_APPROACH_ENEMY )
		{//don't need to approach enemy
			*cpFlags &= ~CP_APPROACH_ENEMY;
		}
		else if ( *cpFlags & CP_COVER )
		{//don't need cover
			*cpFlags &= ~CP_COVER;
			//but let's pick one that makes us duck
			//*cpFlags |= CP_DUCK;
		}
	//	else if ( *cpFlags & CP_CLEAR )
	//	{//don't need a clear shot to enemy
	//		*cpFlags &= ~CP_CLEAR;
	//	}
	//	Never Give Up On Avoiding The Enemy
	//	else if ( *cpFlags & CP_AVOID_ENEMY )
	//	{//don't need to avoid enemy
	//		*cpFlags &= ~CP_AVOID_ENEMY;
	//	}
		else if ( *cpFlags & CP_RETREAT )
		{//don't need to retreat
			*cpFlags &= ~CP_RETREAT;
		}
		else if ( *cpFlags &CP_FLEE )
		{//don't need to flee
			*cpFlags &= ~CP_FLEE;
			//but at least avoid enemy and pick one that gives cover
			*cpFlags |= (CP_COVER|CP_AVOID_ENEMY);
		}
		else if ( *cpFlags & CP_AVOID )
		{//okay, even pick one right by me
			*cpFlags &= ~CP_AVOID;
		}
		else if ( *cpFlags & CP_SHORTEST_PATH )
		{//okay, don't need the one with the shortest path
			*cpFlags &= ~CP_SHORTEST_PATH;
		}
		else
		{//screw it, we give up!
			return -1;
			/*
			if ( *cpFlags & CP_HAS_ROUTE )
			{//NOTE: this is really an absolute worst case scenario - will go to the first cp on the map!
				*cpFlags &= ~CP_HAS_ROUTE;
			}
			else
			{//NOTE: this is really an absolute worst case scenario - will go to the first cp on the map!
				*cpFlags = CP_ANY;
			}
			*/
		}
		//now try again
		cp = NPC_FindCombatPoint( position,
									avoidPosition,
									destPosition,
									*cpFlags,
									avoidDist,
									ignorePoint );
	}
	return cp;
}
/*
-------------------------
NPC_FindSquadPoint
-------------------------
*/

int NPC_FindSquadPoint( vec3_t position )
{
	float	dist, nearestDist = (float)WORLD_SIZE*(float)WORLD_SIZE;
	int		nearestPoint = -1;

	//float			playerDist = DistanceSquared( g_entities[0].currentOrigin, NPC->currentOrigin );

	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		if (!G_CombatPointAvailable(i)) continue;
		//Squad points are only valid if we're looking for them
		if ( ( level.combatPoints[i].flags & CPF_SQUAD ) == qfalse )
			continue;

		//Must be vacant
		if ( level.combatPoints[i].occupied == qtrue )
			continue;

		dist = DistanceSquared( position, level.combatPoints[i].origin );

		//The point cannot take us past the player
		//if ( dist > ( playerDist * DotProduct( dirToPlayer, playerDir ) ) )	//FIXME: Retain this
		//	continue;

		//See if this is closer than the others
		if ( dist < nearestDist )
		{
			nearestPoint = i;
			nearestDist = dist;
		}
	}

	return nearestPoint;
}

/*
-------------------------
NPC_ReserveCombatPoint
-------------------------
*/

qboolean NPC_ReserveCombatPoint( int combatPointID )
{
	//Make sure it's valid
	if ( !G_CombatPointAvailable(combatPointID) )
		return qfalse;

	//Make sure it's not already occupied
	if ( level.combatPoints[combatPointID].occupied )
		return qfalse;

	//Reserve it
	level.combatPoints[combatPointID].occupied = qtrue;

	return qtrue;
}

/*
-------------------------
NPC_FreeCombatPoint
-------------------------
*/

qboolean NPC_FreeCombatPoint( int combatPointID, qboolean failed )
{
	if ( failed )
	{//remember that this one failed for us
		NPCInfo->lastFailedCombatPoint = combatPointID;
	}
	//Make sure it's valid
	if ( combatPointID < 0 || combatPointID >= level.numCombatPoints || combatPointID >= MAX_COMBAT_POINTS )
		return qfalse;

	//Make sure it's currently occupied
	if ( level.combatPoints[combatPointID].occupied == qfalse )
		return qfalse;

	//Free it
	level.combatPoints[combatPointID].occupied = qfalse;

	return qtrue;
}

/*
-------------------------
NPC_SetCombatPoint
-------------------------
*/

qboolean NPC_SetCombatPoint( int combatPointID )
{
	if (!G_CombatPointAvailable(combatPointID)) return qfalse;
	if (combatPointID==NPCInfo->combatPoint)
	{
		return qtrue;
	}

	//Free a combat point if we already have one
	if ( NPCInfo->combatPoint != -1 )
	{
		NPC_FreeCombatPoint( NPCInfo->combatPoint );
	}

	if ( NPC_ReserveCombatPoint( combatPointID ) == qfalse )
		return qfalse;

	NPCInfo->combatPoint = combatPointID;

	return qtrue;
}

