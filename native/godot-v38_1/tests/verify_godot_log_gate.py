#!/usr/bin/env python3
"""Offline proof that the release log checker rejects false-positive PASS banners."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
from check_godot_logs import expected_release_logs

scanner = ROOT / 'tests' / 'check_godot_logs.py'
runner = (ROOT / 'BUILD_WINDOWS.bat').read_text(encoding='utf-8')
workflows = [(ROOT / '.github/workflows/windows-v34_4.yml'),
             (ROOT / 'tools/github_windows_v34_4.yml')]


def verify_log(label: str, body: str, should_pass: bool) -> bool:
    with tempfile.TemporaryDirectory(prefix='moonfall-log-test-') as directory:
        path = Path(directory) / 'GODOT_TEST_LOG.txt'
        path.write_text(body, encoding='utf-8')
        result = subprocess.run([sys.executable, str(scanner), '--files', str(path)],
                                capture_output=True, text=True, timeout=10)
        good = (result.returncode == 0) == should_pass
        print(('PASS' if good else 'FAIL') + ': ' + label)
        return good


checks = [
    verify_log('clean PASS stays accepted', 'Godot Engine 4.7.2\nMOONFALL TEST PASS\n', True),
    verify_log('PASS plus SCRIPT ERROR is rejected', 'MOONFALL TEST PASS\nSCRIPT ERROR: Invalid call\n', False),
    verify_log('PASS plus silent ERROR is rejected', 'MOONFALL TEST PASS\nERROR: failed to load texture\n', False),
    verify_log('parse errors are rejected', 'Parse Error: Identifier missing\n', False),
    verify_log('fatal engine errors are rejected', 'FATAL: cannot initialize resource\n', False),
]
try:
    expected = expected_release_logs(runner)
    checks.append(len(expected) >= 63)
    print(('PASS' if checks[-1] else 'FAIL') + f': {len(expected)} distinct expected release logs')
except ValueError as error:
    checks.append(False)
    print('FAIL: ' + str(error))

missing = subprocess.run([sys.executable, str(scanner), '--files',
                          str(ROOT / 'tests' / 'nonexistent-godot-engine-test.log')],
                         capture_output=True, text=True, timeout=10)
checks.append(missing.returncode != 0)
print(('PASS' if checks[-1] else 'FAIL') + ': missing log fails closed')
checks.append('python tests\\check_godot_logs.py' in runner and
              runner.index('python tests\\check_godot_logs.py') < runner.index('echo SUCCESS:'))
print(('PASS' if checks[-1] else 'FAIL') + ': Windows release blocks before success if logs have errors')
checks.append(all('python tests/verify_godot_log_gate.py' in path.read_text()
                  for path in workflows))
print(('PASS' if checks[-1] else 'FAIL') + ': both GitHub workflows run the verifier')
raise SystemExit(0 if all(checks) else 1)
