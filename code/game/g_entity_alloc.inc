// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void G_InitGentity( gentity_t *e, qboolean bFreeG2 )
{
	e->inuse = qtrue;
	SetInUse(e);
	e->m_iIcarusID = IIcarusInterface::ICARUS_INVALID;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	G_SubBSPAllocated(e);

	// remove any ghoul2 models here in case we're reusing
	if (bFreeG2 && e->ghoul2.IsValid())
	{
		gi.G2API_CleanGhoul2Models(e->ghoul2);
	}
	//Navigational setups
	e->waypoint				= WAYPOINT_NONE;
	e->lastWaypoint			= WAYPOINT_NONE;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
static unsigned int jarRejectedSpawns;
static int jarNextCapacityWarning;

void G_ResetEntityCapacityStats(void)
{
	jarRejectedSpawns = 0;
	jarNextCapacityWarning = 0;
}

void G_EntityCapacity_f(void)
{
	int active = 0, optional = 0;
	for (int i = MAX_CLIENTS; i < ENTITYNUM_WORLD; ++i)
	{
		if (!PInUse(i)) continue;
		++active;
		if (i < ENTITYNUM_WORLD - JAR_ENTITY_RESERVE) ++optional;
	}
	gi.Printf("JA:R capacity: %d slots; %d active normal entities; %d/%d optional-region slots; "
		"%d/%d reserve slots; %u rejected optional spawns\n",
		MAX_GENTITIES, active, optional, ENTITYNUM_WORLD - JAR_ENTITY_RESERVE - MAX_CLIENTS,
		active - optional, JAR_ENTITY_RESERVE, jarRejectedSpawns);
}

gentity_t *G_TrySpawn(qboolean essential)
{
	if (G_OptionalSpawnActive()) essential=qfalse;
	const int limit = ENTITYNUM_WORLD - (essential ? 0 : JAR_ENTITY_RESERVE);
	const int slot = !G_SpawnTransactionCanAllocate() ? -1 : jar::FindEntitySlot(MAX_CLIENTS, globals.num_entities, limit, level.time,
		[](int i) { return PInUse(i) != qfalse; },
		[](int i) { return g_entities[i].freetime; });
	if (slot < 0)
	{
		G_RejectSpawnAllocation();
		if(gi.JAR_Log) gi.JAR_Log(1,"entity","exhausted","essential=%d high_water=%d limit=%d rejected=%u",essential,globals.num_entities,limit,jarRejectedSpawns);
		if (!essential)
		{
			++jarRejectedSpawns;
			if (jarRejectedSpawns == 1 || level.time >= jarNextCapacityWarning)
			{
				gi.Printf("JA:R: optional spawn skipped; entity capacity reached (critical reserve preserved).\n");
				jarNextCapacityWarning = level.time + 5000;
			}
		}
		return NULL;
	}
	if (slot >= globals.num_entities) globals.num_entities = slot + 1;
	gentity_t *entity = &g_entities[slot];
	G_InitGentity(entity, qtrue);
	G_RecordSpawnAllocation(entity);
	if(gi.JAR_Log) gi.JAR_Log(2,"entity","allocated","entity=%d essential=%d time=%d high_water=%d",slot,essential,level.time,globals.num_entities);
	return entity;
}

gentity_t *G_Spawn(void)
{
	gentity_t *entity = G_TrySpawn(qtrue);
	if (!entity)
	{
		if (G_OptionalSpawnActive()) throw G_SpawnCapacity{};
		// Required map/script entities cannot be silently lost. Return to the
		// menu through ERR_DROP; never dereference NULL or overwrite a live slot.
		G_Error("JA:R: all %d entity slots exhausted, including the %d-slot critical reserve. "
			"The level cannot create a required entity.", MAX_GENTITIES, JAR_ENTITY_RESERVE);
	}
	return entity;
}

