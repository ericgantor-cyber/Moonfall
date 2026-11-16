# Moonfall generated AI environment art installer. Windows PowerShell 5.1+.
# Install only the four new scene images, never original Godot or Z/Nyra files.
[CmdletBinding()]
param(
    [string]$ZipPath = (Join-Path (Get-Location).Path 'Moonfall_AI_Art_Ready_To_Import.zip'),
    [string]$RepositoryRoot = (Get-Location).Path
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$root = [IO.Path]::GetFullPath($RepositoryRoot)
$zip = [IO.Path]::GetFullPath($ZipPath)
$pinnedPath = Join-Path $root 'engine-comparison/GENERATED_ART_SHA256.json'
if (-not (Test-Path -LiteralPath $pinnedPath -PathType Leaf)) {
    throw "Not a Moonfall experimental checkout: $root"
}
if (-not (Test-Path -LiteralPath $zip -PathType Leaf)) {
    throw "Missing generated AI art ZIP: $zip"
}
$pinned = Get-Content -LiteralPath $pinnedPath -Raw | ConvertFrom-Json
$names = @('moonfall_ai_moonlit_gate.jpg', 'moonfall_ai_waterfall_ruins.jpg')
$folders = @(
    'engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt',
    'engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt'
)
$stage = New-Object System.Collections.ArrayList
$archive = [IO.Compression.ZipFile]::OpenRead($zip)
try {
    $manifestEntry = $archive.GetEntry('AI_ART_SHA256.json')
    if ($null -eq $manifestEntry) { throw 'Art ZIP missing checksum manifest' }
    $reader = New-Object IO.StreamReader($manifestEntry.Open())
    try { $checks = $reader.ReadToEnd() | ConvertFrom-Json }
    finally { $reader.Dispose() }
    if (@($checks.generated_art).Count -ne 2 -or @($pinned.generated_art).Count -ne 2) {
        throw 'Art ZIP or pinned manifest does not describe exactly two images'
    }
    foreach ($name in $names) {
        $entryChecks = @($checks.generated_art | Where-Object { $_.filename -eq $name })
        $entryPinned = @($pinned.generated_art | Where-Object { $_.filename -eq $name })
        if ($entryChecks.Count -ne 1 -or $entryPinned.Count -ne 1) {
            throw "Missing or duplicate manifest record: $name"
        }
        $expect = [string]$entryPinned[0].sha256
        if ($expect -cnotmatch '^[a-f0-9]{64}$') { throw "Invalid SHA256: $name" }
        if ([string]$entryChecks[0].sha256 -cne $expect -or
            [long]$entryChecks[0].bytes -ne [long]$entryPinned[0].bytes) {
            throw "Art ZIP differs from pinned manifest: $name"
        }
        foreach ($folder in $folders) {
            $archiveName = "$folder/$name"
            $entry = $archive.GetEntry($archiveName)
            if ($null -eq $entry) { throw "Art ZIP missing $archiveName" }
            $inputStream = $entry.Open()
            $buffer = New-Object IO.MemoryStream
            try { $inputStream.CopyTo($buffer); $data = $buffer.ToArray() }
            finally { $buffer.Dispose(); $inputStream.Dispose() }
            if ($data.Length -ne [long]$entryPinned[0].bytes) {
                throw "Wrong file size: $archiveName"
            }
            if ($data.Length -lt 3 -or $data[0] -ne 255 -or
                $data[1] -ne 216 -or $data[2] -ne 255) {
                throw "Invalid JPG signature: $archiveName"
            }
            $sha = [Security.Cryptography.SHA256]::Create()
            try {
                $actualHash = ([BitConverter]::ToString($sha.ComputeHash($data))).Replace('-', '').ToLowerInvariant()
            }
            finally { $sha.Dispose() }
            if ($actualHash -cne $expect) { throw "Checksum mismatch: $archiveName" }
            [void]$stage.Add(@{
                RelativePath = $archiveName
                Bytes = $data
            })
        }
    }
}
finally { $archive.Dispose() }
# Do not write anything until every image in the ZIP passes validation.
foreach ($asset in $stage) {
    $dest = Join-Path $root ([string]$asset.RelativePath)
    $destDir = Split-Path -Parent $dest
    if (-not (Test-Path -LiteralPath $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    [IO.File]::WriteAllBytes($dest, [byte[]]$asset.Bytes)
    Write-Host "Installed verified $($asset.RelativePath)"
}
Write-Host 'All four images installed; Unreal Editor Content import is still required.'
