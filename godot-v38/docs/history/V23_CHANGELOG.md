# Moonfall Native v23: Beast & Guardian Articulation

- Added 4 procedural hip/knee pairs per quadruped, with alternating gait and knee lift.
- Added head pivot animations, owl wing-fan pivots and segmented serpentine sway.
- Added head recoil on successful player hits and phase/windup animated guardian rings.
- Keeps original Z/Nyra and all prior asset bytes unchanged. No hitbox, navigation, drop, AI, or save schema changes.
- High-detail animation uses a 75 ms update cadence, 34-unit distance limit, and Smooth graphics toggles.
- Godot 4.7.2 headless: v23-specific checks + all thirteen older regression suites pass.
- Packed Linux runtime and Windows EXE packaging checked separately; Windows gameplay, GPU performance, controller hardware and installer are not yet verified.
- Visuals are still procedural low-poly geometry, NOT authored realistic models or fully skeletal IK rigs.
