# Windows playtest acceptance checklist

Use a clean Windows 10 or Windows 11 test account or VM. Make a backup of any existing Moonfall save before testing.

1. Run `VERIFY_WINDOWS_RELEASE.ps1` against `MoonfallNative_v30_1.exe` and check it reports PASS; this **does not** install.
2. Open the game EXE and confirm character selection, Z and Nyra sprites, six-region Atlas, movement, combat, pause and resume.
3. Press F9 while in play, then move/fight for 60 seconds. Send the generated JSON FPS report.
4. Check Full and Smooth modes with identical encounters; compare average FPS, p99 frame time, and CPU AI time.
5. On an isolated Windows test machine, run `Moonfall_Native_v30_1_Windows_Setup.exe` (if available), confirm installation under `%LOCALAPPDATA%\Programs\Moonfall Native`, shortcuts and Apps entry, and that uninstaller removes the app. Confirm Godot user:// saves are untouched.
6. Verify controller movement, buttons, and menus on physical hardware (not simulated inputs).

No Windows runtime, installation or GPU tests were performed in the Linux packaging environment.
