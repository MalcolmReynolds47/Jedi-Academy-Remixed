// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if (!jar::subBSPMover::Touches(*ent, *other)) return;

	if ( ent->svFlags & SVF_INACTIVE )
	{
		return;
	}

	if ( ent->owner->spawnflags & MOVER_LOCKED )
	{//don't even try to use the door if it's locked
		return;
	}

	if ( ent->owner->moverState != MOVER_1TO2 )
	{//Door is not already opening
		//if ( ent->owner->moverState == MOVER_POS1 || ent->owner->moverState == MOVER_2TO1 )
		//{//only check these if closed or closing

		//If door is closed, opening or open, check this
		Use_BinaryMover( ent->owner, ent, other );
	}

	/*
	//Old style
	if ( ent->owner->moverState != MOVER_1TO2 ) {
		Use_BinaryMover( ent->owner, ent, other );
	}
	*/
}
