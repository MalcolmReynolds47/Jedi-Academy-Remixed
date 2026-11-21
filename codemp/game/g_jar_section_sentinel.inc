// Owned section lifts, floor pads and pickups. GPL-2.0-only.
extern void InitMoverTrData(gentity_t *ent);
extern void SetMoverState(gentity_t *ent, moverState_t state, int time);
extern int G_ItemDisabled(gitem_t *item);

enum {
    SECTION_SENTINEL_ID = 89854302,
    SECTION_SHAFT_ID = 91270120, SECTION_RIFT_ID = 88177980, SECTION_KORRIBAN_ID = 100481508,
    SENTINEL_LIFT_DELAY = 200, SENTINEL_LIFT_WAIT = 3000, SENTINEL_START_LEAD = 50,
    SENTINEL_LIFTS = 10, SENTINEL_PADS = 5, SENTINEL_PICKUPS = 45,
    SENTINEL_LIFT_TIMER_MAX = 2000 + SENTINEL_START_LEAD,
    SENTINEL_PICKUP_MAX = 40000,
    SENTINEL_WORLD_LIMIT = (ENTITYNUM_WORLD - MAX_CLIENTS) / (JAR_MP_SENTINEL_CHILDREN + 1),
    SHAFT_LIFTS = 2, SHAFT_PADS = 2, SHAFT_WORLD_LIMIT = (ENTITYNUM_WORLD - MAX_CLIENTS) / 36,
    RIFT_PICKUPS = 38, RIFT_WORLD_LIMIT = (ENTITYNUM_WORLD - MAX_CLIENTS) / 123,
    KORRIBAN_WORLD_LIMIT = (ENTITYNUM_WORLD - MAX_CLIENTS) / (JAR_MP_KORRIBAN_CHILDREN + 1),
    YAVIN_WORLD_LIMIT = (ENTITYNUM_WORLD - MAX_CLIENTS) / (JAR_MP_YAVIN_CHILDREN + 1),
    YAVIN_STATE_BYTES = JAR_MP_YAVIN_DAMAGE_BYTES + 2
};

// The stock BSP order is stable. Each timer uses two little-endian bytes
// and carries remaining or elapsed milliseconds, independent of level.time.
typedef struct {
    byte lifts[SENTINEL_LIFTS][3];
    byte pads[SENTINEL_PADS][4];
    byte pickups[SENTINEL_PICKUPS][2];
} jarSentinelState_t;

typedef struct {
    byte lifts[SHAFT_LIFTS][3];
    byte pads[SHAFT_PADS][4];
} jarShaftState_t;

typedef struct {
    byte pickups[RIFT_PICKUPS][2];
} jarRiftState_t;

typedef struct {
    jarSentinelState_t sentinels[SENTINEL_WORLD_LIMIT];
    jarShaftState_t shafts[SHAFT_WORLD_LIMIT];
    jarRiftState_t rifts[RIFT_WORLD_LIMIT];
    byte korriban[KORRIBAN_WORLD_LIMIT][JAR_MP_KORRIBAN_PICKUPS][2];
    byte yavin[YAVIN_WORLD_LIMIT][YAVIN_STATE_BYTES];
} jarSectionStates_t;

static int SectionSentinelTime(const byte *data) {
    return data[0] | (data[1] << 8);
}

static void SectionSentinelWriteTime(byte *data, int time) {
    data[0] = time & 255;
    data[1] = time >> 8;
}

static int SectionSentinelRemaining(int deadline) {
    return deadline > level.time ? deadline - level.time : 0;
}

static qboolean SectionTimersValid(const byte (*lifts)[3], int liftCount, const byte (*pads)[4], int padCount,
                                   const byte (*pickups)[2], int pickupCount) {
    for (int i = 0; i < liftCount; ++i) {
        const int phase = lifts[i][0], timer = SectionSentinelTime(lifts[i] + 1);
        if (phase > MOVER_2TO1 || (phase == MOVER_POS1 && timer) ||
            (phase == MOVER_POS2 && (!timer || timer > SENTINEL_LIFT_WAIT)) ||
            (phase >= MOVER_1TO2 && timer > SENTINEL_LIFT_TIMER_MAX) ||
            (phase == MOVER_2TO1 && timer < SENTINEL_START_LEAD)) return qfalse;
    }
    for (int i = 0; i < padCount; ++i) {
        const int pending = SectionSentinelTime(pads[i]);
        const int cooldown = SectionSentinelTime(pads[i] + 2);
        if (pending > 2 * SENTINEL_LIFT_DELAY || cooldown > 9000 + SENTINEL_LIFT_DELAY ||
            pending > cooldown) return qfalse;
    }
    for (int i = 0; i < pickupCount; ++i)
        if (SectionSentinelTime(pickups[i]) > SENTINEL_PICKUP_MAX) return qfalse;
    return qtrue;
}

