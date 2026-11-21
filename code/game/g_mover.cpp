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

#include "bg_public.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"
#include "objectives.h"
#include "g_local.h"
#include "g_subbsp_movers.h"
#include "g_mover_portal.h"
#include "g_bsp_section.h"
#include "g_workshop_fx_aim.h"

#include "../icarus/IcarusInterface.h"


int	BMS_START = 0;
int	BMS_MID = 1;
int	BMS_END = 2;

extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
void CalcTeamDoorCenter ( gentity_t *ent, vec3_t center );
void InitMover( gentity_t *ent ) ;

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void MatchTeam( gentity_t *teamLeader, int moverState, int time );
extern qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);




/*
-------------------------
G_SetLoopSound
-------------------------
*/

void G_PlayDoorLoopSound( gentity_t *ent )
{
	if ( VALIDSTRING( ent->soundSet ) == false )
		return;

	sfxHandle_t	sfx = CAS_GetBModelSound( ent->soundSet, BMS_MID );

	if ( sfx == -1 )
	{
		ent->s.loopSound = 0;
		return;
	}

	ent->s.loopSound = sfx;
}

/*
-------------------------
G_PlayDoorSound
-------------------------
*/

void G_PlayDoorSound( gentity_t *ent, int type )
{
	if ( VALIDSTRING( ent->soundSet ) == false )
		return;

	sfxHandle_t	sfx = CAS_GetBModelSound( ent->soundSet, type );

	if ( sfx == -1 )
		return;

	vec3_t	doorcenter;
	CalcTeamDoorCenter( ent, doorcenter );
	if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
	{
		AddSoundEvent( ent->activator, doorcenter, 128, AEL_MINOR, qfalse, qtrue );
	}

	G_AddEvent( ent, EV_BMODEL_SOUND, sfx );
}

/*
============
G_TestEntityPosition

============
*/
#include "g_mover_push.inc"
#include "g_sandbox_angular_motion.inc"
#include "g_rail_portable.h"
#include "g_mover_motion.inc"

/*
=================
G_MoverTeam
=================
*/
#include "g_mover_team.inc"

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}
	if ( G_WorkshopTrailFrame(*ent) ) return;

	// if stationary at one of the positions, don't move anything
	if (G_SandboxAngular(*ent)) {
		G_SandboxAngularFrame(*ent,level.time-level.previousTime);
	} else if ( ent->s.pos.trType != TR_STATIONARY || !jar::angular::Stationary(ent->s.apos.trType) ) {
		G_MoverTeam( ent );
	}

/*	if ( ent->classname && Q_stricmp("misc_turret", ent->classname ) == 0 )
	{
		rebolt_turret( ent );
	}
*/
	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
CalcTeamDoorCenter

Finds all the doors of a team and returns their center position
*/

void CalcTeamDoorCenter ( gentity_t *ent, vec3_t center )
{
	if (jar::bspSection::SectionLift(*ent)) {
		jar::bspSection::Center(*ent, center);
		return;
	}
	vec3_t		slavecenter;
	gentity_t	*slave;

	//Start with our center
	VectorAdd(ent->mins, ent->maxs, center);
	VectorScale(center, 0.5, center);
	for ( slave = ent->teamchain ; slave ; slave = slave->teamchain )
	{
		//Find slave's center
		VectorAdd(slave->mins, slave->maxs, slavecenter);
		VectorScale(slavecenter, 0.5, slavecenter);
		//Add that to our own, find middle
		VectorAdd(center, slavecenter, center);
		VectorScale(center, 0.5, center);
	}
}

/*
===============
SetMoverState
===============
*/
#include "g_mover_train.inc"

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t		*slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		SetMoverState( slave, (moverState_t) moverState, time );
	}
}



/*
================
ReturnToPos1
================
*/
#include "g_mover_binary.inc"

/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
#include "g_mover_initmovertrdata.inc"

#include "g_mover_init.inc"


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

