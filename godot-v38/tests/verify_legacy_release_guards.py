#!/usr/bin/env python3
"""Old gameplay feature tests must validate the live game instead of an obsolete v31/v32 label."""
from pathlib import Path
import re
import sys
root=Path(__file__).resolve().parents[1]
legacy=[]
for p in (root/'tests').glob('*.gd'):
    if p.name=='current_release.gd':continue
    t=p.read_text(encoding='utf-8')
    if re.search(r'game\.call\("_release_label"\)\s*(?:in|==)',t):legacy.append(p.name)
    if 'metrics.get("version")=="v32.2"' in t:legacy.append(p.name)
checks={
 'No obsolete exact-release comparisons in historical suites':not legacy,
 'Shared helper verifies the project version': 'ProjectSettings.get_setting("application/config/name"' in (root/'tests/current_release.gd').read_text(),
 'v32.2 chamber regression included in both runners': 'v32_2_lean_chambers.gd' in (root/'BUILD_WINDOWS.bat').read_text() and "'v32_2_lean_chambers'" in (root/'tools/run_tests.py').read_text(),
}
for label,good in checks.items():print(('PASS' if good else 'FAIL')+': '+label)
if legacy:print('Legacy failures:',sorted(set(legacy)))
sys.exit(not all(checks.values()))
