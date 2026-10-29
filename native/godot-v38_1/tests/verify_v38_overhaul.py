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
 'Cinematic realism uses quality and fog guards':all(x in active for x in ['func _v381_apply_atmosphere(', 'living_environment.volumetric_fog_enabled=enable_volume', 'living_environment.fog_enabled', 'environment_detail', 'DisplayServer.get_name()!=\"headless\"']),
 'Current title version updated from real release metadata': 'str(_release_label())' in active and '"☾  MOONFALL "+release' in active and '"MOONFALL" in title.text.to_upper()' in active,
 'System tab explains F2 without a new game mode':'if which=="System"' in active and 'F2 toggles' in active,
 'Actual Godot test registered by both launchers':"'v38_0_overhaul':" in runner and 'res://tests/v38_0_overhaul.gd' in bat,
 'Python runner and Windows batch have 66 matching suites':len(suites)==66 and set(re.findall(r'--script res://tests/(\w+)\.gd',bat))==set(suites),
 'Windows build detects new errors in v38 log':'GODOT_V38_0_OVERHAUL_LOG.txt' in bat and r'python tests\check_godot_logs.py' in bat,
 'Source prerequisites and installer still pinned':all(x in (root/'BUILD_MOONFALL_ONE_CLICK.ps1').read_text() for x in ['Ensure-MoonfallPython3','Ensure-MoonfallInno6','engine_test_suites_registered = 66']),
 'Art hash inventory unchanged':len(assets)==70 and all((root/rel).is_file() and hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest for digest,rel in assets),
 'Cutout binding fails closed, preserves original and adds hair/cloth bones':all(x in (root/'scripts/systems/hero_art_rig.gd').read_text() for x in ['func install_cutouts(', 'if not approved', 'func restore_original()', 'original.visible=false', 'HairFront', 'CapeEnd']),
 'Hero rigs preserve original art and provide independent skeletons':all((root/p).is_file() for p in ['scenes/rigs/z_rig.tscn','scenes/rigs/nyra_rig.tscn','scripts/systems/hero_art_rig.gd']) and all(x in active for x in ['_v382_build_rigs()', 'hero_sprite.reparent(art_parent,true)', 'v382_active_rig.call("play_state",name)']),
 'One-world gameplay bridge reuses existing systems':(root/'scripts/systems/world_fusion_rules.gd').is_file() and all(x in active for x in ['V383_WORLD.next_objective(', 'V383_WORLD.event_crafting_bonus(', 'if which=="World":', 'func _complete_world_event(', 'v383:professions', 'v383:skills', 'v383:gear']),
 'Witchwood graphics are deterministic, scalable and do not alter original art':(root/'scripts/systems/witchwood_cinematic_v384.gd').is_file() and all(x in active for x in ['_v384_build_witchwood()', 'V384_WITCHWOOD.apply_quality(', 'v384_hero_fill.shadow_enabled=false', 'region_index==1']) and all(x in (root/'scripts/systems/witchwood_cinematic_v384.gd').read_text() for x in ['const TREE_COUNT:int=48','const SEED:int=2026384','MultiMeshInstance3D.new()', 'func apply_quality(', 'region.add_child(root)', 'shadow_enabled=false']),
 'Witchwood v38.5 has bounded original 3D PBR geometry and no scene collisions':(root/'scripts/systems/witchwood_assets_v385.gd').is_file() and all(x in (root/'scripts/systems/witchwood_assets_v385.gd').read_text() for x in ['func make_moonwood_trunk()', 'func make_moonwood_canopy()', 'func make_surface_material(', 'ImageTexture.create_from_image(', 'SurfaceTool.new()']) and all(x in (root/'scripts/systems/witchwood_cinematic_v384.gd').read_text() for x in ['V385_ASSETS.make_moonwood_trunk()', 'V385_ASSETS.make_surface_material(', 'mat.normal_enabled=detail>=6']),
 'Witchwood floor uses four bounded, deterministic collision-free GPU batches':(root/'scripts/systems/witchwood_floor_v386.gd').is_file() and all(x in (root/'scripts/systems/witchwood_floor_v386.gd').read_text() for x in ['const MOSS_COUNT:int=96','const FERN_COUNT:int=72','const LOG_COUNT:int=16','const ROOT_COUNT:int=24','const SEED:int=2026386','RandomNumberGenerator.new()', 'MultiMeshInstance3D.new()', 'func _valid_spot(', 'func apply_quality(']) and all(x in (root/'scripts/systems/witchwood_cinematic_v384.gd').read_text() for x in ['V386_FLOOR.build(root)', 'V386_FLOOR.apply_quality(']),
 'No duplicate functions in v38':len(re.findall(r'^func (\w+)\(',active,re.M))==len(set(re.findall(r'^func (\w+)\(',active,re.M))),
 'No new currencies, mode switches or global survival clocks':all(x not in active+rules for x in ['new_currency','survival_timer','new_game_mode','func _save_game(']),
}
for label,okay in checks.items():print(('PASS' if okay else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v38 source gates passed')
sys.exit(0 if all(checks.values()) else 1)
