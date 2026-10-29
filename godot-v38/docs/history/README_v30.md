# Moonfall Native v30 | Performance and Windows release candidate

This builds on v29. Six connected regions, the hidden 2,400-cell terrain grid, core gameplay, schema-2 saves, Z and Nyra's original artwork, and the 29 inherited images are unchanged.

## Changes implemented
- Creature AI: distribute distant idle wildlife across four update phases based on stable encounter indices. Active combatants, bosses, stagger, fleeing animals, and telegraphed attacks continue to update every physics tick. Keep idle-time accumulation bounded to 0.25s.
- Profiling: expose per-run AI CPU update samples in `v30_diagnostics()` and preserve inherited v29 profiling API for existing tools.
- Accessible camera reset: press keyboard **0** or click the controller **right stick** while playing. Existing WASD, controller, dodge, spell, minimap, and menu controls remain.
- Build hardening: update the Godot scene, release labels, Windows batch build, Inno Setup 6 recipe, GitHub Actions workflow and test runner, and fix stale test references.
- Release packaging: provide a single-file Windows EXE with embedded PCK plus an unsigned experimental Go-based installer containing per-user shortcuts and an uninstaller.

## Verification (Linux Godot 4.7.2 headless)
- 22 tests passed when run as isolated test processes, including original world/save/gear/AI tests and new `tests/v30_release.gd`.
- A packed Linux runtime launched for 300 frames without a reported script error.
- All 29 inherited PNG/WebP image assets hash-identical to v29.
- Windows single-file binary's PE header and Godot PCK footer verified.
- Windows installer payload ZIP content and CRC verified, including a matching Linux build of the Go installer executing `--verify`.

## Limits and scope
Headless Linux testing is NOT a Windows gameplay test or FPS/GPU benchmark. The Windows game uses the Godot 4.7.2 Windows editor runtime with embedded PCK, not a proper lean export-template executable. The **custom test installer is not signed** and its installation, shortcuts, uninstall, controller behavior, and performance are not Windows-tested. A conventional Inno Setup configuration is also included, but no Inno-compiled installer is provided. Do not treat this as a production-ready, certified release.

## How to test on Windows
1. Preferred first playtest: extract `Moonfall_Native_v30_Windows.zip`, then double-click `MoonfallNative_v30.exe`.
2. Optional test of the new installer: run `Moonfall_Native_v30_Windows_Setup.exe` on a Windows 10/11 PC, expecting Windows SmartScreen for unsigned binaries; verify Start Menu and Desktop shortcuts, launch, saves, and uninstall.
3. For source development, open this folder with Godot 4.7.2. Run `python tools/run_tests.py` with `GODOT_BIN` pointing to the Godot 4.7.2 executable.
4. Check `V30_TEST_REPORT.md` for what was and was not verified.

The release does not change GitHub; upload the source to a branch if desired.
