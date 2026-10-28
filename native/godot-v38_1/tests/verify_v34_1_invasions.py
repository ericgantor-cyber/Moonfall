#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Invasion-specific source checks. Does not replace Godot runtime testing."""
from pathlib import Path
import hashlib,re
r=Path(__file__).resolve().parents[1]
s=(r/'scripts/native_v34_1.gd').read_text()
q=(r/'scripts/systems/lunar_invasions_v341.gd').read_text()
scene=(r/'scenes/main.tscn').read_text()
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')
assert 'extends "res://scripts/native_v34_1.gd"' in (r/'scripts/native_v34_2.gd').read_text()
assert 'extends "res://scripts/native_v34_0.gd"' in s
assert 'const REGIONS:Array[int]=[1,3,5]' in q
assert 'const PRESSURE_TRIGGER:int=4' in q
assert 'const SOLDIER_SLOTS:int=3' in q
assert 'const ACTIVE_INVADERS_MAX:int=3' in q
assert 'return 2 if wave==1 else SOLDIER_SLOTS if wave==2 else 1 if wave==3 else 0' in q
assert 'func _v341_activate_wave()->void:' in s
assert 'if v341_wave<3 and bool(beast.get("invasion_commander",false))' in s
assert 'if (v341_wave<3 and slot>=count)' in s
assert '_v341_dormant(creature)' in s and 'creature["respawn_at"]=-1.0' in s
assert 'creature["dungeon_site"]=-2' in s
assert 'v341_active_site>=0' in s and 'active_rift>=0 or active_world_event>=0' in s
assert 'func _v341_advance_if_cleared()->void:' in s and 'if bool(beasts[int(id)].get("alive",false)):return' in s
assert '_gain_material("Moon Sigil",2)' in s and '_gain_material("Dusk Cinder",1)' in s
assert 'func _death_return_to_sanctuary()->void:' in s and 'func _restore_data(data:Dictionary)->void:' in s
assert 'func _v341_reset()->void:' in s
assert 'v341_marker_clock>=0.4' in s
assert not re.search(r'^func _physics_process\(',s,re.M)
assert not re.search(r'build(ing)?_mode|place_structure',s,re.I)
assert 'survival timer' in s
names=re.findall(r'^func (\w+)\(',s,re.M)
assert len(names)==len(set(names)), 'Duplicate methods in active script'
assert (r/'tests/v34_1_dynamic_invasions.gd').exists()
assert 'tests\\verify_v34_1_invasions.py' in (r/'BUILD_WINDOWS.bat').read_text()
for line in (r/'docs/ORIGINAL_ART_V34_SHA256.txt').read_text().splitlines():
 sha,name=line.split('  ',1)
 assert hashlib.sha256((r/name).read_bytes()).hexdigest()==sha,name
print('PASS v34.1 preallocated three-site invasions, staged waves, death reset, rewards, original art')
