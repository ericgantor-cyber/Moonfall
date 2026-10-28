#!/usr/bin/env python3
"""Regression check: upload Godot alongside the existing native/dx12-prototype."""
from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
p=(r/'PUBLISH_NATIVE_TO_GITHUB.ps1').read_text()
w=(r/'tools/github_windows_v34_4.yml').read_text()
checks={
  "Keeps nested workflow validation files": ".github" not in p.split("$skip = ",1)[1].split("\n",1)[0],
  'Dedicated non-colliding native folder': "Join-Path $folder 'native\\moonfall-godot-v34_4'" in p,
  'No overwrite of existing workflow': 'if (Test-Path $workflowPath)' in p,
  'No rejection of existing parent native folder': "if (Test-Path $dest)" in p and 'native folder already exists' not in p,
  'Adds only isolated native content': "git -C $folder add 'native/moonfall-godot-v34_4'" in p,
  'Source uses temporary clone and feature branch': 'git clone --depth 1' in p and 'git -C $folder switch -c $Branch' in p,
  'Does not overwrite main branch': 'git -C $folder push -u origin $Branch' in p,
  'CI uses isolated working directory': 'working-directory: native/moonfall-godot-v34_4' in w,
  'CI triggers for isolated source': 'native/moonfall-godot-v34_4/**' in w,
  'CI publishes portable game': 'native/moonfall-godot-v34_4/build/MoonfallNative_v34_4.exe' in w,
  'CI publishes installer': 'native/moonfall-godot-v34_4/Release_Windows/Moonfall_Native_v34_4_Windows_Setup.exe' in w,
  'CI invokes this validation': 'tests/verify_publish_handoff.py' in w,
}
for label,good in checks.items():print(('PASS' if good else 'FAIL')+' publish: '+label)
sys.exit(0 if all(checks.values()) else 1)
