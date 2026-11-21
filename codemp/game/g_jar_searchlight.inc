// Native line-of-sight and independent detection for each connected player.
static void SearchlightState(int n, jarControllerState_t *state) {
    const gentity_t *root = &g_entities[n];
    state->speed = root->genericValue1;
    state->damage = root->genericValue2;
    state->pushSpeed = root->genericValue3;
    state->flags = root->spawnflags;
    state->active = root->genericValue4;
    state->inactive = !!(root->flags & FL_INACTIVE);
}

static qboolean SearchlightApply(int n, const jarControllerState_t *state) {
    if (!JAR_ControllerStateValid(JAR_SEARCHLIGHT_CONTROLLER, state)) return qfalse;
    gentity_t *root = &g_entities[n];
    root->genericValue1 = state->speed;
    root->genericValue2 = state->damage;
    root->genericValue3 = state->pushSpeed;
    root->genericValue4 = state->active;
    root->spawnflags = state->flags;
    if (state->inactive) root->flags |= FL_INACTIVE; else root->flags &= ~FL_INACTIVE;
    if (!state->active || state->inactive) root->s.time = 0;
    return qtrue;
}

static void SearchlightUse(gentity_t *root, gentity_t *other, gentity_t *actor) {
    const int n = root->s.number;
    if (!sandbox.integer || (root->flags & FL_INACTIVE) || objects[n].copyPending) return;
    jarControllerState_t state = {0};
    SearchlightState(n, &state);
    state.active = !state.active;
    SearchlightApply(n, &state);
    ++objects[n].propertyRevision;
    SelectionChanged(n);
}

static void SearchlightInit(gentity_t *root) {
    root->use = SearchlightUse;
    root->r.contents = CONTENTS_ITEM;
    root->s.bolt2 = JAR_MP_SEARCHLIGHT_TAG;
    const jarControllerState_t state = {
        .flags = JAR_SEARCHLIGHT_TRACK | JAR_SEARCHLIGHT_WHITE,
        .speed = JAR_SEARCHLIGHT_DEFAULT_RANGE, .active = 1,
        .damage = JAR_SEARCHLIGHT_DEFAULT_ANGLE, .pushSpeed = JAR_SEARCHLIGHT_DEFAULT_LIGHT
    };
    SearchlightApply(root->s.number, &state);
}

static void SearchlightFrame(gentity_t *root) {
    const int n = root->s.number;
    const jarObject_t *object = &objects[n];
    if (!sandbox.integer || (root->flags & FL_INACTIVE) || !root->genericValue4 || object->copyPending) {
        root->s.time = 0;
        root->genericValue5 = 0;
        return;
    }
    if (root->genericValue6 > level.time) return;
    root->genericValue6 = level.time + JAR_SEARCHLIGHT_SAMPLE_MS;
    vec3_t forward;
    AngleVectors(root->r.currentAngles, forward, NULL, NULL);
    const int aim = object->output[0] - 1;
    int pass = n;
    // A linked aim marker is selectable but its editor hull must not
    // shadow the cone. Physical targets still occlude normally.
    if (JAR_ControllerPosition(ControllerKind(aim)) && ForcePermission(object->owner, aim)) pass = aim;
    if ((root->spawnflags & JAR_SEARCHLIGHT_TRACK) && aim >= MAX_CLIENTS && aim < ENTITYNUM_WORLD &&
        ForcePermission(object->owner, aim) && !objects[aim].copyPending) {
        vec3_t direction;
        VectorSubtract(g_entities[aim].r.currentOrigin, root->r.currentOrigin, direction);
        if (VectorNormalize(direction) > .001f) VectorCopy(direction, forward);
    }
    vec3_t start, end;
    VectorMA(root->r.currentOrigin, 12, forward, start);
    VectorMA(start, root->genericValue1, forward, end);
    trace_t trace;
    trap->Trace(&trace, start, NULL, NULL, end, pass, MASK_SHOT, qfalse, 0, 0);
    vec3_t clipped;
    VectorSubtract(trace.endpos, start, clipped);
    root->s.time = trace.startsolid || trace.allsolid || DotProduct(clipped, forward) <= 0 ? 0 :
        (root->genericValue2 << 24) | (root->spawnflags & JAR_SEARCHLIGHT_WHITE);
    root->s.eventParm = root->genericValue3;
    VectorCopy(trace.endpos, root->s.origin);
    VectorCopy(forward, root->s.angles);
    if (!root->s.time) { root->genericValue5 = 0; return; }

    const int output = object->output[1] - 1;
    if (output < MAX_CLIENTS || output >= ENTITYNUM_WORLD || !ForcePermission(object->owner, output) ||
        objects[output].copyPending || !g_entities[output].use || (g_entities[output].flags & FL_INACTIVE)) {
        root->genericValue5 = 0;
        return;
    }
    const float threshold = cosf(root->genericValue2 * (M_PI / 180.f));
    unsigned inside = 0;
    for (int c = 0; c < level.maxclients; ++c) {
        gentity_t *player = &g_entities[c];
        if (!ActionActivator(root, player)) continue;
        vec3_t center, direction;
        VectorAdd(player->r.mins, player->r.maxs, center);
        VectorMA(player->r.currentOrigin, .5f, center, center);
        VectorSubtract(center, start, direction);
        const float distance = VectorNormalize(direction);
        if (distance <= .001f || distance > root->genericValue1 || DotProduct(direction, forward) < threshold) continue;
        trap->Trace(&trace, start, NULL, NULL, center, pass, MASK_SHOT, qfalse, 0, 0);
        if (!trace.startsolid && !trace.allsolid && (trace.fraction >= 1 || trace.entityNum == c)) inside |= 1u << c;
    }
    const unsigned entering = inside & ~(unsigned)root->genericValue5;
    root->genericValue5 = (int)inside;
    if (!entering || root->painDebounceTime > level.time) return;
    root->painDebounceTime = level.time + JAR_SEARCHLIGHT_REARM_MS;
    for (int c = 0; c < level.maxclients; ++c) {
        if (!(entering & (1u << c))) continue;
        gentity_t *target = &g_entities[output];
        if (ControllerKind(output) == 1 && target->spawnflags == 4 && target->message) {
            // The native map print callback has a debug-only global spam guard.
            // An owned private message may legitimately reach several players at once.
            trap->SendServerCommand(c, va("cp \"%s\"", target->message));
        } else {
            ControllerInvoke(target, root, &g_entities[c]);
            return; // A gameplay callback may remove the light or its target.
        }
    }
}

static void SearchlightClearClient(int c) {
    for (int n = MAX_CLIENTS; n < level.num_entities; ++n)
        if (g_entities[n].inuse && JAR_MP_Searchlight(&g_entities[n].s))
            g_entities[n].genericValue5 = (int)((unsigned)g_entities[n].genericValue5 & ~(1u << c));
}
