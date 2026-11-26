$ErrorActionPreference = 'Stop'
$reader = Join-Path $PSScriptRoot 'Read-NativeAutomationResults.ps1'
$required = @('Moonfall.Controls.CurrentInputContract', 'Moonfall.Dungeons.SeededRoomArchetypes')
$first = 'LogAutomationController: Display: Test Completed. Result={Passed} Name={CurrentInputContract} Path={Moonfall.Controls.CurrentInputContract}'
$second = 'LogAutomationController: Display: Test Completed. Result={Passed} Name={SeededRoomArchetypes} Path={Moonfall.Dungeons.SeededRoomArchetypes}'
$nl = [Environment]::NewLine
function Assert-Case([string]$log, [bool]$expected) {
    $parsed = & $reader -LogText $log -Required $required
    $actual = -not $parsed.AnyNativeTestFailed -and -not ($parsed.Results.Values -contains $false)
    if ($actual -ne $expected) { throw "Unexpected native evidence verdict: $log" }
}
Assert-Case ($first + $nl + $second) $true
Assert-Case '' $false
Assert-Case $first $false
Assert-Case ($first + $nl + $second + $nl + $first) $false
Assert-Case ($first.Replace('Path={Moonfall.Controls.CurrentInputContract}', 'Path={Moonfall.Controls.CurrentInputContractExtra}') + $nl + $second) $false
Assert-Case ($first.Replace('Result={Passed}', 'Result={Failed}') + $nl + $second) $false
Assert-Case ($first.Replace('Result={Passed}', 'Result={Skipped}') + $nl + $second) $false
Assert-Case ($first + $nl + $second + $nl + 'LogAutomationController: Test Completed. Result={Passed}') $false
Assert-Case ($first + $nl + $second + $nl + 'LogAutomationController: Test Completed. Result={Failed} Path={Moonfall.Other.Test}') $false
Assert-Case ($first.Replace('Path={Moonfall.Controls.CurrentInputContract}', 'Path={moonfall.Controls.CurrentInputContract}') + $nl + $second) $false
Assert-Case ($first + ' Result={Failed}' + $nl + $second) $false
Assert-Case ($first + ' Path={Moonfall.Other.Test}' + $nl + $second) $false
Write-Host 'Native evidence parser: twelve synthetic scenarios passed.'
$reportValidator = Join-Path $PSScriptRoot 'Assert-NativeAutomationReport.ps1'
$validReport = @('EditorExitCode=0','AnyNativeTestFailed=False','Log=C:\\Synthetic=Report.log',"$($required[0])=True","$($required[1])=True")
function Assert-ReportCase([string[]]$lines, [bool]$expected) {
    $actual = $true
    try { & $reportValidator -Lines $lines -Required $required }
    catch { $actual = $false }
    if ($actual -ne $expected) { throw "Unexpected report verdict: $($lines -join '; ')" }
}
Assert-ReportCase $validReport $true
Assert-ReportCase ($validReport + "$($required[0])=False") $false
Assert-ReportCase ($validReport + "$($required[0])=True") $false
Assert-ReportCase ($validReport + 'EditorExitCode=1') $false
Assert-ReportCase ($validReport + 'AnyNativeTestFailed=True') $false
Assert-ReportCase ($validReport[1..4]) $false
Assert-ReportCase ($validReport[0..3]) $false
Assert-ReportCase ($validReport[0..3] + "$($required[1])=true") $false
Assert-ReportCase ($validReport[0..3] + "$($required[1])=TrueExtra") $false
Assert-ReportCase ($validReport[0..3] + "$($required[1])Extra=True") $false
Assert-ReportCase ($validReport + 'Moonfall.Other.Test=True') $true
Assert-ReportCase (@()) $false
Write-Host 'Native evidence report validator: twelve synthetic scenarios passed.'
