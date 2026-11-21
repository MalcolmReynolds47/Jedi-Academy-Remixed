// Native Force pickup behavior, shared with the headless regression fixture. GPL-2.0-only.
int Pickup_Holocron( gentity_t *ent, gentity_t *other )
{
	if(!ent||!ent->item||!other||!other->client)return 1;
	int forcePower = ent->item->giTag;
	if(forcePower<0||forcePower>=NUM_FORCE_POWERS)return 1;
	int forceLevel = ent->count;
	// check if out of range
	if( forceLevel < 0 || forceLevel >= NUM_FORCE_POWER_LEVELS )
	{
		gi.Printf(" Pickup_Holocron : count %d not in valid range\n", forceLevel );
		return 1;
	}

	// don't pick up if already known AND your level is higher than pickup level
	if ( ( other->client->ps.forcePowersKnown & ( 1 << forcePower )) )
	{
		//don't pickup if item is lower than current level
		if( other->client->ps.forcePowerLevel[forcePower] >= forceLevel )
		{
			return 1;
		}
	}

	other->client->ps.forcePowerLevel[forcePower] = forceLevel;
	other->client->ps.forcePowersKnown |= ( 1 << forcePower );

	missionInfo_Updated = qtrue;	// Activate flashing text
	gi.cvar_set("cg_updatedDataPadForcePower1", va("%d",forcePower+1)); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower1.integer = forcePower+1;
	gi.cvar_set("cg_updatedDataPadForcePower2", "0"); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower2.integer = 0;
	gi.cvar_set("cg_updatedDataPadForcePower3", "0"); // The +1 is offset in the print routine.
	cg_updatedDataPadForcePower3.integer = 0;

	return 1;
}
