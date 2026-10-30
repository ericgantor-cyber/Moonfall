#!/usr/bin/env python3
"""Offline safety/layout assertions for Moonfall's Windows-only update tooling.

These checks DO NOT execute Windows PowerShell or simulate an actual download.
"""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
script=(root/'tools/updates/Update-DevTools.ps1').read_text(encoding='utf-8')
required=[
 'https://api.github.com/repos/godotengine/godot/releases?per_page=50',
 r"'^4\.7$'", 'godot/releases/download/', 'sha256:',
 'Test-GodotCandidate', 'MOONFALL_TEST_PROFILE',
 "'.godot', '.git', 'build'", 'Copy-Item', 'active_godot.txt',
 "Microsoft.VisualStudioCode", "-Mode Update", "Register-ScheduledTask",
 "Unregister-ScheduledTask", 'ValidatePattern',
 'MOONFALL v30.1 PROFILER PASS', '120',
]
errors=[f'Missing updater contract: {token}' for token in required if token not in script]
for file in ('CHECK_DEV_UPDATES.bat','UPDATE_DEV_TOOLS_NOW.bat','ENABLE_DEV_AUTO_UPDATES.bat','DISABLE_DEV_AUTO_UPDATES.bat'):
    if not (root/file).is_file(): errors.append('Missing wrapper: '+file)
tasks=json.loads((root/'.vscode/tasks.json').read_text(encoding='utf-8'))
labels=[task['label'] for task in tasks['tasks']]
if len(labels)!=len(set(labels)):errors.append('Duplicate VS Code task labels')
for required_label in ('Moonfall: Run all Godot tests','Moonfall: Check Godot and VS Code updates','Moonfall: Disable weekly dev tool updates'):
    if required_label not in labels:errors.append('Missing VS Code task: '+required_label)
if 'v30_1_profiler' not in (root/'tools/run_tests.py').read_text(encoding='utf-8'):
    errors.append('Latest profiler regression is missing from the test runner')
if errors:
    print('FAILED OFFLINE UPDATER CHECK')
    print('\n'.join(errors))
    raise SystemExit(1)
print('PASS offline updater safety/reference checks; Windows PowerShell execution NOT tested')
