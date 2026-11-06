param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [switch]$RunCompile
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# This helper deliberately never downloads a runner, accepts registration tokens,
# or connects a workstation to GitHub without the owner's separate action.
if ([string]::IsNullOrWhiteSpace($UnrealRoot)) {
    $UnrealRoot = 'C:\Program Files\Epic Games\UE_5.4'
}
$UnrealRoot = [IO.Path]::GetFullPath($UnrealRoot)
$engineBuild = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
$project = Join-Path $PSScriptRoot '..\MoonfallUnreal.uproject'
$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'

$unrealFound = Test-Path -LiteralPath $engineBuild -PathType Leaf
$projectFound = Test-Path -LiteralPath $project -PathType Leaf
$compilerFound = $false
if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
    $vsInstallation = @(& $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
    $compilerFound = $LASTEXITCODE -eq 0 -and [bool](@($vsInstallation | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count)
}

Write-Host 'MOONFALL | Unreal C++ compilation readiness'
Write-Host "Unreal build tool: $(if ($unrealFound) {'FOUND'} else {'MISSING'})"
Write-Host "Unreal project:    $(if ($projectFound) {'FOUND'} else {'MISSING'})"
Write-Host "Visual Studio C++: $(if ($compilerFound) {'FOUND'} else {'MISSING'})"
Write-Host ''
Write-Host 'To use GitHub automation on this trusted Windows computer:'
Write-Host '  1. Open https://github.com/ericgantor-cyber/Moonfall/settings/actions/runners'
Write-Host '  2. Register a dedicated Windows x64 runner using GitHub instructions.'
Write-Host '  3. Assign the runner the custom label: unreal-5-4'
Write-Host "  4. Set UE54_ROOT=$UnrealRoot in the runner process environment."
Write-Host '  5. Run the reviewed branch from the manual Unreal C++ compile workflow.'
Write-Host 'Do not share the GitHub runner registration token in chat.'
if (-not ($unrealFound -and $projectFound -and $compilerFound)) {
    throw 'Compilation prerequisites are incomplete. No runner was registered and no compilation was attempted.'
}
if ($RunCompile) {
    Write-Host 'Running real Unreal C++ editor compilation locally...'
    & (Join-Path $PSScriptRoot 'Build-Windows.ps1') -UnrealRoot $UnrealRoot -CompileOnly
    if ($LASTEXITCODE -ne 0) { throw "Unreal compiler exited with code $LASTEXITCODE" }
} else {
    Write-Host 'Ready for manual Windows runner registration. No compilation has run.'
}
