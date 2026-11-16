param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [switch]$SkipBuild,
    [string]$ReportDirectory = ""
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$project = Join-Path $repoRoot 'MoonfallUnreal\MoonfallUnreal.uproject'
if ([string]::IsNullOrWhiteSpace($UnrealRoot)) {
    throw 'Unreal Engine 5.4 is required. Supply -UnrealRoot or set UE54_ROOT.'
}
$editor = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
    throw "Unreal Editor executable missing: $editor"
}
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Unreal project missing: $project"
}
& (Join-Path $PSScriptRoot 'Test-UnrealEngineVersion.ps1') -UnrealRoot $UnrealRoot
if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'Build-Windows.ps1') -UnrealRoot $UnrealRoot -CompileOnly
    if ($LASTEXITCODE -ne 0) { throw "Native UE5 Win64 compilation failed: $LASTEXITCODE" }
}
if (-not $ReportDirectory) {
    $ReportDirectory = Join-Path $repoRoot 'MoonfallUnrealLocalBuild\DungeonAutomation'
}
$ReportDirectory = [IO.Path]::GetFullPath($ReportDirectory)
$log = Join-Path $ReportDirectory 'Moonfall-Dungeon-UE54.log'
$report = Join-Path $ReportDirectory 'Moonfall-Dungeon-TestResult.txt'
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report -Force }
Write-Host 'Running real UE5.4 native dungeon tests (not GitHub source checks).'
& $editor $project -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests Moonfall.Dungeons; Quit' '-TestExit=Automation Test Queue Empty' "-abslog=$log"
$editorExit = $LASTEXITCODE
if (-not (Test-Path -LiteralPath $log -PathType Leaf)) {
    throw "Unreal did not write the automation log. Editor exit code: $editorExit"
}
$logText = Get-Content -LiteralPath $log -Raw
$required = @(
    'Moonfall.Dungeons.All30EntranceLocations',
    'Moonfall.Dungeons.SeededRoomArchetypes',
    'Moonfall.Dungeons.BossVictoryLedgerDefaults'
)
# Fail closed. A test being discovered or started does not count as success.
$results = @{}
foreach ($name in $required) {
    $escaped = [regex]::Escape($name)
    $passPattern = "(?im)^.*Test Completed\..*$escaped.*Result=\{Passed\}.*$"
    $altPattern = "(?im)^.*Test Completed\..*Result=\{Passed\}.*$escaped.*$"
    $results[$name] = [regex]::IsMatch($logText, $passPattern) -or
                      [regex]::IsMatch($logText, $altPattern)
}
$failedMarker = [regex]::IsMatch($logText, '(?im)Test Completed\..*Result=\{Failed\}')
$summary = @(
    "EditorExitCode=$editorExit",
    "AnyNativeTestFailed=$failedMarker",
    "Log=$log"
)
foreach ($name in $required) { $summary += "$name=$($results[$name])" }
$summary | Set-Content -LiteralPath $report -Encoding UTF8
$summary | ForEach-Object { Write-Host $_ }
if ($editorExit -ne 0 -or $failedMarker -or ($results.Values -contains $false)) {
    throw "Native dungeon tests NOT verified as passed. Inspect $log and $report."
}
Write-Host 'MOONFALL_NATIVE_DUNGEON_TESTS_PASS: all three native tests explicitly passed in UE5.4.'
