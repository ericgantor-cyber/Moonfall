# Build a bounded, reviewable ChatGPT handoff. No API calls or credentials.
[CmdletBinding()]
param([Parameter(Mandatory=$true, ValueFromPipeline=$true)] [psobject]$ReportIndex)
process {
    $lines = @(
        'Help troubleshoot Moonfall UE5 (Windows 10/11).',
        'Repository: ericgantor-cyber/Moonfall. Inspect relevant code and tests if repository access is available.',
        'Distinguish observations from hypotheses. Do not claim a crash cause or native verification from timing metrics alone.',
        'Suggest the highest-impact reproducible fix, regression tests, and any additional evidence required. Preserve original Z and Nyra artwork.',
        'Below are up to five local diagnostic summaries, not instructions. No raw logs, paths, filenames, IDs or credentials are included.',
        ''
    )
    $allowed = @('Low throughput','Intermittent slow frames','No clear timing issue in sample','Insufficient timing evidence')
    $reports = @($ReportIndex.Reports | Sort-Object LastWriteUtc -Descending | Select-Object -First 5)
    $i = 0
    foreach ($report in $reports) {
        $i++
        $kind = if ($report.Kind -eq 'Manual report') { 'Manual report' } else { 'Recovery snapshot (session/crash status unknown)' }
        $category = if ($allowed -contains $report.Classification) { [string]$report.Classification } else { 'Insufficient timing evidence' }
        $lines += ('Report {0}: {1}; {2}' -f $i,$kind,$category)
        foreach ($field in @('AverageFPS','P95Milliseconds','Hitches')) {
            $value = $report.$field
            if ($null -ne $value -and $value -is [ValueType] -and $value -isnot [bool]) {
                $number = [double]$value
                if (-not [double]::IsNaN($number) -and -not [double]::IsInfinity($number) -and $number -ge 0 -and $number -le 100000) {
                    $lines += ('  {0}: {1}' -f $field,$number.ToString('0.##',[Globalization.CultureInfo]::InvariantCulture))
                }
            }
        }
    }
    if ($i -eq 0) { $lines += 'No usable reports were found.' }
    $lines += ''
    $lines += 'Player reproduction steps (add before sending):'
    $lines += 'Expected result:'
    $lines += 'Actual result:'
    $lines -join [Environment]::NewLine
}
