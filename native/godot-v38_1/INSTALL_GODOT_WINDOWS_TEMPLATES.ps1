param(
  [string]$Version = '4.7.2',
  [string]$GodotRoot = (Join-Path $PSScriptRoot 'Godot_4.7.2')
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$archiveSha256 = 'f298490b8d44d934be425a5a65a51bf15f422428b229a06a6e11d9ffea248011'
$sourceUrl = "https://github.com/godotengine/godot/releases/download/$Version-stable/Godot_v$Version-stable_export_templates.tpz"
$exportVersion = "$Version.stable"
$cacheDir = Join-Path $GodotRoot "export_templates\$exportVersion"
$cachedTemplate = Join-Path $cacheDir 'windows_release_x86_64.exe'
$cachedHash = Join-Path $cacheDir 'windows_release_x86_64.sha256'
$cachedDebug = Join-Path $cacheDir 'windows_debug_x86_64.exe'
$cachedDebugHash = Join-Path $cacheDir 'windows_debug_x86_64.sha256'
$installedDir = Join-Path $env:APPDATA "Godot\export_templates\$exportVersion"
$installedTemplate = Join-Path $installedDir 'windows_release_x86_64.exe'

# A cache hit must include both official Windows templates and their fingerprints.
foreach ($pair in @(@($cachedTemplate,$cachedHash), @($cachedDebug,$cachedDebugHash))) {
  $binary = $pair[0]
  $fingerprint = $pair[1]
  if ((Test-Path $binary) -and (Test-Path $fingerprint)) {
    $actualBinaryHash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant()
    $expectedBinaryHash = (Get-Content -LiteralPath $fingerprint -Raw).Trim().ToLowerInvariant()
    if ($actualBinaryHash -ne $expectedBinaryHash) { throw "Cached template digest mismatch: $binary" }
  } elseif ((Test-Path $binary) -or (Test-Path $fingerprint)) {
    throw "Incomplete verified template cache: $binary"
  }
}
if ((Test-Path $cachedTemplate) -xor (Test-Path $cachedDebug)) {
  throw 'Only one Windows template is cached; delete the cache and retry'
}
if (-not (Test-Path $cachedTemplate)) {
  New-Item -ItemType Directory -Path $cacheDir -Force | Out-Null
  $tempDirectory = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
  $archive = Join-Path $tempDirectory "moonfall-godot-templates-$Version.tpz"
  try {
    Write-Host 'Downloading official Godot export templates; first download is approximately 1.3 GB. CI caches the extracted Windows x64 template.'
    Invoke-WebRequest -Uri $sourceUrl -OutFile $archive
    $actual = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $archiveSha256) { throw "Export template SHA256 mismatch: $actual" }
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.ZipFile
    $zip = [System.IO.Compression.ZipFile]::OpenRead($archive)
    try {
      foreach ($item in @(@('templates/windows_release_x86_64.exe',$cachedTemplate,$cachedHash), @('templates/windows_debug_x86_64.exe',$cachedDebug,$cachedDebugHash))) {
        $entryName = $item[0]
        $target = $item[1]
        $hashTarget = $item[2]
        $entry = $zip.Entries | Where-Object { $_.FullName -eq $entryName } | Select-Object -First 1
        if ($null -eq $entry) { throw "Missing official template entry: $entryName" }
        $inputStream = $entry.Open()
        $outputStream = [System.IO.File]::Create($target)
        try { $inputStream.CopyTo($outputStream) } finally { $outputStream.Dispose(); $inputStream.Dispose() }
        $hash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
        Set-Content -LiteralPath $hashTarget -Value $hash -NoNewline
      }
    } finally { $zip.Dispose() }
  } finally {
    if (Test-Path $archive) { Remove-Item $archive -Force }
  }
} else {
  Write-Host 'Using previously SHA-verified Windows x64 export template from CI cache.'
}
foreach ($binary in @($cachedTemplate,$cachedDebug)) {
  if ((Get-Item $binary).Length -lt 1048576) { throw "Windows export template is unexpectedly small: $binary" }
  $stream = [IO.File]::OpenRead($binary)
  try {
    $header = New-Object byte[] 2
    if ($stream.Read($header,0,2) -ne 2 -or [Text.Encoding]::ASCII.GetString($header) -ne 'MZ') { throw "Windows template is not a PE executable: $binary" }
  } finally { $stream.Dispose() }
}
New-Item -ItemType Directory -Path $installedDir -Force | Out-Null
Copy-Item -LiteralPath $cachedTemplate -Destination $installedTemplate -Force
Copy-Item -LiteralPath $cachedDebug -Destination (Join-Path $installedDir 'windows_debug_x86_64.exe') -Force
if (-not (Test-Path $installedTemplate)) { throw 'Godot export template installation failed' }
Write-Host "Installed Godot Windows release export template: $installedTemplate"
