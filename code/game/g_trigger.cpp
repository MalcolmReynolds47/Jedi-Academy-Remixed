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
#include "b_local.h"
#include "anims.h"
#include "../cgame/cg_local.h"

#define ENTDIST_PLAYER	1
#define ENTDIST_NPC		2

extern qboolean G_PointInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs );
extern qboolean G_ClearTrace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int ignore, int clipmask );
extern qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
extern qboolean PM_CrouchAnim( int anim );
extern void Boba_FlyStart( gentity_t *self );
extern qboolean Boba_Flying( gentity_t *self );

#include "g_trigger_init.inc"

#include "g_contact_clocks.inc"
#include "g_contact_trigger.inc"
#include "g_sandbox_push.inc"
#include "g_sandbox_teleport.inc"

#include "g_trigger_multiple.inc"

qboolean G_TriggerActive( gentity_t *self )
{
	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return qfalse;
	}

	if ( self->spawnflags & 1 )
	{//player only
		return qfalse;
	}

	/*
	???
	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		return qfalse;
	}
	*/

	/*
	???
	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		return qfalse;
	}
	*/

	/*
	if ( self->radius )
	{//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		return qfalse;
	}
	*/
	return qtrue;
}

/*QUAKED trigger_multiple (.1 .5 .1) ? PLAYERONLY FACING USE_BUTTON FIRE_BUTTON NPCONLY LIMITED_PILOT x INACTIVE MULTIPLE
PLAYERONLY - only a player can trigger this by touch
FACING - Won't fire unless triggering ent's view angles are within 45 degrees of trigger's angles (in addition to any other conditions)
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
FIRE_BUTTON - Won't fire unless player/NPC is in it and pressing fire button (in addition to any other conditions)
NPCONLY - only non-player NPCs can trigger this by touch
LIMITED_PILOT - only spawn if there are open pilot slots
INACTIVE - Start off, has to be activated to be touchable/usable
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"wait"		Seconds between triggerings, 0 default, number < 0 means one time only.
"random"	wait variance, default is 0
"delay"		how many seconds to wait to fire targets after tripped
"hiderange" As long as NPC's head is in this trigger, NPCs out of this hiderange cannot see him.  If you set an angle on the trigger, they're only hidden from enemies looking in that direction.  the player's crouch viewheight is 36, his standing viewheight is 54.  So a trigger thast should hide you when crouched but not standing should be 48 tall.
"target2"	The trigger will fire this only when the trigger has been activated and subsequently 'cleared'( once any of the conditions on the trigger have not been satisfied).  This will not fire the "target" more than once until the "target2" is fired (trigger field is 'cleared')
"speed"		How many seconds to wait to fire the target2, default is 1
"noise"		Sound to play when the trigger fires (plays at activator's origin)
"max_pilots"	Number of pilots this spawner will allow

Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"team" - If set, only this team can trip this trigger
	player
	enemy
	neutral

"soundSet"	Ambient sound set to play when this trigger is activated
*/
#include "g_trigger_multiple_spawn.inc"

/*QUAKED trigger_once (.5 1 .5) ? PLAYERONLY FACING USE_BUTTON FIRE_BUTTON NPCONLY x x INACTIVE MULTIPLE
PLAYERONLY - only a player can trigger this by touch
FACING - Won't fire unless triggering ent's view angles are within 45 degrees of trigger's angles (in addition to any other conditions)
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
FIRE_BUTTON - Won't fire unless player/NPC is in it and pressing fire button (in addition to any other conditions)
NPCONLY - only non-player NPCs can trigger this by touch
INACTIVE - Start off, has to be activated to be touchable/usable
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"random"	wait variance, default is 0
"delay"		how many seconds to wait to fire targets after tripped
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
"noise"		Sound to play when the trigger fires (plays at activator's origin)

"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"team" - If set, only this team can trip this trigger
	player
	enemy
	neutral

"soundSet"	Ambient sound set to play when this trigger is activated
*/
void SP_trigger_once( gentity_t *ent )
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

	ent->wait = -1;

	ent->e_TouchFunc = touchF_Touch_Multi;
	ent->e_UseFunc   = useF_Use_Multi;

	MultiTriggerTeam(*ent,siege!=0);

	ent->delay *= 1000;//1 = 1 msec, 1000 = 1 sec

	InitTrigger( ent );
	gi.linkentity (ent);
}


#include "g_bidirectional.inc"

/*QUAKED trigger_location (.1 .5 .1) ?
When an ent is asked for it's location, it will return this ent's "message" field if it is in it.
  "message" - location name

  NOTE: always rectangular
*/
#include "g_location_lookup.inc"

