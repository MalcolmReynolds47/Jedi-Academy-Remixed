// JA:R bounded, synchronously written diagnostic recorder. GPL-2.0-only.
#include "jar_diagnostics.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace {
std::mutex logMutex;
FILE *logFile = nullptr;
char logDirectory[1024] = {}, logPath[1100] = {};
std::atomic<int> logLevel(1), frameNumber(0), gameTime(0);
std::atomic<bool> healthy(false);
std::atomic<unsigned long long> sequence(0);
unsigned long processId;
unsigned long long sessionTime, windowTime, omitted;
unsigned int windowCount;
std::size_t bytesWritten, byteLimit;
using Clock = std::chrono::steady_clock;
Clock::time_point started;
#ifdef _WIN32
HANDLE crashFile = INVALID_HANDLE_VALUE;
HANDLE logLease = INVALID_HANDLE_VALUE;
PVOID exceptionHandler = nullptr;
LONG CALLBACK OnException(EXCEPTION_POINTERS *info) {
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    // Do not record expected C++ exceptions, breakpoints or single-step events.
    // An AV may be handled by another component; this records observation only.
    if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_STACK_OVERFLOW ||
        code == EXCEPTION_ILLEGAL_INSTRUCTION || code == 0xc0000374 || code == 0xc0000409)
        JAR_LogNativeException(code, info->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
int crashFile = -1;
#endif

void Path(char *out, std::size_t size, int index) {
    std::snprintf(out, size, "%s/jar-diagnostics-%d.jsonl", logDirectory, index);
}
bool Rotate() {
    if (logFile) { std::fclose(logFile); logFile = nullptr; }
    char from[1100], to[1100];
    for (int i=7; i>0; --i) {
        Path(to, sizeof(to), i); Path(from, sizeof(from), i-1);
        std::remove(to);
        std::rename(from, to);
    }
    Path(logPath, sizeof(logPath), 0);
    logFile = std::fopen(logPath, "wb");
    bytesWritten = 0;
    if (logFile) std::setvbuf(logFile, nullptr, _IONBF, 0);
    healthy.store(logFile != nullptr);
    return logFile != nullptr;
}
// ASCII JSON keeps arbitrary asset/console bytes parseable and escapes control
// characters, quotes and backslashes. Input is capped before escaping.
void Escape(const char *in, char *out, std::size_t capacity, std::size_t limit) {
    std::size_t n=0, i=0;
    if (!in) in="";
    for (; in[i] && i<limit && n+7<capacity; ++i) {
        unsigned char c=static_cast<unsigned char>(in[i]);
        if(c=='"' || c=='\\') { out[n++]='\\'; out[n++]=c; }
        else if(c<32 || c>=127) {
            std::snprintf(out+n,capacity-n,"\\u%04x",c); n+=6;
        } else out[n++]=c;
    }
    out[n]=0;
}
void Write(int level, const char *category, const char *event, const char *details, unsigned long long ms) {
    char cat[200], ev[400], data[6200], line[7200];
    Escape(category,cat,sizeof(cat),31); Escape(event,ev,sizeof(ev),63);
    Escape(details,data,sizeof(data),1023);
    const unsigned long long seq=++sequence;
    unsigned long tid=0;
#ifdef _WIN32
    tid=GetCurrentThreadId();
#endif
    int n=std::snprintf(line,sizeof(line),
        "{\"schema\":1,\"session\":\"%llu-%lu\",\"seq\":%llu,\"ms\":%llu,\"pid\":%lu,\"tid\":%lu,\"frame\":%d,\"game_ms\":%d,\"level\":%d,\"category\":\"%s\",\"event\":\"%s\",\"detail\":\"%s\"}\n",
        sessionTime,processId,seq,ms,processId,tid,frameNumber.load(),gameTime.load(),level,cat,ev,data);
    if(n<0 || static_cast<std::size_t>(n)>=sizeof(line)) return;
    if(bytesWritten+static_cast<std::size_t>(n)>byteLimit && !Rotate()) return;
    if(std::fwrite(line,1,n,logFile)!=static_cast<std::size_t>(n)) {
        healthy.store(false); std::fclose(logFile); logFile=nullptr; return;
    }
    bytesWritten+=n;
}
}

