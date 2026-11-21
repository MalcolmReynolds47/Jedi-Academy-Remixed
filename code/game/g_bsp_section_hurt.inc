// Adapt a qualified native hurt brush after its native spawn callback.
// GPL-2.0-only.
#include "g_bsp_section_hurt.h"

void G_BSPSectionRefreshBoxBounds(gentity_t& e) {
    if (!G_BSPSectionBoxTagged(e) || !BSPSectionBoxValid(e.s.origin2, e.s.angles2)) return;
    for (int i = 0; i < 3; ++i) if (!std::isfinite(e.currentAngles[i])) return;
    jar::volume::Hull(e.s.origin2, e.s.angles2, e.currentAngles, e.mins, e.maxs);
    if (e.s.time2 == BSPSectionRiftUnionTag) {
        const auto& second = jar::rift::fallBoxes[1];
        vec3_t low, high;
        jar::volume::Hull(second.mins, second.maxs, e.currentAngles, low, high);
        AddPointToBounds(low, e.mins, e.maxs);
        AddPointToBounds(high, e.mins, e.maxs);
    }
}

static bool BSPSectionInitializeBox(gentity_t& e, const vec3_t lo, const vec3_t hi, int tag) {
    if (!e.inuse || !e.bmodel || e.contents != CONTENTS_TRIGGER ||
        !(e.svFlags & SVF_NOCLIENT) || !BSPSectionBoxValid(lo, hi)) return false;
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(e.currentAngles[i]) || !std::isfinite(e.currentOrigin[i])) return false;

    const bool linked = e.linked != qfalse;
    if (linked) gi.unlinkentity(&e);
    e.bmodel = qfalse;
    e.s.eType = ET_INVISIBLE;
    e.s.modelindex = e.s.solid = 0;
    e.s.time2 = tag;
    if (e.model && gi.bIsFromZone(e.model, TAG_G_ALLOC)) gi.Free(e.model);
    e.model = nullptr;
    VectorCopy(lo, e.s.origin2);
    VectorCopy(hi, e.s.angles2);
    // Keep the same local bounds available to the game-side native helpers.
    VectorCopy(lo, e.pos1);
    VectorCopy(hi, e.pos2);
    G_BSPSectionRefreshBoxBounds(e);
    if (linked) gi.linkentity(&e);
    return true;
}

bool G_BSPSectionInitializeHurt(gentity_t& e, const vec3_t lo, const vec3_t hi, bool riftUnion) {
    const int authoredFlags = e.spawnflags & ~4;
    const bool fall = e.damage == -1 && e.wait == 0 && (authoredFlags == 0 || authoredFlags == 8);
    const bool timed = e.damage == 50 && e.wait == 1000 && authoredFlags == 0;
    const int tag = riftUnion ? BSPSectionRiftUnionTag : BSPSectionHurtTag;
    if (!e.classname || strcmp(e.classname, "trigger_hurt") || e.e_TouchFunc != touchF_hurt_touch ||
        (riftUnion ? !fall : !fall && !timed) || !BSPSectionInitializeBox(e, lo, hi, tag)) return false;
    e.spawnflags |= 4; // Retail multiplayer's hurt callback does not play a sound.
    e.noise_index = 0;
    if (fall) {
        // MP's -1 starts a fatal fall regardless of the NO_PROTECTION flag.
        // The native SP path needs both FALLING and NO_PROTECTION.
        e.damage = Q3_INFINITE;
        e.spawnflags |= 32 | 8;
    }
    return true;
}

bool G_BSPSectionInitializePad(gentity_t& e, const vec3_t lo, const vec3_t hi) {
    return e.classname && !strcmp(e.classname, "trigger_multiple") &&
        e.e_TouchFunc == touchF_Touch_Multi && e.e_UseFunc == useF_Use_Multi && e.target &&
        BSPSectionInitializeBox(e, lo, hi, BSPSectionPadTag);
}
