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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"
static QINLINE void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];
  int           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  end = sortKey + ( size * sizeof( drawSurf_t ) );
  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
R_RadixSort

Radix sort over all eight bytes of the JA:R draw key
===============
*/
void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
  R_Radix( 4, size, source, scratch );
  R_Radix( 5, size, scratch, source );
  R_Radix( 6, size, source, scratch );
  R_Radix( 7, size, scratch, source );
#else
  R_Radix( 7, size, source, scratch );
  R_Radix( 6, size, scratch, source );
  R_Radix( 5, size, source, scratch );
  R_Radix( 4, size, scratch, source );
  R_Radix( 3, size, source, scratch );
  R_Radix( 2, size, scratch, source );
  R_Radix( 1, size, source, scratch );
  R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}

void R_DecomposeSort( uint64_t sort, int *entityNum, shader_t **shader, int *cubemap, int *postRender )
{
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & QSORT_SHADERNUM_MASK ];
	*postRender = (sort >> QSORT_POSTRENDER_SHIFT ) & QSORT_POSTRENDER_MASK;
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & QSORT_ENTITYNUM_MASK;
	*cubemap = (sort >> QSORT_CUBEMAP_SHIFT ) & QSORT_CUBEMAP_MASK;
}

uint64_t R_CreateSortKey(int entityNum, int sortedShaderIndex, int cubemapIndex, int postRender)
{
	uint64_t key = 0;

	key |= (uint64_t(sortedShaderIndex) & QSORT_SHADERNUM_MASK) << QSORT_SHADERNUM_SHIFT;
	key |= (uint64_t(cubemapIndex) & QSORT_CUBEMAP_MASK) << QSORT_CUBEMAP_SHIFT;
	key |= (uint64_t(postRender) & QSORT_POSTRENDER_MASK) << QSORT_POSTRENDER_SHIFT;
	key |= (uint64_t(entityNum) & QSORT_ENTITYNUM_MASK) << QSORT_ENTITYNUM_SHIFT;

	return key;
}

void R_AddDrawSurf(
	surfaceType_t *surface,
	int entityNum,
	shader_t *shader,
	int fogIndex,
	int dlightMap,
	int postRender,
	int cubemap)
{
	int index;
	drawSurf_t *surf;
	shader_t *drawShader = (shader->remappedShader) ? shader->remappedShader : shader;

	if (tr.refdef.rdflags & RDF_NOFOG)
	{
		fogIndex = 0;
	}
	else if (tr.refdef.doLAGoggles)
	{
		fogIndex = tr.world->numfogs;
	}
#ifdef REND2_SP
	else if (entityNum >= 0 && entityNum < tr.refdef.num_entities &&
		drawShader != tr.shadowShader && drawShader != tr.projectionShadowShader)
	{
		const auto& entity = tr.refdef.entities[entityNum];
		if (entity.riftFogRoom)
			fogIndex = entity.riftFogIndex;
	}
#endif
	if ( (drawShader->surfaceFlags & SURF_FORCESIGHT) && !(tr.refdef.rdflags & RDF_ForceSightOn) )
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

	if (tr.viewParms.flags & VPF_DEPTHSHADOW &&
		(postRender == qtrue || drawShader->sort != SS_OPAQUE))
	{
		return;
	}

	// Preserve submitted draws when this frame's capacity is exhausted.
	if (tr.refdef.numDrawSurfs >= MAX_DRAWSURFS) {
		static unsigned long long dropped=0;
		if((++dropped==1 || dropped%4096==0) && ri.JAR_Log)
			ri.JAR_Log(1,"capacity","draw_surfaces","dropped=%llu limit=%d",dropped,MAX_DRAWSURFS);
		return;
	}
	index = tr.refdef.numDrawSurfs;
	surf = tr.refdef.drawSurfs + index;
	surf->surface = surface;

	if (tr.viewParms.flags & VPF_DEPTHSHADOW &&
		drawShader->depthPrepass == DEPTHPREPASS_SIMPLE)
	{
		surf->sort = R_CreateSortKey(entityNum, tr.defaultShader->sortedIndex, 0, 0);
		surf->dlightBits = 0;
		surf->fogIndex = 0;
	}
	else
	{
		surf->sort = R_CreateSortKey(entityNum, drawShader->sortedIndex, cubemap, postRender);
		surf->dlightBits = dlightMap;
		surf->fogIndex = fogIndex;
	}

	tr.refdef.numDrawSurfs++;
}

