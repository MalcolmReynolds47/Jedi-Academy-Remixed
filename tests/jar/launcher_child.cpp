// Test executable for launcher argument and working-directory forwarding.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    wchar_t output[32768], directory[32768];
    if (!GetEnvironmentVariableW(L"JAR_LAUNCHER_TEST_OUTPUT", output, 32768) ||
        !GetCurrentDirectoryW(32768, directory)) return 1;
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args) return 2;
    std::wstring text = std::wstring(directory) + L"\n";
    for (int i = 0; i < count; ++i) text += std::wstring(args[i]) + L"\n";
    LocalFree(args);
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), int(text.size()), nullptr, 0, nullptr, nullptr);
    std::vector<char> utf8(length);
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), int(text.size()), utf8.data(), length, nullptr, nullptr);
    HANDLE file = CreateFileW(output, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 3;
    DWORD written = 0;
    const bool ok = WriteFile(file, utf8.data(), DWORD(utf8.size()), &written, nullptr) && written == utf8.size();
    CloseHandle(file);
    return ok ? 23 : 4;
}
