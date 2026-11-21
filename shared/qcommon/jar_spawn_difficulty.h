// Native single-player difficulty exclusions encoded in BSP spawnflags. GPL-2.0-only.
#pragma once

namespace jar {
inline bool SpawnAtDifficulty(unsigned spawnFlags, int skill) {
    return skill < -8 || skill > 23 || !(spawnFlags & (1u << (8 + skill)));
}
}
