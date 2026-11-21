// Production MP section lifecycle with deterministic engine boundaries.
static int sectionFixtureMain, sectionFixturePublications, sectionFixtureBrushCalls;
static int sectionFixtureRegistered;
static qboolean sectionFixtureMismatch;
static qboolean sectionFixtureEffectsFull, sectionFixtureModelsFull;
static char sectionFixtureConfig[7][MAX_QPATH];
static const char *sectionFixtureModels[] = {"#mp/duel4", "#mp/duel10", "#mp/duel7", "#mp/duel2", "#mp/duel5", "#mp/duel1", "#mp/duel8"};
static const unsigned sectionFixtureChecksums[] = {0xb030d7fcu, 0x2a30df23u, 0xa23dfe0eu, 0xd6bcea3bu, 0x1efc7e4eu, 0xe1233cdau, 0x98d438f5u};
static const int sectionFixtureCounts[] = {27, 60, 40, 18, 35, 25, 106};
static const int sectionFixtureModelCounts[] = {1, 1, 1, 2, 2, 2, 70};
static const int sectionFixtureModelOffsets[] = {0, 1, 2, 3, 5, 7, 9};
static const int sectionFixtureSpeakers[] = {4, 4, 0, 1, 0, 0, 2};

static int SectionFixtureProfile(const char *model) {
    for (int i = 0; i < ARRAY_LEN(sectionFixtureModels); ++i) {
        if (!strcmp(model, sectionFixtureModels[i])) return i;
    }
    assert(0);
    return 0;
}

qboolean JAR_MP_InspectSection(const struct gameImport_s *game, jarMPSectionKind_t kind,
                             jarMPSectionAdmission_t *data, char *reason, int size) {
    assert(game == trap && kind >= JAR_MP_SECTION_IMPERIAL && kind <= JAR_MP_SECTION_CITY);
    const int profile = kind - JAR_MP_SECTION_IMPERIAL;
    memset(data, 0, sizeof(*data));
    data->model = sectionFixtureModels[profile];
    VectorSet(data->mins, -64, -96, -32);
    VectorSet(data->maxs, 96 + profile * 48, 64, 128);
    data->checksum = sectionFixtureChecksums[profile];
    data->modelCount = sectionFixtureModelCounts[profile];
    data->childCount = sectionFixtureCounts[profile];
    data->children[0].kind = JAR_MP_SECTION_ENTRY;
    VectorSet(data->children[0].origin, 20, 12, 10);
    VectorSet(data->children[0].angles, 0, 20, 0);
    const qboolean fire = kind == JAR_MP_SECTION_RANCOR;
    if (kind == JAR_MP_SECTION_CITY) {
        for (int i = 1; i <= JAR_MP_CITY_BRUSHES; ++i) {
            jarMPSectionChild_t *block = &data->children[i];
            block->kind = JAR_MP_SECTION_BREAKABLE;
            block->inlineModel = i;
            block->health = sectionCityHealth[i - 1] - '0';
            block->spawnflags = 2048;
        }
        for (int i = 70; i <= 71; ++i) {
            data->children[i].kind = JAR_MP_SECTION_SPEAKER;
            VectorSet(data->children[i].origin, 0, 50, 100);
            Q_strncpyz(data->children[i].sound, "sound/ambience/prototype/windmix1.wav", MAX_QPATH);
        }
        return qtrue;
    }
    if (kind == JAR_MP_SECTION_BESPIN) {
        jarMPSectionChild_t *bobber = &data->children[1];
        bobber->kind = JAR_MP_SECTION_BOBBER;
        VectorSet(bobber->origin, 42, -16, 60);
        bobber->inlineModel = 1;
        bobber->period = SECTION_BOBBER_PERIOD;
        bobber->amplitude = 20;
        Q_strncpyz(bobber->model, "models/map_objects/bespin/twinpodcc.md3", MAX_QPATH);
        return qtrue;
    }
    if (kind >= JAR_MP_SECTION_GENERATOR) {
        data->children[1].kind = JAR_MP_SECTION_ENTRY;
        VectorSet(data->children[1].origin, 20, 100, 10);
        jarMPSectionChild_t *hurt = &data->children[2];
        hurt->kind = JAR_MP_SECTION_HURT;
        VectorCopy(data->children[0].origin, hurt->origin);
        VectorSet(hurt->mins, -60, -12, -20);
        VectorSet(hurt->maxs, 60, 12, 20);
        hurt->damage = kind == JAR_MP_SECTION_GENERATOR ? -1 : 50;
        hurt->spawnflags = kind == JAR_MP_SECTION_GENERATOR ? 8 : 16;
        if (kind == JAR_MP_SECTION_GENERATOR) {
            Q_strncpyz(hurt->sound, "*falling1.wav", MAX_QPATH);
            data->children[3].kind = JAR_MP_SECTION_SPEAKER;
            VectorSet(data->children[3].origin, 0, 80, 60);
            Q_strncpyz(data->children[3].sound, "sound/ambient/fixture.wav", MAX_QPATH);
        }
        return qtrue;
    }
    for (int i = 1; i <= (fire ? 6 : 4); ++i) {
        jarMPSectionChild_t *child = &data->children[i];
        child->kind = fire ? JAR_MP_SECTION_FIRE : JAR_MP_SECTION_SPEAKER;
        VectorSet(child->origin, 8 * i, 20, 40);
        if (fire) {
            child->angles[PITCH] = -90;
            Q_strncpyz(child->effect, "env/small_fire", MAX_QPATH);
            Q_strncpyz(child->sound, "sound/effects/torch_burning_lp.wav", MAX_QPATH);
        } else Q_strncpyz(child->sound, "sound/ambient/fixture.wav", MAX_QPATH);
    }
    return qtrue;
}

