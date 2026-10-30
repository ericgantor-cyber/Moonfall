#!/usr/bin/env python3
"""Source-level deep water checks; not a substitute for Godot runtime gameplay."""
from pathlib import Path
import hashlib,re,sys
from active_scene_guard import active_scene_inherits
r=Path(__file__).resolve().parents[1]
main=(r/'scripts/native_v37_3.gd').read_text()
rules=(r/'scripts/systems/deep_water_v373.gd').read_text()
lake=(r/'scripts/systems/living_realm_v37.gd').read_text()
batch=(r/'BUILD_WINDOWS.bat').read_text()
registry=(r/'tools/run_tests.py').read_text()
save=(r/'scripts/native_v11.gd').read_text()
manifest=(r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines()
assets=[line.split('  ',1) for line in manifest if line.strip()]
checks={
 'Active scene loads layered depth plus oxygen scripts':active_scene_inherits(r,'native_v37_3.gd') and active_scene_inherits(r,'native_v37_2.gd'),
 'Real vertical hero movement, clamped depth':'player.move_and_collide(Vector3(0.0,vertical_motion,0.0))' in main and 'MAX_DEPTH:float=3.3' in rules and 'minf(MAX_DEPTH,depth+step*DIVE_SPEED)' in rules,
 'Depth persists briefly after releasing dive; oxygen remains submerged':'return super._v372_dive_held() or V373_RULES.truly_submerged(v373_depth)' in main,
 'Both keyboard and controller rise/dive controls':'KEY_SPACE' in main and 'JOY_BUTTON_A' in main and 'KEY_C' in main and 'JOY_BUTTON_RIGHT_STICK' in main,
 'Collision restored when back on the surface':'player.collision_mask=v373_original_collision_mask' in main and 'player.collision_mask=0 if' not in main,
 'Underwater tunnel accessible only when submerged':'y< -0.24' in lake and 'absf(dz)<=1.35' in lake,
 'Six bounded grotto rooms / terrain cutouts / caches':'range(V373_RULES.REGION_COUNT)' in main and '_v373_open_ground(zone,offset)' in main and 'V373_AbyssalCache' in main,
 'Cache rewards use existing material inventory and cooldown':'_gain_material("Moon Sigil",1)' in main and '_gain_material("Ore Shard",2)' in main and 'water_site_days[str(V373_RULES.CACHE_SLOT_START+zone)]=world_day' in main,
 'Save schema accepts reserved cache slots without changing existing slots':'int(id_string)<36' in save and 'CACHE_SLOT_START:int=30' in rules,
 'Real Godot test registered on both runners':"'v37_3_deep_water':" in registry and 'tests/v37_3_deep_water.gd' in batch,
 'Original Z / Nyra and older art assets byte-for-byte unchanged':len(assets)>=70 and all((r/rel).exists() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==h for h,rel in assets),
 'No new game modes, currencies, or timers':all(x not in main for x in ['new_currency','survival_timer','new_game_mode']),
 'No duplicate main integration method names':len(set(re.findall(r'^func\s+(\w+)\(',main,re.M)))==len(re.findall(r'^func\s+(\w+)\(',main,re.M)),
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
print('%d/%d deep-water source checks'%(sum(checks.values()),len(checks)))
sys.exit(0 if all(checks.values()) else 1)
