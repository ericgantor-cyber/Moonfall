# Moonfall v37.4: Solid Grottos and Swimming Presentation

Built from v37.3 without changing original Z, Nyra, or world imagery.

## What changed
- Fixed a critical physics shortcut: underwater movement no longer disables the CharacterBody3D collision mask.
- Depth ascent/descent now uses Godot `move_and_collide`, then derives oxygen depth from the **actual** collision-constrained player position.
- Six existing grottos gain 13 `StaticBody3D` collision objects each (78 total): bed, ceiling, back and side walls, rock pillars. Three walls per grotto have rendered stone geometry (18 total).
- Depth-aware lake boundary protects against drifting out through the terrain; leaving a lake returns to the surface.
- The 2D hero billboard performs a subtle bob and tilt when submerged. **This is not a faithful 3D character rig or newly painted frames.** All original artwork remains byte-identical.
- Same connected world, no new modes, no building, no survival timer, existing oxygen mechanics.

## Verification
Run `python tests/verify_v37_4_solid_grottos.py`, and use the existing offline verifiers. Engine-level test: `python tools/run_tests.py v37_4_solid_grottos` requires a working Godot 4.7.2. Windows export/installer still need running on a Windows environment.

## Known limitations
The existing movement system still uses an XZ plane plus a collision-tested Y movement, and cave geometry is primitive. Real dynamic swimming forces, depth-specific enemy AI, complex underwater navigation, robust 3D character skeletal swimming, and full Windows builds remain untested/unimplemented. Godot engine unavailable locally, so physics is not yet gameplay-verified.
