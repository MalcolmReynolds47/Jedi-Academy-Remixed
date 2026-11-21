/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// Draw-surface sorting. GPL-2.0-only; see LICENSE.txt.
// Extracted from OpenJK tr_main.cpp for headless regression coverage.
#include "../server/exe_headers.h"
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

//==========================================================================================

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( const surfaceType_t *surface, const shader_t *shader, int fogIndex, int dlightMap )
{
	int			index;

	if (tr.refdef.numDrawSurfs >= MAX_DRAWSURFS) {
		static unsigned long long dropped=0;
		if((++dropped==1 || dropped%4096==0) && ri.JAR_Log)
			ri.JAR_Log(1,"capacity","draw_surfaces","dropped=%llu limit=%d",dropped,MAX_DRAWSURFS);
		return;
	}
	index = tr.refdef.numDrawSurfs;

	if ( tr.refdef.doLAGoggles )
	{
		fogIndex = tr.world->numfogs;
	}
	else if (tr.currentEntityNum >= 0 && tr.currentEntityNum < tr.refdef.num_entities &&
		shader != tr.shadowShader && shader != tr.projectionShadowShader)
	{
		const auto& entity = tr.refdef.entities[tr.currentEntityNum];
		if (entity.riftFogRoom)
			fogIndex = entity.riftFogIndex;
	}

	if ( (shader->surfaceFlags & SURF_FORCESIGHT) && !(tr.refdef.rdflags & RDF_ForceSightOn) )
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

	// the sort data is packed into a single 64 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (uint64_t(shader->sortedIndex) << QSORT_SHADERNUM_SHIFT)
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = (surfaceType_t *)surface;
	tr.refdef.numDrawSurfs++;
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( uint64_t sort, int *entityNum, shader_t **shader,
					 int *fogNum, int *dlightMap ) {
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_REFENTITYNUM_SHIFT ) & REFENTITYNUM_MASK;
	*dlightMap = sort & 3;
}
