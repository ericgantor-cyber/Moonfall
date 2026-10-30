#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Source regression contracts for optional world-depth gameplay, not runtime proof."""
from pathlib import Path
import re, hashlib
r=Path(__file__).resolve().parents[1]
s=(r/'scripts/native_v33_8.gd').read_text()
scene=(r/'scenes/main.tscn').read_text()
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')

def method(name):
    m=re.search(r'^func '+re.escape(name)+r'\(',s,re.M)
    assert m, name
    next_m=re.search(r'^func ',s[m.end():],re.M)
    return s[m.start():m.end()+next_m.start()] if next_m else s[m.start():]

build=method('_build_scene')
assert 'super._build_scene()' in build
for f in ('_depth_build_corruption()', '_depth_build_glyphs()', '_depth_build_travelers()'):
    assert f in build
zone=method('_depth_near_corruption')
assert 'if bool(rifts[site].get("cleared",false)):continue' in zone
assert 'DEPTH_CORRUPTION_RADIUS_SQ:float=13.0*13.0' in s
hit=method('_apply_incoming_damage')
assert 'raw*1.08 if _depth_near_corruption()>=0 else raw' in hit
nova=method('_spell')
assert 'super._spell(which)' in nova
assert 'hits>=3:break' in nova and 'depth_spell_charges>0:depth_spell_charges-=1' in nova
assert 'zone_beast_lists[region_index]' in nova and '_has_clear_sight' in nova
assert 'ability_cooldown<=0.0' in nova and 'which=="nova"' in nova
interact=method('_interact')
assert 'active_rift!=site' in interact and 'int(rifts[site].get("stage",0))!=stage' in interact
assert 'if depth_glyph_charged.has(key)' in interact
assert 'depth_spell_charges=mini(3,depth_spell_charges+1)' in interact
assert 'super._interact()' in interact
assert '_combat_threat()' in interact and 'Moon Scribe:' in interact
reset=method('_depth_reset_glyphs')
assert 'depth_glyph_charged.clear()' in reset and 'depth_spell_charges=0' in reset
assert '_depth_reset_glyphs()' in method('_start_game')
assert '_depth_reset_glyphs()' in method('_restore_data')
assert '_depth_reset_glyphs()' in method('_death_return_to_sanctuary')
assert 'DEPTH_TRAVELER_ZONES:Array[int]=[1,3,5]' in s
assert 'depth_visual_clock<DEPTH_VISUAL_TICK:return' in method('_depth_tick')
assert 'no countdown' in s.lower() and 'no base building or construction system' in s.lower()
assert 'if which=="World"' in method('_menu_tab') or 'elif which=="World"' in method('_menu_tab')
# No character imagery or legacy art changes.
for line in (r/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines():
    if not line.strip(): continue
    sha,name=line.split('  ',1)
    assert hashlib.sha256((r/name).read_bytes()).hexdigest()==sha, name
print('PASS: corruption has no countdown, bounded echoes, ordered glyphs, NPC lore, reset safety and unchanged art')
