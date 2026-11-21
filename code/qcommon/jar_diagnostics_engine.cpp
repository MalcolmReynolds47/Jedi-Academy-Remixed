// JA:R engine diagnostic integration. GPL-2.0-only.
#include "q_shared.h"
#include "qcommon.h"
#include "jar_diagnostics.h"
#include "jar_graphics.h"
#include "game_version.h"
#include "client/client.h"
#include "server/server.h"
#include <chrono>
#ifndef JAR_BUILD_ID
#define JAR_BUILD_ID "manual-build"
#endif
extern int com_frameNumber;
extern void FS_Path_f();

namespace {
cvar_t *verbosity;
unsigned long long counters[JAR_COUNTER_COUNT] = {};
using Clock = std::chrono::steady_clock;
Clock::time_point previous, sample;
unsigned int frames=0;
double totalMs=0, worstMs=0;
void Mark() { JAR_Log(0,"user","mark","%s",Cmd_Args()); JAR_DiagnosticsSnapshot("mark"); }
void Status() {
    Com_Printf("JA:R diagnostics: %s, level %d, %s\n",JAR_LogPath(),JAR_LogLevel(),JAR_LogHealthy()?"writing":"unavailable");
    JAR_DiagnosticsSnapshot("console");
}
}
void JAR_DiagnosticsInit() {
    char directory[MAX_OSPATH];
    Com_sprintf(directory,sizeof(directory),"%s/diagnostics",Cvar_VariableString("fs_homepath"));
    Sys_Mkdir(Cvar_VariableString("fs_homepath")); Sys_Mkdir(directory);
    verbosity=Cvar_Get("jar_logLevel","1",CVAR_ARCHIVE);
    Cvar_CheckRange(verbosity,0,3,qtrue);
    JAR_LogSetLevel(verbosity->integer);
    if(!JAR_LogOpen(directory,JK_VERSION " build_id=" JAR_BUILD_ID " compiled=" __DATE__ " " __TIME__))
        Com_Printf("WARNING: JA:R diagnostics could not open %s\n",directory);
    else Com_Printf("JA:R diagnostics: %s\n",JAR_LogPath());
    Cmd_AddCommand("jar_diagnostics",Status);
    Cmd_AddCommand("jar_mark",Mark);
    previous=sample=Clock::now();
    JAR_Log(0,"session","paths","base=%s home=%s game=%s",Cvar_VariableString("fs_basepath"),Cvar_VariableString("fs_homepath"),Cvar_VariableString("fs_game"));
    JAR_Log(0,"capacity","profile","entities=%d reserve=%d ghoul2_handles=%d ghoul2_surfaces=%d scheduled_fx=%d save_version=%d ref_bits=%d",
        JAR_ENTITY_SLOTS,JAR_ENTITY_RESERVE,1<<JAR_GHOUL2_BITS,JAR_GHOUL2_RENDER_SURFACES,JAR_SCHEDULED_EFFECTS,JAR_SAVE_VERSION,JAR_REFENTITY_BITS);
    JAR_DiagnosticsSnapshot("startup");
    FS_Path_f(); // Preserve mounted asset/mod search order in the console trail.
}
void JAR_DiagnosticsSnapshot(const char *reason) {
    JAR_Log(0,"state","snapshot","reason=%s map=%s renderer=%s server_ms=%d frame=%d",
        reason,Cvar_VariableString("mapname"),Cvar_VariableString("cl_renderer"),sv.time,com_frameNumber);
    for(const auto& setting:jar::graphicsSettings) {
        const cvar_t *v=Cvar_FindVar(setting.name);
        if(v) JAR_Log(1,"cvar","snapshot","name=%s current=%s pending=%s flags=%x",v->name,v->string,v->latchedString?v->latchedString:"",v->flags);
    }
    const char *names[]={"g_gravity","timescale","com_maxfps","sv_cheats","s_khz","s_UseOpenAL"};
    for(const char *name:names) JAR_Log(1,"cvar","snapshot","name=%s current=%s",name,Cvar_VariableString(name));
    JAR_LogFlush();
}
void JAR_DiagnosticsCount(JARDiagnosticCounter counter) {
    if(counter>=0 && counter<JAR_COUNTER_COUNT) {
        static const char *names[]={"fx_drop","scheduled_fx_drop","loop_fx_drop","sound_drop","sound_replace","local_entity_recycle"};
        if(++counters[counter]==1) JAR_Log(1,"capacity","first_occurrence","counter=%s",names[counter]);
    }
}
void JAR_DiagnosticsFrame(int frame,int time) {
    JAR_LogContext(frame,sv.time);
    if(verbosity && verbosity->integer!=JAR_LogLevel()) {
        JAR_LogSetLevel(verbosity->integer);
        JAR_Log(0,"logging","level","value=%d",JAR_LogLevel());
    }
    auto now=Clock::now();
    double elapsed=std::chrono::duration<double,std::milli>(now-previous).count();
    previous=now; totalMs+=elapsed; ++frames;
    if(elapsed>worstMs) worstMs=elapsed;
    if(std::chrono::duration<double>(now-sample).count()<5.0) return;
    JAR_Log(1,"performance","frame_interval","frames=%u mean_ms=%.3f max_ms=%.3f map=%s renderer=%s paused=%s fx_dropped=%llu scheduled_dropped=%llu loop_dropped=%llu sound_dropped=%llu sound_stolen=%llu local_recycled=%llu",
        frames,totalMs/frames,worstMs,Cvar_VariableString("mapname"),Cvar_VariableString("cl_renderer"),Cvar_VariableString("cl_paused"),
        counters[0],counters[1],counters[2],counters[3],counters[4],counters[5]);
    frames=0; totalMs=worstMs=0; sample=now;
}
