// Shared billboard geometry for SP and MP. Include after renderer types.
// GPL-2.0-only.
#pragma once
#include "../qcommon/jar_logic_marker.h"

static inline void JAR_LogicMarkerSprite(refEntity_t *marker, const vec3_t origin,
    const vec3_t viewOrigin, const vec3_t viewForward, qhandle_t shader)
{
    memset(marker, 0, sizeof(*marker));
    marker->reType = RT_SPRITE;
    VectorCopy(origin, marker->origin);
    marker->origin[2] += 18.0f;
    // Scale with view depth to keep distant markers readable, with a world-size cap.
    vec3_t toMarker;
    VectorSubtract(marker->origin, viewOrigin, toMarker);
    marker->radius = Com_Clamp(11.0f, 36.0f, DotProduct(toMarker, viewForward) / 33.0f);
    marker->customShader = shader;
    marker->shaderRGBA[0] = 153;
    marker->shaderRGBA[1] = 229;
    marker->shaderRGBA[2] = 255;
    marker->shaderRGBA[3] = 255;
}
