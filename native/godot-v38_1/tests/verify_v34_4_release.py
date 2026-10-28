#!/usr/bin/env python3
"""Fail closed when source, export, installer, and packaging metadata diverge."""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import hashlib, re, sys
ROOT=Path(__file__).resolve().parents[1]
VERSION="34_4"
EXE=f"MoonfallNative_v{VERSION}.exe"
errors=[]
def expect(ok, message):
    if not ok: errors.append(message)
scene=(ROOT/'scenes/main.tscn').read_text()
expect(active_scene_inherits(ROOT,f'native_v{VERSION}.gd'), 'Main scene points to wrong script')
for child,parent in [('34_4','34_3'),('34_3','34_2'),('34_2','34_1'),('34_1','34_0'),('34_0','33_8'),('33_8','33_7'),('33_7','33_6'),('33_6','33_5'),('33_5','33_4'),('33_4','33_3'),('33_3','33_2'),('33_2','33_1'),('33_1','33_0'),('33_0','32_9')]:
    expect(f'extends "res://scripts/native_v{parent}.gd"' in (ROOT/f'scripts/native_v{child}.gd').read_text(), f'Broken chain {child}')
export=(ROOT/'export_presets.cfg').read_text()
match=re.search(r'export_path="([^"]+)"',export)
expect(bool(match) and Path(match.group(1)).name==EXE if match else False,'Export executable/version mismatch')
build=(ROOT/'BUILD_WINDOWS.bat').read_text()
expect(EXE in build and '--export-release' in build,'Godot official release export missing')
expect('binary_format/embed_pck=true' in export and 'EMBED_SINGLE_EXE.ps1' not in build, 'Native embedded export must use Godot release template')
expect('python tests\\verify_v34_4_release.py' in build, 'Release gate not invoked before building')
iss=(ROOT/'Moonfall_Native_v34_4_Setup.iss').read_text()
expect('#define GameVersion "38.0"' in iss and active_scene_inherits(ROOT,'native_v38_0.gd'), 'Installer AppVersion does not match current v38.0 gameplay')
expect(f'#define ExeName "{EXE}"' in iss,'Installer payload exe mismatch')
expect(f'Moonfall_Native_v{VERSION}_Windows_Setup' in iss,'Installer output version mismatch')
expect('Moonfall_Native_v34_4_Setup.iss' in (ROOT/'MAKE_WINDOWS_INSTALLER.bat').read_text(), 'Installer script reference missing')
compile_only=(ROOT/'COMPILE_INSTALLER_ONLY.bat').read_text()
expect(EXE in compile_only, 'Compile-only shortcut targets an obsolete executable')
expect(f'Moonfall_Native_v{VERSION}_Windows_Setup.exe' in compile_only, 'Compile-only shortcut targets obsolete installer')
expect('config/name="Moonfall Native v34.4"' in (ROOT/'project.godot').read_text(), 'Game title is not v34.4')
expected_path='native/moonfall-godot-v34_4/build/MoonfallNative_v34_4.exe'
subdir=(ROOT/'tools/github_windows_v34_4.yml').read_text()
expect('working-directory: native/moonfall-godot-v34_4' in subdir, 'GitHub native workflow not isolated')
expect(expected_path in subdir, 'GitHub native artifact path mismatch')
expect('native/moonfall-godot-v34_4/Release_Windows/Moonfall_Native_v34_4_Windows_Setup.exe' in subdir, 'GitHub native installer artifact path mismatch')

expect("--verify" not in (ROOT/'VERIFY_WINDOWS_RELEASE.ps1').read_text(), 'Verifier calls unsupported installer --verify option')
for rel in ('MAKE_WINDOWS_INSTALLER.bat','VERIFY_WINDOWS_RELEASE.ps1'):
    expect((ROOT/rel).is_file(),f'Missing {rel}')
manifest=ROOT/'docs/ART_SHA256_V30_1_BASELINE.txt'
for line in manifest.read_text().splitlines():
    if not line.strip(): continue
    digest,relative=line.split('  ',1)
    path=ROOT/relative
    expect(path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest()==digest.lower(),f'Artwork changed: {relative}')
expect((ROOT/'tests/v34_1_dynamic_invasions.gd').exists() and 'v34_1_dynamic_invasions.gd' in build, 'Inherited invasion runtime gate missing')
expect((ROOT/'tests/v34_2_elemental_invasions.gd').exists() and 'v34_2_elemental_invasions.gd' in build, 'Inherited elemental invasion runtime gate missing')
expect((ROOT/'tests/v34_3_lunar_war.gd').exists() and 'v34_3_lunar_war.gd' in build,'Inherited v34.3 runtime test missing')
expect((ROOT/'.github/workflows/windows-v34_4.yml').exists(), 'Current v34.4 Windows CI workflow missing')
workflow=(ROOT/'.github/workflows/windows-v34_4.yml').read_text()
expect('python tests/verify_ci_release.py' in workflow, 'CI contract test is not invoked')
expect((ROOT/'tests/v34_4_fair_fight.gd').exists() and 'v34_4_fair_fight.gd' in build, 'v34.4 runtime test missing')
if errors:
    print('FAIL v34.4 release gate:')
    for e in errors: print(' - '+e)
    sys.exit(1)
print('PASS v34.4 source, Windows metadata, inherited artwork integrity')
