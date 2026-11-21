param([string]$Launcher, [string]$Child, [string]$BuildRoot, [switch]$Multiplayer)
$ErrorActionPreference = 'Stop'
$fixture = Join-Path $BuildRoot ('launcher test ' + [guid]::NewGuid().ToString('N'))
$folder = if ($Multiplayer) { 'JediAcademyRemixedMP' } else { 'JediAcademyRemixed' }
$executable = if ($Multiplayer) { 'jar_mp.x86_64.exe' } else { 'jar_sp.x86_64.exe' }
$private = Join-Path $fixture $folder
New-Item -ItemType Directory -Path $private -Force | Out-Null
Copy-Item -LiteralPath $Launcher -Destination (Join-Path $fixture $executable)
Copy-Item -LiteralPath $Child -Destination (Join-Path $private $executable)
# Incompatible files beside the launcher must not be loaded by it.
foreach ($name in @('SDL2.dll', 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    Set-Content -LiteralPath (Join-Path $fixture $name) -Value 'not a PE image'
}
$previous = $env:JAR_LAUNCHER_TEST_OUTPUT
try {
    $env:JAR_LAUNCHER_TEST_OUTPUT = Join-Path $fixture 'arguments.txt'
    $process = Start-Process -FilePath (Join-Path $fixture $executable) -WorkingDirectory $BuildRoot -ArgumentList '+set test_option "value with spaces" +set fs_basepath "custom data"' -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(10000)) { $process.Kill(); throw 'Launcher test timed out' }
    if ($process.ExitCode -ne 23) { throw "Wrong child exit code: $($process.ExitCode)" }
    $actual = @(Get-Content -LiteralPath $env:JAR_LAUNCHER_TEST_OUTPUT -Encoding UTF8)
    $expected = @($fixture, (Join-Path $private $executable), '+set', 'fs_basepath', $fixture)
    if ($Multiplayer) { $expected += @('+set', 'fs_game', 'JediAcademyRemixedMP') }
    $expected += @('+set', 'test_option', 'value with spaces', '+set', 'fs_basepath', 'custom data')
    if ($actual.Count -ne $expected.Count) { throw 'Wrong argument count' }
    for ($i = 0; $i -lt $expected.Count; $i++) {
        if ($actual[$i] -cne $expected[$i]) { throw "Argument $i mismatch: $($actual[$i])" }
    }
    Write-Output "PASS: $folder launcher, quoted arguments and explicit override, GameData working directory, isolated DLLs and exit code"
} finally { $env:JAR_LAUNCHER_TEST_OUTPUT = $previous }
