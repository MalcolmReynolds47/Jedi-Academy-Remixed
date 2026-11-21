[CmdletBinding()]
param(
    [ValidateSet('Release', 'RelWithDebInfo')]
    [string]$Configuration = 'Release',
    [ValidateRange(1, 32)]
    [int]$Parallel = 6,
    [string]$GameDataPath = 'C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData',
    [string]$LocalMapPack,
    [string]$PythonExecutable = (Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'),
    [switch]$Install,
    [switch]$SkipInstall
)

$ErrorActionPreference = 'Stop'
if ($Install -and $SkipInstall) { throw 'Choose either -Install or -SkipInstall.' }
$jarRoot = Split-Path -Parent $PSScriptRoot
$jarBuildStarted = [DateTime]::UtcNow
New-Item -ItemType Directory -Path (Join-Path $jarRoot 'build') -Force | Out-Null
$jarBuildLock = [IO.File]::Open((Join-Path $jarRoot 'build/.jar-build.lock'),
    [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try {
$jarBuild = Join-Path $jarRoot 'build\jar'
$jarStage = Join-Path $jarRoot 'build\jar-package'
$jarRuntime = Join-Path $jarStage 'JediAcademy'
$jarPrivate = Join-Path $jarRuntime 'JediAcademyRemixed'
# One public version feeds runtime banners/resources and the exact package name.
$jarReleaseHeader = Get-Content -LiteralPath (Join-Path $jarRoot 'shared\qcommon\jar_release.h') -Raw
$jarReleaseParts = foreach ($jarPart in @('MAJOR','MINOR','PATCH','BUILD')) {
    $jarMatch = [regex]::Match($jarReleaseHeader, "(?m)^#define JAR_VERSION_$jarPart ([0-9]+)\r?$")
    if (-not $jarMatch.Success -or [int64]$jarMatch.Groups[1].Value -gt 65535) { throw "Invalid JA:R release part: $jarPart" }
    [int]$jarMatch.Groups[1].Value
}
$jarReleaseVersion = $jarReleaseParts[0..2] -join '.'
function Read-JarApiVersion([string]$Header, [string]$Define) {
    $headerText = Get-Content -LiteralPath (Join-Path $jarRoot $Header) -Raw
    $match = [regex]::Match($headerText, "(?m)^#define[ \t]+$Define[ \t]+([0-9]+)\b")
    if (!$match.Success) { throw "Cannot read $Define from $Header" }
    return [int]$match.Groups[1].Value
}
$jarRefApi = Read-JarApiVersion 'code\rd-common\tr_public.h' 'REF_API_VERSION'
$jarGameApi = Read-JarApiVersion 'code\game\g_public.h' 'GAME_API_VERSION'
$jarZip = Join-Path $jarRoot ("build\JediAcademyRemixed-$jarReleaseVersion-win64.zip")
$jarBuildId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$jarSymbols = Join-Path $jarRoot ('build\symbols\' + $jarBuildId)
. (Join-Path $PSScriptRoot 'jar-build-retention.ps1')

function Invoke-JarTool([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-legacy-npcs.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-yt1300.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\yt1300_assets.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-mp-legacy-droids.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\mp_legacy_droids.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\legacy_npc_assets.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-spawn-catalog.py'), '--game-data', $GameDataPath, '--check')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\spawn_catalog.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\mp_frontend_assets.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-mp-catalog.py'))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\mp_pose_smoke.py'), '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-prop-poses.py'), '--game-data', $GameDataPath)
Invoke-JarTool 'cmake' @('-S', $jarRoot, '-B', $jarBuild, '-G', 'Visual Studio 17 2022', '-A', 'x64',
    '-DBuildMPEngine=ON', '-DBuildMPRdVanilla=ON', '-DBuildMPRend2=ON', '-DBuildMPDed=ON', '-DBuildMPGame=ON', '-DBuildMPCGame=ON', '-DBuildMPUI=ON', '-DBuildJARTests=ON', '-DBuildSPRdVanilla=ON', '-DBuildSPRend2=ON', "-DJARBuildId=$jarBuildId", "-DCMAKE_INSTALL_PREFIX=$jarStage")
Invoke-JarTool 'cmake' @('--build', $jarBuild, '--config', $Configuration, '--parallel', "$Parallel")
Invoke-JarTool 'ctest' @('--test-dir', $jarBuild, '-C', $Configuration, '--output-on-failure')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\retail_resources.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\retail_bsp_lighting.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\retail_bsp_fogs.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\retail_bsp_admission.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-vehicle-variants.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-droids.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-yt1300.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--curated-catalog')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-lights', '--network-music')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-hazard-sections')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-city-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-sentinel-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-hoth-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-rift-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-shaft-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-korriban-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-yavin-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-movement')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-space')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-directional')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-searchlight')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-portals')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-angular')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--network-console-npcs')
# A fresh, verified staging path prevents the previous flat runtime layout
# from leaking old DLLs into a newly packaged release.
if (Test-Path -LiteralPath $jarStage) {
    $jarResolvedStage = (Resolve-Path -LiteralPath $jarStage).Path
    $jarExpectedStage = [IO.Path]::GetFullPath((Join-Path $jarRoot 'build\jar-package'))
    if ($jarResolvedStage -ne $jarExpectedStage -or
        ((Get-Item -LiteralPath $jarStage -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Unsafe package staging directory: $jarResolvedStage"
    }
    Remove-Item -LiteralPath $jarResolvedStage -Recurse -Force
}
Invoke-JarTool 'cmake' @('--install', $jarBuild, '--config', $Configuration, '--prefix', $jarStage)

$jarMPPrivate = Join-Path $jarRuntime 'JediAcademyRemixedMP'
New-Item -ItemType Directory -Path $jarMPPrivate -Force | Out-Null
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-mp-player-models.py'),
    '--game-data', $GameDataPath, '--output', (Join-Path $jarMPPrivate 'jar-mp-player-models.pk3'),
    '--companion', (Join-Path $jarRoot 'build\jar-legacy-npcs.pk3'),
    '--companion', (Join-Path $jarRoot 'build\jar-mp-legacy-droids.pk3'))
if (!$LocalMapPack) {
    $jarInstalledMapPack = Join-Path $GameDataPath 'JediAcademyRemixed\jar-gmod-maps.pk3'
    if (Test-Path -LiteralPath $jarInstalledMapPack -PathType Leaf) {
        $LocalMapPack = $jarInstalledMapPack
    }
}
if ($LocalMapPack) {
    Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\gmod_map_pack.py'), $LocalMapPack)
    foreach ($jarDestination in @($jarPrivate, $jarMPPrivate)) {
        Copy-Item -LiteralPath $LocalMapPack -Destination (Join-Path $jarDestination 'jar-gmod-maps.pk3')
    }
    Write-Host 'Local Construct and Flatgrass pack staged for SP and MP.'
}
Copy-Item -LiteralPath (Join-Path $jarRoot 'build\jar-mp-legacy-droids.pk3') -Destination $jarMPPrivate
foreach ($jarLegacyDestination in @($jarPrivate, $jarMPPrivate)) {
    Copy-Item -LiteralPath (Join-Path $jarRoot 'build\jar-legacy-npcs.pk3') -Destination $jarLegacyDestination
    Copy-Item -LiteralPath (Join-Path $jarRoot 'build\jar-yt1300.pk3') -Destination $jarLegacyDestination
}
foreach ($name in @('jar-mp-controls.cfg','jar-server-example.cfg')) {
    Copy-Item -LiteralPath (Join-Path $jarRoot ('assets\mp\' + $name)) -Destination $jarMPPrivate
}
foreach ($name in @('Play-JA-R-Multiplayer.cmd','Play-JA-R-Multiplayer-Rend2.cmd','Host-JA-R-Server.cmd')) {
    Copy-Item -LiteralPath (Join-Path $jarRoot ('assets\mp\' + $name)) -Destination $jarRuntime
}
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-multiplayer.md') -Destination $jarMPPrivate
Copy-Item -LiteralPath (Join-Path $jarPrivate 'SDL2.dll') -Destination $jarMPPrivate
Copy-Item -LiteralPath (Join-Path $jarRoot 'lib\box3d\LICENSE') -Destination (Join-Path $jarMPPrivate 'Box3D-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $jarRoot 'lib\box3d\SOURCE.json') -Destination (Join-Path $jarMPPrivate 'Box3D-SOURCE.json')

Copy-Item -LiteralPath (Join-Path $jarRoot 'README-JAR.md') -Destination $jarRuntime
Copy-Item -LiteralPath (Join-Path $jarRoot 'LICENSE.txt') -Destination $jarPrivate
New-Item -ItemType Directory -Path (Join-Path $jarPrivate 'docs') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-validation.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-diagnostics.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-appearance.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-randomizer.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-overhaul-checkpoint.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-melee-inventory-plan.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-inventory-sandbox.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-sandbox-replacement.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-owner-acceptance-2026-10-10.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-physics.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-spawn-catalog.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-spawn-catalog.json') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-catalog-exclusions.json') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'lib\box3d\LICENSE') -Destination (Join-Path $jarPrivate 'Box3D-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $jarRoot 'lib\box3d\SOURCE.json') -Destination (Join-Path $jarPrivate 'Box3D-SOURCE.json')
if (-not (Test-Path -LiteralPath $PythonExecutable -PathType Leaf)) { throw 'Appearance packaging requires Python with Pillow; supply -PythonExecutable.' }
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-sandbox-icons.py'), '--game-data', $GameDataPath, '--output', (Join-Path $jarPrivate 'jar-sandbox-icons.pk3'))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\sandbox_assets.py'), '--game-data', $GameDataPath, '--pack', (Join-Path $jarPrivate 'jar-sandbox-icons.pk3'))
Copy-Item -LiteralPath (Join-Path $jarPrivate 'jar-sandbox-icons.pk3') -Destination $jarMPPrivate
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-player-appearance.py'),
    '--game-data', $GameDataPath, '--build', (Join-Path $jarBuild $Configuration), '--media', $jarMPPrivate)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-appearance.py'), '--game-data', $GameDataPath,
    '--output', (Join-Path $jarPrivate 'jar-appearance.pk3'))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\appearance_assets.py'), '--game-data', $GameDataPath,
    '--pack', (Join-Path $jarPrivate 'jar-appearance.pk3'))
