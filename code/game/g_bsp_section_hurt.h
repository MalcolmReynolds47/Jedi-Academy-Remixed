// Exact sampled contact for admitted portable stock BSP box hazards.
// GPL-2.0-only. Include after the game or server entity definition.
#ifndef G_BSP_SECTION_HURT_H
#define G_BSP_SECTION_HURT_H
#include "g_bsp_section_box.h"
#include "g_bsp_section_rift.h"

// This invisible trigger has no snapshot consumer for time2/origin2/angles2.
// Use native saved fields that are also in the server's public entity prefix.
constexpr int BSPSectionHurtTag = 0x4a534854;
constexpr int BSPSectionPadTag = 0x4a535044;
constexpr int BSPSectionRiftUnionTag = 0x4a525546;
constexpr int BSPSectionBrushPadTag = 0x4a535042;
constexpr int BSPSectionBrushHurtTag = 0x4a534248;
constexpr int BSPSectionLavaTag = 0x4a534c56;

inline bool G_BSPSectionBrushPad(const gentity_t& e){
    return e.inuse&&e.bmodel&&e.s.time2==BSPSectionBrushPadTag&&e.contents==CONTENTS_TRIGGER&&
           (e.svFlags&SVF_NOCLIENT);
}
inline bool G_BSPSectionBrushHurt(const gentity_t& e){
    return e.inuse&&e.bmodel&&(e.s.time2==BSPSectionBrushHurtTag||e.s.time2==BSPSectionLavaTag)&&
           e.contents==CONTENTS_TRIGGER&&
           (e.svFlags&SVF_NOCLIENT);
}

inline bool G_BSPSectionBoxTagged(const gentity_t& e) {
    return e.inuse && !e.bmodel && e.s.eType == ET_INVISIBLE &&
           (e.svFlags & SVF_NOCLIENT) && e.contents == CONTENTS_TRIGGER &&
           (e.s.time2 == BSPSectionHurtTag || e.s.time2 == BSPSectionPadTag ||
            e.s.time2 == BSPSectionRiftUnionTag);
}

inline bool G_BSPSectionHurtTagged(const gentity_t& e) {
    return G_BSPSectionBoxTagged(e) && e.s.time2 != BSPSectionPadTag;
}

inline bool BSPSectionBoxContact(const gentity_t& e, const vec3_t low, const vec3_t high,
                                 const vec3_t worldMins, const vec3_t worldMaxs) {
    vec3_t axes[3];
    double half[3], otherHalf[3], delta[3], localCenter[3];
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(e.currentOrigin[i]) || !std::isfinite(e.currentAngles[i]) ||
            !std::isfinite(worldMins[i]) || !std::isfinite(worldMaxs[i]) ||
            worldMins[i] > worldMaxs[i]) return false;
        localCenter[i] = (double(low[i]) + high[i]) * .5;
        half[i] = (double(high[i]) - low[i]) * .5;
        otherHalf[i] = (double(worldMaxs[i]) - worldMins[i]) * .5;
    }
    AnglesToAxis(e.currentAngles, axes);
    for (int i = 0; i < 3; ++i) {
        delta[i] = (double(worldMins[i]) + worldMaxs[i]) * .5 - e.currentOrigin[i];
        for (int j = 0; j < 3; ++j) delta[i] -= localCenter[j] * axes[j][i];
    }
    const auto separated = [&](const vec3_t axis) {
        double radius = 0, offset = 0, lengthSquared = 0;
        for (int i = 0; i < 3; ++i) {
            const double projection = double(axes[i][0])*axis[0] +
                double(axes[i][1])*axis[1] + double(axes[i][2])*axis[2];
            radius += std::fabs(projection) * half[i] + std::fabs(axis[i]) * otherHalf[i];
            offset += delta[i] * axis[i];
            lengthSquared += double(axis[i]) * axis[i];
        }
        // A zero cross axis contributes no separation. Scale the boundary
        // tolerance by axis length rather than inflating nearly parallel axes.
        return std::fabs(offset) > radius + .001 * std::sqrt(lengthSquared);
    };
    for (int i = 0; i < 3; ++i) {
        vec3_t world{};
        world[i] = 1;
        if (separated(world) || separated(axes[i])) return false;
        for (int j = 0; j < 3; ++j) {
            vec3_t cross;
            CrossProduct(axes[j], world, cross);
            if (separated(cross)) return false;
        }
    }
    return true;
}

inline bool G_BSPSectionBoxContact(const gentity_t& e, const vec3_t worldMins, const vec3_t worldMaxs) {
    if (!G_BSPSectionBoxTagged(e) || !e.linked || (e.svFlags & SVF_INACTIVE) ||
        !BSPSectionBoxValid(e.s.origin2, e.s.angles2)) return false;
    if (BSPSectionBoxContact(e, e.s.origin2, e.s.angles2, worldMins, worldMaxs)) return true;
    const auto& second = jar::rift::fallBoxes[1];
    return e.s.time2 == BSPSectionRiftUnionTag &&
        BSPSectionBoxContact(e, second.mins, second.maxs, worldMins, worldMaxs);
}

inline bool G_BSPSectionHurtContact(const gentity_t& e, const vec3_t worldMins, const vec3_t worldMaxs) {
    return G_BSPSectionHurtTagged(e) && G_BSPSectionBoxContact(e, worldMins, worldMaxs);
}

bool G_BSPSectionInitializeHurt(gentity_t&, const vec3_t, const vec3_t, bool riftUnion = false);
bool G_BSPSectionInitializePad(gentity_t&, const vec3_t, const vec3_t);
void G_BSPSectionRefreshBoxBounds(gentity_t&);
#endif
