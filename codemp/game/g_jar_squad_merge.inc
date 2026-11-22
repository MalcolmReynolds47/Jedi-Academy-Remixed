// Multiplayer squad maintenance. AI_UpdateGroups prunes all rosters before merging.
static void AI_PruneSquadRoster(AIGroupInfo_t *group)
{
	for (int j = group->numGroup - 1; j >= 0; --j) {
		gentity_t *member = AI_LivingGroupMember(group->member[j].number);
		if (!member || member->client->playerTeam != (npcteam_t)group->team)
			AI_DeleteGroupMember(group, j);
	}
}

static qboolean AI_MergeIntoEarlierGroup(AIGroupInfo_t *group)
{
	for (int i = 0; i < MAX_FRAME_GROUPS; ++i) {
		AIGroupInfo_t *dest = &level.groups[i];
		if (dest == group)
			break;
		if (!dest->numGroup || dest->team != group->team || dest->enemy != group->enemy)
			continue;
		if (dest->numGroup + group->numGroup > MAX_GROUP_MEMBERS - 1)
			continue;

		for (int j = 0; j < group->numGroup; ) {
			gentity_t *member = &g_entities[group->member[j].number];
			if (!dest->enemy && !AI_ValidateNoEnemyGroupMember(dest, member)) {
				++j;
				continue;
			}
			AI_DeleteGroupMember(group, j); // Next member shifts into j.
			AI_InsertGroupMember(dest, member);
		}
		if (!group->numGroup)
			return qtrue;
	}
	return qfalse;
}
