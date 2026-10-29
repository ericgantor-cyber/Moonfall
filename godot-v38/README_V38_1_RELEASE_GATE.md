# Moonfall v38.1: Playable Release Gate

**Build tooling update only.** Gameplay remains v38.0. Z and Nyra source artwork has not been changed.

Double-click **BUILD_MOONFALL_ONE_CLICK.bat** on Windows 10/11 with WinGet, an internet connection for the initial engine and prerequisites downloads, and sufficient disk space. The script checks Python 3, Inno Setup 6, SHA-256 verified Godot, exports and runtime test suites.

## What is different

- The Windows 3D game and installer are generated only after the offline source gates and Godot test suites pass; source checks alone cannot make a playable release.
- A Godot build that hangs is stopped after **60 minutes**, including its child processes. A successful build cannot be claimed from a stuck or timed-out run.
- On failure, incomplete new `.exe` and installer files and any success status are removed. The failure reason is written to `build/BUILD_FAILURE_v38_1.txt`.
- The optional `COLLECT_BUILD_LOGS.bat` creates `build/Moonfall_v38_1_Build_Diagnostics.zip` for troubleshooting. It gathers only relevant engine logs and release status, never game saves or passwords. Review logs before sharing because local paths may appear.
- Both GitHub CI workflows now fail immediately after **every** offline verification, including the previously unchecked v37.8 verification.
- The installer now reports **game version 38.0**, although its internal executable filename still contains `v34_4` for compatibility with scripts and existing save paths. That older filename does not mean the gameplay is v34.4.
- An actual Windows CI workflow includes a headless launch, silent install, shortcut check, and uninstall test. The local build script does not claim a human tested combat, controller, or 4K GPU performance.

## Status

This source was assembled and offline-verified in a Linux workspace. No Godot 4.7.2 executable, Windows engine tests, Windows executable, or installer were available or run here. The official Godot download was unreachable from this workspace. **Do not call this a playable build unless the Windows build succeeds.**
