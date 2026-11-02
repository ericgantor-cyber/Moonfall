#!/usr/bin/env python3
"""Offline contract for automatic trusted Windows prerequisites.
Cannot execute WinGet, PowerShell or Godot on non-Windows machines.
"""
from pathlib import Path
import hashlib
import sys

root=Path(__file__).resolve().parents[1]
boot=(root/'tools/Bootstrap-WindowsPrerequisites.ps1').read_text('utf-8')
main=(root/'BUILD_MOONFALL_ONE_CLICK.ps1').read_text('utf-8')
entry=(root/'BUILD_MOONFALL_ONE_CLICK.bat').read_text('utf-8')
checks={
 'Dedicated bundled bootstrap script':bool(boot) and '. (Join-Path $project \'tools\\Bootstrap-WindowsPrerequisites.ps1\')' in main,
 'Windows Python package is version-pinned to 3.13':"Install-MoonfallWinGetPackage 'Python.Python.3.13' ''" in boot,
 'Inno Setup is strictly version-pinned to 6.7.3':"Install-MoonfallWinGetPackage 'JRSoftware.InnoSetup' '6.7.3'" in boot,
 'WinGet source and exact package identity':all(s in boot for s in ["'--exact'", "'--source','winget'", "'--accept-package-agreements'", "'--accept-source-agreements'"]),
 'WinGet exit code is checked':"if ($LASTEXITCODE -ne 0) { throw \"WinGet install failed" in boot,
 'Missing WinGet gives official App Installer link': 'https://aka.ms/getwinget' in boot,
 'Python Store alias rejected': 'WindowsApps' in boot,
 'Python executables are verified before use': 'Test-MoonfallPythonExe' in boot and 'sys.version_info.major == 3' in boot,
 'Python PATH refreshed for the legacy batch runner': "(Split-Path -Parent $pythonExe) + [IO.Path]::PathSeparator + $env:PATH" in main,
 'Inno 6 compiler detection verifies trusted install path or 6.x version': all(s in boot for s in ["$candidate -match 'Inno Setup 6'", '$version.ProductVersion', '$version.FileVersion']),
 'User and machine install locations checked': all(s in boot for s in ['HKCU:', 'HKLM:', 'InstallLocation']),
 'Both installers checked before Godot downloads': main.index('Ensure-MoonfallPython3') < main.index('Ensure-MoonfallInno6') < main.index('Invoke-WebRequest -Uri $url'),
 'Successful normal build requires installer': 'Require-File $iscc' in main and 'Require-File $installer' in main and 'Inno Setup 6 not found' not in main,
 'Explicit portable-only exception preserved': 'if (!$PortableOnly)' in main and 'param([switch]$PortableOnly)' in main,
 'New evidence file distinct from previous run': 'BUILD_STATUS_v38_1.json' in main and 'python_version' in main and 'inno_compiler' in main,
 'Script cannot claim human testing': 'human_gameplay_tested = $false' in main and 'gpu_4k_tested = $false' in main,
 'Existing image asset hashes preserved': (lambda entries: len(entries)==70 and all((root/p).is_file() and hashlib.sha256((root/p).read_bytes()).hexdigest()==h for h,p in entries))([line.split('  ',1) for line in (root/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines() if line.strip()]),
 'Double-click launcher remains available':'BUILD_MOONFALL_ONE_CLICK.ps1' in entry,
}
for name, okay in checks.items():print(('PASS' if okay else 'FAIL')+': '+name)
print(f'{sum(checks.values())}/{len(checks)} v37.9 prerequisite checks passed')
sys.exit(0 if all(checks.values()) else 1)
