// Native OpenJK map loading/cache control. GPL-2.0-only.
static void CM_LoadMap_Actual( const char *name, qboolean clientload, int *checksum, clipMap_t &cm ) {
	const int		*buf;
	size_t			i;
	dheader_t		header;
	void			*subBSPData = NULL;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE_ND|CVAR_CHEAT );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload ) {
		*checksum = cm.checksum;
		return;
	}

	if (&cm == &cmg)
	{
		// if there was a cached disk image but the name was empty (ie ERR_DROP happened) or just doesn't match
		//	the current name, then ditch it...
		//
		if (gpvCachedMapDiskImage &&
			(gsCachedMapDiskImage[0] == '\0' || strcmp( gsCachedMapDiskImage, name ))
			)
		{
			Z_Free(gpvCachedMapDiskImage);
				   gpvCachedMapDiskImage = NULL;
		   gsCachedMapDiskImage[0] = '\0';

		   CM_ClearMap();
		}
	}

	// if there's a valid map name, and it's the same as last time (respawn?), and it's the server-load,
	//	then keep the data from last time...
	//
	if (name[0] && !strcmp( cm.name, name ) && !clientload && &cm == &cmg )
	{
		// clear some stuff that needs zeroing...
		//
		cm.floodvalid = 0;
		//NO... don't reset this because the brush checkcounts are cached,
		//so when you load up, brush checkcounts equal the cm.checkcount
		//and the trace will be skipped (because everything loads and
		//traces in the same exact order ever time you load the map)
		cm.checkcount++;// = 0;
		memset(cm.areas,		0, cm.numAreas * sizeof( *cm.areas ));
		memset(cm.areaPortals,	0, cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ));
	}
	else
	{
		// ... else load map from scratch...
		//
		if (&cm == &cmg)
		{
			assert(!clientload);	// logic check. I'm assuming that a client load doesn't get this far?

			// free old stuff
			memset( &cm, 0, sizeof( cm ) );
			CM_ClearLevelPatches();
			Z_TagFree(TAG_BSP);

			if ( !name[0] ) {
				cm.numLeafs = 1;
				cm.numClusters = 1;
				cm.numAreas = 1;
				cm.cmodels = (struct cmodel_s *) Z_Malloc( sizeof( *cm.cmodels ), TAG_BSP, qtrue );
				*checksum = 0;
				return;
			}
		}

		// load the file into a buffer that we either discard as usual at the bottom, or if we've got enough memory
		//	then keep it long enough to save the renderer re-loading it, then discard it after that.
		//
		fileHandle_t h;
		const int iBSPLen = FS_FOpenFileRead( name, &h, qfalse );
		if(!h)
		{
			Com_Error (ERR_DROP, "Couldn't load %s", name);
			return;
		}
		//rww - only do this when not loading a sub-bsp!
		if (&cm == &cmg)
		{
			if (gpvCachedMapDiskImage && gsCachedMapDiskImage[0])
			{ //didn't get cleared elsewhere so free it before we allocate the pointer again
			  //Maps with terrain will allow this to happen because they want everything to be cleared out (going between terrain and no-terrain is messy)
				Z_Free(gpvCachedMapDiskImage);
			}
			gsCachedMapDiskImage[0]	= '\0';		// flag that map isn't valid, until name is filled in
			gpvCachedMapDiskImage	= Z_Malloc( iBSPLen, TAG_BSP_DISKIMAGE, qfalse);
			FS_Read(gpvCachedMapDiskImage, iBSPLen, h);
			FS_FCloseFile( h );

			buf = (int*) gpvCachedMapDiskImage;	// so the rest of the code works as normal
		}
		else
		{ //otherwise, read straight in..
			subBSPData = Z_Malloc( iBSPLen, TAG_BSP_DISKIMAGE, qfalse);
			FS_Read(subBSPData, iBSPLen, h);
			FS_FCloseFile( h );

			buf = (int*)subBSPData;
		}

		// carry on as before...

		cm.checksum = LittleLong (Com_BlockChecksum (buf, iBSPLen));

		header = *(dheader_t *)buf;
		for (i=0 ; i<sizeof(dheader_t)/4 ; i++) {
			((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);
		}

		if ( header.version != BSP_VERSION )
		{
			Z_Free(	gpvCachedMapDiskImage);
					gpvCachedMapDiskImage = NULL;

			Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
			, name, header.version, BSP_VERSION );
		}

		cmod_base = (byte *)buf;

		// load into heap
		CMod_LoadShaders( &header.lumps[LUMP_SHADERS], cm );
		CMod_LoadLeafs (&header.lumps[LUMP_LEAFS], cm);
		CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES], cm);
		CMod_LoadLeafSurfaces (&header.lumps[LUMP_LEAFSURFACES], cm);
		CMod_LoadPlanes (&header.lumps[LUMP_PLANES], cm);
		CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES], cm);
		CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES], cm);
		CMod_LoadSubmodels (&header.lumps[LUMP_MODELS], cm);
		CMod_LoadNodes (&header.lumps[LUMP_NODES], cm);
		CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES], cm, name );
		CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY], cm );
		CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], cm );

		TotalSubModels += cm.numSubModels;

		// we are NOT freeing the file, because it is cached for the ref
		//actually we do because the new hunk sys won't allow it
		// actually we DON'T now <g>, if we've got enough ram to keep it for the renderer's disk-load...
		//
		extern qboolean Sys_LowPhysicalMemory();
		if (Sys_LowPhysicalMemory() //|| com_dedicated->integer	// no need to check for dedicated in single-player codebase
			)
		{
			Z_Free(	gpvCachedMapDiskImage );
					gpvCachedMapDiskImage = NULL;
		}
		else
		{
			// ... do nothing, and let the renderer free it after it's finished playing with it...
			//
		}

		if (subBSPData)
		{
			Z_Free(subBSPData);
		}

		if (&cm == &cmg)
		{
			CM_InitBoxHull ();

			Q_strncpyz( gsCachedMapDiskImage, name, sizeof(gsCachedMapDiskImage) );	// so the renderer can check it
		}
	}

	*checksum = cm.checksum;

	// do this whether or not the map was cached from last load...
	//
	CM_FloodAreaConnections (cm);

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		Q_strncpyz( cm.name, name, sizeof( cm.name ) );
	}
	CM_CleanLeafCache();
}