void SP_trigger_location( gentity_t *ent )
{
	if ( !ent->message || !ent->message[0] )
	{
		gi.Printf("WARNING: trigger_location with no message!\n");
		G_FreeEntity(ent);
		return;
	}

	G_JAR_SetBrushModel( ent, ent->model );
	if (jar::volumeEntity::Portable(*ent)) {
		jar::volumeEntity::InitializeLocation(*ent);
		jar::volumeEntity::Refresh(*ent);
	}
	ent->contents = 0;
	ent->svFlags = SVF_NOCLIENT;

	gi.linkentity (ent);
}
/*
==============================================================================

trigger_always

==============================================================================
*/

#include "g_always_trigger.inc"


/*
==============================================================================

trigger_push

==============================================================================
*/
#define PUSH_CONVEYOR 32
void trigger_push_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	if(G_PushKind(*self)==jar::push::Volume){
		if(other&&other->client)G_PushTouchAt(*self,other,other->currentOrigin);
		return;
	}
	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if( level.time < self->painDebounceTime + self->wait  ) // normal 'wait' check
	{
		if( self->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in one frame
		{
			if ( self->painDebounceTime && level.time > self->painDebounceTime ) // if we haven't reached the next frame continue to let ents touch the trigger
			{
				return;
			}
		}
		else // only allowing one ent per frame to touch trigger
		{
			return;
		}
	}

	// if the player has already activated this trigger this frame
	if( other && !other->s.number && self->aimDebounceTime == level.time )
	{
		return;
	}


	if( self->spawnflags & PUSH_CONVEYOR )
	{   // only push player if he's on the ground
		if( other->s.groundEntityNum == ENTITYNUM_NONE )
		{
			return;
		}
	}

	if ( self->spawnflags & 1 )
	{//PLAYERONLY
		if ( other->s.number != 0 )
		{
			return;
		}
	}
	else
	{
		if ( self->spawnflags & 8 )
		{//NPCONLY
			if ( other->NPC == NULL )
			{
				return;
			}
		}
	}

	if ( !other->client ) {
		if ( other->s.pos.trType != TR_STATIONARY && other->s.pos.trType != TR_LINEAR_STOP && other->s.pos.trType != TR_NONLINEAR_STOP && VectorLengthSquared( other->s.pos.trDelta ) )
		{//already moving
			VectorCopy( other->currentOrigin, other->s.pos.trBase );
			VectorCopy( self->s.origin2, other->s.pos.trDelta );
			other->s.pos.trTime = level.time;
		}
		return;
	}

	if ( other->client->ps.pm_type != PM_NORMAL ) {
		return;
	}

	if ( (self->spawnflags&16) )
	{//relative, dir to it * speed
		vec3_t dir;
		VectorSubtract( self->s.origin2, other->currentOrigin, dir );
		if ( self->speed )
		{
			VectorNormalize( dir );
			VectorScale( dir, self->speed, dir );
		}
		VectorCopy( dir, other->client->ps.velocity );
	}
	else if ( (self->spawnflags&4) )
	{//linear dir * speed
		VectorScale( self->s.origin2, self->speed, other->client->ps.velocity );
	}
	else
	{
		VectorCopy( self->s.origin2, other->client->ps.velocity );
	}
	//so we don't take damage unless we land lower than we start here...
	other->client->ps.forceJumpZStart = 0;
	other->client->ps.pm_flags |= PMF_TRIGGER_PUSHED;//pushed by a trigger
	other->client->ps.jumpZStart = other->client->ps.origin[2];

	if ( self->wait == -1 )
	{
		self->e_TouchFunc = touchF_NULL;
	}
	else if ( self->wait > 0 )
	{
		self->painDebounceTime = level.time;

	}
	if( other && !other->s.number )
	{	// mark that the player has activated this trigger this frame
		self->aimDebounceTime =level.time;
	}
}

