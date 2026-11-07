param(
    [Parameter(Mandatory = $true)][string]$UnrealRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$versionFile = Join-Path $UnrealRoot 'Engine\Build\Build.version'
$buildBat = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
$ubt = Join-Path $UnrealRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
foreach ($file in @($versionFile, $buildBat, $ubt)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Unreal installed build missing required file: $file"
    }
}
try { $info = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json }
catch { throw 'Engine/Build/Build.version is not valid JSON.' }
if ($null -eq $info.MajorVersion -or $null -eq $info.MinorVersion) {
    throw 'Engine version metadata is missing a major or minor version.'
}
if ($info.MajorVersion -isnot [int] -and $info.MajorVersion -isnot [long]) {
    throw 'Unreal engine major version must be an integer.'
}
if ($info.MinorVersion -isnot [int] -and $info.MinorVersion -isnot [long]) {
    throw 'Unreal engine minor version must be an integer.'
}
if ([int]$info.MajorVersion -ne 5 -or [int]$info.MinorVersion -ne 4) {
    throw "Moonfall requires UE 5.4; archive declares $($info.MajorVersion).$($info.MinorVersion)."
}
Write-Host 'MOONFALL_UE_VERSION_PASS Unreal Engine 5.4 toolchain and metadata present. Not yet compiled.'
