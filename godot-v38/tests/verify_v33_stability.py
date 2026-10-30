#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Offline v33.0 stability checks for active scene, inheritance and bounded steering."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
scene=(root/'scenes/main.tscn').read_text()
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')
for v,parent in [('33_8','33_7'),('33_7','33_6'),('33_6','33_5'),('33_5','33_4'),('33_4','33_3'),('33_3','33_2'),('33_2','33_1'),('33_1','33_0'),('33_0','32_9'),('32_9','32_8'),('32_8','32_7')]:
    content=(root/f'scripts/native_v{v}.gd').read_text()
    assert f'extends "res://scripts/native_v{parent}.gd"' in content
source=(root/'scripts/native_v32_9.gd').read_text()
assert 'V329_MAX_CANDIDATES:int=32' in source
assert 'if inspected>=V329_MAX_CANDIDATES:break' in source
assert 'v329_neighbor_checks+=inspected' in source
print('PASS v34.4 active scene, inheritance and bounded flanking scan')
