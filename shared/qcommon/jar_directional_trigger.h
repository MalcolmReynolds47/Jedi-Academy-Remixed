// Center-plane crossing for portable directional triggers. GPL-2.0-only.
#pragma once

enum {
    JAR_DIRECTIONAL_MIN_EXTENT = 32,
    JAR_DIRECTIONAL_MAX_EXTENT = 512,
    JAR_DIRECTIONAL_DEFAULT_EXTENT = 64,
    JAR_DIRECTIONAL_MAX_WAIT = 3600000,
    JAR_DIRECTIONAL_MAX_USES = 100000,
    JAR_DIRECTIONAL_PLAYER_ONLY = 1
};

static inline int JAR_DirectionalCrossing(const vec3_t center, const vec3_t forward,
                                          const vec3_t previous, const vec3_t current, vec3_t crossing) {
    vec3_t movement, before, after;
    VectorSubtract(current, previous, movement);
    const float distance = VectorLengthSquared(movement);
    if (!isfinite(distance) || distance > 1024.f * 1024.f) return 0;
    VectorSubtract(previous, center, before);
    VectorSubtract(current, center, after);
    const float a = DotProduct(before, forward), b = DotProduct(after, forward);
    if (!isfinite(a) || !isfinite(b)) return 0;
    const int direction = a <= 0 && b > 0 ? 1 : a >= 0 && b < 0 ? -1 : 0;
    if (direction) VectorMA(previous, -a / (b - a), movement, crossing);
    return direction;
}
