// Portable stock BSP section with independently owned native children.
// GPL-2.0-only. Included by the MP Sandbox controller.
#include "jar_mp_section_scenery.h"
enum { SECTION_MARKER = 255, SECTION_AIM_RANGE = 4096, SECTION_LARGE_AIM_RANGE = 6144 };
static jarMPSectionKind_t SectionKind(const jarMPEntry_t *entry) {
    return entry && entry->kind == 5 ? JAR_MP_SectionKind(entry->name) : JAR_MP_SECTION_NONE;
}

static qboolean SectionCard(const jarMPEntry_t *entry) {
    return SectionKind(entry) != JAR_MP_SECTION_NONE;
}
static int sectionFirst[MAX_GENTITIES], sectionNext[MAX_GENTITIES], sectionParent[MAX_GENTITIES];
static byte sectionRole[MAX_GENTITIES];

static qboolean SectionRoot(int n) {
    return n >= MAX_CLIENTS && n < ENTITYNUM_WORLD && g_entities[n].inuse &&
        objects[n].owner && g_entities[n].s.eType == ET_MOVER &&
        g_entities[n].s.generic1 == SECTION_MARKER && g_entities[n].r.bmodel;
}

static qboolean SectionActive(int root) {
    return sandbox.integer && !(g_entities[root].flags & FL_INACTIVE) && !objects[root].copyPending;
}

#include "g_jar_section_fire.inc"
#include "g_jar_section_bobber.inc"

static void SectionFireSync(int root) {
    const qboolean active = SectionActive(root);
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] == JAR_MP_SECTION_FIRE) SectionFireUpdate(&g_entities[n], active);
    }
}

static void SectionReset(void) {
    memset(sectionFirst, 0, sizeof(sectionFirst));
    memset(sectionNext, 0, sizeof(sectionNext));
    memset(sectionParent, 0, sizeof(sectionParent));
    memset(sectionRole, 0, sizeof(sectionRole));
}

static void SectionForget(int n) {
    if (sectionParent[n]) {
        int *link = &sectionFirst[sectionParent[n] - 1];
        while (*link && *link != n + 1) link = &sectionNext[*link - 1];
        if (*link) *link = sectionNext[n];
        sectionParent[n] = sectionNext[n] = sectionRole[n] = 0;
    }
    while (sectionFirst[n]) {
        const int child = sectionFirst[n] - 1;
        sectionFirst[n] = sectionNext[child];
        sectionParent[child] = sectionNext[child] = sectionRole[child] = 0;
        G_FreeEntity(&g_entities[child]);
    }
}

static void SectionHull(const vec3_t lo, const vec3_t hi, const vec3_t angles,
                        vec3_t mins, vec3_t maxs) {
    vec3_t axis[3];
    AnglesToAxis(angles, axis);
    VectorClear(mins);
    VectorClear(maxs);
    for (int i = 0; i < 3; ++i) {
        for (int a = 0; a < 3; ++a) {
            const float direction = axis[a][i];
            mins[i] += direction * (direction >= 0 ? lo[a] : hi[a]);
            maxs[i] += direction * (direction >= 0 ? hi[a] : lo[a]);
        }
    }
}

static void SectionPlacement(const jarMPSectionAdmission_t *data, const vec3_t angles,
                             const trace_t *hit, vec3_t origin) {
    vec3_t mins, maxs, center;
    SectionHull(data->mins, data->maxs, angles, mins, maxs);
    const float *normal = hit->plane.normal;
    float clearance = 4;
    for (int axis = 0; axis < 3; ++axis) {
        center[axis] = (mins[axis] + maxs[axis]) * .5f;
        clearance -= normal[axis] * (normal[axis] >= 0 ? mins[axis] : maxs[axis]);
    }
    // The retail BSP's model origin can sit far from the visible room.
    // Center the rotated hull along the surface while keeping its clearance.
    VectorMA(hit->endpos, clearance + DotProduct(center, normal), normal, origin);
    VectorSubtract(origin, center, origin);
}

