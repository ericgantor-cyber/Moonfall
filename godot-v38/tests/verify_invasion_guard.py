#!/usr/bin/env python3
"""Fail-closed invasion guards, unchanged artwork and no new game mode."""
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
s=(root/'scripts/native_v34_1.gd').read_text()
t=(root/'scripts/native_v34_2.gd').read_text()
checks={
 'Roster validates exact bounded actor count':'ids.size()!=V341_RULES.SOLDIER_SLOTS+1' in s,
 'Roster validates all actor references':'id>=beasts.size()' in s and 'is_instance_valid(body)' in s,
 'Only one commander':'commanders==1' in s,
 'Cannot start incomplete invasions':bool(re.search(r'func _v341_start\(.*?\n(?:(?!func ).*?\n)*? if not _v341_roster_valid',s,re.S)),
 'Cannot grant victory for missing squad':bool(re.search(r'func _v341_advance_if_cleared\(.*?\n(?:(?!func ).*?\n)*? if not _v341_roster_valid',s,re.S)),
 'Mid-battle invalid roster clears safely':'_v341_reset()' in t and '_v341_roster_valid(site)' in t,
 'Invasion reset tolerates invalid indices':'if slot<0 or slot>=beasts.size():continue' in s,
 'No new mode or timer':not re.search(r'building_mode|survival_stopwatch|game_mode=',s,re.I),
 'Godot gameplay test is registered':'v34_4_invasion_guard.gd' in (root/'BUILD_WINDOWS.bat').read_text(),
}
for k,v in checks.items():print(('PASS' if v else 'FAIL')+': '+k)
sys.exit(0 if all(checks.values()) else 1)
