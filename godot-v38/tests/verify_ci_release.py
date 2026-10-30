#!/usr/bin/env python3
"""Ensure the Windows CI workflow builds, installs, and publishes the active version."""
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
version='34_4'
w=R/'.github/workflows'/f'windows-v{version}.yml'
errors=[]
if not w.is_file():errors.append('Missing current CI workflow')
else:
 t=w.read_text()
 for token in (f'MoonfallNative_v{version}.exe',f'Moonfall_Native_v{version}_Windows_Setup.exe',f'Moonfall-v{version}-single-exe',f'Moonfall-v{version}-windows-installer','BUILD_WINDOWS.bat','verify_v34_4_release.py','VERIFY_WINDOWS_RELEASE.ps1'):
  if token not in t:errors.append('CI missing '+token)
 if 'v31_3' in t:errors.append('Old v31_3 artifacts in workflow')
 if 'Moonfall_Native_v34_4_Setup.iss' not in t:errors.append('Actual installer script reference absent')
 if 'pull_request:' not in t:errors.append('Pull request validation trigger missing')
 if 'windows-2022' not in t:errors.append('Windows runner missing')
if errors:
 print('FAIL CI release verification: '+'; '.join(errors));sys.exit(1)
print('PASS CI release version, executable, installer, artifacts and smoke-test references')
