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
  Remove-Item -LiteralPath $info
  if (Test-MoonfallIntegrity $copy $info) { throw 'Missing manifest incorrectly passed integrity verification' }
  Write-Host 'PASS: genuine file accepted, tampered file rejected, missing manifest rejected'
} finally { Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue }
