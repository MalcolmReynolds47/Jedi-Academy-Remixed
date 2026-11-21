/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// Native impact implementation, shared with the headless regression fixture.
// Raven Software / OpenJK contributors; GPL-2.0-only, as in g_active.cpp.
#include "g_actor_lifetime.h"

void G_GetMassAndVelocityForEnt( gentity_t *ent, float *mass, vec3_t velocity )
{
	if( ent->client )
	{
		VectorCopy( ent->client->ps.velocity, velocity );
		*mass = ent->mass;
	}
	else
	{
		VectorCopy( ent->s.pos.trDelta, velocity );
		if ( ent->s.pos.trType == TR_GRAVITY )
		{
			velocity[2] -= 0.25f * g_gravity->value;
		}
		if( !ent->mass )
		{
			*mass = 1;
		}
		else if ( ent->mass <= 10 )
		{
			*mass = 10;
		}
		else
		{
			*mass = ent->mass;///10;
		}
	}
}

void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf, trace_t *trace )
{
	if (G_JAR_IsActorStandin(self)) return; // Retain slide collision, never damage or push world objects.
	const G_ActorLifetime mover(*self), obstacle(*other);
	const bool hitArchitecture=other->bmodel && other->material!=MAT_GLASS;
	float magnitude, my_mass;
	bool	thrown = false;
	vec3_t	velocity;

	Vehicle_t *pSelfVeh = NULL;
	Vehicle_t *pOtherVeh = NULL;

	// See if either of these guys are vehicles, if so, keep a pointer to the vehicle npc.
	if ( self->client && self->client->NPC_class == CLASS_VEHICLE &&
		self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo )
	{
		pSelfVeh = self->m_pVehicle;
	}
	if ( other->client && other->client->NPC_class == CLASS_VEHICLE &&
		other->m_pVehicle && other->m_pVehicle->m_pVehicleInfo )
	{
		pOtherVeh = other->m_pVehicle;
	}

	G_GetMassAndVelocityForEnt( self, &my_mass, velocity );

	if ( pSelfVeh )
	{
		magnitude = VectorLength( velocity ) * pSelfVeh->m_pVehicleInfo->mass / 50.0f;
	}
	else
	{
		magnitude = VectorLength( velocity ) * my_mass / 50;
	}

	//if vehicle hit another vehicle, factor in their data, too
	// TODO: Bring this back in later on, it's not critical right now...
/*	if ( self->client && self->client->NPC_class == CLASS_VEHICLE )
	{//we're in a vehicle
		if ( other->client && other->client->ps.vehicleIndex != VEHICLE_NONE )
		{//they're in a vehicle
			float	o_mass;
			vec3_t	o_velocity;

			G_GetMassAndVelocityForEnt( other, &o_mass, o_velocity );

			//now combine
			if ( DotProduct( o_velocity, velocity ) < 0 )
			{//were heading towards each other, this is going to be a STRONG impact...
				vec3_t velocityMod;

				//incorportate mass into each velocity to get directional force
				VectorScale( velocity, my_mass/50, velocityMod );
				VectorScale( o_velocity, o_mass/50, o_velocity );
				//figure out the overall magnitude of those 2 directed forces impacting
				magnitude = (DotProduct( o_velocity, velocityMod ) * -1.0f)/500.0f;
			}
		}
	}*/



	// Check For Vehicle On Vehicle Impact (Ramming)
	//-----------------------------------------------
	if ( pSelfVeh &&
		 pSelfVeh->m_pVehicleInfo->type!=VH_ANIMAL &&
		 pOtherVeh &&
		 pSelfVeh->m_pVehicleInfo==pOtherVeh->m_pVehicleInfo
		)
	{
		gentity_t*	attacker	= self;
		Vehicle_t*	attackerVeh = pSelfVeh;
		gentity_t*	victim		= other;
		Vehicle_t*	victimVeh	= pOtherVeh;

		// Is The Attacker Actually Not Attacking?
		//-----------------------------------------
		if (!(attackerVeh->m_ulFlags&VEH_STRAFERAM))
		{
			// Ok, So Is The Victim Actually Attacking?
			//------------------------------------------
			if (victimVeh->m_ulFlags&VEH_STRAFERAM)
			{
				// Ah, Ok.  Swap Who Is The Attacker Then
				//----------------------------------------
				attacker	= other;
				attackerVeh = pOtherVeh;
				victim		= self;
				victimVeh	= pSelfVeh;
			}
			else
			{
				// No Attackers, So Stop
				//-----------------------
				attacker = victim = 0;
			}
		}

		if (attacker && victim)
		{
		//	float		maxMoveSpeed	= pSelfVeh->m_pVehicleInfo->speedMax;
		//	float		minLockingSpeed = maxMoveSpeed * 0.75;

			vec3_t		attackerMoveDir;

			vec3_t		victimMoveDir;
			vec3_t		victimTowardAttacker;
			vec3_t		victimRight;
			float		victimRightAccuracy;

			VectorCopy(attacker->client->ps.velocity,	attackerMoveDir);
			VectorCopy(victim->client->ps.velocity,		victimMoveDir);

			AngleVectors(victim->currentAngles, 0, victimRight, 0);

			VectorSubtract(victim->currentOrigin, attacker->currentOrigin, victimTowardAttacker);
			/*victimTowardAttackerDistance = */VectorNormalize(victimTowardAttacker);

			victimRightAccuracy = DotProduct(victimTowardAttacker, victimRight);

			if (
				fabsf(victimRightAccuracy)>0.25 						// Must Be Exactly Right Or Left
			//	&& victimTowardAttackerDistance<100.0f 					// Must Be Close Enough
			//	&& attackerMoveSpeed>minLockingSpeed					// Must be moving fast enough
			//	&& fabsf(attackerMoveSpeed - victimMoveSpeed)<100	 	// Both must be going about the same speed
				)
			{
				thrown = true;

				vec3_t	victimRight;
				vec3_t	victimAngles;
				VectorCopy(victim->currentAngles, victimAngles);
				victimAngles[2] = 0;
				AngleVectors(victimAngles, 0, victimRight, 0);

				if (attackerVeh->m_fStrafeTime<0)
				{
					VectorScale(victimRight, -1.0f, victimRight);
				}
				if ( !(victim->flags&FL_NO_KNOCKBACK) )
				{
					G_Throw(victim, victimRight, 250);
				}
//				if (false)
//				{
//					VectorMA(victim->currentOrigin, 250.0f, victimRight, victimRight);
//					CG_DrawEdge(victim->currentOrigin, victimRight, EDGE_IMPACT_POSSIBLE);
//				}
				if (victimVeh->m_pVehicleInfo->iImpactFX)
				{
					G_PlayEffect(victimVeh->m_pVehicleInfo->iImpactFX, victim->currentOrigin, trace->plane.normal );
				}
			}
		}
	}


 	if ( !self->client || self->client->ps.lastOnGround+300<level.time || ( self->client->ps.lastOnGround+100 < level.time ) )
	{
		vec3_t dir1, dir2;
		float force = 0, dot;
		qboolean vehicleHitOwner = qfalse;

		if ( other->material == MAT_GLASS || other->material == MAT_GLASS_METAL || other->material == MAT_GRATE1 || ((other->svFlags&SVF_BBRUSH)&&(other->spawnflags&8/*THIN*/)) )//(other->absmax[0]-other->absmin[0]<=32||other->absmax[1]-other->absmin[1]<=32||other->absmax[2]-other->absmin[2]<=32)) )
		{//glass and thin breakable brushes (axially aligned only, unfortunately) take more impact damage
			magnitude *= 2;
		}

		// See if the vehicle has crashed into the ground.
		if ( pSelfVeh && pSelfVeh->m_pVehicleInfo->type!=VH_ANIMAL)
		{
 			if ((magnitude >= 80) && (self->painDebounceTime < level.time))
			{

				// Setup Some Variables
				//----------------------
				vec3_t	vehFwd;
				VectorCopy(velocity, vehFwd);
				float	vehSpeed					= VectorNormalize(vehFwd);
				float	vehToughnessAgainstOther	= pSelfVeh->m_pVehicleInfo->toughness;
				float	vehHitPercent				= fabsf(DotProduct(vehFwd, trace->plane.normal));
				int		vehDFlags					= DAMAGE_NO_ARMOR;
				bool	vehPilotedByPlayer			= (pSelfVeh->m_pPilot && pSelfVeh->m_pPilot->s.number<MAX_CLIENTS);
				bool	vehInTurbo					= (pSelfVeh->m_iTurboTime>level.time);

				self->painDebounceTime				= level.time + 200;


				// Modify Magnitude By Hit Percent And Toughness Against Other
				//-------------------------------------------------------------
				if (pSelfVeh->m_ulFlags & VEH_OUTOFCONTROL)
				{
					vehToughnessAgainstOther *= 0.01f;		// If Out Of Control, No Damage Resistance
				}
				else
				{
 					if (vehPilotedByPlayer)
					{
						vehToughnessAgainstOther *= 1.5f;
					}
					if (other && other->client)
					{
						vehToughnessAgainstOther *= 15.0f;	// Very Tough against other clients (NPCS, Player, etc)
					}
				}
				if (vehToughnessAgainstOther>0.0f)
				{
					magnitude *= (vehHitPercent / vehToughnessAgainstOther);
				}
				else
				{
					magnitude *= vehHitPercent;
				}


				// If We Hit Architecture
				//------------------------
				if (!other || !other->client)
				{
					// Turbo Hurts
					//-------------
					if (vehInTurbo)
					{
						magnitude *= 5.0f;
					}

					else if (trace->plane.normal[2]>0.75f && vehHitPercent<0.2f)
					{
						magnitude /= 10.0f;
					}


					// If No Pilot, Blow This Thing Now
					//----------------------------------
					if (vehHitPercent>0.9f && !pSelfVeh->m_pPilot && vehSpeed>1000.0f)
					{
						vehDFlags |= DAMAGE_IMPACT_DIE;
					}

					// If Out Of Control, And We Hit A Wall Or Landed Or Head On
					//------------------------------------------------------------
					if ((pSelfVeh->m_ulFlags&VEH_OUTOFCONTROL) && (vehHitPercent>0.5f || trace->plane.normal[2]<0.5f || velocity[2]<-50.0f))
					{
						vehDFlags |= DAMAGE_IMPACT_DIE;
					}

					// If This Is A Direct Impact (Debounced By 4 Seconds)
					//-----------------------------------------------------
					if (vehHitPercent>0.9f && (level.time - self->lastImpact)>2000 && vehSpeed>300.0f)
					{
						self->lastImpact = level.time;

						// The Player Has Harder Requirements to Explode
						//-----------------------------------------------
						if (vehPilotedByPlayer)
						{
							if ((vehHitPercent>0.99f && vehSpeed>1000.0f && !Q_irand(0,30)) ||
								(vehHitPercent>0.999f && vehInTurbo))
							{
								vehDFlags |= DAMAGE_IMPACT_DIE;
							}
						}
						else if (player && G_IsRidingVehicle(player) &&
							(Distance(self->currentOrigin, player->currentOrigin)<800.0f) &&
							(vehInTurbo || !Q_irand(0,1) || vehHitPercent>0.999f))
						{
							vehDFlags |= DAMAGE_IMPACT_DIE;
						}
					}

					// Make Sure He Dies This Time.  I will accept no excuses.
					//---------------------------------------------------------
					if (vehDFlags&DAMAGE_IMPACT_DIE)
					{
						// If close enough To The PLayer
 						if (player &&
							G_IsRidingVehicle(player) &&
							self->owner &&
							Distance(self->currentOrigin, player->currentOrigin)<500.0f)
						{
				 			player->lastEnemy = self->owner;
							G_StartMatrixEffect(player, MEF_LOOK_AT_ENEMY|MEF_NO_RANGEVAR|MEF_NO_VERTBOB|MEF_NO_SPIN, 1000);
						}
						magnitude = 100000.0f;
					}
				}

				if (magnitude>10.0f)
				{
					// Play The Impact Effect
					//------------------------
					if (pSelfVeh->m_pVehicleInfo->iImpactFX && vehSpeed>100.0f)
					{
						G_PlayEffect( pSelfVeh->m_pVehicleInfo->iImpactFX, self->currentOrigin, trace->plane.normal );
					}

					// Set The Crashing Flag And Pain Debounce Time
					//----------------------------------------------
					pSelfVeh->m_ulFlags		|= VEH_CRASHING;
				}

				const bool fatalImpact = (vehDFlags & DAMAGE_IMPACT_DIE) != 0;
				const int entityNum = self->s.number;
				if (fatalImpact && gi.JAR_Log)
					gi.JAR_Log(1, "vehicle", "fatal_impact_begin", "entity=%d npc=%s hit=%d speed=%.1f",
						entityNum, self->NPC_type ? self->NPC_type : "", other->s.number, vehSpeed);
				G_Damage( self, player, player, NULL, self->currentOrigin, magnitude, vehDFlags, MOD_FALLING );//FIXME: MOD_IMPACT
				if (fatalImpact && gi.JAR_Log)
					gi.JAR_Log(1, "vehicle", "fatal_impact_end", "entity=%d in_use=%d", entityNum, self->inuse);
				if (!mover.Current() || (other->s.number!=ENTITYNUM_WORLD && !obstacle.Current()))
					return;
			}

			if ( self->owner == other || self->activator == other )
			{//hit owner/activator
				if ( self->m_pVehicle && !self->m_pVehicle->m_pVehicleInfo->Inhabited( self->m_pVehicle ) )
				{//empty swoop
					if ( self->client->respawnTime - level.time < 1000 )
					{//just spawned in a second ago
						//don't actually damage or throw him...
						vehicleHitOwner = qtrue;
					}
				}
			}
			//if 2 vehicles on same side hit each other, tone it down
			//NOTE: we do this here because we still want the impact effect
			if ( pOtherVeh )
			{
				if ( self->client->playerTeam == other->client->playerTeam )
				{
					magnitude /= 25;
				}
			}
		}
		else if ( self->client
			&& (PM_InKnockDown( &self->client->ps )||(self->client->ps.eFlags&EF_FORCE_GRIPPED))
			&& magnitude >= 120 )
		{//FORCE-SMACKED into something
			if ( TIMER_Done( self, "impactEffect" ) )
			{
				G_PlayEffect( G_EffectIndex( "env/impact_dustonly" ), trace->endpos, trace->plane.normal );
				G_Sound( self, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );
				TIMER_Set( self, "impactEffect", 1000 );
			}
		}

		//damage them
		if ( magnitude >= 100 && other->s.number < ENTITYNUM_WORLD )
		{
			VectorCopy( velocity, dir1 );
			VectorNormalize( dir1 );
			if( VectorCompare( other->currentOrigin, vec3_origin ) )
			{//a brush with no origin
				VectorCopy ( dir1, dir2 );
			}
			else
			{
				VectorSubtract( other->currentOrigin, self->currentOrigin, dir2 );
				VectorNormalize( dir2 );
			}

			dot = DotProduct( dir1, dir2 );

			if ( dot >= 0.2 )
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= (magnitude/50);

			int cont = gi.pointcontents( other->absmax, other->s.number );
			if( (cont&CONTENTS_WATER) )
			{//water absorbs 2/3 velocity
				force *= 0.33333f;
			}

			if ( self->NPC && other->s.number == ENTITYNUM_WORLD )
			{//NPCs take less damage
				force *= 0.5f;
			}

			if ( self->s.number >= MAX_CLIENTS && self->client && (PM_InKnockDown( &self->client->ps )||self->client->ps.eFlags&EF_FORCE_GRIPPED) )
			{//NPC: I was knocked down or being gripped, impact should be harder
				//FIXME: what if I was just thrown - force pushed/pulled or thrown from a grip?
				force *= 10;
			}

			//FIXME: certain NPCs/entities should be TOUGH - like Ion Cannons, AT-STs, Mark1 droids, etc.
			if ( pOtherVeh )
			{//if hit another vehicle, take their toughness into account, too
				force /= pOtherVeh->m_pVehicleInfo->toughness;
			}

			if( ( (force >= 1 || pSelfVeh) && other->s.number>=MAX_CLIENTS ) || force >= 10)
			{
	/*
				dprint("Damage other (");
				dprint(loser.classname);
				dprint("): ");
				dprint(ftos(force));
				dprint("\n");
	*/
				if ( other->svFlags & SVF_GLASS_BRUSH )
				{
					other->splashRadius = (float)(self->maxs[0] - self->mins[0])/4.0f;
				}

				if ( pSelfVeh )
				{//if in a vehicle when land on someone, always knockdown, throw, damage
					if ( !vehicleHitOwner )
					{//didn't hit owner
						// If the player was hit don't make the damage so bad...
						if ( other && other->s.number<MAX_CLIENTS )
						{
							force *= 0.5f;
						}
						//Hmm, maybe knockdown?
						if ( !(other->flags&FL_NO_KNOCKBACK) )
						{
							G_Throw( other, dir2, force );
						}
						G_Knockdown( other, self, dir2, force, qtrue );
						G_Damage( other, self, self, velocity, self->currentOrigin, force, DAMAGE_NO_ARMOR|DAMAGE_EXTRA_KNOCKBACK, MOD_IMPACT );
						if (!mover.Current()) return;
					}
				}
				else if ( self->forcePushTime > level.time - 1000//was force pushed/pulled in the last 1600 milliseconds
					&& self->forcePuller == other->s.number )//hit the person who pushed/pulled me
				{//ignore the impact
				}
				else if ( other->takedamage )
				{
					if ( !self->client || other->s.number<MAX_CLIENTS || !other->client )
					{//aw, fuck it, clients no longer take impact damage from other clients, unless you're the player
						if ( other->client //he's a client
							&& self->client //I'm a client
							&& other->client->ps.forceGripEntityNum == self->s.number )//he's force-gripping me
						{//don't damage the other guy if he's gripping me
						}
						else
						{
							G_Damage( other, self, self, velocity, self->currentOrigin, floor(force), DAMAGE_NO_ARMOR, MOD_IMPACT );
						}
					}
					else
					{
						GEntity_PainFunc( other, self, self, self->currentOrigin, force, MOD_IMPACT );
						if (!mover.Current()) return;
						//Hmm, maybe knockdown?
						if (!thrown && obstacle.Current())
						{
							if ( !(other->flags&FL_NO_KNOCKBACK) )
							{
								G_Throw( other, dir2, force );
							}
						}
					}
					if (!mover.Current()) return;
					if ( obstacle.Current() && other->health > 0 )
					{//still alive?
						//TODO: if someone was thrown through the air (in a knockdown or being gripped)
						//		and they hit me hard enough, knock me down
						if ( other->client )
						{
							if ( self->client )
							{
                                if ( PM_InKnockDown( &self->client->ps ) || (self->client->ps.eFlags&EF_FORCE_GRIPPED) )
								{
									G_Knockdown( other, self, dir2, Q_irand( 200, 400 ), qtrue );
								}
							}
							else if ( self->forcePuller != ENTITYNUM_NONE
								&& g_entities[self->forcePuller].client
								&& self->mass > Q_irand( 50, 100 ) )
							{
								G_Knockdown( other, &g_entities[self->forcePuller], dir2, Q_irand( 200, 400 ), qtrue );
							}
						}
					}
				}
				else
				{
					//Hmm, maybe knockdown?
					if (!thrown)
					{
						if ( !(other->flags&FL_NO_KNOCKBACK) )
						{
							G_Throw( other, dir2, force );
						}
					}
				}
			}
		}

		if ( damageSelf && self->takedamage && !(self->flags&FL_NO_IMPACT_DMG))
		{
			//Now damage me
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if ( pSelfVeh && self->client->ps.forceJumpZStart )
			{//we were force-jumping
				if ( self->currentOrigin[2] >= self->client->ps.forceJumpZStart )
				{//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.forceJumpZStart-self->currentOrigin[2])/3;
				}
			}

			if( ( magnitude >= 100 + self->health
					&& self->s.number >= MAX_CLIENTS
					&& self->s.weapon != WP_SABER )
				|| ( self->client && self->client->NPC_class == CLASS_VEHICLE )
				|| ( magnitude >= 700 ) )//health here is used to simulate structural integrity
			{
				if ( (self->s.weapon == WP_SABER || self->s.number<MAX_CLIENTS || (self->client&&(self->client->NPC_class==CLASS_BOBAFETT||self->client->NPC_class==CLASS_ROCKETTROOPER))) && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE && magnitude < 1000 )
				{//players and jedi take less impact damage
					//allow for some lenience on high falls
					magnitude /= 2;
				}
				//drop it some (magic number... sigh)
				magnitude /= 40;
				//If damage other, subtract half of that damage off of own injury
				if ( hitArchitecture )
				{//take off only a little because we broke architecture (not including glass), that should hurt
					magnitude = magnitude - force/8;
				}
				else
				{//take off half damage we did to it
					magnitude = magnitude - force/2;
				}

				if ( pSelfVeh )
				{
					//FIXME: if hit another vehicle, take their toughness into
					//			account, too?  Or should their toughness only matter
					//			when they hit me?
					magnitude /= pSelfVeh->m_pVehicleInfo->toughness * 1000.0f;
					if ( hitArchitecture )
					{//broke through some architecture, take a good amount of damage
					}
					else if ( pOtherVeh )
					{//they're tougher
						//magnitude /= 4.0f;//FIXME: get the toughness of other from vehicles.cfg
					}
					else
					{//take some off because of give
						//NOTE: this covers all other entities and impact with world...
						//FIXME: certain NPCs/entities should be TOUGH - like Ion Cannons, AT-STs, Mark1 droids, etc.
						magnitude /= 10.0f;
					}
					if ( magnitude < 1.0f )
					{
						magnitude = 0;
					}
				}
				if ( magnitude >= 1 )
				{
	//FIXME: Put in a thingtype impact sound function
	/*
					dprint("Damage self (");
					dprint(self.classname);
					dprint("): ");
					dprint(ftos(magnitude));
					dprint("\n");
	*/
					if ( self->NPC && self->s.weapon == WP_SABER )
					{//FIXME: for now Jedi take no falling damage, but really they should if pushed off?
						magnitude = 0;
					}

					G_Damage( self, NULL, NULL, NULL, self->currentOrigin, magnitude/2, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
				}
			}
		}

		//FIXME: slow my velocity some?



		/*
		if(self.flags&FL_ONGROUND)
			self.last_onground=time;
		*/
	}
}

