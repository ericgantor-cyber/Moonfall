# Manual UE5.6 package step on a trusted Windows machine. Fails closed without a real cooked game map.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$UnrealRoot,
    [string]$OutputDirectory = '',
    [string]$MapName = 'MoonfallWorld'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'MoonfallPrototype.uproject'
$RunUAT = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$VersionPath = Join-Path $UnrealRoot 'Engine\Build\Build.version'
$MapAsset = Join-Path $ProjectRoot "Content\Maps\$MapName.umap"
$Generator = Join-Path $ProjectRoot 'Content\Python\generate_startup_world.py'
$EditorCmd = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

foreach ($required in @($Project, $RunUAT, $VersionPath)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing UE5.6 packaging prerequisite: $required"
    }
}
$Version = Get-Content -LiteralPath $VersionPath -Raw | ConvertFrom-Json
if ($Version.MajorVersion -ne 5 -or $Version.MinorVersion -ne 6) {
    throw "UE5.6 is required. Installed version: $($Version.MajorVersion).$($Version.MinorVersion)"
}
$ProjectData = Get-Content -LiteralPath $Project -Raw | ConvertFrom-Json
if ($ProjectData.EngineAssociation -ne '5.6') { throw 'The project is not associated with UE5.6.' }
if ($MapName -notmatch '^[A-Za-z][A-Za-z0-9_]*$') { throw 'Unsafe map asset name.' }
# Validate every package attempt, including a preexisting artist-authored level.
# The generator loads but does not save/modify existing assets.
foreach ($required in @($Generator, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Cannot validate startup level; missing: $required"
    }
}
$ExistingMapHash = $null
if (Test-Path -LiteralPath $MapAsset -PathType Leaf) {
    $ExistingMapHash = (Get-FileHash -LiteralPath $MapAsset -Algorithm SHA256).Hash
}
& $EditorCmd $Project "-ExecutePythonScript=$Generator" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) { throw "UE5.6 startup map generation/validation failed ($LASTEXITCODE)" }
if (-not (Test-Path -LiteralPath $MapAsset -PathType Leaf)) {
    throw "Unreal editor did not produce a startup level at $MapAsset"
}
if ($ExistingMapHash) {
    $UpdatedMapHash = (Get-FileHash -LiteralPath $MapAsset -Algorithm SHA256).Hash
    if ($UpdatedMapHash -ne $ExistingMapHash) {
        throw 'Existing artist-authored MoonfallWorld was changed during validation. Refusing packaging.'
    }
}

# Never claim an installer without a successful cook and a verified executable.
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $ProjectRoot 'Saved\WindowsPackage'
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$MapLongName = "/Game/Maps/$MapName"
$EngineConfigPath = Join-Path $ProjectRoot 'Config\DefaultEngine.ini'
if (-not (Test-Path -LiteralPath $EngineConfigPath -PathType Leaf)) {
    throw 'Config/DefaultEngine.ini is missing.'
}
$EngineConfig = Get-Content -LiteralPath $EngineConfigPath -Raw
$StartMapLine = "GameDefaultMap=$MapLongName"
if (-not (($EngineConfig -split "\r?\n") -contains $StartMapLine)) {
    throw "Set $StartMapLine in DefaultEngine.ini before packaging. A build without a startup level is not playable."
}

$Arguments = @(
    'BuildCookRun',
    "-project=$Project",
    '-noP4',
    '-platform=Win64',
    '-clientconfig=Development',
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-archive',
    "-archivedirectory=$OutputDirectory",
    "-map=$MapLongName",
    '-utf8output'
)
& $RunUAT @Arguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal AutomationTool BuildCookRun failed ($LASTEXITCODE). No Windows artifact should be published."
}
$Exe = Get-ChildItem -LiteralPath $OutputDirectory -Filter 'MoonfallPrototype.exe' -Recurse -File |
    Select-Object -First 1
if (-not $Exe) { throw 'Packaging finished without MoonfallPrototype.exe. Refusing to mark build ready.' }
Write-Host "Cooked Windows package built: $($Exe.FullName)"
Write-Host 'This is not an installer and has not passed a real gameplay smoke test.'
