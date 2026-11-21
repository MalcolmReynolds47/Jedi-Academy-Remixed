// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
void Touch_PlatCenterTrigger(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if (!jar::subBSPMover::Touches(*ent, *other)) return;

	if ( !other->client ) {
		return;
	}

	if ( ent->owner->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->owner, ent, other );
	}
}
