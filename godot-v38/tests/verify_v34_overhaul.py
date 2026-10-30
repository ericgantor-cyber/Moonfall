#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Static v34.0 gameplay and release guard. NOT a substitute for running Godot."""
from pathlib import Path
import hashlib, re
r=Path(__file__).resolve().parents[1]
s=(r/'scripts/native_v34_0.gd').read_text()
rule=(r/'scripts/systems/lunar_overhaul_v34.gd').read_text()
scene=(r/'scenes/main.tscn').read_text()
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')
assert 'extends "res://scripts/native_v33_8.gd"' in s
assert 'MAX_RESONANCE:int=100' in rule and 'FINISHER_TARGET_CAP:int=4' in rule
assert 'return clampi(current+maxi(0,amount),0,MAX_RESONANCE)' in rule
assert 'if hits>=V34_RULES.FINISHER_TARGET_CAP:break' in s
assert '_apply_player_hit(beast,power,0.36)' in s
assert 'v34_finisher_active=true' in s and 'v34_finisher_active=false' in s
assert 'v34_resonance=0' in s and 'func _respawn_player()->void:' in s
assert 'super._hero_damage_multiplier()*V34_RULES.damage_boon' in s
assert 'super._apply_incoming_damage(raw*V34_RULES.guard_boon' in s
assert 'safe_surge_position' in s and 'DEPTH_SURGE_TELEGRAPH' in s
assert 'PERFECT DODGE!' in s and 'dodge_time>0.0:' in s
assert 'v34_purified_markers' in s and 'bool(rifts[site].get("cleared",false))' in s
assert 'Moon Nova' in s and 'No building. No survival stopwatch.' in s
assert 'build' not in re.findall(r'^func (_build_[A-Z]\w*)',s,re.M)
assert len(re.findall(r'^func (\w+)\(',s,re.M))==len(set(re.findall(r'^func (\w+)\(',s,re.M)))
for name in ['tests/v34_0_grand_overhaul.gd','scripts/systems/lunar_overhaul_v34.gd','Moonfall_Native_v34_4_Setup.iss']:
 assert (r/name).is_file(),name
# Record all asset hashes for this package; compare them with the exact v33.8 source at build time as well.
checks=(r/'docs/ORIGINAL_ART_V34_SHA256.txt').read_text().splitlines()
assert len(checks)>=69, 'Original art baseline incomplete'
for line in checks:
 digest,name=line.split('  ',1)
 assert hashlib.sha256((r/name).read_bytes()).hexdigest()==digest,name
print('PASS v34.0 resonance, bounded finisher, boons, predictive surges, original art contracts')
