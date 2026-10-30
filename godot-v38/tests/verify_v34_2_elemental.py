#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""v34.2 source guard: visuals, element differences, performance cap, hashes."""
from pathlib import Path
import re, hashlib
R=Path(__file__).resolve().parents[1]
s=(R/'scripts/native_v34_2.gd').read_text()
q=(R/'scripts/systems/invasion_overhaul_v342.gd').read_text()
assert 'extends "res://scripts/native_v34_1.gd"' in s
assert active_scene_inherits(R, 'native_v34_4.gd')
for token in ('func _v342_begin_strike()', 'func _v342_resolve_strike()', 'func _v342_tick(', 'func _v342_hide_warnings()', 'func _v342_refresh_status()', 'func _v341_advance_if_cleared()', '_save_game()', 'func _perf_report()'):
 assert token in s, token
for token in ('MAX_WARNING_MARKERS:int=2', 'DODGE_RESONANCE:int=18', 'return 2 if site==1 else 1','func marker_radius(', 'func inside_warning(', 'func target_positions(', 'func bonus_material('):
 assert token in q, token
assert 'v342_warnings.size()*V342_RULES.MAX_WARNING_MARKERS' in s
assert 'v341_active_site<0 or v341_wave!=3' in s
assert 'if region_index!=int(v341_sites[site]["zone"])' in s
assert 'if final_blow and v341_active_site<0:' in s
assert 'if dodge_time>0.0:' in s
assert 'super._v341_activate_wave()' in s
assert 'super._v341_advance_if_cleared()' in s
build=(R/'BUILD_WINDOWS.bat').read_text()
assert 'v34_2_elemental_invasions.gd' in build
for gd_script in re.findall(r'res://(tests/[^\s\"\']+\.gd)',build):
 assert (R/gd_script).is_file(),f'Missing Godot runtime test referenced by BUILD_WINDOWS.bat: {gd_script}'
assert not re.search(r'^func _physics_process\(',s,re.M)
assert len(re.findall(r'^func (\w+)\(',s,re.M))==len(set(re.findall(r'^func (\w+)\(',s,re.M)))
assert not re.search(r'building_mode|place_structure|survival_clock',s,re.I)
for line in (R/'docs/ORIGINAL_ART_V34_SHA256.txt').read_text().splitlines():
 sha,name=line.split('  ',1)
 assert hashlib.sha256((R/name).read_bytes()).hexdigest()==sha,name
print('PASS v34.2 commander hazards, region rewards, UI, no timers, original artwork')