# Reuse the exact SP font/UI bytes and notices in MP's private search path.
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-mp-interface.py'),
    '--source', (Join-Path $jarPrivate 'jar-appearance.pk3'), '--output', (Join-Path $jarMPPrivate 'jar-interface.pk3'))
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\build-mp-effects.py'),
    '--source', (Join-Path $jarPrivate 'jar-appearance.pk3'), '--output', (Join-Path $jarMPPrivate 'jar-effects.pk3'),
    '--game-data', $GameDataPath)
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'tests\jar\mp_effect_assets.py'),
    '--source', (Join-Path $jarPrivate 'jar-appearance.pk3'), '--pack', (Join-Path $jarMPPrivate 'jar-effects.pk3'), '--game-data', $GameDataPath)


# Validate the exact staged MP archive set with independent client/server copies.
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'),
    '--game-data', $GameDataPath, '--build', $jarMPPrivate,
    '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-lights', '--network-music')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'),
    '--game-data', $GameDataPath, '--build', $jarMPPrivate,
    '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-angular')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', (Join-Path $jarRuntime 'JediAcademyRemixedMP'),
    '--wire-probe', (Join-Path (Join-Path $jarBuild $Configuration) 'jar_mp_transport_tests.exe'),
    '--network-pure', '--network-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', (Join-Path $jarRuntime 'JediAcademyRemixedMP'),
    '--wire-probe', (Join-Path (Join-Path $jarBuild $Configuration) 'jar_mp_transport_tests.exe'),
    '--network-pure', '--network-hazard-sections')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', (Join-Path $jarRuntime 'JediAcademyRemixedMP'),
    '--wire-probe', (Join-Path (Join-Path $jarBuild $Configuration) 'jar_mp_transport_tests.exe'),
    '--network-pure', '--network-city-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-sentinel-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-hoth-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-rift-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-shaft-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-korriban-section')
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'), '--game-data', $GameDataPath,
    '--build', $jarMPPrivate, '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
    '--network-pure', '--network-yavin-section')
