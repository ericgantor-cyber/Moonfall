# Creates a local diagnostic ZIP. Never reads saves, credentials or personal files.
param()
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$folder = Join-Path $root 'build'
New-Item -Path $folder -ItemType Directory -Force | Out-Null
$temp = Join-Path ([IO.Path]::GetTempPath()) ('moonfall-diagnostics-' + [guid]::NewGuid().ToString('N'))
$zip = Join-Path $folder 'Moonfall_v38_1_Build_Diagnostics.zip'
try {
  New-Item -Path $temp -ItemType Directory -Force | Out-Null
  $logs = @(Get-ChildItem -Path $root -Filter 'GODOT_*_LOG.txt' -File -ErrorAction SilentlyContinue)
  foreach ($file in $logs) {Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $temp $file.Name)}
  foreach ($relative in @('build/BUILD_FAILURE_v38_1.txt','build/BUILD_STATUS_v38_1.json',
      'TEST_REPORT_v38.md','README_V38_1_RELEASE_GATE.md')) {
    $file = Join-Path $root $relative
    if (Test-Path -LiteralPath $file -PathType Leaf) {
      Copy-Item -LiteralPath $file -Destination (Join-Path $temp ([IO.Path]::GetFileName($file)))
    }
  }
  @'
Moonfall v38.1 build diagnostics
These are source-build and Godot engine logs only. No game saves or user credentials
are collected. Review the files yourself before sharing: log messages may contain
local Windows paths or usernames.
If GODOT_*_LOG.txt is absent, the failure happened before Godot started.
'@ | Set-Content -LiteralPath (Join-Path $temp 'ABOUT_DIAGNOSTICS.txt') -Encoding UTF8
  if (Test-Path -LiteralPath $zip) {Remove-Item -LiteralPath $zip -Force}
  Compress-Archive -LiteralPath (Get-ChildItem -LiteralPath $temp -File | ForEach-Object FullName) -DestinationPath $zip -CompressionLevel Optimal
  Write-Host "Diagnostics saved: $zip" -ForegroundColor Green
} finally {
  if (Test-Path -LiteralPath $temp) {Remove-Item -LiteralPath $temp -Recurse -Force}
}
