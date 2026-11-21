// Production next-frame reset; included by the headless scene-lifetime test.
void R_InitNextFrame( void ) {
	backEndData->commands.used = 0;
#ifdef REND2_SP
	// Portable sky controllers can be removed during play.
	if (tr.world) tr.world->skyboxportal = 0;
#endif

	tr.numTimedBlocks = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
#ifdef REND2_SP
	backEndData->numJarPortals=0;
#endif

	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}
