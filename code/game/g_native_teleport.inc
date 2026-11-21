// Native actor relocation shared by authored and Sandbox teleporters.
// GPL-2.0-only.
static void MoveTeleportedActor( gentity_t *player, vec3_t origin, vec3_t angles, bool telefrag, bool keepVelocity )
{
	if ( player->NPC && ( player->NPC->aiFlags&NPCAI_FORM_TELE_NAV ) )
	{
		//My leader teleported, I was trying to catch up, take this off
		player->NPC->aiFlags &= ~NPCAI_FORM_TELE_NAV;

	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	gi.unlinkentity (player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;
	VectorCopy ( player->client->ps.origin, player->currentOrigin );

	// spit the player out
	if(!keepVelocity)VectorClear(player->client->ps.velocity);
	//player->client->ps.pm_time = 160;		// hold time
	//player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	if(telefrag)G_KillBox(player);

	// save results of pmove
	PlayerStateToEntityState( &player->client->ps, &player->s );

	gi.linkentity (player);
}

void TeleportPlayer(gentity_t* player,vec3_t origin,vec3_t angles){
	MoveTeleportedActor(player,origin,angles,true,false);
}

void G_JAR_TeleportActor(gentity_t* player,vec3_t origin,vec3_t angles,bool keepVelocity){
	player->client->ps.groundEntityNum=ENTITYNUM_NONE;
	player->client->ps.pm_flags&=~PMF_TRIGGER_PUSHED;
	player->client->ps.jumpZStart=origin[2]+1;
	player->client->ps.forceJumpZStart=0;
	MoveTeleportedActor(player,origin,angles,false,keepVelocity);
	VectorCopy(angles,player->currentAngles);
	VectorCopy(player->currentOrigin,player->lastOrigin);
}

