#!/usr/bin/env python3
"""Offline contract: reliable one-click bootstrap and truthful release evidence.
Does not run Godot or build a Windows executable in this environment.
"""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import hashlib,re,sys
root=Path(__file__).resolve().parents[1]
ps=(root/'BUILD_MOONFALL_ONE_CLICK.ps1').read_text(encoding='utf-8')
bat=(root/'BUILD_MOONFALL_ONE_CLICK.bat').read_text(encoding='utf-8')
workflows=[(root/name).read_text(encoding='utf-8') for name in ('.github/workflows/windows-v34_4.yml','tools/github_windows_v34_4.yml')]
checks={
 'One-click Windows entrypoint': 'powershell.exe -NoProfile' in bat and 'BUILD_MOONFALL_ONE_CLICK.ps1' in bat,
 'Official version-pinned Godot download': 'https://github.com/godotengine/godot/releases/download/4.7.2-stable/' in ps,
 'Pinned archive digest': '731980f9608d61333e5baf54a2ef17210acc7a538446c0cb9969f002aca1e953' in ps and 'Require-Sha $archive' in ps and 'Expand-Archive -LiteralPath $archive' in ps,
 'Runtime checks engine version': '--headless --version' in ps and r"-notmatch '4\.7\.2'" in ps,
 'Runs all offline verifiers': 'verify_*.py' in ps and 'foreach ($check in $verifiers)' in ps and 'if ($verifiers.Count -lt 39)' in ps,
 'Official export template installer retained': "'INSTALL_GODOT_WINDOWS_TEMPLATES.ps1'" in ps,
 'Stale outputs deleted before attempt': 'foreach ($out in @($statusFile,$failureFile,$game,$installer))' in ps,
 'Stops when Godot build/test fails': 'if ($buildProcess.ExitCode -ne 0) {throw "Godot gameplay / export gate failed' in ps,
 'PE/embedded PCK verification invoked': "'VERIFY_WINDOWS_RELEASE.ps1'" in ps,
 'Installer bootstrap replaces optional dependency': 'Ensure-MoonfallInno6' in ps and '$installerBuilt = $false' in ps and 'Require-File $iscc' in ps,
 'Only successful gate writes build evidence': 'Set-Content -LiteralPath $statusFile' in ps and 'catch {' in ps and 'Remove-Item $statusFile -Force' in ps,
 'Human gameplay and 4K are never claimed': 'human_gameplay_tested = $false' in ps and 'gpu_4k_tested = $false' in ps,
 'GitHub CI source version corrected': all("gameplay_source_version = '38.0'" in t and "release_stage = 'v38.0 Windows build handoff'" in t for t in workflows),
 'New verifier included in both CI paths': all('python tests/verify_v37_8_build_handoff.py' in t for t in workflows),
 'v37.7 gameplay retained by scene inheritance': active_scene_inherits(root,'native_v37_7.gd'),
}
manifest=root/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt'
entries=[line.split('  ',1) for line in manifest.read_text().splitlines() if line.strip()]
checks['All 70 original art hashes unchanged']=len(entries)==70 and all((root/path).is_file() and hashlib.sha256((root/path).read_bytes()).hexdigest()==digest for digest,path in entries)
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v37.8 handoff checks passed')
sys.exit(0 if all(checks.values()) else 1)
