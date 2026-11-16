param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [string]$OutputDirectory = "",
    [switch]$CompileOnly,
    [switch]$PreflightOnly,
    [switch]$CompilePreflightOnly,
    [switch]$LaunchSmokeTest
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$project = Join-Path $repoRoot 'MoonfallUnreal\MoonfallUnreal.uproject'
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repoRoot 'MoonfallUnrealLocalBuild' }
if ([string]::IsNullOrWhiteSpace($UnrealRoot)) {
    throw 'Unreal Engine 5.4 path missing. Supply -UnrealRoot "C:\Program Files\Epic Games\UE_5.4" or set UE54_ROOT.'
}
$build = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $project)) { throw "Unreal project file missing: $project" }
if (-not (Test-Path -LiteralPath $build)) { throw "Unreal Build.bat missing: $build" }
# Fail early when a newer/older installation is accidentally supplied as UE54_ROOT.
& (Join-Path $PSScriptRoot 'Test-UnrealEngineVersion.ps1') -UnrealRoot $UnrealRoot
$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio 2022 C++ tools not detected. Install Visual Studio Build Tools with Desktop development with C++.'
}
$vsInstall = @(& $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if ($LASTEXITCODE -ne 0 -or -not ($vsInstall | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })) {
    throw 'Visual Studio 2022 C++ toolchain is missing. Install the Desktop development with C++ workload and Windows SDK.'
}
Write-Host "Detected Visual Studio C++ toolchain: $($vsInstall | Select-Object -First 1)"

# The release workflow already creates this asset. First-time local packaging
# reuses the same editor generator without overwriting a valid existing map.
$editorCompiledForStartupMap = $false
$startupMap = Join-Path $repoRoot 'MoonfallUnreal\Content\Maps\MoonfallStart.umap'
$engineConfig = Join-Path $repoRoot 'MoonfallUnreal\Config\DefaultEngine.ini'
if (-not $CompileOnly -and -not $CompilePreflightOnly -and -not $PreflightOnly) {
    $hasSavedMap = Test-Path -LiteralPath $startupMap -PathType Leaf
    $configIsReady = $false
    if (Test-Path -LiteralPath $engineConfig -PathType Leaf) {
        $configTextBefore = Get-Content -LiteralPath $engineConfig -Raw
        $configIsReady = ($configTextBefore -match '(?m)^\s*GameDefaultMap=/Game/Maps/MoonfallStart\s*$') -and
            ($configTextBefore -match '(?m)^\s*EditorStartupMap=/Game/Maps/MoonfallStart\s*$')
    }
    if (-not $hasSavedMap -or -not $configIsReady) {
        Write-Host 'Preparing Moonfall startup map with Unreal Editor...'
        & $build MoonfallUnrealEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload
        if ($LASTEXITCODE -ne 0) { throw "Unreal Editor compilation failed before map generation: $LASTEXITCODE" }
        $editorCompiledForStartupMap = $true
        $editor = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        $generator = Join-Path $PSScriptRoot 'Create-StartupLevel.py'
        if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw "UnrealEditor-Cmd.exe missing: $editor" }
        if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) { throw "Startup-level generator missing: $generator" }
        & $editor $project "-ExecutePythonScript=$generator" -unattended -nop4
        if ($LASTEXITCODE -ne 0) { throw "Startup map generation failed: $LASTEXITCODE" }
        if (-not (Test-Path -LiteralPath $startupMap -PathType Leaf)) {
            throw 'Unreal Editor did not create MoonfallStart.umap; inspect its log.'
        }
    }
}


