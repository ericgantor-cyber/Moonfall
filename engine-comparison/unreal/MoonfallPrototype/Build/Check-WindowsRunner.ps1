# Run before Moonfall UE5.6 compiler/cooker steps on a trusted Windows runner.
# Checks requirements without claiming that Unreal compilation has succeeded.
[CmdletBinding()]
param(
    [string]$UnrealRoot = $env:UE56_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($env:OS -ne 'Windows_NT') {
    throw 'Moonfall UE5.6 CI requires Windows x64.'
}
if (-not [Environment]::Is64BitOperatingSystem) {
    throw 'Moonfall UE5.6 CI requires a 64-bit Windows OS.'
}
if ([string]::IsNullOrWhiteSpace($UnrealRoot)) {
    throw 'UE56_ROOT must point to an installed UE5.6 engine directory.'
}
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'MoonfallPrototype.uproject'
$Build = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
$RunUAT = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$Editor = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$EngineVersion = Join-Path $UnrealRoot 'Engine\Build\Build.version'

foreach ($Required in @($Project, $Build, $RunUAT, $Editor, $EngineVersion)) {
    if (-not (Test-Path -LiteralPath $Required -PathType Leaf)) {
        throw "Missing Windows Unreal prerequisite: $Required"
    }
}
$Version = Get-Content -LiteralPath $EngineVersion -Raw | ConvertFrom-Json
if ($Version.MajorVersion -ne 5 -or $Version.MinorVersion -ne 6) {
    throw "Expected Unreal 5.6, found $($Version.MajorVersion).$($Version.MinorVersion)."
}
$ProjectData = Get-Content -LiteralPath $Project -Raw | ConvertFrom-Json
if ($ProjectData.EngineAssociation -ne '5.6') {
    throw 'MoonfallPrototype.uproject must target Unreal Engine 5.6.'
}

$VsWhere = Join-Path "${env:ProgramFiles(x86)}" 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $VsWhere -PathType Leaf)) {
    throw "vswhere.exe was not found at $VsWhere. Install Visual Studio 2022 with Desktop development with C++."
}
$InstallRoot = @(& $VsWhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if ($LASTEXITCODE -ne 0 -or $InstallRoot.Count -eq 0 -or [string]::IsNullOrWhiteSpace($InstallRoot[0])) {
    throw 'Visual Studio 2022 with MSVC x64 C++ tools is missing.'
}
$VcTools = Join-Path $InstallRoot[0] 'VC\Tools\MSVC'
if (-not (Test-Path -LiteralPath $VcTools -PathType Container)) {
    throw 'Visual Studio C++ compiler tools directory is missing.'
}

$Kits = Get-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
if (-not $Kits -or -not $Kits.KitsRoot10) {
    throw 'Windows 10/11 SDK installation was not found in the Windows Kits registry.'
}
$SdkLibs = Join-Path $Kits.KitsRoot10 'Lib'
if (-not (Test-Path -LiteralPath $SdkLibs -PathType Container)) {
    throw 'Windows SDK libraries directory was not found.'
}
$SdkVersions = @(Get-ChildItem -LiteralPath $SdkLibs -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um\x64') -PathType Container })
if ($SdkVersions.Count -eq 0) {
    throw 'No Windows SDK x64 libraries are installed.'
}
Write-Host 'Moonfall trusted Windows UE5.6 runner preflight passed.'
Write-Host "Engine: $UnrealRoot"
Write-Host "Visual Studio: $($InstallRoot[0])"
Write-Host "Windows SDK versions: $(($SdkVersions | ForEach-Object { $_.Name }) -join ', ')"
Write-Host 'Preflight does not prove Unreal source compilation, cooking or gameplay.'
