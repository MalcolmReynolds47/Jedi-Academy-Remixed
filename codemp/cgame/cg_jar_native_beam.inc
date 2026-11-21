// Native ET_BEAM rendering, shared with direct production fixtures. GPL-2.0-only.
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;
/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);

/*
Ghoul2 Insert End
*/
	// add to refresh list
	trap->R_AddRefEntityToScene(&ent);
}

