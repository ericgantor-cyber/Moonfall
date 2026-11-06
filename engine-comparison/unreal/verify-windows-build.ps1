# Compile Moonfall's isolated Unreal 5.6 prototype on a Windows development machine.
# Requires UE 5.6, Visual Studio C++ tools, and Windows SDK. Does not touch Godot.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [switch]$Package,
    [string]$ArchiveDirectory = ""
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Project = Join-Path $PSScriptRoot "MoonfallPrototype\MoonfallPrototype.uproject"
$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw "Moonfall prototype .uproject is missing: $Project"
}
if (-not (Test-Path -LiteralPath $BuildBat -PathType Leaf)) {
    throw "Unreal Build.bat not found. Pass the actual UE 5.6 installation root: $EngineRoot"
}
$Project = (Resolve-Path -LiteralPath $Project).Path
$BuildBat = (Resolve-Path -LiteralPath $BuildBat).Path

function Invoke-UnrealBuild {
    param([string]$Target, [string]$Configuration)
    Write-Host "Building $Target Win64 $Configuration..."
    & $BuildBat $Target Win64 $Configuration "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) {
        throw "UnrealBuildTool failed for $Target Win64 $Configuration (exit $LASTEXITCODE)."
    }
}
Invoke-UnrealBuild "MoonfallPrototypeEditor" "Development"
Invoke-UnrealBuild "MoonfallPrototype" "Development"

$GameExe = Join-Path $PSScriptRoot "MoonfallPrototype\Binaries\Win64\MoonfallPrototype.exe"
if (-not (Test-Path -LiteralPath $GameExe -PathType Leaf)) {
    throw "Build command returned success but the Win64 game executable was not found: $GameExe"
}
Write-Host "Unreal Editor and Win64 Development game compilation succeeded."
Write-Host "Compilation alone does not prove packaged gameplay or save/load behavior."

if ($Package) {
    if (-not (Test-Path -LiteralPath $RunUAT -PathType Leaf)) {
        throw "RunUAT.bat not found: $RunUAT"
    }
    $Maps = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot "MoonfallPrototype\Content") -Recurse -File -Filter "*.umap")
    if ($Maps.Count -eq 0) {
        throw "Cannot verify a playable package: no .umap level is committed. Create and configure a startup map before packaging."
    }
    if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
        $ArchiveDirectory = Join-Path $PSScriptRoot "MoonfallPrototype\Saved\Win64Verification"
    }
    $ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
    & $RunUAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$ArchiveDirectory"
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal BuildCookRun failed (exit $LASTEXITCODE). No valid package was verified."
    }
    Write-Host "BuildCookRun completed. Manually launch and test the staged game on Windows 10 and 11."
}
