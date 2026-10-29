## Current v37.6 source handoff

The active scene loads `scripts/native_v37_6.gd`, extending the complete v37.4 deep-water, combat, original-art, and world gameplay chain. v37.6 adds a **searchable 107-entry Tips Codex** with a focusable Raven Atlas tab, F1 shortcut and Atlas button hover help. The prior v37.4 controls and cave gameplay remain intact.

Both test runners now register **64 Godot suites**. `python tests/verify_v37_6_all_tips.py` provides offline source checks; the runtime test is `tests/v37_6_all_tips.gd`. A passing offline check does not prove the game ran. The Windows build/export has not been executed in this environment. The Windows exported executable/installer name still refers to v34.4 due to prior release metadata and must not be mistaken for feature status.

## v37.4 solid deep-water geometry and swim pose (source-only)

New script `scripts/native_v37_4.gd` is active, layered over the original v37.3 deep-water system. Underwater hero collision is **no longer disabled**; depth is constrained by collision-aware motion, six grottos have 78 persistent static colliders including 18 visible walls, and the original hero sprites receive a subtle procedural swim pose. `tests/v37_4_solid_grottos.gd` is registered in both build runners. **63 Godot suites** and **66 release logs** are now expected. `python tests/verify_v37_4_solid_grottos.py` runs the new offline checks; all 34 offline verification scripts passed when packaged. Actual Godot scene/physics execution and Windows installer compilation are still pending. See `README_SOLID_GROTTOS_v37_4.md`.

## v37.2 oxygen addition (source-only)

This package adds `scripts/native_v37_2.gd`, `scripts/systems/oxygen_v372.gd`, and a Godot integration test `tests/v37_2_oxygen.gd`. The active scene inherits the earlier v37 living-realm script. 61 Godot integration suites are registered in both runners; all 32 offline source verifiers passed locally. Actual Godot runtime, 4K GPU behavior, and Windows `.exe`/installer still have **not** been executed or verified. Details are in `README_OXYGEN_v37_2.md`. No changes to existing source image assets or Z and Nyra.

## v36.3 living-world build note
This source package adds `scripts/native_v36_3.gd` as the active one-world scene, extending the unchanged v34.4 gameplay script. Build and installer metadata remain v34.4 until a Windows export is actually tested. See `README_LIVING_WORLD_v36_3.md`.

# Moonfall Native v34.4: Windows build handoff

This package contains **Godot source code**, not a compiled Windows installer.

## What to do on Windows

1. Extract the ZIP. Install Git for Windows and authenticate your GitHub account.
2. Double-click `PUBLISH_NATIVE_TO_GITHUB.bat`. It clones `ericgantor-cyber/Moonfall` to a temporary directory and creates a **new feature branch**, leaving `main`, the browser game and `native/dx12-prototype` untouched.
3. The publisher adds the project under `native/moonfall-godot-v34_4` and installs the root workflow `.github/workflows/moonfall-native-v34_4.yml`. It retains the *nested* `.github` directory inside the Godot project so its CI/release validation scripts function.
4. Open the pull request URL printed by the script. The new workflow checks the Godot sources, executes gameplay tests, creates a portable EXE, installs/uninstalls the setup on a Windows runner, and publishes EXE and installer artifacts **only if every check passes**.

## Local Windows builds

Use the official Godot 4.7.2 Windows engine at `Godot_4.7.2/Godot_v4.7.2-stable_win64.exe`, then run `MAKE_WINDOWS_INSTALLER.bat` with Inno Setup 6 installed. `COMPILE_INSTALLER_ONLY.bat` compiles the setup if `build/MoonfallNative_v34_4.exe` already exists. Both scripts now use `Moonfall_Native_v34_4_Setup.iss`.

## Verified and unverified

Offline source contracts and all 69 tracked image asset hashes passed; original hero artwork is unchanged. Godot is not available in this execution environment and the Windows build **has not been run**. Headless gameplay tests and Windows installer artifacts remain pending until published to GitHub or run on Windows.

Never add: survival stopwatch, building system, multiple game modes. Keep the entire world connected, preserve Z and Nyra art, and avoid unbounded spawn or effect loops.

## Strict v37 release diagnostics

Read `README_GODOT_RUNTIME_GATE_v37.md` for the new fail-closed engine-log gate, offline tests and the Windows CI handoff. No real Godot runtime or installer was built in this environment.

## v37.9 automatic prerequisites
The one-click Windows launcher now finds or installs Python 3.13 and Inno Setup 6 through WinGet, and fails closed if it cannot build the requested desktop-shortcut installer. See `README_WINDOWS_ONE_CLICK_v37_9.md`. None of the Windows tooling was executed inside this environment.


## v38.1 release readiness
The compatible Windows installer now carries AppVersion `38.0` while preserving
the old on-disk `v34_4` filename so existing build and save paths still work.
The local one-click build has a 60-minute timeout and captures failure evidence;
`COLLECT_BUILD_LOGS.bat` gathers a privacy-conscious local diagnostic ZIP.
**No Windows installer or Godot runtime test was produced in the source workspace.**
