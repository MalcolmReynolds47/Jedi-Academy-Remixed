// Squad navigation shared with the native regression test. GPL-2.0-or-later.
void AI_SetClosestBuddy( AIGroupInfo_t *group )
{
	int	i, j;
	int	dist, bestDist;

	for ( i = 0; i < group->numGroup; i++ )
	{
		group->member[i].closestBuddy = ENTITYNUM_NONE;

		bestDist = Q3_INFINITE;
		for ( j = 0; j < group->numGroup; j++ )
		{
			if (i == j) continue;
			dist = DistanceSquared( g_entities[group->member[i].number].r.currentOrigin, g_entities[group->member[j].number].r.currentOrigin );
			if ( dist < bestDist )
			{
				bestDist = dist;
				group->member[i].closestBuddy = group->member[j].number;
			}
		}
	}
}

void AI_SortGroupByPathCostToEnemy( AIGroupInfo_t *group )
{
	int i, j;
	group->enemyWP = group->enemy ? NAV_FindClosestWaypointForEnt(group->enemy, WAYPOINT_NONE) : WAYPOINT_NONE;
	for (i = 0; i < group->numGroup; ++i) {
		AIGroupMember_t member = group->member[i];
		member.waypoint = group->enemyWP == WAYPOINT_NONE ? WAYPOINT_NONE :
			NAV_FindClosestWaypointForEnt(&g_entities[member.number], WAYPOINT_NONE);
		member.pathCostToEnemy = member.waypoint == WAYPOINT_NONE ? Q3_INFINITE :
			trap->Nav_GetPathCost(member.waypoint, group->enemyWP);
		// Insert the complete record. Equal costs retain their original order.
		for (j = i; j > 0 && member.pathCostToEnemy < group->member[j-1].pathCostToEnemy; --j)
			group->member[j] = group->member[j-1];
		group->member[j] = member;
	}
}
