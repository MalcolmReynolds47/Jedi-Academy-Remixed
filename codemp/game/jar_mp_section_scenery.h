// Scaled, non-solid scenery owned by a portable section. GPL-2.0-only.
#ifndef JAR_MP_SECTION_SCENERY_H
#define JAR_MP_SECTION_SCENERY_H

#define JAR_MP_SECTION_SCENERY_MARKER 248

static inline qboolean JAR_MP_SectionScenery(const entityState_t *state) {
    return state->eType == ET_GENERAL && state->generic1 == JAR_MP_SECTION_SCENERY_MARKER &&
        state->otherEntityNum >= MAX_CLIENTS && state->otherEntityNum < ENTITYNUM_WORLD ? qtrue : qfalse;
}

#endif