bool JAR_LogOpen(const char *directory, const char *build, std::size_t segmentBytes) {
    JAR_LogClose();
    if(!directory || std::strlen(directory)>=sizeof(logDirectory)) return false;
    std::strcpy(logDirectory,directory);
    started=Clock::now();
    sessionTime=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#ifdef _WIN32
    processId=GetCurrentProcessId();
#else
    processId=getpid();
#endif
    sequence=0; omitted=0; windowTime=0; windowCount=0;
    byteLimit=segmentBytes<8192 ? 8192 : segmentBytes;
#ifdef _WIN32
    char leasePath[1100];
    std::snprintf(leasePath,sizeof(leasePath),"%s/jar-diagnostics.lock",directory);
    logLease=CreateFileA(leasePath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    // Concurrent game instances must not rotate or truncate each other's logs.
    if(logLease==INVALID_HANDLE_VALUE) return false;
#endif
    if(!Rotate()) { JAR_LogClose(); return false; }
    char nativePath[1100], previous[1100];
    std::snprintf(nativePath,sizeof(nativePath),"%s/jar-native-exceptions.log",directory);
    std::snprintf(previous,sizeof(previous),"%s/jar-native-exceptions.previous.log",directory);
    std::remove(previous); std::rename(nativePath,previous);
#ifdef _WIN32
    crashFile=CreateFileA(nativePath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    exceptionHandler=AddVectoredExceptionHandler(1,OnException);
#else
    crashFile=open(nativePath,O_CREAT|O_WRONLY|O_TRUNC,0600);
#endif
    JAR_Log(0,"session","start","build=%s pid=%lu log=%s",build,processId,logPath);
    return true;
}
void JAR_Log(int level, const char *category, const char *event, const char *format, ...) {
    if(level>logLevel.load() || !healthy.load()) return;
    std::lock_guard<std::mutex> guard(logMutex);
    if(!logFile) return;
    auto ms=static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count());
    if(ms/1000!=windowTime) {
        if(omitted) {
            char msg[96]; std::snprintf(msg,sizeof(msg),"omitted=%llu limit=2000 events/second",omitted);
            Write(0,"logging","rate_limit",msg,ms);
            if(!logFile) return;
        }
        omitted=0; windowCount=0; windowTime=ms/1000;
    }
    // Error reports bypass the ordinary event budget. Suppression is explicit.
    if(level>0 && windowCount++>=2000) { ++omitted; return; }
    char details[1024];
    va_list args; va_start(args,format); int n=std::vsnprintf(details,sizeof(details),format,args); va_end(args);
    details[sizeof(details)-1]=0;
    if(n<0 || static_cast<std::size_t>(n)>=sizeof(details))
        std::memcpy(details+sizeof(details)-15,"...[truncated]",15);
    Write(level,category,event,details,ms);
}
void JAR_LogClose() {
    if(healthy.load()) JAR_Log(0,"session","end","pending_omitted=%llu",omitted);
#ifdef _WIN32
    if(exceptionHandler) { RemoveVectoredExceptionHandler(exceptionHandler); exceptionHandler=nullptr; }
    if(crashFile!=INVALID_HANDLE_VALUE) { CloseHandle(crashFile); crashFile=INVALID_HANDLE_VALUE; }
#else
    if(crashFile>=0) { close(crashFile); crashFile=-1; }
#endif
    std::lock_guard<std::mutex> guard(logMutex);
    if(logFile) std::fclose(logFile);
    logFile=nullptr; healthy=false;
#ifdef _WIN32
    if(logLease!=INVALID_HANDLE_VALUE) { CloseHandle(logLease); logLease=INVALID_HANDLE_VALUE; }
#endif
}
void JAR_LogSetLevel(int level) { logLevel=level<0 ? 0 : level>3 ? 3 : level; }
int JAR_LogLevel() { return logLevel.load(); }
bool JAR_LogHealthy() { return healthy.load(); }
const char *JAR_LogPath() { return logPath; }
void JAR_LogContext(int frame,int time) { frameNumber=frame; gameTime=time; }
void JAR_LogFlush() { std::lock_guard<std::mutex> guard(logMutex); if(logFile) std::fflush(logFile); }
bool JAR_DiagnosticCvar(const char *name) {
    if(!name) return false;
    char normalized[128];
    std::size_t i=0;
    for(;name[i] && i+1<sizeof(normalized);++i)
        normalized[i]=name[i]>='A' && name[i]<='Z' ? name[i]+('a'-'A') : name[i];
    normalized[i]=0;
    if(name[i]) return false;
    name=normalized;
    if(std::strstr(name,"password") || std::strstr(name,"secret") || std::strstr(name,"token")) return false;
    // Deliberately omit credentials, userinfo and arbitrary mod cvars.
    const char *prefixes[]={"r_","cg_","g_","jar_","timescale","com_maxfps","cl_renderer","s_volume","s_musicvolume","s_khz","s_useopenal"};
    for(const char *prefix:prefixes) if(!std::strncmp(name,prefix,std::strlen(prefix))) return true;
    return false;
}
void JAR_LogNativeException(unsigned long code,const void *address) {
    // No CRT, heap allocation, game access, formatting library or mutex here.
    // Fail-fast can bypass handlers; pre-failure JSONL and WER remain essential.
    static std::atomic<unsigned int> count(0);
    if(count.fetch_add(1)>=32) return;
    char out[100]="native_exception code=0x";
    std::size_t n=sizeof("native_exception code=0x")-1;
    const char hex[]="0123456789abcdef";
    for(int i=7;i>=0;--i) out[n++]=hex[(code>>(i*4))&15];
    const char text[]=" address=0x";
    for(char c:text) if(c) out[n++]=c;
    auto value=reinterpret_cast<std::size_t>(address);
    for(int i=sizeof(value)*2-1;i>=0;--i) out[n++]=hex[(value>>(i*4))&15];
    out[n++]='\n';
#ifdef _WIN32
    DWORD written;
    if(crashFile!=INVALID_HANDLE_VALUE) WriteFile(crashFile,out,static_cast<DWORD>(n),&written,nullptr);
#else
    if(crashFile>=0) write(crashFile,out,n);
#endif
}
