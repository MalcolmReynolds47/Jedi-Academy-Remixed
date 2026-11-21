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
#include "g_functions.h"

// This version shares is in the thinkFunc format
//-----------------------------------------------------------------------------
void WP_Explode( gentity_t *self )
//-----------------------------------------------------------------------------
{
	if(gi.JAR_Log) gi.JAR_Log(2,"explosive","detonate","entity=%d damage=%d radius=%d time=%d",self->s.number,self->splashDamage,self->splashRadius,level.time);
	gentity_t	*attacker = self;
	vec3_t		forwardVec={0,0,1};

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	if ( !self->client )
	{
	AngleVectors( self->s.angles, forwardVec, NULL, NULL );
	}

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, forwardVec );
	}

	if ( self->owner )
	{
		attacker = self->owner;
	}
	else if ( self->activator )
	{
		attacker = self->activator;
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, 0/*don't ignore attacker*/, MOD_EXPLOSIVE_SPLASH );
	}

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_SetOrigin( self, self->currentOrigin );

	self->nextthink = level.time + 50;
	self->e_ThinkFunc = thinkF_G_FreeEntity;
}

// We need to have a dieFunc, otherwise G_Damage won't actually make us die.  I could modify G_Damage, but that entails too many changes
//-----------------------------------------------------------------------------
void WP_ExplosiveDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
//-----------------------------------------------------------------------------
{
	if(gi.JAR_Log) gi.JAR_Log(2,"explosive","chain_deferred","entity=%d attacker=%d inflictor=%d time=%d",self->s.number,attacker?attacker->s.number:-1,inflictor?inflictor->s.number:-1,level.time);
	self->enemy = attacker;

	if ( attacker && !attacker->s.number )
	{
		// less damage when shot by player
		self->splashDamage /= 3;
		self->splashRadius /= 3;
	}

	self->s.eFlags &= ~EF_FIRING; // don't draw beam if we are dead

	// Radius damage can kill hundreds of neighboring packs. Exploding here
	// recursively nests G_RadiusDamage (including its MAX_GENTITIES query
	// buffer) until the native stack overflows. Use the existing saved think
	// state to finish on the next game tick, and disarm immediately so further
	// hits cannot schedule another death or repeatedly reduce the blast.
	self->takedamage = qfalse;
	self->s.loopSound = 0;
	self->e_ThinkFunc = thinkF_WP_Explode;
	self->nextthink = level.time + 1;
}

