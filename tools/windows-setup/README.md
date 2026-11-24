# Windows installer

The public setup contains the Windows engines, launchers, media builders and a private Python runtime. Setup generates the interface, effects, icons and models from the player's installed game files. An adjacent Jedi Outcast installation supplies optional legacy characters. Garry's Mod maps are a separate add-on. Players on a pure server should use the same optional media as the host.

Setup detects Steam library folders and accepts a manually selected Jedi Academy or GameData folder. It verifies the bundled files before installing, backs up changed files under `GameData/JA-R Backups` and verifies each replacement. Setup logs are saved under `%LOCALAPPDATA%/Jedi Academy Remixed/Setup`.

## Building a release

Use a clean checkout of the matching public game source in a neutral path. The build needs Visual Studio 2022 with the C++ workload, CMake, Python 3 and the Windows .NET Framework compiler. Run these commands from the checkout in PowerShell, replacing the version with the one in `shared/qcommon/jar_release.h`.

```powershell
$version = '1.350.0'
cmake -S . -B build/native -G "Visual Studio 17 2022" -A x64 -DBuildMPEngine=ON -DBuildMPRdVanilla=ON -DBuildMPRend2=ON -DBuildMPDed=ON -DBuildMPGame=ON -DBuildMPCGame=ON -DBuildMPUI=ON -DBuildSPRdVanilla=ON -DBuildSPRend2=ON "-DJARBuildId=release-$version"
cmake --build build/native --config Release --parallel
cmake --install build/native --config Release --prefix build/stage
python tools/windows-setup/build.py build/stage/JediAcademy build/installer/JA-R-Windows-x64-Setup.exe
```

The packager verifies the SHA-256 hashes of its pinned dependencies and accepts only the listed native files. For an unattended installation test, run the executable with `--install-to "<test GameData folder>"` and check its exit code. This uses the same installation path as the graphical installer.
