// Exact-stock MP portable section admission. GPL-2.0-only.
#ifndef JAR_MP_SECTION_ADMISSION_H
#define JAR_MP_SECTION_ADMISSION_H
#include "qcommon/q_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gameImport_s;
enum {
    JAR_MP_SECTION_CHILDREN = 323, JAR_MP_SENTINEL_CHILDREN = 228,
    JAR_MP_KORRIBAN_CHILDREN = 310, JAR_MP_KORRIBAN_PICKUPS = 47,
    JAR_MP_CITY_BRUSHES = 69, JAR_MP_CITY_DAMAGE_BYTES = 18,
    JAR_MP_YAVIN_CHILDREN = 323, JAR_MP_YAVIN_BRUSHES = 30,
    JAR_MP_YAVIN_DAMAGE_BYTES = 15
};
typedef enum {
    JAR_MP_SECTION_NONE,
    JAR_MP_SECTION_IMPERIAL,
    JAR_MP_SECTION_VJUN,
    JAR_MP_SECTION_RANCOR,
    JAR_MP_SECTION_GENERATOR,
    JAR_MP_SECTION_TASPIR,
    JAR_MP_SECTION_BESPIN,
    JAR_MP_SECTION_CITY,
    JAR_MP_SECTION_SENTINEL,
    JAR_MP_SECTION_HOTH,
    JAR_MP_SECTION_RIFT,
    JAR_MP_SECTION_SHAFT,
    JAR_MP_SECTION_KORRIBAN,
    JAR_MP_SECTION_YAVIN,
    JAR_MP_SECTION_COUNT
} jarMPSectionKind_t;

static inline jarMPSectionKind_t JAR_MP_SectionKind(const char* card) {
    if (!strcmp(card, "Imperial Control Room")) return JAR_MP_SECTION_IMPERIAL;
    if (!strcmp(card, "Vjun Fuel Processing")) return JAR_MP_SECTION_VJUN;
    if (!strcmp(card, "Rancor Pit")) return JAR_MP_SECTION_RANCOR;
    if (!strcmp(card, "Generator Room")) return JAR_MP_SECTION_GENERATOR;
    if (!strcmp(card, "Taspir Landing")) return JAR_MP_SECTION_TASPIR;
    if (!strcmp(card, "Bespin Courtyard")) return JAR_MP_SECTION_BESPIN;
    if (!strcmp(card, "Abandoned City")) return JAR_MP_SECTION_CITY;
    if (!strcmp(card, "Vjun Sentinel")) return JAR_MP_SECTION_SENTINEL;
    if (!strcmp(card, "Hoth Canyon")) return JAR_MP_SECTION_HOTH;
    if (!strcmp(card, "Rift Sanctuary")) return JAR_MP_SECTION_RIFT;
    if (!strcmp(card, "Imperial Shaft")) return JAR_MP_SECTION_SHAFT;
    if (!strcmp(card, "Korriban FFA")) return JAR_MP_SECTION_KORRIBAN;
    if (!strcmp(card, "Yavin Training Grounds")) return JAR_MP_SECTION_YAVIN;
    return JAR_MP_SECTION_NONE;
}
typedef enum {
    JAR_MP_SECTION_POINT,
    JAR_MP_SECTION_ENTRY,
    JAR_MP_SECTION_INTERMISSION,
    JAR_MP_SECTION_SPEAKER,
    JAR_MP_SECTION_FIRE,
    JAR_MP_SECTION_HURT,
    JAR_MP_SECTION_BOBBER,
    JAR_MP_SECTION_BREAKABLE,
    JAR_MP_SECTION_LIFT,
    JAR_MP_SECTION_PAD,
    JAR_MP_SECTION_PICKUP,
    JAR_MP_SECTION_SCENERY,
    JAR_MP_SECTION_DUST,
    JAR_MP_SECTION_DUST_TIMER
} jarMPSectionChildKind_t;

typedef struct {
    jarMPSectionChildKind_t kind;
    vec3_t origin, angles, scale;
    vec3_t mins, maxs; // Exact hazard extents relative to its local center.
    vec3_t secondMins, secondMaxs; // Second brush in Rift's compound fall volume.
    int damage, spawnflags, health;
    int inlineModel, period;
    float amplitude, travel;
    int group;
    char classname[32], targetName[MAX_QPATH], target[MAX_QPATH], sound[MAX_QPATH], effect[MAX_QPATH];
    char model[MAX_QPATH]; // A moving brush's separate visible model.
    char liftStart[MAX_QPATH], liftStop[MAX_QPATH];
} jarMPSectionChild_t;

typedef struct {
    vec3_t mins, maxs; // Local bounds include the native one-unit collision margin.
    const char* model; // Canonical engine resource from the qualified profile.
    unsigned checksum;
    int modelCount, childCount;
    jarMPSectionChild_t children[JAR_MP_SECTION_CHILDREN];
} jarMPSectionAdmission_t;

qboolean JAR_MP_InspectSection(const struct gameImport_s* game, jarMPSectionKind_t kind,
    jarMPSectionAdmission_t* out, char* reason, int reasonSize);

#ifdef __cplusplus
}
#endif
#endif
