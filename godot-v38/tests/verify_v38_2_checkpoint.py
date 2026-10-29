#!/usr/bin/env python3
"""Static source safety regression for checkpoint decoding."""
from pathlib import Path
root=Path(__file__).resolve().parent.parent
src=(root/'scripts/systems/checkpoint_store.gd').read_text(encoding='utf-8')
assert 'MAX_SAVE_BYTES := 250000' in src
assert 'if absf(float(coords[0]))>100000.0 or absf(float(coords[1]))>100000.0:return {}' in src
assert 'if number<0.0:return {}' in src
assert 'if field in ["hp","level","world_day"] and number<=0.0:return {}' in src
assert 'var primary_abs:=ProjectSettings.globalize_path(primary)' in src
print('PASS v38.2 checkpoint validation source regression')
