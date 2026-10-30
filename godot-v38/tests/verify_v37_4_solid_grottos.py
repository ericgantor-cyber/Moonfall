#!/usr/bin/env python3
"""Source contract checks for solid caves; not a Godot runtime substitute."""
from pathlib import Path
import hashlib,re,sys
from active_scene_guard import active_scene_inherits
r=Path(__file__).resolve().parents[1]
source=(r/'scripts/native_v37_4.gd').read_text()
parent=(r/'scripts/native_v37_3.gd').read_text()
manifest=(r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines()
assets=[line.split('  ',1) for line in manifest if line.strip()]
batch=(r/'BUILD_WINDOWS.bat').read_text()
registry=(r/'tools/run_tests.py').read_text()
checks={
 'v37.4 active scene inherits older gameplay':active_scene_inherits(r,'native_v37_4.gd') and active_scene_inherits(r,'native_v37_3.gd'),
 'No underwater collision-mask suppression':'player.collision_mask=0 if' not in parent and 'player.collision_mask=v373_original_collision_mask' in parent,
 'Vertical movement is collision-aware':'player.move_and_collide(Vector3(0.0,vertical_motion,0.0))' in parent and 'v373_depth=clampf(V373_RULES.PLAYER_SURFACE_Y-player.position.y' in parent,
 'Deep water boundaries resist leaving a lake':'V37_RULES.swim_zone(player.global_position.x' in parent and 'prior_water_position.x' in parent,
 'Six floors, ceilings, side walls, end walls and pillars are collision shapes':all(x in source for x in ['SolidLakeBed','SolidCaveCeiling','SolidFarWall','SolidSideWall_','SolidPillar_','CollisionShape3D.new()','BoxShape3D.new()']),
 'Collision stays enabled on existing layer 1':'body.collision_layer=V374_SOLID_LAYERS' in source and 'V374_SOLID_LAYERS:int=1' in source,
 'Swim animation uses an independent original-art pose anchor':'v374_swim_pose_anchor' in source and 'hero_sprite.reparent(v374_swim_pose_anchor,true)' in source and '.texture=' not in source,
 'Godot test registered in both Windows and Python runners':"'v37_4_solid_grottos':" in registry and 'res://tests/v37_4_solid_grottos.gd' in batch,
 'Original character and environmental assets unchanged':len(assets)>=70 and all((r/rel).exists() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==h for h,rel in assets),
 'No new modes, survival clocks or currencies':not any(s in source for s in ['new_mode','survival_timer','new_currency']),
 'No duplicate GDScript function names':len(re.findall(r'^func\s+(\w+)\(',source,re.M))==len(set(re.findall(r'^func\s+(\w+)\(',source,re.M))),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v37.4 checks passed')
sys.exit(0 if all(checks.values()) else 1)
