param([Parameter(Mandatory=$true)][string]$GameExe,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $GameExe -PathType Leaf)) { throw "Game executable missing: $GameExe" }
$hash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash.ToLowerInvariant()
@{schema=1; game_file=[IO.Path]::GetFileName($GameExe); game_sha256=$hash} |
  ConvertTo-Json | Set-Content -LiteralPath $Output -Encoding UTF8
Write-Host "Generated trusted release integrity manifest: $Output"
