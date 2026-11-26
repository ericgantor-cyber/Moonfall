# Local report index. Does not upload, delete, open arbitrary paths or read saves.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Join-Path (Split-Path $PSScriptRoot -Parent) 'Saved\BugReports'
function Read-ReportDirectory([string]$Directory, [string]$Kind, [string]$Pattern) {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return }
    $folder = Get-Item -LiteralPath $Directory
    if ($folder.Attributes -band [IO.FileAttributes]::ReparsePoint) { return }
    $files = @(Get-ChildItem -LiteralPath $Directory -File -Filter '*.json' |
        Where-Object { $_.Name -match $Pattern -and $_.Length -gt 0 -and $_.Length -le 131072 -and
            -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 100)
    foreach ($file in $files) {
        try {
            $report = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($report.schema -ne 1 -or $report.kind -ne $Kind) { continue }
            $fps = $null
            $p95 = $null
            $hitches = $null
            $classification = 'Insufficient timing evidence'
            if ($report.gameplay_sample_ready -eq $true -and
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
                File = $file.Name
                LastWriteUtc = $file.LastWriteTimeUtc.ToString('o')
                Kind = if ($Kind -eq 'moonfall_recovery_snapshot') { 'Interrupted-session evidence; crash not established' } else { 'Manual report' }
                Classification = $classification
                AverageFPS = $fps
                P95Milliseconds = $p95
                Hitches = $hitches
            }
        } catch { Write-Warning ('Invalid or unreadable report: ' + $file.Name) }
    }
}
$reports = @()
if (Test-Path -LiteralPath $root -PathType Container) {
    $rootItem = Get-Item -LiteralPath $root
    if (-not ($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        $reports += @(Read-ReportDirectory $root 'moonfall_manual_bug_diagnostics' '^[0-9a-fA-F]{32}\.json$')
        $reports += @(Read-ReportDirectory (Join-Path $root 'Recovery') 'moonfall_recovery_snapshot' '^[0-9a-fA-F]{32}\.json\.(next|previous)\.json$')
    }
}
[pscustomobject]@{ Reports = @($reports); Count = $reports.Count; Note = 'Local index only; timing labels are heuristic and do not establish crash cause.' } | ConvertTo-Json -Depth 5
