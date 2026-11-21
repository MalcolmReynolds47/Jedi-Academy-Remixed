// Coordinate frames for native translating sub-BSP movers. GPL-2.0-only.
#ifndef G_SUBBSP_MOVERS_H
#define G_SUBBSP_MOVERS_H
#include "g_subbsp_ownership.h"
#include "qcommon/jar_volume.h"
#include "qcommon/jar_angular_trajectory.h"

namespace jar { namespace subBSPMover {
inline const gentity_t *Root(const gentity_t &e)
{
    if (!e.bmodel || !e.model || e.model[0] != '*') return nullptr;
    const auto *root = G_SubBSPOwner(&e);
    return root && root != &e ? root : nullptr;
}

inline void Vector(const gentity_t &e, const vec3_t local, vec3_t world)
{
    const auto *root = Root(e);
    if (!root) { VectorCopy(local, world); return; }
    vec3_t axis[3], result{}; AnglesToAxis(root->currentAngles, axis);
    for (int i = 0; i < 3; ++i) VectorMA(result, local[i], axis[i], result);
    VectorCopy(result, world);
}

inline float Distance(const gentity_t &e, const vec3_t worldDirection, float lip)
{
    vec3_t local, size; VectorCopy(worldDirection, local);
    if (const auto *root = Root(e))
    {
        vec3_t axis[3]; AnglesToAxis(root->currentAngles, axis);
        for (int i = 0; i < 3; ++i) local[i] = DotProduct(worldDirection, axis[i]);
    }
    VectorSubtract(e.maxs, e.mins, size);
    return fabs(local[0])*size[0] + fabs(local[1])*size[1] + fabs(local[2])*size[2] - lip;
}

inline void MoveDirection(gentity_t &e)
{
    // Transformed keys are ordinary Euler angles. A world yaw of -1/-2 must
    // not be reinterpreted as the editor's untransformed up/down sentinel.
    if (Root(e))
    {
        AngleVectors(e.s.angles, e.movedir, nullptr, nullptr);
        VectorClear(e.s.angles);
    }
    else G_SetMovedir(e.s.angles, e.movedir);
}

inline void Orient(gentity_t &e)
{
    if (const auto *root = Root(e))
    {
        // Door/button angles encode travel, not a rotation of their brush.
        VectorCopy(root->currentAngles, e.s.angles);
        VectorCopy(e.s.angles, e.currentAngles);
        VectorCopy(e.s.angles, e.s.apos.trBase);
    }
}

inline void Angular(gentity_t &e)
{
    if (const auto *root = Root(e))
    {
        vec3_t local;
        G_SpawnVector("_jar_localAngles", "0 0 0", local);
        const auto type = e.s.apos.trType == TR_SINE ? TR_AXIS_SINE :
            e.s.apos.trType == TR_LINEAR ? TR_AXIS_LINEAR : TR_AXIS_STATIONARY;
        jar::angular::Convert(e.s.apos, root->currentAngles, local, type);
        if (type != TR_AXIS_SINE) e.s.apos.trTime = level.time;
        jar::angular::Evaluate(e.s.apos, level.time, e.currentAngles);
    }
}

inline void Trigger(gentity_t &trigger, const gentity_t &mover, const vec3_t lo, const vec3_t hi)
{
    // Native saved fields retain the oriented volume after a full save/load.
    // The linked AABB is broad phase only; touch callbacks use all 15 SAT axes.
    trigger.model = "@jar_bsp_trigger";
    trigger.svFlags |= SVF_NOCLIENT;
    VectorCopy(mover.currentOrigin, trigger.currentOrigin);
    VectorCopy(trigger.currentOrigin, trigger.s.origin);
    VectorCopy(trigger.currentOrigin, trigger.s.pos.trBase);
    VectorCopy(mover.currentAngles, trigger.currentAngles);
    VectorCopy(trigger.currentAngles, trigger.s.angles);
    VectorCopy(trigger.currentAngles, trigger.s.apos.trBase);
    VectorCopy(lo, trigger.pos1); VectorCopy(hi, trigger.pos2);
    volume::Hull(lo, hi, trigger.currentAngles, trigger.mins, trigger.maxs);
}

inline bool Touches(const gentity_t &trigger, const gentity_t &other)
{
    if (!trigger.model || strcmp(trigger.model, "@jar_bsp_trigger")) return true;
    vec3_t axes[3], center, half, otherHalf, delta;
    AnglesToAxis(trigger.currentAngles, axes);
    VectorCopy(trigger.currentOrigin, center);
    for (int i = 0; i < 3; ++i)
    {
        half[i] = (trigger.pos2[i] - trigger.pos1[i])*.5f;
        VectorMA(center, (trigger.pos1[i] + trigger.pos2[i])*.5f, axes[i], center);
        otherHalf[i] = (other.maxs[i] - other.mins[i])*.5f;
    }
    for (int i = 0; i < 3; ++i)
        delta[i] = other.currentOrigin[i] + (other.mins[i] + other.maxs[i])*.5f - center[i];
    const auto separated = [&](const vec3_t axis) {
        float radius = 0;
        for (int i = 0; i < 3; ++i)
            radius += fabs(DotProduct(axes[i], axis))*half[i] + fabs(axis[i])*otherHalf[i];
        return fabs(DotProduct(delta, axis)) > radius + .001f;
    };
    for (int i = 0; i < 3; ++i)
    {
        vec3_t world{}; world[i] = 1;
        if (separated(world) || separated(axes[i])) return false;
        for (int j = 0; j < 3; ++j)
        {
            vec3_t cross; CrossProduct(axes[j], world, cross);
            if (separated(cross)) return false;
        }
    }
    return true;
}

inline bool DoorBounds(const gentity_t &leader, vec3_t lo, vec3_t hi)
{
    if (!Root(leader)) return false;
    vec3_t basis[3]; AnglesToAxis(leader.currentAngles, basis);
    ClearBounds(lo, hi);
    for (const auto *part = &leader; part; part = part->teamchain)
    {
        vec3_t axes[3]; AnglesToAxis(part->currentAngles, axes);
        for (int mask = 0; mask < 8; ++mask)
        {
            vec3_t world, local;
            VectorSubtract(part->currentOrigin, leader.currentOrigin, world);
            for (int i = 0; i < 3; ++i)
                VectorMA(world, mask & (1 << i) ? part->maxs[i] : part->mins[i], axes[i], world);
            for (int i = 0; i < 3; ++i) local[i] = DotProduct(world, basis[i]);
            AddPointToBounds(local, lo, hi);
        }
    }
    // Same link tolerance as stock axis-aligned doors, in the section frame.
    for (int i = 0; i < 3; ++i) { lo[i] -= 1; hi[i] += 1; }
    return true;
}
} }
#endif
