param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [string]$OutputDirectory = "",
    [switch]$CompileOnly,
    [switch]$PreflightOnly
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
$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio 2022 C++ tools not detected. Install Visual Studio Build Tools with Desktop development with C++.'
}
$vsInstall = @(& $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if ($LASTEXITCODE -ne 0 -or -not ($vsInstall | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })) {
    throw 'Visual Studio 2022 C++ toolchain is missing. Install the Desktop development with C++ workload and Windows SDK.'
}
Write-Host "Detected Visual Studio C++ toolchain: $($vsInstall | Select-Object -First 1)"

# A compiler smoke test must not depend on packaging tools, cooked maps, or output folders.
if (-not $CompileOnly -and -not $PreflightOnly) {
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

if ($PreflightOnly) {
    Write-Host 'Moonfall build preflight passed. Compilation and packaging were not run.'
    exit 0
}

Write-Host 'Compiling Unreal 5.4 Editor target...'
& $build MoonfallUnrealEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload
if ($LASTEXITCODE -ne 0) { throw "Unreal editor compilation failed: $LASTEXITCODE" }

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
$previousSetup = Join-Path $OutputDirectory 'MoonfallUnreal-Setup.exe'
foreach ($old in @($previousSetup, "$previousSetup.sha256")) {
    if (Test-Path -LiteralPath $old) { Remove-Item -LiteralPath $old -Force }
}
New-Item -ItemType Directory -Force -Path $gameOutput | Out-Null
Set-Content -LiteralPath $marker -Value 'Managed by Moonfall Build-Windows.ps1. This directory can be replaced by a later build.' -Encoding ascii

Write-Host 'Packaging Unreal Win64 Development game...'
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -prereqs -archive "-archivedirectory=$gameOutput" -utf8output
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
Write-Host 'Building self-contained Windows launcher...'
$launcherProject = Join-Path $repoRoot 'MoonfallUnreal\Launcher\MoonfallLauncher.csproj'
& dotnet publish $launcherProject -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:PublishTrimmed=false "-o=$gameRoot"
if ($LASTEXITCODE -ne 0) { throw "Launcher compilation failed: $LASTEXITCODE" }
$launcher = Join-Path $gameRoot 'MoonfallLauncher.exe'
if (-not (Test-Path $launcher)) { throw 'Launcher executable not found' }
$check = Start-Process -FilePath $launcher -ArgumentList '--check-install' -WorkingDirectory $gameRoot -Wait -PassThru
if ($check.ExitCode -ne 0) { throw "Launcher game-file check failed: $($check.ExitCode)" }

Write-Host 'Building NSIS Windows installer...'
$setup = Join-Path $OutputDirectory 'MoonfallUnreal-Setup.exe'
$script = Join-Path $repoRoot 'MoonfallUnreal\Installer\MoonfallUnreal.nsi'
& makensis.exe "/DSOURCE_DIR=$gameRoot" "/DOUTPUT_EXE=$setup" $script
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $setup)) { throw 'NSIS build failed' }
$installerFile = Get-Item -LiteralPath $setup
if ($installerFile.Length -le 0) { throw 'NSIS returned an empty installer' }
$hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  MoonfallUnreal-Setup.exe" | Set-Content -Encoding ascii -Path "$setup.sha256"
Write-Host "Installer generated: $setup"
Write-Host "Installer size: $($installerFile.Length) bytes"
Write-Host "Installer SHA-256: $hash"
Write-Host "Checksum file: $setup.sha256"
Write-Warning 'A generated installer is NOT proof of working gameplay. Test installation, game launch and uninstall on Windows before sharing.'
