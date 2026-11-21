# Retain the latest staged release and the source/symbol archive needed by the
# installed build. Shareable packages and deployment backups are separate.
function Remove-JarObsoleteBuilds {
    [CmdletBinding(SupportsShouldProcess)]
    param([string]$Root, [version]$LatestVersion, [string]$LatestBuildId,
          [string]$GameDataPath, [datetime]$BuildStarted, [string]$Configuration)

    $rootItem = Get-Item -LiteralPath $Root -Force
    if ($rootItem.Name -ne 'build' -or $rootItem.LinkType -or !$rootItem.PSIsContainer) {
        throw "Unsafe JA:R build root: $Root"
    }
    $rootPath = $rootItem.FullName
    $installedInfo = Join-Path $GameDataPath 'JediAcademyRemixed/build-info.json'
    $installedId = if (Test-Path -LiteralPath $installedInfo -PathType Leaf) {
        (Get-Content -LiteralPath $installedInfo -Raw | ConvertFrom-Json).BuildId
    }
    $deployed = @(Get-ChildItem -LiteralPath (Join-Path $rootPath 'deploy') -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d{8}-\d{6}-[a-f0-9]{8}$' } |
        Sort-Object Name -Descending | Where-Object {
            $receiptPath = Join-Path $_.FullName 'receipt.json'
            if (Test-Path -LiteralPath $receiptPath -PathType Leaf) {
                (Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json).GameDataPath -eq $GameDataPath
            }
        })
    # A verified no-op has a receipt, but no files to restore.
    $rollbackRun = $deployed | Where-Object {
        $receipt = Get-Content -Raw -LiteralPath (Join-Path $_.FullName 'receipt.json') | ConvertFrom-Json
        $receipt.ChangedFiles -gt 0 -and (Test-Path -LiteralPath (Join-Path $_.FullName 'backup') -PathType Container)
    } | Sort-Object {
        $receipt = Get-Content -Raw -LiteralPath (Join-Path $_.FullName 'receipt.json') | ConvertFrom-Json
        if ($receipt.InstalledAt) { [DateTimeOffset]::Parse($receipt.InstalledAt).UtcDateTime }
        else { (Get-Item -LiteralPath (Join-Path $_.FullName 'receipt.json')).LastWriteTimeUtc }
    } -Descending | Select-Object -First 1
    $targets = @(
        Get-ChildItem -LiteralPath $rootPath -File | Where-Object {
            $_.Name -match '^JediAcademyRemixed-(\d+\.\d+\.\d+)-win64\.zip$' -and [version]$Matches[1] -lt $LatestVersion
        }
        Get-ChildItem -LiteralPath $rootPath -File | Where-Object {
            $_.Name -match '^JediAcademyRemixed-\d+\.\d+\.\d+-(\d{8}-\d{6}-[a-f0-9]{8})-staging\.zip$' -and
            $Matches[1] -lt $LatestBuildId
        }
        $deployed | Select-Object -Skip 1 | ForEach-Object {
            Get-ChildItem -LiteralPath $_.FullName -Directory |
                Where-Object { $_.Name -eq 'payload' -or ($_.Name -eq 'backup' -and (!$rollbackRun -or $_.Parent.FullName -ne $rollbackRun.FullName)) }
        }
        Get-ChildItem -LiteralPath (Join-Path $rootPath 'symbols') -Force | Where-Object {
            $_.Name -match '^(\d{8}-\d{6}-[a-f0-9]{8})(?:\.zip)?$' -and
            $Matches[1] -lt $LatestBuildId -and $Matches[1] -ne $installedId
        }
        Get-ChildItem -LiteralPath $rootPath -Directory | Where-Object {
            $_.CreationTimeUtc -lt $BuildStarted -and $_.Name -match '^(mp-smoke|mp-droids|yt1300-native)-\d{8}-\d{6}(?:-[a-f0-9]{6})?$'
        }
        Get-ChildItem -LiteralPath $rootPath -Directory | Where-Object {
            $_.Name -match '^[\w-]+-(\d+\.\d+\.\d+)$' -and [version]$Matches[1] -lt $LatestVersion
        } | ForEach-Object {
            # Keep the logs, results and source used to qualify historic builds.
            Get-ChildItem -LiteralPath $_.FullName -Force | Where-Object {
                ($_.PSIsContainer -and $_.Name -in 'friend-extracted', 'friend-fixture', 'friend-smoke') -or
                (!$_.PSIsContainer -and $_.Name -match '^(?:first|second)-build-\d{8}-\d{6}-[a-f0-9]{8}\.zip$')
            }
        }
        # Opt in only generated runtime layouts. Keep the run logs, stacks,
        # images and receipts alongside them. Never recurse through links.
        foreach ($suite in (Get-ChildItem -LiteralPath $rootPath -Directory | Where-Object {
            !$_.LinkType -and $_.Name -match '^mp-(?:grenade|prop)-visibility-(\d+\.\d+\.\d+)$' -and
            [version]$Matches[1] -lt $LatestVersion
        })) {
            $dir = $suite.FullName
            $raw = Join-Path $dir 'raw-runtime'
            if (Test-Path -LiteralPath (Join-Path $raw 'jar_server.x86_64.exe')) { Get-Item -LiteralPath $raw }
            Get-ChildItem -Directory -LiteralPath $dir | Where-Object Name -Match '^(baseline|candidate|packaged)(-(vanilla|rend2))?-[0-9]{10}$' | ForEach-Object {
                $game = Join-Path $_.FullName 'game'
                if (Test-Path -LiteralPath (Join-Path $game 'base/assets0.pk3')) { Get-Item -LiteralPath $game }
            }
        }
        # New smoke runners use an explicit marker on the disposable runtime only.
        Get-ChildItem -Directory -LiteralPath $rootPath | Where-Object { $_.Name -match '^[\w-]+-\d+\.\d+\.\d+$' } | ForEach-Object {
            Get-ChildItem -Directory -LiteralPath $_.FullName | Where-Object { !$_.LinkType } | ForEach-Object {
                $runtime = Join-Path $_.FullName 'game'
                if ($_.CreationTimeUtc -lt $BuildStarted -and (Test-Path -LiteralPath (Join-Path $runtime '.jar-test-runtime'))) { Get-Item -LiteralPath $runtime }
            }
        }
        Get-ChildItem -LiteralPath (Join-Path $rootPath "jar/$Configuration") -Directory | Where-Object {
            $_.CreationTimeUtc -lt $BuildStarted -and $_.Name -match '^retail-(resources|bsp-(admission|fogs|lighting))-\d{8}-\d{6}-\d{6}$'
        }
        Get-ChildItem -LiteralPath (Join-Path $rootPath 'jar') -Directory | Where-Object {
            $_.Name -match '^(diagnostics-test-|launcher test |install path test )[a-f0-9]{32}$'
        }
        Get-ChildItem -LiteralPath (Join-Path $rootPath 'appearance-cache') -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^[a-f0-9]{64}$' -and
                (Test-Path -LiteralPath (Join-Path $_.FullName 'jar-appearance.pk3') -PathType Leaf) } |
            Sort-Object { (Get-Item -LiteralPath (Join-Path $_.FullName 'jar-appearance.pk3')).LastWriteTimeUtc } -Descending |
            Select-Object -Skip 1
    )
    $prefix = $rootPath.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    $removed = 0
    foreach ($target in $targets) {
        try {
            $resolved = (Resolve-Path -LiteralPath $target.FullName).ProviderPath
            if (!$resolved.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Path is outside $rootPath"
            }
            for ($part = $resolved; $part -ne $rootPath; $part = Split-Path -Parent $part) {
                if ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked path: $part" }
            }
            if ($target.PSIsContainer -and (Get-ChildItem -LiteralPath $resolved -Recurse -Force |
                Where-Object LinkType -in 'Junction','SymbolicLink','MountPoint' | Select-Object -First 1)) {
                throw 'Directory contains a linked entry.'
            }
            if (!$PSCmdlet.ShouldProcess($resolved, "Prune generated JA:R files")) { continue }
            Remove-Item -LiteralPath $resolved -Force -Recurse:$target.PSIsContainer -ErrorAction Stop
            ++$removed
        } catch {
            Write-Warning "Could not prune $($target.FullName): $($_.Exception.Message)"
        }
    }
    Write-Host "JA:R retention pruned $removed generated paths."
}
