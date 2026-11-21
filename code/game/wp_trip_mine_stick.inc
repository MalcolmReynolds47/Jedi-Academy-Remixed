// Native planting callback shared with the Sandbox charge fixture. GPL-2.0-only.
//---------------------------------------------------------
void touchLaserTrap( gentity_t *ent, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	ent->s.eType = ET_GENERAL;

	// a tripwire so add draw line flag
	VectorCopy( trace->plane.normal, ent->movedir );

	// make it shootable
	VectorSet( ent->mins, -4, -4, -4 );
	VectorSet( ent->maxs, 4, 4, 4 );

	ent->clipmask = MASK_SHOT;
	ent->contents = CONTENTS_SHOTCLIP;
	ent->takedamage = qtrue;
	ent->health = 15;

	ent->e_DieFunc = dieF_WP_ExplosiveDie;
	ent->e_TouchFunc = touchF_NULL;

	// so we can trip it too
	ent->activator = ent->owner;
	ent->owner = NULL;

	WP_Stick( ent, trace );

	if ( ent->count == TRIPWIRE_STYLE )
	{
		vec3_t		mins = {-4,-4,-4}, maxs = {4,4,4};//FIXME: global these
		trace_t		tr;
		VectorMA( ent->currentOrigin, 32, ent->movedir, ent->s.origin2 );
		gi.trace( &tr, ent->s.origin2, mins, maxs, ent->currentOrigin, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 0 );
		VectorCopy( tr.endpos, ent->s.origin2 );

		ent->e_ThinkFunc = thinkF_laserTrapThink;
	}
	else
	{
		ent->e_ThinkFunc = thinkF_WP_prox_mine_think;
	}

	ent->nextthink = level.time + LT_ACTIVATION_DELAY;
	if(trace->entityNum==ENTITYNUM_WORLD)ent->s.groundEntityNum=ENTITYNUM_WORLD;
	if(other)G_SandboxChargeAttach(*ent,*other);
}

