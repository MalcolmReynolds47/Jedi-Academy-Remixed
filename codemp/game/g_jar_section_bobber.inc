// Native moving platform owned by a stock Bespin section. GPL-2.0-only.
enum { SECTION_BOBBER_PERIOD = 3000 };

static int SectionLiftRoot(int n);

static int SectionBobberRoot(int n) {
    if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !g_entities[n].inuse ||
        sectionRole[n] != JAR_MP_SECTION_BOBBER) return ENTITYNUM_NONE;
    const int root = sectionParent[n] - 1;
    return SectionRoot(root) ? root : ENTITYNUM_NONE;
}

qboolean JAR_MP_SectionMoverProtected(const gentity_t *pusher, const gentity_t *target) {
    if (!pusher) return qfalse;
    int root = SectionBobberRoot(pusher->s.number);
    if (root == ENTITYNUM_NONE) root = SectionLiftRoot(pusher->s.number);
    if (root == ENTITYNUM_NONE) return qfalse;
    if (!target) return qtrue;
    const int n = target->s.number;
    return !SectionActive(root) || n < 0 || n >= ENTITYNUM_WORLD || !target->inuse ||
        objects[n].copyPending || !LaserDamageAllowed(objects[root].owner, n);
}

static gentity_t *SectionBobber(int root) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] == JAR_MP_SECTION_BOBBER) return &g_entities[n];
    }
    return NULL;
}

static qboolean SectionBobberInit(gentity_t *child, int root, int first,
                                  const jarMPSectionChild_t *record) {
    if (record->inlineModel != 1 || record->period != SECTION_BOBBER_PERIOD) return qfalse;
    const int index = first + record->inlineModel;
    trap->SetBrushModel((sharedEntity_t *)child, va("*%d", index));
    if (!child->r.bmodel || child->s.modelindex != index ||
        (unsigned)child->r.contents != (CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP | CONTENTS_TRANSLUCENT)) return qfalse;

    child->classname = "jar_section_bobber";
    child->s.eType = ET_MOVER;
    child->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
    child->s.otherEntityNum = root;
    child->s.modelindex2 = G_ModelIndex(record->model);
    child->s.pos.trType = TR_SINE;
    child->s.pos.trTime = level.time;
    child->s.pos.trDuration = record->period;
    child->s.apos.trType = TR_STATIONARY;
    child->speed = record->amplitude;
    return child->s.modelindex2 > 0;
}

static void SectionBobberPublish(gentity_t *child, const vec3_t rest, const vec3_t axis) {
    VectorCopy(rest, child->s.pos.trBase);
    VectorScale(axis, child->speed, child->s.pos.trDelta);
    BG_EvaluateTrajectory(&child->s.pos, level.time, child->r.currentOrigin);
    VectorCopy(child->r.currentOrigin, child->s.origin);
    trap->LinkEntity((sharedEntity_t *)child);
}

static int SectionBobberPhase(int root) {
    const gentity_t *child = SectionBobber(root);
    if (!child) return 0;
    const int elapsed = (int)(((int64_t)level.time - child->s.pos.trTime) % SECTION_BOBBER_PERIOD);
    return elapsed < 0 ? elapsed + SECTION_BOBBER_PERIOD : elapsed;
}

static void SectionBobberRestore(int root, int phase) {
    gentity_t *child = SectionBobber(root);
    if (!child) return;
    child->s.pos.trTime = level.time - phase;
    BG_EvaluateTrajectory(&child->s.pos, level.time, child->r.currentOrigin);
    VectorCopy(child->r.currentOrigin, child->s.origin);
    if (child->r.linked) trap->LinkEntity((sharedEntity_t *)child);
}
