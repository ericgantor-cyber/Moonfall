param(
  [string]$Repository = 'https://github.com/ericgantor-cyber/Moonfall.git',
  [string]$Branch = 'feature/moonfall-native-v37-release-gate'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$source = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git is required. Install Git for Windows or GitHub Desktop and authenticate first.' }
$folder = Join-Path ([System.IO.Path]::GetTempPath()) ('moonfall-native-publish-' + [guid]::NewGuid().ToString('N'))
try {
  & git clone --depth 1 $Repository $folder
  if ($LASTEXITCODE -ne 0) { throw 'Could not clone GitHub repository; authenticate Git first.' }
  & git -C $folder switch -c $Branch
  if ($LASTEXITCODE -ne 0) { throw 'Could not create feature branch; choose a unique branch name.' }
  # The existing repo already has native/dx12-prototype. Never replace it.
  $dest = Join-Path $folder 'native\moonfall-godot-v34_4'
  if (Test-Path $dest) { throw 'The dedicated native/moonfall-godot-v34_4 folder already exists. Stop rather than overwrite it.' }
  [void](New-Item -ItemType Directory -Force -Path $dest)
  # Keep nested .github project CI validation files; only root workflow is executed.
  $skip = @('.godot', 'build', 'Release_Windows', '.git')
  Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -notin $skip } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $dest $_.Name) -Recurse -Force
  }
  $workflowDir = Join-Path $folder '.github\workflows'
  [void](New-Item -ItemType Directory -Force -Path $workflowDir)
  $workflowPath = Join-Path $workflowDir 'moonfall-native-v34_4.yml'
  if (Test-Path $workflowPath) { throw 'The dedicated workflow already exists. Stop rather than overwrite it.' }
  Copy-Item -LiteralPath (Join-Path $source 'tools\github_windows_v34_4.yml') -Destination $workflowPath
  & git -C $folder add 'native/moonfall-godot-v34_4' '.github/workflows/moonfall-native-v34_4.yml'
  if ($LASTEXITCODE -ne 0) { throw 'Git add failed.' }
  & git -C $folder commit -m 'Add isolated Moonfall Native v37 world and strict Godot release diagnostics'
  if ($LASTEXITCODE -ne 0) { throw 'Git commit failed. Configure your Git user name and email.' }
  & git -C $folder push -u origin $Branch
  if ($LASTEXITCODE -ne 0) { throw 'Git push failed. Confirm GitHub permissions or use a different branch name.' }
  Write-Host 'SUCCESS: Source pushed to the feature branch. Main has NOT been modified.'
  Write-Host ('Open a pull request: https://github.com/ericgantor-cyber/Moonfall/compare/main...' + $Branch)
  Write-Host 'Opening a pull request triggers Windows build and install tests.'
  Write-Host 'The original browser game and native/dx12-prototype are untouched.'
} finally {
  if (Test-Path $folder) { Remove-Item -LiteralPath $folder -Recurse -Force -ErrorAction SilentlyContinue }
}
