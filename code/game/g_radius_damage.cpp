/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "g_local.h"
#include "g_vehicles.h"
extern Vehicle_t *G_IsRidingVehicle(gentity_t *ent);

extern	void	Boba_DustFallNear(const vec3_t origin, int dustcount);
extern	void	G_GetMassAndVelocityForEnt( gentity_t *ent, float *mass, vec3_t velocity );
/*
============
G_RadiusDamage
============
*/
void G_RadiusDamage ( const vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod) {
	float		points, dist;
	gentity_t	*ent;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	int			dFlags = DAMAGE_RADIUS;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	if (mod==MOD_ROCKET)
	{
		Boba_DustFallNear(origin, 10);
	}

	if ( mod == MOD_GAS )
	{
		dFlags |= DAMAGE_NO_KNOCKBACK;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = entityList[ e ];

		if ( ent == ignore )
			continue;
		if ( !ent->takedamage )
			continue;
		if ( !ent->contents )
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->absmin[i] ) {
				v[i] = ent->absmin[i] - origin[i];
			} else if ( origin[i] > ent->absmax[i] ) {
				v[i] = origin[i] - ent->absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		// Lessen damage to vehicles that are moving away from the explosion
		if (ent->client && (ent->client->NPC_class==CLASS_VEHICLE || G_IsRidingVehicle(ent)))
		{
			gentity_t*	bike = ent;

			if (G_IsRidingVehicle(ent) && ent->owner)
			{
				bike = ent->owner;
			}

			vec3_t	vehMoveDirection;
			float	vehMoveSpeed;

			vec3_t	explosionDirection;
			float	explosionDirectionSimilarity;

			float	mass;
			G_GetMassAndVelocityForEnt( bike, &mass, vehMoveDirection );
			vehMoveSpeed		= VectorNormalize(vehMoveDirection);
			if (vehMoveSpeed>300.0f)
			{
				VectorSubtract(bike->currentOrigin, origin, explosionDirection);
				VectorNormalize(explosionDirection);

				explosionDirectionSimilarity = DotProduct(vehMoveDirection, explosionDirection);
				if (explosionDirectionSimilarity>0.0f)
				{
					points *= (1.0f - explosionDirectionSimilarity);
				}
			}
		}

		if (CanDamage (ent, origin))
		{//FIXME: still do a little damage in in PVS and close?
			if ( ent->svFlags & (SVF_GLASS_BRUSH|SVF_BBRUSH) )
			{
				VectorAdd( ent->absmin, ent->absmax, v );
				VectorScale( v, 0.5f, v );
			}
			else
			{
				VectorCopy( ent->currentOrigin, v );
			}

			VectorSubtract( v, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;

			if ( ent->svFlags & SVF_GLASS_BRUSH )
			{
				if ( points > 1.0f )
				{
					// we want to cap this at some point, otherwise it just gets crazy
					if ( points > 6.0f )
					{
						VectorScale( dir, 6.0f, dir );
					}
					else
					{
						VectorScale( dir, points, dir );
					}
				}

				ent->splashRadius = radius;// * ( 1.0 - dist / radius );
			}

			G_Damage (ent, NULL, attacker, dir, origin, (int)points, dFlags, mod);
		}
	}
}
