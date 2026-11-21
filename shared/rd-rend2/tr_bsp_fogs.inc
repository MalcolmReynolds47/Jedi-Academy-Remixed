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

// Rend2 fog loader. GPL-2.0-only; native BSP/shader conventions retained.
static int R_BSPFogIndex(int diskIndex, const world_t &world)
{
    const int fileCount = world.numfogs - 1 - (world.inheritedFog > 0 ? 1 : 0);
    if (diskIndex < -1 || diskIndex >= fileCount)
        Com_Error(ERR_DROP, "LoadMap: fog index out of range in %s", world.name);
    if (diskIndex == -1 && world.lightmapNamespace && world.globalFogIndex > 0)
        return world.globalFogIndex;
    return diskIndex + 1;
}

static void R_LoadFogs(world_t *worldData, lump_t *l, lump_t *brushesLump, lump_t *sidesLump)
{
    world_t &world = *worldData;
    const int bspIndex = world.lightmapNamespace;
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
    world.globalFogIndex = world.inheritedFog = -1;
    for (int i = 0; i < count; ++i)
    {
        if (LittleLong(fogs[i].brushNum) != -1) continue;
        if (world.globalFogIndex != -1)
            Com_Error(ERR_DROP, "LoadMap: multiple global fogs in %s", world.name);
        world.globalFogIndex = i + 1;
    }
    if (bspIndex && world.globalFogIndex == -1 && tr.world && tr.world->globalFogIndex > 0)
        world.globalFogIndex = world.inheritedFog = count + 1;
    world.numfogs = count + 1 + (world.inheritedFog > 0 ? 1 : 0);
    // Leave one GPU slot for the optional night-vision overlay.
    if (world.numfogs > MAX_GPU_FOGS)
        Com_Error(ERR_DROP, "LoadMap: fogs exceed GPU constant capacity in %s", world.name);
    world.fogs = static_cast<fog_t *>(Hunk_Alloc((world.numfogs + 1) * sizeof(fog_t), h_low));
    memset(world.fogs, 0, (world.numfogs + 1) * sizeof(fog_t));
    if (world.inheritedFog > 0)
        world.fogs[world.inheritedFog] = tr.world->fogs[tr.world->globalFogIndex];

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
        const shader_t *shader = R_FindShader(source.shader, lightmapsNone, stylesDefault, qtrue);
        out.parms = shader->fogParms;
        if (shader->defaultShader || !std::isfinite(out.parms.depthForOpaque) || out.parms.depthForOpaque <= 0.0f)
            out.parms.depthForOpaque = 65535.0f;
        VectorSet4(out.color, out.parms.color[0] * tr.identityLight,
            out.parms.color[1] * tr.identityLight, out.parms.color[2] * tr.identityLight, 1.0f);
        out.tcScale = 1.0f / (out.parms.depthForOpaque < 1.0f ? 1.0f : out.parms.depthForOpaque);
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
    world.globalFog = world.globalFogIndex > 0 ? &world.fogs[world.globalFogIndex] : nullptr;
    fog_t &special = world.fogs[world.numfogs];
    special.originalBrushNumber = -1;
    VectorSet(special.bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
    VectorSet(special.bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
}