static int SectionFixtureRegistry(int index, char *name, int size, int *first, int *count, unsigned *checksum) {
    if (index == -1) {
        name[0] = 0;
        *first = 0;
        *count = sectionFixtureMain;
        *checksum = 101;
        return 1;
    }
    if (index < 0 || index >= sectionFixtureRegistered) return 0;
    Q_strncpyz(name, sectionFixtureModels[index], size);
    *first = sectionFixtureMain + sectionFixtureModelOffsets[index];
    *count = sectionFixtureModelCounts[index];
    *checksum = sectionFixtureMismatch ? 102 : sectionFixtureChecksums[index];
    return 1;
}

static void SectionFixtureConfigGet(int index, char *name, int size) {
    const int slot = index - (CS_BSP_MODELS + 1);
    if (sectionFixtureModelsFull && index > CS_MODELS && index < CS_MODELS + MAX_MODELS) {
        Q_strncpyz(name, "models/map_objects/bespin/Twinpodcc.md3", size);
        return;
    }
    if (sectionFixtureEffectsFull && index > CS_EFFECTS && index < CS_EFFECTS + MAX_FX) {
        Q_strncpyz(name, "env/Small_Fire", size);
        return;
    }
    Q_strncpyz(name, slot >= 0 && slot < ARRAY_LEN(sectionFixtureConfig) ? sectionFixtureConfig[slot] : "", size);
}

static void SectionFixtureConfigSet(int index, const char *name) {
    const int slot = index - (CS_BSP_MODELS + 1);
    assert(slot >= 0 && slot < sectionFixtureRegistered && !strcmp(name, sectionFixtureModels[slot]));
    ++sectionFixturePublications;
    Q_strncpyz(sectionFixtureConfig[slot], name, sizeof(sectionFixtureConfig[slot]));
}

static void SectionFixtureBrush(sharedEntity_t *root, const char *model) {
    if (model[0] == '*') {
        const int index = atoi(model + 1);
        const int cityFirst = sectionFixtureMain + sectionFixtureModelOffsets[6];
        const qboolean city = index > cityFirst && index <= cityFirst + JAR_MP_CITY_BRUSHES;
        assert(city || index == sectionFixtureMain + sectionFixtureModelOffsets[5] + 1);
        root->r.bmodel = qtrue;
        root->r.contents = city ? CONTENTS_SOLID : CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP | CONTENTS_TRANSLUCENT;
        root->s.modelindex = index;
        VectorSet(root->r.mins, -17, -17, -9);
        VectorSet(root->r.maxs, 17, 17, 9);
        if (city) { root->r.mins[0] += index - cityFirst; root->r.maxs[0] += index - cityFirst; }
        return;
    }
    const int slot = SectionFixtureProfile(model);
    assert(slot <= sectionFixtureRegistered);
    ++sectionFixtureBrushCalls;
    if (slot == sectionFixtureRegistered) ++sectionFixtureRegistered;
    root->r.bmodel = qtrue;
    root->r.contents = CONTENTS_SOLID;
    root->s.modelindex = sectionFixtureMain + sectionFixtureModelOffsets[slot];
    VectorSet(root->r.mins, -64, -96, -32);
    VectorSet(root->r.maxs, 96 + slot * 48, 64, 128);
}

static void SectionFixtureLink(sharedEntity_t *entity) {
    if (entity->s.generic1 == SECTION_MARKER || entity->s.eType == ET_SPEAKER || entity->s.eType == ET_FX) {
        const int model = entity->s.generic1 == SECTION_MARKER ? entity->s.modelindex :
                          g_entities[entity->s.otherEntityNum].s.modelindex;
        int slot = 0;
        while (slot < sectionFixtureRegistered && model != sectionFixtureMain + sectionFixtureModelOffsets[slot]) ++slot;
        assert(slot >= 0 && slot < sectionFixtureRegistered && !strcmp(sectionFixtureConfig[slot], sectionFixtureModels[slot]));
    }
    RigidLink(entity);
}

static vec3_t sectionFixtureHit;
static void SectionFixtureTrace(trace_t *trace, const vec3_t start, const vec3_t mins,
                                const vec3_t maxs, const vec3_t end, int pass,
                                int mask, int capsule, int flags, int lod) {
    RigidTrace(trace, start, mins, maxs, end, pass, mask, capsule, flags, lod);
    if (!mins && SectionRoot(aimed)) {
        trace->fraction = Distance(start, sectionFixtureHit) / Distance(start, end);
        VectorCopy(sectionFixtureHit, trace->endpos);
    }
}

static int SectionFixtureChildren(const gentity_t *root) {
    int count = 0, speakers = 0, fires = 0, hazards = 0, bobbers = 0;
    for (int child = sectionFirst[root->s.number]; child; child = sectionNext[child - 1]) {
        const int n = child - 1;
        gentity_t *e = &g_entities[n];
        assert(e->inuse && sectionParent[n] == root->s.number + 1 && !objects[n].owner);
        ++count;
        if (e->s.eType == ET_SPEAKER) {
            ++speakers;
            assert(e->s.loopSound == 1 && e->s.otherEntityNum == root->s.number && e->r.linked);
        } else if (e->s.eType == ET_FX) {
            ++fires;
            assert(e->s.modelindex == 1 && e->s.otherEntityNum == root->s.number && e->r.linked);
            assert(e->think && e->s.speed == 200 && e->splashDamage == 5 && e->splashRadius == 16);
        } else if (sectionRole[n] == JAR_MP_SECTION_BOBBER) {
            ++bobbers;
            assert(e->r.linked && e->r.bmodel && e->s.eType == ET_MOVER && e->s.modelindex == root->s.modelindex + 1);
            assert(e->s.modelindex2 == 1 && e->s.otherEntityNum == root->s.number);
            assert(e->s.pos.trType == TR_SINE && e->s.pos.trDuration == SECTION_BOBBER_PERIOD);
        } else {
            assert(e->r.svFlags & SVF_NOCLIENT);
            if (sectionRole[n] == JAR_MP_SECTION_HURT) {
                ++hazards;
                assert(e->touch && e->r.linked && !e->r.bmodel && e->r.contents == CONTENTS_TRIGGER);
            }
        }
    }
    const int profile = SectionFixtureProfile(root->model);
    assert(count == sectionFixtureCounts[profile] && speakers == sectionFixtureSpeakers[profile] && fires == (profile == 2 ? 6 : 0) && hazards == (profile == 3 || profile == 4) && bobbers == (profile == 5));
    return count;
}

