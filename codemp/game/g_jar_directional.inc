// Swept actor crossings for portable directional triggers. GPL-2.0-only.
static void DirectionalSync(int n) {
    gentity_t *root = &g_entities[n], *sensor = TriggerHelper(n);
    if (!sensor) return;
    const jarObject_t *object = &objects[n];
    if (!sandbox.integer || !object->triggerActive || (root->flags & FL_INACTIVE) || object->copyPending) {
        if (sensor->r.linked) trap->UnlinkEntity((sharedEntity_t *)sensor);
        return;
    }
    vec3_t axes[3];
    AnglesToAxis(root->r.currentAngles, axes);
    vec3_t center;
    VectorMA(root->r.currentOrigin, object->triggerExtent, axes[2], center);
    qboolean changed = !sensor->r.linked || !VectorCompare(sensor->r.currentOrigin, center);
    for (int i = 0; i < 3; ++i) {
        const float span = object->triggerExtent * (fabsf(axes[0][i]) + fabsf(axes[1][i]) + fabsf(axes[2][i]));
        if (sensor->r.maxs[i] != span) changed = qtrue;
        sensor->r.mins[i] = -span;
        sensor->r.maxs[i] = span;
    }
    if (!changed) return;
    G_SetOrigin(sensor, center);
    trap->LinkEntity((sharedEntity_t *)sensor);
}

static void DirectionalState(int n, jarControllerState_t *state) {
    const gentity_t *root = &g_entities[n];
    state->flags = root->spawnflags;
    state->speed = objects[n].triggerExtent;
    state->waitMs = root->genericValue1;
    state->initial = root->genericValue2;
    state->count = root->count;
    state->repeats = root->genericValue3;
    state->active = objects[n].triggerActive;
    state->inactive = !!(root->flags & FL_INACTIVE);
}

static qboolean DirectionalApply(int n, const jarControllerState_t *state) {
    if (!JAR_ControllerStateValid(JAR_DIRECTIONAL_CONTROLLER, state) || !TriggerHelper(n)) return qfalse;
    gentity_t *root = &g_entities[n];
    const qboolean rearmed = root->genericValue3 && state->active;
    root->spawnflags = state->flags;
    root->genericValue1 = state->waitMs;
    root->genericValue2 = state->initial;
    root->genericValue3 = state->repeats;
    root->count = state->count;
    objects[n].triggerExtent = state->speed;
    objects[n].triggerActive = state->active;
    if (state->inactive) root->flags |= FL_INACTIVE; else root->flags &= ~FL_INACTIVE;
    const int wait = state->waitMs > 0 ? state->waitMs : 1;
    if (rearmed) root->painDebounceTime = 0;
    if (root->painDebounceTime > level.time && root->painDebounceTime - level.time > wait)
        root->painDebounceTime = JAR_NumberDeadline(level.time, wait);
    DirectionalSync(n);
    return qtrue;
}

static void DirectionalUse(gentity_t *root, gentity_t *other, gentity_t *actor) {
    const int n = root->s.number;
    if (!sandbox.integer || (root->flags & FL_INACTIVE) || objects[n].copyPending) return;
    jarControllerState_t state = {0};
    DirectionalState(n, &state);
    state.active = !state.active;
    if (state.active && state.repeats) {
        state.count = state.initial;
        state.repeats = 0;
    }
    if (!DirectionalApply(n, &state)) return;
    ++objects[n].propertyRevision;
    SelectionChanged(n);
}

static qboolean DirectionalInit(gentity_t *root) {
    if (!Room()) return qfalse;
    gentity_t *sensor = G_TrySpawn();
    if (!sensor) return qfalse;
    const int n = root->s.number, helper = sensor->s.number;
    sensor->classname = "jar_directional_volume";
    sensor->r.svFlags = SVF_NOCLIENT;
    sensor->r.contents = CONTENTS_TRIGGER;
    triggerParent[helper] = n + 1;
    directionalSensor[helper] = 1;
    ++directionalSensors;
    objects[n].triggerHelper = helper + 1;
    root->use = DirectionalUse;
    jarControllerState_t state = {0};
    state.speed = JAR_DIRECTIONAL_DEFAULT_EXTENT;
    state.waitMs = 1000;
    state.active = 1;
    return DirectionalApply(n, &state);
}

