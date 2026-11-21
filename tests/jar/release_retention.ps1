# Exercise the production pruner with two independent disposable installs.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../../scripts/jar-build-retention.ps1')
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('jar-retention-' + [guid]::NewGuid().ToString('N'))
$root = Join-Path $fixture 'build'
function Write-Fixture([string]$Path,[string]$Text) {
    [IO.Directory]::CreateDirectory((Split-Path -Parent $Path)) | Out-Null
    [IO.File]::WriteAllText($Path,$Text)
}
try {
    foreach ($dir in 'symbols','jar/Release') { [IO.Directory]::CreateDirectory((Join-Path $root $dir)) | Out-Null }
    foreach ($target in 'A','B') {
        $game = Join-Path $fixture $target
        Write-Fixture "$game/module" "prior $target"
        $before = (Get-FileHash -LiteralPath "$game/module").Hash
        foreach ($n in 1,2,3) {
            $id = '20261101-00000'+$n+'-0000000'+$target.ToLowerInvariant()
            $receipt = "$root/deploy/$id"
            Write-Fixture "$receipt/receipt.json" (@{GameDataPath=$game;ChangedFiles=[int]($n -lt 3)} | ConvertTo-Json)
            if ($n -eq 2) { Write-Fixture "$receipt/backup/module" "prior $target"; $rollback="$receipt/backup/module" }
            if ($n -eq 1) { Write-Fixture "$receipt/backup/module" 'obsolete' }
            Write-Fixture "$receipt/payload/module" 'new'
        }
        Write-Fixture "$game/module" "new $target"
        Set-Variable -Name "rollback$target" -Value $rollback
        Set-Variable -Name "hash$target" -Value $before
    }
    $run = "$root/mp-prop-visibility-1.336.0/candidate-vanilla-1794850510"
    Write-Fixture "$run/game/base/assets0.pk3" 'fixture'
    Write-Fixture "$run/report.json" 'evidence'
    Write-Fixture "$root/keep-friend.zip" 'shareable'
    foreach ($target in 'A','B') {
        Remove-JarObsoleteBuilds $root '1.341.0' '20261120-000000-aaaaaaaa' (Join-Path $fixture $target) ([datetime]::UtcNow) 'Release'
    }
    foreach ($target in 'A','B') {
        $game = Join-Path $fixture $target
        Copy-Item -LiteralPath (Get-Variable "rollback$target" -ValueOnly) -Destination "$game/module" -Force
        if ((Get-FileHash -LiteralPath "$game/module").Hash -ne (Get-Variable "hash$target" -ValueOnly)) { throw 'Rollback mismatch' }
    }
    if ((Test-Path -LiteralPath "$run/game") -or !(Test-Path -LiteralPath "$run/report.json") -or
        !(Test-Path -LiteralPath "$root/keep-friend.zip")) { throw 'Unexpected retention selection' }
    Write-Host 'No-op reinstall, rollback hashes for two roots, evidence retention and runtime cleanup passed.'
} finally {
    $actual = (Resolve-Path -LiteralPath $fixture).ProviderPath
    if ((Split-Path -Parent $actual) -ne [IO.Path]::GetTempPath().TrimEnd('\','/') -or
        (Split-Path -Leaf $actual) -notmatch '^jar-retention-[a-f0-9]{32}$') { throw 'Unsafe fixture cleanup' }
    Remove-Item -LiteralPath $actual -Recurse -Force
}
