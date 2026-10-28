param(
 [Parameter(Mandatory=$true)][string]$GameExe,
 [string]$InstallerExe = '',
 [switch]$SkipLaunch
)
$ErrorActionPreference = 'Stop'
function Assert-True([bool]$Condition,[string]$Failure) { if (-not $Condition) {throw $Failure} }
$exe = (Resolve-Path -LiteralPath $GameExe).Path
$stream = [System.IO.File]::OpenRead($exe)
try {
 Assert-True ($stream.Length -gt 16) 'Executable is truncated'
 $magic = New-Object byte[] 2
 [void]$stream.Read($magic,0,2)
 Assert-True ([Text.Encoding]::ASCII.GetString($magic) -eq 'MZ') 'Windows PE MZ header missing'
 [void]$stream.Seek(-12,[System.IO.SeekOrigin]::End)
 $tail = New-Object byte[] 12
 [void]$stream.Read($tail,0,12)
 Assert-True ([Text.Encoding]::ASCII.GetString($tail,8,4) -eq 'GDPC') 'Embedded Godot pack footer missing'
 $packBytes = [BitConverter]::ToInt64($tail,0)
 Assert-True ($packBytes -gt 0 -and $packBytes -lt ($stream.Length - 12)) 'Embedded pack size invalid'
 [void]$stream.Seek(-($packBytes+12),[System.IO.SeekOrigin]::End)
 $packMagic=New-Object byte[] 4
 [void]$stream.Read($packMagic,0,4)
 Assert-True ([Text.Encoding]::ASCII.GetString($packMagic) -eq 'GDPC') 'Embedded Godot pack header missing'
 Write-Host "PASS: Embedded game data verified ($packBytes bytes)"
} finally {$stream.Dispose()}
Write-Host ('GAME SHA256: '+(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash)
if ($InstallerExe -ne '') {
 $setup = (Resolve-Path -LiteralPath $InstallerExe).Path
 Assert-True (([IO.FileInfo]$setup).Length -gt 1024) 'Installer file appears truncated'
 $setupFile = [System.IO.File]::OpenRead($setup)
 try {
  $magic = New-Object byte[] 2
  [void]$setupFile.Read($magic,0,2)
  Assert-True ([Text.Encoding]::ASCII.GetString($magic) -eq 'MZ') 'Installer PE header missing'
 } finally { $setupFile.Dispose() }
 Write-Host ('INSTALLER SHA256: '+(Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash)
 Write-Host 'PASS: Installer existence and PE-header checks only; payload, install and shortcuts NOT verified'
}
if (-not $SkipLaunch) {
 $outLog=Join-Path ([IO.Path]::GetTempPath()) ('Moonfall-v34_4-headless-'+[Guid]::NewGuid().ToString('N')+'.log')
 $p=Start-Process -FilePath $exe -ArgumentList '--headless','--quit-after','180' -PassThru -Wait -RedirectStandardOutput $outLog -RedirectStandardError ($outLog+'.err')
 $text=([IO.File]::ReadAllText($outLog)+"`n"+[IO.File]::ReadAllText($outLog+'.err'))
 Assert-True ($p.ExitCode -eq 0) "Headless game launch failed; logs: $outLog"
 Assert-True (-not ($text -match 'SCRIPT ERROR|Parse Error')) "Godot script error; logs: $outLog"
 Write-Host "PASS: 180-frame headless launch; logs: $outLog"
}
Write-Host 'MOONFALL WINDOWS RELEASE SMOKE PASS (installation, shortcuts, graphics, controller not tested)'
