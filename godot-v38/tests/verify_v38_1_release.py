#!/usr/bin/env python3
"""v38.1 release safety contracts; no claim of Windows/Godot execution."""
from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
main=(root/'BUILD_MOONFALL_ONE_CLICK.ps1').read_text()
installer=(root/'Moonfall_Native_v34_4_Setup.iss').read_text()
checks={
 'Godot batch is supervised with 60-min timeout': all(x in main for x in [
    '$buildProcess = Start-Process', '$buildProcess.WaitForExit($buildLimitMinutes * 60 * 1000)',
    '$buildLimitMinutes = 60', 'taskkill.exe /PID $buildProcess.Id /T /F']),
 'Nonzero exit and timeout both stop the release': all(x in main for x in [
    'if ($buildProcess.ExitCode -ne 0)', 'Godot build timed out after', 'Godot gameplay / export gate failed']),
 'New success status and failure report are distinct': all(x in main for x in [
    'BUILD_STATUS_v38_1.json','BUILD_FAILURE_v38_1.txt','Set-Content -LiteralPath $failureFile']),
 'Partial outputs and success status removed on failure': all(x in main for x in [
    'if (Test-Path $statusFile) {Remove-Item $statusFile -Force}',
    'foreach ($out in @($game,$installer))']),
 'Installer version reflects gameplay': '#define GameVersion "38.0"' in installer and
    '#define ExeName "MoonfallNative_v34_4.exe"' in installer,
 'No claim of real GPU or human test': 'gpu_4k_tested = $false' in main and 'human_gameplay_tested = $false' in main,
 'Build-error diagnostic collector excludes saves': all(x in (root/'COLLECT_BUILD_LOGS.ps1').read_text() for x in [
    "'GODOT_*_LOG.txt'",'BUILD_FAILURE_v38_1.txt','Compress-Archive','No game saves or user credentials']),
 'Offline suite still includes all previous 66 Godot tests':len(re.findall(r'--script res://tests/[\w]+\.gd', (root/'BUILD_WINDOWS.bat').read_text()))==66,
}
for file in [root/'.github/workflows/windows-v34_4.yml',root/'tools/github_windows_v34_4.yml']:
 text=file.read_text()
 sequence=re.findall(r'^\s*python tests/verify_[\w]+\.py\s*\n([^\n]*)',text,re.M)
 checks['GitHub workflow '+file.name+' checks every verifier exit']=len(sequence)>20 and all('if ($LASTEXITCODE -ne 0)' in line for line in sequence)
 checks['GitHub workflow '+file.name+' includes new guard']='python tests/verify_v38_1_release.py' in text
manifest=root/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt'
entries=[line.split('  ',1) for line in manifest.read_text().splitlines() if line.strip()]
checks['Original 70 artwork files unaltered']=len(entries)==70 and all((root/path).is_file() and hashlib.sha256((root/path).read_bytes()).hexdigest()==digest for digest,path in entries)
for label,result in checks.items():print(('PASS' if result else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v38.1 release gate checks')
raise SystemExit(0 if all(checks.values()) else 1)
