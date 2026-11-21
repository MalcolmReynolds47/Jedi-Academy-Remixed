// JA:R private runtimes share the retail assets in their GameData parent.
// This changes only the default: explicit +set fs_basepath still wins.
#pragma once
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

inline bool JAR_RetailAssetsAt(const std::string& directory)
{
    struct stat info;
    for (const char* name : {"/base/assets0.pk3", "/base/assets1.pk3"})
        if (stat((directory + name).c_str(), &info) || !(info.st_mode & S_IFREG))
            return false;
    return true;
}

inline std::string JAR_DefaultInstallDirectory(std::string directory)
{
    const auto end = directory.find_last_not_of("\\/");
    if (end == std::string::npos) return directory;
    directory.resize(end + 1);
    const auto slash = directory.find_last_of("\\/");
    if (slash == std::string::npos) return directory;
    std::string name = directory.substr(slash + 1);
    for (auto& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (name != "jediacademyremixed" && name != "jediacademyremixedmp") return directory;
    // A deliberately standalone runtime with its own complete base remains valid.
    if (JAR_RetailAssetsAt(directory)) return directory;
    const std::string parent = directory.substr(0, slash);
    return JAR_RetailAssetsAt(parent) ? parent : directory;
}

#ifdef _WIN32
inline std::string JAR_ExecutableDirectory()
{
    char path[32768];
    const DWORD length = GetModuleFileNameA(nullptr, path, sizeof(path));
    if (!length || length >= sizeof(path)) return {};
    std::string directory(path, length);
    const auto slash = directory.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : directory.substr(0, slash);
}
#endif
