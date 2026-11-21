#include "qcommon/jar_diagnostics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <chrono>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void ObserveHandledException() {
    __try { RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}
#endif
int main(int argc,char **argv) {
    if(argc!=3) return 2;
    if(!JAR_LogOpen(argv[2],"diagnostic-test",std::strcmp(argv[1],"rotation")==0?8192:4*1024*1024))
        return std::strcmp(argv[1],"unwritable")==0?0:3;
    if(std::strcmp(argv[1],"unwritable")==0) return 4;
    if(std::strcmp(argv[1],"hold")==0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        JAR_LogClose(); return 0;
    }
    JAR_LogContext(42,12345);
    if(std::strcmp(argv[1],"abrupt")==0) {
        JAR_Log(0,"test","before_abrupt_exit","last operation is preserved");
#ifdef _WIN32
        ObserveHandledException();
#endif
        std::_Exit(77);
    }
    if(std::strcmp(argv[1],"rotation")==0) {
        for(int i=0;i<1800;++i) JAR_Log(1,"test","rotating","index=%d padding=abcdefghijklmnopqrstuvwxyz0123456789",i);
    } else {
        if(!JAR_DiagnosticCvar("g_gravity") || !JAR_DiagnosticCvar("R_SSAO") || JAR_DiagnosticCvar("g_password") || JAR_DiagnosticCvar("password") || JAR_DiagnosticCvar("name")) return 5;
        JAR_Log(1,"test","escaping","%s","quote\" slash\\ newline\n\t\001");
        JAR_Log(3,"test","hidden","must not be present");
        JAR_LogSetLevel(3);
        JAR_Log(3,"test","visible","enabled");
        char longText[2048]; std::memset(longText,'X',sizeof(longText)); longText[2047]=0;
        JAR_Log(1,"test","long","%s",longText);
        std::vector<std::thread> workers;
        for(int j=0;j<4;++j) workers.emplace_back([j](){for(int i=0;i<100;++i)JAR_Log(1,"thread","write","worker=%d index=%d",j,i);});
        for(auto& worker:workers) worker.join();
        for(int i=0;i<3000;++i) JAR_Log(1,"test","flood","index=%d",i);
        JAR_Log(0,"test","error_after_flood","must survive rate limit");
        std::this_thread::sleep_for(std::chrono::milliseconds(1050));
        JAR_Log(1,"test","after_window","suppression must be reported");
    }
    JAR_LogClose();
    return 0;
}