static void SectionOffset(const vec3_t angles, const vec3_t local, vec3_t offset) {
    vec3_t axis[3];
    AnglesToAxis(angles, axis);
    VectorClear(offset);
    for (int a = 0; a < 3; ++a) VectorMA(offset, local[a], axis[a], offset);
}

#include "g_jar_section_hurt.inc"
#include "g_jar_section_sentinel.inc"
#include "g_jar_section_yavin.inc"

enum { SECTION_CITY_ID = 88805698 };

// Indices 1 through 69 follow the exact retail inline brush order.
static const char sectionCityHealth[] =
    "112222223221223322111122211222222212232333333333333333332333333333333";

static const byte sectionYavinHealth[JAR_MP_YAVIN_BRUSHES] = {
    8, 4, 10, 6, 10, 8, 4, 6, 10, 8, 3, 10, 8, 6, 10, 8, 10, 4, 2, 3, 3, 4, 2, 3, 2, 5, 5, 5, 5, 5
};

static int SectionYavinDamageAt(const byte *damage, int index) {
    return (damage[index / 2] >> (4 * (index % 2))) & 15;
}

static qboolean SectionYavinStateValid(const byte *state) {
    for (int i = 0; i < JAR_MP_YAVIN_BRUSHES; ++i)
        if (SectionYavinDamageAt(state, i) > sectionYavinHealth[i]) return qfalse;
    return SectionSentinelTime(state + JAR_MP_YAVIN_DAMAGE_BYTES) <= SECTION_YAVIN_DUST_MAX;
}

static int SectionCityDamageAt(const byte *damage, int inlineModel) {
    const int brush = inlineModel - 1;
    return (damage[brush / 4] >> (2 * (brush % 4))) & 3;
}

static void SectionCitySetDamage(byte *damage, int inlineModel, int amount) {
    const int brush = inlineModel - 1, shift = 2 * (brush % 4);
    damage[brush / 4] = (damage[brush / 4] & ~(3 << shift)) | (amount << shift);
}

static qboolean SectionCityDamageValid(unsigned id, const byte *damage) {
    if (id != SECTION_CITY_ID) {
        for (int i = 0; i < JAR_MP_CITY_DAMAGE_BYTES; ++i) if (damage[i]) return qfalse;
        return qtrue;
    }
    if (damage[JAR_MP_CITY_DAMAGE_BYTES - 1] & ~3) return qfalse;
    for (int brush = 1; brush <= JAR_MP_CITY_BRUSHES; ++brush) {
        if (SectionCityDamageAt(damage, brush) > sectionCityHealth[brush - 1] - '0') return qfalse;
    }
    return qtrue;
}

#include "g_jar_section_city.inc"

static qboolean SectionSolidChild(const gentity_t *e) {
    return sectionRole[e->s.number] == JAR_MP_SECTION_BOBBER ||
        sectionRole[e->s.number] == JAR_MP_SECTION_LIFT || SectionCityLive(e);
}

static void SectionSolidChildrenLink(int root, qboolean link) {
    for (int next = sectionFirst[root]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        if (!SectionSolidChild(child) || !!child->r.linked == !!link) continue;
        if (link) trap->LinkEntity((sharedEntity_t *)child);
        else trap->UnlinkEntity((sharedEntity_t *)child);
    }
}

static qboolean SectionSupports(int root, int ground) {
    if (root == ground) return qtrue;
    return ground >= MAX_CLIENTS && ground < ENTITYNUM_WORLD &&
        sectionParent[ground] == root + 1 && SectionSolidChild(&g_entities[ground]);
}

static int SectionToolTarget(int n) {
    const int root = SectionBobberRoot(n);
    if (root != ENTITYNUM_NONE) return root;
    const int lift = SectionLiftRoot(n);
    if (lift != ENTITYNUM_NONE) return lift;
    const int city = SectionCityRoot(n);
    return city == ENTITYNUM_NONE ? n : city;
}

