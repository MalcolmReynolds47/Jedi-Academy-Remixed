// JA:R optional decision scheduling. Physics and scripts retain their normal ticks.
// GPL-2.0-only; see LICENSE.txt.
#ifndef JAR_NPC_LOD_H
#define JAR_NPC_LOD_H
namespace jar {
inline int NpcDecisionInterval(bool enabled, bool idle, float distanceSquared, int normal) {
    if (!enabled || !idle || !(distanceSquared > 1024.0f * 1024.0f)) return normal;
    return distanceSquared > 2048.0f * 2048.0f ? 400 : 200;
}
// Spread work across physics ticks, without extending the maximum interval.
inline int NpcNextDecision(int now, int interval, int entity, int physicsTick) {
    const int phase = (entity % (interval / physicsTick)) * physicsTick;
    int delay = (phase - now % interval + interval) % interval;
    return now + (delay ? delay : interval);
}
}
#endif
