# Creates an UNSIGNED installer from a real, hash-verified UE5.6 cooked game.
# Requires Windows and Inno Setup 6. Installation and gameplay remain untested.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$PackageDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$InnoCompiler = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Windows is required.' }
$PackageDirectory = (Resolve-Path -LiteralPath $PackageDirectory -ErrorAction Stop).Path
$ManifestPath = Join-Path $PackageDirectory 'MoonfallPackageManifest.json'
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) { throw 'Cooked manifest missing.' }
$Manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
if ($Manifest.project -ne 'MoonfallPrototype' -or $Manifest.engine -ne '5.6' -or
    $Manifest.installer -ne $false -or $Manifest.startup_map -ne '/Game/Maps/MoonfallWorld') {
    throw 'Invalid Moonfall cooked package manifest.'
}
$Entries = @($Manifest.files)
if ($Entries.Count -eq 0) { throw 'Empty cooked package manifest.' }
$Recorded = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($Entry in $Entries) {
    $Relative = ([string]$Entry.path).Replace('\', '/')
    if ([string]::IsNullOrWhiteSpace($Relative) -or [IO.Path]::IsPathRooted($Relative) -or
        $Relative -match '(^|/)\.\.(/|$)' -or -not $Recorded.Add($Relative)) {
        throw "Unsafe or duplicate package path: $Relative"
    }
    $Target = Join-Path $PackageDirectory $Relative
    if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) { throw "Missing: $Relative" }
    $File = Get-Item -LiteralPath $Target
    if ($File.Length -ne [long]$Entry.bytes -or
        (Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash.ToLowerInvariant() -ne [string]$Entry.sha256) {
        throw "File hash or size mismatch: $Relative"
    }
}
$Actual = @(Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File |
    Where-Object { $_.FullName -ne $ManifestPath })
if ($Actual.Count -ne $Recorded.Count) { throw 'Unexpected/unlisted cooked package files.' }
foreach ($File in $Actual) {
    $Relative = $File.FullName.Substring($PackageDirectory.Length).TrimStart('\', '/').Replace('\', '/')
    if (-not $Recorded.Contains($Relative)) { throw "Unlisted file: $Relative" }
}
$GameExe = $Actual | Where-Object { $_.Name -eq 'MoonfallPrototype.exe' -and $_.Length -gt 0 } | Select-Object -First 1
if (-not $GameExe) { throw 'Missing nonempty packaged game executable.' }
$CookedContainers = @($Actual | Where-Object { $_.Extension -in @('.pak', '.utoc', '.ucas') -and $_.Length -gt 0 })
if ($CookedContainers.Count -eq 0) { throw 'Missing cooked game data.' }
if ([string]::IsNullOrWhiteSpace($InnoCompiler)) {
    $InnoCompiler = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Inno Setup 6\ISCC.exe'
}
if (-not (Test-Path -LiteralPath $InnoCompiler -PathType Leaf)) {
    throw "Install Inno Setup 6 first: $InnoCompiler"
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$Prefix = $PackageDirectory.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
if ($OutputDirectory.Equals($PackageDirectory, [StringComparison]::OrdinalIgnoreCase) -or
    $OutputDirectory.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Output must be outside the cooked package.'
}
if (Test-Path -LiteralPath $OutputDirectory) {
    if (@(Get-ChildItem -LiteralPath $OutputDirectory -Force).Count -gt 0) {
        throw 'Output must be empty; stale installers are not accepted.'
    }
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$ExeRelative = $GameExe.FullName.Substring($PackageDirectory.Length).TrimStart('\', '/')
foreach ($Value in @($PackageDirectory, $OutputDirectory, $ExeRelative)) {
    if ($Value -match '["\r\n]') { throw 'Unsafe installer path.' }
}
$Newline = [Environment]::NewLine
$Preamble = ('#define GameDir "{0}"' -f $PackageDirectory) + $Newline +
    ('#define GameExeRel "{0}"' -f $ExeRelative) + $Newline +
    ('#define InstallerOut "{0}"' -f $OutputDirectory) + $Newline
$Inno = @'
[Setup]
AppId={{C7AB23A2-EBD4-46E4-957C-3A0CA325DC7B}
AppName=Moonfall Prototype
AppVersion=0.1.0
DefaultDirName={autopf}\Moonfall Prototype
DefaultGroupName=Moonfall Prototype
OutputDir={#InstallerOut}
OutputBaseFilename=MoonfallPrototype-UE56-Win64-UNTESTED-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\{#GameExeRel}

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Files]
Source: "{#GameDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Moonfall Prototype"; Filename: "{app}\{#GameExeRel}"
Name: "{autodesktop}\Moonfall Prototype"; Filename: "{app}\{#GameExeRel}"; Tasks: desktopicon
'@
$IssFile = Join-Path ([IO.Path]::GetTempPath()) ('MoonfallInstaller-' + [guid]::NewGuid().ToString('N') + '.iss')
try {
    [IO.File]::WriteAllText($IssFile, $Preamble + $Inno, [Text.UTF8Encoding]::new($false))
    & $InnoCompiler '/Qp' $IssFile
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup compiler failed: $LASTEXITCODE" }
}
finally { Remove-Item -LiteralPath $IssFile -Force -ErrorAction SilentlyContinue }
$Installers = @(Get-ChildItem -LiteralPath $OutputDirectory -Filter '*-Setup.exe' -File)
if ($Installers.Count -ne 1 -or $Installers[0].Length -le 0) {
    throw 'Expected one new nonempty setup executable.'
}
$Status = [PSCustomObject]@{
    source_manifest_sha256 = (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    installer_sha256 = (Get-FileHash -LiteralPath $Installers[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    signed = $false
    installation_tested = $false
    gameplay_tested = $false
}
$Status | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'InstallerBuildStatus.json') -Encoding UTF8
Write-Host "Unsigned, untested setup created: $($Installers[0].FullName)"
