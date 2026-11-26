# Parse Unreal automation completion lines without trusting partial test names.
param(
    [Parameter(Mandatory=$true)][AllowEmptyString()][string]$LogText,
    [Parameter(Mandatory=$true)][string[]]$Required
)
$ErrorActionPreference = 'Stop'
$results = [hashtable]::new([System.StringComparer]::Ordinal)
$counts = [hashtable]::new([System.StringComparer]::Ordinal)
foreach ($name in $Required) { $results[$name] = $false; $counts[$name] = 0 }
$failed = $false
foreach ($line in ($LogText -split '\r?\n')) {
    if ($line -notmatch 'Test Completed\.') { continue }
    $result = [regex]::Matches($line, '\bResult=\{([^{}]+)\}')
    $path = [regex]::Matches($line, '\bPath=\{([^{}]+)\}')
    # An incomplete completion record is not trustworthy evidence.
    if ($result.Count -ne 1 -or $path.Count -ne 1) {
        $failed = $true
        continue
    }
    $passed = $result[0].Groups[1].Value -ceq 'Passed'
    if (-not $passed) { $failed = $true }
    $name = $path[0].Groups[1].Value
    if ($counts.ContainsKey($name)) {
        $counts[$name]++
        $results[$name] = ($counts[$name] -eq 1 -and $passed)
    }
}
return [pscustomobject]@{ Results = $results; AnyNativeTestFailed = $failed }
