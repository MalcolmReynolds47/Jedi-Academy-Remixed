// Native Raven/OpenJK callbacks shared with the headless regression fixture.
// GPL-2.0-only; copyright and license retained in the including source file.
extern qboolean G_ClearViewEntity( gentity_t *ent );
extern void G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
extern void SP_fx_runner( gentity_t *ent );
void camera_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	if ( player && player->client && player->client->ps.viewEntity == self->s.number )
	{
		G_UseTargets2( self, player, self->target4 );
		G_ClearViewEntity( player );
		G_Sound( player, self->soundPos2 );
	}
	G_UseTargets2( self, player, self->closetarget );
	//FIXME: explosion fx/sound
	//leave sparks at origin- where base's pole is still at?
	gentity_t *sparks = G_TrySpawn();
	if ( sparks )
	{
		sparks->classname = G_NewString("fx_runner");
		sparks->fxFile = G_NewString("sparks/spark");
		sparks->delay = 100;
		sparks->random = 500;
		sparks->s.angles[0] = 180;//point down
		VectorCopy( self->s.origin, sparks->s.origin );
		SP_fx_runner( sparks );
		if (!G_JAR_WorkshopTrackChild(self, sparks)) G_FreeEntity(sparks);
	}

	//bye!
	self->takedamage = qfalse;
	self->contents = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelindex = 0;
}

void camera_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator || !activator->client || activator->s.number )
	{//really only usable by the player
		return;
	}
	self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms

	// FIXME: I guess we are allowing them to switch to a dead camera.  Maybe we should conditionally do this though?
	if ( /*self->health <= 0 ||*/ (player && player->client && player->client->ps.viewEntity == self->s.number) )
	{//I'm already viewEntity, or I'm destroyed, find next
		gentity_t *next = NULL;
		if ( self->target2 != NULL )
		{
			next = G_Find( NULL, FOFS(targetname), self->target2 );
		}
		if ( next )
		{//found another one
			if ( !Q_stricmp( "misc_camera", next->classname ) )
			{//make sure it's another camera
				camera_use( next, other, activator );
			}
		}
		else //if ( self->health > 0 )
		{//I was the last (only?) one, clear out the viewentity
			G_UseTargets2( self, activator, self->target4 );
			G_ClearViewEntity( activator );
			G_Sound( activator, self->soundPos2 );
		}
	}
	else
	{//set me as view entity
		G_UseTargets2( self, activator, self->target3 );
		self->s.eFlags |= EF_NODRAW;
		self->s.modelindex = 0;
		G_SetViewEntity( activator, self );
		G_Sound( activator, self->soundPos1 );
	}
}