/*
================
Blocked_Door
================
*/
void Blocked_Door( gentity_t *ent, gentity_t *other ) {

	// remove anything other than a client -- no longer the case

	// don't remove security keys or goodie keys
	if ( (other->s.eType == ET_ITEM) && (other->item->giTag >= INV_GOODIE_KEY && other->item->giTag <= INV_SECURITY_KEY) )
	{
		// should we be doing anything special if a key blocks it... move it somehow..?
	}
	// if your not a client, or your a dead client remove yourself...
	else if ( other->s.number && (!other->client || (other->client && other->health <= 0 && other->contents == CONTENTS_CORPSE && !other->message)) )
	{
		if ( !IIcarusInterface::GetIcarus()->IsRunning( other->m_iIcarusID ) /*!other->taskManager || !other->taskManager->IsRunning()*/ )
		{
			// if an item or weapon can we do a little explosion..?
			G_FreeEntity( other );
			return;
		}
	}

	if ( ent->damage )
	{
		if ( (ent->spawnflags&MOVER_CRUSHER)//a crusher
 			&& other->s.clientNum >= MAX_CLIENTS//not the player
			&& other->client //NPC
			&& other->health <= 0 //dead
			&& G_OkayToRemoveCorpse( other ) )//okay to remove him
		{//crusher stuck on a non->player corpse that does not have a key and is not running a script
			G_FreeEntity( other );
		}
		else
		{
			G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
		}
	}
	if ( ent->spawnflags & MOVER_CRUSHER ) {
		return;		// crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( ent, ent, other );
}


/*
================
Touch_DoorTrigger
================
*/

#include "g_mover_touch_doortrigger.inc"

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
#include "g_door_trigger_spawn.inc"

void Think_MatchTeam( gentity_t *ent )
{
	MatchTeam( ent, ent->moverState, level.time );
}

qboolean G_EntIsDoor( int entityNum )
{
	if ( entityNum < 0 || entityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	gentity_t *ent = &g_entities[entityNum];
	if ( ent && !Q_stricmp( "func_door", ent->classname ) )
	{//blocked by a door
		return qtrue;
	}
	return qfalse;
}

gentity_t *G_FindDoorTrigger( gentity_t *ent )
{
	gentity_t *owner = NULL;
	gentity_t *door = ent;
	if ( door->flags & FL_TEAMSLAVE )
	{//not the master door, get the master door
		while ( door->teammaster && (door->flags&FL_TEAMSLAVE))
		{
			door = door->teammaster;
		}
	}
	if ( door->targetname )
	{//find out what is targeting it
		//FIXME: if ent->targetname, check what kind of trigger/ent is targetting it?  If a normal trigger (active, etc), then it's okay?
		while ( (owner = G_Find( owner, FOFS( target ), door->targetname )) != NULL )
		{
			if ( owner && (owner->contents&CONTENTS_TRIGGER) )
			{
				return owner;
			}
		}
		owner = NULL;
		while ( (owner = G_Find( owner, FOFS( target2 ), door->targetname )) != NULL )
		{
			if ( owner && (owner->contents&CONTENTS_TRIGGER) )
			{
				return owner;
			}
		}
	}

	owner = NULL;
	while ( (owner = G_Find( owner, FOFS( classname ), "trigger_door" )) != NULL )
	{
		if ( owner->owner == door )
		{
			return owner;
		}
	}

	return NULL;
}

qboolean G_TriggerActive( gentity_t *self );
qboolean G_EntIsUnlockedDoor( int entityNum )
{
	if ( entityNum < 0 || entityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	if ( G_EntIsDoor( entityNum ) )
	{
		gentity_t *ent = &g_entities[entityNum];
		gentity_t *owner = NULL;
		if ( ent->flags & FL_TEAMSLAVE )
		{//not the master door, get the master door
			while ( ent->teammaster && (ent->flags&FL_TEAMSLAVE))
			{
				ent = ent->teammaster;
			}
		}
		if ( ent->targetname )
		{//find out what is targetting it
			owner = NULL;
			//FIXME: if ent->targetname, check what kind of trigger/ent is targetting it?  If a normal trigger (active, etc), then it's okay?
			while ( (owner = G_Find( owner, FOFS( target ), ent->targetname )) != NULL )
			{
				if ( !Q_stricmp( "trigger_multiple", owner->classname )
				  || !Q_stricmp( "trigger_once", owner->classname ) )//FIXME: other triggers okay too?
				{
					if ( G_TriggerActive( owner ) )
					{
						return qtrue;
					}
				}
			}
			owner = NULL;
			while ( (owner = G_Find( owner, FOFS( target2 ), ent->targetname )) != NULL )
			{
				if ( !Q_stricmp( "trigger_multiple", owner->classname ) )//FIXME: other triggers okay too?
				{
					if ( G_TriggerActive( owner ) )
					{
						return qtrue;
					}
				}
			}
			return qfalse;
		}
		else
		{//check the door's auto-created trigger instead
			owner = G_FindDoorTrigger( ent );
			if ( owner && (owner->svFlags&SVF_INACTIVE) )
			{//owning auto-created trigger is inactive
				return qfalse;
			}
		}
		if ( !(ent->svFlags & SVF_INACTIVE) && //assumes that the reactivate trigger isn't right next to the door!
			!ent->health &&
			!(ent->spawnflags & MOVER_PLAYER_USE) &&
			!(ent->spawnflags & MOVER_FORCE_ACTIVATE) &&
			!(ent->spawnflags & MOVER_LOCKED))
			//FIXME: what about MOVER_GOODIE?
		{
			return qtrue;
		}
	}
	return qfalse;
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN FORCE_ACTIVATE CRUSHER TOGGLE LOCKED GOODIE PLAYER_USE INACTIVE
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
FORCE_ACTIVATE	Can only be activated by a force push or pull
CRUSHER		?
TOGGLE		wait in both the start and end states for a trigger event - does NOT work on Trek doors.
LOCKED		Starts locked, with the shader animmap at the first frame and inactive.  Once used, the animmap changes to the second frame and the door operates normally.  Note that you cannot use the door again after this.
GOODIE		Door will not work unless activator has a "goodie key" in his inventory
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

"target"     Door fires this when it starts moving from it's closed position to its open position.
"opentarget" Door fires this after reaching its "open" position
"target2"    Door fires this when it starts moving from it's open position to its closed position.
"closetarget" Door fires this after reaching its "closed" position
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"delay"		when used, how many seconds to wait before moving - default is none
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default, set to negative for no damage)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
"sounds" - sound door makes when opening/closing
"linear"	set to 1 and it will move linearly rather than with acceleration (default is 0)
0 - no sound (default)
*/
#include "g_mover_sp_func_door.inc"
#include "g_sliding_door.inc"
#include "g_bobbing_sandbox.inc"

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
#include "g_mover_touch_platcentertrigger.inc"


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
#include "g_mover_spawnplattrigger.inc"


/*QUAKED func_plat (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius
*/
#include "g_mover_sp_func_plat.inc"


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
#include "g_mover_sp_func_button.inc"
#include "g_button_sandbox.inc"



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
===============
Reached_Train
===============
*/


void TrainUse( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	Reached_Train( ent );
}

/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent ) {
	gentity_t		*path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS(targetname), ent->target );
	if ( !ent->nextTrain ) {
		gi.Printf( "func_train at %s with an unfound target\n", vtos(ent->absmin) );
		//Free me?`
		return;
	}

	//FIXME: this can go into an infinite loop if last path_corner doesn't link to first
	//path_corner, like so:
	// t1---->t2---->t3
	//         ^      |
	//          \_____|
	start = NULL;
	next = NULL;
	int iterations = 2000;	//max attempts to find our path start
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if (!iterations--)
		{
			G_Error( "Think_SetupTrainTargets:  last path_corner doesn't link back to first on func_train(%s)", vtos(ent->absmin) );
		}
		if (!start)
		{
			start = path;
		}
		if ( !path->target ) {
//			gi.Printf( "Train corner at %s without a target\n", vtos(path->s.origin) );
			//end of path
			break;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next ) {
//				gi.Printf( "Train corner at %s without a target path_corner\n", vtos(path->s.origin) );
				//end of path
				break;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		if ( next )
		{
			path->nextTrain = next;
		}
		else
		{
			break;
		}
	}

	if ( !ent->targetname || (ent->spawnflags&1) /*start on*/)
	{
		// start the train moving from the first corner
		Reached_Train( ent );
	}
	else
	{
		G_SetOrigin( ent, ent->s.origin );
	}
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TURN_TRAIN INVISIBLE YAW_TRAIN ROLL_TRAIN

TURN_TRAIN	func_train moving on this path will turn to face the next path_corner within 2 seconds
INVISIBLE - train will become invisible ( but still solid ) when it reaches this path_corner.
		It will become visible again at the next path_corner that does not have this option checked

Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		gi.Printf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in
	VectorCopy(self->s.origin, self->currentOrigin);
}


void func_train_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	if ( self->target3 )
	{
		G_UseTargets2( self, self, self->target3 );
	}

	//HACKHACKHACKHACK
	G_PlayEffect( "explosions/fighter_explosion2", self->currentOrigin );
	G_FreeEntity( self );
	//HACKHACKHACKHACK
}

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS x x LOOP PLAYER_USE INACTIVE TIE

