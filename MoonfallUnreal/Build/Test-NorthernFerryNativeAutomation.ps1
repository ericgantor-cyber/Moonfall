# Run an actual Unreal Editor test, not source-text checks or a mock report.
# Requires a trusted Windows runner with a licensed UE 5.4 installation.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$UnrealRoot,
    [Parameter(Mandatory = $true)][string]$ProjectPath,
    [Parameter(Mandatory = $true)][string]$ReportDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$testName = 'Moonfall.NorthernExpanse.Ferry.ComponentContract'
$editor = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe is missing: $editor"
}
if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
    throw "Moonfall .uproject is missing: $ProjectPath"
}
$resolvedProject = (Resolve-Path -LiteralPath $ProjectPath).Path
# Never reuse old reports. Stale JSON must not masquerade as native proof.
if (Test-Path -LiteralPath $ReportDirectory) {
    throw "Native ferry report directory already exists; refusing stale evidence: $ReportDirectory"
}
New-Item -ItemType Directory -Path $ReportDirectory -Force | Out-Null
$report = Join-Path $ReportDirectory 'index.json'

Write-Host "Running native UE Editor automation test: $testName"
$editorArgs = @(
    $resolvedProject, '-unattended', '-nop4', '-nosplash', '-nullrhi',
    "-ReportExportPath=$ReportDirectory",
    ('-ExecCmds="Automation RunTest ' + $testName + ';Quit"'), '-log'
)
& $editor @editorArgs
$editorExitCode = $LASTEXITCODE
if ($editorExitCode -ne 0) {
    throw "Native Unreal automation exited with code $editorExitCode"
}
if (-not (Test-Path -LiteralPath $report -PathType Leaf)) {
    throw "Native Unreal test did not produce index.json; a zero exit code is not proof."
}
$data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json -Depth 100
$tests = @($data.tests | Where-Object { $_.fullTestPath -eq $testName })
if ($tests.Count -ne 1) {
    throw "Expected exactly one native ferry test result; found $($tests.Count)."
}
$test = $tests[0]
if ($test.state -ne 'Success' -or [int]$test.errors -ne 0 -or
    [int]$data.failed -ne 0 -or [int]$data.notRun -ne 0 -or
    [int]$data.inProcess -ne 0 -or [int]$data.succeeded -lt 1) {
    throw "Native ferry automation has non-success results: $($test.state), failed=$($data.failed), notRun=$($data.notRun)"
}
$proof = Join-Path $ReportDirectory 'MoonfallFerryNativeProof.txt'
@(
    'Native Moonfall Unreal ferry automation: PASSED'
    "Test: $testName"
    "Project: $resolvedProject"
    "Commit: $env:GITHUB_SHA"
    "Runner: $env:RUNNER_NAME"
    "Verified UTC: $([DateTime]::UtcNow.ToString('o'))"
    'Scope: native Editor component/geometry contract; NOT a PIE walkable-deck gameplay test'
) | Set-Content -LiteralPath $proof -Encoding UTF8
Write-Host "Native ferry component test PASSED. Evidence: $report"
if ($env:GITHUB_STEP_SUMMARY) {
    Get-Content -LiteralPath $proof | ForEach-Object { $_ >> $env:GITHUB_STEP_SUMMARY }
}
