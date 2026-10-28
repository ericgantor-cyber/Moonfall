## v38.1 release-readiness update (v38.0 gameplay)

Start on Windows by double-clicking `BUILD_MOONFALL_ONE_CLICK.bat`. The release gate
now times out hung Godot builds, fails closed on partial outputs, and can create a
diagnostic ZIP with `COLLECT_BUILD_LOGS.bat`. See `README_V38_1_RELEASE_GATE.md`.

**This source ZIP is not a compiled or gameplay-tested Windows executable.**

# Moonfall v38.0: Living Realm Stability & Accessibility Overhaul

This is **Godot 4.7.2 source** with Z and Nyra's original artwork intact, **not a prebuilt Windows executable**.

## Changes
- Swimming pose uses a separate `Node3D` anchor. Attack and dodge sprite animations no longer share the same transform with the swim presentation, and pose scale resets on leaving water.
- Press **F2** in gameplay for a compact, optional live readout of region, HP, depth, oxygen and danger. The panel starts **off** and updates at most 4 times per second.
- Critical air and combat/low-health status messages use one shared pure helper (`scripts/systems/overhaul_v380.gd`); no new timers, currencies or game modes.
- One-click Windows pipeline retains automatic Python 3.13, Inno Setup 6 and SHA-verified Godot setup, then 40 offline verifiers and **66 registered Godot runtime suites**. Actual Windows build is still pending.

## Build on Windows 10/11
Extract the ZIP and double-click `BUILD_MOONFALL_ONE_CLICK.bat`. First run requires WinGet and internet access. Read `README_WINDOWS_ONE_CLICK_v37_9.md` for installed prerequisites and `README_BUILD_HANDOFF.md` for limitations. Only call it a verified playable release once Godot tests, Windows export, installer install/uninstall and hands-on combat/controller tests succeed.

## Test locally
Run `python tests/verify_v38_overhaul.py` and `python tests/verify_engine_test_parity.py`. For all offline checks run all `tests/verify_*.py`. On a machine with Godot 4.7.2 set `GODOT_BIN` and run `python tools/run_tests.py --all`. The full engine suites have not been executed in this environment.

## Project rules
**One connected world**. No extra modes, building system, survival stopwatch, or alteration of any Z/Nyra original art. Existing save data and the original item currencies remain intact.
