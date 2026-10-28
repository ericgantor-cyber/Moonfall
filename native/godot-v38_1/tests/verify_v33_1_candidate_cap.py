#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Ensure the 32-candidate ceiling is enforced before each inspection."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
src=(root/'scripts/native_v32_9.gd').read_text()
assert re.search(r'for neighbor in zone_beast_lists\[zone\]:\s+if inspected>=V329_MAX_CANDIDATES:break\s+inspected\+=1',src), 'Cap must be checked BEFORE increment'
assert 'const V329_MAX_CANDIDATES:int=32' in src
assert active_scene_inherits(root, 'native_v34_4.gd')
print('PASS v33.8 exact 32-candidate budget and scene linkage')