static qboolean SectionSpace(const vec3_t origin, const vec3_t mins, const vec3_t maxs) {
    for (int a = 0; a < 3; ++a) {
        if (!isfinite(origin[a]) || origin[a] + mins[a] < -65535 ||
            origin[a] + maxs[a] > 65535) return qfalse;
    }
    return qtrue;
}

static qboolean SectionClear(const vec3_t start, const vec3_t end,
                             const vec3_t mins, const vec3_t maxs, int pass) {
    trace_t trace;
    if (!SectionSpace(start, mins, maxs) || !SectionSpace(end, mins, maxs)) return qfalse;
    trap->Trace(&trace, start, mins, maxs, end, pass, MASK_PLAYERSOLID, qfalse, 0, 0);
    return !trace.startsolid && !trace.allsolid && trace.fraction == 1;
}

static qboolean SectionRiders(const gentity_t *root) {
    for (int i = 0; i < level.num_entities; ++i) {
        const gentity_t *e = &g_entities[i];
        if (!e->inuse) continue;
        if (SectionSupports(root->s.number, e->s.groundEntityNum) ||
            (e->client && SectionSupports(root->s.number, e->client->ps.groundEntityNum))) return qtrue;
    }
    return qfalse;
}

static qboolean SectionRemovalSupport(int root, int ground) {
    return ground >= MAX_CLIENTS && ground < level.num_entities &&
           g_entities[ground].inuse && g_entities[ground].r.linked && SectionSupports(root, ground);
}

static qboolean SectionSupportsActors(const gentity_t *root, int departingClient) {
    if (!SectionRoot(root->s.number)) return qfalse;
    for (int i = 0; i < level.num_entities; ++i) {
        const gentity_t *actor = &g_entities[i];
        if (i == departingClient || !actor->inuse || !actor->client) continue;
        if (SectionRemovalSupport(root->s.number, actor->s.groundEntityNum) ||
            SectionRemovalSupport(root->s.number, actor->client->ps.groundEntityNum) ||
            (actor->m_pVehicle && actor->m_pVehicle->m_LandTrace.fraction < 1 &&
             SectionRemovalSupport(root->s.number, actor->m_pVehicle->m_LandTrace.entityNum))) return qtrue;
    }
    return qfalse;
}

static void SectionPublish(gentity_t *root, const vec3_t origin, const vec3_t angles) {
    G_SetOrigin(root, (float *)origin);
    VectorCopy(origin, root->s.origin);
    VectorCopy(angles, root->r.currentAngles);
    VectorCopy(angles, root->s.angles);
    VectorCopy(angles, root->s.apos.trBase);
    root->s.apos.trType = TR_STATIONARY;
    trap->LinkEntity((sharedEntity_t *)root);
    vec3_t rootAxis[3];
    AnglesToAxis(angles, rootAxis);
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        vec3_t pos, localAxis[3], forward = {0}, left = {0}, unrolled[3];
        VectorCopy(origin, pos);
        AnglesToAxis(child->pos2, localAxis);
        for (int a = 0; a < 3; ++a) {
            VectorMA(pos, child->pos1[a], rootAxis[a], pos);
            VectorMA(forward, localAxis[0][a], rootAxis[a], forward);
            VectorMA(left, localAxis[1][a], rootAxis[a], left);
        }
        const int role = sectionRole[child->s.number];
        if (role != JAR_MP_SECTION_BOBBER && role != JAR_MP_SECTION_LIFT && role != JAR_MP_SECTION_PICKUP) {
            G_SetOrigin(child, pos);
            VectorCopy(pos, child->s.origin);
        }
        vectoangles(forward, child->s.angles);
        AnglesToAxis(child->s.angles, unrolled);
        child->s.angles[ROLL] = atan2f(DotProduct(left, unrolled[2]), DotProduct(left, unrolled[1])) * 180 / M_PI;
        VectorCopy(child->s.angles, child->r.currentAngles);
        VectorCopy(child->s.angles, child->s.apos.trBase);
        if (role == JAR_MP_SECTION_BOBBER) SectionBobberPublish(child, pos, rootAxis[2]);
        else if (role == JAR_MP_SECTION_LIFT) SectionLiftPublish(child, pos, rootAxis[2]);
        else if (role == JAR_MP_SECTION_PICKUP) SectionPickupPublish(child, pos);
        else if (role == JAR_MP_SECTION_HURT) SectionHurtPublish(child);
        else if (role == JAR_MP_SECTION_PAD && !child->r.bmodel) {
            SectionHull(child->s.origin2, child->s.angles2, child->r.currentAngles, child->r.mins, child->r.maxs);
            trap->LinkEntity((sharedEntity_t *)child);
        } else if (role == JAR_MP_SECTION_PAD || role == JAR_MP_SECTION_SCENERY ||
                   child->s.eType == ET_SPEAKER || child->s.eType == ET_FX || SectionCityLive(child))
            trap->LinkEntity((sharedEntity_t *)child);
    }
}

