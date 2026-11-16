# Local, sequential UE5.6 C++ compile, cook/package and headless boot verification.
# Run from a trusted Windows PC that already has Unreal Engine 5.6, VS2022 C++ and Windows SDK.
# Does not install UE, create a signed installer or test graphical gameplay.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$UnrealRoot,
    [int]$BootSeconds = 25,
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'MoonfallPrototype.uproject'
$Preflight = Join-Path $PSScriptRoot 'Check-WindowsRunner.ps1'
$Packager = Join-Path $PSScriptRoot 'Package-Windows.ps1'
$Smoke = Join-Path $PSScriptRoot 'SmokeTest-Windows.ps1'
foreach ($File in @($Project, $Preflight, $Packager, $Smoke)) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
        throw "Missing Moonfall verification prerequisite: $File"
    }
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $env:TEMP ('MoonfallUE56Verify-' + [Guid]::NewGuid().ToString('N'))
}
if (Test-Path -LiteralPath $OutputDirectory) {
    if (@(Get-ChildItem -LiteralPath $OutputDirectory -Force).Count -gt 0) {
        throw "Refusing to reuse a nonempty archive directory: $OutputDirectory"
    }
}

Write-Host '[1/4] Verifying Windows engine and C++ toolchain'
& $Preflight -UnrealRoot $UnrealRoot

$Build = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
foreach ($Target in @('MoonfallPrototypeEditor', 'MoonfallPrototype')) {
    Write-Host "[2/4] Compiling UE5.6 Win64 Development target: $Target"
    & $Build $Target 'Win64' 'Development' "-Project=$Project" '-WaitMutex' '-NoHotReloadFromIDE'
    if ($LASTEXITCODE -ne 0) { throw "Unreal $Target C++ compilation failed ($LASTEXITCODE)" }
}
Write-Host '[3/4] Generating startup map if needed; cooking, staging and packaging Win64'
& $Packager -UnrealRoot $UnrealRoot -OutputDirectory $OutputDirectory
Write-Host '[4/4] Checking package hashes and running a headless startup smoke'
& $Smoke -PackageDirectory $OutputDirectory -BootSeconds $BootSeconds
Write-Host "Moonfall UE5.6 sequential local verification passed. Package: $OutputDirectory"
Write-Host 'No visual rendering, input, quest, save/reload or installer validation was performed.'
