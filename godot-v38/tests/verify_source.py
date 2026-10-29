#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Current Moonfall source, artwork and build contract verification (offline)."""
from pathlib import Path
import hashlib, re, sys
root = Path(__file__).resolve().parents[1]
errors = []
def check(condition, reason):
    if not condition: errors.append(reason)
required = [
    'project.godot','scenes/main.tscn','scripts/native_v11.gd',
    'tools/updates/Update-DevTools.ps1','tools/run_tests.py',
    'scripts/native_v30_7.gd','scripts/native_v30_8.gd','scripts/native_v31_0.gd','scripts/native_v31_1.gd','tests/v31_1_realism.gd','scripts/native_v31_2.gd','scripts/native_v31_4.gd','tests/v31_4_bestiary.gd','scripts/native_v31_5.gd','scripts/systems/creature_mesh_v315.gd','tests/v31_5_living_meshes.gd','tests/v31_6_anatomy_ai.gd','tests/v31_6_second_pass.gd','scripts/native_v31_6.gd','scripts/native_v32_0.gd','scripts/native_v32_1.gd','scripts/systems/dungeon_stage2_rules.gd','tests/v32_1_world_reborn_stage2.gd','scripts/systems/world_reborn_rules.gd','tests/v32_0_world_reborn.gd','tests/v32_0_stream_budget.gd','scripts/systems/anatomy_v316.gd','tests/v31_2_visual_performance.gd','tests/v31_2_presets.gd','scripts/systems/graphics_rules.gd','tests/v31_0_graphics.gd','scripts/systems/money_rules.gd','tests/v30_6_money_economy.gd','tests/v30_7_forge_treasure.gd','tests/v30_8_shop_ui.gd','scripts/systems/forge_rules.gd','tests/v30_1_profiler.gd','tests/v30_4_inventory_rules.gd','scripts/systems/inventory_rules.gd','BUILD_WINDOWS.bat','INSTALL_GODOT_WINDOWS_TEMPLATES.ps1',
    'Moonfall_Native_v34_4_Setup.iss','.vscode/tasks.json',
]
for name in required: check((root/name).is_file(), f'Missing {name}')
if (root/'scripts/native_v11.gd').exists():
    script = (root/'scripts/native_v11.gd').read_text(encoding='utf-8')
    names = re.findall(r'^func\s+(\w+)\(',script,re.M)
    check(len(names)==len(set(names)), 'Duplicate top-level gameplay function names')
if (root/'scenes/main.tscn').exists():
    check(active_scene_inherits(root, 'native_v34_4.gd'), 'Scene is not running current v34.4 gameplay script')
    check('native_v30_3.gd' in (root/'scripts/native_v30_4.gd').read_text(encoding='utf-8'), 'Missing v30.3 compatibility inheritance')
if (root/'BUILD_WINDOWS.bat').exists():
    build = (root/'BUILD_WINDOWS.bat').read_text(encoding='utf-8')
    check('MoonfallNative_v34_4.exe' in build, 'Windows EXE filename mismatch')
    check('--export-release' in build and 'EMBED_SINGLE_EXE.ps1' not in build, 'Official Windows release export missing')
    check('v30_1_profiler.gd' in build, 'Missing latest profiler test')
    check('v30_4_inventory_rules.gd' in build, 'Missing inventory regression test')
    check('v30_5_inventory_expansion.gd' in build, 'Missing expanded inventory regression test')
    check('v30_6_money_economy.gd' in build, 'Missing money and merchant regression test')
    check('v31_6_second_pass.gd' in build and 'v31_6_anatomy_ai.gd' in build and 'v31_5_living_meshes.gd' in build and 'v30_8_shop_ui.gd' in build and 'v31_1_realism.gd' in build and 'v31_2_visual_performance.gd' in build and 'v31_2_presets.gd' in build, 'Missing shop/graphics regressions')
    check('v32_0_world_reborn.gd' in build and 'v32_0_stream_budget.gd' in build and 'v32_1_world_reborn_stage2.gd' in build, 'Missing world and dungeon regression gates')
if (root/'Moonfall_Native_v34_4_Setup.iss').exists():
    install = (root/'Moonfall_Native_v34_4_Setup.iss').read_text(encoding='utf-8')
    check('MoonfallNative_v34_4.exe' in install, 'Installer is not for current game')
manifest = root/'docs/ART_SHA256_V30_1_BASELINE.txt'
if manifest.exists():
    expected={}
    for line in manifest.read_text(encoding='utf-8').splitlines():
        if not line.strip(): continue
        sha, relative = line.split('  ',1)
        expected[relative] = sha.lower()
    actual={}
    for file in (root/'assets').rglob('*'):
        if file.is_file() and file.suffix.lower() in ('.png','.webp','.jpg','.jpeg'):
            actual[file.relative_to(root).as_posix()] = hashlib.sha256(file.read_bytes()).hexdigest()
    check(all(actual.get(k)==v for k,v in expected.items()) and len(expected)==29, 'Inherited artwork hashes changed')
    check(len(list((root/'assets/v31_realism').glob('*.png')))==14, 'Realism material textures missing')
    check(len(list((root/'assets/v31_4_bestiary').glob('*.png')))==18, 'v31.4 illustrated assets missing')
else: check(False, 'Missing artwork hash baseline')
# Keep the user-requested no-survival-stopwatch UI policy enforced in offline tests.
import subprocess
no_timer = subprocess.run([sys.executable, str(root/'tests/verify_no_survival_timer.py')],
                          text=True, capture_output=True, check=False)
check(no_timer.returncode == 0, 'Survival stopwatch regression: ' + no_timer.stdout + no_timer.stderr)
if errors:
    print('SOURCE CHECK FAILED:')
    for e in errors: print(' - '+e)
    sys.exit(1)
print('PASS current build scripts, game source functions, updater, and all 29 inherited artwork hashes plus 14 new textures')
