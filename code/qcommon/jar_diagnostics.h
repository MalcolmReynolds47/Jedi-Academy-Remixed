// JA:R diagnostic flight recorder. GPL-2.0-only.
#ifndef JAR_DIAGNOSTICS_H
#define JAR_DIAGNOSTICS_H
#include <cstddef>

// One engine-owned sink shared by the game and both renderers through their
// versioned import tables. Level 0: errors; 1: lifecycle; 2: allocations;
// 3: individual sound/FX requests. No game state or random stream is changed.
void JAR_Log(int level, const char *category, const char *event, const char *format, ...);
bool JAR_LogOpen(const char *directory, const char *build, std::size_t segmentBytes = 4*1024*1024);
void JAR_LogClose();
void JAR_LogSetLevel(int level);
int JAR_LogLevel();
void JAR_LogContext(int frame, int gameTime);
const char *JAR_LogPath();
bool JAR_LogHealthy();
void JAR_LogFlush();
bool JAR_DiagnosticCvar(const char *name);
void JAR_LogNativeException(unsigned long code, const void *address);

// Engine integration; separate from the independently testable recorder.
void JAR_DiagnosticsInit();
void JAR_DiagnosticsFrame(int frame, int gameTime);
void JAR_DiagnosticsSnapshot(const char *reason);
enum JARDiagnosticCounter { JAR_FX_DROPPED, JAR_FX_SCHEDULE_DROPPED, JAR_FX_LOOP_DROPPED,
    JAR_SOUND_DROPPED, JAR_SOUND_STOLEN, JAR_LOCAL_RECYCLED, JAR_COUNTER_COUNT };
void JAR_DiagnosticsCount(JARDiagnosticCounter counter);
#endif
