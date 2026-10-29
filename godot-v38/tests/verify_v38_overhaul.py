#!/usr/bin/env python3
"""Moonfall v38 offline source, bloat, artwork, build and test integrity gate.
Not a substitute for executing Godot or Windows installers.
"""
from pathlib import Path
import hashlib,ast,re,sys
from active_scene_guard import active_scene_inherits
root=Path(__file__).resolve().parents[1]
active=(root/'scripts/native_v38_0.gd').read_text()
pose=(root/'scripts/native_v37_4.gd').read_text()
rules=(root/'scripts/systems/overhaul_v380.gd').read_text()
runner=(root/'tools/run_tests.py').read_text()
bat=(root/'BUILD_WINDOWS.bat').read_text()
manifest=(root/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines()
assets=[line.split('  ',1) for line in manifest if line.strip()]
module=ast.parse(runner)
suites=next(ast.literal_eval(n.value) for n in ast.walk(module) if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SUITES' for t in n.targets))
checks={
 'Main scene runs v38 while preserving previous v37 scripts':active_scene_inherits(root,'native_v38_0.gd') and active_scene_inherits(root,'native_v37_7.gd'),
 'Swim pose is decoupled from hero sprites and never cumulative':all(x in pose for x in ['hero_sprite.reparent(v374_swim_pose_anchor,true)','v374_swim_pose_anchor.rotation.z=','v374_swim_pose_anchor.scale=Vector3(','v374_swim_pose_anchor.scale=Vector3.ONE']),
 'Sprite texture is never replaced by new code':'.texture=' not in active and '.texture=' not in pose,
 'Safe F2 status shortcut and correct default':all(x in active for x in ['KEY_F2','v380_status_enabled:bool=false','not menu_open','get_viewport().set_input_as_handled()']),
 'HUD is optional and bounded to <=4hz updates': 'const UPDATE_PERIOD:float=0.25' in rules and 'if v380_update_elapsed<V380_RULES.UPDATE_PERIOD:return' in active,
 'Critical oxygen and combat/health warning priorities':all(x in rules for x in ['CRITICAL AIR','LOW AIR','COMBAT','LOW HEALTH']),
 'Status hides during menus/death/title': 'v380_status_enabled and hero_playing and not death_active and not menu_open' in active,
 'Current title version updated': 'MOONFALL v38.0' in active,
 'System tab explains F2 without a new game mode':'if which=="System"' in active and 'F2 toggles' in active,
 'Actual Godot test registered by both launchers':"'v38_0_overhaul':" in runner and 'res://tests/v38_0_overhaul.gd' in bat,
 'Python runner and Windows batch have 66 matching suites':len(suites)==66 and set(re.findall(r'--script res://tests/(\w+)\.gd',bat))==set(suites),
 'Windows build detects new errors in v38 log':'GODOT_V38_0_OVERHAUL_LOG.txt' in bat and r'python tests\check_godot_logs.py' in bat,
 'Source prerequisites and installer still pinned':all(x in (root/'BUILD_MOONFALL_ONE_CLICK.ps1').read_text() for x in ['Ensure-MoonfallPython3','Ensure-MoonfallInno6','engine_test_suites_registered = 66']),
 'Art hash inventory unchanged':len(assets)==70 and all((root/rel).is_file() and hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest for digest,rel in assets),
 'No duplicate functions in v38':len(re.findall(r'^func (\w+)\(',active,re.M))==len(set(re.findall(r'^func (\w+)\(',active,re.M))),
 'No new currencies, mode switches or global survival clocks':all(x not in active+rules for x in ['new_currency','survival_timer','new_game_mode','func _save_game(']),
}
for label,okay in checks.items():print(('PASS' if okay else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v38 source gates passed')
sys.exit(0 if all(checks.values()) else 1)