LOOP - if it's a ghoul2 model, it will just loop the animation
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used
TIE  flying Tie-fighter hack, should be made more flexible so other things can use this if needed

A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		default 100
"dmg"		default	2
"health"	default 0
"noise"		looping sound to play when the train is in motion
"targetname" will not start until used
"target"	next path corner
"target3" what to use when it breaks
"color"		constantLight color
"light"		constantLight radius
"linear" set to 1 and it will move linearly rather than with acceleration (default is 0)
"startframe" default 0...ghoul2 animation start frame
"endframe" default 0...ghoul2 animation end frame
*/
void SP_func_train (gentity_t *self) {
	VectorClear (self->s.angles);

	if (self->spawnflags & TRAIN_BLOCK_STOPS) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		gi.Printf ("func_train without a target at %s\n", vtos(self->absmin));
		G_FreeEntity( self );
		return;
	}

	char *noise;

	G_SpawnInt( "startframe", "0", &self->startFrame );
	G_SpawnInt( "endframe", "0", &self->endFrame );

	if ( G_SpawnString( "noise", "", &noise ) )
	{
		if ( VALIDSTRING( noise ) )
		{
			self->s.loopSound = cgi_S_RegisterSound( noise );//G_SoundIndex( noise );
		}
	}

	G_JAR_SetBrushModel( self, self->model );
	InitMover( self );

	if ( self->spawnflags & 2048 ) // TIE HACK
	{
		//HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
		self->s.modelindex2 = G_ModelIndex( "models/map_objects/ships/tie_fighter.md3" );
		G_EffectIndex( "explosions/fighter_explosion2" );

		self->contents = CONTENTS_SHOTCLIP;
		self->takedamage = qtrue;
		VectorSet( self->maxs, 112, 112, 112 );
		VectorSet( self->mins, -112, -112, -112 );
		self->e_DieFunc  = dieF_func_train_die;
		gi.linkentity( self );
		//HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
	}

	if ( self->targetname )
	{
		self->e_UseFunc = useF_TrainUse;
	}

	self->e_ReachedFunc = reachedF_Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + START_TIME_LINK_ENTS;
	self->e_ThinkFunc = thinkF_Think_SetupTrainTargets;


	if ( self->playerModel >= 0 && self->spawnflags & 32 ) // loop...dunno, could support it on other things, but for now I need it for the glider...so...kill the flag
	{
		self->spawnflags &= ~32; // once only

		gi.G2API_SetBoneAnim( &self->ghoul2[self->playerModel], "model_root", self->startFrame, self->endFrame, BONE_ANIM_OVERRIDE_LOOP, 1.0f + Q_flrand(-1.0f, 1.0f) * 0.1f, 0, -1, -1 );
		self->endFrame = 0; // don't allow it to do anything with the animation function in G_main
	}
}