if ($LocalMapPack) {
    foreach ($jarMap in @('gm_construct', 'gm_flatgrass')) {
        Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\test-mp-server.py'),
            '--game-data', $GameDataPath, '--build', $jarMPPrivate,
            '--wire-probe', (Join-Path $jarBuild "$Configuration\jar_mp_transport_tests.exe"),
            '--network-pure', '--map', $jarMap)
    }
}

New-Item -ItemType Directory -Path (Join-Path $jarPrivate 'tools') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $jarRoot 'scripts\collect-jar-diagnostics.ps1') -Destination (Join-Path $jarPrivate 'tools')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-renderer-sources.md') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'docs\JAR-renderer-notices.txt') -Destination (Join-Path $jarPrivate 'docs')
Copy-Item -LiteralPath (Join-Path $jarRoot 'lib\SDL2\include\SDL_copying.h') -Destination (Join-Path $jarPrivate 'SDL2-license.txt')

# Preserve the exact optimized binaries and matching PDBs before deployment.
# The unique directory is never reused, so subsequent builds cannot overwrite
# symbols needed to investigate an older crash.
New-Item -ItemType Directory -Path $jarSymbols -ErrorAction Stop | Out-Null
foreach ($module in @('jar_sp.x86_64','jar_gamex86_64','rdjar-vanilla_x86_64','rdjar-rend2_x86_64')) {
    $directory = if ($module -eq 'jar_sp.x86_64') { Join-Path $jarBuild "engine\$Configuration" } else { Join-Path $jarBuild $Configuration }
    $extension = if ($module -eq 'jar_sp.x86_64') { '.exe' } else { '.dll' }
    foreach ($suffix in @($extension, '.pdb')) {
        $file = Join-Path $directory ($module + $suffix)
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Required diagnostic artifact missing: $file" }
        Copy-Item -LiteralPath $file -Destination $jarSymbols
    }
}
foreach ($module in @('jar_mp.x86_64','jar_server.x86_64','jampgamex86_64','cgamex86_64','uix86_64','rdjar-mp-vanilla_x86_64','rdjar-mp-rend2_x86_64')) {
    $extension = if ($module -like 'jar_*.x86_64') { '.exe' } else { '.dll' }
    foreach ($suffix in @($extension,'.pdb')) {
        Copy-Item -LiteralPath (Join-Path $jarBuild "$Configuration\$module$suffix") -Destination $jarSymbols
    }
}
$jarLauncherSymbols = Join-Path $jarSymbols 'launchers'
New-Item -ItemType Directory -Path $jarLauncherSymbols -Force | Out-Null
foreach ($launcher in @(@('jar_sp.x86_64', "$Configuration"), @('jar_mp.x86_64', "mp-launcher\$Configuration"))) {
    foreach ($suffix in @('.exe','.pdb')) {
        Copy-Item -LiteralPath (Join-Path $jarBuild ($launcher[1] + '\' + $launcher[0] + $suffix)) -Destination $jarLauncherSymbols
    }
}
$jarHead = & git -C $jarRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot capture source revision' }
$jarSources = & git -C $jarRoot ls-files --cached --others --exclude-standard -- code codemp shared scripts tests tools cmake CMakeLists.txt README-JAR.md docs assets lib/box3d
if ($LASTEXITCODE -ne 0) { throw 'Cannot enumerate source provenance' }
$jarSourceHashes = foreach ($relative in ($jarSources | Sort-Object -Unique)) {
    $source = Join-Path $jarRoot $relative
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        $dest = Join-Path (Join-Path $jarSymbols 'source') $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $dest
        [pscustomobject]@{File=$relative;SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash}
    }
}
$jarSourceHashes | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $jarSymbols 'source-sha256.json') -Encoding UTF8
[pscustomobject]@{Schema=1;Version=$jarReleaseVersion;BuildId=$jarBuildId;Configuration=$Configuration;
    BuiltUtc=[DateTime]::UtcNow.ToString('o');GitHead="$jarHead";SourceState='working tree snapshot included in symbol archive';
    SymbolArchive=($jarBuildId+'.zip');RefApi=$jarRefApi;GameApi=$jarGameApi} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $jarPrivate 'build-info.json') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $jarPrivate 'build-info.json') -Destination $jarSymbols

