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
// Native Rend2 lightmap decoding and packing. GPL-2.0-only.
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
static	void R_ColorShiftLightingBytes( byte in[4], byte out[4] ) {
	int		shift, r, g, b;

	// shift the color data based on overbright range
	shift = Q_max( 0, r_mapOverBrightBits->integer - tr.overbrightBits );

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
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
R_ColorShiftLightingFloats

===============
*/
static void R_ColorShiftLightingFloats(float in[4], float out[4], float scale, bool overbrightBits = true )
{
	float r, g, b;

	if (overbrightBits)
		scale *= pow(2.0f, r_mapOverBrightBits->integer - tr.overbrightBits);

	r = in[0] * scale;
	g = in[1] * scale;
	b = in[2] * scale;

	if (!glRefConfig.floatLightmap)
	{
		if (r > 1.0f || g > 1.0f || b > 1.0f)
		{
			float high = Q_max(Q_max(r, g), b);

			r /= high;
			g /= high;
			b /= high;
		}
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

void ColorToRGBA16F(const vec3_t color, unsigned short rgba16f[4])
{
	rgba16f[0] = FloatToHalf(color[0]);
	rgba16f[1] = FloatToHalf(color[1]);
	rgba16f[2] = FloatToHalf(color[2]);
	rgba16f[3] = FloatToHalf(1.0f);
}

/*
===============
R_LoadLightmaps

===============
*/
#define	DEFAULT_LIGHTMAP_SIZE	128
#include <climits>
static	void R_LoadLightmaps( world_t *worldData, lump_t *l, lump_t *surfs ) {
	worldData->lighting = {};
	char mapDirectory[MAX_QPATH];
	COM_StripExtension(worldData->name,mapDirectory,sizeof(mapDirectory));
	if (l->filelen < 0 || l->filelen % (DEFAULT_LIGHTMAP_SIZE*DEFAULT_LIGHTMAP_SIZE*3) ||
		surfs->filelen < 0 || surfs->filelen % sizeof(dsurface_t))
		ri.Error(ERR_DROP, "Malformed BSP lightmap/surface lump");
	byte		*buf = NULL, *buf_p = NULL;
	dsurface_t  *surf;
	int			len;
	byte		*image;
	int			imageSize;
	int			i, j, numLightmaps = 0, textureInternalFormat = 0;
	float maxIntensity = 0;
	int numColorComponents = 3;

	bool hdr_capable = glRefConfig.floatLightmap && r_hdr->integer;

	worldData->lighting.lightmapSize = DEFAULT_LIGHTMAP_SIZE;
	worldData->lighting.hdrLighting = qfalse;
	worldData->lighting.worldInternalLightmapping = qfalse;

	len = l->filelen;
	// test for external lightmaps
	if (!len) {
		for (i = 0, surf = (dsurface_t *)(fileBase + surfs->fileofs);
			(unsigned)i < surfs->filelen / sizeof(dsurface_t);
			i++, surf++) {
			for (int j = 0; j < MAXLIGHTMAPS; j++)
			{
				const int index=LittleLong(surf->lightmapNum[j]);
				if(index >= JAR_BSP_LIGHTMAP_LIMIT) ri.Error(ERR_DROP,"External BSP lightmap index exceeds capacity");
				numLightmaps = MAX(numLightmaps,index+1);
			}
		}
		buf = NULL;
	}
	else
	{
		numLightmaps = len / (worldData->lighting.lightmapSize * worldData->lighting.lightmapSize * 3);
		buf = fileBase + l->fileofs;
		worldData->lighting.worldInternalLightmapping = qtrue;
	}

	if (numLightmaps == 0) { R_PublishBSPLightmaps(worldData); return; }

	// test for hdr lighting
	if (hdr_capable && worldData->lighting.worldInternalLightmapping)
	{
		char filename[MAX_QPATH];
		byte *externalLightmap = NULL;
		int lightmapWidth = worldData->lighting.lightmapSize;
		int lightmapHeight = worldData->lighting.lightmapSize;
		Com_sprintf(filename, sizeof(filename), "%s/lm_%04d.hdr", mapDirectory, 0);
		R_LoadHDRImage(filename, &externalLightmap, &lightmapWidth, &lightmapHeight);
		if (externalLightmap != NULL)
		{
			worldData->lighting.worldInternalLightmapping = qfalse;
			Hunk_FreeTempMemory(externalLightmap);
		}
	}

	// we are about to upload textures
	R_IssuePendingRenderCommands();

	// check for deluxe mapping
	if (numLightmaps <= 1 || (len && (numLightmaps & 1)))
	{
		worldData->lighting.worldDeluxeMapping = qfalse;
	}
	else
	{
		worldData->lighting.worldDeluxeMapping = qtrue;
		worldData->lighting.worldInternalDeluxeMapping = qtrue;
		// Check that none of the deluxe maps are referenced by any of the map surfaces.
		for (i = 0, surf = (dsurface_t *)(fileBase + surfs->fileofs);
			worldData->lighting.worldDeluxeMapping && (unsigned)i < surfs->filelen / sizeof(dsurface_t);
			i++, surf++) {
			for (int j = 0; j < MAXLIGHTMAPS; j++)
			{
				int lightmapNum = LittleLong(surf->lightmapNum[j]);

				if (lightmapNum >= 0 && (lightmapNum & 1) != 0) {
					worldData->lighting.worldDeluxeMapping = qfalse;
					worldData->lighting.worldInternalDeluxeMapping = qfalse;
					break;
				}
			}
		}
		if (worldData->lighting.worldDeluxeMapping == qtrue && (!len))
			numLightmaps++;
	}

	imageSize = worldData->lighting.lightmapSize * worldData->lighting.lightmapSize * 4 * 2;


	worldData->lighting.rawLightmaps = numLightmaps;
	if (worldData->lighting.worldDeluxeMapping)
		numLightmaps >>= 1;

	if (worldData->lighting.worldInternalLightmapping)
	{
		const int targetLightmapsPerX = (int)ceilf(sqrtf(numLightmaps));

		int lightmapsPerX = 1;
		while (lightmapsPerX < targetLightmapsPerX)
			lightmapsPerX *= 2;

		worldData->lighting.lightmapsPerAtlasSide[0] = lightmapsPerX;
		worldData->lighting.lightmapsPerAtlasSide[1] = (int)ceilf((float)numLightmaps / lightmapsPerX);

		worldData->lighting.lightmapAtlasSize[0] = worldData->lighting.lightmapsPerAtlasSide[0] * LIGHTMAP_WIDTH;
		worldData->lighting.lightmapAtlasSize[1] = worldData->lighting.lightmapsPerAtlasSide[1] * LIGHTMAP_HEIGHT;

		// FIXME: What happens if we need more?
		worldData->lighting.numLightmaps = 1;
	}
	else
	{
		worldData->lighting.numLightmaps = numLightmaps;
	}

	const int baseCount = worldData == &s_worldData ? 0 : tr.numLightmaps;
	if (worldData->lighting.numLightmaps > JAR_BSP_LIGHTMAP_LIMIT - baseCount)
		ri.Error(ERR_DROP, "BSP lightmap registry capacity exceeded");
	if (worldData->lighting.lightmapAtlasSize[0] > glConfig.maxTextureSize || worldData->lighting.lightmapAtlasSize[1] > glConfig.maxTextureSize)
		ri.Error(ERR_DROP, "BSP lightmap atlas exceeds GPU texture size");

	image = (byte *)Z_Malloc(imageSize, TAG_BSP, qfalse);
	worldData->lighting.lightmaps = (image_t **)Hunk_Alloc( worldData->lighting.numLightmaps * sizeof(image_t *), h_low );

	if (worldData->lighting.worldDeluxeMapping)
	{
		worldData->lighting.deluxemaps = (image_t **)Hunk_Alloc( worldData->lighting.numLightmaps * sizeof(image_t *), h_low );
	}

	if (hdr_capable)
		textureInternalFormat = GL_RGBA16F;
	else
		textureInternalFormat = GL_RGBA8;

	if (worldData->lighting.worldInternalLightmapping)
	{
		for (i = 0; i < worldData->lighting.numLightmaps; i++)
		{
			worldData->lighting.lightmaps[i] = R_CreateImage(
				va("_lightmapatlas_jar_%d_%d", worldData->lightmapNamespace, i),
				NULL,
				worldData->lighting.lightmapAtlasSize[0],
				worldData->lighting.lightmapAtlasSize[1],
				IMGTYPE_COLORALPHA,
				IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
				textureInternalFormat);

			if (worldData->lighting.worldDeluxeMapping)
			{
				worldData->lighting.deluxemaps[i] = R_CreateImage(
					va("_fatdeluxemap_jar_%d_%d", worldData->lightmapNamespace, i),
					NULL,
					worldData->lighting.lightmapAtlasSize[0],
					worldData->lighting.lightmapAtlasSize[1],
					IMGTYPE_DELUXE,
					IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
					0);
			}
		}
	}

	for (i = 0; i < numLightmaps; i++)
	{
		int xoff = 0, yoff = 0;
		int lightmapnum = i;
		// expand the 24 bit on-disk to 32 bit

		if (worldData->lighting.worldInternalLightmapping)
		{
			xoff = (i % worldData->lighting.lightmapsPerAtlasSide[0]) * worldData->lighting.lightmapSize;
			yoff = (i / worldData->lighting.lightmapsPerAtlasSide[0]) * worldData->lighting.lightmapSize;
			lightmapnum = 0;
		}

		// if (tr.worldLightmapping)
		{
			char filename[MAX_QPATH];
			byte *externalLightmap = NULL;
			float *hdrL = NULL;
			int lightmapWidth = worldData->lighting.lightmapSize;
			int lightmapHeight = worldData->lighting.lightmapSize;
			int bppc;
			bool foundLightmap = true;

			if (!worldData->lighting.worldInternalLightmapping)
			{
				bppc = 16;
				if (hdr_capable) {
					Com_sprintf(filename,sizeof(filename),"%s/lm_%04d.hdr",mapDirectory,i*(worldData->lighting.worldDeluxeMapping?2:1));
					R_LoadHDRImage(filename,&externalLightmap,&lightmapWidth,&lightmapHeight);
				}
				if (!externalLightmap) {
					bppc = 8;
					Com_sprintf(filename,sizeof(filename),"%s/lm_%04d.tga",mapDirectory,i*(worldData->lighting.worldDeluxeMapping?2:1));
					R_LoadImage(filename,&externalLightmap,&lightmapWidth,&lightmapHeight);
				}
			}

			if (externalLightmap)
			{
				if (lightmapWidth<1 || lightmapHeight<1 || lightmapWidth>glConfig.maxTextureSize || lightmapHeight>glConfig.maxTextureSize || lightmapWidth>INT_MAX/8/lightmapHeight)
					ri.Error(ERR_DROP,"Invalid external lightmap size");
				int newImageSize = lightmapWidth * lightmapHeight * 4 * 2;
				if (worldData->lighting.worldInternalLightmapping && (lightmapWidth != worldData->lighting.lightmapSize || lightmapHeight != worldData->lighting.lightmapSize))
				{
					ri.Printf(PRINT_ALL, "Error loading %s: non %dx%d lightmaps\n", filename, worldData->lighting.lightmapSize, worldData->lighting.lightmapSize);
					if (bppc > 8) Hunk_FreeTempMemory(externalLightmap); else Z_Free(externalLightmap);
					externalLightmap = NULL;
					continue;
				}
				else if (newImageSize > imageSize)
				{
					Z_Free(image);
					imageSize = newImageSize;
					image = (byte *)Z_Malloc(imageSize, TAG_BSP, qfalse);
				}
				numColorComponents = 4;
			}
			if (!externalLightmap)
			{
				lightmapWidth = worldData->lighting.lightmapSize;
				lightmapHeight = worldData->lighting.lightmapSize;
				numColorComponents = 3;
			}

			foundLightmap = true;
			if (externalLightmap)
			{
				if (bppc > 8)
				{
					hdrL = (float *)externalLightmap;
					worldData->lighting.hdrLighting = qtrue;
				}
				else
				{
					buf_p = externalLightmap;
				}
			}
			else if (buf)
			{
				if (worldData->lighting.worldDeluxeMapping)
					buf_p = buf + (i * 2) * worldData->lighting.lightmapSize * worldData->lighting.lightmapSize * 3;
				else
					buf_p = buf + i * worldData->lighting.lightmapSize * worldData->lighting.lightmapSize * 3;
			}
			else
			{
				buf_p = NULL;
				foundLightmap = false;
			}

			if (foundLightmap)
			{
				for (j = 0; j < lightmapWidth * lightmapHeight; j++)
				{
					if (hdrL && hdr_capable)
					{
						vec4_t color;
						memcpy(color, &hdrL[j * 3], 12);

						color[3] = 1.0f;

						R_ColorShiftLightingFloats(color, color, 1.0f, false);

						ColorToRGBA16F(color, (uint16_t *)(&image[j * 8]));
					}
					else if (buf_p && hdr_capable)
					{
						vec4_t color;

						//hack: convert LDR lightmap to HDR one
						color[0] = MAX(buf_p[j*numColorComponents + 0], 0.499f);
						color[1] = MAX(buf_p[j*numColorComponents + 1], 0.499f);
						color[2] = MAX(buf_p[j*numColorComponents + 2], 0.499f);

						// if under an arbitrary value (say 12) grey it out
						// this prevents weird splotches in dimly lit areas
						if (color[0] + color[1] + color[2] < 12.0f)
						{
							float avg = (color[0] + color[1] + color[2]) * 0.3333f;
							color[0] = avg;
							color[1] = avg;
							color[2] = avg;
						}
						color[3] = 1.0f;

						R_ColorShiftLightingFloats(color, color, 1.0f / 255.0f, true);

						ColorToRGBA16F(color, (unsigned short *)(&image[j * 8]));
					}
					else if (buf_p)
					{
						if (r_lightmap->integer == 2)
						{	// color code by intensity as development tool	(FIXME: check range)
							float r = buf_p[j*numColorComponents + 0];
							float g = buf_p[j*numColorComponents + 1];
							float b = buf_p[j*numColorComponents + 2];
							float intensity;
							float out[3] = { 0.0, 0.0, 0.0 };

							intensity = 0.33f * r + 0.685f * g + 0.063f * b;

							if (intensity > 255)
								intensity = 1.0f;
							else
								intensity /= 255.0f;

							if (intensity > maxIntensity)
								maxIntensity = intensity;

							HSVtoRGB(intensity, 1.00, 0.50, out);

							image[j * 4 + 0] = out[0] * 255;
							image[j * 4 + 1] = out[1] * 255;
							image[j * 4 + 2] = out[2] * 255;
							image[j * 4 + 3] = 255;
						}
						else
						{
							byte rgba[4]={buf_p[j*numColorComponents],buf_p[j*numColorComponents+1],buf_p[j*numColorComponents+2],255};
							R_ColorShiftLightingBytes(rgba, &image[j * 4]);
							image[j * 4 + 3] = 255;
						}
					}
				}

				if (worldData->lighting.worldInternalLightmapping)
					R_UpdateSubImage(
						worldData->lighting.lightmaps[lightmapnum],
						image,
						xoff,
						yoff,
						lightmapWidth,
						lightmapHeight);
				else
					worldData->lighting.lightmaps[i] = R_CreateImage(
						va("*lightmap_jar_%d_%d", worldData->lightmapNamespace, i),
						image,
						lightmapWidth,
						lightmapHeight,
						IMGTYPE_COLORALPHA,
						IMGFLAG_NOLIGHTSCALE |
						IMGFLAG_NO_COMPRESSION |
						IMGFLAG_CLAMPTOEDGE,
						textureInternalFormat);
			}

			if (externalLightmap)
				if (bppc > 8) Hunk_FreeTempMemory(externalLightmap); else Z_Free(externalLightmap);
		}

		if (worldData->lighting.worldDeluxeMapping && buf)
		{
			buf_p = buf + (i * 2 + 1) * worldData->lighting.lightmapSize * worldData->lighting.lightmapSize * 3;

			for (j = 0; j < worldData->lighting.lightmapSize * worldData->lighting.lightmapSize; j++) {
				image[j * 4 + 0] = buf_p[j * 3 + 0];
				image[j * 4 + 1] = buf_p[j * 3 + 1];
				image[j * 4 + 2] = buf_p[j * 3 + 2];

				// make 0,0,0 into 127,127,127
				if ((image[j * 4 + 0] == 0) && (image[j * 4 + 1] == 0) && (image[j * 4 + 2] == 0))
				{
					image[j*4+0] =
					image[j*4+1] =
					image[j*4+2] = 127;
				}

				image[j * 4 + 3] = 255;
			}

			if (worldData->lighting.worldInternalLightmapping)
			{
				R_UpdateSubImage(
					worldData->lighting.deluxemaps[lightmapnum],
					image,
					xoff,
					yoff,
					worldData->lighting.lightmapSize,
					worldData->lighting.lightmapSize);
			}
			else
			{
				worldData->lighting.deluxemaps[i] = R_CreateImage(
					va("*deluxemap_jar_%d_%d", worldData->lightmapNamespace, i),
					image,
					worldData->lighting.lightmapSize,
					worldData->lighting.lightmapSize,
					IMGTYPE_DELUXE,
					IMGFLAG_NOLIGHTSCALE |
					IMGFLAG_NO_COMPRESSION |
					IMGFLAG_CLAMPTOEDGE,
					0);
			}
		}
		else if (r_deluxeMapping->integer)
		{
			char filename[MAX_QPATH];
			byte *externalLightmap = NULL;
			int lightmapWidth = worldData->lighting.lightmapSize;
			int lightmapHeight = worldData->lighting.lightmapSize;

			// try loading additional deluxemaps
			if (worldData->lighting.worldDeluxeMapping)
				Com_sprintf(filename, sizeof(filename), "%s/lm_%04d.tga", mapDirectory, i * 2 + 1);
			else
				Com_sprintf(filename, sizeof(filename), "%s/dm_%04d.tga", mapDirectory, i);

			R_LoadImage(filename, &externalLightmap, &lightmapWidth, &lightmapHeight);
			if (!externalLightmap)
				continue;

			if (worldData->lighting.worldInternalLightmapping && (lightmapWidth != worldData->lighting.lightmapSize || lightmapHeight != worldData->lighting.lightmapSize))
			{
				ri.Printf(PRINT_ALL, "Error loading %s: non %dx%d deluxemaps\n", filename, worldData->lighting.lightmapSize, worldData->lighting.lightmapSize);
				Z_Free(externalLightmap);
				externalLightmap = NULL;
				continue;
			}

			if (lightmapWidth<1 || lightmapHeight<1 || lightmapWidth>glConfig.maxTextureSize || lightmapHeight>glConfig.maxTextureSize || lightmapWidth>INT_MAX/8/lightmapHeight)
				ri.Error(ERR_DROP,"Invalid external deluxemap size");
			int newImageSize = lightmapWidth * lightmapHeight * 4 * 2;
			if (newImageSize > imageSize)
			{
				Z_Free(image);
				imageSize = newImageSize;
				image = (byte *)Z_Malloc(imageSize, TAG_BSP, qfalse);
			}

			buf_p = externalLightmap;

			for (j = 0; j < lightmapWidth * lightmapHeight; j++) {
				image[j * 4 + 0] = buf_p[j * 4 + 0];
				image[j * 4 + 1] = buf_p[j * 4 + 1];
				image[j * 4 + 2] = buf_p[j * 4 + 2];

				// make 0,0,0 into 127,127,127
				if ((image[j * 4 + 0] == 0) && (image[j * 4 + 1] == 0) && (image[j * 4 + 2] == 0))
				{
					image[j * 4 + 0] =
					image[j * 4 + 1] =
					image[j * 4 + 2] = 127;
				}

				image[j * 4 + 3] = 255;
			}

			if (!worldData->lighting.deluxemaps)
			{
				worldData->lighting.deluxemaps = (image_t **)Hunk_Alloc(worldData->lighting.numLightmaps * sizeof(image_t *), h_low);
				if (worldData->lighting.worldInternalLightmapping)
				{
					const int pixels=worldData->lighting.lightmapAtlasSize[0]*worldData->lighting.lightmapAtlasSize[1];
					byte *neutral=(byte *)Z_Malloc(pixels*4,TAG_BSP,qfalse);
					for(int p=0;p<pixels;++p){neutral[p*4]=neutral[p*4+1]=neutral[p*4+2]=127;neutral[p*4+3]=255;}
					worldData->lighting.deluxemaps[lightmapnum] = R_CreateImage(
						va("_fatdeluxemap_jar_%d_%d", worldData->lightmapNamespace, i),
						neutral,
						worldData->lighting.lightmapAtlasSize[0],
						worldData->lighting.lightmapAtlasSize[1],
						IMGTYPE_DELUXE,
						IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
						0);
					Z_Free(neutral);
				}
			}

			if (worldData->lighting.worldInternalLightmapping)
			{
				R_UpdateSubImage(
					worldData->lighting.deluxemaps[lightmapnum],
					image,
					xoff,
					yoff,
					lightmapWidth,
					lightmapHeight);
			}
			else
			{
				worldData->lighting.deluxemaps[i] = R_CreateImage(
					va("*deluxemap_jar_%d_%d", worldData->lightmapNamespace, i),
					image,
					lightmapWidth,
					lightmapHeight,
					IMGTYPE_DELUXE,
					IMGFLAG_NOLIGHTSCALE |
					IMGFLAG_NO_COMPRESSION |
					IMGFLAG_CLAMPTOEDGE,
					0);
			}

			Z_Free(externalLightmap);
			externalLightmap = NULL;
		}
	}

	if ( r_lightmap->integer == 2 )	{
		ri.Printf( PRINT_ALL, "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}

	Z_Free(image);

	if (worldData->lighting.deluxemaps)
		worldData->lighting.worldDeluxeMapping = qtrue;
	R_PublishBSPLightmaps(worldData);
	}


static float FatPackU(const world_t *worldData, float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (worldData->lighting.worldInternalDeluxeMapping)
		lightmapnum >>= 1;

	if (worldData->lighting.lightmapAtlasSize[0] > 0)
	{
		const int lightmapXOffset = lightmapnum % worldData->lighting.lightmapsPerAtlasSide[0];
		const float invLightmapSide = 1.0f / worldData->lighting.lightmapsPerAtlasSide[0];

		return (lightmapXOffset * invLightmapSide) + (input * invLightmapSide);
	}

	return input;
}

static float FatPackV(const world_t *worldData, float input, int lightmapnum)
{
	if (lightmapnum < 0)
		return input;

	if (worldData->lighting.worldInternalDeluxeMapping)
		lightmapnum >>= 1;

	if (worldData->lighting.lightmapAtlasSize[1] > 0)
	{
		const int lightmapYOffset = lightmapnum / worldData->lighting.lightmapsPerAtlasSide[0];
		const float invLightmapSide = 1.0f / worldData->lighting.lightmapsPerAtlasSide[1];

		return (lightmapYOffset * invLightmapSide) + (input * invLightmapSide);
	}

	return input;
}


static int FatLightmap(const world_t *worldData, int lightmapnum)
{
	if (lightmapnum < 0)
		return lightmapnum;

	if (lightmapnum >= worldData->lighting.rawLightmaps) return LIGHTMAP_BY_VERTEX;

	if (worldData->lighting.worldInternalDeluxeMapping)
		lightmapnum >>= 1;

	if (worldData->lighting.lightmapAtlasSize[0] > 0)
		return worldData->lighting.firstLightmap;

	return worldData->lighting.firstLightmap + lightmapnum;
}

