# Parse Unreal automation completion lines without trusting partial test names.
param(
    [Parameter(Mandatory=$true)][AllowEmptyString()][string]$LogText,
    [Parameter(Mandatory=$true)][string[]]$Required
)
$ErrorActionPreference = 'Stop'
$results = @{}
$counts = @{}
foreach ($name in $Required) { $results[$name] = $false; $counts[$name] = 0 }
$failed = $false
foreach ($line in ($LogText -split '\r?\n')) {
    if ($line -notmatch 'Test Completed\.') { continue }
    $result = [regex]::Match($line, '\bResult=\{([^{}]+)\}')
    $path = [regex]::Match($line, '\bPath=\{([^{}]+)\}')
    # An incomplete completion record is not trustworthy evidence.
    if (-not $result.Success -or -not $path.Success) {
        $failed = $true
        continue
    }
    $passed = $result.Groups[1].Value -ceq 'Passed'
    if (-not $passed) { $failed = $true }
    $name = $path.Groups[1].Value
    if ($counts.ContainsKey($name)) {
        $counts[$name]++
        $results[$name] = ($counts[$name] -eq 1 -and $passed)
    }
}
return [pscustomobject]@{ Results = $results; AnyNativeTestFailed = $failed }
