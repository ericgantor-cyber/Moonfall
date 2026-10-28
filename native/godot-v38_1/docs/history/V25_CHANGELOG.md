# Moonfall v25: legacy-code modernization

- Removed repeated legacy HUD-version string replacements from 13 inherited files. A single virtual release label renders **v25**, preserving v14's health/XP bars.
- Cached 72 static diorama nodes for repeated LOD updates; optional decoration still respects Smooth mode and culling.
- Fixed invisible-grid rebuild count accumulation and scaled obstacle-footprint indexing.
- Strengthened schema-2 checkpoint verification to reject nonnumeric/invalid core progression values, retaining old saves and backup recovery.
- Added an engine regression test for release labels, LOD cache, grid rebuilding, and save decoding; the older regression suites remain.
- Updated Godot scene, VS Code test runner, Windows build, GitHub workflow, and Inno Setup recipe for v25.
- All inherited artwork remains byte-for-byte unchanged; there are no art/style changes.

This is an incremental safe refactor, not a rewrite of the entire 2,400-line native_v11.gd gameplay foundation. Windows packaging has not been hardware-tested.

- Repaired the stale `verify_v13.py` contract check as `tests/verify_source.py`, and modernized the alternative installer batch, export preset, and installer version metadata.
- Test runner now accepts optional suite names for targeted tests while retaining the complete default suite.
