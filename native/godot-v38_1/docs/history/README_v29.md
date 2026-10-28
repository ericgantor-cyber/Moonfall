# Moonfall Native v29 - performance and release hardening

Start: run `MoonfallNative_v29.exe` (64-bit Windows 10/11). This is an unsigned developer playtest build; GPU and controller hardware remain unverified.

Changes: modular v29 creature-update scheduler, 1-in-8-frame AI CPU timing samples for diagnostics, cached frame player position and time, and squared-distance threat checks. A new headless performance integration test runs 96 direct beast update calls and reports timings. It does NOT estimate game FPS.

**Windows installer:** run `BUILD_WINDOWS.bat` from the extracted source with the official Godot 4.7.2 Windows executable in `Godot_4.7.2/`. It runs regression tests, creates the packed single-file EXE, then `MAKE_WINDOWS_INSTALLER.bat` compiles the Inno Setup installer if Inno Setup 6 is installed. A GitHub Actions workflow is included, but no Windows environment installer validation was performed here.

All original artwork including Z and Nyra is unchanged. Save schema 2 is unchanged. Back up your save before switching builds.
