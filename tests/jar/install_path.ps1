param([string]$Probe, [string]$BuildRoot)
$ErrorActionPreference = 'Stop'
$fixture = Join-Path $BuildRoot ('install path test ' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $fixture 'base') -Force | Out-Null
function Check-Path([string]$Directory, [string]$Expected) {
    $exe = Join-Path $Directory 'path-probe.exe'
    $process = Start-Process -FilePath $exe -WorkingDirectory $BuildRoot -ArgumentList @('"' + $Directory + '"', '"' + $Expected + '"') -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(10000)) { $process.Kill(); throw 'Path discovery probe timed out' }
    if ($process.ExitCode -ne 0) { throw "Path discovery failed: $Directory -> $Expected" }
}
foreach ($folder in @('JediAcademyRemixed','JediAcademyRemixedMP','unrelated-mod')) {
    $directory = Join-Path $fixture $folder
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Copy-Item -LiteralPath $Probe -Destination (Join-Path $directory 'path-probe.exe')
    Check-Path $directory $directory
}
Set-Content -LiteralPath (Join-Path $fixture 'base/assets0.pk3') -Value 'test marker'
Check-Path (Join-Path $fixture 'JediAcademyRemixed') (Join-Path $fixture 'JediAcademyRemixed')
Set-Content -LiteralPath (Join-Path $fixture 'base/assets1.pk3') -Value 'test marker'
foreach ($folder in @('JediAcademyRemixed','JediAcademyRemixedMP')) {
    $directory = Join-Path $fixture $folder
    Check-Path $directory $fixture
    New-Item -ItemType Directory -Path (Join-Path $directory 'base') -Force | Out-Null
    foreach ($asset in @('assets0.pk3','assets1.pk3')) {
        Set-Content -LiteralPath (Join-Path $directory ('base/' + $asset)) -Value 'test marker'
    }
    Check-Path $directory $directory
}
Check-Path (Join-Path $fixture 'unrelated-mod') (Join-Path $fixture 'unrelated-mod')
Write-Output 'PASS: SP/MP executable-relative startup, missing/partial/complete parent assets, standalone layouts and unrelated mods'
