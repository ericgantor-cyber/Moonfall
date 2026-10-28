#!/usr/bin/env python3
"""Checks v37.5 maintenance contracts, not a replacement for runtime testing."""
from pathlib import Path
from hashlib import sha256
import sys
from active_scene_guard import active_scene_inherits
r=Path(__file__).resolve().parents[1]
pose=(r/'scripts/native_v37_4.gd').read_text()
realm=(r/'scripts/native_v37_0.gd').read_text()
water=(r/'scripts/native_v36_3.gd').read_text()
hero=(r/'scripts/native_v16.gd').read_text()
manifest=(r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines()
assets=[line.split('  ',1) for line in manifest if line.strip()]
checks={
  'Swim pose preserves inherited combat tilt': 'v374_swim_pose_anchor.rotation.z=' in pose and 'combat_pose_kind' in hero,
  'Swim pose preserves inherited combat scale': 'v374_swim_pose_anchor.scale=Vector3(' in pose and 'hero_sprite.reparent(v374_swim_pose_anchor,true)' in pose,
  'Swim pose blends bob with original position': 'v374_swim_pose_anchor.position.y=' in pose and 'hero_sprite.position=Vector3(0,0.85+bob,0)' in hero,
  'Ward state changes are cached': 'var v37_ward_clean_states:Array[int]=[]' in realm and 'v37_ward_clean_states[zone]==state:continue' in realm,
  'Ward still reacts to actual rift/invasion state': 'V37_RULES.purified(zone,rifts,v341_active_site,v341_sites)' in realm,
  'Water glow updates only on state change': 'if night_changed:v363_water_materials[zone].set_shader_parameter' in water,
  'Flora material assignment is idempotent': 'if flower.material_override!=flower_material' in water,
  'Art manifest unchanged': len(assets)>=70 and all((r/path).is_file() and sha256((r/path).read_bytes()).hexdigest()==digest for digest,path in assets),
  'Previous solid-cave physics preserved': 'body.collision_layer=V374_SOLID_LAYERS' in pose and 'player.move_and_collide(' in (r/'scripts/native_v37_3.gd').read_text(),
  'Main scene still inherits the v37.4 gameplay chain': active_scene_inherits(r,'native_v37_4.gd'),
  'No new mechanics bloating script': 'new_mode' not in pose and 'survival_timer' not in pose,
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v37.5 maintenance checks passed')
sys.exit(0 if all(checks.values()) else 1)
