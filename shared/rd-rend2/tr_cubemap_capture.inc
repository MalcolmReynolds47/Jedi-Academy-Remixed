// Native six-face capture body, shared with the lifecycle fixture. GPL-2.0-or-later.
void R_RenderCubemapSide(int cubemapIndex, int cubemapSide, bool bounce)
{
	refdef_t refdef = {};
	if (tr.jarProbes.capturing) refdef.time = tr.jarProbes.captureTime;

	VectorCopy(tr.cubemaps[cubemapIndex].origin, refdef.vieworg);
	refdef.fov_x = 90;
	refdef.fov_y = 90;
	refdef.width = tr.renderCubeFbo[cubemapSide]->width;
	refdef.height = tr.renderCubeFbo[cubemapSide]->height;
	refdef.x = 0;
	refdef.y = 0;

	switch (cubemapSide)
	{
	case 0:
		// +X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 1:
		// -X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 2:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 3:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 4:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 5:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	}

	RE_BeginFrame((glConfig.stereoEnabled || r_anaglyphMode->integer) ? STEREO_LEFT : STEREO_CENTER);

	RE_BeginScene(&refdef);

	tr.refdef.colorScale = 1.0f;

	for (int i = 0; i < tr.numCachedViewParms; i++)
	{
		if (!tr.cachedViewParms[i].targetFbo)
		{
			tr.cachedViewParms[i].targetFbo = tr.renderCubeFbo[cubemapSide];
			tr.cachedViewParms[i].targetFboLayer = 0;
			tr.cachedViewParms[i].flags |= VPF_NOVIEWMODEL;
			if (!bounce)
				tr.cachedViewParms[i].flags |= VPF_NOCUBEMAPS;
		}
#ifdef REND2_SP
		R_PushDebugGroup(AL_VIEW, va("Cubemap %i, side %i", cubemapIndex, cubemapSide));
#endif
		R_RenderView(&tr.cachedViewParms[i]);
		R_IssuePendingRenderCommands();
		tr.refdef.numDrawSurfs = 0;
	}

	RE_EndScene();

	R_NewFrameSync();
}
