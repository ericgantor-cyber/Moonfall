# Moonfall v37.7: Game-wide Playability and Underwater Recovery

This is a source update, **not** a compiled Windows executable or an engine-tested release.

## Improvements
- The hero-select title no longer incorrectly advertises a v13 isometric slice; original character portraits stay untouched.
- Both Windows CI definitions fail immediately if either Tips or v37.7 verification fails.
- The Raven Atlas now shows a one-shot **RIGHT NOW** summary (region, current health, oxygen and depth) on World and System pages and at the top of the Tips Codex. No recurring UI work per frame.
- Context tips cover underwater oxygen, low health, combat danger and safe exploration.
- **EMERGENCY SURFACE** appears on System/World only while underwater and out of combat. It moves the hero to a fixed dry shoreline within the same region and restores the air/collision state. It cannot be used during combat, while mounted, on the title screen or while dead. No supplies or gold are changed, and it is not a new fast-travel mode.
- Low-air guidance now correctly tells the player to reach the surface (releasing a dive control while still submerged does not restore oxygen).
- F1 Tips, saved progress, invasion logic, crafting, water exploration and original hero art are preserved.

## Verification
- `python tests/verify_v37_7_playability.py` validates source changes, test manifest and artwork integrity.
- `python tools/run_tests.py --all` runs the registered Godot gameplay tests when Godot 4.7.2 is installed and `GODOT_BIN` is set.
- `BUILD_WINDOWS.bat` must pass the engine suite and strict log gate before exporting.

## Pending
Actual Godot gameplay, collision and GPU benchmarking, 4K tests, and a tested Windows installer. The older executable naming remains v34.4 until release compatibility is migrated and verified.
