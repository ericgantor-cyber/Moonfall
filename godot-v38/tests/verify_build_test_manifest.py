#!/usr/bin/env python3
"""Check the real Windows Godot test inventory, not just its count."""
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
bat=(root/'BUILD_WINDOWS.bat').read_text()
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
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
if missing:print('Missing:',missing)
if dupes:print('Duplicate:',dupes)
if missing_markers:print('Without diagnostics:',missing_markers)
sys.exit(0 if all(checks.values()) else 1)