static gentity_t *SectionFixtureHurt(int root) {
    for (int child = sectionFirst[root]; child; child = sectionNext[child - 1]) {
        if (sectionRole[child - 1] == JAR_MP_SECTION_HURT) return &g_entities[child - 1];
    }
    assert(0);
    return NULL;
}

static int sectionFixtureBobberSweeps, sectionFixtureCitySweeps;
static void SectionFixtureBobberTrace(trace_t *trace, const vec3_t start, const vec3_t mins,
                                      const vec3_t maxs, const vec3_t end, int pass,
                                      int mask, int capsule, int flags, int lod) {
    if (mins && SectionRoot(pass)) {
        gentity_t *child = SectionBobber(pass);
        if (child) { assert(!child->r.linked); ++sectionFixtureBobberSweeps; }
        if (!strcmp(g_entities[pass].model, "#mp/duel8")) {
            ++sectionFixtureCitySweeps;
            for (int i = sectionFirst[pass]; i; i = sectionNext[i - 1]) {
                if (sectionRole[i - 1] == JAR_MP_SECTION_BREAKABLE) assert(!g_entities[i - 1].r.linked);
            }
        }
    }
    RigidTrace(trace, start, mins, maxs, end, pass, mask, capsule, flags, lod);
}

static gentity_t *SectionFixtureCityBlock(int root, int model) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        if (sectionRole[child->s.number] == JAR_MP_SECTION_BREAKABLE && child->genericValue1 == model) return child;
    }
    assert(0);
    return NULL;
}

static void SectionPlacementCases(void) {
    // Offset retail BSP origins must not displace the hull along the surface.
    jarMPSectionAdmission_t placement = {0};
    VectorSet(placement.mins, -7232, 1344, -1472);
    VectorSet(placement.maxs, -3712, 2816, 1152);
    const vec3_t placementAngles = {0, 37, 0};
    trace_t surface = {0};
    VectorSet(surface.endpos, 5100, 2200, 40);
    VectorSet(surface.plane.normal, .6f, 0, .8f);
    vec3_t placed, low, high, center, contact, delta;
    SectionPlacement(&placement, placementAngles, &surface, placed);
    SectionHull(placement.mins, placement.maxs, placementAngles, low, high);
    for (int i = 0; i < 3; ++i) {
        center[i] = placed[i] + (low[i] + high[i]) * .5f;
        contact[i] = placed[i] + (surface.plane.normal[i] >= 0 ? low[i] : high[i]);
    }
    VectorSubtract(center, surface.endpos, delta);
    VectorMA(delta, -DotProduct(delta, surface.plane.normal), surface.plane.normal, delta);
    assert(VectorLength(delta) < .005f);
    VectorSubtract(contact, surface.endpos, delta);
    assert(fabsf(DotProduct(delta, surface.plane.normal) - 4) < .005f);
    puts("MP room placement: offset Generator hull centers on a rotated, sloped surface.");
}

static int sectionPickupRoot, sectionPickupTraces;
static void SectionFixturePickupTrace(trace_t *trace, const vec3_t start, const vec3_t mins,
                                     const vec3_t maxs, const vec3_t end, int pass,
                                     int mask, int capsule, int flags, int lod) {
    assert(pass == sectionPickupRoot && mask == MASK_SOLID && flags == JAR_TRACE_SINGLE_BRUSH);
    assert(!capsule && !lod && fabsf(maxs[2] - 15.9f) < .01f && mins[2] == 0);
    assert(fabsf(start[0] - end[0] - 4096) < .01f && fabsf(start[2] - end[2]) < .01f);
    memset(trace, 0, sizeof(*trace));
    ++sectionPickupTraces;
    trace->startsolid = sectionPickupTraces == 1;
    trace->fraction = 16.f / 4096;
    vec3_t direction;
    VectorSubtract(end, start, direction);
    VectorMA(start, trace->fraction, direction, trace->endpos);
}

static void SectionPickupCases(void) {
    RigidFixture();
    gentity_t *root = G_Spawn(), *pickup = G_Spawn();
    Track(root, 0, -1);
    root->s.eType = ET_MOVER;
    root->s.generic1 = SECTION_MARKER;
    root->r.bmodel = qtrue;
    VectorSet(root->r.currentAngles, 90, 0, 0);
    sectionPickupRoot = root->s.number;
    sectionFirst[root->s.number] = pickup->s.number + 1;
    sectionParent[pickup->s.number] = root->s.number + 1;
    sectionRole[pickup->s.number] = JAR_MP_SECTION_PICKUP;
    VectorSet(pickup->s.origin, 1024, 2048, 512);
    VectorSet(pickup->r.mins, -8, -8, 0);
    VectorSet(pickup->r.maxs, 8, 8, 16);
    imports.Trace = SectionFixturePickupTrace;
    sectionPickupTraces = 0;
    assert(JAR_MP_SectionPickup(pickup) && JAR_MP_PlaceSectionPickup(pickup));
    const vec3_t expected = {1016, 2048, 512};
    assert(Distance(pickup->r.currentOrigin, expected) < .01f && sectionPickupTraces == 2);
    assert(pickup->s.groundEntityNum == ENTITYNUM_WORLD && pickup->r.maxs[2] == 16);
    G_FreeEntity(root);
}

