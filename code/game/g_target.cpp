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

#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "g_functions.h"
#include "g_sandbox_laser.h"
#include "g_siege_item.h"
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );

//==========================================================

//==========================================================

#include "g_target_logic.inc"

//==========================================================

//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8)
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(ent,BSET_USE);
	if(G_SiegeKorribanPrint(*ent))return;

	// Portable timers/always triggers fire without a player activator.
	if ((!activator || !activator->client) && ent->targetname && !strncmp(ent->targetname,"jar_sb_",7))
		activator=&g_entities[0];
	if ( activator && activator->client ) {
		gi.SendServerCommand( activator-g_entities, "cp \"%s\"", ent->message );
	}
}

void SP_target_print( gentity_t *ent ) {
	ent->e_UseFunc = useF_Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

"sounds" va() min max, so if your sound string is borgtalk%d.wav, and you set a "sounds" value of 4, it will randomly play borgtalk1.wav - borgtalk4.wav when triggered
to use this, you must store the wav name in "soundGroup", NOT "noise"

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
#include "g_sandbox_speaker.inc"

void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if (G_SandboxSpeaker(*ent)) {
		speakerUse(*ent, activator);
		return;
	}
	if(ent->painDebounceTime > level.time)
	{
		return;
	}

	G_ActivateBehavior(ent,BSET_USE);

	if ( ent->sounds )
	{
		ent->noise_index = G_SoundIndex( va( ent->paintarget, Q_irand(1, ent->sounds ) ) );
	}

	if (ent->spawnflags & 3) {	// looping sound toggles
		gentity_t *looper = ent;
		if ( ent->spawnflags & 8 ) {
			looper = activator;
		}
		if (looper->s.loopSound)
			looper->s.loopSound = 0;	// turn it off
		else
			looper->s.loopSound = ent->noise_index;	// start it
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}

	if(ent->wait < 0)
	{//BYE!
		ent->e_UseFunc = useF_NULL;
	}
	else
	{
		ent->painDebounceTime = level.time + ent->wait;
	}
}

#include "g_target_sp_speaker.inc"



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
#include "g_sandbox_laser.inc"
void target_laser_think (gentity_t *self) {
	if(G_SandboxLaser(*self)){sandboxLaserThink(*self);return;}
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->mins, point);
		VectorMA (point, 0.5, self->enemy->maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	gi.trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE, (EG2_Collision)0, 0);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir,
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_ENERGY );
	}

	VectorCopy (tr.endpos, self->s.origin2);

	gi.linkentity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	gi.unlinkentity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if(G_SandboxLaser(*self)){
		G_SandboxLaserUse(*self);
		return;
	}
	G_ActivateBehavior(self,BSET_USE);

	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			gi.Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		G_SetEnemy( self, ent );
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->e_UseFunc   = useF_target_laser_use;
	self->e_ThinkFunc = thinkF_target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	if(G_SandboxLaser(*self)){G_SandboxLaserInitialize(*self,{});return;}
	// let everything else get spawned before we start firing
	self->e_ThinkFunc = thinkF_target_laser_start;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}


//==========================================================

#include "g_bsp_section.h"
#include "g_start_marker.inc"
#include "g_sandbox_teleport.h"

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if(G_BSPSectionEnter(self,activator))return;
	if(G_TeleportKind(*self)==jar::teleport::Marker){G_TeleportUse(*self,activator);return;}
	gentity_t	*dest;

	if (!activator->client)
		return;

	G_ActivateBehavior(self,BSET_USE);

	dest = 	G_PickTarget( self->target );
	if (!dest) {
		gi.Printf ("Couldn't find teleporter destination\n");
		return;
	}

	if (!G_JAR_VisitStart(dest, activator)) TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAK-ED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if (!self->targetname)
		gi.Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->e_UseFunc = useF_target_teleporter_use;
}

//==========================================================


/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
info_notnull does the same thing
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}

#include "g_target_markers.inc"

//===NEW===================================================================

#include "g_target_scriptrunner.inc"
#include "g_target_movement.inc"

