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
#include "jar_mp_number.h"
#include "bg_saga.h"

int gTrigFallSound;

#include "g_jar_native_triggers.inc"

/*
======================================================================
trigger_lightningstrike -rww
======================================================================
*/
//lightning strike trigger lightning strike event
void Do_Strike(gentity_t *ent)
{
	trace_t localTrace;
	vec3_t strikeFrom;
	vec3_t strikePoint;
	vec3_t fxAng;

	//maybe allow custom fx direction at some point?
	VectorSet(fxAng, 90.0f, 0.0f, 0.0f);

	//choose a random point to strike within the bounds of the trigger
	strikePoint[0] = flrand(ent->r.absmin[0], ent->r.absmax[0]);
	strikePoint[1] = flrand(ent->r.absmin[1], ent->r.absmax[1]);

	//consider the bottom mins the ground level
	strikePoint[2] = ent->r.absmin[2];

	//set the from point
	strikeFrom[0] = strikePoint[0];
	strikeFrom[1] = strikePoint[1];
	strikeFrom[2] = ent->r.absmax[2]-4.0f;

	//now trace for damaging stuff, and do the effect
	trap->Trace(&localTrace, strikeFrom, NULL, NULL, strikePoint, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
	VectorCopy(localTrace.endpos, strikePoint);

	if (localTrace.startsolid || localTrace.allsolid)
	{ //got a bad spot, think again next frame to try another strike
		ent->nextthink = level.time;
		return;
	}

	if (ent->radius)
	{ //do a radius damage at the end pos
		G_RadiusDamage(strikePoint, ent, ent->damage, ent->radius, ent, NULL, MOD_SUICIDE);
	}
	else
	{ //only damage individuals
		gentity_t *trHit = &g_entities[localTrace.entityNum];

		if (trHit->inuse && trHit->takedamage)
		{ //damage it then
			G_Damage(trHit, ent, ent, NULL, trHit->r.currentOrigin, ent->damage, 0, MOD_SUICIDE);
		}
	}

	G_PlayEffectID(ent->genericValue2, strikeFrom, fxAng);
}

//lightning strike trigger think loop
void Think_Strike(gentity_t *ent)
{
	if (ent->genericValue1)
	{ //turned off currently
		return;
	}

	ent->nextthink = level.time + ent->wait + Q_irand(0, ent->random);
	Do_Strike(ent);
}

//lightning strike trigger use event function
void Use_Strike( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	ent->genericValue1 = !ent->genericValue1;

	if (!ent->genericValue1)
	{ //turn it back on
		ent->nextthink = level.time;
	}
}

/*QUAKED trigger_lightningstrike (.1 .5 .1) ? START_OFF
START_OFF - start trigger disabled

"lightningfx"	effect to use for lightning, MUST be specified
"wait"			Seconds between strikes, 1000 default
"random"		wait variance, default is 2000
"dmg"			damage on strike (default 50)
"radius"		if non-0, does a radius damage at the lightning strike
				impact point (using this value as the radius). otherwise
				will only do line trace damage. default 0.

use to toggle on and off
*/
void SP_trigger_lightningstrike( gentity_t *ent )
{
	char *s;

	ent->use = Use_Strike;
	ent->think = Think_Strike;
	ent->nextthink = level.time + 500;

	G_SpawnString("lightningfx", "", &s);
	if (!s || !s[0])
	{
		Com_Error(ERR_DROP, "trigger_lightningstrike with no lightningfx");
	}

	//get a configstring index for it
	ent->genericValue2 = G_EffectIndex(s);

	if (ent->spawnflags & 1)
	{ //START_OFF
		ent->genericValue1 = 1;
	}

	if (!ent->wait)
	{ //default 1000
		ent->wait = 1000;
	}
	if (!ent->random)
	{ //default 2000
		ent->random = 2000;
	}
	if (!ent->damage)
	{ //default 50
		ent->damage = 50;
	}

	InitTrigger( ent );
	trap->LinkEntity ((sharedEntity_t *)ent);
}


/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think( gentity_t *ent ) {
	G_UseTargets(ent, ent);
	G_FreeEntity( ent );
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = trigger_always_think;
}


/*
==============================================================================

trigger_push

==============================================================================
*/
#include "g_jar_native_push_trigger.inc"

// Native use is shared with Sandbox transport qualification.
extern void Use_target_push(gentity_t*,gentity_t*,gentity_t*);

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad CONSTANT
CONSTANT will push activator in direction of 'target' at constant 'speed'

Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of none
*/
void SP_target_push( gentity_t *self ) {
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		self->noise_index = G_SoundIndex("sound/weapons/force/jump.wav");
	} else {
		self->noise_index = 0;	//G_SoundIndex("sound/misc/windfly.wav");
	}
	if ( self->target ) {
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

#include "g_jar_native_teleport_trigger.inc"


/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF CAN_TARGET SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

CAN_TARGET		if you target it, it will toggle on and off
SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"team"			team (1 or 2) to allow hurting (if none then hurt anyone) only applicable for siege
"dmg"			default 5 (whole numbers only)
If dmg is set to -1 this brush will use the fade-kill method

*/
#include "g_jar_native_hurt.inc"

#define	INITIAL_SUFFOCATION_DELAY	500 //.5 seconds
void space_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if (!other || !other->inuse || !other->client )
		//NOTE: we need vehicles to know this, too...
		//|| other->s.number >= MAX_CLIENTS)
	{
		return;
	}

	if ( other->s.number < MAX_CLIENTS//player
		&& other->client->ps.m_iVehicleNum//in a vehicle
		&& other->client->ps.m_iVehicleNum >= MAX_CLIENTS )
	{//a player client inside a vehicle
		gentity_t *veh = &g_entities[other->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->hideRider)
		{ //if they are "inside" a vehicle, then let that protect them from THE HORRORS OF SPACE.
			other->client->inSpaceSuffocation = 0;
			other->client->inSpaceIndex = ENTITYNUM_NONE;
			return;
		}
	}

	if (!G_PointInBounds(other->client->ps.origin, self->r.absmin, self->r.absmax))
	{ //his origin must be inside the trigger
		return;
	}

	if (!other->client->inSpaceIndex ||
		other->client->inSpaceIndex == ENTITYNUM_NONE)
	{ //freshly entering space
		other->client->inSpaceSuffocation = level.time + INITIAL_SUFFOCATION_DELAY;
	}

	other->client->inSpaceIndex = self->s.number;
}

/*QUAKED trigger_space (.5 .5 .5) ?
causes human clients to suffocate and have no gravity.

*/
void SP_trigger_space(gentity_t *self)
{
	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;

	self->touch = space_touch;

    trap->LinkEntity((sharedEntity_t *)self);
}

void shipboundary_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *ent;

	if (!other || !other->inuse || !other->client ||
		other->s.number < MAX_CLIENTS ||
		!other->m_pVehicle)
	{ //only let vehicles touch
		return;
	}

	if ( other->client->ps.hyperSpaceTime && level.time - other->client->ps.hyperSpaceTime < HYPERSPACE_TIME )
	{//don't interfere with hyperspacing ships
		return;
	}

	ent = G_Find (NULL, FOFS(targetname), self->target);
	if (!ent || !ent->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_shipboundary has invalid target '%s'\n", self->target );
		return;
	}

	if (!other->client->ps.m_iVehicleNum || other->m_pVehicle->m_iRemovedSurfaces)
	{ //if a vehicle touches a boundary without a pilot in it or with parts missing, just blow the thing up
		G_Damage(other, other, other, NULL, other->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		return;
	}

	//make sure this sucker is linked so the prediction knows where to go
	trap->LinkEntity((sharedEntity_t *)ent);

	other->client->ps.vehTurnaroundIndex = ent->s.number;
	other->client->ps.vehTurnaroundTime = level.time + (self->genericValue1*2);

	//keep up the detailed checks for another 2 seconds
	self->genericValue7 = level.time + 2000;
}

void shipboundary_think(gentity_t *ent)
{
	int			iEntityList[MAX_GENTITIES];
	int			numListedEntities;
	int			i = 0;
	gentity_t	*listedEnt;

	ent->nextthink = level.time + 100;

	if (ent->genericValue7 < level.time)
	{ //don't need to be doing this check, no one has touched recently
		return;
	}

	numListedEntities = trap->EntitiesInBox( ent->r.absmin, ent->r.absmax, iEntityList, MAX_GENTITIES );
	while (i < numListedEntities)
	{
		listedEnt = &g_entities[iEntityList[i]];
		if (listedEnt->inuse && listedEnt->client && listedEnt->client->ps.m_iVehicleNum)
		{
            if (listedEnt->s.eType == ET_NPC &&
				listedEnt->s.NPC_class == CLASS_VEHICLE)
			{
				Vehicle_t *pVeh = listedEnt->m_pVehicle;
				if (pVeh && pVeh->m_pVehicleInfo->type == VH_FIGHTER)
				{
                    shipboundary_touch(ent, listedEnt, NULL);
				}
			}
		}
		i++;
	}
}

/*QUAKED trigger_shipboundary (.5 .5 .5) ?
causes vehicle to turn toward target and travel in that direction for a set time when hit.

"target"		name of entity to turn toward (can be info_notnull, or whatever).
"traveltime"	time to travel in this direction

*/
void SP_trigger_shipboundary(gentity_t *self)
{
	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;

	if (!self->target || !self->target[0])
	{
		trap->Error( ERR_DROP, "trigger_shipboundary without a target." );
	}
	G_SpawnInt("traveltime", "0", &self->genericValue1);

	if (!self->genericValue1)
	{
		trap->Error( ERR_DROP, "trigger_shipboundary without traveltime." );
	}

	self->think = shipboundary_think;
	self->nextthink = level.time + 500;
	self->touch = shipboundary_touch;

    trap->LinkEntity((sharedEntity_t *)self);
}

void hyperspace_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *ent;

	if (!other || !other->inuse || !other->client ||
		other->s.number < MAX_CLIENTS ||
		!other->m_pVehicle)
	{ //only let vehicles touch
		return;
	}

	if ( other->client->ps.hyperSpaceTime && level.time - other->client->ps.hyperSpaceTime < HYPERSPACE_TIME )
	{//already hyperspacing, just keep us moving
		if ( (other->client->ps.eFlags2&EF2_HYPERSPACE) )
		{//they've started the hyperspace but haven't been teleported yet
			float timeFrac = ((float)(level.time-other->client->ps.hyperSpaceTime))/HYPERSPACE_TIME;
			if ( timeFrac >= HYPERSPACE_TELEPORT_FRAC )
			{//half-way, now teleport them!
				vec3_t	diff, fwd, right, up, newOrg;
				float	fDiff, rDiff, uDiff;
				//take off the flag so we only do this once
				other->client->ps.eFlags2 &= ~EF2_HYPERSPACE;
				//Get the offset from the local position
				ent = G_Find (NULL, FOFS(targetname), self->target);
				if (!ent || !ent->inuse)
				{ //this is bad
					trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
					return;
				}
				VectorSubtract( other->client->ps.origin, ent->s.origin, diff );
				AngleVectors( ent->s.angles, fwd, right, up );
				fDiff = DotProduct( fwd, diff );
				rDiff = DotProduct( right, diff );
				uDiff = DotProduct( up, diff );
				//Now get the base position of the destination
				ent = G_Find (NULL, FOFS(targetname), self->target2);
				if (!ent || !ent->inuse)
				{ //this is bad
					trap->Error( ERR_DROP, "trigger_hyperspace has invalid target2 '%s'\n", self->target2 );
					return;
				}
				VectorCopy( ent->s.origin, newOrg );
				//finally, add the offset into the new origin
				AngleVectors( ent->s.angles, fwd, right, up );
				VectorMA( newOrg, fDiff, fwd, newOrg );
				VectorMA( newOrg, rDiff, right, newOrg );
				VectorMA( newOrg, uDiff, up, newOrg );
				//trap->Print("hyperspace from %s to %s\n", vtos(other->client->ps.origin), vtos(newOrg) );
				//now put them in the offset position, facing the angles that position wants them to be facing
				TeleportPlayer( other, newOrg, ent->s.angles );
				if ( other->m_pVehicle && other->m_pVehicle->m_pPilot )
				{//teleport the pilot, too
					TeleportPlayer( (gentity_t*)other->m_pVehicle->m_pPilot, newOrg, ent->s.angles );
					//FIXME: and the passengers?
				}
				//make them face the new angle
				//other->client->ps.hyperSpaceIndex = ent->s.number;
				VectorCopy( ent->s.angles, other->client->ps.hyperSpaceAngles );
				//sound
				G_Sound( other, CHAN_LOCAL, G_SoundIndex( "sound/vehicles/common/hyperend.wav" ) );
			}
		}
		return;
	}
	else
	{
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent || !ent->inuse)
		{ //this is bad
			trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
			return;
		}

		if (!other->client->ps.m_iVehicleNum || other->m_pVehicle->m_iRemovedSurfaces)
		{ //if a vehicle touches a boundary without a pilot in it or with parts missing, just blow the thing up
			G_Damage(other, other, other, NULL, other->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
			return;
		}
		//other->client->ps.hyperSpaceIndex = ent->s.number;
		VectorCopy( ent->s.angles, other->client->ps.hyperSpaceAngles );
		other->client->ps.hyperSpaceTime = level.time;
	}
}

