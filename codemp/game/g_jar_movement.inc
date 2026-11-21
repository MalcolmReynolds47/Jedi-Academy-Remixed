// Portable movement markers affect an eligible activator's predicted movement.
#include "bg_jar_movement.h"

void JAR_MP_ApplyMovementGravity(gentity_t *entity) {
    playerState_t *ps = &entity->client->ps;
    if (!sandbox.integer) {
        ps->stats[STAT_JAR_ACTOR_GRAVITY] = 0;
        ps->stats[STAT_JAR_ACTOR_FRICTION] = 0;
    }
    if ((entity->m_pVehicle && entity->m_pVehicle->m_pVehicleInfo && entity->m_pVehicle->m_pVehicleInfo->gravity) ||
        (entity->NPC && (entity->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY)) ||
        JAR_MP_SpaceGravity(entity) >= 0 ||
        (ps->eFlags2 & EF2_SHIP_DEATH)) return;
    const int gravity = ActorVehicle(entity) ? -1 : JAR_ActorMovementValue(ps, STAT_JAR_ACTOR_GRAVITY, JAR_MP_ACTOR_MOVEMENT_MAX);
    ps->gravity = gravity >= 0 ? gravity : g_gravity.value;
}

static qboolean MovementActorEligible(gentity_t *actor) {
    if (!actor || !ActorAlive(actor) || actor->client->noclip || ActorVehicle(actor) ||
        JAR_MP_ActorControlled(actor) || actor->client->ps.pm_type != PM_NORMAL ||
        actor->client->ps.emplacedIndex) return qfalse;
    return !actor->NPC || !(actor->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY);
}

static int MovementPropertyStat(int field) {
    return field == JAR_PROPERTY_ACTOR_GRAVITY ? STAT_JAR_ACTOR_GRAVITY : STAT_JAR_ACTOR_FRICTION;
}

static void MovementUse(gentity_t *source, gentity_t *other, gentity_t *actor) {
    if (!MovementActorEligible(actor) || !ActionActivator(source, actor)) return;
    const qboolean gravity = !strcmp(source->classname, "target_gravity_change");
    const int stat = gravity ? STAT_JAR_ACTOR_GRAVITY : STAT_JAR_ACTOR_FRICTION;
    const int value = (int)source->speed + 1;
    if (actor->client->ps.stats[stat] == value) return;
    actor->client->ps.stats[stat] = value;
    if (gravity) JAR_MP_ApplyMovementGravity(actor);
    ++objects[actor->s.number].propertyRevision;
    SelectionChanged(actor->s.number);
}
