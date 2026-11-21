// Sandbox distance trigger settings and link roles. GPL-2.0-only.
#pragma once

enum {
    JAR_DISTANCE_MIN = 1, JAR_DISTANCE_MAX = 4096, JAR_DISTANCE_DEFAULT = 192,
    JAR_DISTANCE_PLAYER = 1, JAR_DISTANCE_NPC = 2,
    JAR_DISTANCE_INSIDE = 0, JAR_DISTANCE_OUTSIDE, JAR_DISTANCE_REFERENCE, JAR_DISTANCE_SUBJECT,
    JAR_DISTANCE_LINKS, JAR_DISTANCE_LINK_MISSING = -1
};

static inline int JAR_DistanceSettingsValid(int range, int subjects)
{
    return range >= JAR_DISTANCE_MIN && range <= JAR_DISTANCE_MAX &&
           subjects >= JAR_DISTANCE_PLAYER && subjects <= (JAR_DISTANCE_PLAYER | JAR_DISTANCE_NPC);
}
