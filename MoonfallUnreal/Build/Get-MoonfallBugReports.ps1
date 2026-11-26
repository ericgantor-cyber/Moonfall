# Local report index. Does not upload, delete, open arbitrary paths or read saves.
[CmdletBinding()]
param([switch]$IncludeInstalled, [string]$InstallDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path (Split-Path $PSScriptRoot -Parent) 'Saved\BugReports'
$roots = @([pscustomobject]@{ Path=$root; Source='Development checkout' })
if ($IncludeInstalled -and $env:LOCALAPPDATA) {
    $roots += [pscustomobject]@{ Path=(Join-Path $env:LOCALAPPDATA 'MoonfallUnreal\Saved\BugReports'); Source='Installed game (user data)' }
    $roots += [pscustomobject]@{ Path=(Join-Path $env:LOCALAPPDATA 'Programs\MoonfallUnreal\MoonfallUnreal\Saved\BugReports'); Source='Installed game (package)' }
}
# Reject junctions/symlinks in every ancestor, not merely the leaf directory.
function Test-PlainDirectory([string]$Directory) {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return $false }
    $item = Get-Item -LiteralPath $Directory -Force
    while ($null -ne $item) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { return $false }
        $item = $item.Parent
    }
    return $true
}
# Accept only local, absolute installation roots with the expected game exe.
# Registry data is a hint, never executed or used to scan arbitrary trees.
function Add-InstallReportRoot([string]$InstallPath) {
    if ([string]::IsNullOrWhiteSpace($InstallPath) -or $InstallPath -notmatch '^[A-Za-z]:[\\/]') { return }
    try {
        $full = [IO.Path]::GetFullPath($InstallPath)
        if (-not (Test-PlainDirectory $full)) { return }
        $game = Join-Path $full 'MoonfallUnreal.exe'
        if (-not (Test-Path -LiteralPath $game -PathType Leaf)) { return }
        if ((Get-Item -LiteralPath $game -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { return }
        [pscustomobject]@{ Path=(Join-Path $full 'MoonfallUnreal\Saved\BugReports'); Source='Custom installation' }
    } catch { Write-Warning 'Moonfall installation location could not be verified.' }
}
if ($IncludeInstalled -and -not $InstallDirectory) {
    try {
        $registration = Get-ItemProperty -LiteralPath 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal' -Name InstallLocation -ErrorAction Stop
        $InstallDirectory = $registration.InstallLocation
    } catch { }
}
if ($IncludeInstalled -and $InstallDirectory) {
    $roots += @(Add-InstallReportRoot $InstallDirectory)
}
function Read-ReportDirectory([string]$Directory, [string]$Kind, [string]$Pattern, [string]$Source) {
    if (-not (Test-PlainDirectory $Directory)) { return }
    $files = @(Get-ChildItem -LiteralPath $Directory -File -Filter '*.json' |
        Where-Object { $_.Name -match $Pattern -and $_.Length -gt 0 -and $_.Length -le 131072 -and
            -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 100)
    $seenSessions = @{}
    foreach ($file in $files) {
        $session = $file.Name.Substring(0,32)
        if ($Kind -eq 'moonfall_recovery_snapshot' -and $seenSessions.ContainsKey($session)) { continue }
        try {
            $report = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($report.schema -ne 1 -or $report.kind -ne $Kind) { continue }
            if ($Kind -eq 'moonfall_recovery_snapshot') { $seenSessions[$session] = $true }
            $fps = $null
            $p95 = $null
            $hitches = $null
            $classification = 'Insufficient timing evidence'
            if ($report.gameplay_sample_ready -is [bool] -and $report.gameplay_sample_ready -and
                $report.average_fps -is [ValueType] -and $report.p95_ms -is [ValueType]) {
                $f = [double]$report.average_fps
                $p = [double]$report.p95_ms
                if (-not [double]::IsNaN($f) -and -not [double]::IsInfinity($f) -and
                    -not [double]::IsNaN($p) -and -not [double]::IsInfinity($p) -and
                    $f -gt 0 -and $f -le 100000 -and $p -gt 0 -and $p -le 2000) {
                    $fps = [math]::Round($f,2)
                    $p95 = [math]::Round($p,2)
                    if ($report.hitches -is [ValueType] -and $report.hitches -ge 0 -and $report.hitches -le 240) {
                        $hitches = [int]$report.hitches
                    }
                    $classification = if ($f -lt 30 -or $p -gt 50) { 'Low throughput' }
                        elseif ($hitches -gt 0 -or $p -gt 33.334) { 'Intermittent slow frames' }
                        else { 'No clear timing issue in sample' }
                }
            }
            [pscustomobject]@{
                Source = $Source
                File = $file.Name
                LastWriteUtc = $file.LastWriteTimeUtc.ToString('o')
                Kind = if ($Kind -eq 'moonfall_recovery_snapshot') { 'Recovery snapshot; session status and crash not established' } else { 'Manual report' }
                Classification = $classification
                AverageFPS = $fps
                P95Milliseconds = $p95
                Hitches = $hitches
            }
        } catch { Write-Warning ('Invalid or unreadable report: ' + $file.Name) }
    }
}
$reports = @()
$seenRoots = @{}
foreach ($entry in $roots) {
    $normalized = [IO.Path]::GetFullPath($entry.Path).TrimEnd('\','/')
    if ($seenRoots.ContainsKey($normalized)) { continue }
    $seenRoots[$normalized] = $true
    if (Test-PlainDirectory $entry.Path) {
        $reports += @(Read-ReportDirectory $entry.Path 'moonfall_manual_bug_diagnostics' '^[0-9a-fA-F]{32}\.json$' $entry.Source)
        $reports += @(Read-ReportDirectory (Join-Path $entry.Path 'Recovery') 'moonfall_recovery_snapshot' '^[0-9a-fA-F]{32}\.json\.(next|previous)\.json$' $entry.Source)
    }
}
if ($IncludeInstalled) { $reports = @($reports | Sort-Object LastWriteUtc -Descending | Select-Object -First 200) }
[pscustomobject]@{ Reports = @($reports); Count = $reports.Count; Note = 'Local index only; recovery files may belong to a running game. Timing labels do not establish crash cause.' } | ConvertTo-Json -Depth 5
