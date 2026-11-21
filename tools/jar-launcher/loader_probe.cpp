// Windows loader diagnostic. Stops only the process it creates, at the loader
// breakpoint before the program entry point. No game assets or settings run.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) return 2;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    std::wstring directory(argv[1]);
    directory.resize(directory.find_last_of(L"\\/"));
    STARTUPINFOW startup = {}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(argv[1], &command[0], nullptr, nullptr, FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, directory.c_str(), &startup, &process)) {
        std::printf("CreateProcess failed: %lu\n", GetLastError()); return 3;
    }
    const ULONGLONG deadline = GetTickCount64() + 15000;
    BOOL wow64 = FALSE;
    IsWow64Process(process.hProcess, &wow64);
    bool loadedSDL = false, passed = false, exited = false;
    while (GetTickCount64() < deadline && !exited) {
        DEBUG_EVENT event = {};
        if (!WaitForDebugEvent(&event, 500)) continue;
        DWORD disposition = DBG_CONTINUE;
        HANDLE file = nullptr;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            file = event.u.CreateProcessInfo.hFile;
            CloseHandle(event.u.CreateProcessInfo.hThread);
            CloseHandle(event.u.CreateProcessInfo.hProcess);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            file = event.u.LoadDll.hFile;
            wchar_t path[32768] = {};
            if (file && GetFinalPathNameByHandleW(file, path, 32768, 0)) {
                std::wstring name(path);
                for (auto& c : name) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
                if (name.find(L"sdl2.dll") != std::wstring::npos) loadedSDL = true;
                if (name.find(L"sdl2") != std::wstring::npos || name.find(L"msvcp") != std::wstring::npos ||
                    name.find(L"vcruntime") != std::wstring::npos || name.find(L"msvcr120") != std::wstring::npos)
                    std::wprintf(L"Loaded: %ls\n", path);
            }
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            const DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT || code == 0x4000001f) {
                // WOW64 first stops in the native bootstrap, before loading
                // the 32-bit imports. Its second breakpoint marks that stage.
                if (wow64 && code == EXCEPTION_BREAKPOINT) {
                    std::printf("WOW64 native bootstrap breakpoint\n");
                } else {
                    passed = argc > 2 || loadedSDL;
                    std::printf("Loader breakpoint: 0x%08lx; SDL loaded: %s\n", code, loadedSDL ? "yes" : "no");
                    TerminateProcess(process.hProcess, 0);
                }
            } else {
                std::printf("Loader exception: 0x%08lx\n", code);
                disposition = DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            exited = true;
            std::printf("Exit: 0x%08lx\n", event.u.ExitProcess.dwExitCode);
        }
        if (file) CloseHandle(file);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    if (!exited) TerminateProcess(process.hProcess, 1);
    WaitForSingleObject(process.hProcess, 2000);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    std::printf("%s: native dependency loading before application entry\n", passed && exited ? "PASS" : "FAIL");
    return passed && exited ? 0 : 1;
}
