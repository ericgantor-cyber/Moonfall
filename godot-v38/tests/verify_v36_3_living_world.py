#!/usr/bin/env python3
"""Static contracts for v36.3. No Godot execution is implied."""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import hashlib,re,sys
r=Path(__file__).resolve().parents[1]
scene=(r/'scenes/main.tscn').read_text()
source=(r/'scripts/native_v36_3.gd').read_text()
rules=(r/'scripts/systems/living_world_v363.gd').read_text()
art=r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt'
checks={
 'One live main scene, v36.3 inheriting all v34.4 systems':active_scene_inherits(r,'native_v36_3.gd') and active_scene_inherits(r,'native_v34_4.gd'),
 'No extra game mode':scene.count('script = ExtResource(')==1,
 'Preserves all 70 v36.2 image hashes':art.is_file() and all((r/rel).is_file() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==sha for line in art.read_text().splitlines() if line.strip() for sha,rel in [line.split('  ',1)]),
 'Six 3D living water materials': 'for zone in range(V363_RULES.WATER_ZONE_COUNT):' in source and 'v363_water_materials.append(material)' in source,
 'Water animates on GPU with edge foam': all(x in source for x in ('shader_type spatial','foam=','TIME*2.4','night_glow')),
 'Animated non-colliding cascades': 'V363_MoonlitCascade' in source and 'zone==2 or zone==3' in source and 'CollisionShape3D.new()' not in source,
 'Bounded flora and fish counts':all(x in rules for x in ('FLORA_PER_ZONE:int=5','FISH_PER_ZONE:int=2','WATER_ZONE_COUNT:int=6')),
 'Proximity response to player': 'flora_response(dist_sq' in source and 'fish_escape_radius(dist_sq)' in source,
 'Rift purification and invasion state change water':all(x in rules for x in ('active_invasion_site','entry.get("cleared",false)','is_tainted')) and 'set_shader_parameter("corrupted",' in source,
 'Only local region gets flora/fish CPU animation':source.count('!=region_index:continue')>=2 and 'UPDATE_SECONDS:float=0.25' in rules,
 'Crafting is gated from combat': '_combat_threat()' in source and 'recipe_issue(recipe,materials,gear_items.size(),bag_capacity)' in source,
 'Recipes rely on existing materials and professions': all(x in rules for x in ('"Cooking"','"Alchemy"','"Smithing"','"River Reed"','"Ore Shard"')),
 'New consumables prevent infinite healing':'INVENTORY_RULES.consume_one(materials,item)' in source and 'if kind=="food" and hp>=max_hp:' in source,
 'Ward respects finite duration and actual corruption':'ward_damage(raw,v363_ward_seconds,_depth_near_corruption()>=0)' in source and 'v363_ward_seconds=maxf(0.0' in source,
 'No new persistent save schema or currency': all(x not in source for x in ('var new_currency','"schema":3','SAVE_V3')),
 'Original survival stopwatch policy still present':(r/'tests/verify_no_survival_timer.py').is_file(),
 'One runtime suite in both runners':'v36_3_living_world.gd' in (r/'BUILD_WINDOWS.bat').read_text() and "'v36_3_living_world':" in (r/'tools/run_tests.py').read_text(),
 'No duplicate method declarations':len(re.findall(r'^func\s+(\w+)\(',source,re.M))==len(set(re.findall(r'^func\s+(\w+)\(',source,re.M))),
}
for title,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+title)
sys.exit(0 if all(checks.values()) else 1)
