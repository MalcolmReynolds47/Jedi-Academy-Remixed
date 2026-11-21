// Settings travel in the existing player-state stats mask for prediction.
#pragma once
#include "../../shared/qcommon/jar_movement_limits.h"
#include <limits.h>

// Zero on the wire means the map default; nonnegative values are stored plus one.
enum { JAR_MP_ACTOR_MOVEMENT_MAX = SHRT_MAX - 1 };

static inline int JAR_ActorMovementValue(const playerState_t *ps, int stat, int maximum) {
    if (ps->pm_type != PM_NORMAL || ps->m_iVehicleNum || ps->emplacedIndex || (ps->eFlags2 & EF2_HELD_BY_MONSTER))
        return -1;
    const int value = ps->stats[stat] - 1;
    return value >= 0 && value <= maximum ? value : -1;
}
