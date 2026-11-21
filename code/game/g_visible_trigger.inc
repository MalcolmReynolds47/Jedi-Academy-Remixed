// Owned markers survive the pulse so that the Sandbox can rearm them.
#include "g_visible_trigger.h"

static void G_VisibleSpawn(gentity_t* entity)
{
    G_VisibleSet(*entity, {JAR_VISIBLE_RANGE_DEFAULT, JAR_VISIBLE_CONE_DEFAULT, 0});
    entity->e_UseFunc = useF_portable_visible_use;
    G_SetOrigin(entity, entity->s.origin);
    gi.linkentity(entity);
    G_VisibleArm(*entity, true);
}

static void G_VisibleCheck(gentity_t* entity)
{
    G_VisibleArm(*entity, true);
    auto& player = g_entities[0];
    const auto settings = G_VisibleSettings(*entity);
    if ((entity->svFlags & SVF_INACTIVE) || !player.inuse || !player.client || player.health <= 0 ||
        ((settings.flags & JAR_VISIBLE_FORCE_SIGHT) && !(player.client->ps.forcePowersActive & (1 << FP_SEE))))
        return;

    const auto& view = player.client->renderInfo;
    vec3_t forward;
    AngleVectors(view.eyeAngles, forward, nullptr, nullptr);
    if (!JAR_VisibleFacing(entity->currentOrigin, view.eyePoint, forward, settings.range,
                           JAR_VisibleCosine(settings.cone)) ||
        !gi.inPVS(view.eyePoint, entity->currentOrigin))
        return;

    const vec3_t mins = {-1, -1, -1}, maxs = {1, 1, 1};
    if (!(settings.flags & JAR_VISIBLE_IGNORE_WALLS) &&
        !G_ClearTrace(view.eyePoint, mins, maxs, entity->currentOrigin, player.s.number, MASK_OPAQUE))
        return;

    G_VisibleArm(*entity, false);
    G_UseTargets(entity, &player);
}

void portable_visible_use(gentity_t* entity, gentity_t*, gentity_t*)
{
    if (G_VisiblePortable(*entity) && !(entity->svFlags & SVF_INACTIVE) &&
        entity->e_ThinkFunc != thinkF_trigger_visible_check_player_visibility)
        G_VisibleArm(*entity, true);
}
