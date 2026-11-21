// Native FinishSpawningItem, unchanged. GPL-2.0-only.
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;
//	gitem_t		*item;

//	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
//	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	if (level.gametype == GT_SIEGE)
	{ //in siege remove all powerups
		if (ent->item->giType == IT_POWERUP)
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype != GT_JEDIMASTER)
	{
		if (HasSetSaberOnly())
		{
			if (ent->item->giType == IT_AMMO)
			{
				G_FreeEntity( ent );
				return;
			}

			if (ent->item->giType == IT_HOLDABLE)
			{
				if (ent->item->giTag == HI_SEEKER ||
					ent->item->giTag == HI_SHIELD ||
					ent->item->giTag == HI_SENTRY_GUN)
				{
					G_FreeEntity( ent );
					return;
				}
			}
		}
	}
	else
	{ //no powerups in jedi master
		if (ent->item->giType == IT_POWERUP)
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype == GT_HOLOCRON)
	{
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	if (g_forcePowerDisable.integer)
	{ //if force powers disabled, don't add force powerups
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK ||
				ent->item->giTag == PW_FORCE_BOON)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if ( ent->item->giType == IT_ARMOR ||
			ent->item->giType == IT_HEALTH ||
			(ent->item->giType == IT_HOLDABLE && (ent->item->giTag == HI_MEDPAC || ent->item->giTag == HI_MEDPAC_BIG)) )
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype != GT_CTF &&
		level.gametype != GT_CTY &&
		ent->item->giType == IT_TEAM)
	{
		int killMe = 0;

		switch (ent->item->giTag)
		{
		case PW_REDFLAG:
			killMe = 1;
			break;
		case PW_BLUEFLAG:
			killMe = 1;
			break;
		case PW_NEUTRALFLAG:
			killMe = 1;
			break;
		default:
			break;
		}

		if (killMe)
		{
			G_FreeEntity( ent );
			return;
		}
	}

	VectorSet (ent->r.mins, -8, -8, -0);
	VectorSet (ent->r.maxs, 8, 8, 16);

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;

	// create a Ghoul2 model if the world model is a glm
/*	item = &bg_itemlist[ ent->s.modelindex ];
	if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
	{
		trap->G2API_InitGhoul2Model(&ent->s, item->world_model[0], G_ModelIndex(item->world_model[0] ), 0, 0, 0, 0);
		ent->s.radius = 60;
	}
*/
	if (JAR_MP_SectionPickup(ent)) {
		if (!JAR_MP_PlaceSectionPickup(ent)) {
			G_FreeEntity(ent);
			return;
		}
	} else if ( ent->spawnflags & ITMSF_SUSPEND ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor

		//if it is directly even with the floor it will return startsolid, so raise up by 0.1
		//and temporarily subtract 0.1 from the z maxs so that going up doesn't push into the ceiling
		ent->s.origin[2] += 0.1f;
		ent->r.maxs[2] -= 0.1f;

		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap->Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID, qfalse, 0, 0 );
		if ( tr.startsolid ) {
			trap->Print ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		//add the 0.1 back after the trace
		ent->r.maxs[2] += 0.1f;

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// powerups don't spawn in for a while
	/*
	if ( ent->item->giType == IT_POWERUP ) {
		float	respawn;

		respawn = 45 + Q_flrand(-1.0f, 1.0f) * 15;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}
	*/

	trap->LinkEntity ((sharedEntity_t *)ent);
}
