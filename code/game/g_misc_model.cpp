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

#include "g_local.h"
#include "g_functions.h"
#include "g_workshop.h"
#include "bg_public.h"
#include "../cgame/cg_local.h"
extern cvar_t *g_spskill;

//
// Helper functions
//
//------------------------------------------------------------
void SetMiscModelModels( char *modelNameString, gentity_t *ent, qboolean damage_model )
{
	char	damageModel[MAX_QPATH];
	char	chunkModel[MAX_QPATH];
	int		len;

	//Main model
	ent->s.modelindex = G_ModelIndex( modelNameString );

	if ( damage_model )
	{
		len = strlen( modelNameString ) - 4; // extract the extension

		//Dead/damaged model
		strncpy( damageModel, modelNameString, len );
		damageModel[len] = 0;
		strncpy( chunkModel, damageModel, sizeof(chunkModel));
		strcat( damageModel, "_d1.md3" );
		ent->s.modelindex2 = G_ModelIndex( damageModel );

		ent->spawnflags |= 4; // deadsolid

		//Chunk model
		strcat( chunkModel, "_c1.md3" );
		ent->s.modelindex3 = G_ModelIndex( chunkModel );
	}
}

//------------------------------------------------------------
void SetMiscModelDefaults( gentity_t *ent, useFunc_t use_func, const char *material, int solid_mask,int animFlag,
									qboolean take_damage, qboolean damage_model = qfalse )
{
	// Apply damage and chunk models if they exist
	SetMiscModelModels( ent->model, ent, damage_model );

	ent->s.eFlags = animFlag;
	ent->svFlags |= SVF_PLAYER_USABLE;
	ent->contents = solid_mask;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	gi.linkentity (ent);

	// Set a generic use function

	ent->e_UseFunc = use_func;
/*	if (use_func == useF_health_use)
	{
		G_SoundIndex("sound/player/suithealth.wav");
	}
	else if (use_func == useF_ammo_use )
	{
		G_SoundIndex("sound/player/suitenergy.wav");
	}
*/
	G_SpawnInt( "material", material, (int *)&ent->material );

	if (ent->health)
	{
		ent->max_health = ent->health;
		ent->takedamage = take_damage;
		ent->e_PainFunc = painF_misc_model_breakable_pain;
		ent->e_DieFunc  = dieF_misc_model_breakable_die;
	}
}

void HealthStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 100;
			break;
		case 1:	//	MEDIUM
			ent->count = 75;
			break;
		default :
		case 2:	//	HARD
			ent->count = 50;
			break;
		}
	}
}


void CrystalAmmoSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 75;
			break;
		case 1:	//	MEDIUM
			ent->count = 75;
			break;
		default :
		case 2:	//	HARD
			ent->count = 75;
			break;
		}
	}
}


//------------------------------------------------------------

//------------------------------------------------------------
/*QUAKED misc_model_ghoul (1 0 0) (-16 -16 -37) (16 16 32)
"model"		arbitrary .glm file to display
"health" - how much health the model has - default 60 (zero makes non-breakable)
*/
//------------------------------------------------------------
#include "anims.h"
extern int G_ParseAnimFileSet( const char *skeletonName, const char *modelName=0);
int temp_animFileIndex;
void set_MiscAnim( gentity_t *ent)
{
	animation_t *animations = level.knownAnimFileSets[temp_animFileIndex].animations;
	if (ent->playerModel & 1)
	{
		int anim = BOTH_STAND3;
		float animSpeed = 50.0f / animations[anim].frameLerp;

		// yes, its the same animation, so work out where we are in the leg anim, and blend us
		gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
							(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
							BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND , animSpeed, (cg.time?cg.time:level.time), -1, 350);
	}
	else
	{
		int anim = BOTH_PAIN3;
		float animSpeed = 50.0f / animations[anim].frameLerp;
		gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
						(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
						BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeed, (cg.time?cg.time:level.time), -1, 350);
	}
	ent->nextthink = level.time + 900;
	ent->playerModel++;

}

