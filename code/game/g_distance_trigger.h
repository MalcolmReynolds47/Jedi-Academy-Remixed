// Owned SP distance trigger fields use the stock entity serializer.
#pragma once
#include "../../shared/qcommon/jar_distance_trigger.h"

constexpr char JAR_DISTANCE_MISSING_REFERENCE_NAME[]="@jar_missing_reference";

inline bool G_DistanceClass(const gentity_t& entity)
{
    return entity.classname && !strcmp(entity.classname, "trigger_entdist");
}

inline bool G_DistancePortable(const gentity_t& entity)
{
    return G_DistanceClass(entity) && entity.model && !strcmp(entity.model, "@jar_entdist");
}

inline char*& G_DistanceLink(gentity_t& entity, int link)
{
    switch (link) {
    case JAR_DISTANCE_OUTSIDE: return entity.target2;
    case JAR_DISTANCE_REFERENCE: return entity.ownername;
    case JAR_DISTANCE_SUBJECT: return entity.NPC_target;
    default: return entity.target;
    }
}

inline const char* G_DistanceLink(const gentity_t& entity, int link)
{
    switch (link) {
    case JAR_DISTANCE_OUTSIDE: return entity.target2;
    case JAR_DISTANCE_REFERENCE: return entity.ownername;
    case JAR_DISTANCE_SUBJECT: return entity.NPC_target;
    default: return entity.target;
    }
}

inline void G_DistanceSetLink(gentity_t& entity, int link, const char* name)
{
    char*& field = G_DistanceLink(entity, link);
    char* replacement = name && *name ? G_NewString(name) : nullptr;
    if (field && gi.bIsFromZone(field, TAG_G_ALLOC)) gi.Free(field);
    field = replacement;
}
