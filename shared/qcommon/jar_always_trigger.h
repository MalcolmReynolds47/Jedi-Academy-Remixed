// Portable trigger_always timing limits, in milliseconds. GPL-2.0-only.
#pragma once
enum { JAR_ALWAYS_DEFAULT_MS = 300, JAR_ALWAYS_MIN_MS = 50, JAR_ALWAYS_MAX_MS = 60000 };

static inline int JAR_AlwaysDelayValid(int delay)
{
    return delay >= JAR_ALWAYS_MIN_MS && delay <= JAR_ALWAYS_MAX_MS;
}
