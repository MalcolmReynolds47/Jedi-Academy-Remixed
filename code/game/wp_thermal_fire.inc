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

// Shared native firing implementation for runtime and regression tests.
//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float		damageScale = 1.0f;

	VectorCopy( forwardVec, dir );
	VectorCopy( muzzle, start );

	bolt = G_TrySpawn();
	if (!bolt) return NULL;

	bolt->classname = "thermal_detonator";

	if ( ent->s.number != 0 )
	{
		// If not the player, cut the damage a bit so we don't get pounded on so much
		damageScale = TD_NPC_DAMAGE_CUT;
	}

	if ( !alt_fire && ent->s.number == 0 )
	{
		// Main fires for the players do a little bit of extra thinking
		bolt->e_ThinkFunc = thinkF_WP_ThermalThink;
		bolt->nextthink = level.time + TD_THINK_TIME;
		bolt->delay = level.time + TD_TIME; // How long 'til she blows
	}
	else
	{
		bolt->e_ThinkFunc = thinkF_thermalDetonatorExplode;
		bolt->nextthink = level.time + TD_TIME; // How long 'til she blows
	}

	bolt->mass = 10;

	// How 'bout we give this thing a size...
	VectorSet( bolt->mins, -4.0f, -4.0f, -4.0f );
	VectorSet( bolt->maxs, 4.0f, 4.0f, 4.0f );
	bolt->clipmask = MASK_SHOT;
	bolt->clipmask &= ~CONTENTS_CORPSE;
	bolt->contents = CONTENTS_SHOTCLIP;
	bolt->takedamage = qtrue;
	bolt->health = 15;
	bolt->e_DieFunc = dieF_thermal_die;

	WP_TraceSetStart( ent, start, bolt->mins, bolt->maxs );//make sure our start point isn't on the other side of a wall

	float chargeAmount = 1.0f; // default of full charge

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	float	thrownSpeed = TD_VELOCITY;
	const qboolean thisIsAShooter = (qboolean)(!Q_stricmp( "misc_weapon_shooter", ent->classname) || !Q_stricmp("shooter_grenade",ent->classname));

	if (thisIsAShooter)
	{
		if (ent->delay != 0)
		{
			thrownSpeed = ent->delay;
		}
	}

	// normal ones bounce, alt ones explode on impact
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->owner = ent;
	VectorScale( dir, thrownSpeed * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health > 0 )
	{
		bolt->s.pos.trDelta[2] += 120;

		if ( (ent->NPC || (ent->s.number && thisIsAShooter) )
			&& ent->enemy )
		{//NPC or misc_weapon_shooter
			//FIXME: we're assuming he's actually facing this direction...
			vec3_t	target;

			VectorCopy( ent->enemy->currentOrigin, target );
			if ( target[2] <= start[2] )
			{
				vec3_t	vec;
				VectorSubtract( target, start, vec );
				VectorNormalize( vec );
				VectorMA( target, Q_flrand( 0, -32 ), vec, target );//throw a little short
			}

			const float aimError = ent->NPC ? (6-ent->NPC->currentAim)*2.0f : 0.0f;
			target[0] += Q_flrand( -5, 5 )+Q_flrand(-1.0f, 1.0f)*aimError;
			target[1] += Q_flrand( -5, 5 )+Q_flrand(-1.0f, 1.0f)*aimError;
			target[2] += Q_flrand( -5, 5 )+Q_flrand(-1.0f, 1.0f)*aimError;

			WP_LobFire( ent, start, target, bolt->mins, bolt->maxs, bolt->clipmask, bolt->s.pos.trDelta, qtrue, ent->s.number, ent->enemy->s.number );
		}
		else if ( thisIsAShooter && ent->target && !VectorCompare( ent->pos1, vec3_origin ) )
		{//misc_weapon_shooter firing at a position
			WP_LobFire( ent, start, ent->pos1, bolt->mins, bolt->maxs, bolt->clipmask, bolt->s.pos.trDelta, qtrue, ent->s.number, ENTITYNUM_NONE );
		}
	}

	if ( alt_fire )
	{
		bolt->alt_fire = qtrue;
	}
	else
	{
		bolt->s.eFlags |= EF_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );

	bolt->damage = weaponData[WP_THERMAL].damage * damageScale;
	bolt->dflags = 0;
	bolt->splashDamage = weaponData[WP_THERMAL].splashDamage * damageScale;
	bolt->splashRadius = weaponData[WP_THERMAL].splashRadius;

	bolt->s.eType = ET_MISSILE;
	bolt->svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;

	if ( alt_fire )
	{
		bolt->methodOfDeath = MOD_THERMAL_ALT;
		bolt->splashMethodOfDeath = MOD_THERMAL_ALT;//? SPLASH;
	}
	else
	{
		bolt->methodOfDeath = MOD_THERMAL;
		bolt->splashMethodOfDeath = MOD_THERMAL;//? SPLASH;
	}

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );

	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->currentOrigin);

	VectorCopy( start, bolt->pos2 );

	return bolt;
}

