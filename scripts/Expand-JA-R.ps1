# Extract the distribution ZIP before running this script. Only the missing MP
# archive is created. The underlying game, archives and settings are untouched.
[CmdletBinding()]
param([string]$Payload = (Join-Path $PSScriptRoot 'JediAcademy'),
      [string]$Recipe = (Join-Path $PSScriptRoot 'JAR-interface.json'))
$ErrorActionPreference = 'Stop'
$outFile = Join-Path $Payload 'JediAcademyRemixedMP/jar-interface.pk3'
$baseFile = Join-Path $Payload 'JediAcademyRemixed/jar-appearance.pk3'
$matchesFound = @(Get-Content -LiteralPath (Join-Path $Payload 'JAR-SHA256SUMS.txt') |
    Where-Object { $_ -match '^([0-9a-fA-F]{64})  JediAcademyRemixedMP/jar-interface\.pk3$' })
if ($matchesFound.Count -ne 1) { throw 'Missing or ambiguous interface checksum.' }
$expected = $matchesFound[0].Substring(0,64)
if (Test-Path -LiteralPath $outFile) {
    if ((Get-FileHash -LiteralPath $outFile).Hash -ne $expected) { throw 'Existing interface differs. Extract to a new empty folder.' }
    Write-Host 'The multiplayer interface is already verified.'
    return
}
if ((Get-Item -LiteralPath $Recipe).Length -gt 16MB) { throw 'Oversized interface recipe.' }
$plan = Get-Content -LiteralPath $Recipe -Raw | ConvertFrom-Json
if ($plan.schema -ne 1 -or $plan.sha256 -ne $expected -or $plan.size -le 0 -or $plan.size -ge 2GB) { throw 'Invalid interface recipe.' }
$tmp = "$outFile.$([guid]::NewGuid().ToString('N')).tmp"
$buffer = New-Object byte[] 1048576
try {
    $source = [IO.File]::OpenRead($baseFile)
    try {
        $output = [IO.File]::Open($tmp,'CreateNew','Write','None')
        try {
            foreach ($op in $plan.segments) {
                if ($op -is [string]) {
                    $bytes = [Convert]::FromBase64String($op)
                    if ($output.Position + $bytes.Length -gt $plan.size) { throw 'Invalid literal length.' }
                    $output.Write($bytes,0,$bytes.Length)
                } else {
                    if ($op.Count -ne 2) { throw 'Invalid source range.' }
                    [long]$start = $op[0]; [long]$left = $op[1]
                    if ($start -lt 0 -or $left -lt 0 -or $start -gt $source.Length - $left -or
                        $left -gt $plan.size - $output.Position) { throw 'Source range outside archive.' }
                    $source.Position = $start
                    while ($left -gt 0) {
                        $n = $source.Read($buffer,0,[int][Math]::Min($left,[long]$buffer.Length))
                        if (!$n) { throw 'Unexpected end of source.' }
                        $output.Write($buffer,0,$n); $left -= $n
                    }
                }
            }
            if ($output.Position -ne $plan.size) { throw 'Incomplete interface.' }
        } finally { $output.Dispose() }
    } finally { $source.Dispose() }
    if ((Get-FileHash -LiteralPath $tmp).Hash -ne $expected) { throw 'Interface checksum mismatch.' }
    [IO.File]::Move($tmp,$outFile)
    Write-Host 'Multiplayer interface restored and verified.'
} finally {
    if (Test-Path -LiteralPath $tmp) { Remove-Item -LiteralPath $tmp -Force }
}
