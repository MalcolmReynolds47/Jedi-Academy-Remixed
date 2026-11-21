[CmdletBinding()]
param(
    [string]$ZipPath,
    [string]$GameDataPath = 'C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression.FileSystem
$jarRoot = Split-Path -Parent $PSScriptRoot
if (-not $ZipPath) {
    $jarHeader = Get-Content -LiteralPath (Join-Path $jarRoot 'shared\qcommon\jar_release.h') -Raw
    $jarParts = foreach ($jarPart in @('MAJOR','MINOR','PATCH')) {
        $jarMatch = [regex]::Match($jarHeader, "(?m)^#define JAR_VERSION_$jarPart ([0-9]+)\r?$")
        if (-not $jarMatch.Success -or [int64]$jarMatch.Groups[1].Value -gt 65535) { throw "Invalid JA:R release part: $jarPart" }
        [int]$jarMatch.Groups[1].Value
    }
    $ZipPath = Join-Path $jarRoot ("build\JediAcademyRemixed-" + ($jarParts -join '.') + '-win64.zip')
}
$jarZip = (Resolve-Path -LiteralPath $ZipPath).Path
$jarGame = (Resolve-Path -LiteralPath $GameDataPath).Path.TrimEnd('\', '/')
foreach ($asset in @('base\assets0.pk3', 'base\assets1.pk3')) {
    if (-not (Test-Path -LiteralPath (Join-Path $jarGame $asset) -PathType Leaf)) {
        throw "Jedi Academy game data is missing: $asset under $jarGame"
    }
}

# Sharing this directory with other mods is intentional. Do not replace their
# executables, archives or settings, even if a malformed package contains them.
$jarAllowed = @(
    'jar_sp.x86_64.exe', 'jar_mp.x86_64.exe', 'OpenJK/jar_gamex86_64.dll', 'README-JAR.md', 'JAR-SHA256SUMS.txt',
    'JediAcademyRemixed/jar_sp.x86_64.exe', 'JediAcademyRemixed/rdjar-vanilla_x86_64.dll',
    'JediAcademyRemixed/rdjar-rend2_x86_64.dll', 'JediAcademyRemixed/docs/JAR-renderer-sources.md',
    'JediAcademyRemixed/docs/JAR-renderer-notices.txt', 'JediAcademyRemixed/docs/JAR-overhaul-checkpoint.md',
    'JediAcademyRemixed/docs/JAR-melee-inventory-plan.md', 'JediAcademyRemixed/docs/JAR-inventory-sandbox.md',
    'JediAcademyRemixed/docs/JAR-randomizer.md', 'JediAcademyRemixed/docs/JAR-appearance.md', 'JediAcademyRemixed/jar-appearance.pk3',
    'JediAcademyRemixed/docs/JAR-diagnostics.md', 'JediAcademyRemixed/build-info.json',
    'JediAcademyRemixed/tools/collect-jar-diagnostics.ps1',
    'JediAcademyRemixed/jar-sandbox-icons.pk3', 'JediAcademyRemixed/docs/JAR-sandbox-replacement.md',
    'JediAcademyRemixed/docs/JAR-physics.md', 'JediAcademyRemixed/docs/JAR-spawn-catalog.md', 'JediAcademyRemixed/docs/JAR-spawn-catalog.json', 'JediAcademyRemixed/Box3D-LICENSE.txt', 'JediAcademyRemixed/Box3D-SOURCE.json',
    'JediAcademyRemixed/SDL2.dll',
    'JediAcademyRemixed/msvcp140.dll', 'JediAcademyRemixed/msvcp140_1.dll', 'JediAcademyRemixed/msvcp140_2.dll',
    'JediAcademyRemixed/msvcp140_atomic_wait.dll', 'JediAcademyRemixed/msvcp140_codecvt_ids.dll',
    'JediAcademyRemixed/vcruntime140.dll', 'JediAcademyRemixed/vcruntime140_1.dll', 'JediAcademyRemixed/concrt140.dll',
    'JediAcademyRemixed/LICENSE.txt', 'JediAcademyRemixed/SDL2-license.txt', 'JediAcademyRemixed/docs/JAR-validation.md'
)
$jarAllowed += @('JediAcademyRemixed/docs/JAR-catalog-exclusions.json')
$jarLegacyPacks = @('JediAcademyRemixed/jar-legacy-npcs.pk3', 'JediAcademyRemixedMP/jar-legacy-npcs.pk3')
$jarAllowed += $jarLegacyPacks
$jarAllowed += @('JediAcademyRemixed/jar-yt1300.pk3','JediAcademyRemixedMP/jar-yt1300.pk3')
$jarMPDroidPack = 'JediAcademyRemixedMP/jar-mp-legacy-droids.pk3'
$jarAllowed += $jarMPDroidPack
$jarMPRend2Files = @('Play-JA-R-Multiplayer-Rend2.cmd','JediAcademyRemixedMP/rdjar-mp-rend2_x86_64.dll')
$jarAllowed += $jarMPRend2Files
$jarAllowed += @('Play-JA-R-Multiplayer.cmd','Host-JA-R-Server.cmd','JediAcademyRemixed/docs/JAR-owner-acceptance-2026-10-10.md')
foreach ($name in @('jar_mp.x86_64.exe','jar_server.x86_64.exe','jampgamex86_64.dll','cgamex86_64.dll','uix86_64.dll','rdjar-mp-vanilla_x86_64.dll','SDL2.dll','jar-sandbox-icons.pk3','jar-mp-controls.cfg','jar-server-example.cfg','JAR-multiplayer.md','Box3D-LICENSE.txt','Box3D-SOURCE.json',
 'msvcp140.dll','msvcp140_1.dll','msvcp140_2.dll','msvcp140_atomic_wait.dll','msvcp140_codecvt_ids.dll','vcruntime140.dll','vcruntime140_1.dll','concrt140.dll')) {
    $jarAllowed += 'JediAcademyRemixedMP/' + $name
}
$jarRequired = @('JediAcademyRemixed/jar-appearance.pk3', 'jar_sp.x86_64.exe', 'OpenJK/jar_gamex86_64.dll', 'JAR-SHA256SUMS.txt',
    'JediAcademyRemixed/rdjar-rend2_x86_64.dll',
    'JediAcademyRemixed/jar_sp.x86_64.exe', 'JediAcademyRemixed/rdjar-vanilla_x86_64.dll',
    'JediAcademyRemixed/jar-sandbox-icons.pk3', 'JediAcademyRemixed/docs/JAR-sandbox-replacement.md',
    'JediAcademyRemixed/docs/JAR-physics.md', 'JediAcademyRemixed/docs/JAR-spawn-catalog.md', 'JediAcademyRemixed/docs/JAR-spawn-catalog.json', 'JediAcademyRemixed/Box3D-LICENSE.txt', 'JediAcademyRemixed/Box3D-SOURCE.json',
    'JediAcademyRemixed/SDL2.dll', 'JediAcademyRemixed/msvcp140.dll',
    'JediAcademyRemixed/vcruntime140.dll', 'JediAcademyRemixed/vcruntime140_1.dll')

$jarAllowed += @('JediAcademyRemixedMP/jar-interface.pk3','JediAcademyRemixedMP/jar-effects.pk3')
$jarAllowed += 'JediAcademyRemixedMP/jar-mp-player-models.pk3'
$jarLocalMapFiles = @('JediAcademyRemixed/jar-gmod-maps.pk3','JediAcademyRemixedMP/jar-gmod-maps.pk3')
$jarAllowed += $jarLocalMapFiles
$jarRequired += @('JediAcademyRemixedMP/jar-effects.pk3','JediAcademyRemixedMP/jar-interface.pk3','JediAcademyRemixedMP/jar-sandbox-icons.pk3','jar_mp.x86_64.exe','JediAcademyRemixedMP/Box3D-LICENSE.txt','JediAcademyRemixedMP/Box3D-SOURCE.json','JediAcademyRemixedMP/jar_mp.x86_64.exe','JediAcademyRemixedMP/jar_server.x86_64.exe','JediAcademyRemixedMP/jampgamex86_64.dll','JediAcademyRemixedMP/cgamex86_64.dll','JediAcademyRemixedMP/uix86_64.dll','JediAcademyRemixedMP/rdjar-mp-vanilla_x86_64.dll')

function Get-JarChildPath([string]$Root, [string]$Relative) {
    $full = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + '\'
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the deployment root: $full"
    }
    # Resolve directory junctions/symlinks conservatively: deployment must stay
    # in the selected installation, not follow a link to some other directory.
    $parent = Split-Path -Parent $full
    while ($parent -and $parent.Length -ge $Root.Length) {
        if (Test-Path -LiteralPath $parent) {
            $item = Get-Item -LiteralPath $parent -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Deployment directory is a reparse point: $parent"
            }
        }
        $parent = Split-Path -Parent $parent
    }
    if (Test-Path -LiteralPath $full) {
        $item = Get-Item -LiteralPath $full -Force
        if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Deployment file is a directory or reparse point: $full"
        }
    }
    return $full
}

