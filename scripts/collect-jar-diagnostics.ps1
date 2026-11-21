[CmdletBinding()]
param(
    [string]$ProfilePath = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'My Games\JediAcademyRemixed'),
    [string]$GameDataPath = 'C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData',
    [string]$OutputDirectory,
    [switch]$NoDump
)
$ErrorActionPreference='Stop'
if(-not $OutputDirectory){$OutputDirectory=Join-Path $ProfilePath 'diagnostic-bundles'}
$stamp=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8)
$bundle=Join-Path $OutputDirectory ('JAR-diagnostics-'+$stamp)
New-Item -ItemType Directory -Path $bundle -Force | Out-Null
$records=[Collections.Generic.List[object]]::new()
function Copy-Evidence([string]$Path,[string]$Name) {
    if(-not (Test-Path -LiteralPath $Path -PathType Leaf)){return}
    $dest=Join-Path $bundle $Name
    Copy-Item -LiteralPath $Path -Destination $dest
    $item=Get-Item -LiteralPath $Path
    $records.Add([pscustomobject]@{Source=$Path;File=$Name;SourceModifiedUtc=$item.LastWriteTimeUtc.ToString('o');Bytes=(Get-Item -LiteralPath $dest).Length;SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash})
}
$logs=Join-Path $ProfilePath 'diagnostics'
if(Test-Path -LiteralPath $logs){
    foreach($file in Get-ChildItem -LiteralPath $logs -File){
        if($file.Name -match '^jar-diagnostics-[0-7]\.jsonl$|^jar-native-exceptions(\.previous)?\.log$'){
            Copy-Evidence $file.FullName $file.Name
        }
    }
}
Copy-Evidence (Join-Path $GameDataPath 'JediAcademyRemixed\build-info.json') 'build-info.json'
Copy-Evidence (Join-Path $GameDataPath 'JAR-SHA256SUMS.txt') 'installed-manifest.txt'
if(-not $NoDump){
    $dump=Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'CrashDumps') -Filter 'jar_sp.x86_64.exe.*.dmp' -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if($dump){Copy-Evidence $dump.FullName $dump.Name}
}
$binaries=@('JediAcademyRemixed\jar_sp.x86_64.exe','JediAcademyRemixed\rdjar-vanilla_x86_64.dll','JediAcademyRemixed\rdjar-rend2_x86_64.dll','OpenJK\jar_gamex86_64.dll')
$hashes=@(foreach($relative in $binaries){
    $file=Join-Path $GameDataPath $relative
    if(Test-Path -LiteralPath $file -PathType Leaf){[pscustomobject]@{File=$relative;SHA256=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash}}
})
[pscustomobject]@{Schema=1;CollectedUtc=[DateTime]::UtcNow.ToString('o');Profile=$ProfilePath;GameData=$GameDataPath;Files=$records;InstalledBinaries=$hashes;
    Note='Dump is the newest available JA:R WER dump; compare its timestamp and module build with the session. It may predate these logs. No settings, saves, or game assets are included.'} |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $bundle 'collection.json') -Encoding UTF8
$zip=$bundle+'.zip'
Compress-Archive -LiteralPath $bundle -DestinationPath $zip
Write-Host "Local diagnostics bundle: $zip"
Write-Host "SHA256: $((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash)"
