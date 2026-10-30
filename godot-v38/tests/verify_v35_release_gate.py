#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Verify readiness checks are wired before a Windows installer is ever called tested."""
from pathlib import Path
import re
import sys
root = Path(__file__).resolve().parents[1]
workflows = [root/'.github/workflows/windows-v34_4.yml', root/'tools/github_windows_v34_4.yml']
bat=(root/'BUILD_WINDOWS.bat').read_text(encoding='utf-8')
runner=(root/'tools/run_tests.py').read_text(encoding='utf-8')
checks = {
 'Godot test parity checked before runtime': 'verify_engine_test_parity.py' in bat and bat.index('verify_engine_test_parity.py')<bat.index('--editor --path') if '--editor --path' in bat else 'verify_engine_test_parity.py' in bat and bat.index('verify_engine_test_parity.py')<bat.index('--import --quit'),
 'Real gameplay run before Windows export': bat.index('v34_4_invasion_guard.gd') < bat.index('--export-release'),
 'Windows exit status required': 'if errorlevel 1 goto failed' in bat,
 'Both 4K and invasion tests registered': all(t in runner for t in ('v34_4_4k_display','v34_4_invasion_guard')),
 'All CI workflows validate test parity': all('python tests/verify_engine_test_parity.py' in p.read_text() for p in workflows),
 'All CI workflows record evidence after installer smoke': all(p.read_text().index('Smoke test installed game and uninstall') < p.read_text().index('Record verified Windows build evidence') for p in workflows),
 'Hashes generated for both built artifacts': all('game_sha256' in p.read_text() and 'installer_sha256' in p.read_text() for p in workflows),
 'CI never claims human gameplay/4K was tested': all('real_gpu_4k_tested = $false' in p.read_text() and 'actual_human_gameplay_tested = $false' in p.read_text() for p in workflows),
 'Artwork reference is present': (root/'docs/ART_SHA256_V30_1_BASELINE.txt').is_file(),
 'One-world project main scene stays unchanged': active_scene_inherits(root, 'native_v34_4.gd'),
}
for label,ok in checks.items(): print(('PASS' if ok else 'FAIL')+': '+label)
if not all(checks.values()):sys.exit(1)