#define PUSH_CONSTANT 2

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget( gentity_t *self )
{
	gentity_t	*ent;
	vec3_t		origin;
	float		height, gravity, time, forward;
	float		dist;

	VectorAdd( self->absmin, self->absmax, origin );
	VectorScale ( origin, 0.5, origin );

	ent = G_PickTarget( self->target );
	if ( !ent )
	{
		G_FreeEntity( self );
		return;
	}

	if ( self->classname && !Q_stricmp( "trigger_push", self->classname ) )
	{
		if ( (self->spawnflags&2) )
		{//check once a second to see if we should activate or deactivate ourselves
			self->e_ThinkFunc = thinkF_trigger_push_checkclear;
			self->nextthink = level.time + FRAMETIME;
		}

		if ( (self->spawnflags&16) )
		{//relative, not an arc or linear
			VectorCopy( ent->currentOrigin, self->s.origin2 );
			return;
		}
		else if ( (self->spawnflags&4) )
		{//linear, not an arc
			VectorSubtract( ent->currentOrigin, origin, self->s.origin2 );
			VectorNormalize( self->s.origin2 );
			return;
		}
	}

	if ( self->classname && !Q_stricmp( "target_push", self->classname ) )
	{
		if( self->spawnflags & PUSH_CONSTANT )
		{
			VectorSubtract ( ent->s.origin, self->s.origin, self->s.origin2 );
			VectorNormalize( self->s.origin2);
			VectorScale (self->s.origin2, self->speed, self->s.origin2);
			return;
		}
	}
	height = ent->s.origin[2] - origin[2];
	if ( height < 0 )
	{//sqrt of negative is bad!
		height = 0;
	}
	gravity = g_gravity->value;
	if ( gravity < 0 )
	{
		gravity = 0;
	}
	time = sqrt( height / ( .5 * gravity ) );
	if ( !time ) {
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract ( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[2] = 0;
	dist = VectorNormalize( self->s.origin2);

	forward = dist / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[2] = time * gravity;
}

void trigger_push_checkclear( gentity_t *self )
{
	trace_t	trace;
	vec3_t	center;

	self->nextthink = level.time + 500;

	VectorAdd( self->absmin, self->absmax, center );
	VectorScale( center, 0.5, center );

	gentity_t *target = G_Find( NULL, FOFS(targetname), self->target );
	gi.trace( &trace, center, vec3_origin, vec3_origin, target->currentOrigin, ENTITYNUM_NONE, CONTENTS_SOLID, (EG2_Collision)0, 0 );

	if ( trace.fraction >= 1.0f )
	{//can trace, turn on
		self->contents |= CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		self->e_TouchFunc = touchF_trigger_push_touch;
		gi.linkentity( self );
	}
	else
	{//no trace, turn off
		self->contents &= ~CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		self->e_TouchFunc = touchF_NULL;
		gi.unlinkentity( self );
	}
}
/*QUAKED trigger_push (.1 .5 .1) ? PLAYERONLY CHECKCLEAR LINEAR NPCONLY RELATIVE CONVEYOR x INACTIVE MULTIPLE
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
PLAYERONLY - only the player is affected
LINEAR - Instead of tossing the client at the target_position, it will push them towards it.  Must set a "speed" (see below)
CHECKCLEAR - Every 1 second, it will check to see if it can trace to the target_position, if it can, the trigger is touchable, if it can't, the trigger is not touchable
NPCONLY - only NPCs are affected
RELATIVE - instead of pushing you in a direction that is always from the center of the trigger to the target_position, it pushes *you* toward the target position, relative to your current location (can use with "speed"... if don't set a speed, it will use the distance from you to the target_position)
CONVEYOR - acts like a conveyor belt, will only push if player is on the ground ( should probably use RELATIVE also, if you want a true conveyor belt )
INACTIVE - not active until targeted by a target_activate
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

wait - how long to wait between pushes: -1 = push only once
speed - when used with the LINEAR spawnflag, pushes the client toward the position at a constant speed (default is 1000)
*/
void SP_trigger_push( gentity_t *self ) {
	InitTrigger (self);

	if ( self->wait > 0 )
	{
		self->wait *= 1000;
	}

	// unlike other triggers, we need to send this one to the client
	self->svFlags &= ~SVF_NOCLIENT;

	self->s.eType = ET_PUSH_TRIGGER;
	if ( !(self->spawnflags&2) )
	{//start on
		self->e_TouchFunc = touchF_trigger_push_touch;
	}
	if ( self->spawnflags & 4 )
	{//linear
		self->speed = 1000;
	}
	self->e_ThinkFunc = thinkF_AimAtTarget;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
	gi.linkentity (self);
}

void Use_target_push( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if(G_PushKind(*self)==jar::push::Marker){
		G_PushUse(*self,activator);
		return;
	}
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL ) {
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	VectorCopy( self->s.origin2, activator->client->ps.velocity );

	if( self->spawnflags & 4 ) // lower
	{
		// reset this so I don't take falling damage when I land
		activator->client->ps.jumpZStart = activator->currentOrigin[2];
	}

	//so we don't take damage unless we land lower than we start here...
	activator->client->ps.forceJumpZStart = 0;
	activator->client->ps.pm_flags |= PMF_TRIGGER_PUSHED;//pushed by a trigger

	// play fly sound every 1.5 seconds
	if ( self->noise_index && activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound( activator, self->noise_index );
	}
}


/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) ENERGYNOISE CONSTANT NO_DAMAGE
When triggered, pushes the activator in the direction of angles
"speed"		defaults to 1000
ENERGYNOISE plays energy noise
CONSTANT will push activator in direction of 'target' at constant 'speed'
NO_DAMAGE the activator won't take falling damage after being pushed
*/
void SP_target_push( gentity_t *self ) {


	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		//self->noise_index = G_SoundIndex("sound/ambience/forge/antigrav.wav");
	}
	if ( self->target ) {

		VectorCopy( self->s.origin, self->absmin );
		VectorCopy( self->s.origin, self->absmax );
		self->e_ThinkFunc = thinkF_AimAtTarget;
		self->nextthink = level.time + START_TIME_LINK_ENTS;

	}
	self->e_UseFunc = useF_Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/
#define SNAP_ANGLES 1
#define NO_MISSILES 2
#define NO_NPCS		4
#define TTSF_STASIS		8
#define TTSF_DEAD_OK	16
void TeleportMover( gentity_t *mover, vec3_t origin, vec3_t diffAngles, qboolean snapAngle );
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace )
{
	if(G_TeleportKind(*self)==jar::teleport::Volume){
		if(other&&other->client)G_TeleportTouchAt(*self,other,other->currentOrigin);
		return;
	}
	gentity_t	*dest;

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	dest = 	G_PickTarget( self->target );
	if (!dest)
	{
		gi.Printf ("Couldn't find teleporter destination\n");
		return;
	}

	if ( other->client )
	{
		if ( other->client->ps.pm_type == PM_DEAD )
		{
			if ( !(self->spawnflags&TTSF_DEAD_OK) )
			{//dead men can't teleport
				return;
			}
		}
		if ( other->NPC )
		{
			if ( self->spawnflags & NO_NPCS )
			{
				return;
			}
		}

		if ( other->client->playerTeam != TEAM_FREE && SpotWouldTelefrag2( other, dest->currentOrigin ) )//SpotWouldTelefrag( dest, other->client->playerTeam ) )
		{//Don't go through if something blocking on the other side
			return;
		}

		TeleportPlayer( other, dest->s.origin, dest->s.angles );
	}
	//FIXME: check for SVF_NO_TELEPORT
	else if ( !(self->svFlags & SVF_NO_TELEPORT) && !(self->spawnflags & NO_MISSILES) && VectorLengthSquared( other->s.pos.trDelta ) )
	{//It's a mover of some sort and is currently moving
		vec3_t	diffAngles = {0, 0, 0};
		qboolean	snap = qfalse;

		if ( self->lastEnemy )
		{
			VectorSubtract( dest->s.angles, self->lastEnemy->s.angles, diffAngles );
		}
		else
		{//snaps to angle
			VectorSubtract( dest->s.angles, other->currentAngles, diffAngles );
			snap = qtrue;
		}

		TeleportMover( other, dest->s.origin, diffAngles, snap );
	}
}

