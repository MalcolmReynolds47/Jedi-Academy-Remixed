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

// Shared classic fog loader. GPL-2.0-only; native BSP/shader conventions retained.
static int R_BSPFogIndex(int diskIndex, const world_t &world, int bspIndex)
{
    const int fileCount = world.numfogs - 1 - (world.inheritedFog > 0 ? 1 : 0);
    if (diskIndex < -1 || diskIndex >= fileCount)
        Com_Error(ERR_DROP, "LoadMap: fog index out of range in %s", world.name);
    if (diskIndex == -1 && bspIndex && world.globalFog > 0)
        return world.globalFog;
    return diskIndex + 1;
}

static void R_LoadFogs(lump_t *l, lump_t *brushesLump, lump_t *sidesLump,
    world_t &world, int bspIndex)
{
    if (l->filelen < 0 || l->filelen % sizeof(dfog_t) ||
        brushesLump->filelen < 0 || brushesLump->filelen % sizeof(dbrush_t) ||
        sidesLump->filelen < 0 || sidesLump->filelen % sizeof(dbrushside_t))
        Com_Error(ERR_DROP, "LoadMap: malformed fog lumps in %s", world.name);
    const int count = l->filelen / sizeof(dfog_t);
    const auto *fogs = reinterpret_cast<const dfog_t *>(fileBase + l->fileofs);
    const auto *brushes = reinterpret_cast<const dbrush_t *>(fileBase + brushesLump->fileofs);
    const auto *sides = reinterpret_cast<const dbrushside_t *>(fileBase + sidesLump->fileofs);
    const int brushCount = brushesLump->filelen / sizeof(dbrush_t);
    const int sideCount = sidesLump->filelen / sizeof(dbrushside_t);
    world.globalFog = world.inheritedFog = -1;
    for (int i = 0; i < count; ++i)
    {
        if (LittleLong(fogs[i].brushNum) != -1) continue;
        if (world.globalFog != -1)
            Com_Error(ERR_DROP, "LoadMap: multiple global fogs in %s", world.name);
        world.globalFog = i + 1;
    }
    if (bspIndex && world.globalFog == -1 && tr.world && tr.world->globalFog > 0)
        world.globalFog = world.inheritedFog = count + 1;
    world.numfogs = count + 1 + (world.inheritedFog > 0 ? 1 : 0);
    // Sort keys have five fog bits. Reserve the special/nightvision slot too.
    if (world.numfogs > 31)
        Com_Error(ERR_DROP, "LoadMap: fogs exceed classic sort capacity in %s", world.name);
#ifdef JAR_CLASSIC_SP
    world.fogs = static_cast<fog_t *>(R_Hunk_Alloc((world.numfogs + 1) * sizeof(fog_t), qtrue));
#else
    world.fogs = static_cast<fog_t *>(Hunk_Alloc((world.numfogs + 1) * sizeof(fog_t), h_low));
#endif
    memset(world.fogs, 0, (world.numfogs + 1) * sizeof(fog_t));
    if (world.inheritedFog > 0)
        world.fogs[world.inheritedFog] = tr.world->fogs[tr.world->globalFog];

    for (int i = 0; i < count; ++i)
    {
        const dfog_t &source = fogs[i];
        fog_t &out = world.fogs[i + 1];
        out.originalBrushNumber = LittleLong(source.brushNum);
        int firstSide = 0, numSides = 0;
        if (out.originalBrushNumber == -1)
        {
            VectorSet(out.bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
            VectorSet(out.bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
        }
        else
        {
            if (out.originalBrushNumber < 0 || out.originalBrushNumber >= brushCount)
                Com_Error(ERR_DROP, "LoadMap: fog brush out of range in %s", world.name);
            const dbrush_t &brush = brushes[out.originalBrushNumber];
            firstSide = LittleLong(brush.firstSide);
            numSides = LittleLong(brush.numSides);
            if (firstSide < 0 || numSides < 6 || numSides > sideCount || firstSide > sideCount - numSides)
                Com_Error(ERR_DROP, "LoadMap: fog brush sides out of range in %s", world.name);
            for (int j = 0; j < 6; ++j)
            {
                const int plane = LittleLong(sides[firstSide + j].planeNum);
                if (plane < 0 || plane >= world.numplanes)
                    Com_Error(ERR_DROP, "LoadMap: fog plane out of range in %s", world.name);
                out.bounds[j & 1][j / 2] = world.planes[plane].dist * ((j & 1) ? 1.0f : -1.0f);
            }
        }
        if (!memchr(source.shader, 0, sizeof(source.shader)))
            Com_Error(ERR_DROP, "LoadMap: unterminated fog shader in %s", world.name);
        int lightmaps[MAXLIGHTMAPS] = { LIGHTMAP_NONE };
        shader_t *shader = R_FindShader(source.shader, lightmaps, stylesDefault, qtrue);
        if (shader->fogParms) out.parms = *shader->fogParms;
        else
        {
            VectorSet(out.parms.color, 1.0f, 0.0f, 0.0f);
            out.parms.depthForOpaque = 250.0f;
        }
#ifdef JAR_CLASSIC_SP
        const float colorScale = 1.0f;
#else
        const float colorScale = tr.identityLight;
#endif
        out.colorInt = ColorBytes4(out.parms.color[0] * colorScale,
            out.parms.color[1] * colorScale, out.parms.color[2] * colorScale, 1.0f);
        out.tcScale = 1.0f / (8.0f * (out.parms.depthForOpaque < 1.0f ? 1.0f : out.parms.depthForOpaque));
        const int visibleSide = LittleLong(source.visibleSide);
        if (visibleSide != -1)
        {
            if (visibleSide < 0 || visibleSide >= numSides)
                Com_Error(ERR_DROP, "LoadMap: fog visible side out of range in %s", world.name);
            const int plane = LittleLong(sides[firstSide + visibleSide].planeNum);
            if (plane < 0 || plane >= world.numplanes)
                Com_Error(ERR_DROP, "LoadMap: fog visible plane out of range in %s", world.name);
            VectorNegate(world.planes[plane].normal, out.surface);
            out.surface[3] = -world.planes[plane].dist;
            out.hasSurface = qtrue;
        }
    }
    fog_t &special = world.fogs[world.numfogs];
    special.originalBrushNumber = -1;
    VectorSet(special.bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
    VectorSet(special.bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
}
