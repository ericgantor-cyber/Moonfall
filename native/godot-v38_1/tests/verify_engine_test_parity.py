#!/usr/bin/env python3
"""Verify Windows and Python launch exactly the same Godot engine tests.

No dynamic evaluation: Python's literal suite map is parsed with ast.literal_eval.
"""
from pathlib import Path
import ast
import re
import sys
root = Path(__file__).resolve().parents[1]
runner = root / 'tools' / 'run_tests.py'
batch = root / 'BUILD_WINDOWS.bat'
tree = ast.parse(runner.read_text(encoding='utf-8'))
assignments = [
    node for node in ast.walk(tree) if isinstance(node, ast.Assign)
    and any(isinstance(target, ast.Name) and target.id == 'SUITES' for target in node.targets)
]
if len(assignments) != 1:
    raise SystemExit('FAIL: Expected one constant SUITES test registry')
suites = ast.literal_eval(assignments[0].value)
launches = re.findall(r'--script res://tests/([A-Za-z0-9_]+)\.gd', batch.read_text(encoding='utf-8'))
files = set(path.stem for path in (root / 'tests').glob('*.gd'))
problems = []
if len(launches) != len(set(launches)):
    problems.append('Windows batch launches duplicate tests')
if set(launches) != set(suites):
    problems.append(f'Windows-only: {sorted(set(launches)-set(suites))}; Python-only: {sorted(set(suites)-set(launches))}')
if not set(suites) <= files:
    problems.append(f'Suite sources missing: {sorted(set(suites)-files)}')
if len(suites) < 50:
    problems.append('Too few regression tests registered')
for name, marker in suites.items():
    # Most tests embed their PASS string literally. A few build markers from constant pieces.
    if not isinstance(marker, str) or not marker.endswith('PASS') or not (marker.startswith('MOON') or marker.startswith('v32.2')):
        problems.append(f'Invalid pass marker: {name}')
    script = (root / 'tests' / f'{name}.gd')
    if script.exists():
        source = script.read_text(encoding='utf-8')
        if not ('quit(' in source and ('print(' in source or 'push_error(' in source)):
            problems.append(f'Test does not report result and exit: {name}')
if problems:
    for problem in problems:
        print('FAIL:', problem)
    sys.exit(1)
print(f'PASS: {len(suites)} unique Godot test suites registered in both runners, all scripts present')
