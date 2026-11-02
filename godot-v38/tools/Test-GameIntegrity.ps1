param([Parameter(Mandatory=$true)][string]$GameExe,[Parameter(Mandatory=$true)][string]$Manifest)
$ErrorActionPreference = 'Stop'
function Test-MoonfallIntegrity([string]$File, [string]$InfoFile) {
  if (!(Test-Path -LiteralPath $File -PathType Leaf) -or !(Test-Path -LiteralPath $InfoFile -PathType Leaf)) { return $false }
  try {
    $info = Get-Content -LiteralPath $InfoFile -Raw | ConvertFrom-Json
    if ($info.schema -ne 1 -or $info.game_file -ne [IO.Path]::GetFileName($File)) { return $false }
    if ([string]$info.game_sha256 -notmatch '^[a-fA-F0-9]{64}$') { return $false }
    return ((Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash -ieq $info.game_sha256)
  } catch { return $false }
}
if (!(Test-MoonfallIntegrity $GameExe $Manifest)) { throw 'Valid build failed SHA256 verification' }
$originalHash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash
$temp = Join-Path ([IO.Path]::GetTempPath()) ('moonfall-integrity-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
  $copy = Join-Path $temp ([IO.Path]::GetFileName($GameExe))
  $info = Join-Path $temp 'manifest.json'
  Copy-Item -LiteralPath $GameExe -Destination $copy
  Copy-Item -LiteralPath $Manifest -Destination $info
  if (!(Test-MoonfallIntegrity $copy $info)) { throw 'Untampered copy did not verify' }
  # Change one byte; detection must fail without changing the original executable.
  $stream = [IO.File]::Open($copy,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite)
  try {
    $stream.Position = 0
    $byte = $stream.ReadByte()
    $stream.Position = 0
    $stream.WriteByte([byte]($byte -bxor 1))
  } finally { $stream.Dispose() }
  if (Test-MoonfallIntegrity $copy $info) { throw 'Tampered executable incorrectly passed checksum verification' }
  # A matching executable must still reject forged, malformed, or wrong-name manifests.
  Copy-Item -LiteralPath $GameExe -Destination $copy -Force
  $validInfo = Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
  $validInfo.game_sha256 = '0' * 64
  $validInfo | ConvertTo-Json | Set-Content -LiteralPath $info -Encoding UTF8
  if (Test-MoonfallIntegrity $copy $info) { throw 'Incorrect manifest checksum was accepted' }
  $validInfo.game_sha256 = (Get-FileHash -LiteralPath $copy -Algorithm SHA256).Hash
  $validInfo.game_file = 'NotMoonfall.exe'
  $validInfo | ConvertTo-Json | Set-Content -LiteralPath $info -Encoding UTF8
  if (Test-MoonfallIntegrity $copy $info) { throw 'Incorrect filename was accepted' }
  $validInfo.game_file = [IO.Path]::GetFileName($copy)
  $validInfo.schema = 999
  $validInfo | ConvertTo-Json | Set-Content -LiteralPath $info -Encoding UTF8
  if (Test-MoonfallIntegrity $copy $info) { throw 'Unsupported manifest schema was accepted' }
  $validInfo.schema = 1
  $validInfo.game_sha256 = 'INVALID'
  $validInfo | ConvertTo-Json | Set-Content -LiteralPath $info -Encoding UTF8
  if (Test-MoonfallIntegrity $copy $info) { throw 'Invalid checksum length was accepted' }
  $validInfo.game_sha256 = (Get-FileHash -LiteralPath $copy -Algorithm SHA256).Hash
  $validInfo.game_file = [IO.Path]::GetFileName($copy)
  $validInfo.schema = 1
  $validInfo | ConvertTo-Json | Set-Content -LiteralPath $info -Encoding UTF8
  if (!(Test-MoonfallIntegrity $copy $info)) { throw 'Restored valid manifest did not pass' }
  Set-Content -LiteralPath $info -Value '{bad-json' -Encoding UTF8
  if (Test-MoonfallIntegrity $copy $info) { throw 'Malformed manifest was accepted' }
  Remove-Item -LiteralPath $info
  if (Test-MoonfallIntegrity $copy $info) { throw 'Missing manifest incorrectly passed integrity verification' }
  Write-Host 'PASS: good file accepted; damaged file, wrong checksum, wrong filename, malformed manifest and missing manifest rejected'
} finally {
  Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
  if ((Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash -ne $originalHash) { throw 'Integrity tests modified original game executable' }
}
