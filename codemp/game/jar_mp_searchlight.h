// Snapshot fields for a portable searchlight. GPL-2.0-only.
#pragma once
#include "../../shared/qcommon/jar_searchlight.h"

#define JAR_MP_SEARCHLIGHT_ID 95681750u
#define JAR_MP_SEARCHLIGHT_TAG 16
enum { JAR_SEARCHLIGHT_SAMPLE_MS = 50, JAR_SEARCHLIGHT_REARM_MS = 1000 };

static inline int JAR_MP_Searchlight(const entityState_t *state) {
    return state->eType == ET_GENERAL && state->generic1 == 250 && state->bolt2 == JAR_MP_SEARCHLIGHT_TAG;
}
