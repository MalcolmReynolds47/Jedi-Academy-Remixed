// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
static void MultiTriggerTeam(gentity_t& ent,bool siege){
	if(!ent.team||!ent.team[0])return;
	if(siege&&!strcmp(ent.team,"1"))ent.noDamageTeam=TEAM_PLAYER;
	else if(siege&&!strcmp(ent.team,"2"))ent.noDamageTeam=TEAM_ENEMY;
	else ent.noDamageTeam=static_cast<team_t>(GetIDForString(TeamTable,ent.team));
	ent.team=nullptr;
}

void SP_trigger_multiple( gentity_t *ent )
{
	int siege;
	G_SpawnInt("siegetrig","0",&siege);
	if(siege)ent->spawnflags|=jar::siege::requiresItem;
	char	buffer[MAX_QPATH];
	char	*s;
	if ( G_SpawnString( "noise", "*NOSOUND*", &s ) )
	{
		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		ent->noise_index = G_SoundIndex(buffer);
	}

	G_SpawnFloat( "wait", "0", &ent->wait );//was 0.5 ... but that means wait can never be zero... we should probably put it back to 0.5, though...
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnInt( "max_pilots", "2", &ent->lastInAirTime );


	if ( (ent->wait > 0) && (ent->random >= ent->wait) ) {
		ent->random = ent->wait - FRAMETIME;
		gi.Printf(S_COLOR_YELLOW"trigger_multiple has random >= wait\n");
	}

	ent->delay *= 1000;//1 = 1 msec, 1000 = 1 sec
	if ( !ent->speed && ent->target2 && ent->target2[0] )
	{
		ent->speed = 1000;
	}
	else
	{
		ent->speed *= 1000;
	}

	ent->e_TouchFunc = touchF_Touch_Multi;
	ent->e_UseFunc   = useF_Use_Multi;

	MultiTriggerTeam(*ent,siege!=0);

	InitTrigger( ent );
	gi.linkentity (ent);
}
