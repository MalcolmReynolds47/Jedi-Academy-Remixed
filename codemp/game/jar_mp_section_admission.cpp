// Read the qualified stock BSP through the active MP filesystem. GPL-2.0-only.
#include "g_local.h"
#include "qcommon/qfiles.h"
#include "jar_mp_section_admission.h"
#include "../../code/game/g_subbsp_admission.h"
#include "../../shared/qcommon/jar_hoth_sky.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include <cstdio>

uint32_t Com_BlockChecksum(const void* buffer, int length);

namespace {
namespace admission = jar::bspAdmission;
namespace fire = jar::bspSectionFX;
namespace sentinel = jar::sentinel;
namespace rift = jar::rift;
namespace shaft = jar::shaft;
namespace korriban = jar::korriban;
using jar::bspSection::Mode;
static_assert(JAR_MP_CITY_BRUSHES == admission::cityBreakables, "MP City brush count must match shared admission");
static_assert(JAR_MP_SENTINEL_CHILDREN + 1 == sentinel::entityCount);
static_assert(JAR_MP_KORRIBAN_CHILDREN + 1 == korriban::entityCount);
static_assert(JAR_MP_KORRIBAN_PICKUPS == korriban::pickupCount);
static_assert(JAR_MP_YAVIN_BRUSHES == admission::yavinBreakables);

struct Profile {
    const char* model;
    int size;
    unsigned checksum;
    Mode mode;
    int entries, points, intermissions, speakers, fires, hurtDamage;
    const char* loops[4];
};

constexpr Profile profiles[JAR_MP_SECTION_COUNT] = {
    {},
    {"#mp/duel4", 1167576, 0xb030d7fcu, Mode::Stationary, 13, 9, 1, 4, 0, 0,
        {"sound/ambience/kejim/kejim_interior.wav", "sound/ambience/cairn/cairn_circuits.wav"}},
    {"#mp/duel10", 3778576, 0x2a30df23u, Mode::Stationary, 17, 38, 1, 4, 0, 0,
        {"sound/ambience/cairn/cairn_reactor.wav", "sound/ambience/artus/artus_shaft.wav"}},
    {"#mp/duel7", 2730040, 0xa23dfe0eu, Mode::Fire, 15, 18, 1, 0, 6, 0, {}},
    {"#mp/duel2", 6073332, 0xd6bcea3bu, Mode::Hurt, 12, 3, 1, 1, 0, -1,
        {"sound/ambience/artus/artus_shaft.wav"}},
    {"#mp/duel5", 6900108, 0x1efc7e4eu, Mode::Hurt, 13, 21, 0, 0, 0, 50, {}},
    {"#mp/duel1", 3036068, 0xe1233cdau, Mode::Bobbing, 17, 6, 1, 0, 0, 0, {}},
    {"#mp/duel8", 5689552, 0x98d438f5u, Mode::Breakable, 13, 21, 1, 2, 0, 0,
        {"sound/ambience/prototype/windmix1.wav"}},
    {"#mp/ffa1", 4617652, sentinel::checksum, Mode::Sentinel, sentinel::entryCount, 140, 1, 12, 0, -1,
        {"sound/ambience/cairn/cairn_reactor.wav", "sound/ambience/cairn/cairn_duct.wav",
         "sound/ambience/cairn/cairn_circuits.wav", "sound/ambience/kejim/kejim_interior.wav"}},
    {jar::hothSky::resource, 5004800, jar::hothSky::checksum, Mode::Hurt, 14, 4, 1, 8, 0, -1,
        {"sound/ambience/prototype/windmix1.wav"}},
    {rift::resource, 7430960, rift::checksum, Mode::Rift, 16, 65, 1, 0, 0, -1, {}},
    {"#mp/duel3", 2456616, shaft::checksum, Mode::Shaft, 9, 11, 1, 3, 0, -1,
        {"sound/ambience/cairn/cairn_energy.wav", "sound/ambience/cairn/cairn_circuits.wav",
         "sound/ambience/prototype/busyroom.wav"}},
    {korriban::resource, 8168232, korriban::checksum, Mode::Korriban, korriban::entryCount, 202, 1,
        korriban::windCount, korriban::fireCount, -1, {"sound/ambience/prototype/windmix1.wav"}},
    {jar::yavinSky::resource, 7619140, jar::yavinSky::checksum, Mode::Yavin, 16, 134, 1, 0, 16, 0, {}}
};

class ScopedFile {
    const gameImport_t* game;
public:
    fileHandle_t handle = 0;
    int size;
    ScopedFile(const gameImport_t* imports, const char* path) : game(imports),
        size(game->FS_Open(path, &handle, FS_READ)) {}
    ~ScopedFile() { if (handle) game->FS_Close(handle); }
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;
};

qboolean Reject(char* reason, int size, const char* message) {
    if (reason && size > 0) std::snprintf(reason, size, "%s", message);
    return qfalse;
}

template<size_t N> bool CopyField(char (&out)[N], const char* text) {
    const size_t length = std::strlen(text);
    if (length >= N) return false;
    std::memcpy(out, text, length + 1);
    return true;
}

bool HurtRecord(const Profile& profile, const admission::Report& report,
    const admission::Entity& record, jarMPSectionChild_t& child) {
    // Mode::Hurt verifies the exact model planes and either the stock fall
    // keys or the stock timed hurt keys. Only Taspir needs an MP flag adapter.
    const int damage = std::atoi(admission::Value(record, "dmg"));
    const bool shortFall = profile.mode == Mode::Sentinel || profile.mode == Mode::Rift ||
        profile.mode == Mode::Shaft || profile.mode == Mode::Korriban || profile.checksum == jar::hothSky::checksum;
    if (record.size() != (shortFall ? 3 : 4) || damage != profile.hurtDamage) return false;

    child.kind = JAR_MP_SECTION_HURT;
    child.damage = damage;
    child.spawnflags = damage == -1 ? 8 : 16; // MP's SLOW flag honors the retail wait=1.
    if (profile.mode == Mode::Rift) {
        const bool compound = !std::strcmp(admission::Value(record, "model"), "*1");
        const auto& box = rift::fallBoxes[compound ? 0 : 2];
        VectorCopy(box.mins, child.mins);
        VectorCopy(box.maxs, child.maxs);
        if (compound) {
            VectorCopy(rift::fallBoxes[1].mins, child.secondMins);
            VectorCopy(rift::fallBoxes[1].maxs, child.secondMaxs);
        }
    } else if (profile.mode == Mode::Korriban) {
        const int model = std::atoi(admission::Value(record, "model") + 1);
        if (model < 1 || model > ARRAY_LEN(korriban::fallBoxes)) return false;
        const auto& box = korriban::fallBoxes[model - 1];
        VectorCopy(box.mins, child.mins);
        VectorCopy(box.maxs, child.maxs);
    } else for (int axis = 0; axis < 3; ++axis) {
        child.origin[axis] = (report.hurtMins[axis] + report.hurtMaxs[axis]) * .5f;
        child.maxs[axis] = (report.hurtMaxs[axis] - report.hurtMins[axis]) * .5f;
        child.mins[axis] = -child.maxs[axis];
    }
    return CopyField(child.classname, "trigger_hurt") &&
           (damage != -1 || CopyField(child.sound, "*falling1.wav"));
}

bool LiftTravel(const byte* bytes, const admission::Report& report, int local, float lip, float& travel) {
    dheader_t header;
    std::memcpy(&header, bytes, sizeof(header));
    auto models = header.lumps[LUMP_MODELS];
    models.fileofs = LittleLong(models.fileofs);
    models.filelen = LittleLong(models.filelen);
    dmodel_t model;
    if (!admission::LumpRecord(bytes, models, local, model)) return false;
    // Inline bounds have the same one-unit padding as native func_door.
    travel = LittleFloat(model.maxs[2]) - LittleFloat(model.mins[2]) + 2.f - lip;
    return travel > 0 && LittleFloat(model.mins[2]) - travel >= report.mins[2];
}

bool LiftSounds(jarMPSectionChild_t& child, const char* const* sounds) {
    return !sounds || (CopyField(child.liftStart, sounds[0]) && CopyField(child.sound, sounds[1]) &&
        CopyField(child.liftStop, sounds[2]));
}

bool ShaftMover(const byte* bytes, const admission::Report& report,
    const admission::Entity& record, jarMPSectionChild_t& child, bool lift) {
    const int local = std::atoi(admission::Value(record, "model") + 1);
    child.inlineModel = local;
    child.group = local == 1 || local == 2 ? 0 : 1;
    if (lift) {
        child.kind = JAR_MP_SECTION_LIFT;
        child.damage = 2;
        child.spawnflags = 1;
        return LiftTravel(bytes, report, local, 8.f, child.travel) && LiftSounds(child, shaft::liftSounds);
    }
    child.kind = JAR_MP_SECTION_PAD;
    child.period = 4000;
    VectorCopy(report.padMins[child.group], child.mins);
    VectorCopy(report.padMaxs[child.group], child.maxs);
    return true;
}

bool SentinelRecord(const byte* bytes, const admission::Report& report,
    const admission::Entity& record, jarMPSectionChild_t& child) {
    const char* classname = admission::Value(record, "classname");
    if (std::strcmp(classname, "func_door") && std::strcmp(classname, "trigger_multiple")) return false;
    const int local = std::atoi(admission::Value(record, "model") + 1);
    child.inlineModel = local;
    if (!std::strcmp(classname, "trigger_multiple")) {
        for (int i = 0; i < ARRAY_LEN(sentinel::pads); ++i) if (sentinel::pads[i].model == local) {
            child.kind = JAR_MP_SECTION_PAD;
            child.group = i;
            child.period = sentinel::pads[i].wait * 1000;
            return true;
        }
        return false;
    }
    for (const auto& lift : sentinel::lifts) if (lift.model == local) {
        const float lip = *lift.lip ? std::strtof(lift.lip, nullptr) : 8.f;
        if (!LiftTravel(bytes, report, local, lip, child.travel)) return false;
        child.kind = JAR_MP_SECTION_LIFT;
        child.group = lift.group;
        child.damage = lift.flags == 5 ? 5000 : 2;
        child.spawnflags = lift.flags;
        const auto* sounds = !*lift.sound ? nullptr : !std::strcmp(lift.sound, "piston") ?
            sentinel::pistonSounds : jar::shaft::liftSounds;
        return LiftSounds(child, sounds);
    }
    return false;
}

bool BobbingClip(const byte* bytes, const admission::Report& report, const jarMPSectionChild_t& child) {
    // Inspect already validated the BSP header, lump ranges and mover keys.
    dheader_t header;
    std::memcpy(&header, bytes, sizeof(header));
    for (auto& lump : header.lumps) {
        lump.fileofs = LittleLong(lump.fileofs);
        lump.filelen = LittleLong(lump.filelen);
    }
    dmodel_t model;
    dbrush_t brush;
    dshader_t shader;
    constexpr unsigned contents = CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP | CONTENTS_TRANSLUCENT;
    if (!admission::LumpRecord(bytes, header.lumps[LUMP_MODELS], child.inlineModel, model) ||
        LittleLong(model.numSurfaces) != 0 || LittleLong(model.numBrushes) != 1 ||
        !admission::LumpRecord(bytes, header.lumps[LUMP_BRUSHES], LittleLong(model.firstBrush), brush) ||
        LittleLong(brush.numSides) != 6 ||
        !admission::LumpRecord(bytes, header.lumps[LUMP_SHADERS], LittleLong(brush.shaderNum), shader) ||
        static_cast<unsigned>(LittleLong(shader.contentFlags)) != contents) return false;

    const int firstSide = LittleLong(brush.firstSide);
    if (!admission::LumpRange<dbrushside_t>(header.lumps[LUMP_BRUSHSIDES], firstSide, 6)) return false;
    for (int axis = 0; axis < 3; ++axis) {
        const float lo = LittleFloat(model.mins[axis]), hi = LittleFloat(model.maxs[axis]);
        const float excursion = axis == 2 ? child.amplitude : 0;
        if (child.origin[axis] + lo - excursion < report.mins[axis] ||
            child.origin[axis] + hi + excursion > report.maxs[axis]) return false;
        for (int positive = 0; positive <= 1; ++positive) {
            dbrushside_t side;
            dplane_t plane;
            if (!admission::LumpRecord(bytes, header.lumps[LUMP_BRUSHSIDES], firstSide + axis * 2 + positive, side) ||
                !admission::LumpRecord(bytes, header.lumps[LUMP_PLANES], LittleLong(side.planeNum), plane) ||
                LittleLong(side.shaderNum) != LittleLong(brush.shaderNum) ||
                LittleFloat(plane.dist) != (positive ? hi : -lo)) return false;
            for (int component = 0; component < 3; ++component) {
                const float expected = component == axis ? (positive ? 1 : -1) : 0;
                if (LittleFloat(plane.normal[component]) != expected) return false;
            }
        }
    }
    return true;
}

bool ChildRecord(const byte* bytes, const Profile& profile, const admission::Report& report,
    const admission::Entity& record, jarMPSectionChild_t& child) {
    const char* classname = admission::Value(record, "classname");
    if (profile.mode == Mode::Sentinel) {
        if (!std::strcmp(classname, "func_door") || !std::strcmp(classname, "trigger_multiple"))
            return SentinelRecord(bytes, report, record, child);
    }
    if (profile.mode == Mode::Shaft && admission::OneOf(classname, {"func_door", "trigger_multiple"}))
        return ShaftMover(bytes, report, record, child, !std::strcmp(classname, "func_door"));
    size_t pickupKinds = 0;
    const auto* specs = jar::bspSection::PickupSpecs(profile.mode, pickupKinds);
    for (size_t i = 0; i < pickupKinds; ++i) if (!std::strcmp(classname, specs[i].name)) {
        child.kind = JAR_MP_SECTION_PICKUP;
        child.period = specs[i].seconds;
        break;
    }
    if (jar::bspSection::Breakable(profile.mode) && !std::strcmp(classname, "func_breakable")) {
        // Shared admission verifies the exact key set and all 69 unique inline
        // model numbers. Authored brushes use the section origin and angles.
        child.kind = JAR_MP_SECTION_BREAKABLE;
        child.inlineModel = std::atoi(admission::Value(record, "model") + 1);
        child.health = std::atoi(admission::Value(record, "health"));
        child.spawnflags = std::atoi(admission::Value(record, "spawnflags"));
        return CopyField(child.classname, classname);
    }
    if (child.kind == JAR_MP_SECTION_PICKUP) { /* Stock origin and angle below. */ }
    else if (jar::bspSection::Start(classname)) child.kind = JAR_MP_SECTION_ENTRY;
    else if (!std::strcmp(classname, "info_player_intermission")) child.kind = JAR_MP_SECTION_INTERMISSION;
    else if (!std::strcmp(classname, "target_speaker")) child.kind = JAR_MP_SECTION_SPEAKER;
    else if (profile.fires && !std::strcmp(classname, "fx_runner"))
        child.kind = profile.mode == Mode::Yavin && !std::strcmp(admission::Value(record, "fxfile"), fire::dust) ?
            JAR_MP_SECTION_DUST : JAR_MP_SECTION_FIRE;
    else if (profile.hurtDamage && !std::strcmp(classname, "trigger_hurt")) return HurtRecord(profile, report, record, child);
    else if (profile.mode == Mode::Bobbing && !std::strcmp(classname, "func_bobbing")) child.kind = JAR_MP_SECTION_BOBBER;
    else if ((profile.mode == Mode::Shaft || profile.mode == Mode::Yavin) && !std::strcmp(classname, "misc_model_static"))
        child.kind = JAR_MP_SECTION_SCENERY;
    else if (profile.mode == Mode::Yavin && !std::strcmp(classname, "func_timer")) child.kind = JAR_MP_SECTION_DUST_TIMER;
    else if (admission::OneOf(classname, {"info_null", "info_notnull", "target_position"}) ||
             (profile.mode == Mode::Yavin && admission::OneOf(classname, {"target_random", "misc_skyportal"})) ||
             (profile.checksum == jar::hothSky::checksum && !std::strcmp(classname, "misc_skyportal")))
        child.kind = JAR_MP_SECTION_POINT;
    else return false;

    const char* origin = admission::Value(record, "origin");
    if (std::sscanf(origin, "%f %f %f", &child.origin[0], &child.origin[1], &child.origin[2]) != 3) return false;
    const char* angles = admission::Value(record, "angles");
    if (*angles) {
        if (std::sscanf(angles, "%f %f %f", &child.angles[0], &child.angles[1], &child.angles[2]) != 3) return false;
    } else {
        child.angles[YAW] = std::strtof(admission::Value(record, "angle"), nullptr);
    }
    if (child.kind == JAR_MP_SECTION_SCENERY) {
        if (!CopyField(child.model, admission::Value(record, "model"))) return false;
        VectorSet(child.scale, 1, 1, 1);
        const char* scale = admission::Value(record, "modelscale");
        const char* scaleVec = admission::Value(record, "modelscale_vec");
        if (*scale) {
            const float uniform = std::strtof(scale, nullptr);
            VectorSet(child.scale, uniform, uniform, uniform);
        } else if (*scaleVec && std::sscanf(scaleVec, "%f %f %f", &child.scale[0], &child.scale[1], &child.scale[2]) != 3) {
            return false;
        }
        child.amplitude = std::strtof(admission::Value(record, "zoffset"), nullptr);
    } else if (child.kind == JAR_MP_SECTION_SPEAKER) {
        const char* sound = admission::Value(record, "noise");
        const bool stockSound = std::any_of(std::begin(profile.loops), std::end(profile.loops),
            [sound](const char* path) { return path && !std::strcmp(sound, path); });
        if (!profile.speakers || std::strcmp(admission::Value(record, "spawnflags"), "1") ||
            !stockSound || !CopyField(child.sound, sound)) return false;
    } else if (child.kind == JAR_MP_SECTION_FIRE) {
        // The shared fire policy verifies the three stock FX keys. The exact
        // torch records have no authored angles or targets; native FX face up.
        if (!CopyField(child.sound, fire::loop) || !CopyField(child.effect, fire::effect)) return false;
        child.angles[PITCH] = -90;
    } else if (child.kind == JAR_MP_SECTION_DUST) {
        if (!CopyField(child.effect, fire::dust)) return false;
    } else if (child.kind == JAR_MP_SECTION_BOBBER) {
        // The shared bobbing policy requires the exact stock model and motion.
        child.inlineModel = 1;
        child.period = 1000 * std::atoi(admission::Value(record, "speed"));
        child.amplitude = std::strtof(admission::Value(record, "height"), nullptr);
        if (record.size() != 6 || !CopyField(child.model, admission::Value(record, "model2"))) return false;
    }
    return CopyField(child.classname, classname) &&
           CopyField(child.targetName, admission::Value(record, "targetname")) &&
           CopyField(child.target, admission::Value(record, "target"));
}
}