# A playable package needs a real project-owned level, not the engine's
# generic /Engine/Maps/Entry placeholder. Compile-only remains possible.
if (-not $CompileOnly -and -not $CompilePreflightOnly) {
    $startupMap = Join-Path $repoRoot 'MoonfallUnreal\Content\Maps\MoonfallStart.umap'
    $engineConfig = Join-Path $repoRoot 'MoonfallUnreal\Config\DefaultEngine.ini'
    if (-not (Test-Path -LiteralPath $startupMap -PathType Leaf)) {
        throw 'Playable startup level missing: MoonfallUnreal/Content/Maps/MoonfallStart.umap. Create and save this level in Unreal Editor before packaging.'
    }
    if ((Get-Item -LiteralPath $startupMap).Length -le 0) {
        throw 'MoonfallStart.umap is empty. Save the level in Unreal Editor before packaging.'
    }
    if (-not (Test-Path -LiteralPath $engineConfig -PathType Leaf)) {
        throw "Unreal engine configuration missing: $engineConfig"
    }
    $engineConfigText = Get-Content -LiteralPath $engineConfig -Raw
    $mapSectionMatches = @([regex]::Matches($engineConfigText, '(?m)^\s*\[/Script/EngineSettings.GameMapsSettings\]\s*$'))
    if ($mapSectionMatches.Count -ne 1) {
        throw 'Expected exactly one GameMapsSettings section in DefaultEngine.ini.'
    }
    $mapSection = ($engineConfigText -split '(?m)^\s*\[/Script/EngineSettings.GameMapsSettings\]\s*$', 2)[1]
    $mapSection = ($mapSection -split '(?m)^\s*\[[^\r\n]+\]\s*$', 2)[0]
    foreach ($mapKey in @('GameDefaultMap', 'EditorStartupMap')) {
        $matchingEntries = @([regex]::Matches($mapSection, "(?m)^\s*$mapKey\s*=\s*(.*?)\s*$"))
        if ($matchingEntries.Count -ne 1 -or
            $matchingEntries[0].Groups[1].Value.Trim() -ne '/Game/Maps/MoonfallStart') {
            throw "Set exactly one $mapKey=/Game/Maps/MoonfallStart in DefaultEngine.ini before packaging."
        }
    }
    $modeMatches = @([regex]::Matches($mapSection, '(?m)^\s*GlobalDefaultGameMode\s*=\s*(.*?)\s*$'))
    if ($modeMatches.Count -ne 1 -or
        $modeMatches[0].Groups[1].Value.Trim() -ne '/Script/MoonfallUnreal.MoonfallUnrealGameMode') {
        throw 'Set GlobalDefaultGameMode=/Script/MoonfallUnreal.MoonfallUnrealGameMode before packaging.'
    }

}

# Packaging preflight must check tools as well as maps. Compile-only stays independent
# of packaging dependencies; preflight-only performs no compile or file deletion.
if (-not $CompileOnly -and -not $CompilePreflightOnly) {
    $uat = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
    if (-not (Test-Path -LiteralPath $uat)) { throw "Unreal packaging tool missing: $uat" }
    foreach ($command in @('dotnet','makensis.exe')) {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
            throw "Packaging prerequisite not on PATH: $command"
        }
    }
    $sdks = @(& dotnet --list-sdks)
    if (-not ($sdks | Where-Object { $_ -match '^8\.' })) { throw 'The .NET 8 SDK is required to package the launcher.' }
    $OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
    $gameOutput = Join-Path $OutputDirectory 'Game'
    $repoRootFull = [IO.Path]::GetFullPath($repoRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
    if ($OutputDirectory.TrimEnd([IO.Path]::DirectorySeparatorChar) -eq $repoRootFull) {
        throw 'OutputDirectory cannot be the repository root. Choose a dedicated build-output folder.'
    }

}

if ($CompilePreflightOnly) {
    Write-Host 'Moonfall compile preflight passed: UE 5.4 and VS C++ toolchain checked. No map, packaging tools or compilation required.'
    exit 0
}
if ($PreflightOnly) {
    Write-Host 'Moonfall packaging preflight passed: engine, compiler, map, .NET 8 and NSIS checked. Compilation and packaging were not run.'
    exit 0
}

if (-not $editorCompiledForStartupMap) {
    Write-Host 'Compiling Unreal 5.4 Editor target...'
    & $build MoonfallUnrealEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload
    if ($LASTEXITCODE -ne 0) { throw "Unreal editor compilation failed: $LASTEXITCODE" }
} else {
    Write-Host 'Editor compilation already succeeded during startup map generation.'
}

if ($CompileOnly) {
    Write-Host 'Unreal Editor compilation succeeded. Compile-only mode: skipping packaging, launcher and NSIS.'
    exit 0
}

# Only clear previous installer and cooked output after Editor compilation succeeds.
# Never recursively delete an arbitrary existing 'Game' folder.
# Require a dedicated marker created by this script on an earlier run.
$marker = Join-Path $gameOutput '.moonfall-managed-output'
if (Test-Path -LiteralPath $gameOutput) {
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw "Refusing to delete unmarked folder: $gameOutput. Choose a new -OutputDirectory, or move this folder aside manually."
    }
    Write-Host "Removing previously managed packaged game output: $gameOutput"
    Remove-Item -LiteralPath $gameOutput -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
# Preserve the last successfully built installer and checksum until a new
# complete installer has passed hashing and the atomic promotion below.
# Failed cooking, launcher compilation, or NSIS must not delete a valid release.
New-Item -ItemType Directory -Force -Path $gameOutput | Out-Null
Set-Content -LiteralPath $marker -Value 'Managed by Moonfall Build-Windows.ps1. This directory can be replaced by a later build.' -Encoding ascii

Write-Host 'Packaging Unreal Win64 Development game...'
# Explicitly cook the same project-owned map validated above. A successful C++ build alone
# does not establish that the executable can load its intended startup level.
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook "-map=/Game/Maps/MoonfallStart" -stage -pak -prereqs -archive "-archivedirectory=$gameOutput" -utf8output
if ($LASTEXITCODE -ne 0) { throw "Unreal packaging failed: $LASTEXITCODE" }

