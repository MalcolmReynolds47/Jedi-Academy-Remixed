// Native Raven/OpenJK vehicle projectile emitter, GPL-2.0-only.
// Return allocation success to autonomous turrets before charging their ammunition.
gentity_t *G_FireVehicleProjectile( gentity_t *ent, vec3_t start, vec3_t dir, vehWeaponInfo_t *vehWeapon )
{
	if ( !vehWeapon )
	{//invalid vehicle weapon
		return nullptr;
	}
	else if ( vehWeapon->bIsProjectile )
	{//projectile entity
		gentity_t	*missile;
		vec3_t		mins, maxs;

		VectorSet( maxs, vehWeapon->fWidth/2.0f,vehWeapon->fWidth/2.0f,vehWeapon->fHeight/2.0f );
		VectorScale( maxs, -1, mins );

		//make sure our start point isn't on the other side of a wall
		WP_TraceSetStart( ent, start, mins, maxs );

		//QUERY: alt_fire true or not?  Does it matter?
		missile = CreateMissile( start, dir, vehWeapon->fSpeed, 10000, ent, qfalse );
		if (!missile) return nullptr;
		if ( vehWeapon->bHasGravity )
		{//TESTME: is this all we need to do?
			missile->s.pos.trType = TR_GRAVITY;
		}

		missile->classname = "vehicle_proj";

		missile->damage = vehWeapon->iDamage;
		missile->splashDamage = vehWeapon->iSplashDamage;
		missile->splashRadius = vehWeapon->fSplashRadius;

		// HUGE HORRIBLE HACK
		if (ent->owner && ent->owner->s.number==0)
		{
			//Should only be for speeders - mainly for t2_trip
			if (ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo && ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{
				missile->damage			*= 20.0f;
				missile->splashDamage	*= 20.0f;
				missile->splashRadius	*= 20.0f;
			}
		}

		//FIXME: externalize some of these properties?
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->clipmask = MASK_SHOT;
		//Maybe by checking flags...?
		if ( vehWeapon->bSaberBlockable )
		{
			missile->clipmask |= CONTENTS_LIGHTSABER;
		}
		/*
		if ( (vehWeapon->iFlags&VWF_KNOCKBACK) )
		{
			missile->dflags &= ~DAMAGE_DEATH_KNOCKBACK;
		}
		if ( (vehWeapon->iFlags&VWF_DISTORTION_TRAIL) )
		{
			missile->s.eFlags |= EF_DISTORTION_TRAIL;
		}
		if ( (vehWeapon->iFlags&VWF_RADAR) )
		{
			missile->s.eFlags |= EF_RADAROBJECT;
		}
		*/
		missile->s.weapon = WP_BLASTER;//does this really matter?

		// Make it easier to hit things
		VectorCopy( mins, missile->mins );
		VectorCopy( maxs, missile->maxs );
		//some slightly different stuff for things with bboxes
		if ( vehWeapon->fWidth || vehWeapon->fHeight )
		{//we assume it's a rocket-like thing
			missile->methodOfDeath = MOD_ROCKET;
			missile->splashMethodOfDeath = MOD_ROCKET;// ?SPLASH;

			// we don't want it to ever bounce
			missile->bounceCount = 0;

			missile->mass = 10;
		}
		else
		{//a blaster-laser-like thing
			missile->s.weapon = WP_BLASTER;//does this really matter?
			missile->methodOfDeath = MOD_EMPLACED;//MOD_TURBLAST; //count as a heavy weap
			missile->splashMethodOfDeath = MOD_EMPLACED;//MOD_TURBLAST;// ?SPLASH;
			// we don't want it to bounce forever
			missile->bounceCount = 8;
		}

		if ( vehWeapon->iHealth )
		{//the missile can take damage
			missile->health = vehWeapon->iHealth;
			missile->takedamage = qtrue;
			missile->contents = MASK_SHOT;
			missile->e_DieFunc = dieF_WP_ExplosiveDie;//dieF_RocketDie;
		}

		//set veh as cgame side owner for purpose of fx overrides
		if (ent->m_pVehicle && ent->m_pVehicle->m_pPilot)
		{
			missile->owner = ent->m_pVehicle->m_pPilot;
		}
		else
		{
			missile->owner = ent;
		}
		missile->s.otherEntityNum = ent->s.number;
		missile->s.otherEntityNum2 = (vehWeapon-&g_vehWeaponInfo[0]);

		if ( vehWeapon->iLifeTime )
		{//expire after a time
			if ( vehWeapon->bExplodeOnExpire )
			{//blow up when your lifetime is up
				missile->e_ThinkFunc = thinkF_WP_Explode;//FIXME: custom func?
			}
			else
			{//just remove yourself
				missile->e_ThinkFunc = thinkF_G_FreeEntity;//FIXME: custom func?
			}
			missile->nextthink = level.time + vehWeapon->iLifeTime;
		}
		if ( vehWeapon->fHoming )
		{//homing missile
			//crap, we need to set up the homing stuff like it is in MP...
			WP_RocketLock( ent, 16384 );
			if ( ent->client && ent->client->rocketLockIndex != ENTITYNUM_NONE )
			{
				int dif = 0;
				float rTime;
				rTime = ent->client->rocketLockTime;

				if (rTime == -1)
				{
					rTime = ent->client->rocketLastValidTime;
				}

				if ( !vehWeapon->iLockOnTime )
				{//no minimum lock-on time
					dif = 10;//guaranteed lock-on
				}
				else
				{
					float lockTimeInterval = vehWeapon->iLockOnTime/16.0f;
					dif = ( level.time - rTime ) / lockTimeInterval;
				}

				if (dif < 0)
				{
					dif = 0;
				}

				//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
				if ( dif >= 10 && rTime != -1 )
				{
					missile->enemy = &g_entities[ent->client->rocketLockIndex];

					if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
					{ //if enemy became invalid, died, or is on the same team, then don't seek it
						missile->spawnflags |= 1;//just to let it know it should be faster... FIXME: EXTERNALIZE
						missile->speed = vehWeapon->fSpeed;
						missile->angle = vehWeapon->fHoming;
						if ( vehWeapon->iLifeTime )
						{//expire after a time
							missile->disconnectDebounceTime = level.time + vehWeapon->iLifeTime;
							missile->lockCount = (int)(vehWeapon->bExplodeOnExpire);
						}
						missile->e_ThinkFunc = thinkF_rocketThink;
						missile->nextthink = level.time + VEH_HOMING_MISSILE_THINK_TIME;
						//FIXME: implement radar in SP?
						//missile->s.eFlags |= EF_RADAROBJECT;
					}
				}

				ent->client->rocketLockIndex = ENTITYNUM_NONE;
				ent->client->rocketLockTime = 0;
				ent->client->rocketTargetTime = 0;

				VectorCopy( dir, missile->movedir );
				missile->random = 1.0f;//FIXME: externalize?
			}
		}
        if(G_JAR_ProjectilesEnabled()&&ent->s.number==0&&gi.JAR_Log)gi.JAR_Log(2,"randomizer","vehicle_projectile","name=%s entity=%d model=%d effect=%d think=%d homing=%d target=%d",vehWeapon->name,missile->s.number,vehWeapon->iModel,vehWeapon->iShotFX,int(missile->e_ThinkFunc),missile->e_ThinkFunc==thinkF_rocketThink,missile->enemy?missile->enemy->s.number:ENTITYNUM_NONE);
		return missile;
	}
	else
	{//traceline
		//FIXME: implement
	}
	return nullptr;
}

void WP_FireVehicleWeapon(gentity_t *ent,vec3_t start,vec3_t dir,vehWeaponInfo_t *weapon)
{
	G_FireVehicleProjectile(ent,start,dir,weapon);
}