static qboolean SectionPose(gentity_t *root, const vec3_t origin, const vec3_t angles, qboolean sweep) {
    vec3_t mins, maxs, start;
    for (int a = 0; a < 3; ++a) if (!isfinite(angles[a]) || fabsf(angles[a]) > 3600) return qfalse;
    if (VectorCompare(root->r.currentOrigin, origin) && VectorCompare(root->r.currentAngles, angles)) return qtrue;
    if (SectionRiders(root)) return qfalse;
    SectionHull(root->r.mins, root->r.maxs, angles, mins, maxs);
    VectorCopy(origin, start);
    if (sweep) {
        vec3_t oldMins, oldMaxs;
        float turn = 0;
        VectorCopy(root->r.currentOrigin, start);
        SectionHull(root->r.mins, root->r.maxs, root->r.currentAngles, oldMins, oldMaxs);
        for (int a = 0; a < 3; ++a) turn += fabsf(AngleSubtract(angles[a], root->r.currentAngles[a]));
        // Bound the intervening Euler rotation by the distance traveled from
        // the nearer endpoint, including when pitch and yaw change together.
        const float pad = RadiusFromBounds(root->r.mins, root->r.maxs) * fminf(2, turn * M_PI / 360);
        const qboolean yawOnly = angles[PITCH] == root->r.currentAngles[PITCH] &&
                                 angles[ROLL] == root->r.currentAngles[ROLL];
        for (int a = 0; a < 3; ++a) {
            const float axisPad = a == 2 && yawOnly ? 0 : pad;
            mins[a] = fminf(mins[a], oldMins[a]) - axisPad;
            maxs[a] = fmaxf(maxs[a], oldMaxs[a]) + axisPad;
        }
    }
    int suspended[JAR_MP_SECTION_CHILDREN], suspendedCount = 0;
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1]) {
        gentity_t *child = &g_entities[next - 1];
        if (!child->r.linked || !SectionSolidChild(child)) continue;
        suspended[suspendedCount++] = child->s.number;
        trap->UnlinkEntity((sharedEntity_t *)child);
    }
    if (!SectionClear(start, origin, mins, maxs, root->s.number)) {
        for (int i = 0; i < suspendedCount; ++i)
            trap->LinkEntity((sharedEntity_t *)&g_entities[suspended[i]]);
        return qfalse;
    }
    SectionPublish(root, origin, angles);
    ++objects[root->s.number].propertyRevision;
    return qtrue;
}

