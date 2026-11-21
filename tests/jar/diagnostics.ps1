param([string]$Probe,[string]$BuildRoot)
$ErrorActionPreference='Stop'
$testRoot=Join-Path $BuildRoot ('diagnostics-test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
foreach($mode in @('normal','rotation','abrupt')) {
    $dir=Join-Path $testRoot $mode
    New-Item -ItemType Directory -Path $dir | Out-Null
    & $Probe $mode $dir
    $expected=if($mode -eq 'abrupt'){77}else{0}
    if($LASTEXITCODE -ne $expected){throw "$mode probe exit $LASTEXITCODE"}
    $files=@(Get-ChildItem -LiteralPath $dir -Filter '*.jsonl')
    $records=@($files | ForEach-Object { Get-Content -LiteralPath $_.FullName | ForEach-Object { $_ | ConvertFrom-Json } })
    if(!$records.Count){throw 'No log records'}
    if(@($records | Where-Object {$_.schema -ne 1 -or !$_.session -or !$_.pid}).Count){throw 'Missing context'}
    if(@($records.seq | Sort-Object -Unique).Count -ne $records.Count){throw 'Duplicate sequence numbers'}
    if($mode -eq 'normal') {
        $escaped=$records | Where-Object event -eq 'escaping'
        if($escaped.detail -cne "quote`" slash\ newline`n`t$([char]1)"){throw 'Escaping failed'}
        if($escaped.frame -ne 42 -or $escaped.game_ms -ne 12345){throw 'Frame context lost'}
        if($records.event -contains 'hidden' -or $records.event -notcontains 'visible'){throw 'Level filter failed'}
        if(($records | Where-Object event -eq 'long').detail -notlike '*[[]truncated[]]'){throw 'Truncation unmarked'}
        if(@($records | Where-Object category -eq 'thread').Count -ne 400){throw 'Concurrent writes lost'}
        if($records.event -notcontains 'rate_limit' -or $records.event -notcontains 'error_after_flood'){throw 'Flood reporting failed'}
    }
    if($mode -eq 'rotation') {
        if($files.Count -ne 8 -or @($files | Where-Object Length -gt 8192).Count){throw 'Log retention exceeded'}
    }
    if($mode -eq 'abrupt') {
        if($records.event -notcontains 'before_abrupt_exit' -or $records.event -contains 'end'){throw 'Abrupt-exit evidence incorrect'}
        if((Get-Content -LiteralPath (Join-Path $dir 'jar-native-exceptions.log') -Raw) -notmatch 'code=0xc0000005'){throw 'Native exception breadcrumb missing'}
    }
}
& $Probe unwritable (Join-Path $testRoot 'does-not-exist\child')
if($LASTEXITCODE -ne 0){throw 'Log open failure was not handled'}
$leaseDir=Join-Path $testRoot 'lease'
New-Item -ItemType Directory -Path $leaseDir | Out-Null
$holder=Start-Process -FilePath $Probe -ArgumentList @('hold',('"'+$leaseDir+'"')) -PassThru -WindowStyle Hidden
for($attempt=0;$attempt -lt 30 -and -not (Test-Path -LiteralPath (Join-Path $leaseDir 'jar-diagnostics-0.jsonl'));$attempt++){Start-Sleep -Milliseconds 25}
& $Probe unwritable $leaseDir
if($LASTEXITCODE -ne 0){throw 'Concurrent logger did not respect the file lease'}
if(-not $holder.WaitForExit(5000)){throw 'Lease probe did not exit'}
Write-Host "PASS: JSON, rotation, filtering, concurrency, flood limits, abrupt exit, native exception chaining, missing directory. Evidence: $testRoot"