void set_mission_stats_cvars( void )
{
	char text[1024]={0};

	//we'll assume that the activator is the player
	gclient_t* const client = &level.clients[0];

	if (!client)
	{
		return;
	}

	gi.cvar_set("ui_stats_enemieskilled", va("%d",client->sess.missionStats.enemiesKilled));	//pass this on to the menu

	if (cg_entities[0].gent->client->sess.missionStats.totalSecrets)
	{
		cgi_SP_GetStringTextString( "SP_INGAME_SECRETAREAS_OF", text, sizeof(text) );
		gi.cvar_set("ui_stats_secretsfound", va("%d %s %d",
			cg_entities[0].gent->client->sess.missionStats.secretsFound,
			text,
			cg_entities[0].gent->client->sess.missionStats.totalSecrets));
	}
	else	// Setting ui_stats_secretsfound to 0 will hide the text on screen
	{
		gi.cvar_set("ui_stats_secretsfound", "0");
	}

	// Find the favorite weapon
	int wpn=0,i;
	int max_wpn = cg_entities[0].gent->client->sess.missionStats.weaponUsed[0];
	for (i = 1; i<WP_NATIVE_WEAPON_COUNT; i++)
	{
		if (cg_entities[0].gent->client->sess.missionStats.weaponUsed[i] > max_wpn)
		{
			max_wpn = cg_entities[0].gent->client->sess.missionStats.weaponUsed[i];
			wpn = i;
		}
	}

	if ( wpn )
	{
		gitem_t	*wItem= FindItemForWeapon( (weapon_t)wpn);
		cgi_SP_GetStringTextString( va("SP_INGAME_%s",wItem->classname ), text, sizeof( text ));
		gi.cvar_set("ui_stats_fave", va("%s",text));	//pass this on to the menu
	}

	gi.cvar_set("ui_stats_shots", va("%d",client->sess.missionStats.shotsFired));				//pass this on to the menu

	gi.cvar_set("ui_stats_hits", va("%d",client->sess.missionStats.hits));						//pass this on to the menu

	const float percent = cg_entities[0].gent->client->sess.missionStats.shotsFired? 100.0f * (float)cg_entities[0].gent->client->sess.missionStats.hits / cg_entities[0].gent->client->sess.missionStats.shotsFired : 0;
	gi.cvar_set("ui_stats_accuracy", va("%.2f%%",percent));						//pass this on to the menu

	gi.cvar_set("ui_stats_thrown", va("%d",client->sess.missionStats.saberThrownCnt));						//pass this on to the menu

	gi.cvar_set("ui_stats_blocks", va("%d",client->sess.missionStats.saberBlocksCnt));
	gi.cvar_set("ui_stats_legattacks", va("%d",client->sess.missionStats.legAttacksCnt));
	gi.cvar_set("ui_stats_armattacks", va("%d",client->sess.missionStats.armAttacksCnt));
	gi.cvar_set("ui_stats_bodyattacks", va("%d",client->sess.missionStats.torsoAttacksCnt));

	gi.cvar_set("ui_stats_absorb", va("%d",client->sess.missionStats.forceUsed[FP_ABSORB]));
	gi.cvar_set("ui_stats_heal", va("%d",client->sess.missionStats.forceUsed[FP_HEAL]));
	gi.cvar_set("ui_stats_mindtrick", va("%d",client->sess.missionStats.forceUsed[FP_TELEPATHY]));
	gi.cvar_set("ui_stats_protect", va("%d",client->sess.missionStats.forceUsed[FP_PROTECT]));

	gi.cvar_set("ui_stats_jump", va("%d",client->sess.missionStats.forceUsed[FP_LEVITATION]));
	gi.cvar_set("ui_stats_pull", va("%d",client->sess.missionStats.forceUsed[FP_PULL]));
	gi.cvar_set("ui_stats_push", va("%d",client->sess.missionStats.forceUsed[FP_PUSH]));
	gi.cvar_set("ui_stats_sense", va("%d",client->sess.missionStats.forceUsed[FP_SEE]));
	gi.cvar_set("ui_stats_speed", va("%d",client->sess.missionStats.forceUsed[FP_SPEED]));
	gi.cvar_set("ui_stats_defense", va("%d",client->sess.missionStats.forceUsed[FP_SABER_DEFENSE]));
	gi.cvar_set("ui_stats_offense", va("%d",client->sess.missionStats.forceUsed[FP_SABER_OFFENSE]));
	gi.cvar_set("ui_stats_throw", va("%d",client->sess.missionStats.forceUsed[FP_SABERTHROW]));

	gi.cvar_set("ui_stats_drain", va("%d",client->sess.missionStats.forceUsed[FP_DRAIN]));
	gi.cvar_set("ui_stats_grip", va("%d",client->sess.missionStats.forceUsed[FP_GRIP]));
	gi.cvar_set("ui_stats_lightning", va("%d",client->sess.missionStats.forceUsed[FP_LIGHTNING]));
	gi.cvar_set("ui_stats_rage", va("%d",client->sess.missionStats.forceUsed[FP_RAGE]));

}

