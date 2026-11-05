# Install Moonfall's two new AI-generated scene images from its verified ZIP.
# Run from repository root or supply -RepositoryRoot. Does not touch production Godot.
[CmdletBinding()]
param(
    [string]$ZipPath = (Join-Path (Get-Location).Path "Moonfall_AI_Art_Ready_To_Import.zip"),
    [string]$RepositoryRoot = (Get-Location).Path
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
$root = [IO.Path]::GetFullPath($RepositoryRoot)
$zip = [IO.Path]::GetFullPath($ZipPath)
if (-not (Test-Path -LiteralPath (Join-Path $root "engine-comparison"))) { throw "Repository root is not a Moonfall checkout: $root" }
if (-not (Test-Path -LiteralPath $zip -PathType Leaf)) { throw "Missing art ZIP: $zip" }
$names = @('moonfall_ai_moonlit_gate.jpg', 'moonfall_ai_waterfall_ruins.jpg')
$targets = @(
    'engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt',
    'engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt'
)
$archive = [IO.Compression.ZipFile]::OpenRead($zip)
try {
    $manifest = $archive.GetEntry('AI_ART_SHA256.json')
    if ($null -eq $manifest) { throw 'Archive has no SHA-256 manifest' }
    $reader = New-Object IO.StreamReader($manifest.Open())
    try { $checks = ($reader.ReadToEnd() | ConvertFrom-Json) }
    finally { $reader.Dispose() }
    foreach ($name in $names) {
        $expect = $checks.$name
        if (-not $expect) { throw "Missing checksum for $name" }
        foreach ($relative in $targets) {
            $archiveName = "$relative/$name"
            $entry = $archive.GetEntry($archiveName)
            if ($null -eq $entry) { throw "Missing asset entry: $archiveName" }
            # Stage and validate BEFORE replacing files in the project.
            $temp = [IO.Path]::GetTempFileName()
            try {
                $inputStream = $entry.Open()
                $outputStream = [IO.File]::Create($temp)
                try { $inputStream.CopyTo($outputStream) }
                finally { $outputStream.Dispose(); $inputStream.Dispose() }
                $digest = (Get-FileHash -LiteralPath $temp -Algorithm SHA256).Hash.ToLowerInvariant()
                if ($digest -ne ([string]$expect).ToLowerInvariant()) {
                    throw "Checksum mismatch: $archiveName"
                }
                $destinationDir = Join-Path $root $relative
                New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
                $destination = Join-Path $destinationDir $name
                Copy-Item -LiteralPath $temp -Destination $destination -Force
                Write-Host "Installed and verified $archiveName"
            }
            finally { Remove-Item -LiteralPath $temp -ErrorAction SilentlyContinue }
        }
    }
}
finally { $archive.Dispose() }
Write-Host "Done. Unity Resources is ready. Unreal Editor still needs its Python art import."
