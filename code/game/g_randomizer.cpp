// JA:R map pickup randomizer. GPL-2.0-only.
#include "g_local.h"
#include "g_functions.h"
#include "b_local.h"
#include "g_randomizer.h"
#include "g_workshop.h"
#include "qcommon/jar_randomizer.h"
#include <cmath>
#include <cstring>

extern SavedGameJustLoaded_e g_eSavedGameJustLoaded;

namespace {
enum Category { Weapons, Ammo, Supplies, CategoryCount };
const char *const categoryNames[] = {"weapons", "ammo", "supplies"};
struct Pickup { const char *name; itemType_t type; int tag; Category category; };
// Stable ordering is part of v1 seed reproducibility. Never admit items simply
// because they share an IT_* type: keys, force crystals and holocrons are special.
const Pickup catalog[] = {
    {"weapon_blaster_pistol", IT_WEAPON, WP_BLASTER_PISTOL, Weapons},
    {"weapon_blaster", IT_WEAPON, WP_BLASTER, Weapons},
    {"weapon_bowcaster", IT_WEAPON, WP_BOWCASTER, Weapons},
    {"weapon_repeater", IT_WEAPON, WP_REPEATER, Weapons},
    {"weapon_flechette", IT_WEAPON, WP_FLECHETTE, Weapons},
    {"weapon_concussion_rifle", IT_WEAPON, WP_CONCUSSION, Weapons},
    {"ammo_blaster", IT_AMMO, AMMO_BLASTER, Ammo},
    {"ammo_metallic_bolts", IT_AMMO, AMMO_METAL_BOLTS, Ammo},
    {"item_bacta", IT_HEALTH, ITM_MEDPAK_PICKUP, Supplies},
    {"item_medpak_instant", IT_HEALTH, ITM_MEDPAK_PICKUP, Supplies},
    {"item_shield_sm_instant", IT_ARMOR, ITM_SHIELD_SM_PICKUP, Supplies},
    {"item_shield_lrg_instant", IT_ARMOR, ITM_SHIELD_LRG_PICKUP, Supplies}
};
const unsigned conservativeCount = sizeof(catalog) / sizeof(catalog[0]);
const Pickup extras[] = {
    {"weapon_disruptor", IT_WEAPON, WP_DISRUPTOR, Weapons},
    {"weapon_demp2", IT_WEAPON, WP_DEMP2, Weapons},
    {"weapon_rocket_launcher", IT_WEAPON, WP_ROCKET_LAUNCHER, Weapons},
    {"weapon_thermal", IT_WEAPON, WP_THERMAL, Weapons},
    {"weapon_trip_mine", IT_WEAPON, WP_TRIP_MINE, Weapons},
    {"weapon_det_pack", IT_WEAPON, WP_DET_PACK, Weapons},
    {"ammo_powercell", IT_AMMO, AMMO_POWERCELL, Ammo},
    {"ammo_rockets", IT_AMMO, AMMO_ROCKETS, Ammo}
};
bool enabled[CategoryCount] = {};
bool wide = false, enemies = false, loadout = false, validSettings = true;
int pickupChanges = 0, enemyChanges = 0, objectChanges = 0;
bool objects = false, projectiles = false;
bool fixedMapping = true, worldFixedMapping = true;
uint32_t worldSeed = 1, objectDigest = 2166136261u;
struct StaticProp { const char *name; int bytes; uint32_t fingerprint; float mins[3], maxs[3]; };
const StaticProp staticProps[] = {
#include "g_randomizer_objects.inc"
};
bool propValid[sizeof(staticProps)/sizeof(staticProps[0])] = {};
uint32_t pickupDigest = 2166136261u, enemyDigest = 2166136261u;
unsigned loadoutMask = 0;
uint32_t seed = 1;

gitem_t *Lookup(const Pickup& entry) {
    // Match G_CallSpawn/FinishSpawningItem: the first missing classname ends
    // the spawnable item table, even if a mod defines later indexed entries.
    for (int i = 1; i < bg_numItems && bg_itemlist[i].classname; ++i) {
        gitem_t *item = &bg_itemlist[i];
        if (item->classname && !std::strcmp(item->classname, entry.name))
            return item->giType == entry.type && item->giTag == entry.tag &&
                item->world_model && *item->world_model && item->quantity > 0 ? item : nullptr;
    }
    return nullptr;
}

bool Bounds(const gitem_t *item, float *mins, float *maxs) {
    bool zero = true;
    for (int i = 0; i < 3; ++i) zero &= item->mins[i] == 0 && item->maxs[i] == 0;
    for (int i = 0; i < 3; ++i) {
        mins[i] = zero ? (i == 2 ? -2.0f : -float(ITEM_RADIUS)) : item->mins[i];
        maxs[i] = zero ? float(ITEM_RADIUS) : item->maxs[i];
        if (!std::isfinite(mins[i]) || !std::isfinite(maxs[i]) || mins[i] >= maxs[i]) return false;
    }
    return true;
}

bool Fits(const gitem_t *item, const gitem_t *original) {
    float mins[3], maxs[3], originalMins[3], originalMaxs[3];
    if (!Bounds(item, mins, maxs) || !Bounds(original, originalMins, originalMaxs)) return false;
    for (int i = 0; i < 3; ++i)
        if (mins[i] < originalMins[i] || maxs[i] > originalMaxs[i]) return false;
    return true;
}

const char *ProtectionReason(const gentity_t *ent) {
    if ((ent->spawnflags & ~(wide ? (256|512|1024) : 0)) || ent->flags || ent->count || ent->target || ent->targetname ||
        ent->target2 || ent->target3 || ent->target4 || ent->targetJump || ent->team ||
        ent->script_targetname || ent->NPC_targetname || ent->NPC_type)
        return "entity-metadata";
    for (int i = 0; i < NUM_BSETS; ++i) if (ent->behaviorSet[i]) return "script";
    // Unknown map keys fail closed, including empty script/target fields, model
    // overrides, custom quantities and mapper-specific progression metadata.
    char *key, *value;
    for (unsigned i = 0; G_SpawnField(i, &key, &value); ++i) {
        bool ordinary = false;
        for (const char *allowed : {"classname", "origin", "angle", "angles", "notsingle"})
            ordinary |= !Q_stricmp(key, allowed);
        if (!Q_stricmp(key, "spawnflags")) {
            uint32_t flags;
            ordinary = jar::ParsePickupSeed(value, flags) && !(flags & ~(wide ? (256u|512u|1024u) : 0u));
        }
        if (!ordinary) return "map-metadata";
    }
    return nullptr;
}
}

