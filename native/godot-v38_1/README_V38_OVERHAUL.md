# v38 Full Stability Pass

### Actual code modifications
1. **Swimming pose independence**: `scripts/native_v37_4.gd` now wraps the original Sprite3D in a zero-centered `Node3D` pose anchor. Parent rotation/position/scale are absolute (not additive); the original v16 attack/dodge sprite transforms continue untouched. Anchor transforms reset when not diving.
2. **Accessible status**: `scripts/native_v38_0.gd` has F2 toggle, no permanent HUD clutter, context-aware health/oxygen/region readout capped at 4 Hz and hidden during menus and death. `scripts/systems/overhaul_v380.gd` keeps pure rules isolated.
3. **Release verifiers**: `tests/v38_0_overhaul.gd` added to both runner manifests and strict log scanner. Existing offline checks remain; revised older contracts test the improved pose behavior rather than expecting the prior direct edits.
4. **Documentation bloat**: historical Markdown guides and change logs moved to a separate optional history ZIP, without deleting any gameplay script, engine test, art file or required build script.

### Actual verification
Offline static tests, ZIP integrity and SHA-256 character art verification will be recorded in `TEST_REPORT_v38.md`. Godot tests, Windows export and installer are **not run here**; do not interpret source checks as proof of runtime behavior, 4K FPS or game-playability. Retain the existing v34.4-named exe/installer artifacts for compatibility with Windows build and scripts, but gameplay source in v38.0 is newer.
