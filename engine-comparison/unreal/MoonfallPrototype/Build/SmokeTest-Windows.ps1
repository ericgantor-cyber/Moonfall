# Headless boot smoke test only. A passing result does not establish playable gameplay.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$PackageDirectory,
    [int]$BootSeconds = 25
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($BootSeconds -lt 10 -or $BootSeconds -gt 120) {
    throw 'BootSeconds must be between 10 and 120 seconds.'
}
$PackageDirectory = (Resolve-Path -LiteralPath $PackageDirectory -ErrorAction Stop).Path
$ManifestPath = Join-Path $PackageDirectory 'MoonfallPackageManifest.json'
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw 'Missing package integrity manifest; refusing a boot test against unknown files.'
}
$Manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
if ($Manifest.project -ne 'MoonfallPrototype' -or $Manifest.engine -ne '5.6' -or
    $Manifest.tested_in_game -ne $false -or $Manifest.installer -ne $false) {
    throw 'Package integrity manifest is invalid.'
}
# Verify every archived file against the packager's SHA256 inventory before launch.
foreach ($Entry in @($Manifest.files)) {
    $RelativePath = [string]$Entry.path
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or
        [System.IO.Path]::IsPathRooted($RelativePath) -or
        $RelativePath -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "Invalid manifest file path: $RelativePath"
    }
    $Target = Join-Path $PackageDirectory $RelativePath
    if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) {
        throw "Missing packaged file: $RelativePath"
    }
    $Item = Get-Item -LiteralPath $Target
    if ($Item.Length -ne [long]$Entry.bytes) {
        throw "Packaged file length mismatch: $RelativePath"
    }
    $Sha = (Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($Sha -ne [string]$Entry.sha256) {
        throw "Packaged file SHA256 mismatch: $RelativePath"
    }
}
$Files = @(Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File -Filter 'MoonfallPrototype*.exe')
$Exe = $Files | Where-Object { $_.Name -match '^MoonfallPrototype-Win64-Development\.exe$' -and $_.Length -gt 0 } |
    Select-Object -First 1
if (-not $Exe) {
    $Exe = $Files | Where-Object { $_.Name -eq 'MoonfallPrototype.exe' -and $_.Length -gt 0 } |
        Select-Object -First 1
}
if (-not $Exe) { throw 'No nonempty MoonfallPrototype Windows executable in the package.' }

$SmokeDir = Join-Path $PackageDirectory 'SmokeDiagnostics'
New-Item -ItemType Directory -Force -Path $SmokeDir | Out-Null
$LogPath = Join-Path $SmokeDir 'MoonfallBoot.log'
if (Test-Path -LiteralPath $LogPath) {
    Remove-Item -LiteralPath $LogPath -Force
}
$Process = $null
try {
    $Args = @('-nullrhi', '-unattended', '-nosplash', "-abslog=$LogPath")
    $Process = Start-Process -FilePath $Exe.FullName -ArgumentList $Args -PassThru -WorkingDirectory $Exe.DirectoryName
    Start-Sleep -Seconds $BootSeconds
    $Process.Refresh()
    if ($Process.HasExited) {
        throw "Moonfall exited before $BootSeconds-second boot check (exit code $($Process.ExitCode))."
    }
    if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
        throw 'Moonfall did not write its expected Unreal boot log.'
    }
    $Log = Get-Content -LiteralPath $LogPath -Raw
    if ($Log -notmatch '(?i)(/Game/Maps/MoonfallWorld|MoonfallWorld)') {
        throw 'Boot log does not confirm the MoonfallWorld startup map.'
    }
    if ($Log -notmatch 'MOONFALL_BOOT_READY pawn=MoonfallPawn regions=6 chambers=30') {
        throw 'The Moonfall gameplay pawn did not confirm continuous-world initialization.'
    }
    if ($Log -match '(?i)(Fatal error:|Unhandled Exception:|Assertion failed:|LogWindows: Error:)') {
        throw 'Unreal boot log reported a fatal error.'
    }
    Write-Host "Headless UE5.6 boot smoke passed for $BootSeconds seconds."
    Write-Host "Log: $LogPath"
    Write-Host 'This is not a display, input, quest, save-load, audio or performance test.'
}
finally {
    if ($Process) {
        $Process.Refresh()
        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
            $Process.WaitForExit(5000) | Out-Null
        }
        $Process.Dispose()
    }
}
