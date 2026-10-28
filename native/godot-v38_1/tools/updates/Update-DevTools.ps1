# Moonfall developer tool updates. Windows PowerShell 5.1+, per-user, opt-in.
# Never updates the shipped Moonfall game, game saves, or original image assets.
[CmdletBinding()]
param(
    [ValidateSet('Check', 'Update', 'EnableWeekly', 'DisableWeekly')]
    [string]$Mode = 'Check',
    [string]$ProjectRoot = '',
    [ValidatePattern('^4\.7$')]
    [string]$GodotSeries = '4.7',
    [switch]$SkipGodot,
    [switch]$SkipVSCode
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$TaskName = 'Moonfall Developer Tool Updates'
$ProjectRoot = if ($ProjectRoot) { [IO.Path]::GetFullPath($ProjectRoot) } else { [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')) }
$DataHome = Join-Path $env:LOCALAPPDATA 'MoonfallDev'
$GodotHome = Join-Path $DataHome 'Godot'
$LogDir = Join-Path $DataHome 'logs'
$ActiveFile = Join-Path $DataHome 'active_godot.txt'
[IO.Directory]::CreateDirectory($LogDir) | Out-Null
$LogFile = Join-Path $LogDir 'updates.log'
function Log([string]$Message) {
    $line = ('[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Message)
    Write-Host $line
    Add-Content -LiteralPath $LogFile -Value $line -Encoding UTF8
}
function Test-Executable([string]$Program) { return [bool](Get-Command $Program -ErrorAction SilentlyContinue) }
function Get-GodotStablePatch {
    # Only the project's major.minor line; never pick a new feature or prerelease version automatically.
    if ($GodotSeries -notmatch '^\d+\.\d+$') { throw 'Invalid Godot major.minor series' }
    $uri = 'https://api.github.com/repos/godotengine/godot/releases?per_page=50'
    $headers = @{ 'User-Agent' = 'Moonfall-DevTools-Updater'; 'Accept' = 'application/vnd.github+json' }
    $releases = @(Invoke-RestMethod -Uri $uri -Headers $headers -TimeoutSec 25)
    $candidates = @()
    foreach ($release in $releases) {
        if ($release.draft -or $release.prerelease) { continue }
        $match = [regex]::Match([string]$release.tag_name, ('^' + [regex]::Escape($GodotSeries) + '\.(\d+)-stable$'))
        if (-not $match.Success) { continue }
        $patch = [int]$match.Groups[1].Value
        $assetName = 'Godot_v{0}-stable_win64.exe.zip' -f (([string]$release.tag_name) -replace '-stable$', '')
        $asset = @($release.assets | Where-Object { $_.name -ceq $assetName } | Select-Object -First 1)
        if ($asset.Count -ne 1) { continue }
        $candidates += [pscustomobject]@{Version = ([string]$release.tag_name -replace '-stable$', ''); Patch = $patch; Asset = $asset[0] }
    }
    if (-not $candidates) { throw "No stable $GodotSeries Windows editor release was returned by the official Godot release API" }
    return ($candidates | Sort-Object Patch -Descending | Select-Object -First 1)
}
function Get-LocalGodotVersion {
    if (-not (Test-Path -LiteralPath $ActiveFile)) { return $null }
    $path = (Get-Content -LiteralPath $ActiveFile -Raw).Trim()
    if (-not $path -or -not (Test-Path -LiteralPath $path -PathType Leaf)) { return $null }
    if ($path -match 'Godot[\\/]([0-9]+\.[0-9]+\.[0-9]+)[\\/]') { return $Matches[1] }
    return $null
}
function Test-GodotCandidate([string]$EditorExe) {
    # Work on a temporary copy so a new editor never touches the real source, its imports or saved games.
    $scratch = Join-Path $env:TEMP ('moonfall-godot-upgrade-' + [guid]::NewGuid().ToString('N'))
    $projectCopy = Join-Path $scratch 'project'
    [IO.Directory]::CreateDirectory($projectCopy) | Out-Null
    try {
        # Copy only source files. Exclude generated cache/build/git folders and avoid reparse points.
        # A candidate engine must never import or modify the user's actual project directory.
        foreach ($item in (Get-ChildItem -LiteralPath $ProjectRoot -Force)) {
            if ($item.Name -in @('.godot', '.git', 'build')) { continue }
            if ($item.Name -match '^(Godot_[0-9]|GODOT_.*_LOG\.txt$)' ) { continue }
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { continue }
            Copy-Item -LiteralPath $item.FullName -Destination $projectCopy -Recurse -Force
        }
        if (-not (Test-Path -LiteralPath (Join-Path $projectCopy 'project.godot'))) { throw 'Disposable project copy is incomplete' }
        $env:APPDATA = Join-Path $scratch 'Roaming'
        $env:LOCALAPPDATA = Join-Path $scratch 'Local'
        $env:MOONFALL_TEST_PROFILE = Join-Path $scratch 'test-profile'
        [IO.Directory]::CreateDirectory($env:APPDATA) | Out-Null
        [IO.Directory]::CreateDirectory($env:LOCALAPPDATA) | Out-Null
        [IO.Directory]::CreateDirectory($env:MOONFALL_TEST_PROFILE) | Out-Null
        $import = & $EditorExe --headless --editor --path $projectCopy --import --quit 2>&1
        if ($LASTEXITCODE -ne 0 -or ($import -match '(SCRIPT ERROR|Parse Error)')) { throw 'Godot editor import failed' }
        $test = & $EditorExe --headless --path $projectCopy --script 'res://tests/smoke_v11.gd' 2>&1
        if ($LASTEXITCODE -ne 0 -or -not ($test -match 'MOONFALL NATIVE v11 SMOKE PASS')) { throw 'Moonfall smoke test failed with the new Godot editor' }
        $profiler = & $EditorExe --headless --path $projectCopy --script 'res://tests/v30_1_profiler.gd' 2>&1
        if ($LASTEXITCODE -ne 0 -or -not ($profiler -match 'MOONFALL v30.1 PROFILER PASS')) { throw 'Moonfall v30.1 profiler regression failed with the new Godot editor' }
        $runtime = & $EditorExe --headless --path $projectCopy --quit-after 120 2>&1
        if ($LASTEXITCODE -ne 0 -or ($runtime -match '(SCRIPT ERROR|Parse Error)')) { throw 'Moonfall main-scene startup failed with candidate Godot editor' }
        return $true
    }
    finally {
        $env:APPDATA = $script:OriginalAppData
        $env:LOCALAPPDATA = $script:OriginalLocalAppData
        $env:MOONFALL_TEST_PROFILE = $script:OriginalTestProfile
        Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
    }
}
function Update-Godot {
    $latest = Get-GodotStablePatch
    $current = Get-LocalGodotVersion
    Log ('Godot latest stable compatible patch: {0}; active local update: {1}; bundled editor remains pinned at 4.7.2' -f $latest.Version, $(if ($current) {$current} else {'none'}))
    if ($Mode -eq 'Check') { return }
    # Never downgrade; keep the bundled editor and older installed patches available for rollback.
    $candidateVer = [version]$latest.Version
    if ($candidateVer -le [version]'4.7.2' -or ($current -and $candidateVer -le [version]$current)) {
        Log 'No newer compatible Godot patch to install.'
        return
    }
    $asset = $latest.Asset
    $url = [string]$asset.browser_download_url
    if ($url -notmatch '^https://github\.com/godotengine/godot/releases/download/') { throw 'Refusing unexpected Godot download host' }
    $digest = [string]$asset.digest
    if ($digest -notmatch '^sha256:[a-fA-F0-9]{64}$') { throw 'GitHub did not supply an authenticated SHA-256 asset digest: refusing unattended download.' }
    $destination = Join-Path $GodotHome $latest.Version
    $exe = Join-Path $destination ('Godot_v{0}-stable_win64.exe' -f $latest.Version)
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        [IO.Directory]::CreateDirectory($destination) | Out-Null
        $download = Join-Path $destination 'godot.zip'
        try {
            Log "Downloading official Godot $($latest.Version) into a separate version folder."
            Invoke-WebRequest -Uri $url -OutFile $download -UseBasicParsing -TimeoutSec 240
            $actual = (Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($actual -cne $digest.Substring(7).ToLowerInvariant()) { throw 'Downloaded Godot ZIP SHA-256 mismatch' }
            Expand-Archive -LiteralPath $download -DestinationPath $destination -Force
            if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Expected Godot editor EXE missing after extraction' }
        }
        finally { Remove-Item -LiteralPath $download -Force -ErrorAction SilentlyContinue }
    }
    $testVersion = (& $exe --headless --version 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0 -or $testVersion -notmatch ([regex]::Escape($latest.Version))) { throw 'Downloaded Godot editor failed executable/version test' }
    Log 'Running import + Moonfall smoke tests on a disposable project copy.'
    if (-not (Test-GodotCandidate $exe)) { throw 'Godot validation failed; keeping original editor.' }
    Set-Content -LiteralPath $ActiveFile -Value $exe -Encoding Ascii
    Log "Godot $($latest.Version) validated and selected for future DEVELOPMENT builds. Existing game EXEs and original 4.7.2 editor are unchanged."
}
function Update-VSCode {
    # VS Code's built-in update mode remains untouched. winget handles an installed Windows package if available.
    if (-not (Test-Executable 'winget.exe')) { Log 'winget unavailable; VS Code built-in auto updates may still work.'; return }
    $id = 'Microsoft.VisualStudioCode'
    $listOutput = (& winget.exe list --id $id --exact --accept-source-agreements 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0 -or $listOutput -notmatch [regex]::Escape($id)) { Log 'VS Code not found by winget; no automatic installation attempted.'; return }
    if ($Mode -eq 'Check') {
        $upgrades = (& winget.exe upgrade --id $id --exact --accept-source-agreements 2>&1 | Out-String)
        Log ('VS Code upgrade check: ' + (($upgrades -split '\r?\n' | Where-Object { $_.Trim() } | Select-Object -Last 5) -join ' | '))
        return
    }
    if (Get-Process -Name Code -ErrorAction SilentlyContinue) { Log 'VS Code is running; skipping winget update so its own auto-updater can handle it safely.'; return }
    Log 'Asking winget to update installed VS Code (user setup recommended).'
    $output = (& winget.exe upgrade --id $id --exact --silent --accept-source-agreements --accept-package-agreements --disable-interactivity 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { Log ('VS Code upgrade returned exit ' + $LASTEXITCODE + ': ' + $output); return }
    Log 'VS Code winget upgrade command completed (or reports already current).'
}
function Enable-Weekly {
    $user = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
    [IO.Directory]::CreateDirectory($DataHome) | Out-Null
    $installedScript = Join-Path $DataHome 'Update-DevTools.ps1'
    Copy-Item -LiteralPath $PSCommandPath -Destination $installedScript -Force
    $action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument ('-NoProfile -ExecutionPolicy Bypass -File "{0}" -Mode Update -ProjectRoot "{1}" -GodotSeries "{2}"' -f $installedScript, $ProjectRoot, $GodotSeries)
    $trigger = New-ScheduledTaskTrigger -Weekly -DaysOfWeek Sunday -At '17:00'
    $settings = New-ScheduledTaskSettingsSet -StartWhenAvailable -ExecutionTimeLimit (New-TimeSpan -Minutes 25) -MultipleInstances IgnoreNew
    $principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
    Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger -Settings $settings -Principal $principal -Force | Out-Null
    Log 'Weekly updater ENABLED for Sundays at 5 PM, only when signed in. Godot patches are validated first.'
}
$OriginalAppData = $env:APPDATA
$OriginalLocalAppData = $env:LOCALAPPDATA
$OriginalTestProfile = $env:MOONFALL_TEST_PROFILE
try {
    switch ($Mode) {
        'EnableWeekly' { Enable-Weekly; break }
        'DisableWeekly' {
            if (Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue) { Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false }
            Log 'Weekly updater disabled.'
            break
        }
        default {
            Log ('Moonfall developer tools: ' + $Mode)
            if (-not $SkipVSCode) { try { Update-VSCode } catch { Log ('VS Code update issue: ' + $_.Exception.Message) } }
            if (-not $SkipGodot) { try { Update-Godot } catch { Log ('Godot update issue: ' + $_.Exception.Message) } }
            Log ('Update check completed; log at ' + $LogFile)
        }
    }
}
catch { Log ('ERROR: ' + $_.Exception.Message); exit 1 }
finally { $env:APPDATA = $OriginalAppData; $env:LOCALAPPDATA = $OriginalLocalAppData; $env:MOONFALL_TEST_PROFILE = $OriginalTestProfile }