static int SectionLiftRoot(int n) {
    if (n < MAX_CLIENTS || n >= ENTITYNUM_WORLD || !g_entities[n].inuse ||
        sectionRole[n] != JAR_MP_SECTION_LIFT) return ENTITYNUM_NONE;
    const int root = sectionParent[n] - 1;
    return SectionRoot(root) ? root : ENTITYNUM_NONE;
}

qboolean JAR_MP_SectionPickup(const gentity_t *item) {
    const int n = item->s.number;
    return n >= MAX_CLIENTS && n < ENTITYNUM_WORLD && sectionRole[n] == JAR_MP_SECTION_PICKUP &&
        SectionRoot(sectionParent[n] - 1);
}

qboolean JAR_MP_PlaceSectionPickup(gentity_t *item) {
    const int root = sectionParent[item->s.number] - 1;
    vec3_t up;
    AngleVectors(g_entities[root].r.currentAngles, NULL, NULL, up);
    static const float lifts[] = { .1f, 8, 16, 24, 32 };
    for (int i = 0; i < ARRAY_LEN(lifts); ++i) {
        vec3_t start, end, top;
        VectorMA(item->s.origin, lifts[i], up, start);
        VectorMA(start, -4096, up, end);
        VectorCopy(item->r.maxs, top);
        top[2] -= .1f;
        trace_t trace;
        trap->Trace(&trace, start, item->r.mins, top, end, root, MASK_SOLID, qfalse,
                    JAR_TRACE_SINGLE_BRUSH, 0);
        if (trace.startsolid || trace.allsolid) continue;
        // Keep unsupported anchors inside the authored layout.
        G_SetOrigin(item, trace.fraction < 1 ? trace.endpos : start);
        item->s.groundEntityNum = ENTITYNUM_WORLD;
        return qtrue;
    }
    return qfalse;
}

static void SectionLiftSound(gentity_t *lift, int sound) {
    if (!sound || !Room()) return;
    vec3_t center, offset;
    VectorAdd(lift->r.mins, lift->r.maxs, center);
    VectorScale(center, .5f, center);
    SectionOffset(lift->r.currentAngles, center, offset);
    VectorAdd(lift->r.currentOrigin, offset, center);
    gentity_t *event = G_TempEntity(center, EV_GENERAL_SOUND);
    if (event) event->s.eventParm = sound;
}

static void SectionLiftMove(gentity_t *lift, moverState_t state, int time) {
    lift->think = NULL;
    lift->nextthink = 0;
    SetMoverState(lift, state, time);
    lift->s.loopSound = lift->genericValue4;
    SectionLiftSound(lift, lift->noise_index);
}

static void SectionLiftReturn(gentity_t *lift) {
    SectionLiftMove(lift, MOVER_2TO1, level.time);
}

static void SectionLiftReached(gentity_t *lift) {
    const qboolean opened = lift->moverState == MOVER_1TO2;
    SetMoverState(lift, opened ? MOVER_POS2 : MOVER_POS1, level.time);
    lift->s.loopSound = 0;
    SectionLiftSound(lift, lift->genericValue3);
    lift->think = opened ? SectionLiftReturn : NULL;
    lift->nextthink = opened ? level.time + SENTINEL_LIFT_WAIT : 0;
}

static void SectionLiftReverse(gentity_t *lift, moverState_t state) {
    const int duration = lift->s.pos.trDuration;
    const int elapsed = Com_Clampi(0, duration, level.time - lift->s.pos.trTime);
    SectionLiftMove(lift, state, level.time - (duration - elapsed));
}

static void SectionLiftBlocked(gentity_t *lift, gentity_t *obstacle) {
    if (!obstacle || JAR_MP_SectionMoverProtected(lift, obstacle)) return;
    G_Damage(obstacle, lift, lift, NULL, NULL, lift->damage, 0, MOD_CRUSH);
    if (!(lift->spawnflags & 4))
        SectionLiftReverse(lift, lift->moverState == MOVER_1TO2 ? MOVER_2TO1 : MOVER_1TO2);
}

static void SectionLiftActivate(gentity_t *lift) {
    switch (lift->moverState) {
    case MOVER_POS1: SectionLiftMove(lift, MOVER_1TO2, level.time + SENTINEL_START_LEAD); break;
    case MOVER_POS2: lift->nextthink = level.time + SENTINEL_LIFT_WAIT; break;
    case MOVER_1TO2: SectionLiftReverse(lift, MOVER_2TO1); break;
    case MOVER_2TO1: SectionLiftReverse(lift, MOVER_1TO2); break;
    default: break;
    }
}