void trigger_teleporter_find_closest_portal( gentity_t *self )
{
	gentity_t *found = NULL;
	vec3_t		org, vec;
	float		dist, bestDist = 64*64;

	VectorAdd( self->mins, self->maxs, org );
	VectorScale( org, 0.5, org );
	while ( (found = G_Find( found, FOFS(classname), "misc_portal_surface" )) != NULL )
	{
		VectorSubtract( found->currentOrigin, org, vec );
		dist = VectorLengthSquared( vec );
		if ( dist < bestDist )
		{
			self->lastEnemy = found;
			bestDist = dist;
		}
	}

	if ( self->lastEnemy )
	{
		gi.Printf("trigger_teleporter found misc_portal_surface\n");
	}
	self->e_ThinkFunc = thinkF_NULL;
}

/*QUAKED trigger_teleport (.1 .5 .1) ? SNAP_ANGLES NO_MISSILES NO_NPCS STASIS DEAD_OK x x INACTIVE
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

SNAP_ANGLES - Make the entity that passes through snap to the target_position's angles
NO_MISSILES - Missiles and thrown objects cannot pass through
NO_NPCS - NPCs cannot pass through
STASIS - will play stasis teleport sound and fx instead of starfleet
DEAD_OK - even if dead, you will teleport
*/
void SP_trigger_teleport( gentity_t *self )
{
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	self->svFlags &= ~SVF_NOCLIENT;

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->e_TouchFunc = touchF_trigger_teleporter_touch;

	self->e_ThinkFunc = thinkF_trigger_teleporter_find_closest_portal;
	self->nextthink = level.time + START_TIME_LINK_ENTS;

	gi.linkentity (self);
}



