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

function Test-WindowsExecutable {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $stream = [System.IO.File]::OpenRead($Path)
    $reader = [System.IO.BinaryReader]::new($stream)
    try {
        # Validate the PE header as well as DOS 'MZ'; two magic bytes alone
        # do not establish a real Win64 executable.
        if ($stream.Length -lt 90) { return $false }
        if ($reader.ReadUInt16() -ne 0x5A4D) { return $false }
        $stream.Position = 0x3C
        $peOffset = [long]$reader.ReadInt32()
        if ($peOffset -lt 64 -or $peOffset -gt $stream.Length - 26) { return $false }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { return $false }
        if ($reader.ReadUInt16() -ne 0x8664) { return $false } # AMD64
        $stream.Position = $peOffset + 24
        return $reader.ReadUInt16() -eq 0x20B # PE32+ (64-bit)
    } finally {
        $reader.Dispose()
    }
}

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
if (-not (Test-WindowsExecutable -Path $GameExe)) {
    throw "Unreal build returned success but the game executable is empty or not a Windows PE file: $GameExe"
}
# Incremental builds may reuse an existing executable, but it must not
# predate C++/build-rule changes. Runtime .ini edits do not require relinking.
$ProjectRoot = Join-Path $PSScriptRoot "MoonfallPrototype"
$SourceRoot = Join-Path $ProjectRoot "Source"
$SourceFiles = @(Get-ChildItem -LiteralPath $SourceRoot -Recurse -File |
    Where-Object { $_.Extension -in @(".cpp", ".h", ".cs") })
$BuildInputs = @($SourceFiles) + @(Get-Item -LiteralPath $Project)
$LatestInputUtc = ($BuildInputs | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc
if ((Get-Item -LiteralPath $GameExe).LastWriteTimeUtc -lt $LatestInputUtc.AddSeconds(-2)) {
    throw "Win64 game executable is older than project sources or build rules. Rebuild MoonfallPrototype: $GameExe"
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
    if ((Get-Item -LiteralPath $MapAsset).Length -eq 0) {
        throw "Unreal Editor startup map is empty: $MapAsset"
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
    if ((Get-Item -LiteralPath $StartupMap).Length -eq 0) {
        throw "Configured startup map is empty: $StartupMap"
    }
    Write-Host "Verified configured startup map: $StartupMap"
    if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
        $ArchiveDirectory = Join-Path $PSScriptRoot "MoonfallPrototype\Saved\Win64Verification"
    }
    $ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
    # Isolate every verification run: Unreal may preserve original file timestamps
    # when copying, so mtime alone cannot establish whether a package is new.
    $RunArchiveDirectory = Join-Path $ArchiveDirectory ("run-" + [guid]::NewGuid().ToString("N"))
    if (Test-Path -LiteralPath $RunArchiveDirectory) {
        throw "Expected a new, empty archive directory: $RunArchiveDirectory"
    }
    & $RunUAT BuildCookRun "-project=$Project" "-map=/Game/$($ConfiguredMap.Groups[1].Value)" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$RunArchiveDirectory"
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal BuildCookRun failed (exit $LASTEXITCODE). No valid package was verified."
    }
    if (-not (Test-Path -LiteralPath $RunArchiveDirectory -PathType Container)) {
        throw "BuildCookRun returned success but the isolated archive directory does not exist: $RunArchiveDirectory"
    }
    $PackagedExecutables = @(Get-ChildItem -LiteralPath $RunArchiveDirectory -File -Recurse -Filter "MoonfallPrototype.exe" |
        Where-Object { Test-WindowsExecutable -Path $_.FullName })
    if ($PackagedExecutables.Count -eq 0) {
        throw "BuildCookRun returned success but no valid packaged MoonfallPrototype.exe was found in $RunArchiveDirectory"
    }
    # A valid launcher alone is not a game. Confirm that the freshly archived
    # build also contains nonempty cooked content (.pak or IoStore .ucas).
    $PackagedContent = @(Get-ChildItem -LiteralPath $RunArchiveDirectory -File -Recurse |
        Where-Object { $_.Extension -in @(".pak", ".ucas") -and $_.Length -gt 0 })
    if ($PackagedContent.Count -eq 0) {
        throw "BuildCookRun returned success but the Win64 archive has no nonempty cooked content (.pak or .ucas): $RunArchiveDirectory"
    }
    Write-Host "Verified isolated Win64 package executable: $($PackagedExecutables[0].FullName)"
    Write-Host "Verified cooked content files: $($PackagedContent.Count)"
    Write-Host "BuildCookRun completed. Manually launch and test the staged game on Windows 10 and 11."
}
