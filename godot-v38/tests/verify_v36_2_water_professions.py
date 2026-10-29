#!/usr/bin/env python3
"""Offline source checks only; a green result is not a Godot runtime test."""
from pathlib import Path
import re,hashlib,sys
r=Path(__file__).resolve().parents[1]
g=(r/'scripts/native_v34_4.gd').read_text()
b=(r/'scripts/native_v11.gd').read_text()
p=(r/'scripts/systems/water_professions_v362.gd').read_text()
checks={
 'Existing art unchanged':all((r/rel).exists() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==digest for line in (r/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines() if line.strip() for digest,rel in [line.split('  ',1)]),
 'Six in-world water bodies':'for zone in range(6):' in g and 'V362_AnimatedWaterSurface' in g,
 '18 gathering sites':'for slot in range(3):' in g and 'zone*3+slot' in g,
 'GPU water shader':'shader_type spatial;' in g and 'TIME*1.35' in g,
 'Six colors vary by region':'var hue:Color=' in g and 'set_shader_parameter("shallow_color",hue)' in g,
 'No physics water simulation':'RigidBody3D.new()' not in g and 'Area3D.new()' not in g,
 'Wading preserves camera movement':'V36_CAMERA_RULES.move_relative' in g and 'v362_is_wading' in g,
 'Fishing herb mining, cooking alchemy smithing':'"Fishing","Herbalism","Mining","Cooking","Alchemy","Smithing"' in p,
 'Materials reused for crafting':'V362_JOBS.consume_recipe(recipe,materials)' in g and '_gain_material(str(recipe["result"]),1)' in g,
 'Professions persist through save/load':all(token in b for token in ('"water_professions":water_professions.duplicate(true)','data.get("water_professions",{})','"water_site_days":water_site_days.duplicate(true)','data.get("water_site_days",{})')),
 'Harvests not infinitely replayable':'V362_JOBS.can_gather(site_id,world_day,water_site_days)' in g and 'water_site_days[str(site_id)]=world_day' in g,
 'Cannot craft during combat':'_combat_threat()' in g and 'func _v362_craft' in g,
 'Regressions registered in both test runners':'v36_2_water_professions.gd' in (r/'BUILD_WINDOWS.bat').read_text() and "'v36_2_water_professions':" in (r/'tools/run_tests.py').read_text(),
 'No duplicate callbacks':len(re.findall(r'^func (\w+)\(',g,re.M))==len(set(re.findall(r'^func (\w+)\(',g,re.M))),
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
sys.exit(0 if all(checks.values()) else 1)