void SP_misc_model_ghoul( gentity_t *ent )
{
#if 1
	ent->s.modelindex = G_ModelIndex( ent->model );
	gi.G2API_InitGhoul2Model(ent->ghoul2, ent->model, ent->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0);
	ent->s.radius = 50;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	qboolean bHasScale = G_SpawnVector( "modelscale_vec", "1 1 1", ent->s.modelScale );
	if ( !bHasScale ) {
		float temp;
		G_SpawnFloat( "modelscale", "0", &temp );
		if ( temp != 0.0f ) {
			ent->s.modelScale[0] = ent->s.modelScale[1] = ent->s.modelScale[2] = temp;
			bHasScale = qtrue;
		}
	}
	if ( bHasScale ) {
		//scale the x axis of the bbox up.
		ent->maxs[0] *= ent->s.modelScale[0];
		ent->mins[0] *= ent->s.modelScale[0];

		//scale the y axis of the bbox up.
		ent->maxs[1] *= ent->s.modelScale[1];
		ent->mins[1] *= ent->s.modelScale[1];

		//scale the z axis of the bbox up and adjust origin accordingly
		ent->maxs[2] *= ent->s.modelScale[2];
		float oldMins2 = ent->mins[2];
		ent->mins[2] *= ent->s.modelScale[2];
		ent->s.origin[2] += (oldMins2 - ent->mins[2]);
	}

	gi.linkentity (ent);
#else
	char name1[200] = "models/players/kyle/model.glm";
	ent->s.modelindex = G_ModelIndex( name1 );

	gi.G2API_InitGhoul2Model(ent->ghoul2, name1, ent->s.modelindex);
	ent->s.radius = 150;

			// we found the model ok - load it's animation config
	temp_animFileIndex = G_ParseAnimFileSet("_humanoid", "kyle");
 	if ( temp_animFileIndex<0 )
 	{
 		Com_Printf( S_COLOR_RED"Failed to load animation file set models/players/jedi/animation.cfg\n");
 	}


	ent->s.angles[0] = 0;
	ent->s.angles[1] = 90;
	ent->s.angles[2] = 0;

	ent->s.origin[2] = 20;
	ent->s.origin[1] = 80;
//	ent->s.modelScale[0] = ent->s.modelScale[1] = ent->s.modelScale[2] = 0.8f;

	VectorSet (ent->mins, -16, -16, -37);
	VectorSet (ent->maxs, 16, 16, 32);
//#if _DEBUG
//loadsavecrash
//	VectorCopy(ent->mins, ent->s.mins);
//	VectorCopy(ent->maxs, ent->s.maxs);
//#endif
	ent->contents = CONTENTS_BODY;
	ent->clipmask = MASK_NPCSOLID;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->health = 1000;

//	ent->s.modelindex = G_ModelIndex( "models/weapons2/blaster_r/g2blaster_w.glm" );
//	gi.G2API_InitGhoul2Model(ent->ghoul2, "models/weapons2/blaster_r/g2blaster_w.glm", ent->s.modelindex);
//	gi.G2API_AddBolt(&ent->ghoul2[0], "*weapon");
//	gi.G2API_AttachG2Model(&ent->ghoul2[1],&ent->ghoul2[0], 0, 0);

	gi.linkentity (ent);

	animation_t *animations = level.knownAnimFileSets[temp_animFileIndex].animations;
	int anim = BOTH_STAND3;
	float animSpeed = 50.0f / animations[anim].frameLerp;
	gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
					(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
					BONE_ANIM_OVERRIDE_FREEZE , animSpeed, cg.time);

//	int test = gi.G2API_GetSurfaceRenderStatus(&ent->ghoul2[0], "l_hand");
//	gi.G2API_SetSurfaceOnOff(&ent->ghoul2[0], "l_arm",0x00000100);
//	test = gi.G2API_GetSurfaceRenderStatus(&ent->ghoul2[0], "l_hand");

//	gi.G2API_SetNewOrigin(&ent->ghoul2[0], gi.G2API_AddBolt(&ent->ghoul2[0], "rhang_tag_bone"));
//	ent->s.apos.trDelta[1] = 10;
//	ent->s.apos.trType = TR_LINEAR;


	ent->nextthink = level.time + 1000;
	ent->e_ThinkFunc = thinkF_set_MiscAnim;
#endif
}


#include "g_rack_spawn.inc"

#define DROP_MEDPACK	1
#define DROP_SHIELDS	2
#define DROP_BACTA		4
#define DROP_BATTERIES	8