static void SectionUse(gentity_t *root, gentity_t *other, gentity_t *actor) {
    if (other != actor || !Alive(actor) || !Edit(actor, root->s.number)) return;
    if (objects[root->s.number].heldBy || (root->flags & FL_INACTIVE)) {
        Tell(actor, "Park and enable the section before entering.");
        return;
    }
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1]) {
        const int n = next - 1;
        if (sectionRole[n] != JAR_MP_SECTION_ENTRY) continue;
        gentity_t *child = &g_entities[n];
        vec3_t arrival;
        VectorCopy(child->r.currentOrigin, arrival);
        arrival[2] += 1;
        if (!SectionClear(arrival, arrival, actor->r.mins, actor->r.maxs, actor->s.number) ||
            !SectionHurtEntryClear(root, actor, arrival)) continue;
        actor->client->ps.groundEntityNum = ENTITYNUM_NONE;
        TeleportPlayer(actor, child->r.currentOrigin, child->r.currentAngles);
        return;
    }
    Tell(actor, "No clear section entry point.");
}

static qboolean SectionRegistry(const jarMPSectionAdmission_t *data, int *first, int *slot) {
    if (!trap->BSPModelAt || data->modelCount < 1) return qfalse;
    int count, next, offset;
    unsigned checksum;
    char name[MAX_QPATH], published[MAX_QPATH];
    if (trap->BSPModelAt(-1, name, sizeof(name), &offset, &next, &checksum) != 1 ||
        offset || next < 1 || next > CM_MAX_INLINE_MODEL_COUNT) return qfalse;
    *slot = *first = -1;
    for (int i = 1; i < MAX_SUB_BSP; ++i) {
        const int found = trap->BSPModelAt(i - 1, name, sizeof(name), &offset, &count, &checksum);
        trap->GetConfigstring(CS_BSP_MODELS + i, published, sizeof(published));
        if (!found) {
            if (*published) return qfalse;
            if (*slot < 0) *slot = i;
            continue;
        }
        if (found != 1 || *slot >= 0 || offset != next || count < 1 ||
            count > CM_MAX_INLINE_MODEL_COUNT - next || Q_stricmp(name, published)) return qfalse;
        if (!Q_stricmp(name, data->model)) {
            if (*first >= 0 || count != data->modelCount || checksum != data->checksum) return qfalse;
            *first = offset;
        }
        next += count;
    }
    if (*first >= 0) {
        *slot = 0;
        return qtrue;
    }
    if (*slot < 0 || data->modelCount > CM_MAX_INLINE_MODEL_COUNT - next) return qfalse;
    *first = next;
    return qtrue;
}

static qboolean SectionResourcesFit(const jarMPSectionAdmission_t *data, int base, int limit) {
    const char *missing[JAR_MP_SECTION_CHILDREN + 21]; // Extra lift cues and Yavin's break sound.
    int count = 0;
    for (int i = 0; i < data->childCount; ++i) {
        const jarMPSectionChild_t *child = &data->children[i];
        const char *paths[] = {base == CS_EFFECTS ? child->effect : base == CS_MODELS ? child->model : child->sound,
                               base == CS_SOUNDS ? child->liftStart : "", base == CS_SOUNDS ? child->liftStop : ""};
        for (int cue = 0; cue < ARRAY_LEN(paths); ++cue) {
            const char *path = paths[cue];
            if (!*path) continue;
            int j;
            for (j = 0; j < count && strcmp(path, missing[j]); ++j) {}
            if (j == count) missing[count++] = path;
        }
    }
    if (base == CS_SOUNDS && !strcmp(data->model, "#mp/duel6"))
        missing[count++] = "sound/weapons/explosions/cargoexplode.wav";
    for (int slot = 1; count && slot < limit; ++slot) {
        char path[MAX_QPATH];
        trap->GetConfigstring(base + slot, path, sizeof(path));
        if (!*path) return limit - slot >= count;
        for (int j = 0; j < count; ++j) {
            if (!strcmp(path, missing[j])) { missing[j] = missing[--count]; break; }
        }
    }
    return count == 0;
}

