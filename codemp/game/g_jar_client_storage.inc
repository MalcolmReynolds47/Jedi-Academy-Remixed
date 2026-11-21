// Entity-owned storage does not consume the small shared animation/string arena.
static gclient_t jarClientStorage[MAX_GENTITIES];
gclient_t *gClPtrs[MAX_GENTITIES];

void G_CreateFakeClient(int entNum, gclient_t **cl)
{
	if (!cl) return;
	if (entNum < 0 || entNum >= ENTITYNUM_WORLD) { *cl = NULL; return; }
	if (!gClPtrs[entNum])
	{
		gClPtrs[entNum] = &jarClientStorage[entNum];
	}
	*cl = gClPtrs[entNum];
}

