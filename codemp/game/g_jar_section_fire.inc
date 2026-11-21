// Native effects and consent-aware damage for section-owned torches.
// GPL-2.0-only.
extern void fx_runner_think(gentity_t *);
static void SectionFireThink(gentity_t *child);

enum { SECTION_FIRE_PERIOD = 200, SECTION_FIRE_DAMAGE = 5, SECTION_FIRE_RADIUS = 16 };

qboolean JAR_MP_SectionFireSource(const gentity_t *source) {
    return source && source->think == SectionFireThink;
}

static int SectionFireParent(const gentity_t *child) {
    if (!JAR_MP_SectionFireSource(child) || !child->inuse || child->s.eType != ET_FX)
        return ENTITYNUM_NONE;
    const int n = child->s.number;
    if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || sectionRole[n] != JAR_MP_SECTION_FIRE)
        return ENTITYNUM_NONE;
    const int root = sectionParent[n] - 1;
    return child->s.otherEntityNum == root && SectionRoot(root) ? root : ENTITYNUM_NONE;
}

qboolean JAR_MP_SectionRadiusDenied(const gentity_t *source, const gentity_t *target) {
    const int root = SectionFireParent(source);
    const int n = target ? target->s.number : ENTITYNUM_NONE;
    return root == ENTITYNUM_NONE || !SectionActive(root) ||
        n < 0 || n >= ENTITYNUM_WORLD || !target->inuse || objects[n].copyPending ||
        !LaserDamageAllowed(objects[root].owner, n);
}

static void SectionFireUpdate(gentity_t *child, qboolean active) {
    child->s.modelindex2 = active ? FX_STATE_CONTINUOUS : FX_STATE_OFF;
    child->s.loopSound = active ? child->noise_index : 0;
}

static void SectionFireThink(gentity_t *child) {
    const int root = SectionFireParent(child);
    if (root == ENTITYNUM_NONE) {
        G_FreeEntity(child);
        return;
    }
    const qboolean active = SectionActive(root);
    SectionFireUpdate(child, active);
    if (active) {
        // Raven's runner handles native falloff, visibility and tick scheduling.
        fx_runner_think(child);
    } else {
        child->nextthink = level.time + SECTION_FIRE_PERIOD;
    }
}

static void SectionFireInit(gentity_t *child, int effectIndex, int soundIndex) {
    child->classname = "jar_section_fire";
    child->s.eType = ET_FX;
    child->s.modelindex = effectIndex;
    child->noise_index = soundIndex;
    child->spawnflags = 4;
    child->splashDamage = SECTION_FIRE_DAMAGE;
    child->splashRadius = SECTION_FIRE_RADIUS;
    child->s.speed = child->delay = SECTION_FIRE_PERIOD;
    child->think = SectionFireThink;
    child->nextthink = level.time + SECTION_FIRE_PERIOD;
    SectionFireUpdate(child, sandbox.integer != 0);
}