static void RiftCompoundCases(void) {
    RigidFixture();
    gentity_t *root = G_Spawn();
    Track(root, 0, -1);
    root->s.eType = ET_MOVER;
    root->s.generic1 = SECTION_MARKER;
    root->r.bmodel = qtrue;
    gentity_t *compound = G_Spawn(), *single = G_Spawn();
    sectionFirst[root->s.number] = compound->s.number + 1;
    sectionNext[compound->s.number] = single->s.number + 1;
    sectionNext[single->s.number] = 0;
    sectionParent[compound->s.number] = sectionParent[single->s.number] = root->s.number + 1;
    sectionRole[compound->s.number] = sectionRole[single->s.number] = JAR_MP_SECTION_HURT;
    jarMPSectionChild_t hazard = {0};
    hazard.damage = -1;
    hazard.spawnflags = 8;
    VectorSet(hazard.mins, 4608, 1536, -2176);
    VectorSet(hazard.maxs, 7296, 2688, -512);
    VectorSet(hazard.secondMins, 4608, 2688, -2176);
    VectorSet(hazard.secondMaxs, 9120, 5632, -512);
    SectionHurtInit(compound, &hazard);
    VectorClear(hazard.secondMins);
    VectorClear(hazard.secondMaxs);
    VectorSet(hazard.mins, 8576, 960, -2176);
    VectorSet(hazard.maxs, 10240, 2688, -512);
    SectionHurtInit(single, &hazard);

    const vec3_t origin = {10000, 12000, 3000}, angles = {0, 45, 0};
    const vec3_t local[] = {{5000, 2000, -1000}, {8000, 3500, -1000},
                            {8000, 2000, -1000}, {9000, 2000, -1000}};
    SectionPublish(root, origin, angles);
    gentity_t *actor = &g_entities[1];
    VectorSet(actor->r.mins, -2, -2, -2);
    VectorSet(actor->r.maxs, 2, 2, 2);
    for (int i = 0; i < ARRAY_LEN(local); ++i) {
        vec3_t contact;
        SectionOffset(angles, local[i], contact);
        VectorAdd(origin, contact, contact);
        assert(SectionHurtContact(compound, actor, contact) == (i < 2));
        assert(SectionHurtContact(single, actor, contact) == (i == 3));
        assert(SectionHurtEntryClear(root, actor, contact) == (i == 2));
    }
    G_FreeEntity(root);
}

