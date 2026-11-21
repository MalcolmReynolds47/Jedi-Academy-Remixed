// Native model-specific surface replication. GPL-2.0-only.
void NPC_SetSurfaceOnOff(gentity_t *ent, const char *surfaceName, int surfaceFlags)
{
	int i = 0;
	int galak=ent->client && ent->client->NPC_class==CLASS_GALAKMECH;
	const char *surface;
	qboolean foundIt = qfalse;

	while (i < BG_NUM_TOGGLEABLE_SURFACES && (surface=galak?JAR_GalakSurfaceName(i):bgToggleableSurfaces[i])!=NULL)
	{
		if (!Q_stricmp(surfaceName, surface))
		{ //got it
			foundIt = qtrue;
			break;
		}
		i++;
	}

	if (!foundIt)
	{
		Com_Printf("WARNING: Tried to toggle NPC surface that isn't in toggleable surface list (%s)\n", surfaceName);
		return;
	}

	if (surfaceFlags == TURN_ON)
	{ //Make sure the entitystate values reflect this surface as on now.
		ent->s.surfacesOn |= (1 << i);
		ent->s.surfacesOff &= ~(1 << i);
	}
	else
	{ //Otherwise make sure they're off.
		ent->s.surfacesOn &= ~(1 << i);
		ent->s.surfacesOff |= (1 << i);
	}

	if (!ent->ghoul2)
	{
		return;
	}

	trap->G2API_SetSurfaceOnOff(ent->ghoul2, surfaceName, surfaceFlags);
}
