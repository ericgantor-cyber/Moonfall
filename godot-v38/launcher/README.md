# Moonfall Native Windows launcher

The `MoonfallLauncher.ps1` launchpad is packaged with the Windows installer and opened by the default desktop and Start menu shortcuts. It uses Windows PowerShell 5.1 and WinForms already available on supported Windows 10/11 computers; it does not install dependencies, auto-update the game or require administrator access.

- **Play Moonfall:** starts the installed `MoonfallNative_v34_4.exe`. The button is disabled if the game executable is missing. The launcher never claims to validate a file hash.
- **Check Game Files / F5:** checks for the installed game executable and updates the status message.
- **Game Folder:** opens the installation directory.
- **Game Logs:** opens the Godot per-user data parent folder if it exists, or provides a clear message if it does not.
- **Help / Controls:** explains input bindings and how to access camera settings inside the game.
- **Exit / Escape:** closes only the launcher. **Enter** activates the Play button.

The installer also keeps a direct-play Start menu shortcut to the actual game executable in case PowerShell or WinForms is unavailable on a managed PC.

The launcher does not expose or modify hero saves, character graphics, game settings or original Z and Nyra artwork. The in-game Atlas (Esc) owns the advanced settings. The installer and its full Windows smoke tests, not this document, determine whether an actual Windows build is verified.

Run `python tests/verify_launcher_v38.py` from `godot-v38` to validate packaging and source contracts.
