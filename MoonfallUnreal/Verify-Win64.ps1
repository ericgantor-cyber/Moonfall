# Requires a local Unreal Engine 5.4 installation and Visual Studio C++ tools.
# Example: .\Verify-Win64.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.4"
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [switch]$BuildGame
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$project = Join-Path $PSScriptRoot "MoonfallUnreal.uproject"
$buildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$versionFile = Join-Path $EngineRoot "Engine\Build\Build.version"
if (-not (Test-Path -LiteralPath $project)) { throw "MoonfallUnreal.uproject is missing." }
if (-not (Test-Path -LiteralPath $buildBat)) { throw "Unreal Build.bat not found under EngineRoot: $EngineRoot" }
if (-not (Test-Path -LiteralPath $versionFile)) { throw "Engine Build.version is missing." }
$version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 4) {
    throw "Moonfall's Unreal migration targets UE 5.4, but this engine is $($version.MajorVersion).$($version.MinorVersion)."
}
$targets = @("MoonfallUnrealEditor")
if ($BuildGame) { $targets += "MoonfallUnreal" }
foreach ($target in $targets) {
    Write-Host "Building $target for Win64 Development with Unreal Engine 5.4..."
    $args = @($target, "Win64", "Development", "-Project=$project", "-WaitMutex", "-NoHotReload")
    & $buildBat @args
    $result = $LASTEXITCODE
    if ($result -ne 0) { throw "$target failed native Unreal compilation (exit $result)." }
}
Write-Host "Native Unreal Win64 compile checks passed. This does NOT prove packaging or gameplay correctness."
