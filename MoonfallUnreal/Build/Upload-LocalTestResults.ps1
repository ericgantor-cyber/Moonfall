#requires -Version 5.1
[CmdletBinding()]
param(
 [string]$ProjectRoot="",
 [string]$BuildLogRoot=(Join-Path $env:LOCALAPPDATA "MoonfallBuildCenter\Logs"),
 [string]$CacheRoot=(Join-Path $env:LOCALAPPDATA "Moonfall\TestUpload"),
 [string]$GitHubCli="gh",
 [switch]$DryRun
)
$ErrorActionPreference="Stop"
# Resolve script-relative defaults after param binding, including nested powershell.exe invocations.
if([string]::IsNullOrWhiteSpace($ProjectRoot)){
 $ProjectRoot=Split-Path -Parent $PSScriptRoot
}
if([string]::IsNullOrWhiteSpace($ProjectRoot)){throw "Unable to locate the Moonfall Unreal project root."}
$statePath=$CacheRoot
New-Item -ItemType Directory -Force -Path $statePath | Out-Null
# Exclusive per-user lock prevents a second scheduler/manual run from re-sending
# the same digest before the first GitHub dispatch is recorded.
$lockFile=Join-Path $statePath "uploader.lock"
try {
 $lockHandle=[IO.File]::Open($lockFile,[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
} catch [IO.IOException] {
 Write-Output "Another Moonfall upload scan is already running; skipping this scan."
 return
}
try {
$cacheFile=Join-Path $statePath "sent.json"
$sent=@{}
if(Test-Path $cacheFile){try{$obj=Get-Content $cacheFile -Raw|ConvertFrom-Json;foreach($p in $obj.PSObject.Properties){$sent[$p.Name]=$p.Value}}catch{}}
if(-not $DryRun -and -not (Get-Command -Name $GitHubCli -ErrorAction SilentlyContinue)){throw "Install gh and run gh auth login."}
$maxReportsPerRun=10
$reportCount=0
$earliestUtc=[DateTime]::UtcNow.AddDays(-7)
$roots=@(
 @{ Folder=(Join-Path $ProjectRoot "Saved\Logs"); Kind="unreal-log"; Extensions=@(".log") },
 @{ Folder=(Join-Path $ProjectRoot "Saved\Automation\Reports"); Kind="automation-report"; Extensions=@(".log",".json") },
 @{ Folder=$BuildLogRoot; Kind="build-center-log"; Extensions=@(".log",".err") }
)
:sourceLoop foreach($root in $roots){
 $folder=$root.Folder
 if(-not(Test-Path $folder)){continue}
 foreach($f in (Get-ChildItem -LiteralPath $folder -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending)){
  if($reportCount -ge $maxReportsPerRun){break sourceLoop}
  if($f.LastWriteTimeUtc -lt $earliestUtc -or $f.Extension -notin $root.Extensions -or $f.Length -lt 1 -or $f.Length -gt 10MB -or ($f.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $f.LastWriteTimeUtc -gt [DateTime]::UtcNow.AddSeconds(-60)){continue}
  $hash=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  if($sent.ContainsKey($hash)){continue}
  $tail=Get-Content -LiteralPath $f.FullName -Tail 500|Out-String
  $errors=[regex]::Matches($tail,'(?im)(?:error\s*:|fatal error|BUILD FAILED|Automation Test Failed|FAILED \(failures=|error (?:C\d{4}|MSB\d{4}|LNK\d{4})\b)').Count
  $warnings=[regex]::Matches($tail,'(?im)warning\s*:').Count
  $ok=$tail -match '(?im)(?:BUILD SUCCEEDED|BUILD SUCCESSFUL|Automation Test Passed|Tests passed)'
  $status=if($errors -gt 0){"failed"}elseif($ok){"passed"}else{"unknown"}
  $category=$root.Kind
  $report=@{format_version=1;source=$category;outcome=$status;errors=[Math]::Min($errors,999);warnings=[Math]::Min($warnings,999);file_sha256=$hash}
  $json=@{event_type="moonfall-local-test";client_payload=$report}|ConvertTo-Json -Depth 5 -Compress
  if($DryRun){Write-Output $json;$reportCount++;continue}
  $json|& $GitHubCli api "repos/ericgantor-cyber/Moonfall/dispatches" -X POST --input - 2>$null
  if($LASTEXITCODE -ne 0){Write-Warning "Upload failed: check GitHub login";continue}
  $sent[$hash]=[DateTime]::UtcNow.ToString("o")
  $reportCount++
  $sent|ConvertTo-Json -Depth 3|Set-Content -LiteralPath $cacheFile -Encoding UTF8
  Write-Output "Sent Moonfall $status test summary."
 }
}

} finally {
 $lockHandle.Dispose()
}
