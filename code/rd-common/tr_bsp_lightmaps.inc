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
// Native classic lightmap loading shared by SP and MP. GPL-2.0-only.
static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = Q_max( 0, r_mapOverBrightBits->integer - tr.overbrightBits );

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( (r|g|b) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
void R_ColorShiftLightingBytes( byte in[3] ) {
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = Q_max( 0, r_mapOverBrightBits->integer - tr.overbrightBits );

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( (r|g|b) > 255 ) {
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	in[0] = r;
	in[1] = g;
	in[2] = b;
}

/*
===============
R_LoadLightmaps

===============
*/
#define	LIGHTMAP_SIZE	128
static	void R_LoadLightmaps( lump_t *l, lump_t *surfs, const char *psMapName, world_t &worldData )
{
	byte				*buf, *buf_p;
	int					len;
	byte		image[LIGHTMAP_SIZE*LIGHTMAP_SIZE*4];
	int					i, j;
	float				maxIntensity = 0;
	double				sumIntensity = 0;
	int					count;

	if (&worldData == &s_worldData)
	{
		tr.numLightmaps = 0;
	}

	worldData.startLightMapIndex = tr.numLightmaps;
	worldData.numLightmaps = 0;
	len = l->filelen;
	const int bytesPerMap = LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;
	if (len < 0 || len % bytesPerMap || surfs->filelen < 0 || surfs->filelen % sizeof(dsurface_t))
		Com_Error(ERR_DROP, "Malformed BSP lightmap/surface lump");
	count = len / bytesPerMap;
	if (!len) {
		const dsurface_t *surfaces = (const dsurface_t *)(fileBase + surfs->fileofs);
		for (unsigned n = 0; n < surfs->filelen / sizeof(*surfaces); ++n)
			for (int style = 0; style < MAXLIGHTMAPS; ++style) {
				const int index = LittleLong(surfaces[n].lightmapNum[style]);
				if (index >= MAX_LIGHTMAPS) Com_Error(ERR_DROP, "External BSP lightmap index exceeds capacity");
				if (index >= count) count = index + 1;
			}
	}
	if (count > MAX_LIGHTMAPS - tr.numLightmaps)
		Com_Error(ERR_DROP, "BSP lightmaps require %d slots; capacity is %d", tr.numLightmaps+count, MAX_LIGHTMAPS);
	worldData.numLightmaps = count;
	memset(tr.lightmaps + tr.numLightmaps, 0, count * sizeof(*tr.lightmaps));
	tr.numLightmaps += count;
	if (!count || r_vertexLight->integer) return;
	buf = fileBase + l->fileofs;
	R_IssuePendingRenderCommands();

	char sMapName[MAX_QPATH];
	COM_StripExtension(psMapName,sMapName, sizeof(sMapName));

	for ( i = 0 ; i < count ; i++ ) {
		if (!len) {
			tr.lightmaps[worldData.startLightMapIndex+i] = R_FindImageFile(
				va("%s/lm_%04d.tga", sMapName, i), qfalse, qfalse,
				(qboolean)(r_ext_compressed_lightmaps->integer != 0), GL_CLAMP);
			continue;
		}
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * LIGHTMAP_SIZE*LIGHTMAP_SIZE * 3;

		if ( r_lightmap->integer == 2 )
		{	// color code by intensity as development tool	(FIXME: check range)
			for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
			{
				float r = buf_p[j*3+0];
				float g = buf_p[j*3+1];
				float b = buf_p[j*3+2];
				float intensity;
				float out[3] = {0.0f, 0.0f, 0.0f};

				intensity = 0.33f * r + 0.685f * g + 0.063f * b;

				if ( intensity > 255 )
					intensity = 1.0f;
				else
					intensity /= 255.0f;

				if ( intensity > maxIntensity )
					maxIntensity = intensity;

				HSVtoRGB( intensity, 1.00, 0.50, out );

				image[j*4+0] = out[0] * 255;
				image[j*4+1] = out[1] * 255;
				image[j*4+2] = out[2] * 255;
				image[j*4+3] = 255;

				sumIntensity += intensity;
			}
		} else {
			for ( j = 0 ; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ ) {
				byte rgba[4] = {buf_p[j*3],buf_p[j*3+1],buf_p[j*3+2],255};
				R_ColorShiftLightingBytes(rgba, &image[j*4]);
				image[j*4+3] = 255;
			}
		}
		tr.lightmaps[worldData.startLightMapIndex+i] = R_CreateImage(
			va("$%s/lightmap%d", sMapName, worldData.startLightMapIndex+i),
			image, LIGHTMAP_SIZE, LIGHTMAP_SIZE, GL_RGBA, qfalse, qfalse,
			(qboolean)(r_ext_compressed_lightmaps->integer != 0),
			GL_CLAMP);
	}

	if ( r_lightmap->integer == 2 )	{
		ri.Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}
}



// Reject references outside this map, even if another map owns that global slot.
static int R_BSPLightmapIndex(int index, const world_t &worldData) {
	if (index < 0) return index;
	return index < worldData.numLightmaps ? worldData.startLightMapIndex + index : LIGHTMAP_BY_VERTEX;
}

class JAR_ClassicBSPLightingScope {
	bool sub;
	vec3_t sunLight, sunDirection;
	int sunSurfaceLight;
public:
	explicit JAR_ClassicBSPLightingScope(bool isSub) : sub(isSub), sunSurfaceLight(tr.sunSurfaceLight) {
		VectorCopy(tr.sunLight,sunLight);VectorCopy(tr.sunDirection,sunDirection);
	}
	~JAR_ClassicBSPLightingScope() {
		if (!sub) return;
		VectorCopy(sunLight,tr.sunLight);VectorCopy(sunDirection,tr.sunDirection);tr.sunSurfaceLight=sunSurfaceLight;
	}
};
