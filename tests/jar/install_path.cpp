// Runs only startup path discovery. No engine initialization or game assets.
#include "../../shared/sys/jar_install_path.h"
#include <cstdio>
int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    const std::string binary = JAR_ExecutableDirectory();
    const std::string install = JAR_DefaultInstallDirectory(binary);
    if (binary != argv[1] || install != argv[2]) {
        std::fprintf(stderr, "binary=%s\ninstall=%s\n", binary.c_str(), install.c_str());
        return 1;
    }
    return 0;
}
