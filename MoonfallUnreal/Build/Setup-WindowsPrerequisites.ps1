param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [switch]$InstallMissing
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

Write-Host 'Moonfall Windows build dependency setup'
if (-not $UnrealRoot) {
    $UnrealRoot = 'C:\Program Files\Epic Games\UE_5.4'
}
$build = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
$uat = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$entry = Join-Path $UnrealRoot 'Engine\Content\Maps\Entry.umap'
foreach ($file in @($build, $uat, $entry)) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Unreal Engine 5.4 prerequisite missing: $file. Install/repair UE 5.4 first; Unreal Engine is not downloaded by this script."
    }
}
$env:UE54_ROOT = $UnrealRoot
$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'
function Test-VsCpp {
    if (-not (Test-Path -LiteralPath $vswhere)) { return $false }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    return ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace(($installation | Out-String)))
}
function Test-DotNet8 {
    if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) { return $false }
    $installed = @(& dotnet --list-sdks)
    return [bool]($installed | Where-Object { $_ -match '^8\.' })
}
function Test-Nsis {
    return [bool](Get-Command makensis.exe -ErrorAction SilentlyContinue)
}
$missing = @()
if (-not (Test-DotNet8)) { $missing += 'Microsoft.DotNet.SDK.8' }
if (-not (Test-Nsis)) { $missing += 'NSIS.NSIS' }
if (-not (Test-VsCpp)) { $missing += 'Microsoft.VisualStudio.2022.BuildTools' }
if ($missing.Count -eq 0) {
    Write-Host 'All supported build prerequisites are detected.'
} else {
    Write-Host "Missing packages: $($missing -join ', ')"
    if (-not $InstallMissing) {
        Write-Host 'Preview only. Re-run with -InstallMissing to install packages through winget.'
        exit 0
    }
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw 'winget not found. Install Microsoft App Installer, then retry.'
    }
    foreach ($id in $missing) {
        Write-Host "Installing $id through winget..."
        if ($id -eq 'Microsoft.VisualStudio.2022.BuildTools') {
            & winget install --id $id --exact --source winget --accept-source-agreements --accept-package-agreements --override '--quiet --wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended'
        } else {
            & winget install --id $id --exact --source winget --accept-source-agreements --accept-package-agreements
        }
        if ($LASTEXITCODE -ne 0) { throw "Package installation failed for $id (exit $LASTEXITCODE)" }
    }
    Write-Host 'Install commands finished. Open a NEW PowerShell window to refresh PATH and verify .NET, NSIS and Visual Studio.'
}
Write-Host "Unreal root checked: $UnrealRoot"
Write-Host 'For GitHub self-hosted runner registration, follow GitHub Settings > Actions > Runners.'
Write-Host 'Only connect a trusted isolated runner to a private repository; do not paste a registration token into chat.'
Write-Host 'When runner is registered, add the unreal-5-4 label and set UE54_ROOT in the runner process environment.'
