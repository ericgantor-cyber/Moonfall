# Compile Moonfall's isolated Unreal 5.6 prototype on a Windows development machine.
# Requires UE 5.6, Visual Studio C++ tools, and Windows SDK. Does not touch Godot.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,
    [switch]$Package,
    [switch]$CreateMap,
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
# EngineAssociation in the .uproject is 5.6. Fail before compiling with
# a different installed engine, whose generated files can be incompatible.
$VersionFile = Join-Path $EngineRoot "Engine\Build\Build.version"
if (-not (Test-Path -LiteralPath $VersionFile -PathType Leaf)) {
    throw "Unreal Build.version is missing: $VersionFile"
}
try {
    $EngineVersion = Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json
} catch {
    throw "Unreal Build.version could not be parsed: $VersionFile"
}
if ($EngineVersion.MajorVersion -ne 5 -or $EngineVersion.MinorVersion -ne 6) {
    throw "Moonfall prototype requires Unreal 5.6; selected engine is $($EngineVersion.MajorVersion).$($EngineVersion.MinorVersion)."
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

if ($CreateMap) {
    $EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    $MapScript = Join-Path $PSScriptRoot "MoonfallPrototype\Content\Python\create_startup_map.py"
    $MapAsset = Join-Path $PSScriptRoot "MoonfallPrototype\Content\Maps\MoonfallPrototype.umap"
    if (-not (Test-Path -LiteralPath $EditorExe -PathType Leaf)) {
        throw "Unreal editor executable is missing: $EditorExe"
    }
    if (-not (Test-Path -LiteralPath $MapScript -PathType Leaf)) {
        throw "Startup level creation script is missing: $MapScript"
    }
    if (-not (Test-Path -LiteralPath $MapAsset -PathType Leaf)) {
        & $EditorExe $Project "-ExecutePythonScript=$MapScript" -unattended -nop4
        if ($LASTEXITCODE -ne 0) {
            throw "Unreal Editor startup map creation failed (exit $LASTEXITCODE)."
        }
    }
    if (-not (Test-Path -LiteralPath $MapAsset -PathType Leaf)) {
        throw "Unreal Editor exited without creating the required .umap: $MapAsset"
    }
    Write-Host "Startup map is ready: $MapAsset"
}

if ($Package) {
    if (-not (Test-Path -LiteralPath $RunUAT -PathType Leaf)) {
        throw "RunUAT.bat not found: $RunUAT"
    }
    $ProjectRoot = Join-Path $PSScriptRoot "MoonfallPrototype"
    $ContentRoot = Join-Path $ProjectRoot "Content"
    $ConfigPath = Join-Path $ProjectRoot "Config\DefaultEngine.ini"
    $Maps = @(Get-ChildItem -LiteralPath $ContentRoot -Recurse -File -Filter "*.umap")
    if ($Maps.Count -eq 0) {
        throw "Cannot verify a playable package: no .umap level is committed. Create and configure a startup map before packaging."
    }
    if (-not (Test-Path -LiteralPath $ConfigPath -PathType Leaf)) {
        throw "Cannot package without Config/DefaultEngine.ini and a GameDefaultMap."
    }
    $ConfigText = Get-Content -LiteralPath $ConfigPath -Raw
    $ConfiguredMap = [regex]::Match($ConfigText, '(?m)^\s*GameDefaultMap\s*=\s*/Game/([A-Za-z0-9_/]+)\s*$')
    if (-not $ConfiguredMap.Success) {
        throw "Cannot verify a playable package: configure GameDefaultMap=/Game/Maps/YourLevel in DefaultEngine.ini."
    }
    $RelativeMap = $ConfiguredMap.Groups[1].Value.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
    $StartupMap = Join-Path $ContentRoot ($RelativeMap + ".umap")
    if (-not (Test-Path -LiteralPath $StartupMap -PathType Leaf)) {
        throw "Configured startup map does not exist: $StartupMap"
    }
    Write-Host "Verified configured startup map: $StartupMap"
    if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
        $ArchiveDirectory = Join-Path $PSScriptRoot "MoonfallPrototype\Saved\Win64Verification"
    }
    $ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
    $PackageStartedUtc = [DateTime]::UtcNow
    & $RunUAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$ArchiveDirectory"
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal BuildCookRun failed (exit $LASTEXITCODE). No valid package was verified."
    }
    if (-not (Test-Path -LiteralPath $ArchiveDirectory -PathType Container)) {
        throw "BuildCookRun returned success but the archive directory does not exist: $ArchiveDirectory"
    }
    # Prevent a stale build from being mistaken for the output of this run.
    # Allow two seconds of timestamp granularity for Windows filesystems.
    $PackagedExecutables = @(Get-ChildItem -LiteralPath $ArchiveDirectory -File -Recurse -Filter "MoonfallPrototype.exe" |
        Where-Object { $_.Length -gt 0 -and $_.LastWriteTimeUtc -ge $PackageStartedUtc.AddSeconds(-2) })
    if ($PackagedExecutables.Count -eq 0) {
        throw "BuildCookRun returned success but no fresh, nonempty packaged MoonfallPrototype.exe was found in $ArchiveDirectory"
    }
    Write-Host "Verified fresh Win64 package executable: $($PackagedExecutables[0].FullName)"
    Write-Host "BuildCookRun completed. Manually launch and test the staged game on Windows 10 and 11."
}