#include "../cgame/cg_media.h"	//access to cgs
extern void G_ChangeMap (const char *mapname, const char *spawntarget, qboolean hub);	//g_utils
#include "g_level_change_use.inc"
void target_level_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if(self&&G_LevelChangePortable(*self)){G_LevelChangeUse(self);return;}
	G_ActivateBehavior(self,BSET_USE);

	if( self->message && !Q_stricmp( "disconnect", self->message ) )
	{
		gi.SendConsoleCommand( "disconnect\n");
	}
	else
	{
		G_ChangeMap( self->message, self->target, (qboolean)((self->spawnflags&1) != 0) );
	}
	if (self->count>=0)
	{
		gi.cvar_set("tier_storyinfo", va("%i",self->count));
		if (level.mapname[0] == 't' && level.mapname[2] == '_'
			&& ( level.mapname[1] == '1' || level.mapname[1] == '2' || level.mapname[1] == '3' )
			)
		{
			char s[2048];
			gi.Cvar_VariableStringBuffer("tiers_complete", s, sizeof(s));	//get the current list
			if (*s)
			{
				gi.cvar_set("tiers_complete", va("%s %s", s, level.mapname));	//strcat this level into the existing list
			}
			else
			{
				gi.cvar_set("tiers_complete", level.mapname);	//set this level into the list
			}
		}
		if (self->noise_index)
		{
			cgi_S_StopSounds();
			cgi_S_StartSound( NULL, 0, CHAN_VOICE, cgs.sound_precache[ self->noise_index ] );
		}
	}

	set_mission_stats_cvars();

}

/*QUAKED target_level_change (1 0 0) (-4 -4 -4) (4 4 4) HUB NO_STORYSOUND
HUB - Will save the current map's status and load the next map with any saved status it may have
NO_STORYSOUND - will not play storyinfo wav file, even if you '++' or set tier_storyinfo

"mapname" - Name of map to change to or "+menuname" to launch a menu instead
"target" - Name of spawnpoint to start at in the new map
"tier_storyinfo" - integer to set cvar or "++" to just increment
"storyhead"	 - which head to show on menu [luke, kyle, or prot]
"saber_menu" - integer to set cvar for menu
"weapon_menu" - integer to set cvar for ingame weapon menu
*/
void SP_target_level_change( gentity_t *self )
{
	if(G_LevelChangePortable(*self)){if(!G_LevelChangeSpawn(*self))G_FreeEntity(self);return;}
	if ( !self->message )
	{
		G_Error( "target_level_change with no mapname!\n");
		return;
	}

	char *s;
	if (G_SpawnString( "tier_storyinfo", "", &s ))
	{
		if (*s == '+')
		{
			self->noise_index = G_SoundIndex(va("sound/chars/tiervictory/%s.mp3",level.mapname) );
			self->count = gi.Cvar_VariableIntegerValue("tier_storyinfo")+1;
								G_SoundIndex(va("sound/chars/storyinfo/%d.mp3",self->count));	//cache for menu
		}
		else
		{
			self->count = atoi(s);
			if( !(self->spawnflags & 2) )
			{
				self->noise_index = G_SoundIndex(va("sound/chars/storyinfo/%d.mp3",self->count) );
			}
		}

		if (G_SpawnString( "storyhead", "", &s ))
		{	//[luke, kyle, or prot]
			gi.cvar_set("storyhead", s);	//pass this on to the menu
		}
		else
		{	//show head based on mapname
			gi.cvar_set("storyhead", level.mapname);	//pass this on to the menu
		}
	}
	if (G_SpawnString( "saber_menu", "", &s ))
	{
		gi.cvar_set("saber_menu", s);	//pass this on to the menu
	}

	if (G_SpawnString( "weapon_menu", "1", &s ))
	{
		gi.cvar_set("weapon_menu", s);	//pass this on to the menu
	}
	else
	{
		gi.cvar_set("weapon_menu", "0");	//pass this on to the menu
	}

	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_level_change_use;
}

/*QUAKED target_change_parm (1 0 0) (-4 -4 -4) (4 4 4)
Copies any parms set on this ent to the entity that  fired the trigger/button/whatever that uses this
parm1
parm2
parm3
parm4
parm5
parm6
parm7
parm8
parm9
parm10
parm11
parm12
parm13
parm14
parm15
parm16
*/
#include "g_target_parms.inc"

#include "g_target_music.inc"


void target_autosave_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	//gi.SendServerCommand( NULL, "cp @SP_INGAME_CHECKPOINT" );
	CG_CenterPrint( "@SP_INGAME_CHECKPOINT", SCREEN_HEIGHT * 0.25 );	//jump the network

	gi.SendConsoleCommand( "wait 2;save auto\n" );
}

/*QUAKED target_autosave (1 0 0) (-4 -4 -4) (4 4 4)
Auto save the game in two frames.
Make sure it won't trigger during dialogue or cinematic or it will stutter!
*/
void SP_target_autosave( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_autosave_use;
}

#include "g_target_secret.inc"

/*QUAKED target_secret (1 0 1) (-4 -4 -4) (4 4 4)
You found a Secret!
"count" - how many secrets on this level,
          if more than one on a level, be sure they all have the same count!
*/
void SP_target_secret( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_secret_use;
	self->noise_index = G_SoundIndex("sound/interface/secret_area");
	if (self->count)
	{
		gi.cvar_set("newTotalSecrets", va("%i",self->count));
	}
}

#include "g_siege_item.inc"
