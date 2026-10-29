# Moonfall Native v11 Offline Validation

Result: PASS for offline source contracts, project-path consistency, Windows build-script references, ZIP integrity and byte-for-byte hero art checks.

**Godot parser/import, scene smoke test, gameplay, Windows export and installer were NOT run** because the Godot engine and Windows toolchain were unavailable here.

## Original artwork SHA-256 (unchanged from v10)

- `Z_Original.webp`: `81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666`
- `Nyra_Original.png`: `64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9`

## Regression checks to run in Godot on Windows

`VERIFY_GODOT_PROJECT.bat` runs import and `tests/smoke_v11.gd`; `BUILD_WINDOWS.bat` builds a Windows .exe only if verification passes. `MAKE_WINDOWS_INSTALLER.bat` optionally packages Inno Setup. See the FIRST_PLAYTEST_CHECKLIST.txt.

The static checks do not certify a bug-free or release-ready game.
