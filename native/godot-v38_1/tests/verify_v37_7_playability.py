#!/usr/bin/env python3
"""v37.7 source contract and regression manifest, not a Godot runtime test."""
from pathlib import Path
from hashlib import sha256
import sys,re,ast
from active_scene_guard import active_scene_inherits
r=Path(__file__).resolve().parents[1]
source=(r/'scripts/native_v37_7.gd').read_text()
rules=(r/'scripts/systems/playability_v377.gd').read_text()
prior=(r/'scripts/native_v37_2.gd').read_text()
runner=(r/'tools/run_tests.py').read_text()
bat=(r/'BUILD_WINDOWS.bat').read_text()
art=[line.split('  ',1) for line in (r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines() if line.strip()]
checks={
 'New active scene preserves entire v37.6 inheritance chain':active_scene_inherits(r,'native_v37_7.gd') and active_scene_inherits(r,'native_v37_6.gd') and active_scene_inherits(r,'native_v37_4.gd'),
 'Submerged recovery gated by combat, death and mount':all(x in rules for x in ['not dead','not mounted','not threat','zone>=0','depth>MIN_SUBMERGED_DEPTH']),
 'Surface escape returns hero within the same world':all(x in source for x in ['V377_RULES.shoreline(REGIONS,zone','player.global_position=destination','_v373_reset_depth()','v37_swimming=false','v372_oxygen=V372_OXYGEN.MAX_OXYGEN']),
 'No return-to-surface exploit from land': 'zone>=0 and depth>MIN_SUBMERGED_DEPTH' in rules,
 'Context-sensitive HP, oxygen, battle and water hints':all(x in rules for x in ['LOW AIR','UNDERWATER','LOW HEALTH','COMBAT','EXPLORE']),
 'HUD help computed on demand instead of per frame':'func _process(' not in source and 'func _physics_process(' not in source,
 'Tips inherited and extended':'func _v376_render(' in source and 'super._v376_render()' in source,
 'Help and rescue accessible from existing Raven Atlas':'_atlas_action("EMERGENCY SURFACE' in source and '_atlas_action("OPEN SEARCHABLE TIPS' in source,
 'Outdated v13 startup text replaced without changing art':'func _build_ui(' in source and 'MOONFALL v37.7 • THE LIVING REALM' in source and 'hero_sprite.texture=' not in source,
 'CI verifies each offline test exit independently':all('python tests/verify_v37_6_all_tips.py\n          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }\n          python tests/verify_v37_7_playability.py' in (r/loc).read_text() for loc in ['.github/workflows/windows-v34_4.yml','tools/github_windows_v34_4.yml']),
 'User-facing low oxygen text demands surfacing':'LOW OXYGEN: reach the surface for air!' in prior and 'LOW OXYGEN: release dive to breathe!' not in prior,
 'No extra currencies, game modes or survival stopwatch':all(s not in source for s in ['new_currency','new_game_mode','survival_timer','func _save_game(']),
 'v37.7 runtime suite exists and registered in both launchers':"'v37_7_playability':" in runner and '--script res://tests/v37_7_playability.gd' in bat and (r/'tests/v37_7_playability.gd').is_file(),
 'Legacy tips test accepts inheritance':"active_scene_inherits(r,'native_v37_6.gd')" in (r/'tests/verify_v37_6_all_tips.py').read_text(),
 'No duplicate new GDScript functions': len(set(re.findall(r'^func\s+(\w+)\(',source,re.M)))==len(re.findall(r'^func\s+(\w+)\(',source,re.M)),
 'All original image SHA256 hashes match':len(art)>=70 and all((r/rel).is_file() and sha256((r/rel).read_bytes()).hexdigest()==digest for digest,rel in art),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
print(f'{sum(checks.values())}/{len(checks)} v37.7 source checks passed')
sys.exit(0 if all(checks.values()) else 1)
