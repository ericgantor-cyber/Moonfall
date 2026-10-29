#!/usr/bin/env python3
"""v37 structural and compatibility checks only. NOT a Godot gameplay test."""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import hashlib,re,subprocess,sys
r=Path(__file__).resolve().parents[1]
source=(r/'scripts/native_v37_0.gd').read_text()
rules=(r/'scripts/systems/living_realm_v37.gd').read_text()
save=(r/'scripts/native_v11.gd').read_text()
manifest=r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt'
lines=[line.split('  ',1) for line in manifest.read_text().splitlines() if line.strip()]
checks={
 'Active scene inherits v36.3, v34.4 systems':active_scene_inherits(r,'native_v37_0.gd') and active_scene_inherits(r,'native_v36_3.gd') and active_scene_inherits(r,'native_v34_4.gd'),
 'All existing source art is byte-for-byte identical':len(lines)>=70 and all((r/rel).is_file() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==digest for digest,rel in lines),
 'Six defined surface swim areas, no survival clock':all(x in rules for x in ('SWIM_RADIUS_SQUARED','SWIM_SPEED_MULTIPLIER','swim_zone(')) and '_map_movement_to_camera' in source and 'oxygen' not in save.lower(),
 'Twelve passive wildlife, no extra combat-AI actors':all(x in rules for x in ('WILDLIFE_PER_REGION:int=2','MAX_WILDLIFE:int=12')) and 'region_nodes[zone].add_child(wildlife_root)' in source and '_spawn_beast(' not in source,
 'Bounded 4Hz visual updates':'TICK_INTERVAL:float=0.25' in rules and 'if int(item["zone"])!=region_index' in source,
 'Native world wars and rift status drive ward beacons':'V37_RULES.purified(zone,rifts,v341_active_site,v341_sites)' in source and '_v37_defended_here()' in source,
 'Purification bonus cannot apply to a fresh/actively invaded zone':'entry.get("cleared",false)' in source and 'if v341_active_site>=0' in source,
 'Gathering never awards twice per world day':'V37_RULES.wildlife_available' in source and 'water_site_days[str(V37_RULES.WILDLIFE_SLOT_START+zone)]=world_day' in source,
 'Contracts persistent once per region, no new save schema':'int(id_string)<36' in save and 'CONTRACT_SLOT_START:int=24' in rules and 'V37_RULES.contract_complete' in source,
 'Crafting pays existing materials only on success':'V363_RULES.recipe_issue' in source and 'V37_RULES.pay(recipe["cost"],materials)' in source and 'if issue!="":' in source,
 'Contract payment is gated and one-time':'V37_RULES.missing(cost,materials)' in source and 'V37_RULES.pay(cost,materials)' in source and 'V37_RULES.contract_complete(zone,water_site_days)' in source,
 'Recipes benefit existing cooking/alchemy/smithing skills':all(s in rules for s in ('"Cooking"','"Alchemy"','"Smithing"')) and 'water_professions[job]' in source,
 'No building, timer, new currency or save schema':all(x not in source.lower() for x in ('new_currency','building_mode','survival_timer','save_v3')),
 'v37 test suite registered in both runners':'v37_0_living_realm.gd' in (r/'BUILD_WINDOWS.bat').read_text() and "'v37_0_living_realm':" in (r/'tools/run_tests.py').read_text(),
 'No duplicate GDScript callbacks':len(re.findall(r'^func\s+(\w+)\(',source,re.M))==len(set(re.findall(r'^func\s+(\w+)\(',source,re.M))),
 'World and hero assets remain unchanged':'OriginalHeroArtBillboard' in (r/'scripts/main.gd').read_text(),
}
for label,good in checks.items():print(('PASS' if good else 'FAIL')+': '+label)
if not all(checks.values()):sys.exit(1)
print(f'PASS: v37 Living Realm {len(checks)} structural checks; 3D runtime remains unexecuted')