static void SectionPadFire(gentity_t *pad) {
    const int root = sectionParent[pad->s.number] - 1;
    if (!SectionRoot(root) || !SectionActive(root)) return;
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        if (sectionRole[child->s.number] == JAR_MP_SECTION_LIFT &&
            child->genericValue1 == pad->genericValue1) SectionLiftActivate(child);
    }
}

static void SectionPadTouch(gentity_t *pad, gentity_t *actor, trace_t *trace) {
    const int root = sectionParent[pad->s.number] - 1;
    if (!actor || !actor->client || actor->health <= 0 || actor->client->sess.sessionTeam == TEAM_SPECTATOR ||
        !SectionRoot(root) || !SectionActive(root) || pad->nextthink || pad->timestamp > level.time) return;
    if (!pad->r.bmodel && !SectionBoxContact(pad, actor, actor->client->ps.origin, pad->s.origin2, pad->s.angles2)) return;
    pad->timestamp = level.time + SENTINEL_LIFT_DELAY + pad->genericValue2;
    pad->nextthink = level.time + 2 * SENTINEL_LIFT_DELAY;
    pad->think = SectionPadFire;
}

static qboolean SectionMoverInit(gentity_t *child, int root, int first, const jarMPSectionChild_t *record) {
    const qboolean boxPad = record->kind == JAR_MP_SECTION_PAD && record->maxs[2] > record->mins[2];
    if (boxPad) {
        VectorCopy(record->mins, child->s.origin2);
        VectorCopy(record->maxs, child->s.angles2);
    } else {
        const int index = first + record->inlineModel;
        trap->SetBrushModel((sharedEntity_t *)child, va("*%d", index));
        if (!child->r.bmodel || child->s.modelindex != index) return qfalse;
    }
    child->genericValue1 = record->group;
    if (record->kind == JAR_MP_SECTION_PAD) {
        child->classname = "jar_section_pad";
        child->r.contents = CONTENTS_TRIGGER;
        child->genericValue2 = record->period;
        child->touch = SectionPadTouch;
        return qtrue;
    }
    if (!(child->r.contents & CONTENTS_SOLID)) return qfalse;
    child->classname = "jar_section_lift";
    child->s.eType = ET_MOVER;
    child->s.otherEntityNum = root;
    child->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
    child->s.apos.trType = TR_STATIONARY;
    child->alt_fire = qtrue; // Stock linear func_door.
    child->speed = 500;
    child->spawnflags = record->spawnflags;
    child->damage = record->damage;
    child->moverState = MOVER_POS1;
    child->reached = SectionLiftReached;
    child->blocked = SectionLiftBlocked;
    child->radius = record->travel;
    if (*record->sound) {
        child->noise_index = G_SoundIndex(record->liftStart);
        child->genericValue4 = G_SoundIndex(record->sound);
        child->genericValue3 = G_SoundIndex(record->liftStop);
    }
    return qtrue;
}

static void SectionLiftPublish(gentity_t *lift, const vec3_t origin, const vec3_t up) {
    const int time = lift->s.pos.trTime;
    VectorMA(origin, -lift->radius, up, lift->pos1);
    VectorCopy(origin, lift->pos2);
    InitMoverTrData(lift);
    SetMoverState(lift, lift->moverState, time);
}

static qboolean SectionPickupInit(gentity_t *child, const jarMPSectionChild_t *record) {
    gitem_t *item = NULL;
    for (int i = 1; i < bg_numItems; ++i) if (bg_itemlist[i].classname &&
        !strcmp(bg_itemlist[i].classname, record->classname)) {
        item = &bg_itemlist[i];
        break;
    }
    if (!item) return qfalse;
    const qboolean duel = level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL;
    const int disabledWeapons = duel ? g_duelWeaponDisable.integer : g_weaponDisable.integer;
    if (G_ItemDisabled(item) || (item->giType == IT_WEAPON && level.gametype != GT_JEDIMASTER &&
        (disabledWeapons & (1u << item->giTag))) ||
        (item->giType == IT_AMMO && level.gametype != GT_JEDIMASTER && HasSetSaberOnly()) ||
        (duel && (item->giType == IT_ARMOR || item->giType == IT_HEALTH))) return qtrue;
    child->classname = item->classname;
    child->r.svFlags = SVF_BROADCAST;
    VectorCopy(child->r.currentOrigin, child->s.origin);
    const int oldCount = level.numSpawnVars;
    const qboolean oldSpawning = level.spawning;
    level.numSpawnVars = 0;
    level.spawning = qtrue;
    G_SpawnItem(child, item);
    level.numSpawnVars = oldCount;
    level.spawning = oldSpawning;
    if (!child->inuse || child->item != item) return qfalse;
    child->wait = record->period;
    child->think = NULL;
    child->nextthink = 0;
    FinishSpawningItem(child);
    if (child->inuse) child->s.otherEntityNum = sectionParent[child->s.number] - 1;
    return child->inuse && child->s.eType == ET_ITEM;
}