static void DirectionalCross(gentity_t *root, gentity_t *actor, const vec3_t start, const vec3_t end) {
    const int n = root->s.number;
    jarObject_t *object = &objects[n];
    if (!object->triggerActive || root->painDebounceTime > level.time ||
        ((root->spawnflags & JAR_DIRECTIONAL_PLAYER_ONLY) && actor->s.number >= MAX_CLIENTS) ||
        !ActionActivator(root, actor)) return;
    vec3_t axes[3], crossing, offset;
    AnglesToAxis(root->r.currentAngles, axes);
    vec3_t center;
    VectorMA(root->r.currentOrigin, object->triggerExtent, axes[2], center);
    const int direction = JAR_DirectionalCrossing(center, axes[0], start, end, crossing);
    if (!direction) return;
    VectorSubtract(crossing, center, offset);
    for (int a = 0; a < 3; ++a)
        if (fabsf(DotProduct(offset, axes[a])) > object->triggerExtent + .001f) return;
    const int output = direction < 0;
    const int target = object->output[output] - 1;
    if (target < MAX_CLIENTS || target >= ENTITYNUM_WORLD || !ForcePermission(object->owner, target) ||
        objects[target].copyPending || !g_entities[target].use || (g_entities[target].flags & FL_INACTIVE)) return;

    const int wait = root->genericValue1 > 0 ? root->genericValue1 : 1;
    root->painDebounceTime = JAR_NumberDeadline(level.time, wait);
    if (root->count) --root->count;
    if (root->genericValue1 < 0 || (root->genericValue2 && !root->count)) {
        root->genericValue3 = 1;
        object->triggerActive = 0;
        DirectionalSync(n);
    }
    ++object->propertyRevision;
    SelectionChanged(n);
    ControllerInvoke(&g_entities[target], root, actor);
}

void JAR_MP_DirectionalCrossings(gentity_t *actor, int teleportBit) {
    if (!directionalSensors || !sandbox.integer || !actor->client || actor->client->noclip ||
        actor->health <= 0 || actor->client->sess.sessionTeam == TEAM_SPECTATOR ||
        !trap->EntitiesInBox || (actor->client->ps.eFlags & EF_TELEPORT_BIT) != teleportBit) return;
    vec3_t start, end, low, high;
    VectorCopy(actor->client->oldOrigin, start);
    VectorCopy(actor->client->ps.origin, end);
    const float distance = DistanceSquared(start, end);
    if (!isfinite(distance) || distance <= 0 || distance > 1024.f * 1024.f) return;
    for (int a = 0; a < 3; ++a) {
        low[a] = fminf(start[a], end[a]) - 1;
        high[a] = fmaxf(start[a], end[a]) + 1;
    }
    int hits[MAX_GENTITIES];
    const int count = trap->EntitiesInBox(low, high, hits, MAX_GENTITIES);
    for (int i = 0; i < count; ++i) {
        const int helper = hits[i];
        if (helper < MAX_CLIENTS || helper >= ENTITYNUM_WORLD || !directionalSensor[helper] || !g_entities[helper].r.linked) continue;
        const int n = triggerParent[helper] - 1;
        if (ControllerKind(n) != JAR_DIRECTIONAL_CONTROLLER || TriggerHelper(n) != &g_entities[helper]) continue;
        DirectionalCross(&g_entities[n], actor, start, end);
        if (!actor->inuse || !actor->client || actor->health <= 0 ||
            (actor->client->ps.eFlags & EF_TELEPORT_BIT) != teleportBit) break;
    }
}