static qboolean SectionRead(gentity_t *player, jarMPSectionKind_t kind, jarMPSectionAdmission_t *data) {
    char reason[160];
    if (JAR_MP_InspectSection(trap, kind, data, reason, sizeof(reason))) return qtrue;
    Tell(player, reason);
    return qfalse;
}

static gentity_t *SectionSpawn(gentity_t *player, int catalog, const vec3_t origin,
                              const vec3_t angles, const jarMPSectionAdmission_t *data) {
    int first, slot;
    vec3_t mins, maxs;
    SectionHull(data->mins, data->maxs, angles, mins, maxs);
    if (!SectionClear(origin, origin, mins, maxs, ENTITYNUM_NONE)) {
        Tell(player, "Not enough clear space for the section's full hull.");
        return NULL;
    }
    if (!RoomExtra(data->childCount) || !SectionRegistry(data, &first, &slot) ||
        !SectionResourcesFit(data, CS_SOUNDS, MAX_SOUNDS) || !SectionResourcesFit(data, CS_EFFECTS, MAX_FX) ||
        !SectionResourcesFit(data, CS_MODELS, MAX_MODELS)) {
        Tell(player, "Section collision, media or entity registration capacity unavailable.");
        return NULL;
    }
    gentity_t *bobber = NULL;
    const jarMPSectionChild_t *bobberRecord = NULL;
    gentity_t *root = G_TrySpawn();
    if (!root) return NULL;
    root->classname = "jar_section";
    root->model = (char *)data->model;
    root->use = SectionUse;
    for (int i = data->childCount - 1; i >= 0; --i) {
        gentity_t *child = G_TrySpawn();
        if (!child) { G_FreeEntity(root); return NULL; }
        const int n = child->s.number;
        sectionNext[n] = sectionFirst[root->s.number];
        sectionFirst[root->s.number] = n + 1;
        sectionParent[n] = root->s.number + 1;
        sectionRole[n] = data->children[i].kind;
        VectorCopy(data->children[i].origin, child->pos1);
        VectorCopy(data->children[i].angles, child->pos2);
        child->classname = "jar_section_point";
        child->r.svFlags = SVF_NOCLIENT;
        if (sectionRole[n] == JAR_MP_SECTION_BOBBER) {
            bobber = child;
            bobberRecord = &data->children[i];
        } else if (sectionRole[n] == JAR_MP_SECTION_HURT) {
            if (*data->children[i].sound) G_SoundIndex(data->children[i].sound);
            SectionHurtInit(child, &data->children[i]);
        } else if (sectionRole[n] == JAR_MP_SECTION_SCENERY) {
            child->classname = "jar_section_scenery";
            child->s.eType = ET_GENERAL;
            child->s.generic1 = JAR_MP_SECTION_SCENERY_MARKER;
            child->s.otherEntityNum = root->s.number;
            child->s.modelindex = G_ModelIndex(data->children[i].model);
            child->r.svFlags = SVF_BROADCAST;
            VectorCopy(data->children[i].scale, child->s.origin2);
            child->s.angles2[2] = data->children[i].amplitude;
            if (!child->s.modelindex) { G_FreeEntity(root); return NULL; }
        } else if (sectionRole[n] == JAR_MP_SECTION_DUST) {
            child->classname = "jar_section_dust";
            child->s.modelindex = G_EffectIndex(data->children[i].effect);
        } else if (sectionRole[n] == JAR_MP_SECTION_DUST_TIMER) {
            child->classname = "jar_section_dust_timer";
            child->think = SectionDustThink;
            child->nextthink = level.time + FRAMETIME;
        } else if (sectionRole[n] == JAR_MP_SECTION_SPEAKER || sectionRole[n] == JAR_MP_SECTION_FIRE) {
            // Hoth can be placed beyond the host map's distance cull. Its
            // ambient loops still need to reach clients entering the canyon.
            const jarMPSectionKind_t kind = SectionKind(&jarMPCatalog[catalog]);
            child->r.svFlags = kind == JAR_MP_SECTION_HOTH || kind == JAR_MP_SECTION_YAVIN ? SVF_BROADCAST : 0;
            child->s.otherEntityNum = root->s.number;
            const int sound = G_SoundIndex(data->children[i].sound);
            if (sectionRole[n] == JAR_MP_SECTION_FIRE) {
                SectionFireInit(child, G_EffectIndex(data->children[i].effect), sound);
            } else {
                child->classname = "jar_section_speaker";
                child->s.eType = ET_SPEAKER;
                child->s.eventParm = child->s.loopSound = sound;
            }
        }
    }
    trap->SetBrushModel((sharedEntity_t *)root, data->model);
    if (root->s.modelindex != first || !root->r.bmodel) { G_FreeEntity(root); return NULL; }
    if (slot) trap->SetConfigstring(CS_BSP_MODELS + slot, data->model);
    if (!(root->r.contents & CONTENTS_SOLID) ||
        (bobber && !SectionBobberInit(bobber, root->s.number, first, bobberRecord))) {
        G_FreeEntity(root);
        return NULL;
    }
    int record = 0;
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1], ++record) {
        gentity_t *child = &g_entities[next - 1];
        if ((sectionRole[child->s.number] == JAR_MP_SECTION_LIFT || sectionRole[child->s.number] == JAR_MP_SECTION_PAD) &&
            !SectionMoverInit(child, root->s.number, first, &data->children[record])) {
            Tell(player, "A stock section lift or pad could not be registered.");
            G_FreeEntity(root);
            return NULL;
        }
        if (sectionRole[child->s.number] != JAR_MP_SECTION_BREAKABLE) continue;
        const jarMPSectionChild_t *brush = &data->children[record];
        const qboolean yavin = SectionKind(&jarMPCatalog[catalog]) == JAR_MP_SECTION_YAVIN;
        const int maxBrush = yavin ? JAR_MP_YAVIN_BRUSHES : JAR_MP_CITY_BRUSHES;
        const int local = brush->inlineModel;
        if (local < 1 || local > maxBrush ||
            brush->health != (yavin ? sectionYavinHealth[local - 1] : sectionCityHealth[local - 1] - '0') ||
            !SectionCityInit(child, root->s.number, first, brush)) {
            Tell(player, "A stock breakable brush could not be registered.");
            G_FreeEntity(root);
            return NULL;
        }
    }
    Track(root, player ? player->s.number : MAX_CLIENTS, catalog);
    objects[root->s.number].frozen = 1;
    root->clipmask = MASK_PLAYERSOLID;
    root->s.eType = ET_MOVER;
    root->s.generic1 = SECTION_MARKER;
    root->r.svFlags |= SVF_BROADCAST;
    SectionPublish(root, origin, angles);
    record = 0;
    qboolean hasPickups = qfalse;
    for (int next = sectionFirst[root->s.number]; next; next = sectionNext[next - 1], ++record) {
        gentity_t *child = &g_entities[next - 1];
        if (sectionRole[child->s.number] != JAR_MP_SECTION_PICKUP) continue;
        if (!SectionPickupInit(child, &data->children[record])) {
            Tell(player, "A stock section pickup could not be placed.");
            G_FreeEntity(root);
            return NULL;
        }
        if (child->item) {
            vec3_t axis[3], relative;
            AnglesToAxis(angles, axis);
            VectorSubtract(child->r.currentOrigin, origin, relative);
            for (int a = 0; a < 3; ++a) child->pos1[a] = DotProduct(relative, axis[a]);
            child->s.groundEntityNum = ENTITYNUM_WORLD;
            hasPickups = qtrue;
        }
    }
    if (hasPickups) SaveRegisteredItems();
    return root;
}
