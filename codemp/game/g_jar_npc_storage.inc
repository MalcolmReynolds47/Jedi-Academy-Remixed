// Stable per-entity NPC state, separate from BG animation/string allocation.
static gNPC_t jarNPCStorage[MAX_GENTITIES];
gNPC_t *gNPCPtrs[MAX_GENTITIES];

gNPC_t *New_NPC_t(int entNum)
{
	gNPC_t *ptr;
	if (entNum < 0 || entNum >= ENTITYNUM_WORLD) return NULL;

	if (!gNPCPtrs[entNum])
	{
		gNPCPtrs[entNum] = &jarNPCStorage[entNum];
	}

	ptr = gNPCPtrs[entNum];

	if (ptr)
	{
		// clear it...
		//
		memset(ptr, 0, sizeof( *ptr ) );
	}

	return ptr;
}