/*
===============================================================================

STATIC

===============================================================================
*/

/*QUAKED func_static (0 .5 .8) ? F_PUSH F_PULL SWITCH_SHADER CRUSHER IMPACT SOLITARY PLAYER_USE INACTIVE BROADCAST
F_PUSH		Will be used when you Force-Push it
F_PULL		Will be used when you Force-Pull it
SWITCH_SHADER	Toggle the shader anim frame between 1 and 2 when used
CRUSHER		Make it do damage when it's blocked
IMPACT		Make it do damage when it hits any entity
SOLITARY	Can only be pushed when directly under crosshair, when pushed you shall push nothing else BUT me.
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used
BROADCAST   don't ever use this, it's evil

A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius
"dmg"		how much damage to do when it crushes (use with CRUSHER spawnflag default is 2)
"linear" set to 1 and it will move linearly rather than with acceleration (default is 0)
"NPC_targetname" if set up to be push/pullable, only this NPC can push/pull it (for the player, use "player")
*/
void SP_func_static( gentity_t *ent )
{
	G_JAR_SetBrushModel( ent, ent->model );

	VectorCopy( ent->s.origin, ent->pos1 );
	VectorCopy( ent->s.origin, ent->pos2 );

	InitMover( ent );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->e_UseFunc = useF_func_static_use;
	ent->e_ReachedFunc = reachedF_NULL;


	if( ent->spawnflags & 2048 )
	{								   // yes this is very very evil, but for now (pre-alpha) it's a solution
		ent->svFlags |= SVF_BROADCAST; // I need to rotate something that is huge and it's touching too many area portals...
	}

	if ( ent->spawnflags & 4/*SWITCH_SHADER*/ )
	{
		ent->s.eFlags |= EF_SHADER_ANIM;//use frame-controlled shader anim
		ent->s.frame = 0;//first stage of anim
		ent->spawnflags &= ~4;//this is the CRUSHER spawnflag!  remove it!
	}
	if ( ent->spawnflags & 8 )
	{//!!! 8 is NOT the crusher spawnflag, 4 is!!!
		ent->spawnflags &= ~8;
		ent->spawnflags |= MOVER_CRUSHER;
		if ( !ent->damage )
		{
			ent->damage = 2;
		}
	}
	gi.linkentity( ent );

	if (level.mBSPInstanceDepth)
	{	// this means that this guy will never be updated, moved, changed, etc.
		ent->s.eFlags = EF_PERMANENT;
	}
}

