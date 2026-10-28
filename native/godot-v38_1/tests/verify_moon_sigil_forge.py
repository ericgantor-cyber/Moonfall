#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Offline source and data-contract checks, not a Godot gameplay simulation."""
import ast
import hashlib
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'scripts/native_v33_8.gd').read_text()
legacy = (root / 'scripts/native_v11.gd').read_text()
assert active_scene_inherits(root, 'native_v34_4.gd')
match = re.search(r'^const MOON_RECIPES := (\[.*?^\])', source, re.M | re.S)
assert match, 'Missing Moon Sigil recipes'
recipes = ast.literal_eval(match.group(1))
assert {r['id'] for r in recipes} == {'blade','armor','charm','feather','oath'}
assert len({r['name'] for r in recipes}) == len(recipes)
for recipe in recipes:
    assert recipe['cost']['Moon Sigil'] >= 1
    assert all(isinstance(value, int) and value > 0 for value in recipe['cost'].values())
    if recipe['kind'] == 'gear':
        assert 0 < recipe['power'] <= 250
    else:
        assert recipe['power'] == 0

def method(name):
    start = re.search(r'^func '+re.escape(name)+r'\(', source,re.M)
    assert start, 'Missing '+name
    next_method = re.search(r'^func ', source[start.end():], re.M)
    return source[start.start():start.end()+next_method.start()] if next_method else source[start.start():]

craft = method('_moon_craft')
enchant = method('_moon_enchant')
menu = method('_menu_tab')
revive = method('_respawn_player')
assert 'INVENTORY_RULES.craft_issue' in craft and 'INVENTORY_RULES.craft_commit' in craft
assert craft.index('INVENTORY_RULES.craft_issue') < craft.index('INVENTORY_RULES.craft_commit')
assert 'if issue!="":' in craft and 'return' in craft
assert 'gear_items.append(gear)' in craft
assert 'gear["rarity"]="Legendary"' in craft
assert 'kind=="feather" and int(materials.get(PHOENIX_FEATHER,0))>0' in craft
assert 'kind=="oath" and int(materials.get(MOON_OATH,0))>0' in craft
assert 'materials[MOON_OATH]=1' in craft
assert '_combat_threat()' in craft and '_combat_threat()' in enchant
assert 'rank>=3' in enchant and 'mini(250,' in enchant
assert enchant.index('int(materials.get("Moon Sigil",0))<sigils') < enchant.index('materials["Moon Sigil"]=')
assert enchant.index('int(materials.get("Dusk Cinder",0))<cinders') < enchant.index('materials["Dusk Cinder"]=')
assert 'gear_items[index]=enhanced' in enchant and 'if equipped==old:equipped=enhanced.duplicate(true)' in enchant
assert 'and not inventory_show_vault' in menu and 'inventory_page*INVENTORY_RULES.BAG_PAGE_SIZE' in menu
assert 'oath_active' in revive and '0.45 if oath_active else 0.30' in revive
assert '3.0 if oath_active else PHOENIX_GRACE_SECONDS' in revive
assert 'phoenix_used_this_expedition=true' in revive
assert 'materials[PHOENIX_FEATHER]=int(materials[PHOENIX_FEATHER])-1' in revive
assert '"materials":materials.duplicate(true)' in legacy
assert '"gear":gear_items.duplicate(true)' in legacy and '"equipped":equipped.duplicate(true)' in legacy
assert '_save_game()' in craft and '_save_game()' in enchant
assert 'survival timer' not in method('_death_stats_text').lower()

# All 69 existing image assets must remain byte-for-byte unchanged.
baseline = root / 'docs/ART_SHA256_V30_1_BASELINE.txt'
for line in baseline.read_text().splitlines():
    if not line.strip():continue
    digest, file = line.split('  ',1)
    assert hashlib.sha256((root / file).read_bytes()).hexdigest() == digest, file
print('PASS: five unique recipes, one-time oath/feather, bounded enchants, safe ingredient checks, saved progression, no stopwatch, artwork')
