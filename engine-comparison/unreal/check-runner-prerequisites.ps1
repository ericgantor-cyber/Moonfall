# Moonfall Unreal 5.6 self-hosted runner readiness check.
# Safe to run before registering the GitHub runner. Does not install software
# or handle registration tokens.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$EngineRoot)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if (-not $IsWindows -and $PSVersionTable.PSEdition -eq 'Core') {
    throw "This check requires Windows x64."
}
if (-not [Environment]::Is64BitOperatingSystem) {
    throw "A 64-bit Windows installation is required."
}
$build = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$uat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$versionPath = Join-Path $EngineRoot "Engine\Build\Build.version"
foreach ($required in @($build, $editor, $uat, $versionPath)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required Unreal 5.6 file missing: $required"
    }
}
try {
    $version = Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json
} catch {
    throw "Cannot parse the selected Unreal Build.version: $versionPath"
}
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 6) {
    throw "Unreal Engine 5.6 is required; found $($version.MajorVersion).$($version.MinorVersion)."
}
$vswhere = Join-Path "${env:ProgramFiles(x86)}" "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    throw "Visual Studio Installer (vswhere.exe) was not found. Install Visual Studio C++ build tools."
}
$vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace(($vsInstall | Out-String))) {
    throw "Visual Studio C++ x64 toolchain was not found. Install Desktop development with C++."
}
$sdkRoot = Join-Path "${env:ProgramFiles(x86)}" "Windows Kits\10\Include"
if (-not (Test-Path -LiteralPath $sdkRoot -PathType Container)) {
    throw "Windows SDK headers not found. Install the Windows 10/11 SDK."
}
$sdks = @(Get-ChildItem -LiteralPath $sdkRoot -Directory | Where-Object {
    Test-Path -LiteralPath (Join-Path $_.FullName "um\Windows.h")
})
if ($sdks.Count -eq 0) {
    throw "Windows SDK is incomplete: Windows.h not found beneath $sdkRoot."
}
Write-Host "Moonfall Unreal runner prerequisites: READY"
Write-Host "UE: $($version.MajorVersion).$($version.MinorVersion)"
Write-Host "VS C++: $(($vsInstall | Select-Object -First 1))"
Write-Host "SDK: $(($sdks | Select-Object -Last 1).Name)"
Write-Host "Next: register a trusted GitHub Windows x64 self-hosted runner with the unreal-5.6 label."
Write-Host "This is only a prerequisite check; it does not compile Moonfall or connect a runner."