void func_static_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior( self, BSET_USE );

	if ( self->spawnflags & 4/*SWITCH_SHADER*/ )
	{
		self->s.frame = self->s.frame ? 0 : 1;//toggle frame
	}
	G_UseTargets( self, activator );
}

/*
===============================================================================

ROTATING

===============================================================================
*/
#include "g_mover_func_rotating_touch.inc"


#include "g_mover_func_rotating_use.inc"


/*QUAKED func_rotating (0 .5 .8) ? START_ON TOUCH_KILL X_AXIS Y_AXIS x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
TOUCH_KILL  Inflicts massive damage upon touching it, disitegrates bodies
INACTIVE	must be used by a target_activate before it can be used

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
#include "g_mover_sp_func_rotating.inc"


/*
===============================================================================

BOBBING

===============================================================================
*/

#include "g_mover_func_bobbing_use.inc"

/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS START_OFF x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

Normally bobs on the Z axis
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"targetname" turns on/off when used
*/
#include "g_mover_sp_func_bobbing.inc"

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
#include "g_mover_sp_func_pendulum.inc"

/*
===============================================================================

WALL

===============================================================================
*/

//static -slc
#include "g_wall_sandbox.h"

void use_wall( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if (G_WallPortable(*ent))
	{
		if (!(ent->svFlags & SVF_INACTIVE) && !G_WallSetVisible(*ent, !ent->count) &&
			activator && activator->client && activator->s.number == 0)
			gi.SendServerCommand(0, "cp \"Wall is blocked. Clear the space and try again.\"");
		return;
	}
	G_ActivateBehavior(ent,BSET_USE);

	// Not there so make it there
	//if (!(ent->contents & CONTENTS_SOLID))
	if (!ent->count) // off
	{
		ent->svFlags &= ~SVF_NOCLIENT;
		ent->s.eFlags &= ~EF_NODRAW;
		ent->count = 1;
		//NOTE: reset the brush model because we need to get *all* the contents back
		//ent->contents |= CONTENTS_SOLID;
		G_JAR_SetBrushModel( ent, ent->model );
		if ( !(ent->spawnflags&1) )
		{//START_OFF doesn't effect area portals
			gi.AdjustAreaPortalState( ent, qfalse );
		}
	}
	// Make it go away
	else
	{
		//NOTE: MUST do this BEFORE clearing contents, or you may not open the area portal!!!
		if ( !(ent->spawnflags&1) )
		{//START_OFF doesn't effect area portals
			gi.AdjustAreaPortalState( ent, qtrue );
		}
		ent->contents = 0;
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->count = 0; // off
	}
}

