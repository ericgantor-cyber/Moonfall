#!/usr/bin/env python3
from active_scene_guard import active_scene_inherits
"""Offline source contract, NOT a substitute for running the Godot engine."""
from pathlib import Path
import hashlib,sys
r=Path(__file__).resolve().parents[1]
g=(r/'scripts/native_v34_4.gd').read_text()
b=(r/'scripts/native_v11.gd').read_text()
h=(r/'scripts/systems/third_person_camera_v36.gd').read_text()
runner=(r/'tools/run_tests.py').read_text()
bat=(r/'BUILD_WINDOWS.bat').read_text()
checks={
 'Perspective camera': 'Camera3D.PROJECTION_PERSPECTIVE' in g and 'camera.fov=v36_fov' in g,
 'Camera preferences persist': 'V36_CAMERA_PREFS' in g and 'config.save(V36_CAMERA_PREFS)' in g and 'config.load(V36_CAMERA_PREFS)' in g,
 'Camera sensitivity presets': 'KEY_F6' in g and 'V36_SENSITIVITIES' in g,
 'Vertical inversion': 'KEY_F7' in g and 'v36_invert_y=not v36_invert_y' in g,
 'Horizontal inversion': 'KEY_F8' in g and 'v36_invert_x=not v36_invert_x' in g,
 'Camera recenter': 'KEY_F9' in g and '_v36_recenter_camera()' in g,
 'Wide FOV presets': 'KEY_F11' in g and 'V36_FOVS' in g,
 'Keyboard camera zoom': 'KEY_PAGEUP' in g and 'KEY_PAGEDOWN' in g,
 'Precise and fast wheel zoom': 'event.shift_pressed' in g and 'event.ctrl_pressed' in g,
 'Mouse and controller share sensitivity': g.count('v36_orbit_sensitivity*(-1.0')>=4,
 'Camera input unavailable while resting': 'not wait_open and not wait_transition_active' in g,
 'Camera movement hook': '_map_movement_to_camera((dir+_gamepad_direction())' in b and 'func _map_movement_to_camera(raw_direction:Vector3)' in g,
 'Right-drag and controller look': 'MOUSE_BUTTON_RIGHT' in g and 'JOY_AXIS_RIGHT_X' in g,
 'Wall avoidance and follow damping': 'intersect_ray(ray)' in g and 'exp(-9.0' in g,
 'Orbit/zoom bounds': 'MAX_PITCH' in h and 'MAX_DISTANCE' in h and 'MIN_DISTANCE' in h,
 'Godot test in both launchers': 'v36_0_third_person_3d' in runner and 'v36_0_third_person_3d.gd' in bat,
 'Original hero images unchanged': all((r/rel).is_file() and hashlib.sha256((r/rel).read_bytes()).hexdigest()==digest for line in (r/'docs/ART_SHA256_V30_1_BASELINE.txt').read_text().splitlines() if line.strip() for digest,rel in [line.split('  ',1)]),
 'No alternate game mode': active_scene_inherits(r, 'native_v34_4.gd'),
}
for label,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+label)
sys.exit(0 if all(checks.values()) else 1)