static void SectionPickupPublish(gentity_t *child, const vec3_t pos) {
    if (child->item) child->s.groundEntityNum = ENTITYNUM_WORLD;
    G_SetOrigin(child, (float *)pos);
    VectorCopy(pos, child->s.origin);
    if (child->item) trap->LinkEntity((sharedEntity_t *)child);
}

static void SectionTimersCapture(int root, byte (*lifts)[3], int liftCount, byte (*pads)[4], int padCount,
                                 byte (*pickups)[2], int pickupCount) {
    int lift = 0, pad = 0, pickup = 0;
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const gentity_t *child = &g_entities[next - 1];
        switch (sectionRole[child->s.number]) {
        case JAR_MP_SECTION_LIFT: {
            if (lift >= liftCount) return;
            byte *saved = lifts[lift++];
            saved[0] = child->moverState;
            int timer = 0;
            if (child->moverState == MOVER_POS2)
                timer = Com_Clampi(1, SENTINEL_LIFT_WAIT, SectionSentinelRemaining(child->nextthink));
            else if (child->moverState >= MOVER_1TO2)
                timer = SENTINEL_START_LEAD + Com_Clampi(-SENTINEL_START_LEAD, child->s.pos.trDuration,
                    level.time - child->s.pos.trTime);
            SectionSentinelWriteTime(saved + 1, timer);
            break;
        }
        case JAR_MP_SECTION_PAD: {
            if (pad >= padCount) return;
            byte *saved = pads[pad++];
            SectionSentinelWriteTime(saved, SectionSentinelRemaining(child->nextthink));
            SectionSentinelWriteTime(saved + 2, SectionSentinelRemaining(child->timestamp));
            break;
        }
        case JAR_MP_SECTION_PICKUP: {
            if (pickup >= pickupCount) return;
            byte *saved = pickups[pickup++];
            if (child->item && child->think == RespawnItem)
                SectionSentinelWriteTime(saved, Com_Clampi(1, SENTINEL_PICKUP_MAX,
                    SectionSentinelRemaining(child->nextthink)));
            break;
        }
        default: break;
        }
    }
}

static qboolean SectionTimersRestore(int root, const byte (*lifts)[3], int liftCount, const byte (*pads)[4], int padCount,
                                     const byte (*pickups)[2], int pickupCount) {
    int lift = 0, pad = 0, pickup = 0;
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        switch (sectionRole[child->s.number]) {
        case JAR_MP_SECTION_LIFT: {
            if (lift >= liftCount) return qfalse;
            const byte *saved = lifts[lift++];
            const moverState_t phase = (moverState_t)saved[0];
            const int timer = SectionSentinelTime(saved + 1);
            int start = level.time;
            if (phase >= MOVER_1TO2) {
                const int elapsed = timer - SENTINEL_START_LEAD;
                // A rotated pose can round the native travel duration by 1 ms.
                if (elapsed > child->s.pos.trDuration + 1) return qfalse;
                start -= elapsed;
                child->s.loopSound = child->genericValue4;
            } else if (phase == MOVER_POS2) {
                child->think = SectionLiftReturn;
                child->nextthink = level.time + timer;
            }
            SetMoverState(child, phase, start);
            break;
        }
        case JAR_MP_SECTION_PAD: {
            if (pad >= padCount) return qfalse;
            const byte *saved = pads[pad++];
            const int pending = SectionSentinelTime(saved), cooldown = SectionSentinelTime(saved + 2);
            if (cooldown > child->genericValue2 + SENTINEL_LIFT_DELAY) return qfalse;
            if (pending) {
                child->think = SectionPadFire;
                child->nextthink = level.time + pending;
            }
            child->timestamp = cooldown ? level.time + cooldown : 0;
            break;
        }
        case JAR_MP_SECTION_PICKUP: {
            if (pickup >= pickupCount) return qfalse;
            const int timer = SectionSentinelTime(pickups[pickup++]);
            if (!timer || !child->item) break;
            if (timer > child->wait * 1000) return qfalse;
            if (child->item->giType == IT_WEAPON) child->s.eFlags |= EF_ITEMPLACEHOLDER;
            else {
                child->s.eFlags |= EF_NODRAW;
                child->r.svFlags |= SVF_NOCLIENT;
            }
            child->r.contents = 0;
            child->think = RespawnItem;
            child->nextthink = level.time + timer;
            trap->LinkEntity((sharedEntity_t *)child);
            break;
        }
        default: break;
        }
    }
    return lift == liftCount && pad == padCount && pickup == pickupCount;
}
