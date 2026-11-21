// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void SP_func_button( gentity_t *ent ) {
	float		distance;
	float		lip;

//	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");

	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	G_JAR_SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	jar::subBSPMover::MoveDirection(*ent);
	distance = jar::subBSPMover::Distance(*ent, ent->movedir, lip);
	jar::subBSPMover::Orient(*ent);
	VectorMA (ent->pos1, distance, ent->movedir, ent->pos2);

	if (ent->health) {
		// shootable button
		ent->takedamage = qtrue;
	} else {
		// touchable button
		ent->e_TouchFunc = touchF_Touch_Button;
	}

	InitMover( ent );
}
