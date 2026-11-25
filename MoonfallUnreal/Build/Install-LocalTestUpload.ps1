#requires -Version 5.1
[CmdletBinding()]
param([switch]$Uninstall,[switch]$TestNow,[switch]$Status)
$ErrorActionPreference="Stop"
$task="Moonfall Local Test Upload"
$script=Join-Path $PSScriptRoot "Upload-LocalTestResults.ps1"
if($Status){
 & schtasks.exe /Query /TN $task *> $null
 if($LASTEXITCODE -eq 0){Write-Output "Moonfall automatic test uploads: ENABLED"}
 else{Write-Output "Moonfall automatic test uploads: NOT INSTALLED"}
 exit 0
}
if($Uninstall){& schtasks.exe /Delete /TN $task /F;exit $LASTEXITCODE}
# Preview is strictly local: do not require GitHub CLI or credentials.
if($TestNow){& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script -DryRun;exit $LASTEXITCODE}
if(-not(Get-Command gh -ErrorAction SilentlyContinue)){throw "Install GitHub CLI and run gh auth login."}
& gh auth status *> $null
if($LASTEXITCODE -ne 0){throw "Run gh auth login first."}
$command='powershell.exe -NoProfile -ExecutionPolicy Bypass -File "'+$script+'"'
& schtasks.exe /Create /TN $task /SC MINUTE /MO 5 /TR $command /F
if($LASTEXITCODE -ne 0){throw "Task setup failed."}
Write-Output "Installed. Local test summaries will be checked every five minutes."
