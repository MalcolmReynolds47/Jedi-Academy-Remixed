// The SP game also compiles client FX code. Forward its diagnostic calls to
// the engine-owned recorder rather than opening an independent log per DLL.
#include "g_local.h"
#include "qcommon/jar_diagnostics.h"
#include <cstdarg>
#include <cstdio>
void JAR_Log(int level,const char *category,const char *event,const char *format,...) {
    if(!gi.JAR_Log || (gi.JAR_LogLevel && level>gi.JAR_LogLevel())) return;
    char message[1024];
    va_list args; va_start(args,format); std::vsnprintf(message,sizeof(message),format,args); va_end(args);
    message[sizeof(message)-1]=0;
    gi.JAR_Log(level,category,event,"%s",message);
}
void JAR_DiagnosticsCount(JARDiagnosticCounter counter) {
    if(gi.JAR_DiagnosticsCount) gi.JAR_DiagnosticsCount(counter);
}
