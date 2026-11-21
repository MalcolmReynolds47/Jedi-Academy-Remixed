// Owned SP visibility marker state, stored on the native entity.
#pragma once
#include "../../shared/qcommon/jar_visible_trigger.h"

inline bool G_VisibleClass(const gentity_t& entity)
{
    return entity.classname && !strcmp(entity.classname, "trigger_visible");
}

inline bool G_VisiblePortable(const gentity_t& entity)
{
    return G_VisibleClass(entity) && entity.model && !strcmp(entity.model, "@jar_visible");
}

inline jarVisibleSettings_t G_VisibleSettings(const gentity_t& entity)
{
    return {int(entity.radius), int(entity.random), entity.spawnflags};
}

inline void G_VisibleSet(gentity_t& entity, jarVisibleSettings_t settings)
{
    entity.radius = settings.range;
    entity.random = settings.cone;
    entity.spawnflags = settings.flags;
}

inline void G_VisibleArm(gentity_t& entity, bool armed)
{
    entity.e_ThinkFunc = armed ? thinkF_trigger_visible_check_player_visibility : thinkF_NULL;
    entity.nextthink = armed ? level.time + JAR_VISIBLE_SAMPLE_MS : 0;
}
