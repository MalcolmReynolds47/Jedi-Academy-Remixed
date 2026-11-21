// Property/assembly fixtures have no authored tags. Actual authored precedence
// and TAG_Get* integration are tested separately in reference_tags.cpp.
#include "game/g_reference_tags.h"
static reference_tag_t*TAG_FindAuthored(const char*,const char*){return nullptr;}
#include "game/g_reference_tags.inc"
#include "game/g_nav.h"
static int combatNearestNode(const vec3_t&){return 1;}
#include "game/g_combat_points.inc"
#include "game/g_skyportal.inc"
