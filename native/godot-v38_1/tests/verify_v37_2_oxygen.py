#!/usr/bin/env python3
"""Offline source guards for v37.2 underwater breathing. Not a Godot gameplay test."""
from pathlib import Path
from active_scene_guard import active_scene_inherits
import hashlib, re, sys
root=Path(__file__).resolve().parents[1]
logic=(root/'scripts/native_v37_2.gd').read_text()
rules=(root/'scripts/systems/oxygen_v372.gd').read_text()
base=(root/'scripts/native_v37_0.gd').read_text()
manifest=root/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt'
images=[]
for line in manifest.read_text().splitlines():
    if line.strip():
        digest,rel=line.split('  ',1); images.append((digest,rel))
checks={
 'Main scene inherits optional dive logic':active_scene_inherits(root,'native_v37_2.gd') and active_scene_inherits(root,'native_v37_0.gd'),
 'Oxygen is bounded at 100 and drains only underwater':all(k in rules for k in ('MAX_OXYGEN:float=100.0','DRAIN_PER_SECOND:float=4.0','REFILL_PER_SECOND:float=25.0','if underwater else','clampf(current+change,0.0,MAX_OXYGEN)')),
 'Diving requires lake, gameplay, no menu, no mount or death':'can_dive(v37_swimming,hero_playing,menu_open,mounted,death_active)' in logic and 'and playing and not paused and not mounted and not dead' in rules,
 'Surface air recovery and land reset':'V372_OXYGEN.advance(v372_oxygen,delta,v372_diving)' in logic and 'v372_oxygen=V372_OXYGEN.MAX_OXYGEN' in logic,
 'HUD has clear low oxygen warning and bar':'v372_oxygen_panel' in logic and 'v372_oxygen_bar.value=v372_oxygen' in logic and 'LOW OXYGEN' in logic,
 'Oxygen depletion is handled via existing damage path':'_apply_incoming_damage(V372_OXYGEN.damage_tick(max_hp))' in logic,
 'Keyboard and controller hold-to-dive input':'Input.is_key_pressed(KEY_C)' in logic and 'JOY_BUTTON_RIGHT_STICK' in logic,
 'Old swim wake suppressed while submerged':'if v372_diving and v37_swim_zone' in logic and 'wake.visible=false' in logic,
 'Original six swimming regions remain':"swim_zone(player.global_position.x,player.global_position.z,REGIONS" in base,
 'Game has no general survival clock':'survival_seconds' not in logic.lower() and 'survival_timer' not in logic.lower(),
 'No additional saves or currencies': 'func _save_game' not in logic and 'new_currency' not in logic,
 'Both Godot runners register the oxygen suite':"'v37_2_oxygen':" in (root/'tools/run_tests.py').read_text() and 'tests/v37_2_oxygen.gd' in (root/'BUILD_WINDOWS.bat').read_text(),
 'All original image assets unchanged':len(images)>=70 and all((root/rel).is_file() and hashlib.sha256((root/rel).read_bytes()).hexdigest()==digest for digest,rel in images),
 'No duplicate Godot function overrides':len(set(re.findall(r'^func\s+(\w+)\(',logic,re.M)))==len(re.findall(r'^func\s+(\w+)\(',logic,re.M)),
}
for label,passed in checks.items():print(('PASS' if passed else 'FAIL')+': '+label)
print('PASS: %d oxygen source checks' % len(checks) if all(checks.values()) else 'FAIL: oxygen source validation')
sys.exit(0 if all(checks.values()) else 1)