/*
void trigger_hyperspace_find_targets( gentity_t *self )
{
	gentity_t *targEnt = NULL;
	targEnt = G_Find (NULL, FOFS(targetname), self->target);
	if (!targEnt || !targEnt->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
		return;
	}
	targEnt->r.svFlags |= SVF_BROADCAST;//crap, need to tell the cgame about the target_position
	targEnt = G_Find (NULL, FOFS(targetname), self->target2);
	if (!targEnt || !targEnt->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_hyperspace has invalid target2 '%s'\n", self->target2 );
		return;
	}
	targEnt->r.svFlags |= SVF_BROADCAST;//crap, need to tell the cgame about the target_position
}
*/
/*QUAKED trigger_hyperspace (.5 .5 .5) ?
Ship will turn to face the angles of the first target_position then fly forward, playing the hyperspace effect, then pop out at a relative point around the target

"target"		whatever position the ship teleports from in relation to the target_position specified here, that's the relative position the ship will spawn at around the target2 target_position
"target2"		name of target_position to teleport the ship to (will be relative to it's origin)
*/
void SP_trigger_hyperspace(gentity_t *self)
{
	//register the hyperspace end sound (start sounds are customized)
	G_SoundIndex( "sound/vehicles/common/hyperend.wav" );

	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;

	if (!self->target || !self->target[0])
	{
		trap->Error( ERR_DROP, "trigger_hyperspace without a target." );
	}
	if (!self->target2 || !self->target2[0])
	{
		trap->Error( ERR_DROP, "trigger_hyperspace without a target2." );
	}

	self->delay = Distance( self->r.absmax, self->r.absmin );//my size

	self->touch = hyperspace_touch;

    trap->LinkEntity((sharedEntity_t *)self);

	//self->think = trigger_hyperspace_find_targets;
	//self->nextthink = level.time + FRAMETIME;
}
/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
#include "g_jar_native_timer.inc"

