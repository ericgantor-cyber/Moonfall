#!/usr/bin/env python3
"""Offline source contracts; runtime requires Godot."""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import re,hashlib
r=Path(__file__).resolve().parents[1]
s=(r/'scripts/native_v33_8.gd').read_text()
def m(n):
    q=re.search(r'^func '+re.escape(n)+r'\(',s,re.M);assert q,n
    e=re.search(r'^func ',s[q.end():],re.M)
    return s[q.start():q.end()+e.start()] if e else s[q.start():]
assert active_scene_inherits(r, 'native_v34_4.gd')
assert ' _depth_build_surges()' in m('_build_scene')
assert ' _depth_surge_tick(delta)' in m('_process')
assert 'depth_surge_nodes:Array[Dictionary]=[]' in s
assert 'if not hero_playing or death_active or menu_open or mounted' in m('_depth_surge_tick')
assert 'if dodge_time>0.0:' in m('_depth_surge_tick')
assert 'distance_squared_to(root.global_position)<=DEPTH_SURGE_HIT_RADIUS_SQ' in m('_depth_surge_tick')
assert 'root.visible=false' in m('_depth_surge_tick')
assert '_depth_reset_surge()' in m('_depth_reset_glyphs')
assert '_depth_attunement_count(site)==3' in m('_on_rift_enemy_defeated')
assert 'not bool(rifts[site].get("cleared",false))' in m('_on_rift_enemy_defeated')
assert 'if eligible and bool(rifts[site].get("cleared",false)):' in m('_on_rift_enemy_defeated')
assert '_gain_material("Moon Sigil",2)' in m('_on_rift_enemy_defeated')
assert '_save_game()' in m('_on_rift_enemy_defeated')
assert '45%" if oath_active else "30%' in s
assert 'No time limit' in s and 'No base building or construction system' in s
assert 'survival stopwatch' in s
for line in (r/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines():
    if not line.strip():continue
    digest,name=line.split('  ',1)
    assert hashlib.sha256((r/name).read_bytes()).hexdigest()==digest,name
print('PASS: rift telegraph/dodge and one-time attunement reward contracts; original art unchanged')
