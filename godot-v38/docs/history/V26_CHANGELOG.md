# Moonfall Native v26: Modular Core

## Completed
- Extracted checkpoint decoding, staged writes, backup/rollback logic into `scripts/systems/checkpoint_store.gd`.
- Extracted inventory/equipment sanitization into `scripts/systems/gear_codec.gd`.
- Extracted damage multipliers and region power scaling into `scripts/systems/hero_rules.gd`.
- Retained old method names as compatibility wrappers in `native_v11.gd`; no new save schema or collision changes.
- Added `v26_modular_core.gd` regression checks covering modules, rejected data, saves and class delegation.
- Updated Godot project, test runner, Windows build and installer source references for v26.

## Not changed
- No new game modes, region rewrites, or character art edits.
- Older monolithic world-generation/AI paths remain in `native_v11.gd` for later incremental extraction.

## Validation limitations
- Headless engine tests verify logic, not real Windows graphics, gamepad hardware, or installer runtime.
