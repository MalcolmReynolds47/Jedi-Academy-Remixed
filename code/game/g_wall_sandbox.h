// Native SP hull and activation for a movable Sandbox wall.
#pragma once
#include "../../shared/qcommon/jar_wall.h"

inline bool G_WallPortable(const gentity_t& wall)
{
    return !wall.bmodel && JAR_WallPortable(wall.classname, wall.model);
}

inline void G_WallHull(const vec3_t angles, vec3_t mins, vec3_t maxs)
{
    // Verified against the only stock cell_wall model frame.
    vec3_t axis[3];
    AnglesToAxis(angles, axis);
    for (int a = 0; a < 3; ++a) {
        maxs[a] = fabsf(axis[0][a])*32 + fabsf(axis[1][a])*64 + fabsf(axis[2][a])*64;
        mins[a] = -maxs[a];
    }
}

inline void G_WallVisibleBounds(const gentity_t& wall, const vec3_t angles, vec3_t mins, vec3_t maxs)
{
    if (!strcmp(wall.model, JAR_WALL_PORTABLE_MODEL)) G_WallHull(angles, mins, maxs);
    else { VectorSet(mins,-21,-21,0); VectorSet(maxs,21,21,22); }
}

inline bool G_WallClear(const gentity_t& wall, const vec3_t origin, const vec3_t angles)
{
    vec3_t mins,maxs;
    G_WallVisibleBounds(wall, angles, mins, maxs);
    trace_t trace{};
    gi.trace(&trace, origin, mins, maxs, origin, wall.s.number,
             MASK_SOLID | CONTENTS_BODY, (EG2_Collision)0, 0);
    return !trace.startsolid && !trace.allsolid;
}

inline void G_WallRefresh(gentity_t& wall)
{
    if (wall.count) G_WallVisibleBounds(wall, wall.currentAngles, wall.mins, wall.maxs);
    else { VectorSet(wall.mins,-16,-16,-16); VectorSet(wall.maxs,16,16,16); }
    wall.contents = wall.count ? CONTENTS_SOLID : CONTENTS_ITEM;
    wall.svFlags &= ~SVF_NOCLIENT;
    wall.s.eFlags &= ~(EF_NODRAW | EF_ANIM_ALLFAST);
}

inline bool G_WallSetVisible(gentity_t& wall, bool visible)
{
    if (!G_WallPortable(wall)) return false;
    if (visible && !wall.count && !G_WallClear(wall, wall.currentOrigin, wall.currentAngles)) return false;
    wall.count = visible ? 1 : 0;
    G_WallRefresh(wall);
    gi.linkentity(&wall);
    return true;
}
