// Native NPC surface deltas, with the original table retained for other classes.
	if (cent->npcLocalSurfOff != cent->currentState.surfacesOff ||
		cent->npcLocalSurfOn != cent->currentState.surfacesOn)
	{ //looks like it's time for an update.
		int i = 0;
		int galak = cent->currentState.NPC_class == CLASS_GALAKMECH;
		const char *surface;

		while (i < BG_NUM_TOGGLEABLE_SURFACES && (surface = galak ? JAR_GalakSurfaceName(i) : bgToggleableSurfaces[i]))
		{
			if (!(cent->npcLocalSurfOff & (1 << i)) &&
				(cent->currentState.surfacesOff & (1 << i)))
			{ //it wasn't off before but it's off now, so reflect this change in the g2 instance.
				if (!galak && bgToggleableSurfaceDebris[i] > 0)
				{ //make some local debris of this thing?
					//FIXME: throw off the proper model effect, too
					CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestDestroyed, qtrue);
				}

				trap->G2API_SetSurfaceOnOff(cent->ghoul2, surface, TURN_OFF);
			}

			if (!(cent->npcLocalSurfOn & (1 << i)) &&
				(cent->currentState.surfacesOn & (1 << i)))
			{ //same as above, but on instead of off.
				trap->G2API_SetSurfaceOnOff(cent->ghoul2, surface, TURN_ON);
			}

			i++;
		}

		cent->npcLocalSurfOff = cent->currentState.surfacesOff;
		cent->npcLocalSurfOn = cent->currentState.surfacesOn;
	}

