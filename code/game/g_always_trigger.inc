// The owned marker survives the pulse. Authored map triggers remain one-shot.
#include "g_always_trigger.h"

void trigger_always_think(gentity_t* entity)
{
    if (G_AlwaysPortable(*entity)) {
        G_AlwaysSchedule(*entity, 0);
        if (!(entity->svFlags & SVF_INACTIVE)) G_UseTargets(entity, entity);
        return;
    }
    G_UseTargets(entity, entity);
    G_FreeEntity(entity);
}

void portable_always_use(gentity_t* entity, gentity_t*, gentity_t*)
{
    if (G_AlwaysPortable(*entity) && !(entity->svFlags & SVF_INACTIVE) &&
        !G_AlwaysRemaining(*entity) && JAR_AlwaysDelayValid(entity->delay))
        G_AlwaysSchedule(*entity, entity->delay);
}

/*QUAKED trigger_always (.1 .5 .1) (-8 -8 -8) (8 8 8)
This trigger will always fire. It is activated by the world.
*/
void SP_trigger_always(gentity_t* entity)
{
    if (G_AlwaysPortable(*entity)) {
        entity->delay = JAR_ALWAYS_DEFAULT_MS;
        entity->e_UseFunc = useF_portable_always_use;
    }
    // Give the map or Sandbox setup time to finish wiring the targets.
    G_AlwaysSchedule(*entity, JAR_ALWAYS_DEFAULT_MS);
}
