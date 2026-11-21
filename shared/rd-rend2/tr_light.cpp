/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.c

#include "tr_local.h"

#define	DLIGHT_AT_RADIUS		16
// at the edge of a dlight's influence, this amount of light will be added

#define	DLIGHT_MINIMUM_RADIUS	16
// never calculate a range less than this to prevent huge light numbers


/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *ori) {
	int		i;
	vec3_t	temp;

	for ( i = 0 ; i < count ; i++, dl++ ) {
		VectorSubtract( dl->origin, ori->origin, temp );
		dl->transformed[0] = DotProduct( temp, ori->axis[0] );
		dl->transformed[1] = DotProduct( temp, ori->axis[1] );
		dl->transformed[2] = DotProduct( temp, ori->axis[2] );
	}
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel( bmodel_t *bmodel, trRefEntity_t *ent ) {
	int			i, j;
	dlight_t	*dl;
	int			mask;
	msurface_t	*surf;

	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.ori );

	mask = 0;
	for ( i=0 ; i<tr.refdef.num_dlights ; i++ ) {
		dl = &tr.refdef.dlights[i];

		// see if the point is close enough to the bounds to matter
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( dl->transformed[j] - bmodel->bounds[1][j] > dl->radius ) {
				break;
			}
			if ( bmodel->bounds[0][j] - dl->transformed[j] > dl->radius ) {
				break;
			}
		}
		if ( j < 3 ) {
			continue;
		}

		// we need to check this light
		mask |= 1 << i;
	}

	ent->needDlights = (qboolean)(mask != 0);

	world_t *world = R_GetWorld(bmodel->worldIndex);
	// set the dlight bits in all the surfaces
	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		surf = world->surfaces + bmodel->firstSurface + i;

		switch(*surf->data)
		{
			case SF_FACE:
			case SF_GRID:
			case SF_TRIANGLES:
			case SF_VBO_MESH:
				((srfBspSurface_t *)surf->data)->dlightBits = mask;
				break;

			default:
				break;
		}
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern	cvar_t	*r_ambientScale;
extern	cvar_t	*r_directedScale;
extern	cvar_t	*r_debugLight;

/*
=================
R_SetupEntityLightingGrid

=================
*/
#define JAR_GRID_REND2
#include "../../code/rd-common/tr_bsp_grid_sample.inc"
#undef JAR_GRID_REND2

static void LogLight( trRefEntity_t *ent ) {
	int	max1, max2;

	if ( !(ent->e.renderfx & RF_FIRST_PERSON ) ) {
		return;
	}

	max1 = ent->ambientLight[0];
	if ( ent->ambientLight[1] > max1 ) {
		max1 = ent->ambientLight[1];
	} else if ( ent->ambientLight[2] > max1 ) {
		max1 = ent->ambientLight[2];
	}

	max2 = ent->directedLight[0];
	if ( ent->directedLight[1] > max2 ) {
		max2 = ent->directedLight[1];
	} else if ( ent->directedLight[2] > max2 ) {
		max2 = ent->directedLight[2];
	}

	ri.Printf( PRINT_ALL, "amb:%i  dir:%i\n", max1, max2 );
}

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent ) {
	int				i;
	float			d;
	vec3_t			lightDir;
	vec3_t			lightOrigin;

	// lighting calculations
	if ( ent->lightingCalculated ) {
		return;
	}
	ent->lightingCalculated = qtrue;

	//
	// trace a sample point down to find ambient light
	//
	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if ((refdef->rdflags & RDF_NOWORLDMODEL) || !R_SetupEntityLightingGrid(ent)) {
		ent->ambientLight[0] = ent->ambientLight[1] =
			ent->ambientLight[2] = tr.identityLight * 150;
		ent->directedLight[0] = ent->directedLight[1] =
			ent->directedLight[2] = tr.identityLight * 150;
		VectorCopy( tr.sunDirection, ent->lightDir );
	}

	// only do min lighting when there is no hdr light data
	if (tr.hdrLighting != qtrue)
	{
		if (ent->e.renderfx & RF_MINLIGHT)
		{ //the minlight flag is now for items rotating on their holo thing
			if (ent->e.shaderRGBA[0] == 255 &&
				ent->e.shaderRGBA[1] == 255 &&
				ent->e.shaderRGBA[2] == 0)
			{
				ent->ambientLight[0] += tr.identityLight * 255;
				ent->ambientLight[1] += tr.identityLight * 255;
				ent->ambientLight[2] += tr.identityLight * 0;
			}
			else
			{
				ent->ambientLight[0] += tr.identityLight * 16;
				ent->ambientLight[1] += tr.identityLight * 96;
				ent->ambientLight[2] += tr.identityLight * 150;
			}
		}
		// bonus items and view weapons have a fixed minimum add
		else if (ent->e.renderfx & RF_MORELIGHT) {
			ent->ambientLight[0] += tr.identityLight * 96;
			ent->ambientLight[1] += tr.identityLight * 96;
			ent->ambientLight[2] += tr.identityLight * 96;
		}
		else {
			// give everything a minimum light add
			ent->ambientLight[0] += tr.identityLight * 32;
			ent->ambientLight[1] += tr.identityLight * 32;
			ent->ambientLight[2] += tr.identityLight * 32;
		}
	}

	d = VectorLength( ent->directedLight );
	VectorScale( ent->lightDir, d, lightDir );

	// clamp ambient
	if (tr.hdrLighting != qtrue)
	{
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( ent->ambientLight[i] > tr.identityLightByte ) {
				ent->ambientLight[i] = tr.identityLightByte;
			}
		}
	}

	if ( r_debugLight->integer ) {
		LogLight( ent );
	}

	// save out the byte packet version
	((byte *)&ent->ambientLightInt)[0] = Q_ftol(ent->ambientLight[0]);
	((byte *)&ent->ambientLightInt)[1] = Q_ftol(ent->ambientLight[1]);
	((byte *)&ent->ambientLightInt)[2] = Q_ftol(ent->ambientLight[2]);
	((byte *)&ent->ambientLightInt)[3] = 0xff;

	// transform the direction to local space
	VectorNormalize( lightDir );
	VectorCopy(lightDir, ent->lightDir);

	ent->modelLightDir[0] = DotProduct( lightDir, ent->e.axis[0] );
	ent->modelLightDir[1] = DotProduct( lightDir, ent->e.axis[1] );
	ent->modelLightDir[2] = DotProduct( lightDir, ent->e.axis[2] );
}

