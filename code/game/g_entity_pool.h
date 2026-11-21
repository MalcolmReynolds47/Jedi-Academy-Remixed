// Jedi Academy Remixed entity allocation policy. GPL-2.0-only.
#ifndef JAR_ENTITY_POOL_H
#define JAR_ENTITY_POOL_H

namespace jar {

// Return an existing reusable slot or the next unused slot. The upper bound is
// exclusive, so the world/none sentinels and the critical reserve stay intact.
// Callbacks let the real game's in-use bitset remain the source of truth,
// including after a savegame load. No parallel free list can become stale.
template<class InUse, class FreeTime>
int FindEntitySlot(int first, int highWater, int limit, int now,
                   InUse inUse, FreeTime freeTime)
{
    const int end = highWater < limit ? highWater : limit;
    for (int force = 0; force < 2; ++force)
    {
        for (int i = first; i < end; ++i)
        {
            if (inUse(i))
                continue;
            if (!force && freeTime(i) > 2000 && now - freeTime(i) < 1000)
                continue;
            return i;
        }
        if (highWater < limit)
            return highWater < first ? first : highWater;
    }
    return -1;
}

} // namespace jar
#endif
