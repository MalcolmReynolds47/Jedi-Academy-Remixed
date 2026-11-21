// Native pilot attachment; shared by production and headless seat fixtures. GPL-2.0-only.
void AttachRidersGeneric( Vehicle_t *pVeh )
{
	// If we have a pilot, attach him to the driver tag.
	if ( pVeh->m_pPilot )
	{
		mdxaBone_t boltMatrix;
		vec3_t	yawOnlyAngles;
		bgEntity_t *parent = pVeh->m_pParentEntity;
		bgEntity_t *pilot = pVeh->m_pPilot;
		int crotchBolt = trap->G2API_AddBolt(parent->ghoul2, 0, "*driver");

		assert(parent->playerState);

		VectorSet(yawOnlyAngles, 0, parent->playerState->viewangles[YAW], 0);

		// Get the driver tag.
		trap->G2API_GetBoltMatrix( parent->ghoul2, 0, crotchBolt, &boltMatrix, yawOnlyAngles, parent->playerState->origin, BG_GetTime(), NULL, parent->modelScale );
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, pilot->playerState->origin );
	}
}