/*
=================
R_LightForPoint
=================
*/
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	trRefEntity_t ent;


	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy( point, ent.e.origin );
	if (!R_SetupEntityLightingGrid(&ent)) return qfalse;
	VectorCopy(ent.ambientLight, ambientLight);
	VectorCopy(ent.directedLight, directedLight);
	VectorCopy(ent.lightDir, lightDir);

	return qtrue;
}


int R_LightDirForPoint( vec3_t point, vec3_t lightDir, vec3_t normal, world_t *world )
{
	trRefEntity_t ent;

	if (!R_ValidBSPGrid(world))
	{
		VectorCopy(normal, lightDir);
		return qfalse;
	}

	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy( point, ent.e.origin );
	if (!R_SetupEntityLightingGrid(&ent, world))
	{ VectorCopy(normal, lightDir); return qfalse; }

	if (DotProduct(ent.lightDir, normal) > 0.2f)
		VectorCopy(ent.lightDir, lightDir);
	else
		VectorCopy(normal, lightDir);

	return qtrue;
}

int R_DLightsForPoint(const vec3_t point, const float radius)
{
	int dlightBits = 0;
	vec3_t delta;
	dlight_t currentDlight;
	float distance;
	float radiusSum;
	for (int i = 0; i < tr.refdef.num_dlights; i++)
	{
		currentDlight = tr.refdef.dlights[i];

		VectorSubtract(point, currentDlight.origin, delta);
		distance = VectorLength(delta);
		radiusSum = radius + currentDlight.radius;

		if (distance < radiusSum)
			dlightBits |= 1 << i;
	}
	return dlightBits;
}

#include "tr_cubemap_select.inc"

#ifdef REND2_SP
//pass in origin
qboolean RE_GetLighting(const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
	trRefEntity_t tr_ent;

	memset(&tr_ent, 0, sizeof(tr_ent));

	if (ambientLight[0] == 666)
	{//HAX0R
		tr_ent.e.hModel = -1;
	}

	VectorCopy(origin, tr_ent.e.origin);

	if (!R_SetupEntityLightingGrid(&tr_ent)) {
		ambientLight[0] = ambientLight[1] = ambientLight[2] = 255.0;
		directedLight[0] = directedLight[1] = directedLight[2] = 255.0;
		VectorCopy(tr.sunDirection, lightDir);
		return qfalse;
	}
	VectorCopy(tr_ent.ambientLight, ambientLight);
	VectorCopy(tr_ent.directedLight, directedLight);
	VectorCopy(tr_ent.lightDir, lightDir);
	return qtrue;
}
#endif