// Native section bobbing has separate rest and paused origins. GPL-2.0-only.
#include "g_bsp_section.h"
static bool BSPSectionRebaseBobbing(gentity_t& child, const vec3_t oldOrigin,
                                    const vec3_t oldAngles, const vec3_t newOrigin,
                                    const vec3_t newAngles) {
    if (!jar::bspSection::Bobbing(child)) return false;

    const bool rotate = !VectorCompare(oldAngles, newAngles);
    auto movePoint = [&](vec3_t p) {
        vec3_t offset;
        if (rotate) {
            VectorSubtract(p, oldOrigin, offset);
            jar::angular::RebaseVector(offset, oldAngles, newAngles, offset);
            VectorAdd(newOrigin, offset, p);
        } else {
            VectorSubtract(newOrigin, oldOrigin, offset);
            VectorAdd(p, offset, p);
        }
    };

    movePoint(child.s.origin);      // Original rest point restored on USE.
    movePoint(child.s.pos.trBase);  // Rest while running; frozen point while paused.
    movePoint(child.currentOrigin); // Current physical pose, even if a native push was blocked.
    if (rotate) {
        // Native non-axis movers encode model2 as an additive Euler offset.
        // Rebase the visible model in world space, then encode its new offset.
        const bool axisPath = jar::angular::Path(child.s.apos.trType);
        vec3_t appearance;
        if (!axisPath) {
            VectorAdd(child.currentAngles, child.modelAngles, appearance);
            jar::angular::RebaseAngles(appearance, oldAngles, newAngles, appearance);
        }
        jar::angular::RebaseAngles(child.currentAngles, oldAngles, newAngles, child.currentAngles);
        jar::angular::RebaseAngles(child.s.angles, oldAngles, newAngles, child.s.angles);
        if (!jar::angular::Rebase(child.s.apos, oldAngles, newAngles))
            jar::angular::RebaseAngles(child.s.apos.trBase, oldAngles, newAngles, child.s.apos.trBase);
        if (!axisPath) VectorSubtract(appearance, child.currentAngles, child.modelAngles);
        jar::angular::RebaseVector(child.s.pos.trDelta, oldAngles, newAngles, child.s.pos.trDelta);
    }

    // The trajectory type, duration, time and saved pause phase are unchanged.
    if (child.linked) gi.linkentity(&child);
    return true;
}