#include "g_trigger_hurt.inc"

#include "g_space.inc"

#include "g_shipboundary.inc"
/*
==============================================================================

timer

==============================================================================
*/


#include "g_logic_timer.inc"

/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED trigger_entdist (.1 .5 .1) (-8 -8 -8) (8 8 8) PLAYER NPC
fires if the given entity is within the given distance.  Sets itself inactive after one use.
----- KEYS -----
distance - radius entity can be away to fire trigger
target - fired if entity is within distance
target2 - fired if entity not within distance

NPC_target - NPC_types to look for
ownername - If any, who to calc the distance from- default is the trigger_entdist himself
example: target "biessman telsia" will look for the biessman and telsia NPC
if it finds either of these within distance it will fire.

  todo -
  add delay, count
  add monster classnames?????
  add LOS to it???
*/

#include "g_trigger_distance.inc"
#include "g_visible_trigger.inc"

// spawnflag
#define	TRIGGERVISIBLE_FORCESIGHT		2

void trigger_visible_check_player_visibility( gentity_t *self )
{
	if (G_VisiblePortable(*self))
	{
		G_VisibleCheck(self);
		return;
	}

	//Check every FRAMETIME*2
	self->nextthink = level.time + FRAMETIME*2;

	if ( self->svFlags & SVF_INACTIVE )
	{
		return;
	}

	vec3_t	dir;
	float	dist;
	gentity_t	*player = &g_entities[0];

	if (!player || !player->client )
	{
		return;
	}

	// Added 01/20/03 by AReis
	// If this trigger can only be used if the players force sight is on...
	if ( self->spawnflags & TRIGGERVISIBLE_FORCESIGHT )
	{
		// If their force sight is not on, leave...
		if ( !( player->client->ps.forcePowersActive & (1 << FP_SEE) ) )
		{
			return;
		}
	}

	//1: see if player is within 512*512 range
	VectorSubtract( self->currentOrigin, player->client->renderInfo.eyePoint, dir );
	dist = VectorNormalize( dir );
	if ( dist < self->radius )
	{//Within range
		vec3_t	forward;
		float	dot;
		//2: see if dot to us and player viewangles is > 0.7
		AngleVectors( player->client->renderInfo.eyeAngles, forward, NULL, NULL );
		dot = DotProduct( forward, dir );
		if ( dot > self->random )
		{//Within the desired FOV
			//3: see if player is in PVS
			if ( gi.inPVS( self->currentOrigin, player->client->renderInfo.eyePoint ) )
			{
				vec3_t	mins = {-1, -1, -1};
				vec3_t	maxs = {1, 1, 1};
				//4: If needbe, trace to see if there is clear LOS from player viewpos
				if ( (self->spawnflags&1) || G_ClearTrace( player->client->renderInfo.eyePoint, mins, maxs, self->currentOrigin, 0, MASK_OPAQUE ) )
				{
					//5: Fire!
					G_UseTargets( self, player );
					//6: Remove yourself
					G_FreeEntity( self );
				}
			}
		}
	}

}

/*QUAKED trigger_visible (.1 .5 .1) (-8 -8 -8) (8 8 8) NOTRACE FORCESIGHT x x x x x INACTIVE

  Only fires when player is looking at it, fires only once then removes itself.

  NOTRACE - Doesn't check to make sure the line of sight is completely clear (penetrates walls, forcefields, etc)
  FORCESIGHT - Only activates this trigger if force sight is on.
  INACTIVE - won't check for player visibility until activated

  radius - how far this ent can be from player's eyes, max, and still be considered "seen"
  FOV - how far off to the side of the player's field of view this can be, max, and still be considered "seen".  Player FOV is 80, so the default for this value is 30.

  "target" - What to use when it fires.
*/
void SP_trigger_visible( gentity_t *self )
{
	if (G_VisiblePortable(*self))
	{
		G_VisibleSpawn(self);
		return;
	}

	if ( self->radius <= 0 )
	{
		self->radius = 512;
	}

	if ( self->random <= 0 )
	{//about 30 degrees
		self->random = 0.7f;
	}
	else
	{//convert from FOV degrees to number meaningful for dot products
		self->random = 1.0f - (self->random/90.0f);
	}

	if ( self->spawnflags & 128 )
	{// Make it inactive
		self->svFlags |= SVF_INACTIVE;
	}

	G_SetOrigin( self, self->s.origin );
	gi.linkentity( self );

	self->e_ThinkFunc = thinkF_trigger_visible_check_player_visibility;
	self->nextthink = level.time + FRAMETIME*2;
}
