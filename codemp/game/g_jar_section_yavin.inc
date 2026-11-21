// Yavin's stock timer chooses one local dust emitter per pulse. GPL-2.0-only.
enum { SECTION_YAVIN_ID = 95133242, SECTION_YAVIN_DUST_MAX = 8000 };

static void SectionDustThink(gentity_t *timer) {
    const int root = sectionParent[timer->s.number] - 1;
    if (!SectionRoot(root)) return;
    if (!SectionActive(root)) {
        timer->nextthink = level.time + FRAMETIME;
        return;
    }

    timer->nextthink = level.time + Q_irand(2000, SECTION_YAVIN_DUST_MAX);
    if (!Room()) return;
    gentity_t *emitter = NULL;
    int count = 0;
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        if (sectionRole[child->s.number] == JAR_MP_SECTION_DUST && !Q_irand(0, count++)) emitter = child;
    }
    if (emitter && emitter->s.modelindex) {
        gentity_t *event = G_TempEntity(emitter->r.currentOrigin, EV_PLAY_EFFECT_ID);
        if (event) {
            VectorCopy(emitter->r.currentAngles, event->s.angles);
            event->s.eventParm = emitter->s.modelindex;
            event->s.otherEntityNum = root;
            event->r.svFlags |= SVF_BROADCAST;
        }
    }
}

static gentity_t *SectionDustTimer(int root) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1])
        if (sectionRole[next - 1] == JAR_MP_SECTION_DUST_TIMER) return &g_entities[next - 1];
    return NULL;
}
