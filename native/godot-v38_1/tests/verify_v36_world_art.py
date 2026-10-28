#!/usr/bin/env python3
"""Offline contracts only. Does NOT establish Godot render/runtime correctness."""
from pathlib import Path
import hashlib,re,sys,struct
root=Path(__file__).resolve().parents[1]
s=(root/'scripts/native_v34_4.gd').read_text()
art=(root/'scripts/systems/lunar_art_palette_v36.gd').read_text()
sky=root/'assets/v36_world_art/lunar_pink_moon_sky.png'
entries=[]
for line in (root/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines():
 if line.strip(): entries.append(line.split('  ',1))
checks={
 'All inherited artwork unchanged':all((root/rel).is_file() and hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest for digest,rel in entries),
 'Three-dimensional sky texture':sky.is_file() and sky.read_bytes()[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',sky.read_bytes()[16:24])==(1536,768),
 'Six native region landmarks':'for zone in range(mini(6,region_nodes.size()))' in s and 'V361_Art_' in s,
 'Non-colliding decorative geometry':'func _v361_primitive' in s and 'StaticBody3D.new()' not in s,
 'Original sprite art not replaced':'V361_OriginalHeroGroundAura' in s and 'hero_sprite.texture=' not in s,
 'Rifts readable at low quality':'ring.visible=true' in s,
 'Art update capped at 10Hz':'if v361_art_elapsed<0.1:return' in s,
 'No duplicate GDScript callbacks':len(re.findall(r'^func (\w+)\(',s,re.M))==len(set(re.findall(r'^func (\w+)\(',s,re.M))),
 'Godot art integration test included in both runners':('v36_1_world_art' in (root/'tools/run_tests.py').read_text() and 'v36_1_world_art.gd' in (root/'BUILD_WINDOWS.bat').read_text()),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
sys.exit(0 if all(checks.values()) else 1)
