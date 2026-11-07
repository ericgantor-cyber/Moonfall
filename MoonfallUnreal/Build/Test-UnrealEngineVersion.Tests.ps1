$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$checker = Join-Path $PSScriptRoot 'Test-UnrealEngineVersion.ps1'
$base = Join-Path $env:RUNNER_TEMP 'MoonfallEngineVersionSyntheticTests'
if (Test-Path -LiteralPath $base) { throw 'Synthetic engine test root already exists.' }
New-Item -ItemType Directory -Force -Path $base | Out-Null
function Test-EngineFixture {
    param([string]$Name, [string]$Metadata, [bool]$ExpectedPass, [string]$Missing = '')
    $root = Join-Path $base $Name
    foreach ($dir in @('Engine\Build\BatchFiles', 'Engine\Binaries\DotNET\UnrealBuildTool')) {
        New-Item -ItemType Directory -Force -Path (Join-Path $root $dir) | Out-Null
    }
    Set-Content -LiteralPath (Join-Path $root 'Engine\Build\BatchFiles\Build.bat') -Value 'test fixture'
    Set-Content -LiteralPath (Join-Path $root 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll') -Value 'test fixture'
    Set-Content -LiteralPath (Join-Path $root 'Engine\Build\Build.version') -Value $Metadata
    if ($Missing) { Remove-Item -LiteralPath (Join-Path $root $Missing) -Force }
    $passed = $false
    try {
        & $checker -UnrealRoot $root
        $passed = $true
    } catch { Write-Host "Expected validation rejection $Name : $($_.Exception.Message)" }
    if ($passed -ne $ExpectedPass) { throw "Version test mismatch $Name : got $passed expected $ExpectedPass" }
    Write-Host "MOONFALL_ENGINE_VERSION_TEST_PASS $Name"
}
Test-EngineFixture -Name 'valid54' -Metadata '{"MajorVersion":5,"MinorVersion":4,"PatchVersion":4}' -ExpectedPass $true
Test-EngineFixture -Name 'wrong53' -Metadata '{"MajorVersion":5,"MinorVersion":3}' -ExpectedPass $false
Test-EngineFixture -Name 'wrong55' -Metadata '{"MajorVersion":5,"MinorVersion":5}' -ExpectedPass $false
Test-EngineFixture -Name 'wrongMajor' -Metadata '{"MajorVersion":4,"MinorVersion":4}' -ExpectedPass $false
Test-EngineFixture -Name 'badJson' -Metadata '{broken' -ExpectedPass $false
Test-EngineFixture -Name 'missingVersion' -Metadata '{}' -ExpectedPass $false
Test-EngineFixture -Name 'fakeTextVersion' -Metadata '{"MajorVersion":"5","MinorVersion":"4"}' -ExpectedPass $false
Test-EngineFixture -Name 'missingUbt' -Metadata '{"MajorVersion":5,"MinorVersion":4}' -ExpectedPass $false -Missing 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
Test-EngineFixture -Name 'missingBuildBat' -Metadata '{"MajorVersion":5,"MinorVersion":4}' -ExpectedPass $false -Missing 'Engine\Build\BatchFiles\Build.bat'
Remove-Item -LiteralPath $base -Recurse -Force
Write-Host 'All Moonfall Unreal Engine 5.4 version and toolchain fixture tests passed.'
