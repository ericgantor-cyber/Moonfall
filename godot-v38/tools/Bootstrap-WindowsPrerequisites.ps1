# Moonfall v37.9 Windows prerequisite bootstrap.
# Source this script from BUILD_MOONFALL_ONE_CLICK.ps1. WinGet validates installer
# hashes against its Microsoft community package manifests; binaries are NOT bundled.
Set-StrictMode -Version Latest

function Test-MoonfallPythonExe([string]$Exe) {
    if ([string]::IsNullOrWhiteSpace($Exe) -or !(Test-Path -LiteralPath $Exe -PathType Leaf)) { return $false }
    # A Store alias is not a usable Python interpreter, and may open the Store.
    if ($Exe -match '(?i)\\Microsoft\\WindowsApps\\python(?:3)?\.exe$') { return $false }
    try {
        & $Exe -c 'import sys; sys.exit(0 if sys.version_info.major == 3 else 1)' 1>$null 2>$null
        return ($LASTEXITCODE -eq 0)
    } catch { return $false }
}

function Find-MoonfallPython3 {
    $candidates = [System.Collections.Generic.List[string]]::new()
    foreach ($v in @('313','314','312','311','310')) {
        foreach ($base in @(${env:LOCALAPPDATA}, ${env:ProgramFiles}, ${env:ProgramW6432})) {
            if (![string]::IsNullOrWhiteSpace($base)) {
                $candidates.Add((Join-Path $base "Programs\Python\Python$v\python.exe"))
                $candidates.Add((Join-Path $base "Python$v\python.exe"))
            }
        }
    }
    $cmds = @(Get-Command python.exe -All -ErrorAction SilentlyContinue)
    foreach ($cmd in $cmds) { if ($cmd.Source) { $candidates.Add($cmd.Source) } }
    $launcher = Get-Command py.exe -ErrorAction SilentlyContinue
    if ($null -ne $launcher) {
        try {
            $fromLauncher = (& $launcher.Source -3 -c 'import sys; print(sys.executable)' 2>$null | Select-Object -Last 1)
            if ($LASTEXITCODE -eq 0 -and $fromLauncher) { $candidates.Add(([string]$fromLauncher).Trim()) }
        } catch { }
    }
    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if (Test-MoonfallPythonExe $candidate) { return $candidate }
    }
    return $null
}

function Find-MoonfallInno6 {
    $candidates = [System.Collections.Generic.List[string]]::new()
    foreach ($base in @(${env:ProgramFiles(x86)}, ${env:ProgramFiles}, ${env:LOCALAPPDATA})) {
        if (![string]::IsNullOrWhiteSpace($base)) {
            $candidates.Add((Join-Path $base 'Inno Setup 6\ISCC.exe'))
            $candidates.Add((Join-Path $base 'Programs\Inno Setup 6\ISCC.exe'))
        }
    }
    $registry = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1',
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1'
    )
    foreach ($key in $registry) {
        if (Test-Path -LiteralPath $key) {
            $info = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
            if ($null -ne $info -and $info.InstallLocation) {
                $candidates.Add((Join-Path $info.InstallLocation 'ISCC.exe'))
            }
        }
    }
    $iscc = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($null -ne $iscc) { $candidates.Add($iscc.Source) }
    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if (!(Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        try {
            if ($candidate -match 'Inno Setup 6' -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
            $version = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($candidate)
            if ($version.ProductVersion -match '(^|\\D)6\\.' -or $version.FileVersion -match '(^|\\D)6\\.') { return $candidate }
        } catch { }
    }
    return $null
}

function Install-MoonfallWinGetPackage([string]$Package,[string]$Version) {
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
    if ($null -eq $winget) {
        throw "Windows Package Manager (winget) is missing. Install Microsoft's 'App Installer' (https://aka.ms/getwinget), then rerun. No executables were downloaded from unverified mirrors."
    }
    Write-Host "Installing $Package from the official WinGet catalog (internet required)..." -ForegroundColor Cyan
    $args = @('install','--id',$Package,'--exact','--source','winget','--scope','user','--silent',
              '--accept-package-agreements','--accept-source-agreements','--disable-interactivity')
    if ($Version) { $args += @('--version',$Version) }
    & $winget.Source @args
    if ($LASTEXITCODE -ne 0) { throw "WinGet install failed for $Package (exit code $LASTEXITCODE). See the error above and try again." }
}

function Ensure-MoonfallPython3 {
    $found = Find-MoonfallPython3
    if ($found) { Write-Host "Using Python 3: $found"; return $found }
    Install-MoonfallWinGetPackage 'Python.Python.3.13' ''
    # WinGet's per-user installer often updates the registry/PATH for future sessions only.
    # Find the interpreter via its installed path or Python launcher without restarting.
    $found = Find-MoonfallPython3
    if (!$found) { throw 'WinGet finished, but Python 3 cannot be located. Reopen PowerShell or repair the Python 3.13 installation.' }
    Write-Host "Python 3 installed: $found" -ForegroundColor Green
    return $found
}

function Ensure-MoonfallInno6 {
    $found = Find-MoonfallInno6
    if ($found) { Write-Host "Using Inno Setup 6: $found"; return $found }
    # Pin major version 6. Current 6.x WinGet manifest; never silently upgrade to Inno 7.
    Install-MoonfallWinGetPackage 'JRSoftware.InnoSetup' '6.7.3'
    $found = Find-MoonfallInno6
    if (!$found) { throw 'WinGet finished, but Inno Setup 6 (ISCC.exe) cannot be found. Check the installer output.' }
    Write-Host "Inno Setup 6 installed: $found" -ForegroundColor Green
    return $found
}
