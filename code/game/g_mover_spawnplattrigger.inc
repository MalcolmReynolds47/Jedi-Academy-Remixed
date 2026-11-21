// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t	*trigger;
	vec3_t	tmin, tmax;
	const bool section = jar::subBSPMover::Root(*ent) != nullptr;
	const float *base = section ? vec3_origin : ent->pos1;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->e_TouchFunc = touchF_Touch_PlatCenterTrigger;
	trigger->contents = CONTENTS_TRIGGER;
	trigger->owner = ent;

	tmin[0] = base[0] + ent->mins[0] + 33;
	tmin[1] = base[1] + ent->mins[1] + 33;
	tmin[2] = base[2] + ent->mins[2];

	tmax[0] = base[0] + ent->maxs[0] - 33;
	tmax[1] = base[1] + ent->maxs[1] - 33;
	tmax[2] = base[2] + ent->maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = base[0] + (ent->mins[0] + ent->maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = base[1] + (ent->mins[1] + ent->maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	if (section)
	{
		jar::subBSPMover::Trigger(*trigger, *ent, tmin, tmax);
		VectorCopy(ent->pos1, trigger->currentOrigin);
		VectorCopy(ent->pos1, trigger->s.origin);
		VectorCopy(ent->pos1, trigger->s.pos.trBase);
	}
	else
	{
		VectorCopy(tmin, trigger->mins);
		VectorCopy(tmax, trigger->maxs);
	}

	gi.linkentity (trigger);
}