void camera_aim( gentity_t *self )
{
	self->nextthink = level.time + FRAMETIME;
	if ( player && player->client && player->client->ps.viewEntity == self->s.number )
	{//I am the viewEntity
		if ( player->client->usercmd.forwardmove || player->client->usercmd.rightmove || player->client->usercmd.upmove )
		{//player wants to back out of camera
			G_UseTargets2( self, player, self->target4 );
			G_ClearViewEntity( player );
			G_Sound( player, self->soundPos2 );
			self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms
			if ( player->client->usercmd.upmove > 0 )
			{//stop player from doing anything for a half second after
				player->aimDebounceTime = level.time + 500;
			}
		}
		else if ( self->painDebounceTime < level.time )
		{//check for use button
			if ( (player->client->usercmd.buttons&BUTTON_USE) )
			{//player pressed use button, wants to cycle to next
				camera_use( self, player, player );
			}
		}
		else
		{//don't draw me when being looked through
			self->s.eFlags |= EF_NODRAW;
			self->s.modelindex = 0;
		}
	}
	else if ( self->health > 0 )
	{//still alive, can draw me again
		self->s.eFlags &= ~EF_NODRAW;
		self->s.modelindex = self->s.modelindex3;
	}
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			vec3_t angles, dir;
			VectorSubtract( targ->currentOrigin, self->currentOrigin, dir );
			vectoangles( dir, angles );
			//FIXME: if a G2 model, do a bone override..???
			VectorCopy( self->currentAngles, self->s.apos.trBase );

			for( int i = 0; i < 3; i++ )
			{
				angles[i] = AngleNormalize180( angles[i] );
				self->s.apos.trDelta[i] = AngleNormalize180( (angles[i]-self->currentAngles[i])*10 );
			}
			//VectorSubtract( angles, self->currentAngles, self->s.apos.trDelta );
			//VectorScale( self->s.apos.trDelta, 10, self->s.apos.trDelta );
			self->s.apos.trTime = level.time;
			self->s.apos.trDuration = FRAMETIME;
			VectorCopy( angles, self->currentAngles );

			if ( DistanceSquared( self->currentAngles, self->lastAngles ) > 0.01f ) // if it moved at all, start a loop sound? not exactly the "bestest" solution
			{
				self->s.loopSound = G_SoundIndex( "sound/movers/objects/cameramove_lp2" );
			}
			else
			{
				self->s.loopSound = 0; // not moving so don't bother
			}

			VectorCopy( self->currentAngles, self->lastAngles );
			//G_SetAngles( self, angles );
		}
	}
}
/*QUAKED misc_camera (0 0 1) (-8 -8 -12) (8 8 16) VULNERABLE
A model in the world that can be used by the player to look through it's viewpoint

There will be a video overlay instead of the regular HUD and the FOV will be wider

VULNERABLE - allow camera to be destroyed

"target" - camera will remain pointed at this entity (if it's a train or some other moving object, it will keep following it)
"target2" - when player is in this camera and hits the use button, it will cycle to this next camera (if no target2, returns to normal view )
"target3" - thing to use when player enters this camera view
"target4" - thing to use when player leaves this camera view
"closetarget" - (sigh...) yet another target, fired this when it's destroyed
"wait"    - how long to wait between being used (default 0.5)
*/
void SP_misc_camera( gentity_t *self )
{
	G_SpawnFloat( "wait", "0.5", &self->wait );

	//FIXME: spawn base, too
	gentity_t *base = G_JAR_SpawnSetupChild(self);
	if ( base )
	{
		base->classname = G_NewString("misc_model_static");
		base->model = G_NewString("models/map_objects/kejim/impcam_base.md3");
		base->s.modelindex = G_ModelIndex(base->model);
		VectorSet(base->mins,-5.5f,-6.35f,-17.55f); VectorSet(base->maxs,5.5f,6.36f,0);
		VectorCopy( self->s.origin, base->s.origin );
		base->s.origin[2] += 16;
		G_SetOrigin( base, base->s.origin );
		G_SetAngles( base, self->s.angles );
		gi.linkentity( base );
	}
	self->s.modelindex3 = self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam.md3" );
	// The original camera_on/off paths are absent from retail Jedi Academy.
	self->soundPos1 = G_SoundIndex( "sound/movers/switches/button_08.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/switches/button_09.mp3" );
	G_SoundIndex( "sound/movers/objects/cameramove_lp2" );

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	self->s.apos.trType = TR_LINEAR_STOP;//TR_INTERPOLATE;//
	self->alt_fire = qtrue;
	VectorSet( self->mins, -8, -8, -12 );
	VectorSet( self->maxs, 8, 8, 0 );
	self->contents = CONTENTS_SOLID;
	gi.linkentity( self );

	self->fxID = G_EffectIndex( "sparks/spark" );

	if ( self->spawnflags & 1 ) // VULNERABLE
	{
		self->takedamage = qtrue;
	}

	self->health = 10;
	self->e_DieFunc = dieF_camera_die;

	self->e_UseFunc = useF_camera_use;

	self->e_ThinkFunc = thinkF_camera_aim;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}
