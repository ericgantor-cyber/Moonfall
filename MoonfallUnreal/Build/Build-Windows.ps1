param(
    [string]$UnrealRoot = $env:UE54_ROOT,
    [string]$OutputDirectory = ""
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
$uat = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$entryMap = Join-Path $UnrealRoot 'Engine\Content\Maps\Entry.umap'
foreach ($file in @($project,$build,$uat,$entryMap)) {
    if (-not (Test-Path $file)) { throw "Required file not found: $file" }
}
foreach ($command in @('dotnet','makensis.exe')) {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) { throw "Required tool is not on PATH: $command" }
}
$sdks = @(& dotnet --list-sdks)
if (-not ($sdks | Where-Object { $_ -match '^8\.' })) { throw 'The .NET 8 SDK is required.' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$gameOutput = Join-Path $OutputDirectory 'Game'
$repoRootFull = [IO.Path]::GetFullPath($repoRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
if ($OutputDirectory.TrimEnd([IO.Path]::DirectorySeparatorChar) -eq $repoRootFull) {
    throw 'OutputDirectory cannot be the repository root. Choose a dedicated build-output folder.'
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$previousSetup = Join-Path $OutputDirectory 'MoonfallUnreal-Setup.exe'
foreach ($old in @($previousSetup, "$previousSetup.sha256")) {
    if (Test-Path -LiteralPath $old) { Remove-Item -LiteralPath $old -Force }
}
# Do not package stale cooked assets from a previous run if the current build fails.
if (Test-Path -LiteralPath $gameOutput) {
    Write-Host "Removing previous packaged game output: $gameOutput"
    Remove-Item -LiteralPath $gameOutput -Recurse -Force
}

Write-Host 'Compiling Unreal 5.4 Editor target...'
& $build MoonfallUnrealEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload
if ($LASTEXITCODE -ne 0) { throw "Unreal editor compilation failed: $LASTEXITCODE" }

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
