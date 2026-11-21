// Jedi Academy Remixed launcher. GPL-2.0-only; see LICENSE.txt.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

// Statically linked CRT: this executable must never depend on an app-local
// SDL or VC runtime in the shared, mixed-architecture GameData directory.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR arguments, int)
{
    wchar_t path[32768];
    const DWORD length = GetModuleFileNameW(nullptr, path, DWORD(sizeof(path) / sizeof(path[0])));
    if (!length || length >= sizeof(path) / sizeof(path[0])) return ERROR_BAD_PATHNAME;
    std::wstring gameDirectory(path, length);
    gameDirectory.resize(gameDirectory.find_last_of(L"\\/"));
#ifdef JAR_MP_LAUNCHER
    const std::wstring engine = gameDirectory + L"\\JediAcademyRemixedMP\\jar_mp.x86_64.exe";
#else
    const std::wstring engine = gameDirectory + L"\\JediAcademyRemixed\\jar_sp.x86_64.exe";
#endif
    std::wstring command = L"\"" + engine + L"\" +set fs_basepath \"" + gameDirectory + L"\" ";
#ifdef JAR_MP_LAUNCHER
    command += L"+set fs_game JediAcademyRemixedMP ";
#endif
    command += arguments;
    if (command.size() >= 32767) return ERROR_INVALID_PARAMETER;

    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child = {};
    if (!CreateProcessW(engine.c_str(), &command[0], nullptr, nullptr, FALSE, 0,
                        nullptr, gameDirectory.c_str(), &startup, &child))
    {
        const DWORD error = GetLastError();
        const std::wstring message = L"Unable to start Jedi Academy Remixed (Windows error " +
            std::to_wstring(error) + L").\n\nReinstall the complete JA:R package, including "
            L"its private runtime folders.\n\n" + engine;
        MessageBoxW(nullptr, message.c_str(), L"Jedi Academy Remixed", MB_OK | MB_ICONERROR);
        return int(error);
    }
    CloseHandle(child.hThread);
    WaitForSingleObject(child.hProcess, INFINITE);
    DWORD result = 1;
    GetExitCodeProcess(child.hProcess, &result);
    CloseHandle(child.hProcess);
    return int(result);
}