$games = @(Get-ChildItem $gameOutput -Filter 'MoonfallUnreal.exe' -Recurse -File)
if ($games.Count -ne 1) { throw "Expected one game executable, found $($games.Count)" }
$gameRoot = $games[0].Directory.FullName
foreach ($dir in @('Engine','MoonfallUnreal')) {
    if (-not (Test-Path (Join-Path $gameRoot $dir))) { throw "Packaged game is missing $dir" }
}
if (-not (Get-ChildItem (Join-Path $gameRoot 'MoonfallUnreal') -Recurse -File | Select-Object -First 1)) {
    throw 'Cooked Moonfall game content is empty; refusing incomplete installer.'
}
if ($LaunchSmokeTest) {
    & (Join-Path $PSScriptRoot 'Test-PackagedGame.ps1') -GameExecutable $games[0].FullName
}
Write-Host 'Building self-contained Windows launcher...'
$launcherProject = Join-Path $repoRoot 'MoonfallUnreal\Launcher\MoonfallLauncher.csproj'
& dotnet publish $launcherProject -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:PublishTrimmed=false "-o=$gameRoot"
if ($LASTEXITCODE -ne 0) { throw "Launcher compilation failed: $LASTEXITCODE" }
$launcher = Join-Path $gameRoot 'MoonfallLauncher.exe'
if (-not (Test-Path $launcher)) { throw 'Launcher executable not found' }
$check = Start-Process -FilePath $launcher -ArgumentList '--check-install' -WorkingDirectory $gameRoot -Wait -PassThru
if ($check.ExitCode -ne 0) { throw "Launcher game-file check failed: $($check.ExitCode)" }

Write-Host 'Building NSIS Windows installer...'
$setup = Join-Path $OutputDirectory 'MoonfallUnreal-Setup.pending.exe'
$script = Join-Path $repoRoot 'MoonfallUnreal\Installer\MoonfallUnreal.nsi'
& makensis.exe "/DSOURCE_DIR=$gameRoot" "/DOUTPUT_EXE=$setup" $script
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $setup)) { throw 'NSIS build failed' }
$installerFile = Get-Item -LiteralPath $setup
if ($installerFile.Length -le 0) { throw 'NSIS returned an empty installer' }
$hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  MoonfallUnreal-Setup.exe" | Set-Content -Encoding ascii -Path "$setup.sha256"
$finalSetup = Join-Path $OutputDirectory 'MoonfallUnreal-Setup.exe'
$finalChecksum = "$finalSetup.sha256"
$backupSetup = "$finalSetup.previous"
$backupChecksum = "$finalChecksum.previous"
# Fail closed if a previous interrupted promotion left backups to recover.
if ((Test-Path -LiteralPath $backupSetup) -or (Test-Path -LiteralPath $backupChecksum)) {
    throw "Previous installer promotion backup exists. Review $backupSetup and $backupChecksum before retrying."
}
$hadSetup = Test-Path -LiteralPath $finalSetup
$hadChecksum = Test-Path -LiteralPath $finalChecksum
try {
    if ($hadSetup) { Move-Item -LiteralPath $finalSetup -Destination $backupSetup }
    if ($hadChecksum) { Move-Item -LiteralPath $finalChecksum -Destination $backupChecksum }
    Move-Item -LiteralPath $setup -Destination $finalSetup
    Move-Item -LiteralPath "$setup.sha256" -Destination $finalChecksum
    $promotedHash = (Get-FileHash -LiteralPath $finalSetup -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($promotedHash -ne $hash) { throw 'Promoted installer SHA256 mismatch.' }
    $checksumLine = (Get-Content -LiteralPath $finalChecksum -TotalCount 1).Trim()
    if ($checksumLine -ne "$hash  MoonfallUnreal-Setup.exe") { throw 'Promoted checksum manifest mismatch.' }
} catch {
    Remove-Item -LiteralPath $finalSetup -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $finalChecksum -Force -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $backupSetup) { Move-Item -LiteralPath $backupSetup -Destination $finalSetup }
    if (Test-Path -LiteralPath $backupChecksum) { Move-Item -LiteralPath $backupChecksum -Destination $finalChecksum }
    throw
}
if (Test-Path -LiteralPath $backupSetup) { Remove-Item -LiteralPath $backupSetup -Force }
if (Test-Path -LiteralPath $backupChecksum) { Remove-Item -LiteralPath $backupChecksum -Force }
Write-Host "Installer generated: $finalSetup"
Write-Host "Installer size: $($installerFile.Length) bytes"
Write-Host "Installer SHA-256: $hash"
Write-Host "Checksum file: $finalSetup.sha256"
Write-Warning 'A generated installer is NOT proof of working gameplay. Test installation, game launch and uninstall on Windows before sharing.'
