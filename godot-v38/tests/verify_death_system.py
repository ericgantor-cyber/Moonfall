#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Offline death-system regression tests for Moonfall v33.8.

These tests verify contracts, but do NOT claim a Godot runtime playthrough.
"""
from pathlib import Path
import hashlib
import re
root = Path(__file__).resolve().parents[1]
scene = (root/'scenes/main.tscn').read_text()
source = (root/'scripts/native_v33_8.gd').read_text()
base = (root/'scripts/native_v11.gd').read_text()
assert active_scene_inherits(ROOT if 'ROOT' in globals() else R if 'R' in globals() else r if 'r' in globals() else root, 'native_v34_4.gd')
assert 'extends "res://scripts/native_v33_7.gd"' in source

def block(name):
    found = re.search(r'^func '+re.escape(name)+r'\(',source,re.M)
    assert found, 'Missing death method: '+name
    nxt = re.search(r'^func ',source[found.end():],re.M)
    return source[found.start():found.end()+nxt.start()] if nxt else source[found.start():]

assert '_respawn_player()' in block('_apply_incoming_damage') or '_respawn_player()' in base
assert 'death_active=true' in block('_respawn_player')
assert 'set_physics_process(false)' in block('_respawn_player')
assert 'death_overlay.show()' in block('_respawn_player')
assert 'death_continue.call_deferred("grab_focus")' in block('_respawn_player')
assert 'death_stats.text=_death_stats_text()' in block('_respawn_player')
assert 'super._respawn_player()' in block('_death_return_to_sanctuary')
assert '_death_reset_active_rift()' in block('_death_return_to_sanctuary')
assert 'entry["stage"]=0' in block('_death_reset_active_rift')
assert '_despawn_native_beast' in block('_death_reset_active_rift')
assert 'set_physics_process(true)' in block('_death_return_to_sanctuary')
assert '_save_game()' in block('_death_return_to_sanctuary')
assert 'suppress_autosave_message=true' in block('_death_return_to_sanctuary')
assert '_death_return_to_sanctuary()' in block('_death_return_to_title')
assert 'PHOENIX_DROP_CHANCE:float=0.18' in source
assert 'PHOENIX_GRACE_SECONDS:float=2.0' in source
assert 'materials[PHOENIX_FEATHER]=int(materials[PHOENIX_FEATHER])-1' in block('_respawn_player')
assert 'phoenix_used_this_expedition=true' in block('_respawn_player')
assert 'phoenix_grace_until' in block('_apply_incoming_damage')
assert 'rival_defeated' in block('_defeat_beast')
assert '_show_rival_mark()' in block('_death_return_to_sanctuary')
assert 'MARKED RIVAL' in block('_show_rival_mark')
assert 'revenge_target_id=-1' in block('_defeat_beast')
assert 'gold+=40+' in block('_defeat_beast')
assert 'death_attacker=beast' in block('_update_beast_step')
assert 'menu_open=true' in block('_respawn_player')
assert 'death_active or map_time<phoenix_grace_until' in block('_apply_incoming_damage')
assert '_save_game()' in block('_defeat_beast')
assert '"schema":2' in base and 'SAVE_STORE.commit_checkpoint' in base
assert '"materials":materials.duplicate(true)' in base
assert '"level":hero_level' in base and '"xp":hero_xp' in base
assert '"gear":gear_items.duplicate(true)' in base
assert '"bosses":defeated_boss_ids.duplicate()' in base
assert '"rifts":rifts_cleared.duplicate()' in base
assert '"world_day":world_day' in base
assert 'survival timer' not in block('_death_stats_text').lower()
assert not re.search(r'\b(map_time|survival_seconds|run_elapsed)\b',block('_death_stats_text'))
# No artwork modifications and no new character art files.
manifest = (root/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines()
for line in manifest:
    if not line.strip(): continue
    checksum, relative = line.split('  ',1)
    assert hashlib.sha256((root/relative).read_bytes()).hexdigest() == checksum, relative
print('PASS death overlay, revival guards, shrine save, revenge, progression, no stopwatch, artwork checks')
