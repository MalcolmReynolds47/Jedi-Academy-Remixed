// Owned player-start markers use the native entity and save fields.
// GPL-2.0-only.
#pragma once
#include "g_functions.h"
#include "../../shared/qcommon/jar_start_marker.h"

inline bool G_JAR_PortableStart(const gentity_t& entity)
{
    return entity.inuse && JAR_StartCard(entity.classname) && entity.model && !strcmp(entity.model, "@jar_start");
}

bool G_JAR_VisitStart(gentity_t* marker, gentity_t* actor);
