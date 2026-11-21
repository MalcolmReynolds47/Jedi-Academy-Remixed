// Native item placement and respawn within a portable section. GPL-2.0-only.
#include "g_bsp_section.h"

static bool BSPSectionPlacePickup(gentity_t& item, const gentity_t& root) {
    vec3_t axes[3];
    AnglesToAxis(root.currentAngles, axes);
    for (float lift : {1.0f, 8.0f, 16.0f, 24.0f, 32.0f}) {
        vec3_t start, end;
        VectorMA(item.s.origin, lift, axes[2], start);
        VectorMA(start, -4096.0f, axes[2], end);
        trace_t trace{};
        gi.trace(&trace, start, item.mins, item.maxs, end, root.s.number,
            MASK_SOLID | CONTENTS_PLAYERCLIP, G2_SINGLE_BRUSH, 0);
        if (trace.startsolid || trace.allsolid)
            continue;
        // Use the room's own floor. An unsupported anchor keeps its authored
        // position instead of dropping into the host map.
        const auto* anchor = trace.fraction < 1 ? trace.endpos : start;
        G_SetOrigin(&item, anchor);
        VectorCopy(item.currentOrigin, item.s.origin);
        return true;
    }
    return false;
}

static bool BSPSectionSchedulePickup(gentity_t& item) {
    const auto* root=G_BSPSectionPickupRoot(item);
    if (!root)
        return false;
    size_t count;
    const auto* specs=jar::bspSection::PickupSpecs(jar::bspSection::Resource(root->model)->mode,count);
    for(size_t i=0;i<count;++i){
        const auto& spec=specs[i];
        if (strcmp(item.classname, spec.name))
            continue;
        item.s.eFlags |= EF_NODRAW;
        item.contents = 0;
        item.e_TouchFunc = touchF_NULL;
        item.e_UseFunc = useF_NULL;
        item.svFlags &= ~SVF_PLAYER_USABLE;
        item.e_ThinkFunc = thinkF_RespawnItem;
        item.nextthink = level.time + 1000 * spec.seconds;
        gi.linkentity(&item);
        return true;
    }
    return false;
}

static void BSPSectionRespawnPickup(gentity_t& item) {
    if (!G_BSPSectionPickupRoot(item) || !(item.s.eFlags & EF_NODRAW))
        return;
    item.s.eFlags &= ~EF_NODRAW;
    item.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;
    item.e_TouchFunc = touchF_Touch_Item;
    item.e_UseFunc = useF_Use_Item;
    item.svFlags |= SVF_PLAYER_USABLE;
    item.e_ThinkFunc = thinkF_NULL;
    item.nextthink = 0;
    gi.linkentity(&item);
}
