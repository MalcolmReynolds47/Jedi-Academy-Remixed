// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_func_plat (gentity_t *ent) {
	float		lip, height;

//	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
//	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");

	VectorClear (ent->s.angles);

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	G_JAR_SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = (ent->maxs[2] - ent->mins[2]) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	vec3_t local = {0, 0, -height}, offset;
	jar::subBSPMover::Vector(*ent, local, offset);
	VectorAdd(ent->pos1, offset, ent->pos1);
	jar::subBSPMover::Orient(*ent);

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->e_TouchFunc = touchF_Touch_Plat;

	ent->e_BlockedFunc = blockedF_Blocked_Door;

	ent->owner = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger(ent);
	}
}
