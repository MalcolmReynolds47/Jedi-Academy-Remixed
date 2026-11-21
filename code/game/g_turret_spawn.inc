// Native Raven turret setup shared with the offline fixture. GPL-2.0-only.
void finish_spawning_turret( gentity_t *base )
{
	vec3_t		fwd;

	if ( base->spawnflags & 2 )
	{
		base->s.angles[ROLL] += 180;
		base->s.origin[2] -= 22.0f;
	}

	G_SetAngles( base, base->s.angles );
	AngleVectors( base->currentAngles, fwd, NULL, NULL );

	G_SetOrigin(base, base->s.origin);

	base->noDamageTeam = TEAM_ENEMY;

	base->s.eType = ET_GENERAL;

	if ( base->team && base->team[0] )
	{
		base->noDamageTeam = (team_t)GetIDForString( TeamTable, base->team );
		base->team = NULL;
	}

	// Set up our explosion effect for the ExplodeDeath code....
	base->fxID = G_EffectIndex( "turret/explode" );
	G_EffectIndex( "sparks/spark_exp_nosnd" );

	base->e_UseFunc = useF_turret_base_use;
	base->e_PainFunc = painF_TurretPain;

	// don't start working right away
	base->e_ThinkFunc = thinkF_turret_base_think;
	base->nextthink = level.time + FRAMETIME * 5;

	// this is really the pitch angle.....
	base->speed = 0;

	G_SpawnFloat( "shotspeed", "0", &base->mass );
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 4000;
		}

		if ( !base->health )
		{
			base->health = 2000;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 32768;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 500;// + Q_flrand(0.0f, 1.0f) * 500;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 200;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 500;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 10;
		}

		VectorSet( base->s.modelScale, 2.0f, 2.0f, 2.0f );
		VectorSet( base->maxs, 128.0f, 128.0f, 120.0f );
		VectorSet( base->mins, -128.0f, -128.0f, -120.0f );

		// Cull Radius.
		base->s.radius = 256;

		//start in "off" anim
		TurboLaser_SetBoneAnim( base, 4, 5 );

		// Make sure it doesn't do sparks and such when saber contacts with it.
		base->flags = FL_DMG_BY_HEAVY_WEAP_ONLY;
		base->takedamage = qfalse;
		base->contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;

		base->noDamageTeam = TEAM_NEUTRAL;
		base->team = NULL;
	}
	else
	{
		// this is a random time offset for the no-enemy-search-around-mode
		base->count = Q_flrand(0.0f, 1.0f) * 9000;

		if ( !base->health )
		{
			base->health = 100;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 512;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 150 + Q_flrand(0.0f, 1.0f) * 55;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 10;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 25;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 5;
		}

		if ( base->spawnflags & 2 )
		{//upside-down, invert mins and maxe
			VectorSet( base->maxs, 10.0f, 10.0f, 30.0f );
			VectorSet( base->mins, -10.0f, -10.0f, 0.0f );
		}
		else
		{
			VectorSet( base->maxs, 10.0f, 10.0f, 0.0f );
			VectorSet( base->mins, -10.0f, -10.0f, -30.0f );
		}

		base->takedamage = qtrue;
		base->contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;
	}

	// Precache special FX and moving sounds
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		G_EffectIndex( "turret/turb_muzzle_flash" );
		G_EffectIndex( "turret/turb_shot" );
		G_EffectIndex( "turret/turb_impact" );
		//FIXME: Turbo Laser Cannon sounds!
		G_SoundIndex( "sound/vehicles/weapons/turbolaser/turn.wav" );
		G_EffectIndex( "explosions/fighter_explosion2" );
		RegisterItem( FindItemForWeapon( WP_TIE_FIGHTER ));
	}
	else
	{
		// Precache moving sounds
		G_SoundIndex( "sound/chars/turret/startup.wav" );
		G_SoundIndex( "sound/chars/turret/shutdown.wav" );
		G_SoundIndex( "sound/chars/turret/ping.wav" );
		G_SoundIndex( "sound/chars/turret/move.wav" );
	}

	base->max_health = base->health;
	base->e_DieFunc  = dieF_turret_die;

	base->material = MAT_METAL;

	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		RegisterItem( FindItemForWeapon( WP_TURRET ));

		base->svFlags |= SVF_NO_TELEPORT|SVF_SELF_ANIMATING;
	}
	else
	{
		// Register this so that we can use it for the missile effect
		RegisterItem( FindItemForWeapon( WP_BLASTER ));

		base->svFlags |= SVF_NO_TELEPORT|SVF_NONNPC_ENEMY|SVF_SELF_ANIMATING;
	}

	// But set us as a turret so that we can be identified as a turret
	base->s.weapon = WP_TURRET;

	gi.linkentity( base );
}

void SP_misc_ns_turret( gentity_t *base )
//-----------------------------------------------------
{
	const char* original="models/map_objects/nar_shaddar/turret/turret.glm";
	const bool fallback=gi.FS_ReadFile(original,nullptr)<=0;
	const char* model=fallback?"models/map_objects/imp_mine/turret_canon.glm":original;
	base->s.modelindex = G_ModelIndex( model );
	base->s.modelindex2 = G_ModelIndex( "models/map_objects/imp_mine/turret_damage.md3" ); // FIXME!
	base->playerModel = gi.G2API_InitGhoul2Model( base->ghoul2, model, base->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
	if(base->playerModel<0||base->playerModel>=base->ghoul2.size()){G_FreeEntity(base);return;}
	base->s.radius = 80.0f;

	if(!gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "Bone_body", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 )){G_FreeEntity(base);return;}
	base->torsoBolt = gi.G2API_AddBolt( &base->ghoul2[base->playerModel], fallback?"*flash03":"*flash02" );
	if(base->torsoBolt<0){G_FreeEntity(base);return;}

	finish_spawning_turret( base );
	if(fallback){base->s.eFlags|=EF_SHADER_ANIM;base->s.frame=(base->spawnflags&1)?1:0;}
}
