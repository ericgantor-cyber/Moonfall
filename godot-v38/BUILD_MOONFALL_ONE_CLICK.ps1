param([switch]$PortableOnly)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$project = $PSScriptRoot
$godotRoot = Join-Path $project 'Godot_4.7.2'
$engine = Join-Path $godotRoot 'Godot_v4.7.2-stable_win64.exe'
$archive = Join-Path $godotRoot 'Godot_v4.7.2-stable_win64.exe.zip'
$url = 'https://github.com/godotengine/godot/releases/download/4.7.2-stable/Godot_v4.7.2-stable_win64.exe.zip'
$expectedZipHash = '731980f9608d61333e5baf54a2ef17210acc7a538446c0cb9969f002aca1e953'
$game = Join-Path $project 'build\MoonfallNative_v34_4.exe'
$installer = Join-Path $project 'Release_Windows\Moonfall_Native_v34_4_Windows_Setup.exe'
$statusFile = Join-Path $project 'build\BUILD_STATUS_v38_1.json'
$failureFile = Join-Path $project 'build\BUILD_FAILURE_v38_1.txt'
. (Join-Path $project 'tools\Bootstrap-WindowsPrerequisites.ps1')

function Require-File([string]$path,[string]$label) {
 if (!(Test-Path -LiteralPath $path -PathType Leaf)) {throw "Missing $label : $path"}
}
function Require-Sha([string]$path,[string]$hash) {
 $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
 if ($actual -ne $hash) {throw "SHA256 mismatch for $path. Expected $hash; got $actual"}
}
try {
  Write-Host '[1/7] Checking tools and setting up Python 3...' -ForegroundColor Cyan
  Require-File (Join-Path $project 'project.godot') 'Godot project'
  Require-File (Join-Path $project 'scenes\main.tscn') 'Godot scene'
  Require-File (Join-Path $project 'BUILD_WINDOWS.bat') 'release test runner'
  $pythonExe = Ensure-MoonfallPython3
  & $pythonExe --version
  if ($LASTEXITCODE -ne 0) {throw 'Python 3 verification failed.'}
  # The existing BUILD_WINDOWS.bat calls plain `python`; WinGet's PATH update
  # does not propagate to an already running PowerShell session.
  $env:PATH = (Split-Path -Parent $pythonExe) + [IO.Path]::PathSeparator + $env:PATH
  New-Item -ItemType Directory -Force -Path $godotRoot | Out-Null
  New-Item -ItemType Directory -Force -Path (Join-Path $project 'build') | Out-Null
  # Prevent an old binary or installer from being confused with a new successful build.
  foreach ($out in @($statusFile,$failureFile,$game,$installer)) {if (Test-Path $out) {Remove-Item $out -Force}}

  Write-Host '[2/7] Setting up Inno Setup 6...' -ForegroundColor Cyan
  $installerBuilt = $false
  $iscc = $null
  if (!$PortableOnly) {
    $iscc = Ensure-MoonfallInno6
    # A requested installer is mandatory. No silent downgrade to portable-only.
  } else {
    Write-Host 'PortableOnly selected: skipping Inno Setup and installer creation.'
  }

  Write-Host '[3/7] Checking official Godot 4.7.2 engine...' -ForegroundColor Cyan
  if (!(Test-Path $archive)) {
   Write-Host 'Downloading official Godot 4.7.2 editor (first run requires internet).'
   Invoke-WebRequest -Uri $url -OutFile $archive
  }
  # Always validate and extract the original signed-release archive, even when
  # another Godot executable already exists at the expected path.
  Require-Sha $archive $expectedZipHash
  Expand-Archive -LiteralPath $archive -DestinationPath $godotRoot -Force
  Require-File $engine 'verified-source Godot executable'
  $versionOutput = (& $engine --headless --version 2>&1 | Out-String).Trim()
  if ($LASTEXITCODE -ne 0 -or $versionOutput -notmatch '4\.7\.2') {throw "Wrong Godot engine; required 4.7.2, got: $versionOutput"}
  Write-Host "Godot version: $versionOutput"

  Write-Host '[4/7] Installing SHA256-verified Windows export templates...' -ForegroundColor Cyan
  & (Join-Path $project 'INSTALL_GODOT_WINDOWS_TEMPLATES.ps1')
  if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {throw "Export template installation failed: $LASTEXITCODE"}

  Write-Host '[5/7] Executing all source gates, Godot tests and official Windows export...' -ForegroundColor Cyan
  $verifiers = @(Get-ChildItem -LiteralPath (Join-Path $project 'tests') -Filter 'verify_*.py' -File | Sort-Object Name)
  if ($verifiers.Count -lt 39) {throw "Missing offline verification scripts: expected at least 39, found $($verifiers.Count)"}
  foreach ($check in $verifiers) {
   Write-Host "SOURCE CHECK: $($check.Name)"
   & $pythonExe $check.FullName
   if ($LASTEXITCODE -ne 0) {throw "Offline source regression: $($check.Name) (exit $LASTEXITCODE)"}
  }
  Write-Host "PASS: $($verifiers.Count) offline source checks. Starting the 66 Godot suites."
  $env:MOONFALL_GODOT_EXE = $engine
  # A hung Godot integration test must not leave a double-click build running forever.
  # cmd.exe /D /S /C ""C:\path with spaces\BUILD_WINDOWS.bat"" handles spaces.
  $buildRunner = Join-Path $project 'BUILD_WINDOWS.bat'
  $buildCmd = '""' + $buildRunner + '""'
  $buildProcess = Start-Process -FilePath $env:ComSpec -ArgumentList @('/D','/S','/C',$buildCmd) -WorkingDirectory $project -NoNewWindow -PassThru
  $buildLimitMinutes = 60
  if (-not $buildProcess.WaitForExit($buildLimitMinutes * 60 * 1000)) {
    # /T terminates the whole child tree, including a hanging Godot process.
    & taskkill.exe /PID $buildProcess.Id /T /F 2>$null | Out-Null
    throw "Godot build timed out after $buildLimitMinutes minutes. See GODOT_*_LOG.txt."
  }
  if ($buildProcess.ExitCode -ne 0) {throw "Godot gameplay / export gate failed (exit $($buildProcess.ExitCode)). Inspect GODOT_*_LOG.txt."}
  Require-File $game 'newly built portable Windows game'
  & (Join-Path $project 'VERIFY_WINDOWS_RELEASE.ps1') -GameExe $game -SkipLaunch
  if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {throw 'Windows executable verification failed.'}

  Write-Host '[6/7] Compiling Windows installer and verifying output...' -ForegroundColor Cyan
  if (!$PortableOnly) {
    Require-File $iscc 'Inno Setup 6 compiler'
    & $iscc (Join-Path $project 'Moonfall_Native_v34_4_Setup.iss')
    if ($LASTEXITCODE -ne 0) {throw "Inno Setup compilation failed: $LASTEXITCODE"}
    Require-File $installer 'newly built Windows installer'
    & (Join-Path $project 'VERIFY_WINDOWS_RELEASE.ps1') -GameExe $game -InstallerExe $installer -SkipLaunch
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {throw 'Installer verification failed.'}
    $installerBuilt = $true
  }
  Write-Host '[7/7] Writing build evidence...' -ForegroundColor Cyan
  $evidence = [ordered]@{
    package = 'Moonfall v38.1 release gate (v38.0 gameplay)'
    python_exe = $pythonExe
    python_version = (& $pythonExe --version 2>&1 | Out-String).Trim()
    inno_compiler = if ($installerBuilt) {$iscc} else {$null}
    gameplay_source = 'v38.0'
    godot_version = $versionOutput
    engine_test_suites_registered = 66
    game_exe = $game
    game_sha256 = (Get-FileHash -LiteralPath $game -Algorithm SHA256).Hash.ToLowerInvariant()
    installer_built = $installerBuilt
    installer_exe = if ($installerBuilt) {$installer} else {$null}
    installer_sha256 = if ($installerBuilt) {(Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()} else {$null}
    engine_tests_and_headless_smoke = 'passed in BUILD_WINDOWS.bat'
    successful_build = $true
    build_timeout_minutes = $buildLimitMinutes
    human_gameplay_tested = $false
    gpu_4k_tested = $false
    timestamp_utc = (Get-Date).ToUniversalTime().ToString('o')
  }
  $evidence | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $statusFile -Encoding UTF8
  Write-Host "PASS: portable game at $game" -ForegroundColor Green
  if ($installerBuilt) {Write-Host "PASS: installer at $installer" -ForegroundColor Green}
  Write-Host "Evidence: $statusFile"
  Write-Host 'This does not replace hands-on combat, controller or GPU testing.'
  exit 0
} catch {
  # Fail closed: never leave release evidence that might be mistaken for success.
  if (Test-Path $statusFile) {Remove-Item $statusFile -Force}
  # Never leave a partial new exe/installer that looks like a successful release.
  foreach ($out in @($game,$installer)) {
    if (Test-Path -LiteralPath $out) {Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue}
  }
  $errorText = "MOONFALL BUILD FAILED: $($_.Exception.Message)`r`nNo verified release was produced. Run COLLECT_BUILD_LOGS.bat to gather diagnostics."
  $failureDir = Split-Path -Parent $failureFile
  if (!(Test-Path -LiteralPath $failureDir)) {New-Item -ItemType Directory -Force -Path $failureDir | Out-Null}
  Set-Content -LiteralPath $failureFile -Value $errorText -Encoding UTF8
  Write-Error $errorText
  exit 1
}
