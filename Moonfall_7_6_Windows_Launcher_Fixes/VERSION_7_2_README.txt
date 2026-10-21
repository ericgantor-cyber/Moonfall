Moonfall 7.2: Windows 10/11 launcher and save-origin bugfix

FIXED: The launcher previously chose a new random localhost port each run.
Browser localStorage is scoped to origin (including port), so saves, permanent
upgrades, settings and keybinds could appear lost after every restart.
The launcher now uses 127.0.0.1:47831 consistently across restarts.

LIMITATION: Existing saves created by older random-port launchers cannot be
automatically migrated, because the old port varies between runs.
If another program occupies port 47831, Moonfall will display a clear error
instead of silently switching ports and losing access to saves.

SECURITY: Local server only accepts the intended loopback Host header.

BUILD: Upload extracted project to GitHub and run the Windows installer
workflow in .github/workflows/build-windows.yml, or on Windows run
Build-Moonfall-Windows.bat (Node.js LTS required for local build).

This archive is source/build kit, NOT a compiled Windows installer.
Windows 10/11 execution and GPU gameplay have not been tested here.
Z and Nyra artwork has not been modified.