/*QUAKED misc_model_cargo_small (1 0 0.25) (-14 -14 -4) (14 14 30) MEDPACK SHIELDS BACTA BATTERIES
model="models/map_objects/kejim/cargo_small.md3"

  Cargo crate that can only be destroyed by heavy class weapons ( turrets, emplaced guns, at-st )  Can spawn useful things when it breaks

MEDPACK - instant use medpacks
SHIELDS - instant shields
BACTA - bacta tanks
BATTERIES -

"health" - how much damage to take before blowing up ( default 25 )
"splashRadius" - damage range when it explodes ( default 96 )
"splashDamage" - damage to do within explode range ( default 1 )

*/
extern gentity_t *LaunchItem( gitem_t *item, const vec3_t origin, const vec3_t velocity, char *target );

void misc_model_cargo_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	int		flags;
	vec3_t	org, temp;
	gitem_t *health = NULL, *shields = NULL, *bacta = NULL, *batteries = NULL;

	// copy these for later
	flags = self->spawnflags;
	VectorCopy( self->currentOrigin, org );

	// we already had spawn flags, but we don't care what they were...we just need to set up the flags we want for misc_model_breakable_die
	self->spawnflags = 8; // NO_DMODEL

	// pass through to get the effects and such
	misc_model_breakable_die( self, inflictor, attacker, damage, mod );

	// now that the model is broken, we can safely spawn these in it's place without them being in solid
	temp[2] = org[2] + 16;

	// annoying, but spawn each thing in its own little quadrant so that they don't end up on top of each other
	if (( flags & DROP_MEDPACK ))
	{
		health = FindItem( "item_medpak_instant" );

		if ( health )
		{
			temp[0] = org[0] + Q_flrand(-1.0f, 1.0f) * 8 + 16;
			temp[1] = org[1] + Q_flrand(-1.0f, 1.0f) * 8 + 16;

			LaunchItem( health, temp, vec3_origin, NULL );
		}
	}
	if (( flags & DROP_SHIELDS ))
	{
		shields = FindItem( "item_shield_sm_instant" );

		if ( shields )
		{
			temp[0] = org[0] + Q_flrand(-1.0f, 1.0f) * 8 - 16;
			temp[1] = org[1] + Q_flrand(-1.0f, 1.0f) * 8 + 16;

			LaunchItem( shields, temp, vec3_origin, NULL );
		}
	}

	if (( flags & DROP_BACTA ))
	{
		bacta = FindItem( "item_bacta" );

		if ( bacta )
		{
			temp[0] = org[0] + Q_flrand(-1.0f, 1.0f) * 8 - 16;
			temp[1] = org[1] + Q_flrand(-1.0f, 1.0f) * 8 - 16;

			LaunchItem( bacta, temp, vec3_origin, NULL );
		}
	}

	if (( flags & DROP_BATTERIES ))
	{
		batteries = FindItem( "item_battery" );

		if ( batteries )
		{
			temp[0] = org[0] + Q_flrand(-1.0f, 1.0f) * 8 + 16;
			temp[1] = org[1] + Q_flrand(-1.0f, 1.0f) * 8 - 16;

			LaunchItem( batteries, temp, vec3_origin, NULL );
		}
	}
}

//---------------------------------------------
void SP_misc_model_cargo_small( gentity_t *ent )
{
	G_SpawnInt( "splashRadius", "96", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "1", &ent->splashDamage );

	if (( ent->spawnflags & DROP_MEDPACK ))
	{
		RegisterItem( FindItem( "item_medpak_instant" ));
	}

	if (( ent->spawnflags & DROP_SHIELDS ))
	{
		RegisterItem( FindItem( "item_shield_sm_instant" ));
	}

	if (( ent->spawnflags & DROP_BACTA ))
	{
//		RegisterItem( FindItem( "item_bacta" ));
	}

	if (( ent->spawnflags & DROP_BATTERIES ))
	{
		RegisterItem( FindItem( "item_battery" ));
	}

	G_SpawnInt( "health", "25", &ent->health );

	SetMiscModelDefaults( ent, useF_NULL, "11", CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, 0, qtrue, qfalse );
	ent->s.modelindex2 = G_ModelIndex("/models/map_objects/kejim/cargo_small.md3");	// Precache model

	// we only take damage from a heavy weapon class missile
	ent->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;

	ent->e_DieFunc = dieF_misc_model_cargo_die;

	ent->radius = 1.5f; // scale number of chunks spawned
}

