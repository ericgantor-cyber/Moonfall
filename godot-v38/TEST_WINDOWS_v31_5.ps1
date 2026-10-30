param([string]$GameExe = (Join-Path $PSScriptRoot 'MoonfallNative_v31_5.exe'))
$ErrorActionPreference = 'Stop'
$report = Join-Path $PSScriptRoot 'WINDOWS_v31_5_CHECK_RESULT.txt'
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('Moonfall v31.5 Windows diagnostic (portable, no installation)')
$lines.Add('OS: ' + [Environment]::OSVersion.VersionString)
if (!(Test-Path -LiteralPath $GameExe)) { throw "Game executable missing: $GameExe" }
$hash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash
$lines.Add('Game SHA256: ' + $hash)
$lines.Add('File size: ' + (Get-Item -LiteralPath $GameExe).Length)
$stdout = Join-Path $PSScriptRoot 'WINDOWS_v31_5_GODOT_STDOUT.txt'
$stderr = Join-Path $PSScriptRoot 'WINDOWS_v31_5_GODOT_STDERR.txt'
$p=Start-Process -FilePath $GameExe -ArgumentList '--headless','--quit-after','300' -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$lines.Add('Headless exit code: ' + $p.ExitCode)
$bad = Select-String -Path $stdout,$stderr -Pattern 'SCRIPT ERROR','Parse Error','ERROR: Cannot','ERROR: Failed' -SimpleMatch -Quiet
$lines.Add('Script errors found: ' + [bool]$bad)
$lines.Add('Manual graphics/controller/installer validation: NOT RUN by this script')
[IO.File]::WriteAllLines($report,$lines)
Get-Content -LiteralPath $report
if ($p.ExitCode -ne 0 -or $bad) { throw 'Automated Windows diagnostic failed. Inspect output logs.' }
