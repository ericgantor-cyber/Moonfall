#!/usr/bin/env python3
"""Source checks for the v37.6 accessible tips codex; not an engine runtime test."""
from pathlib import Path
from hashlib import sha256
import ast,re,sys
from active_scene_guard import active_scene_inherits
r=Path(__file__).resolve().parents[1]
source=(r/'scripts/native_v37_6.gd').read_text()
db=(r/'scripts/systems/tips_codex_v376.gd').read_text()
scene=(r/'scenes/main.tscn').read_text()
win=(r/'BUILD_WINDOWS.bat').read_text()
py=(r/'tools/run_tests.py').read_text()
art=[]
for line in (r/'docs/ART_SHA256_V36_2_ALL_IMAGES.txt').read_text().splitlines():
 if not line.strip():continue
 digest,path=line.split('  ',1)
 art.append((digest,r/path))
rows=re.findall(r'^ \{"category":(.*?),"title":(.*?),"tip":(.*?)\},?$',db,re.MULTILINE)
titles=[m[1] for m in rows]
checks={
 'At least 100 unique help entries':len(rows)>=100 and len(titles)==len(set(titles)),
 'Broad coverage including underwater and controller':['Water & Diving','Combat','Professions & Crafting','Controls & Accessibility','Bosses & Invasions','Death & Recovery','Performance & Saves'] and all(key in db for key in ['Water & Diving','Combat','Professions & Crafting','Controls & Accessibility','Bosses & Invasions','Death & Recovery','Performance & Saves']),
 'Search filters and pagination present':'static func matching(' in db and 'PAGE_SIZE:int=8' in db and 'tippage:next' in source,
 'Keyboard shortcut F1 implemented':'KEY_F1' in source and 'current_tab=="Tips"' in source,
 'Focus-friendly Atlas Tips tab':'MoonfallTipsTab' in source and 'v376_help_tab.pressed.connect' in source,
 'Hover help on existing Atlas actions':'func _atlas_action(' in source and 'V376_TIPS.action_tip(action,label)' in source,
 'Dedicated search field and topic controls':'MoonfallTipsSearch' in source and 'tipcat:' in source,
 'Existing scene and gameplay preserved':active_scene_inherits(r,'native_v37_6.gd') and 'extends "res://scripts/native_v37_4.gd"' in source,
 'No new combat/game modes or save fields':'func _save_game(' not in source and 'new_mode' not in source,
 'Original images unchanged':len(art)>=70 and all(f.is_file() and sha256(f.read_bytes()).hexdigest()==digest for digest,f in art),
 'Engine test in both runners':'res://tests/v37_6_all_tips.gd' in win and "'v37_6_all_tips':" in py,
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
print('%d/%d v37.6 tips checks passed'%(sum(checks.values()),len(checks)))
sys.exit(0 if all(checks.values()) else 1)