static void SectionCases(void) {
    RiftCompoundCases();
    SectionPlacementCases();
    SectionPickupCases();
    RigidFixture();
    imports.GetConfigstring = SectionFixtureConfigGet;
    imports.SetConfigstring = SectionFixtureConfigSet;
    imports.BSPModelAt = SectionFixtureRegistry;
    imports.SetBrushModel = SectionFixtureBrush;
    imports.LinkEntity = SectionFixtureLink;
    gentity_t *p = &g_entities[0];
    const unsigned id = ControllerId("Imperial Control Room");
    const unsigned vjunId = ControllerId("Vjun Fuel Processing");
    const unsigned rancorId = ControllerId("Rancor Pit");
    const unsigned generatorId = ControllerId("Generator Room");
    const unsigned taspirId = ControllerId("Taspir Landing");
    const unsigned bespinId = ControllerId("Bespin Courtyard");
    const unsigned cityId = ControllerId("Abandoned City");
    assert(cityId == SECTION_CITY_ID);
    assert(id == 99974636 && vjunId == 99392891 && rancorId == 96116923);
    assert(generatorId == 99006637 && taspirId == 89725001 && bespinId == 98485091);
    const vec3_t point = {1000, 1000, 100}, angle = {0, 0, 0};

    sectionFixtureMain = CM_MAX_INLINE_MODEL_COUNT;
    const int live = LiveCount();
    assert(!SpawnAt(p, id, point, angle) && LiveCount() == live && !sectionFixtureBrushCalls);
    sectionFixtureMain = 3;
    gentity_t *root = SpawnAt(p, id, point, angle);
    assert(root && SectionRoot(root->s.number) && SectionFixtureChildren(root) == 27);
    assert(root->s.modelindex == 3 && root->r.svFlags & SVF_BROADCAST);
    assert(!(root->s.eFlags & EF_PERMANENT) && sectionFixturePublications == 1);
    const int rootIndex = root->s.number;

    gentity_t *first = &g_entities[sectionFirst[rootIndex] - 1];
    assert(sectionRole[first->s.number] == JAR_MP_SECTION_ENTRY);
    vec3_t moved = {1400, 1600, 180}, turned = {0, 90, 0};
    assert(SectionPose(root, moved, turned, qtrue));
    vec3_t expected = {1388, 1620, 190};
    assert(Distance(first->r.currentOrigin, expected) < .01f);
    p->client->ps.groundEntityNum = rootIndex;
    assert(!SectionPose(root, point, angle, qtrue));
    assert(RemovalOccupied(root) && !(ContextCaps(p, rootIndex) & JAR_CONTEXT_REMOVE));
    Undo(p, qfalse);
    assert(root->inuse && !WorldClear());
    p->client->ps.groundEntityNum = ENTITYNUM_NONE;
    assert(!RemovalOccupied(root));
    blocked = 1;
    assert(!SectionPose(root, point, angle, qfalse));
    vec3_t actorBefore;
    VectorCopy(p->client->ps.origin, actorBefore);
    ControllerShot(p, rootIndex, qfalse);
    assert(VectorCompare(p->client->ps.origin, actorBefore));
    blocked = 0;
    ControllerShot(p, rootIndex, qfalse);
    expected[2] += 1;
    assert(Distance(p->client->ps.origin, expected) < .01f);

    // A distant off-center grab uses the struck point as its beam endpoint and pivot.
    const vec3_t hit = {1400, 1550, 225};
    vec3_t eye = {-1100, 1550, 225}, playerOrigin;
    VectorCopy(eye, playerOrigin);
    playerOrigin[2] -= p->client->ps.viewheight;
    VectorCopy(hit, sectionFixtureHit);
    Move(p, playerOrigin);
    VectorClear(p->client->ps.viewangles);
    imports.Trace = SectionFixtureTrace;
    ActorArm(0, rootIndex);
    assert(players[0].held == rootIndex && players[0].distance == 2500);
    ActorTick();
    assert(Distance(root->r.currentOrigin, moved) < .01f);
    assert(Distance(g_entities[players[0].beam].s.origin2, hit) < .01f);
    Command(0, "jar_distance", "out");
    ActorTick();
    assert(players[0].distance == 2524 && fabsf(root->r.currentOrigin[0] - moved[0] - 24) < .01f);
    vec3_t pivot;
    VectorCopy(g_entities[players[0].beam].s.origin2, pivot);
    ActorTurnInput(p,10,0);
    for(int i=0;i<60;++i)ActorTick();
    assert(fabsf(root->r.currentAngles[YAW] - 100) < .01f &&
           Distance(g_entities[players[0].beam].s.origin2, pivot) < .01f && p->client->ps.viewangles[YAW]==0);
    JAR_MP_Fire(p, qtrue);
    assert(players[0].held == ENTITYNUM_NONE && objects[rootIndex].frozen);
    assert(SectionPose(root, moved, turned, qfalse));
    imports.Trace = RigidTrace;

    sectionFixtureMismatch = qtrue;
    const int before = LiveCount();
    assert(!SpawnAt(p, id, point, angle) && LiveCount() == before && sectionFixturePublications == 1);
    sectionFixtureMismatch = qfalse;
    CopyAssembly(p, rootIndex);
    const vec3_t duplicate = {3000, 2000, 180};
    assert(copies[0].count == 1 && PlaceCopy(p, &copies[0], duplicate, NULL, NULL));
    const int copyIndex = players[0].selected;
    assert(copyIndex != rootIndex && SectionFixtureChildren(&g_entities[copyIndex]) == 27);
    assert(sectionFixturePublications == 1 && g_entities[copyIndex].s.modelindex == root->s.modelindex);
    Undo(p, qfalse);
    assert(!g_entities[copyIndex].inuse && !sectionFirst[copyIndex] && SectionRoot(rootIndex));

    const vec3_t vjunPosition = {6000, 8000, 250};
    gentity_t *vjun = SpawnAt(p, vjunId, vjunPosition, angle);
    assert(vjun && vjun->s.modelindex == 4 && vjun->r.maxs[0] == 144 && SectionFixtureChildren(vjun) == 60);
    assert(sectionFixturePublications == 2);
    const int vjunIndex = vjun->s.number;
    const int vjunChild = sectionFirst[vjunIndex] - 1;

    const vec3_t rancorPosition = {9000, 12000, 450};
    sectionFixtureEffectsFull = qtrue;
    const int beforeRancor = LiveCount();
    assert(!SpawnAt(p, rancorId, rancorPosition, angle) && LiveCount() == beforeRancor && sectionFixturePublications == 2);
    sectionFixtureEffectsFull = qfalse;
    gentity_t *rancor = SpawnAt(p, rancorId, rancorPosition, angle);
    assert(rancor && rancor->s.modelindex == 5 && SectionFixtureChildren(rancor) == 40);
    const int rancorIndex = rancor->s.number;
    const int torchIndex = sectionNext[sectionFirst[rancorIndex] - 1] - 1;
    gentity_t *torch = &g_entities[torchIndex];
    assert(JAR_MP_SectionFireSource(torch) && torch->s.loopSound == 1 && torch->s.modelindex2 == FX_STATE_CONTINUOUS);
    assert(!JAR_MP_SectionRadiusDenied(torch, &g_entities[1]));
    players[1].noTools = qtrue;
    assert(JAR_MP_SectionRadiusDenied(torch, &g_entities[1]));
    players[1].noTools = qfalse;
    playerTools.integer = 0;
    assert(JAR_MP_SectionRadiusDenied(torch, &g_entities[1]));
    playerTools.integer = 1;

    const vec3_t rancorTilt = {20, 30, 0};
    assert(SectionPose(rancor, rancorPosition, rancorTilt, qtrue));
    vec3_t torchOffset, torchPosition, torchDirection, rootUp;
    SectionOffset(rancorTilt, torch->pos1, torchOffset);
    VectorAdd(rancorPosition, torchOffset, torchPosition);
    AngleVectors(torch->s.angles, torchDirection, NULL, NULL);
    AngleVectors(rancorTilt, NULL, NULL, rootUp);
    assert(Distance(torch->r.currentOrigin, torchPosition) < .01f && Distance(torchDirection, rootUp) < .001f);
    const int calls = sectionFireRadiusCalls;
    torch->think(torch);
    assert(sectionFireRadiusCalls == calls + 1 && torch->nextthink == level.time + 200);
    sandbox.integer = 0;
    torch->think(torch);
    assert(sectionFireRadiusCalls == calls + 1 && torch->s.loopSound == 0 && torch->s.modelindex2 == FX_STATE_OFF);
    sandbox.integer = 1;
    torch->think(torch);
    assert(sectionFireRadiusCalls == calls + 2 && torch->s.loopSound == 1 && torch->s.modelindex2 == FX_STATE_CONTINUOUS);

    jarControllerState_t disabled = {0};
    disabled.inactive = 1;
    assert(ControllerStateApply(rancorIndex, &disabled));
    assert(JAR_MP_SectionRadiusDenied(torch, &g_entities[1]) && !torch->s.loopSound && torch->s.modelindex2 == FX_STATE_OFF);
    torch->think(torch);
    assert(sectionFireRadiusCalls == calls + 2);

    const vec3_t generatorPosition = {10000, 17000, 600}, taspirPosition = {15000, 19000, 650};
    const vec3_t hazardAngles = {0, 45, 0};
    sectionFixtureMain = CM_MAX_INLINE_MODEL_COUNT - 4;
    const int beforeHazard = LiveCount();
    assert(!SpawnAt(p, generatorId, generatorPosition, hazardAngles));
    assert(LiveCount() == beforeHazard && sectionFixturePublications == 3);
    sectionFixtureMain = 3;
    gentity_t *generator = SpawnAt(p, generatorId, generatorPosition, hazardAngles);
    gentity_t *taspir = SpawnAt(p, taspirId, taspirPosition, hazardAngles);
    assert(generator && generator->s.modelindex == 6 && SectionFixtureChildren(generator) == 18);
    assert(taspir && taspir->s.modelindex == 8 && SectionFixtureChildren(taspir) == 35);
    gentity_t *fall = SectionFixtureHurt(generator->s.number);
    gentity_t *burn = SectionFixtureHurt(taspir->s.number);
    const int fallIndex = fall->s.number, burnIndex = burn->s.number;
    assert(fall->damage == -1 && fall->spawnflags == 8 && burn->damage == 50 && burn->spawnflags == 16);

    gentity_t *victim = &g_entities[1];
    victim->takedamage = qtrue;
    VectorSet(victim->r.mins, -2, -2, -2);
    VectorSet(victim->r.maxs, 2, 2, 2);
    Move(victim, fall->r.currentOrigin);
    const int falls = sectionFallSounds;
    fall->touch(fall, victim, NULL);
    assert(victim->client->ps.fallingToDeath == level.time && sectionFallSounds == falls + 1);
    assert(victim->client->ps.eFlags & EF_RAG);
    victim->client->ps.fallingToDeath = 0;
    victim->client->ps.eFlags &= ~EF_RAG;

    vec3_t broadCorner;
    VectorCopy(burn->r.currentOrigin, broadCorner);
    broadCorner[0] += 40;
    broadCorner[1] -= 40;
    assert(burn->r.mins[0] < -40 && burn->r.maxs[1] > 40);
    Move(victim, broadCorner);
    const int initialHurtCalls = hurtCalls;
    burn->touch(burn, victim, NULL);
    assert(hurtCalls == initialHurtCalls);

    // The trigger path samples playerstate after movement, before relinking.
    VectorCopy(burn->r.currentOrigin, victim->client->ps.origin);
    assert(VectorCompare(victim->r.currentOrigin, broadCorner));
    players[1].noTools = qtrue;
    burn->touch(burn, victim, NULL);
    assert(hurtCalls == initialHurtCalls);
    players[1].noTools = qfalse;
    burn->touch(burn, victim, NULL);
    assert(hurtCalls == initialHurtCalls + 1 && hurtAmount == 50 && hurtVictim == 1);
    assert(SectionHurtRemaining(taspir->s.number) == 1000);
    level.time += 999;
    burn->touch(burn, victim, NULL);
    assert(hurtCalls == initialHurtCalls + 1 && SectionHurtRemaining(taspir->s.number) == 1);
    ++level.time;
    burn->touch(burn, victim, NULL);
    assert(hurtCalls == initialHurtCalls + 2 && hurtAmount == 50);

    assert(!SectionHurtEntryClear(taspir, p, burn->r.currentOrigin));
    ControllerShot(p, taspir->s.number, qfalse);
    vec3_t safeArrival;
    const int safeEntry = sectionNext[sectionFirst[taspir->s.number] - 1] - 1;
    VectorCopy(g_entities[safeEntry].r.currentOrigin, safeArrival);
    ++safeArrival[2];
    assert(Distance(p->client->ps.origin, safeArrival) < .01f);
    const int remaining = SectionHurtRemaining(taspir->s.number);
    assert(remaining > 0 && remaining <= 1000);
    CopyAssembly(p, taspir->s.number);
    const vec3_t copyLocation = {20000, 22000, 600};
    assert(copies[0].count == 1 && copies[0].node[0].controllerRemaining == remaining);
    assert(PlaceCopy(p, &copies[0], copyLocation, NULL, NULL));
    const int copied = players[0].selected;
    assert(copied != taspir->s.number && SectionHurtRemaining(copied) == remaining);
    Undo(p, qfalse);
    assert(!SectionRoot(copied) && SectionHurtRemaining(taspir->s.number) == remaining);

    const vec3_t bespinPosition = {21000, 26000, 750}, relocated = {23000, 28000, 900};
    const vec3_t cloudAngles = {20, 30, 0};
    sectionFixtureModelsFull = qtrue;
    const int beforeCloud = LiveCount();
    assert(!SpawnAt(p, bespinId, bespinPosition, angle));
    assert(LiveCount() == beforeCloud && sectionFixturePublications == 5);
    sectionFixtureModelsFull = qfalse;
    gentity_t *bespin = SpawnAt(p, bespinId, bespinPosition, angle);
    assert(bespin && bespin->s.modelindex == 10 && SectionFixtureChildren(bespin) == 25);
    const int bespinIndex = bespin->s.number;
    gentity_t *cloud = SectionBobber(bespinIndex);
    const int cloudIndex = cloud->s.number;
    assert(SectionToolTarget(cloudIndex) == bespinIndex && JAR_MP_SectionMoverProtected(cloud, NULL));
    assert(!JAR_MP_SectionMoverProtected(cloud, victim));
    players[1].noTools = qtrue;
    assert(JAR_MP_SectionMoverProtected(cloud, victim));
    players[1].noTools = qfalse;

    aimed = cloudIndex;
    Command(0, "jar_select", "");
    assert(players[0].selected == bespinIndex);
    const int cloudEpoch = cloud->s.pos.trTime;
    level.time = cloudEpoch + 750;
    victim->client->ps.groundEntityNum = cloudIndex;
    assert(!SectionPose(bespin, relocated, cloudAngles, qtrue));
    victim->client->ps.groundEntityNum = ENTITYNUM_NONE;
    imports.Trace = SectionFixtureBobberTrace;
    blocked = 1;
    assert(!SectionPose(bespin, relocated, cloudAngles, qtrue) && cloud->r.linked);
    blocked = 0;
    assert(SectionPose(bespin, relocated, cloudAngles, qtrue));
    assert(sectionFixtureBobberSweeps == 2 && cloud->r.linked && cloud->s.pos.trTime == cloudEpoch);
    vec3_t rest, delta, position;
    SectionOffset(cloudAngles, cloud->pos1, rest);
    VectorAdd(rest, relocated, rest);
    AngleVectors(cloudAngles, NULL, NULL, delta);
    VectorMA(rest, 20, delta, position);
    assert(Distance(rest, cloud->s.pos.trBase) < .01f && Distance(position, cloud->r.currentOrigin) < .01f);
    assert(SectionBobberPhase(bespinIndex) == 750);
    imports.Trace = RigidTrace;

    CopyAssembly(p, bespinIndex);
    const vec3_t cloudCopyPosition = {26000, 30000, 850};
    assert(copies[0].count == 1 && copies[0].node[0].controllerRemaining == 750);
    assert(PlaceCopy(p, &copies[0], cloudCopyPosition, NULL, NULL));
    const int cloudCopy = players[0].selected;
    assert(cloudCopy != bespinIndex && SectionBobberPhase(cloudCopy) == 750 && SectionBobber(cloudCopy)->r.linked);
    Undo(p, qfalse);
    assert(!SectionRoot(cloudCopy) && SectionBobber(bespinIndex) == cloud);
    const int taspirRemaining = SectionHurtRemaining(taspir->s.number);

    const vec3_t cityStart = {30000, 32000, 950}, cityLocation = {34000, 35000, 1200}, cityTurn = {0, 90, 0};
    gentity_t *city = SpawnAt(p, cityId, cityStart, angle);
    assert(city && city->s.modelindex == 12);
    const int cityIndex = city->s.number;
    for (int i = 1; i <= JAR_MP_CITY_BRUSHES; ++i) {
        gentity_t *block = SectionFixtureCityBlock(cityIndex, i);
        assert(SectionCityLive(block) && block->r.linked && block->r.bmodel && block->die);
        assert(block->health == sectionCityHealth[i - 1] - '0');
        assert(block->s.otherEntityNum == cityIndex && SectionToolTarget(block->s.number) == cityIndex);
    }
    gentity_t *destroyed = SectionFixtureCityBlock(cityIndex, 1);
    gentity_t *partial = SectionFixtureCityBlock(cityIndex, 3);
    assert(!JAR_MP_SectionCityDamageBlocked(partial));
    sandbox.integer = 0;
    assert(JAR_MP_SectionCityDamageBlocked(partial));
    sandbox.integer = 1;
    partial->health -= 1;
    destroyed->health = 0;
    destroyed->die(destroyed, p, p, 1, MOD_SABER);
    assert(!destroyed->r.linked && !destroyed->r.contents && !SectionCityLive(destroyed));
    assert(partial->health == 1 && SectionCityLive(partial));
    victim->client->ps.groundEntityNum = partial->s.number;
    assert(!SectionPose(city, cityLocation, cityTurn, qtrue));
    victim->client->ps.groundEntityNum = ENTITYNUM_NONE;
    imports.Trace = SectionFixtureBobberTrace;
    blocked = 1;
    assert(!SectionPose(city, cityLocation, cityTurn, qtrue) && partial->r.linked && !destroyed->r.linked);
    blocked = 0;
    assert(SectionPose(city, cityLocation, cityTurn, qtrue) && sectionFixtureCitySweeps == 2);
    assert(partial->r.linked && !destroyed->r.linked && VectorCompare(partial->r.currentOrigin, cityLocation));
    const vec3_t cityCenter = {34000, 35003, 1200}, outsideBlock = {34024, 35003, 1200};
    vec3_t actualCenter;
    float distance;
    assert(JAR_MP_SectionCityDamageCenter(partial, actualCenter) && Distance(actualCenter, cityCenter) < .01f);
    assert(JAR_MP_SectionCityDamageDistance(partial, cityCenter, &distance) && distance < .01f);
    assert(JAR_MP_SectionCityDamageDistance(partial, outsideBlock, &distance) && fabsf(distance - 7) < .01f);
    assert(!JAR_MP_SectionCityDamageDistance(destroyed, cityCenter, &distance));

    partial->pain(partial, p, 1);
    const gentity_t *debris = NULL;
    for (int n = MAX_CLIENTS; n < level.num_entities; ++n) {
        const gentity_t *event = &g_entities[n];
        if (!event->inuse || event->s.eType != ET_EVENTS + EV_DEBRIS ||
            event->s.generic1 != JAR_MP_CITY_BRUSH_MARKER || Distance(event->s.origin, cityCenter) > .01f) continue;
        assert(!debris);
        debris = event;
    }
    assert(debris && debris->freeAfterEvent && debris->s.eventParm >= 1 && debris->s.eventParm <= 3);
    assert(debris->s.trickedentindex == MAT_LT_STONE && Distance(debris->s.apos.trDelta, cityTurn) < .01f);
    assert(VectorCompare(debris->s.angles2, partial->r.mins) && VectorCompare(debris->s.origin2, partial->r.maxs));
    jarCityDebrisFrame_t debrisFrame;
    JAR_MP_CityDebrisFrame(debris->s.origin, debris->s.apos.trDelta, debris->s.angles2, debris->s.origin2, &debrisFrame);
    vec3_t sample = {6, -8, 3};
    const vec3_t expectedSample = {34008, 35006, 1203};
    JAR_MP_CityDebrisPoint(&debrisFrame, sample);
    assert(Distance(sample, expectedSample) < .01f);

    imports.Trace = RigidTrace;
    CopyAssembly(p, cityIndex);
    const vec3_t intactLocation = {38000, 39000, 1200};
    assert(copies[0].count == 1 && PlaceCopy(p, &copies[0], intactLocation, NULL, NULL));
    const int cityCopy = players[0].selected;
    assert(cityCopy != cityIndex && SectionCityLive(SectionFixtureCityBlock(cityCopy, 1)));
    assert(SectionFixtureCityBlock(cityCopy, 3)->health == 2);
    Undo(p, qfalse);
    assert(!SectionRoot(cityCopy) && !SectionCityLive(destroyed) && partial->health == 1);
    assert(PlaceCopy(p, &copies[0], intactLocation, NULL, NULL));
    const int secondCity = players[0].selected;
    gentity_t *lastBrush = SectionFixtureCityBlock(secondCity, JAR_MP_CITY_BRUSHES);
    assert(lastBrush->health == 3);
    --lastBrush->health;

    static jarWorld_t world, decoded;
    static byte data[WORLD_BYTES];
    assert(WorldCapture(&world) && world.copy.count == 8);
    assert(world.copy.node[0].id != SECTION_CITY_ID);
    world.copy.node[0].cityDamage[0] = 1;
    assert(!WorldEncode(&world, data));
    world.copy.node[0].cityDamage[0] = 0;
    const int length = WorldEncode(&world, data);
    assert(length && WorldDecode(data, length, &decoded));
    const int trailer = length - 4 - 2 * JAR_MP_CITY_DAMAGE_BYTES;
    assert(length == WORLD_HEADER + 8 * WORLD_NODE + 2 * JAR_MP_CITY_DAMAGE_BYTES + 4 && data[3] == 'J');
    assert(data[trailer] == (1 | (1 << 4)) && data[length - 5] == 1);
    // Rehash malformed payloads so the reader reaches structural validation.
    data[trailer] |= 2;
    int hashAt = length - 4;
    WorldPut(data, &hashAt, WorldHash(data, hashAt));
    assert(!WorldDecode(data, length, &decoded));
    data[trailer] &= ~2;
    data[length - 5] |= 4;
    hashAt = length - 4;
    WorldPut(data, &hashAt, WorldHash(data, hashAt));
    assert(!WorldDecode(data, length, &decoded));
    assert(WorldEncode(&world, data) == length && WorldDecode(data, length, &decoded));
    assert(WorldClear() && !sectionFirst[rootIndex] && !g_entities[first->s.number].inuse);
    assert(!g_entities[vjunChild].inuse && !sectionFirst[vjunIndex]);
    assert(!g_entities[torchIndex].inuse && !sectionFirst[rancorIndex]);
    assert(!g_entities[fallIndex].inuse && !g_entities[burnIndex].inuse && !g_entities[cloudIndex].inuse);
    for (int i = 0; i < ARRAY_LEN(sectionFixtureModels); ++i) {
        assert(!strcmp(sectionFixtureConfig[i], sectionFixtureModels[i]));
    }
    assert(PlaceCopy(NULL, &decoded.copy, vec3_origin, decoded.velocity, decoded.angular));
    int restored = 0;
    for (int n = MAX_CLIENTS; n < level.num_entities; ++n) {
        if (!SectionRoot(n)) continue;
        ++restored;
        gentity_t *restoredRoot = &g_entities[n];
        assert(objects[n].owner == MAX_CLIENTS + 1);
        if (restoredRoot->s.modelindex == 3) {
            assert(SectionFixtureChildren(restoredRoot) == 27);
            assert(Distance(restoredRoot->r.currentOrigin, moved) < .01f && restoredRoot->r.currentAngles[YAW] == 90);
        } else if (restoredRoot->s.modelindex == 4) {
            assert(restoredRoot->r.maxs[0] == 144);
            assert(SectionFixtureChildren(restoredRoot) == 60 && VectorCompare(restoredRoot->r.currentOrigin, vjunPosition));
        } else if (restoredRoot->s.modelindex == 5) {
            assert(restoredRoot->r.maxs[0] == 192);
            assert(SectionFixtureChildren(restoredRoot) == 40 && VectorCompare(restoredRoot->r.currentOrigin, rancorPosition));
            assert(restoredRoot->flags & FL_INACTIVE);
            gentity_t *restoredTorch = &g_entities[sectionNext[sectionFirst[n] - 1] - 1];
            assert(restoredTorch->s.eType == ET_FX && !restoredTorch->s.loopSound && restoredTorch->s.modelindex2 == FX_STATE_OFF);
        } else if (restoredRoot->s.modelindex == 6) {
            assert(VectorCompare(restoredRoot->r.currentOrigin, generatorPosition));
            assert(SectionFixtureChildren(restoredRoot) == 18 && SectionHurtRemaining(n) == 0);
        } else if (restoredRoot->s.modelindex == 8) {
            assert(restoredRoot->s.modelindex == 8 && VectorCompare(restoredRoot->r.currentOrigin, taspirPosition));
            assert(SectionFixtureChildren(restoredRoot) == 35 && SectionHurtRemaining(n) == taspirRemaining);
        } else if (restoredRoot->s.modelindex == 10) {
            assert(restoredRoot->s.modelindex == 10 && VectorCompare(restoredRoot->r.currentOrigin, relocated));
            assert(SectionFixtureChildren(restoredRoot) == 25 && SectionBobberPhase(n) == 750);
            assert(Distance(SectionBobber(n)->r.currentOrigin, position) < .01f);
        } else {
            assert(restoredRoot->s.modelindex == 12);
            gentity_t *firstBlock = SectionFixtureCityBlock(n, 1);
            gentity_t *thirdBlock = SectionFixtureCityBlock(n, 3);
            if (VectorCompare(restoredRoot->r.currentOrigin, cityLocation)) {
                assert(!firstBlock->r.linked && !SectionCityLive(firstBlock));
                assert(thirdBlock->health == 1 && thirdBlock->r.linked && SectionCityLive(thirdBlock));
                assert(SectionFixtureCityBlock(n, JAR_MP_CITY_BRUSHES)->health == 3);
            } else {
                assert(VectorCompare(restoredRoot->r.currentOrigin, intactLocation));
                assert(firstBlock->r.linked && firstBlock->health == 1 && thirdBlock->health == 2);
                assert(SectionFixtureCityBlock(n, JAR_MP_CITY_BRUSHES)->health == 2);
            }
        }
    }
    assert(restored == 8 && WorldClear() && sectionFixturePublications == 7);
    imports.GetConfigstring = Config;
    imports.SetConfigstring = NULL;
    imports.BSPModelAt = NULL;
    imports.SetBrushModel = NULL;
    imports.LinkEntity = RigidLink;
    puts("MP portable sections: mixed brush publication, poses, blocked entry, native torch and hazard dispatch, rotated contact, cooldown and bobber phase restore, rider safety, consent, City native breaks, pristine copies and JRW19 damage passed.");
}
