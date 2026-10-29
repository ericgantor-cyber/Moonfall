#!/usr/bin/env python3
from pathlib import Path
import re,hashlib
R=Path(__file__).resolve().parents[1]
s=(R/'scripts/native_v34_3.gd').read_text()
q=(R/'scripts/systems/invasion_tactics_v343.gd').read_text()
assert 'extends "res://scripts/native_v34_2.gd"' in s
assert 'res://scripts/native_v34_3.gd' in (R/'scripts/native_v34_4.gd').read_text()
for name in ('_v343_update_phase','_v342_begin_strike','_v342_resolve_strike','_apply_player_hit','_apply_incoming_damage','_v341_advance_if_cleared','_v341_reset','_perf_report'):
 assert ('func '+name+'(') in s,name
for token in ('MAX_MARKERS:int=2','EXPOSE_AFTER_DODGES:int=2','EXPOSED_HIT_MULTIPLIER:float=1.55','func commander_phase(','func target_positions(','func perfect_seal('):assert token in q,token
assert 'super._v342_begin_strike()' in s and 'super._v342_resolve_strike()' in s
assert 'super._v341_advance_if_cleared()' in s and 'super._v341_reset()' in s
assert 'v343_real_hits+=1' in s and 'float(target.get("hp",0.0))<before' in s
assert '_gain_material("Moon Sigil",V343_RULES.CLEAN_SEAL_BONUS)' in s
assert 'if finishing and v341_active_site<0 and clean:' in s
assert not re.search(r'^func _physics_process\(',s,re.M)
assert len(re.findall(r'^func (\w+)\(',s,re.M))==len(set(re.findall(r'^func (\w+)\(',s,re.M)))
assert not re.search(r'building_mode|place_structure|survival_clock',s,re.I)
assert 'v34_3_lunar_war.gd' in (R/'BUILD_WINDOWS.bat').read_text()
for line in (R/'docs/ORIGINAL_ART_V34_SHA256.txt').read_text().splitlines():
 sha,name=line.split('  ',1)
 assert hashlib.sha256((R/name).read_bytes()).hexdigest()==sha,name
print('PASS v34.3 phase tactics, counterplay rewards, single commander budget and unchanged original artwork')