$jarHashFile = Join-Path $jarRuntime 'JAR-SHA256SUMS.txt'
$jarHashes = Get-ChildItem -LiteralPath $jarRuntime -Recurse -File |
    Where-Object { $_.FullName -ne $jarHashFile } | Sort-Object FullName | ForEach-Object {
        $jarRelative = $_.FullName.Substring($jarRuntime.Length + 1).Replace('\', '/')
        '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $jarRelative
    }
Set-Content -LiteralPath $jarHashFile -Value $jarHashes -Encoding ascii
Copy-Item -LiteralPath $jarHashFile -Destination $jarSymbols
Compress-Archive -LiteralPath $jarSymbols -DestinationPath ($jarSymbols+'.zip')
Write-Host "JA:R symbols and source: $jarSymbols.zip"
$jarPendingZip = Join-Path $jarRoot ("build\JediAcademyRemixed-$jarReleaseVersion-$jarBuildId-staging.zip")
Compress-Archive -LiteralPath $jarRuntime -DestinationPath $jarPendingZip
Invoke-JarTool $PythonExecutable @((Join-Path $jarRoot 'scripts\verify-jar-release.py'), $jarPendingZip, $jarReleaseVersion, $jarBuildId)
Move-Item -LiteralPath $jarPendingZip -Destination $jarZip -Force
Write-Host "JA:R runtime: $jarRuntime"
Write-Host "JA:R archive: $jarZip"
if ($Install) {
    & (Join-Path $PSScriptRoot 'deploy-jar-release.ps1') -ZipPath $jarZip -GameDataPath $GameDataPath
} else {
    Write-Host 'JA:R staged locally. Installation requires -Install or an explicit deployment.'
}
Remove-JarObsoleteBuilds -Root (Join-Path $jarRoot 'build') -LatestVersion $jarReleaseVersion `
    -LatestBuildId $jarBuildId -GameDataPath $GameDataPath -BuildStarted $jarBuildStarted -Configuration $Configuration
} finally {
    $jarBuildLock.Dispose()
}
