#!/usr/bin/env python3
"""Regression guard: Moonfall must not show a survival stopwatch in the native HUD or death feedback.

Game timers required for attacks, boss phases, world day/night and status effects remain intact.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

main = (ROOT / 'scripts/main.gd').read_text(encoding='utf-8')
base = (ROOT / 'scripts/native_v11.gd').read_text(encoding='utf-8')

def method_block(source, name):
    match = re.search(r'^func ' + re.escape(name) + r'\([^\n]*', source, re.M)
    if match is None:
        errors.append('Missing function: ' + name)
        return ''
    next_func = re.search(r'^func ', source[match.end():], re.M)
    return source[match.start():match.end()+next_func.start()] if next_func else source[match.start():]

for source, function in ((main, '_update_hud'), (base, '_update_hud'), (main, '_respawn_player')):
    block = method_block(source, function)
    for marker in ('survival time', 'time survived', 'survived for', 'survival timer', 'time alive', 'run timer', 'run time'):
        if marker in block.lower():
            errors.append(f'{function} includes survival stopwatch text: {marker}')
    if re.search(r'\bmap_time\b|\brun_elapsed\b|\bsurvival_seconds\b', block):
        # Native v11 uses map_time solely to calculate COMBO eligibility, not elapsed time text.
        expression = re.sub(r'combat_combo if map_time<=combat_combo_until else 0', '', block)
        if re.search(r'\bmap_time\b|\brun_elapsed\b|\bsurvival_seconds\b', expression):
            errors.append(f'{function} appears to display elapsed gameplay time')

if errors:
    for e in errors:
        print('FAIL:', e)
    sys.exit(1)
print('PASS: no survival stopwatch on native gameplay HUD or death message')