void G_JAR_InitPickupRandomizer() {
    bool values[jar::randomizerCvarCount] = {};
    validSettings = true; pickupChanges = enemyChanges = 0; loadoutMask = 0;
    pickupDigest = enemyDigest = objectDigest = 2166136261u; objectChanges = 0;
    for (unsigned i = 0; i < jar::randomizerCvarCount; ++i) {
        const char *value = gi.cvar(jar::pickupCvars[i], i == 0 ? "1" : "0",
            CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART)->string;
        if (i == 0) validSettings &= jar::ParsePickupSeed(value, seed);
        else {
            validSettings &= !std::strcmp(value, "0") || !std::strcmp(value, "1");
            values[i] = !std::strcmp(value, "1");
        }
    }
    validSettings &= jar::ParseRandomMode(gi.cvar("jar_randomMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART)->string,fixedMapping);
    const bool fresh = validSettings && g_eSavedGameJustLoaded != eFULL;
    for (int i = 0; i < CategoryCount; ++i) enabled[i] = fresh && values[i+1];
    wide = fresh && values[4]; enemies = fresh && values[5]; loadout = fresh && values[6];
    // Static visuals are rebuilt from the BSP even during full-save loads.
    // Future-map preferences must not replace the current world's applied seed.
    gi.cvar("jar_projectileSequence","0",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    gi.cvar("jar_projectileSequenceSeed","",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    gi.cvar("jar_worldSeed", "1", CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
    gi.cvar("jar_worldMode", "0", CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
    gi.cvar("jar_worldObjects", "0", CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
    gi.cvar("jar_worldProjectiles", "0", CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
    if (g_eSavedGameJustLoaded != eFULL) {
        char number[16]; Com_sprintf(number,sizeof(number),"%u",seed);
        gi.cvar_set("jar_worldSeed",validSettings ? number : "1");
        gi.cvar_set("jar_worldMode",fixedMapping ? "0" : "1");
        gi.cvar_set("jar_projectileSequence","0");
        gi.cvar_set("jar_worldObjects",fresh && values[7] ? "1" : "0");
        gi.cvar_set("jar_worldProjectiles",fresh && values[8] ? "1" : "0");
    }
    const bool worldValid = jar::ParsePickupSeed(gi.cvar("jar_worldSeed","1",0)->string,worldSeed) &&
        jar::ParseRandomMode(gi.cvar("jar_worldMode","0",0)->string,worldFixedMapping);
    objects = worldValid && !std::strcmp(gi.cvar("jar_worldObjects","0",0)->string,"1");
    projectiles = worldValid && !std::strcmp(gi.cvar("jar_worldProjectiles","0",0)->string,"1");
    for (unsigned i=0; i<sizeof(staticProps)/sizeof(staticProps[0]); ++i) {
        propValid[i] = false;
        if (!objects || !gi.FS_ReadFile || !gi.FS_FreeFile) continue;
        void *data = nullptr;
        const auto& p = staticProps[i];
        // Reject modified models, even if a mod keeps the stock filename.
        const long size = gi.FS_ReadFile(p.name,&data);
        if (data && size == p.bytes) {
            uint32_t hash = 2166136261u;
            for (int j=0; j<size; ++j) hash = jar::PickupHashByte(hash,static_cast<unsigned char *>(data)[j]);
            propValid[i] = hash == p.fingerprint;
        }
        if (data) gi.FS_FreeFile(data);
    }
    G_JAR_InitShotCatalog(projectiles);
    if (!validSettings) gi.Printf("JA:R randomizer disabled: invalid seed or toggle. Use seed 0-2147483647 and toggles 0/1.\n");
    G_JAR_InitNpcCatalog();
    // Precache before the client starts. Add three distinct ranged weapons; keep
    // all existing mission tools/sabers. The normal item definitions supply ammo.
    if (loadout) {
        const Pickup *pool[16] = {}; unsigned count = 0;
        for (const auto& p : catalog) if (p.category == Weapons && Lookup(p)) pool[count++] = &p;
        for (const auto& p : extras) if (p.type == IT_WEAPON && p.tag <= WP_ROCKET_LAUNCHER && Lookup(p)) pool[count++] = &p;
        for (unsigned slot = 0; slot < 3 && count; ++slot) {
            char key[32]; Com_sprintf(key,sizeof(key),"player/slot-%u",slot+1);
            const unsigned choice = fixedMapping ? jar::FixedCatalogIndex(seed,"arsenal-v1",key,count,
                [&](unsigned i){return pool[i]->name;}) : jar::PickupDecision(seed,level.mapname,slot+1,"arsenal-v1","player")%count;
            const Pickup *p = pool[choice]; pool[choice] = pool[--count];
            loadoutMask |= 1u << p->tag; RegisterItem(Lookup(*p));
        }
    }
    gi.cvar("jar_randomStatus", "", CVAR_ROM | CVAR_NORESTART);
    G_JAR_RandomizerStatus();
    if (gi.JAR_Log) gi.JAR_Log(1, "randomizer", "pickups_init",
        "version=%d map=%s seed=%u weapons=%d ammo=%d supplies=%d wide=%d enemies=%d loadout=%d arsenal=%u valid=%d full_save=%d",
        wide ? 2 : 1, level.mapname, seed, enabled[0], enabled[1], enabled[2], wide, enemies, loadout, loadoutMask, validSettings, g_eSavedGameJustLoaded == eFULL);
}

void G_JAR_RandomizerStatus() {
    if (!gi.cvar_set) return;
    char text[160];
    if (!validSettings) Q_strncpyz(text,"Current map: disabled (invalid settings).",sizeof(text));
    else if (g_eSavedGameJustLoaded == eFULL) Q_strncpyz(text,"Current map: saved world; existing randomization preserved.",sizeof(text));
    else if (!enabled[0] && !enabled[1] && !enabled[2] && !enemies && !loadout && !objects && !projectiles) Q_strncpyz(text,"Current map: randomizer OFF.",sizeof(text));
    else Com_sprintf(text,sizeof(text),"Current map: seed %u | %d pickups / %d NPCs / %d props | guns %s / shots %s",seed,pickupChanges,enemyChanges,objectChanges,loadout ? "ON" : "off",projectiles ? "ON" : "off");
    gi.cvar_set("jar_randomStatus",text);
}

void G_JAR_RandomizeMapPickup(gentity_t *ent, int mapOrdinal) {
    if (!ent || !ent->classname || mapOrdinal <= 0) return;
    const Pickup *source = nullptr;
    for (const auto& entry : catalog)
        if (!std::strcmp(ent->classname, entry.name)) { source = &entry; break; }
    if (!source || !enabled[source->category]) return;
    const char *reason = ProtectionReason(ent);
    gitem_t *original = Lookup(*source);
    if (!original) reason = "missing-or-altered-source";
    const Pickup *candidates[conservativeCount + sizeof(extras)/sizeof(extras[0])] = {};
    unsigned count = 0;
    if (!reason) {
        for (const auto& entry : catalog) {
            if ((!wide && entry.category != source->category) || (wide && !std::strcmp(entry.name, source->name))) continue;
            gitem_t *candidate = Lookup(entry);
            if (candidate && Fits(candidate, original)) candidates[count++] = &entry;
        }
        if (wide) for (const auto& entry : extras) {
            gitem_t *candidate = Lookup(entry);
            if (candidate && Fits(candidate, original)) candidates[count++] = &entry;
        }
        if (!count) reason = "no-fitting-candidates";
    }
    const char *domain = wide ? "wide-pickups-v1" : categoryNames[source->category];
    const uint32_t decision = fixedMapping ? jar::FixedDecision(seed,domain,source->name) :
        jar::PickupDecision(seed,level.mapname,uint32_t(mapOrdinal),domain,source->name);
    const Pickup *chosen = reason ? source : candidates[fixedMapping ?
        jar::FixedCatalogIndex(seed,domain,source->name,count,[&](unsigned i){return candidates[i]->name;}) : decision%count];
    // Preserve the no-op path without allocating or touching the entity. The
    // normal G_CallSpawn -> G_SpawnItem path handles precache, counts and models.
    const bool changed = std::strcmp(source->name, chosen->name) != 0;
    if (changed) {
        pickupDigest = jar::PickupHashText(jar::PickupHashWord(pickupDigest,mapOrdinal),chosen->name);
        ent->classname = G_NewString(chosen->name); ++pickupChanges; G_JAR_RandomizerStatus();
    }
    if (gi.JAR_Log) gi.JAR_Log(2, "randomizer", "pickup_decision",
        "version=%d map=%s seed=%u ordinal=%d category=%s decision=%u original=%s chosen=%s candidates=%u applied=%d reason=%s",
        wide ? 2 : 1, level.mapname, seed, mapOrdinal, categoryNames[source->category], decision,
        source->name, chosen->name, count, changed, reason ? reason : changed ? "selected" : "same-item");
}

#include "g_randomizer_npc.inc"
#include "g_actor_recovery.inc"

void G_JAR_RandomizePlayerLoadout(gentity_t *ent, int spawnflags) {
    if (!loadout || !ent || !ent->client || ent->s.number != 0 ||
        g_eSavedGameJustLoaded == eFULL || (spawnflags & (32|64))) return;
    unsigned granted = 0;
    for (int weapon=WP_BLASTER_PISTOL; weapon<=WP_CONCUSSION; ++weapon) {
        if (!(loadoutMask & (1u << weapon))) continue;
        const int ammo = weaponData[weapon].ammoIndex;
        if (ammo <= AMMO_NONE || ammo >= AMMO_MAX || ammoData[ammo].max <= 0) continue;
        ent->client->ps.stats[STAT_WEAPONS] |= 1u << weapon;
        ent->client->ps.ammo[ammo] = ammoData[ammo].max;
        granted |= 1u << weapon;
    }
    if (gi.JAR_Log) gi.JAR_Log(1,"randomizer","player_arsenal",
        "version=1 map=%s seed=%u granted=%u inventory=%u",level.mapname,seed,granted,ent->client->ps.stats[STAT_WEAPONS]);
    G_JAR_RandomizerStatus();
}

void G_JAR_ReportRandomizer() {
    // Exactly one bounded summary per map, retained even when detailed model
    // loading events hit the diagnostic rate limit. Hashes cover every change.
    if (gi.JAR_Log) gi.JAR_Log(0,"randomizer","world_ready",
        "map=%s seed=%u pickups=%d enemies=%d pickup_digest=%u enemy_digest=%u arsenal=%u full_save=%d valid=%d objects=%d object_digest=%u world_seed=%u projectiles=%d mode=%s world_mode=%s",
        level.mapname,seed,pickupChanges,enemyChanges,pickupDigest,enemyDigest,loadoutMask,g_eSavedGameJustLoaded==eFULL,validSettings,objectChanges,objectDigest,worldSeed,projectiles,fixedMapping?"fixed":"reroll",worldFixedMapping?"fixed":"reroll");
}

void G_JAR_RandomizeMapObject(gentity_t *ent, int ordinal) {
    if (!objects || !ent || !ent->classname || Q_stricmp(ent->classname,"misc_model_static") || !ent->model || ordinal <= 0) return;
    if (ent->target || ent->targetname || ent->script_targetname || ent->flags || ent->count) return;
    for (int i=0; i<NUM_BSETS; ++i) if (ent->behaviorSet[i]) return;
    // Structural/interactive entities and scaled or scripted props stay original.
    // Radiant bounds/shadow keys are ignored by static model construction; the
    // existing culling-only zoffset is preserved along with origin and angles.
    char *key, *value;
    for (unsigned i=0; G_SpawnField(i,&key,&value); ++i) {
        bool ok = false;
        for (const char *allowed : {"classname","model","origin","angle","angles","notsingle","autobound","mins","maxs","_cs","_rs","zoffset"}) ok |= !Q_stricmp(key,allowed);
        if (!Q_stricmp(key,"spawnflags")) { uint32_t flags; ok = jar::ParsePickupSeed(value,flags) && !(flags & ~(256u|512u|1024u)); }
        if (!ok) return;
    }
    const StaticProp *source = nullptr;
    for (unsigned i=0; i<sizeof(staticProps)/sizeof(staticProps[0]); ++i)
        if (propValid[i] && !Q_stricmp(ent->model,staticProps[i].name)) source = &staticProps[i];
    if (!source) return;
    const StaticProp *pool[sizeof(staticProps)/sizeof(staticProps[0])] = {}; unsigned count = 0;
    for (unsigned i=0; i<sizeof(staticProps)/sizeof(staticProps[0]); ++i) {
        const auto& p = staticProps[i]; bool fits = propValid[i] && &p != source;
        for (int axis=0; axis<3; ++axis) fits &= p.mins[axis] >= source->mins[axis] && p.maxs[axis] <= source->maxs[axis];
        if (fits) pool[count++] = &p;
    }
    if (!count) return;
    const unsigned choice = worldFixedMapping ? jar::FixedCatalogIndex(worldSeed,"objects-v1",source->name,count,
        [&](unsigned i){return pool[i]->name;}) : jar::PickupDecision(worldSeed,level.mapname,ordinal,"objects-v1",source->name)%count;
    const char *chosen = pool[choice]->name;
    ent->model = G_NewString(chosen); ++objectChanges;
    objectDigest = jar::PickupHashText(jar::PickupHashWord(objectDigest,ordinal),chosen);
    if (gi.JAR_Log) gi.JAR_Log(2,"randomizer","object_decision","seed=%u ordinal=%d original=%s chosen=%s",worldSeed,ordinal,source->name,chosen);
    G_JAR_RandomizerStatus();
}

bool G_JAR_ProjectilesEnabled() { return projectiles; }
