#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
from pathlib import Path
import hashlib,re
R=Path(__file__).resolve().parents[1]
s=(R/'scripts/native_v34_4.gd').read_text()
r=(R/'scripts/systems/invasion_fairness_v344.gd').read_text()
scene=(R/'scenes/main.tscn').read_text()
assert 'extends "res://scripts/native_v34_3.gd"' in s
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')
assert 'recovered_during_hit(' in s and 'v343_real_hits+=1' in s
assert 'super._apply_incoming_damage(raw)' in s
assert 'v342_phase!=0' in s and 'v342_phase_clock+=clampf(delta,0.0,0.15)*extra' in s
assert 'rest_acceleration' in r and 'MAX_EXTRA:float=0.34' in r
assert 'v34_4_fair_fight.gd' in (R/'BUILD_WINDOWS.bat').read_text()
assert all(t in (R/'BUILD_WINDOWS.bat').read_text() for t in ('v33_8_death_resurrection.gd','v33_8_moon_sigil_forge.gd'))
assert 'v34_4_fair_fight' in (R/'tools/run_tests.py').read_text()
assert len(re.findall(r'^func (\w+)\(',s,re.M))==len(set(re.findall(r'^func (\w+)\(',s,re.M)))
assert not re.search('building_mode|survival_clock|place_structure',s,re.I)
for line in (R/'docs/ORIGINAL_ART_V34_SHA256.txt').read_text().splitlines():
 sha,rel=line.split('  ',1)
 assert hashlib.sha256((R/rel).read_bytes()).hexdigest()==sha,rel
print('PASS v34.4 resurrection integrity, fair attack rhythm, and unchanged artwork')
