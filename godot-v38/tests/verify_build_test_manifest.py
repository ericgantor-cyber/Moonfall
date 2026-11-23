#!/usr/bin/env python3
"""Check the real Windows Godot test inventory, not just its count."""
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
bat=(root/'BUILD_WINDOWS.bat').read_text()
windows_workflow=(root.parent/'.github/workflows/godot-v38-windows-installer.yml').read_text(encoding='utf-8')
commands=re.findall(r'--script res://(tests/[A-Za-z0-9_]+\.gd)',bat)
missing=[path for path in commands if not (root/path).is_file()]
dupes=sorted({x for x in commands if commands.count(x)>1})
missing_markers=[]
for path in commands:
    test=(root/path).read_text()
    if not re.search(r'\b(?:print|push_error)\(',test):missing_markers.append(path)
# Source and policy guards must precede the actual Godot import and runtime sequence.
checks={
    "All Godot test scripts exist":not missing,
    "No duplicate headless test launches":not dupes,
    "Runtime tests emit diagnostic messages":not missing_markers,
    "The invasion guard runs":'tests/v34_4_invasion_guard.gd' in commands,
    "The 4K regression runs":'tests/v34_4_4k_display.gd' in commands,
    "Windows export executes after tests":bat.find('v34_4_invasion_guard.gd')<bat.find('--export-release "Windows Desktop"'),
    "Windows export uses official template":'GODOT_TEMPLATE_SOURCE' in bat and '--export-release "Windows Desktop"' in bat,
    "No obsolete editor embedding":'EMBED_SINGLE_EXE.ps1' not in bat,
    "Windows inspects the playable scene root script": "Get-Content 'scenes/main.tscn'" in windows_workflow and '$rootScript.Groups[1].Value' in windows_workflow and 'VALIDATING PLAYABLE ROOT SCRIPT:' in windows_workflow,
    "Windows rejects circular and missing inherited scripts": 'Circular script inheritance detected' in windows_workflow and 'Missing inherited script:' in windows_workflow,
    "Windows fails on resource import errors": 'Godot resource import failed before script-chain validation' in windows_workflow,
    "Godot preflight subprocesses have bounded wait and tree kill": all(
        token in windows_workflow for token in (
            'function Invoke-GodotChecked', 'WaitForExit($TimeoutSeconds * 1000)',
            '$process.Kill($true)', '-TimeoutSeconds 600', '-TimeoutSeconds 180'
        )
    ),
    "Playable leaf compiled first, ancestor diagnosis only on failure": all(
        token in windows_workflow for token in (
            'VERIFY FULL PLAYABLE SCRIPT CHAIN:', 'foreach ($ancestor in $chain)',
            'FIRST FAILING SCRIPT:', 'PASS: playable inheritance chain checked'
        )
    ) and windows_workflow.index('VERIFY FULL PLAYABLE SCRIPT CHAIN:') <
        windows_workflow.index('foreach ($ancestor in $chain)'),
    "Live-panel Windows tests use supervised isolated runner": all(
        f"python tools\\run_tests.py --script res://tests/{name}.gd" in bat
        and f'findstr /c:"PASS {name}"' in bat
        for name in ("talent_world_live", "panels_world_live")
    ),
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
if missing:print('Missing:',missing)
if dupes:print('Duplicate:',dupes)
if missing_markers:print('Without diagnostics:',missing_markers)
sys.exit(0 if all(checks.values()) else 1)
