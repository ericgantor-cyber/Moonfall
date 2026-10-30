#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
rules=(root/'scripts/systems/graphics_rules.gd').read_text()
script=(root/'scripts/native_v30_9.gd').read_text()
project=(root/'project.godot').read_text()
build=(root/'BUILD_WINDOWS.bat').read_text()
checks={
 "4K is selectable": '"3840x2160"' in rules,
 "Existing resolutions retained": all(f'"{r}"' in rules for r in ('1280x800','1920x1080','2560x1440')),
 "Resolution window restored at launch": '_gfx_collect_scatter(world)\n # Restore saved window size/mode on launch; headless checks remain display-free.\n _gfx_apply()' in script,
 "4K clamped to monitor usable size": 'screen_get_usable_rect' in script and 'mini(requested.x,usable.size.x)' in script,
 "UI canvas scaled": 'window/stretch/mode="canvas_items"' in project,
 "16:9 and 16:10 aspect allowed": 'window/stretch/aspect="expand"' in project,
 "3D render scaling retained": 'vp.scaling_3d_scale' in script,
 "Test registered in build": 'res://tests/v34_4_4k_display.gd' in build and 'MOONFALL 4K DISPLAY PASS' in build,
 "Godot test exists": (root/'tests/v34_4_4k_display.gd').is_file(),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
sys.exit(0 if all(checks.values()) else 1)
