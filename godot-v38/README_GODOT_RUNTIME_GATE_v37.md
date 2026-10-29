# Moonfall v37: strict Godot runtime release gate

**Status:** Offline source verification only. Godot 4.7.2 is not available in the current build environment, so no Godot gameplay test, 4K GPU playthrough or Windows executable has passed here. Gameplay source is v37; the underlying game export/title stays v34.4 until an actual validated release.

## Release blocker corrected

Previously, `BUILD_WINDOWS.bat` checked each Godot test's return code and `PASS` string, but a test that printed `PASS` and later emitted an `ERROR:` or `SCRIPT ERROR` could still let the build through. Now `tests/check_godot_logs.py` checks *all 66 required engine logs*, including import, every one of the 63 tests, Windows export and packed-game startup, before reporting a successful build. Missing logs, duplicated log targets, GDScript parse errors and runtime engine errors block release.

`tests/verify_godot_log_gate.py` runs isolated synthetic log cases proving the gate rejects a misleading PASS banner, missing files, `ERROR:`, `SCRIPT ERROR`, `Parse Error` and `FATAL:`. Both GitHub workflows invoke the checker regression before building; the Windows batch invokes the full log scanner after game export and packed startup.

## Build on Windows 10/11 with GitHub Actions

1. Extract this complete ZIP to a local folder. On a Windows PC with authenticated Git for Windows, run `PUBLISH_NATIVE_TO_GITHUB.bat`.
2. Open the pull request URL printed by the script. The publishing helper creates an isolated branch, copies source to `native/moonfall-godot-v34_4`, and adds the top-level GitHub Windows workflow without overwriting `main` or `native/dx12-prototype`.
3. GitHub Actions Windows runner downloads Godot 4.7.2 and SHA-verified Windows export templates, executes engine tests, creates the portable exe and Inno Setup installer, smoke-tests installation and uninstall, and generates SHA-256 evidence.
4. **Only if the Windows workflow passes** download artifacts named `Moonfall-v34_4-single-exe` and `Moonfall-v34_4-windows-installer`. The file version remains v34.4 despite v37 features being in source. Do not call the installer tested until the workflow succeeds.

## Local checks without the Godot engine

`python tests/verify_godot_log_gate.py`

`python tests/verify_engine_test_parity.py`

`python tests/verify_build_test_manifest.py`

`python tests/verify_v37_living_realm.py`

These checks do not substitute for real engine gameplay. When Godot 4.7.2 is available, use `python tools/run_tests.py --all` to run all 63 registered engine suites.

## Preserved game constraints

The source retains one connected world, no building or survival stopwatch, and the original Z/Nyra art untouched. No new gameplay modes, currencies, enemy caps or character redesigns are introduced by this test-gate patch.