gentity_t *asteroid_pick_random_asteroid( gentity_t *self )
{
	int			t_count = 0, pick;
	gentity_t	*t = NULL;

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return NULL;
	}

	if(t_count == 1)
	{
		return t;
	}

	//FIXME: need a seed
	pick = Q_irand(1, t_count);
	t_count = 0;
	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
		else
		{
			continue;
		}

		if(t_count == pick)
		{
			return t;
		}
	}
	return NULL;
}

int asteroid_count_num_asteroids( gentity_t *self )
{
	int	i, count = 0;

	for ( i = MAX_CLIENTS; i < ENTITYNUM_WORLD; i++ )
	{
		if ( !g_entities[i].inuse )
		{
			continue;
		}
		if ( g_entities[i].r.ownerNum == self->s.number )
		{
			count++;
		}
	}
	return count;
}

extern void SP_func_rotating (gentity_t *ent);
extern void Q3_Lerp2Origin( int taskID, int entID, vec3_t origin, float duration );
void asteroid_field_think(gentity_t *self)
{
	int numAsteroids = asteroid_count_num_asteroids( self );

	self->nextthink = level.time + 500;

	if ( numAsteroids < self->count )
	{
		//need to spawn a new asteroid
		gentity_t *newAsteroid = G_Spawn();
		if ( newAsteroid )
		{
			vec3_t startSpot, endSpot, startAngles;
			float dist, speed = flrand( self->speed * 0.25f, self->speed * 2.0f );
			int	capAxis, axis, time = 0;
			gentity_t *copyAsteroid = asteroid_pick_random_asteroid( self );
			if ( copyAsteroid )
			{
				newAsteroid->model = copyAsteroid->model;
				newAsteroid->model2 = copyAsteroid->model2;
				newAsteroid->health = copyAsteroid->health;
				newAsteroid->spawnflags = copyAsteroid->spawnflags;
				newAsteroid->mass = copyAsteroid->mass;
				newAsteroid->damage = copyAsteroid->damage;
				newAsteroid->speed = copyAsteroid->speed;

				G_SetOrigin( newAsteroid, copyAsteroid->s.origin );
				G_SetAngles( newAsteroid, copyAsteroid->s.angles );
				newAsteroid->classname = "func_rotating";

				SP_func_rotating( newAsteroid );

				newAsteroid->genericValue15 = copyAsteroid->genericValue15;
				newAsteroid->s.iModelScale = copyAsteroid->s.iModelScale;
				newAsteroid->maxHealth = newAsteroid->health;
				G_ScaleNetHealth(newAsteroid);
				newAsteroid->radius = copyAsteroid->radius;
				newAsteroid->material = copyAsteroid->material;
				//CacheChunkEffects( self->material );

				//keep track of it
				newAsteroid->r.ownerNum = self->s.number;

				//move it
				capAxis = Q_irand( 0, 2 );
				for ( axis = 0; axis < 3; axis++ )
				{
					if ( axis == capAxis )
					{
						if ( Q_irand( 0, 1 ) )
						{
							startSpot[axis] = self->r.mins[axis];
							endSpot[axis] = self->r.maxs[axis];
						}
						else
						{
							startSpot[axis] = self->r.maxs[axis];
							endSpot[axis] = self->r.mins[axis];
						}
					}
					else
					{
						startSpot[axis] = self->r.mins[axis]+(flrand(0,1.0f)*(self->r.maxs[axis]-self->r.mins[axis]));
						endSpot[axis] = self->r.mins[axis]+(flrand(0,1.0f)*(self->r.maxs[axis]-self->r.mins[axis]));
					}
				}
				//FIXME: maybe trace from start to end to make sure nothing is in the way?  How big of a trace?

				G_SetOrigin( newAsteroid, startSpot );
				dist = Distance( endSpot, startSpot );
				time = ceil(dist/speed)*1000;
				Q3_Lerp2Origin( -1, newAsteroid->s.number, endSpot, time );

				//spin it
				startAngles[0] = flrand( -360, 360 );
				startAngles[1] = flrand( -360, 360 );
				startAngles[2] = flrand( -360, 360 );
				G_SetAngles( newAsteroid, startAngles );
				newAsteroid->s.apos.trDelta[0] = flrand( -100, 100 );
				newAsteroid->s.apos.trDelta[1] = flrand( -100, 100 );
				newAsteroid->s.apos.trDelta[2] = flrand( -100, 100 );
				newAsteroid->s.apos.trTime = level.time;
				newAsteroid->s.apos.trType = TR_LINEAR;

				//remove itself when done
				newAsteroid->think = G_FreeEntity;
				newAsteroid->nextthink = level.time+time;

				//think again sooner if need even more
				if ( numAsteroids+1 < self->count )
				{//still need at least one more
					//spawn it in 100ms
					self->nextthink = level.time + 100;
				}
			}
		}
	}
}

/*QUAKED trigger_asteroid_field (.5 .5 .5) ?
speed - how fast, on average, the asteroid moves
count - how many asteroids, max, to have at one time
target - target this at func_rotating asteroids
*/
void SP_trigger_asteroid_field(gentity_t *self)
{
	trap->SetBrushModel( (sharedEntity_t *)self, self->model );
	self->r.contents = 0;

	if ( !self->count )
	{
		self->health = 20;
	}

	if ( !self->speed )
	{
		self->speed = 10000;
	}

	self->think = asteroid_field_think;
	self->nextthink = level.time + 100;

    trap->LinkEntity((sharedEntity_t *)self);
}