function Assert-JarGameStopped {
    foreach ($process in Get-Process) {
        $path = $null
        try { $path = $process.Path } catch { continue }
        if ($path -and $path.StartsWith($jarGame + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Close $($process.ProcessName) (PID $($process.Id)) before installing JA:R. No process was stopped."
        }
    }
}

Assert-JarGameStopped
$jarRun = Join-Path $jarRoot ('build\deploy\' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
$jarPayload = Join-Path $jarRun 'payload'
$jarBackup = Join-Path $jarRun 'backup'
New-Item -ItemType Directory -Path $jarPayload -Force | Out-Null
$jarEntries = @{}
$zip = [IO.Compression.ZipFile]::OpenRead($jarZip)
try {
    foreach ($entry in $zip.Entries) {
        $name = $entry.FullName.Replace('\', '/')
        if ($name.EndsWith('/')) { continue }
        if (-not $name.StartsWith('JediAcademy/', [StringComparison]::Ordinal)) {
            throw "Unexpected release ZIP root: $name"
        }
        $relative = $name.Substring('JediAcademy/'.Length)
        if ($jarAllowed -notcontains $relative -or $jarEntries.ContainsKey($relative)) {
            throw "Unexpected or duplicate JA:R release file: $relative"
        }
        $jarEntries[$relative] = $entry
        [void](Get-JarChildPath $jarGame $relative)
    }
    foreach ($required in $jarRequired) {
        if (-not $jarEntries.ContainsKey($required)) { throw "Missing release file: $required" }
    }
    if ($jarEntries.ContainsKey($jarLocalMapFiles[0]) -ne $jarEntries.ContainsKey($jarLocalMapFiles[1])) {
        throw 'Local map pack must be present in both SP and MP.'
    }
    # Fully extract and validate the archive before changing any installed file.
    foreach ($relative in $jarEntries.Keys) {
        $destination = Get-JarChildPath $jarPayload $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($jarEntries[$relative], $destination)
    }
} finally { $zip.Dispose() }

$jarHashes = @{}
foreach ($line in Get-Content -LiteralPath (Join-Path $jarPayload 'JAR-SHA256SUMS.txt')) {
    if ($line -notmatch '^([a-fA-F0-9]{64})  (.+)$') { throw 'Invalid JAR-SHA256SUMS.txt entry' }
    $hash = $Matches[1]; $relative = $Matches[2]
    if ($relative -eq 'JAR-SHA256SUMS.txt' -or -not $jarEntries.ContainsKey($relative) -or $jarHashes.ContainsKey($relative)) {
        throw "Invalid or duplicate hash manifest file: $relative"
    }
    $jarHashes[$relative] = $hash
}
if ($jarHashes.Count -ne $jarEntries.Count - 1) { throw 'Release hash manifest is incomplete' }
foreach ($relative in $jarHashes.Keys) {
    $actual = (Get-FileHash -LiteralPath (Get-JarChildPath $jarPayload $relative) -Algorithm SHA256).Hash
    if ($actual -ne $jarHashes[$relative]) { throw "Release hash mismatch: $relative" }
}
$jarHashes['JAR-SHA256SUMS.txt'] = (Get-FileHash -LiteralPath (Join-Path $jarPayload 'JAR-SHA256SUMS.txt') -Algorithm SHA256).Hash

# Current catalogs require the same companion on both clients. Retain older
# package deployment for rollback; inspect only the already hash-verified info.
$jarBuildInfoPath = Join-Path $jarPayload 'JediAcademyRemixed/build-info.json'
if (Test-Path -LiteralPath $jarBuildInfoPath) {
    $jarPackageInfo = Get-Content -LiteralPath $jarBuildInfoPath -Raw | ConvertFrom-Json
    if ([version]$jarPackageInfo.Version -ge [version]'1.44.0') {
        foreach ($jarMPRend2File in $jarMPRend2Files) {
            if (-not $jarEntries.ContainsKey($jarMPRend2File)) { throw "Missing MP Rend2 release file: $jarMPRend2File" }
        }
    }
    if ([version]$jarPackageInfo.Version -ge [version]'0.98.0' -and -not $jarEntries.ContainsKey($jarMPDroidPack)) {
        throw "Missing MP droid adapter pack: $jarMPDroidPack"
    }
    if ([version]$jarPackageInfo.Version -ge [version]'0.95.0') {
        foreach ($jarLegacyPack in $jarLegacyPacks) {
            if (-not $jarEntries.ContainsKey($jarLegacyPack)) { throw "Missing legacy NPC asset pack: $jarLegacyPack" }
        }
        if ($jarHashes[$jarLegacyPacks[0]] -ne $jarHashes[$jarLegacyPacks[1]]) { throw 'SP and MP legacy NPC assets differ' }
    }
}

# Save everything that would change before the first replacement. Copies are
# verified again at the destination; rollback restores the prior bytes on error.
$jarChanges = @()
$jarRows = @()
foreach ($relative in ($jarHashes.Keys | Sort-Object)) {
    $destination = Get-JarChildPath $jarGame $relative
    $previous = $null
    if (Test-Path -LiteralPath $destination -PathType Leaf) {
        $previous = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    }
    if ($previous -ne $jarHashes[$relative]) {
        if ($previous) {
            $backup = Get-JarChildPath $jarBackup $relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
            Copy-Item -LiteralPath $destination -Destination $backup
            if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne $previous) {
                throw "Backup hash mismatch: $relative"
            }
        }
        $jarChanges += [pscustomobject]@{ File = $relative; Destination = $destination; PreviousSHA256 = $previous }
    }
    $jarRows += [pscustomobject]@{ File = $relative; SHA256 = $jarHashes[$relative]; PreviousSHA256 = $previous }
}
Assert-JarGameStopped
$jarAttempted = @()
try {
    foreach ($change in $jarChanges) {
        $jarAttempted += $change
        New-Item -ItemType Directory -Path (Split-Path -Parent $change.Destination) -Force | Out-Null
        Copy-Item -LiteralPath (Get-JarChildPath $jarPayload $change.File) -Destination $change.Destination -Force
    }
    foreach ($row in $jarRows) {
        if ((Get-FileHash -LiteralPath (Get-JarChildPath $jarGame $row.File) -Algorithm SHA256).Hash -ne $row.SHA256) {
            throw "Installed hash mismatch: $($row.File)"
        }
    }
} catch {
    $failure = $_
    $rollbackErrors = @()
    foreach ($change in $jarAttempted) {
        try {
            if ($change.PreviousSHA256) {
                Copy-Item -LiteralPath (Get-JarChildPath $jarBackup $change.File) -Destination $change.Destination -Force
            } elseif (Test-Path -LiteralPath $change.Destination -PathType Leaf) {
                Remove-Item -LiteralPath $change.Destination -Force
            }
        } catch { $rollbackErrors += $_.Exception.Message }
    }
    if ($rollbackErrors.Count) { throw "Installation failed: $failure. Rollback needs attention: $($rollbackErrors -join '; '). Backups: $jarBackup" }
    throw "Installation failed; attempted file changes were rolled back. $failure"
}

$receipt = [ordered]@{
    InstalledAt = (Get-Date).ToString('o')
    ZipPath = $jarZip
    ZipSHA256 = (Get-FileHash -LiteralPath $jarZip -Algorithm SHA256).Hash
    GameDataPath = $jarGame
    ChangedFiles = $jarChanges.Count
    VerifiedFiles = $jarRows.Count
    Files = $jarRows
}
$receiptPath = Join-Path $jarRun 'receipt.json'
$receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receiptPath -Encoding UTF8
Write-Host "JA:R installed to: $jarGame"
Write-Host "Verified $($jarRows.Count) files against the ZIP; updated $($jarChanges.Count) files."
Write-Host "Installation receipt and backups: $jarRun"
Write-Host "Launch: $(Join-Path $jarGame 'jar_sp.x86_64.exe')"
