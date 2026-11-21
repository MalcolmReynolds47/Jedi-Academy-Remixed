// Retail start card identities and shared labels. GPL-2.0-only.
#pragma once
#include <string.h>

static inline int JAR_StartCard(const char *classname)
{
    return classname && (!strcmp(classname, "info_player_start") || !strcmp(classname, "info_player_deathmatch"));
}
