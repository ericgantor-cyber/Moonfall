# Moonfall v38.1 release gate: test report

## Delivered source changes
- Fixed CI fail-open bug: the v37.8 offline verifier now has an immediate nonzero exit guard in both Windows GitHub workflows.
- Added a 60-minute watchdog around BUILD_WINDOWS.bat in the local one-click launcher, with child-process cleanup when stuck.
- Removed stale, partially built EXE/installer outputs and success status on failure; wrote a distinct failure report.
- Changed Inno Setup AppVersion from 34.4 to actual v38.0 gameplay version, preserving legacy binary filenames and installer paths to avoid breaking build scripts.
- Added opt-in COLLECT_BUILD_LOGS.bat/ps1 to collect only Godot logs and build reports, not saves or credentials.
- Updated earlier source verifiers to enforce the corrected current release version and file names.

## Verifications completed here
- **41/41** independent Python offline regression verifiers passed, including the new v38.1 release gate verifier.
- **13/13** detailed v38.1 release gate assertions passed.
- **66** existing Godot engine test suites remain registered in the Windows and Python runners. Their actual engine execution is not claimed.
- **70 original art files** preserved byte-for-byte, verified against SHA-256 manifest.
- ZIP integrity check performed after packaging.

## Not verified / next steps
- Godot 4.7.2 is not installed and external downloads are blocked in this Linux workspace. Thus engine import, Godot runtime tests, Windows game EXE export, Windows installer compilation, actual shortcut checks, gamepad and GPU 4K tests **were not executed here**.
- For a Windows build, extract this ZIP and run BUILD_MOONFALL_ONE_CLICK.bat. If it fails, run COLLECT_BUILD_LOGS.bat, then review the ZIP in build before sharing it.
- The Godot source project has not been pushed to GitHub in this turn. Its CI cannot build this source until it is published in the dedicated native path.
- All existing design constraints hold: one connected world, no building mode, no general survival timer, no edits to Z/Nyra artwork.
