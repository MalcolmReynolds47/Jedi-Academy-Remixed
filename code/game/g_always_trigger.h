// Owned trigger_always state uses native save fields and callbacks. GPL-2.0-only.
#pragma once
#include "g_functions.h"
#include "../../shared/qcommon/jar_always_trigger.h"

inline bool G_AlwaysClass(const gentity_t& entity)
{
    return entity.inuse && entity.classname && !strcmp(entity.classname, "trigger_always");
}

inline bool G_AlwaysPortable(const gentity_t& entity)
{
    return G_AlwaysClass(entity) && entity.model && !strcmp(entity.model, "@jar_always");
}

inline void G_AlwaysSchedule(gentity_t& entity, int remaining)
{
    entity.e_ThinkFunc = remaining ? thinkF_trigger_always_think : thinkF_NULL;
    entity.nextthink = remaining ? level.time + remaining : 0;
}

inline int G_AlwaysRemaining(const gentity_t& entity, int now = level.time)
{
    if (entity.e_ThinkFunc != thinkF_trigger_always_think || entity.nextthink <= 0) return 0;
    const int remaining = entity.nextthink - now;
    return remaining > 0 ? remaining : 1;
}
