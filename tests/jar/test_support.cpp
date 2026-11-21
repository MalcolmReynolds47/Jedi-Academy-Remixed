#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "test_support.h"

void QDECL Com_Error(int, const char *format, ...)
{
    char message[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    throw std::runtime_error(message);
}
void QDECL Com_Printf(const char *, ...) {}
void QDECL Com_DPrintf(const char *, ...) {}

static cvar_t quiet = {};
cvar_t *cl_shownet = &quiet;

#ifndef JAR_TEST_NATIVE_PARSER
int QDECL Com_sprintf(char *dest, int size, const char *format, ...)
{
    va_list args; va_start(args, format);
    int result = vsnprintf(dest, size, format, args);
    va_end(args); return result;
}
char *QDECL va(const char *format, ...)
{
    // Match q_shared.cpp's four rotating buffers for nested va arguments.
    static char buffers[4][2048];
    static unsigned index=0;
    char *text=buffers[index++&3];
    va_list args; va_start(args, format);
    vsnprintf(text, sizeof(buffers[0]), format, args);
    va_end(args); return text;
}
#endif // JAR_TEST_NATIVE_PARSER
#ifndef JAR_TEST_CUSTOM_CVARS
cvar_t *Cvar_Get(const char *, const char *, int) { return &quiet; }
#endif
void *Z_Malloc(int size, memtag_t, qboolean, int) { return calloc(1, size); }
int Z_Free(void *ptr) { free(ptr); return 0; }

// Production hooks are disabled in unrelated headless unit fixtures.
void JAR_Log(int,const char*,const char*,const char*,...) {}
