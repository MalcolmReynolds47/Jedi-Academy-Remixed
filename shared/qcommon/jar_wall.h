// Stock model for the portable Sandbox wall. GPL-2.0-only.
#pragma once
#include <string.h>

#define JAR_WALL_MODEL "models/map_objects/rocky_ruins/cell_wall.md3"
#define JAR_WALL_PORTABLE_MODEL "@jar_wall"
#define JAR_WALL_CATALOG_ID 99448732u
#define JAR_MP_WALL_OFF_TAG 10

static inline int JAR_WallClass(const char *classname)
{
    return classname && !strcmp(classname, "func_wall");
}

static inline int JAR_WallPortable(const char *classname, const char *model)
{
    return JAR_WallClass(classname) && model &&
           (!strcmp(model, JAR_WALL_PORTABLE_MODEL) || !strcmp(model, "@jar_box"));
}