qboolean JAR_MP_InspectSection(const gameImport_t* game, jarMPSectionKind_t kind,
    jarMPSectionAdmission_t* out, char* reason, int reasonSize) {
    if (reason && reasonSize > 0) reason[0] = '\0';
    if (!out || kind <= JAR_MP_SECTION_NONE || kind >= JAR_MP_SECTION_COUNT)
        return Reject(reason, reasonSize, "The multiplayer section is outside the stock catalog.");
    if (!game || !game->FS_Open || !game->FS_Read || !game->FS_Close)
        return Reject(reason, reasonSize, "The server filesystem is unavailable.");

    const Profile& profile = profiles[kind];
    char path[MAX_QPATH];
    std::snprintf(path, sizeof(path), "maps/%s.bsp", profile.model + 1);
    std::vector<byte> bytes;
    {
        ScopedFile file(game, path);
        if (!file.handle || file.size != profile.size)
            return Reject(reason, reasonSize, "The selected retail section BSP is missing or differs in size.");
        bytes.resize(file.size);
        if (game->FS_Read(bytes.data(), file.size, file.handle) != file.size)
            return Reject(reason, reasonSize, "The retail section BSP read was incomplete.");
    }
    const unsigned checksum = LittleLong(Com_BlockChecksum(bytes.data(), profile.size));
    // The native engine decodes collision and renderer lumps beyond the shared
    // spawn policy. Publish only the qualified retail BSP identity.
    if (checksum != profile.checksum)
        return Reject(reason, reasonSize, "The section BSP differs from the qualified retail map.");

    admission::Report report;
    std::vector<admission::Entity> records;
    if (!admission::Inspect(bytes.data(), bytes.size(), report, profile.mode, &records))
        return Reject(reason, reasonSize, report.reason.c_str());
    const bool isKorriban = profile.mode == Mode::Korriban;
    const bool isYavin = profile.mode == Mode::Yavin;
    const int hurts = isKorriban ? ARRAY_LEN(korriban::fallBoxes) : profile.mode == Mode::Rift ? 2 : profile.hurtDamage != 0;
    const int bobbers = profile.mode == Mode::Bobbing;
    const int breakables = isYavin ? JAR_MP_YAVIN_BRUSHES : profile.mode == Mode::Breakable ? JAR_MP_CITY_BRUSHES : 0;
    const bool isSentinel = profile.mode == Mode::Sentinel;
    const bool isShaft = profile.mode == Mode::Shaft;
    const int lifts = isSentinel ? ARRAY_LEN(sentinel::lifts) : isShaft ? 2 : 0;
    const int pads = isSentinel ? ARRAY_LEN(sentinel::pads) : isShaft ? 2 : 0;
    const int scenery = isYavin ? 51 : isShaft ? 6 : 0;
    const int dust = isYavin ? 74 : 0;
    const int pickups = isSentinel ? sentinel::pickupCount : isKorriban ? korriban::pickupCount :
        profile.mode == Mode::Rift ? rift::pickupCount : 0;
    const int expectedChildren = profile.entries + profile.points + profile.intermissions +
        profile.speakers + profile.fires + hurts + bobbers + breakables + lifts + pads + pickups + scenery + dust + isYavin;
    if (records.size() != expectedChildren + 1u || expectedChildren > JAR_MP_SECTION_CHILDREN)
        return Reject(reason, reasonSize, "The section child count differs from retail.");

    jarMPSectionAdmission_t result{};
    result.model = profile.model;
    result.modelCount = report.models;
    result.childCount = expectedChildren;
    result.checksum = checksum;
    for (int axis = 0; axis < 3; ++axis) {
        result.mins[axis] = report.mins[axis] - 1;
        result.maxs[axis] = report.maxs[axis] + 1;
    }
    int counts[JAR_MP_SECTION_DUST_TIMER + 1]{};
    for (int i = 0; i < result.childCount; ++i) {
        auto& child = result.children[i];
        if (!ChildRecord(bytes.data(), profile, report, records[i + 1], child))
            return Reject(reason, reasonSize, "A section child has invalid local fields.");
        if (child.kind == JAR_MP_SECTION_BOBBER && !BobbingClip(bytes.data(), report, child))
            return Reject(reason, reasonSize, "The Bespin cloud car clip or motion is invalid.");
        ++counts[child.kind];
    }
    if (isYavin) for (int i = 0; i < result.childCount; ++i) {
        auto& child = result.children[i];
        if (child.kind != JAR_MP_SECTION_FIRE && child.kind != JAR_MP_SECTION_DUST) continue;
        const auto* aim = std::find_if(result.children, result.children + result.childCount, [&](const auto& point) {
            return !std::strcmp(point.classname, "info_notnull") && !Q_stricmp(point.targetName, child.target);
        });
        if (aim == result.children + result.childCount) return Reject(reason, reasonSize, "A Yavin effect has no local aim.");
        vec3_t direction;
        VectorSubtract(aim->origin, child.origin, direction);
        if (!VectorLengthSquared(direction)) return Reject(reason, reasonSize, "A Yavin effect has a zero-length aim.");
        vectoangles(direction, child.angles);
    }
    if (counts[JAR_MP_SECTION_ENTRY] != profile.entries || counts[JAR_MP_SECTION_POINT] != profile.points ||
        counts[JAR_MP_SECTION_INTERMISSION] != profile.intermissions || counts[JAR_MP_SECTION_SPEAKER] != profile.speakers ||
        counts[JAR_MP_SECTION_FIRE] != profile.fires || counts[JAR_MP_SECTION_HURT] != hurts ||
        counts[JAR_MP_SECTION_BOBBER] != bobbers || counts[JAR_MP_SECTION_BREAKABLE] != breakables ||
        counts[JAR_MP_SECTION_LIFT] != lifts || counts[JAR_MP_SECTION_PAD] != pads ||
        counts[JAR_MP_SECTION_PICKUP] != pickups || counts[JAR_MP_SECTION_SCENERY] != scenery ||
        counts[JAR_MP_SECTION_DUST] != dust || counts[JAR_MP_SECTION_DUST_TIMER] != isYavin)
        return Reject(reason, reasonSize, "The section local child types differ from retail.");

    if (profile.hurtDamage == -1) {
        // The native *falling1.wav configstring is a per-character alias.
        report.resources.emplace_back("sound/chars/mp_generic_male/misc/falling1.mp3");
        report.resources.emplace_back("sound/chars/mp_generic_female/misc/falling1.mp3");
    }
    if (breakables && !isYavin) {
        // MP's native light-stone fragments also use these bounce sounds.
        report.resources.emplace_back("sound/effects/stone_bounce.wav");
        report.resources.emplace_back("sound/effects/stone_bounce2.wav");
    }
    std::sort(report.resources.begin(), report.resources.end());
    report.resources.erase(std::unique(report.resources.begin(), report.resources.end()), report.resources.end());
    for (const auto& resource : report.resources) {
        ScopedFile file(game, resource.c_str());
        if (!file.handle || file.size <= 0) return Reject(reason, reasonSize, "A retail section resource is missing.");
        if (resource.size() >= 4 && resource.compare(resource.size() - 4, 4, ".md3") == 0) {
            md3Header_t header;
            const int headerBytes = sizeof(header);
            if (file.size <= headerBytes || game->FS_Read(&header, headerBytes, file.handle) != headerBytes ||
                LittleLong(header.ident) != MD3_IDENT || LittleLong(header.version) != MD3_VERSION ||
                LittleLong(header.numFrames) < 1 || LittleLong(header.numSurfaces) < 1 ||
                LittleLong(header.ofsEnd) <= headerBytes || LittleLong(header.ofsEnd) > file.size)
                return Reject(reason, reasonSize, "A retail section scenery model is invalid.");
        }
    }
    *out = result;
    return qtrue;
}
