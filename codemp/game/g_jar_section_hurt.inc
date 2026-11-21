// Native damage and oriented contact for admitted retail hurt volumes.
// GPL-2.0-only.
extern void hurt_touch(gentity_t *, gentity_t *, trace_t *);

enum { SECTION_HURT_MAX_REMAINING = 1000 };

static qboolean SectionBoxContact(const gentity_t *child, const gentity_t *actor, const vec3_t origin,
                                 const vec3_t lo, const vec3_t hi) {
    vec3_t half, localCenter, center;
    for (int axis = 0; axis < 3; ++axis) {
        localCenter[axis] = (lo[axis] + hi[axis]) * .5f;
        half[axis] = (hi[axis] - lo[axis]) * .5f;
        if (!isfinite(origin[axis]) || !isfinite(actor->r.mins[axis]) || !isfinite(actor->r.maxs[axis]) ||
            actor->r.mins[axis] > actor->r.maxs[axis]) return qfalse;
    }
    SectionOffset(child->r.currentAngles, localCenter, center);
    VectorAdd(child->r.currentOrigin, center, center);
    return JAR_BoxVolumeContact(center, child->r.currentAngles, half, origin, actor->r.mins, actor->r.maxs);
}

static qboolean SectionHurtContact(const gentity_t *child, const gentity_t *actor, const vec3_t origin) {
    return SectionBoxContact(child, actor, origin, child->s.origin2, child->s.angles2) ||
        (child->genericValue1 && SectionBoxContact(child, actor, origin, child->movedir, child->s.apos.trDelta));
}

static void SectionHurtTouch(gentity_t *child, gentity_t *other, trace_t *trace) {
    const int n = child->s.number, root = sectionParent[n] - 1;
    if (!child->inuse || !child->r.linked || sectionRole[n] != JAR_MP_SECTION_HURT ||
        !SectionRoot(root) || !SectionActive(root) || !other || !other->inuse || !other->takedamage) return;
    const int target = other->s.number;
    if (target < 0 || target >= ENTITYNUM_WORLD || objects[target].copyPending ||
        !LaserDamageAllowed(objects[root].owner, target)) return;
    const float *origin = other->client ? other->client->ps.origin : other->r.currentOrigin;
    if (SectionHurtContact(child, other, origin)) hurt_touch(child, other, trace);
}

static void SectionHurtInit(gentity_t *child, const jarMPSectionChild_t *record) {
    child->classname = "jar_section_hurt";
    child->s.eType = ET_INVISIBLE;
    child->r.svFlags = SVF_NOCLIENT;
    child->r.contents = CONTENTS_TRIGGER;
    child->damage = record->damage;
    child->spawnflags = record->spawnflags;
    child->touch = SectionHurtTouch;
    VectorCopy(record->mins, child->s.origin2);
    VectorCopy(record->maxs, child->s.angles2);
    if (record->secondMaxs[2] > record->secondMins[2]) {
        child->genericValue1 = 1;
        VectorCopy(record->secondMins, child->movedir);
        VectorCopy(record->secondMaxs, child->s.apos.trDelta);
    }
}

static void SectionHurtPublish(gentity_t *child) {
    // Engine trigger dispatch tests an enclosing AABB before the exact test.
    SectionHull(child->s.origin2, child->s.angles2, child->r.currentAngles, child->r.mins, child->r.maxs);
    if (child->genericValue1) {
        vec3_t mins, maxs;
        SectionHull(child->movedir, child->s.apos.trDelta, child->r.currentAngles, mins, maxs);
        for (int a = 0; a < 3; ++a) {
            child->r.mins[a] = fminf(child->r.mins[a], mins[a]);
            child->r.maxs[a] = fmaxf(child->r.maxs[a], maxs[a]);
        }
    }
    trap->LinkEntity((sharedEntity_t *)child);
}

static gentity_t *SectionHurtChild(int root) {
    // Timed damage profiles currently have one trigger. Fatal falls do not
    // need a persisted damage cooldown.
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int child = next - 1;
        if (sectionRole[child] == JAR_MP_SECTION_HURT) return &g_entities[child];
    }
    return NULL;
}

static qboolean SectionHurtEntryClear(const gentity_t *root, const gentity_t *actor, const vec3_t arrival) {
    if (!SectionActive(root->s.number)) return qtrue;
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1]) {
        const gentity_t *child = &g_entities[next - 1];
        if (sectionRole[next - 1] == JAR_MP_SECTION_HURT && child->r.linked &&
            SectionHurtContact(child, actor, arrival)) return qfalse;
    }
    return qtrue;
}

static int SectionHurtRemaining(int root) {
    const gentity_t *child = SectionHurtChild(root);
    if (!child || child->damage <= 0 || child->timestamp <= level.time) return 0;
    const double remaining = (double)child->timestamp - level.time;
    return remaining > SECTION_HURT_MAX_REMAINING ? SECTION_HURT_MAX_REMAINING : (int)remaining;
}

static void SectionHurtRestore(int root, int remaining) {
    gentity_t *child = SectionHurtChild(root);
    if (!child || child->damage <= 0) return;
    if (remaining <= 0) {
        child->timestamp = 0;
    } else {
        if (remaining > SECTION_HURT_MAX_REMAINING) remaining = SECTION_HURT_MAX_REMAINING;
        child->timestamp = level.time > INT_MAX - remaining ? INT_MAX : level.time + remaining;
    }
}