#define FUNC_WALL_OFF	1
#define FUNC_WALL_ANIM	2


/*QUAKED func_wall (0 .5 .8) ? START_OFF AUTOANIMATE x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius

START_OFF - the wall will not be there
AUTOANIMATE - if a model is used it will animate
*/
void SP_func_wall( gentity_t *ent )
{
	if (G_WallPortable(*ent))
	{
		G_JAR_SetBrushModel(ent, ent->model);
		G_SetOrigin(ent, ent->s.origin);
		ent->s.eType = ET_GENERAL;
		ent->e_UseFunc = useF_use_wall;
		if (!G_WallSetVisible(*ent, !(ent->spawnflags & FUNC_WALL_OFF)))
			G_FreeEntity(ent);
		return;
	}
	G_JAR_SetBrushModel( ent, ent->model );

	VectorCopy( ent->s.origin, ent->pos1 );
	VectorCopy( ent->s.origin, ent->pos2 );

	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// count is used as an on/off switch (start it on)
	ent->count = 1;

	// it must be START_OFF
	if (ent->spawnflags & FUNC_WALL_OFF)
	{
		ent->spawnContents = ent->contents;
		ent->contents = 0;
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		// turn it off
		ent->count = 0;
	}

	if (!(ent->spawnflags & FUNC_WALL_ANIM))
	{
		ent->s.eFlags |= EF_ANIM_ALLFAST;
	}
	ent->e_UseFunc = useF_use_wall;

	gi.linkentity (ent);

}


void security_panel_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}
	if ( INV_SecurityKeyCheck( activator, self->message ) )
	{//congrats!
		gi.SendServerCommand( 0, "cp @SP_INGAME_SECURITY_KEY_UNLOCKEDDOOR" );
		//use targets
		G_UseTargets( self, activator );
		//take key
		INV_SecurityKeyTake( activator, self->message );
		if ( activator->ghoul2.size() )
		{
			gi.G2API_SetSurfaceOnOff( &activator->ghoul2[activator->playerModel], "l_arm_key", 0x00000002 );
		}
		//FIXME: maybe set frame on me to have key sticking out?
		//self->s.frame = 1;
		//play sound
		G_Sound( self, self->soundPos2 );
		//unusable
		self->e_UseFunc = useF_NULL;
	}
	else
	{//failure sound/display
		if ( activator->message )
		{//have a key, just the wrong one
			gi.SendServerCommand( 0, "cp @SP_INGAME_INCORRECT_KEY" );
		}
		else
		{//don't have a key at all
			gi.SendServerCommand( 0, "cp @SP_INGAME_NEED_SECURITY_KEY" );
		}
		G_UseTargets2( self, activator, self->target2 );
		//FIXME: change display?  Maybe shader animmap change?
		//play sound
		G_Sound( self, self->soundPos1 );
	}
}

/*QUAKED misc_security_panel (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x INACTIVE
model="models/map_objects/kejim/sec_panel.md3"
  A model that just sits there and opens when a player uses it and has right key

INACTIVE - Start off, has to be activated to be usable

"message"	name of the key player must have
"target"	thing to use when successfully opened
"target2"	thing to use when player uses the panel without the key
*/
void SP_misc_security_panel ( gentity_t *self )
{
	self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/sec_panel.md3" );
	self->soundPos1 = G_SoundIndex( "sound/movers/sec_panel_fail.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/sec_panel_pass.mp3" );
	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	VectorSet( self->mins, -8, -8, -8 );
	VectorSet( self->maxs, 8, 8, 8 );
	self->contents = CONTENTS_SOLID;
	gi.linkentity( self );

	//NOTE: self->message is the key
	self->svFlags |= SVF_PLAYER_USABLE;
	if(self->spawnflags & 128)
	{
		self->svFlags |= SVF_INACTIVE;
	}
	self->e_UseFunc = useF_security_panel_use;
}
