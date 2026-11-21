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
// Native styled/HDR sampling shared by all JA:R renderers.
#include "tr_bsp_grid_context.inc"

static bool R_SampleBSPGrid(trRefEntity_t *entity, world_t *world, const vec3_t point,
    const bspGridInstance_t *instance)
{
    int indices[8]; float factors[8];
    if (!R_BSPGridCorners(world, point, indices, factors)) return false;
    vec3_t direction{};
    VectorClear(entity->ambientLight);
    VectorClear(entity->directedLight);
    float total = 0;
    for (int corner = 0; corner < 8; ++corner)
    {
        const int index = indices[corner];
        const float factor = factors[corner];
        const mgrid_t &data = world->lightGridData[world->lightGridArray[index]];
        if (factor <= 0 || data.styles[0] == LS_LSNONE) continue;
        total += factor;
#ifdef JAR_GRID_REND2
        if (world->hdrLightGrid)
        {
            const float *hdr = world->hdrLightGrid + index * 6;
            for (int channel = 0; channel < 3; ++channel)
            {
                entity->ambientLight[channel] += factor * hdr[channel] * 255;
                entity->directedLight[channel] += factor * hdr[channel + 3] * 255;
            }
        }
        else
#endif
        for (int style = 0; style < MAXLIGHTMAPS && data.styles[style] != LS_LSNONE; ++style)
        {
            if (data.styles[style] >= MAX_LIGHT_STYLES) return false;
            for (int channel = 0; channel < 3; ++channel)
            {
                const float color = factor * styleColors[data.styles[style]][channel] / 255.0f;
                entity->ambientLight[channel] += color * data.ambientLight[style][channel];
                entity->directedLight[channel] += color * data.directLight[style][channel];
            }
        }
        const int latitude = data.latLong[1] * (FUNCTABLE_SIZE / 256);
        const int longitude = data.latLong[0] * (FUNCTABLE_SIZE / 256);
        vec3_t normal = {
            tr.sinTable[(latitude + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK] * tr.sinTable[longitude],
            tr.sinTable[latitude] * tr.sinTable[longitude],
            tr.sinTable[(longitude + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK]
        };
        VectorMA(direction, factor, normal, direction);
#ifdef JAR_CLASSIC_SP
        if (r_debugLight->integer && entity->e.hModel == -1)
        {
            vec3_t gridPoint, worldNormal;
            const int x = index % world->lightGridBounds[0];
            const int y = (index / world->lightGridBounds[0]) % world->lightGridBounds[1];
            const int z = index / (world->lightGridBounds[0] * world->lightGridBounds[1]);
            const int cell[3] = {x,y,z};
            for (int axis = 0; axis < 3; ++axis) gridPoint[axis] = world->lightGridOrigin[axis] + cell[axis] * world->lightGridSize[axis];
            VectorCopy(normal, worldNormal);
            if (instance)
            {
                vec3_t local; VectorCopy(gridPoint, local); VectorCopy(instance->origin, gridPoint); VectorClear(worldNormal);
                for (int axis = 0; axis < 3; ++axis)
                { VectorMA(gridPoint, local[axis], instance->axis[axis], gridPoint); VectorMA(worldNormal, normal[axis], instance->axis[axis], worldNormal); }
                VectorNormalize(worldNormal);
            }
            refEntity_t debug{};
            VectorCopy(gridPoint, debug.origin); vectoangles(worldNormal, debug.angles); AnglesToAxis(debug.angles, debug.axis);
            debug.reType = RT_MODEL; RE_AddRefEntityToScene(&debug);
            debug.renderfx = RF_DEPTHHACK; debug.reType = RT_SPRITE; debug.customShader = RE_RegisterShader("gfx/misc/debugAmbient");
            for (int channel = 0; channel < 3; ++channel) debug.shaderRGBA[channel] = data.ambientLight[0][channel];
            debug.shaderRGBA[3] = 255; debug.radius = factor * 50 + 2; RE_AddRefEntityToScene(&debug);
            debug.reType = RT_LINE; debug.customShader = RE_RegisterShader("gfx/misc/debugArrow");
            for (int channel = 0; channel < 3; ++channel) debug.shaderRGBA[channel] = data.directLight[0][channel];
            VectorCopy(gridPoint, debug.oldorigin); VectorMA(gridPoint, factor * -255 - 2, worldNormal, debug.origin);
            debug.radius = 1.5f; RE_AddRefEntityToScene(&debug);
        }
#endif
    }
    if (total <= 0) return false; // No usable samples inside the map section.
    VectorScale(entity->ambientLight, r_ambientScale->value / total, entity->ambientLight);
    VectorScale(entity->directedLight, r_directedScale->value / total, entity->directedLight);
    if (instance)
    {
        vec3_t local; VectorCopy(direction, local); VectorClear(direction);
        for (int axis = 0; axis < 3; ++axis) VectorMA(direction, local[axis], instance->axis[axis], direction);
    }
    VectorNormalize2(direction, entity->lightDir);
    return true;
}

static bool R_SetupEntityLightingGrid(trRefEntity_t *entity, world_t *explicitWorld = nullptr)
{
    bool fullbright = r_fullbright->integer != 0;
#ifdef JAR_CLASSIC_SP
    fullbright = fullbright || (tr.refdef.rdflags & RDF_doLAGoggles);
#elif defined(JAR_GRID_REND2)
    fullbright = fullbright || tr.refdef.doLAGoggles;
#endif
    if (fullbright)
    {
        VectorSet(entity->ambientLight,255,255,255); VectorSet(entity->directedLight,255,255,255);
        VectorCopy(tr.sunDirection,entity->lightDir); return true;
    }
    vec3_t point;
    if (entity->e.renderfx & RF_LIGHTING_ORIGIN) VectorCopy(entity->e.lightingOrigin,point);
    else VectorCopy(entity->e.origin,point);
    if (explicitWorld) return R_SampleBSPGrid(entity,explicitWorld,point,nullptr);
    vec3_t local;
    const bspGridInstance_t *instance = R_BSPGridAtPoint(point,local);
    if (instance && R_SampleBSPGrid(entity,instance->world,local,instance)) return true;
    return R_SampleBSPGrid(entity,tr.world,point,nullptr);
}
